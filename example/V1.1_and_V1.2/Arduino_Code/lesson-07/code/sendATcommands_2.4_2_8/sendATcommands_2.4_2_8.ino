#include "config.h"
#include <Arduino.h>
#include "At_Config.hpp"

/*---------------------------------------------------------------
 * Serial AT command buffer
 * Incoming serial bytes are collected here before the AT parser
 * checks the command name, parameters, and action flags.
 *--------------------------------------------------------------*/
uint8_t cmd_data_buf[244] = { 0 };

// Stores the number of valid bytes currently held in cmd_data_buf.
uint8_t cmd_data_size = 0;

/**
 * @brief Initialize serial AT control and the LoRa radio.
 *
 * Arduino calls this function once after reset. It enables the wireless
 * module path, starts the SPI bus, initializes the AT command parser, and
 * prepares the RadioLib radio object for later LoRaWAN operations.
 *
 * @param None.
 * @return None.
 */
void setup() {
  Serial.begin(115200);

  /*---------------------------------------------------------------
   * Enable the wireless module path
   * GPIO45 selects the wireless module instead of the alternate audio
   * path on this board.
   *--------------------------------------------------------------*/
  pinMode(45, OUTPUT);
  digitalWrite(45, LOW);
  
  while (!Serial)
    ;
  delay(2000);
  SPI.begin(HSPI_SCLK, HSPI_MISO, HSPI_MOSI, HSPI_SS);

  at_config.begin();
  int16_t state = 0;
  state = radio.begin();
  debug(state != RADIOLIB_ERR_NONE, F("Initialise radio failed"), state, false);
  radio.setCurrentLimit(140.0);
  radio.setTCXO(3.3);
  Serial.println(F("\nAT Begin"));
}

/**
 * @brief Receive AT commands and apply pending parameter changes.
 *
 * Arduino calls this function repeatedly after setup() finishes. Serial input
 * is parsed first, then ParameterSetting() consumes the flags raised by the
 * parser so joining, sending, and radio parameter changes happen in the main
 * control loop.
 *
 * @param None.
 * @return None.
 */
void loop() {
  if (at_config.receiveSerialCmd(cmd_data_buf, &cmd_data_size)) {
    at_config.parseCmd((char *)cmd_data_buf, cmd_data_size);

    memset(cmd_data_buf, 0, cmd_data_size);
    cmd_data_size = 0;
  }
  ParameterSetting();
}

/**
 * @brief Apply LoRaWAN actions requested by AT commands.
 *
 * AT command callbacks update global flags and parameter structures. This
 * function checks those flags, performs the requested LoRaWAN operation, and
 * clears each flag so the action runs once per command.
 *
 * @param None.
 * @return None.
 */
void ParameterSetting() {
  /*---------------------------------------------------------------
   * Join by ABP
   * ABP uses stored session keys directly, so it can activate without
   * the OTAA join exchange.
   *--------------------------------------------------------------*/
  if (ABP_Join_Flag == 1) {
    ABP_Join_Flag = 0;
    Serial.println("");
    Serial.println("Join ABP");
    int16_t state = 0;
    uint8_t joinDR = 0;

    node.beginABP(app_param.lora_info.DevAddr, NULL, NULL, app_param.lora_info.NwkSKey, app_param.lora_info.AppSKey);
    node.activateABP(joinDR);
    debug(state != RADIOLIB_ERR_NONE, F("Activate ABP failed"), state, false);
  }

  /*---------------------------------------------------------------
   * Join by OTAA
   * OTAA performs a network join, then applies runtime parameters such
   * as ADR, data rate, duty cycle, and dwell time.
   *--------------------------------------------------------------*/
  if (OTAA_Join_Flag == 1) {
    OTAA_Join_Flag = 0;
    Serial.println("");
    Serial.println("Join OTAA");
    int16_t state = 0;

    uint8_t joinDR = 0;
    if (app_param.lora_info.ActiveRegion == 0) {
      joinDR = 2;
    } else {
      joinDR = 0;
    }

    node.beginOTAA(app_param.lora_info.JoinEui, app_param.lora_info.DevEui, NULL, app_param.lora_info.AppKey);
    Serial.println("Join ('login') the LoRaWAN Network");
    state = node.activateOTAA(joinDR);
    debug(state != RADIOLIB_LORAWAN_NEW_SESSION, F("Join failed"), state, false);
    Serial.print("[LoRaWAN] DevAddr: ");
    Serial.println((unsigned long)node.getDevAddr(), HEX);
    if (app_param.lora_info.ADR == 1) {
      node.setADR(true);
    } else {
      node.setADR(false);
    }
    node.setDatarate(app_param.lora_info.DR);
    if (app_param.lora_info.dutyCycleEnabled == 1) {
      node.setDutyCycle(true, app_param.lora_info.msPerHour);
    } else {
      node.setDutyCycle(false, app_param.lora_info.msPerHour);
    }
    node.setDwellTime(true, 400);
  }

  /*---------------------------------------------------------------
   * Send an uplink message
   * The send path differs by activation type, but both paths report
   * whether a downlink was received after the uplink.
   *--------------------------------------------------------------*/
  if (Send_Flag == 1) {
    Send_Flag = 0;
    if (app_param.lora_info.ActivationType == 1)
    {
      Serial.println("Send ABP OK");
      int state = node.sendReceive(MsgTemp, MsgTemp_size);
      debug(state < RADIOLIB_ERR_NONE, F("Error in sendReceive"), state, false);

      if (state > 0) {
        Serial.println(F("Received a downlink"));
      } else {
        Serial.println(F("No downlink received"));
      }
    }
    if (app_param.lora_info.ActivationType == 2)
    {
      int16_t state = RADIOLIB_ERR_NONE;
      Serial.println("Send OTAA OK");
      uint8_t downlinkPayload[10];
      size_t downlinkSize;
      LoRaWANEvent_t uplinkDetails;
      LoRaWANEvent_t downlinkDetails;
      if (Ack_State == 1) {
        Serial.println(F("and requesting LinkCheck and DeviceTime"));
        node.sendMacCommandReq(RADIOLIB_LORAWAN_MAC_LINK_CHECK);
        node.sendMacCommandReq(RADIOLIB_LORAWAN_MAC_DEVICE_TIME);

        state = node.sendReceive(MsgTemp, MsgTemp_size, Port_Number, downlinkPayload, &downlinkSize, true, &uplinkDetails, &downlinkDetails);
      } else {

        state = node.sendReceive(MsgTemp, MsgTemp_size, Port_Number, downlinkPayload, &downlinkSize, true, &uplinkDetails, &downlinkDetails);
      }
      debug(state < RADIOLIB_ERR_NONE, F("Error in sendReceive"), state, false);
      if (state > 0) {
        Serial.println(F("Received a downlink"));
        if (downlinkSize > 0) {
          Serial.println(F("Downlink data: "));
          arrayDump(downlinkPayload, downlinkSize);
        } else {
          Serial.println(F("<MAC commands only>"));
        }
      } else {
        Serial.println(F("[LoRaWAN] No downlink received"));
      }
    }
  }

  /*---------------------------------------------------------------
   * Apply radio parameter updates
   * These flags allow AT commands to change individual LoRaWAN settings
   * without forcing a full reinitialization of the sketch.
   *--------------------------------------------------------------*/
  if (ADR_set_Flag == 1) {
    ADR_set_Flag = 0;
    if (app_param.lora_info.ADR == 1) {
      node.setADR(true);
    } else {
      node.setADR(false);
    }
  }
  if (DR_set_Flag == 1) {
    DR_set_Flag = 0;
    node.setDatarate(app_param.lora_info.DR);
  }
  if (DCS_set_Flag == 1) {
    DCS_set_Flag = 0;
    if (app_param.lora_info.dutyCycleEnabled) {
      node.setDutyCycle(true, app_param.lora_info.msPerHour);
    } else {
      node.setDutyCycle(false, app_param.lora_info.msPerHour);
    }
  }
  if (TXP_set_Flag == 1) {
    TXP_set_Flag = 0;
    node.setTxPower(app_param.lora_info.txPower);
  }
  if (RX2DR_set_Flag == 1) {
    RX2DR_set_Flag = 0;
    node.setRx2Dr(app_param.lora_info.rx2dr);
  }
  if (Band_set_Flag == 1) {
    Band_set_Flag = 0;
    if (app_param.lora_info.ActiveRegion == 0) {
      Serial.println(F("Set Regional Choices EU868"));
      node = LoRaWANNode(&radio, &Region_868, subBand);
    } else if (app_param.lora_info.ActiveRegion == 1) {
      Serial.println(F("Set Regional Choices US915"));
      node = LoRaWANNode(&radio, &Region_915, subBand);
    }
  }
}
