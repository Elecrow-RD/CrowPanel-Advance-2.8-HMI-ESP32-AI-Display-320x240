#include <WiFi.h>              // WiFi library for connecting and managing WiFi networks
#include <WiFiManager.h>       // Used to simplify the WiFi configuration process
#include <WebSocketsClient.h>  // It is used to implement WebSocket client functions
#include <base64.h>            // Used for Base64 encoding and decoding
#include <ArduinoJson.h>       // Used to process JSON data

// Include the I2S driver library, which provides functions and structures
// for interacting with the Inter - IC Sound (I2S) interface on the ESP32.
// The I2S interface is commonly used for audio input and output operations,
// such as connecting microphones and speakers.
#include <driver/i2s.h>

// Include the Wire library, which is used for communicating with I2C (Inter - Integrated Circuit)
// devices. I2C is a serial communication protocol that allows multiple devices to be connected
// to the same bus, enabling communication between the microcontroller and various sensors,
// expanders, and other peripherals.
#include <Wire.h>

// Include the TCA9534 library. The TCA9534 is an I2C - based 8 - bit I/O expander.
// This library provides functions to control the input and output pins of the TCA9534,
// allowing you to expand the number of available GPIO (General - Purpose Input/Output) pins
// on your microcontroller.
#include <TCA9534.h>

// Include the SPI library, which is used for communicating with Serial Peripheral Interface (SPI)
// devices. SPI is a synchronous serial communication protocol that provides high - speed data
// transfer between the microcontroller and external devices, such as SD cards, displays, and sensors.
#include <SPI.h>

// #include <lvgl.h>
#include "LGFX_Setup.h"  // Display driver configuration
#include "img.h"         // Hexadecimal array of the bottom image of the display
#include "listen.h"      // The hexadecimal array of the icon when the microphone is recording.
#include "speak.h"       // The hexadecimal array of the icon when the speaker is playing.
#include <Button.h>

#define BUTTON_PIN 0        // Button connected to pin 0
Button button(BUTTON_PIN);  // Create a Button object

// Variables needed to create the display
LGFX TFT;
LGFX_Sprite sprite(&TFT);

// Custom initialization variables
const char* websocket_server = "192.168.50.66";  // Server ip address

const int websocket_port = 8765;
const char* websocket_path = "/";
String macAddress;           // Define global variables
WebSocketsClient webSocket;  // Define the WebSocket client
// I2S configuration
#define I2S_MIC_NUM I2S_NUM_0
#define I2S_SPK_NUM I2S_NUM_1
#define SAMPLE_RATE 16000  // Sampling rate
#define SAMPLE_SIZE 1024   // Number of samples per transfer



bool isOpenMic = false;                        // Initialize the definition of microphone on
bool isOpenSpk = false;                        // Initialize the definition of microphone off
unsigned long isOpenMic_true_time = millis();  // Current time when the microphone is turned on

// I2S microphone configuration structure
const i2s_config_t i2s_config_mic = {
  .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),  // Master mode and receive mod
  .sample_rate = SAMPLE_RATE,                           // Sampling rate
  .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,         // Number of bits per sample
  .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,          // Only use the left channel
  .communication_format = I2S_COMM_FORMAT_I2S,          // Communication format
  .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,             // Interrupt allocation flag
  .dma_buf_count = 8,                                   // Number of DMA buffer counts
  .dma_buf_len = SAMPLE_SIZE,                           // Length of the DMA buffer
  .use_apll = false,                                    // Do not use the APLL (Audio PLL)
  .tx_desc_auto_clear = false,                          // Do not automatically clear the transmit descriptor
  .fixed_mclk = 0                                       // Fixed MCLK (Master Clock) frequency
};

// I2S microphone pin configuration structure
const i2s_pin_config_t pin_config_mic = {
  .bck_io_num = 9,                    // The pin number of the bit clock (*CLK) pin. For 7.0-inch, 5.0-inch, and 4.3-inch devices, it is 19.
  .ws_io_num = 3,                     // The pin number of the word select (*WS) pin. For 7.0-inch, 5.0-inch, and 4.3-inch devices, it is 2.
  .data_out_num = I2S_PIN_NO_CHANGE,  // The data output pin number remains unchanged.
  .data_in_num = 10                   // The pin number of the data input pin. For 7.0-inch, 5.0-inch, and 4.3-inch devices, it is 20.
};

// I2S speaker configuration
const i2s_config_t i2s_config_spk = {
  .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
  .sample_rate = 12000,  // Set it according to your audio data format.
  .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
  .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
  .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
  .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
  .dma_buf_count = 8,
  .dma_buf_len = 1024,
  .use_apll = false,
  .tx_desc_auto_clear = true,
  .fixed_mclk = 0
};

// I2S speaker pin configuration
const i2s_pin_config_t pin_config_spk = {
  .bck_io_num = 13,    //*BCLK,For 7.0-inch, 5.0-inch, and 4.3-inch devices, it is 5.
  .ws_io_num = 11,     //*LRCLK,For 7.0-inch, 5.0-inch, and 4.3-inch devices, it is 6.
  .data_out_num = 12,  //*DATA,For 7.0-inch, 5.0-inch, and 4.3-inch devices, it is 4.
  .data_in_num = I2S_PIN_NO_CHANGE
};

//-----------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);  // Initialize the serial port
  wifiServer();          // Connect to the Wi-Fi network
  connect_ws();          // Connect to the WebSocket server
  init_screen();         // Initialize the screen
  init_MIC();            // Initialize the microphone
  init_SPK();            // Initialize the speaker
  button.begin();        // Initialize button
  pinMode(21, OUTPUT);   // Sound shutdown pin
  pinMode(14, OUTPUT);   // MUTE pin
  digitalWrite(14, LOW);
  digitalWrite(21, LOW);
}

void loop() {
  buttonWakeUpTask();
  change_status();
  webSocket.loop();     // Start the WebSocket event loop
  record_send_audio();  // Record and send audio
}

//----------------------------------------------------------------------------- Screen display ----------------------------------------------------------
// Function to initialize the screen and related components
void init_screen() {

  TFT.init();
  pinMode(38, OUTPUT);
  digitalWrite(38, HIGH);     //enable the backlight
  TFT.fillScreen(TFT_BLACK);  // Clear the screen to black
  TFT.setRotation(1);         // This display orientation is correct. Set it to 1.
  TFT.setSwapBytes(true);

  Serial.println("Setup done");
}
//----------------------------------------------------------------------------- Screen display ----------------------------------------------------------

// Button wake-up event
void buttonWakeUpTask() {
  static unsigned long interruptStartTime = 0;  // Record interrupt start time
  static bool waitingToOpenMic = false;         // Mark whether you are waiting to turn on the microphone

  button.read();  // Read button status

  if (button.pressed()) {
    if (isOpenSpk && !isOpenMic) {
      Serial.print("Interrupt!");
      isOpenSpk = false;
      interruptStartTime = millis();  // Record current time
      waitingToOpenMic = true;        // Flag start wait
      String jsonString_i = createJsonString("interrupt_audio", "");
      webSocket.sendTXT(jsonString_i);

    } else if (!isOpenSpk && !isOpenMic) {
      Serial.print("Wake up!");
      // Send wake up signal
      String jsonString_i = createJsonString("wake_up", "");
      webSocket.sendTXT(jsonString_i);
      isOpenSpk = true;
    }
  }

  // Check if you have waited 1 second
  if (waitingToOpenMic && (millis() - interruptStartTime >= 1000)) {
    isOpenMic = true;
    isOpenMic_true_time = millis();
    waitingToOpenMic = false;  // Cancel wait
  }
}


// * Connect to the Wi-Fi network
void wifiServer() {
  WiFiManager manager;
  // Get the MAC address of the ESP and assign it to the global variable
  macAddress = WiFi.macAddress();
  // Remove the colons
  macAddress.replace(":", "");
  Serial.print("The address is:");
  Serial.println(macAddress);
  // manager.resetSettings();  // Reset the WiFi settings
  String APName = "AI-" + macAddress;
  manager.autoConnect(APName.c_str());
  // Connect to the Wi-Fi
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("WiFi is connected");
}

// * Connect to the WebSocket server
void connect_ws() {
  webSocket.begin(websocket_server, websocket_port, websocket_path);
  webSocket.onEvent(webSocketEvent);
}

// * WebSocket event handling function
void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.println("WebSocket is disconnected");
      break;
    case WStype_CONNECTED:
      Serial.println("WebSocket is connected");
      // Play the opening remarks
      {
        String jsonString = createJsonString("open_word", "");
        webSocket.sendTXT(jsonString);
        Serial.println("WebSocket is connected");
        Serial.println("Start, ready to record");
        isOpenMic = true;
        isOpenMic_true_time = millis();
      }
      break;
    case WStype_TEXT:
      Serial.printf("Received text data: %s\n", payload);
      // If the received data is "close_mic", turn off the recording microphone and turn on the speaker
      if (String((char*)payload) == "close_mic") {
        isOpenMic = false;
        Serial.println("Turn off the microphone");
        isOpenSpk = true;
        Serial.println("Turn on the speaker");
      }
      // If the received data is "finish_tts", turn off the speaker and notify the server to start a new process_audio coroutine object
      else if (String((char*)payload) == "finish_tts") {
        delay(1000);
        String jsonString = createJsonString("re_process_audio", "");
        webSocket.sendTXT(jsonString);
        Serial.println("Start ASR again, ready to record");
        isOpenSpk = false;
        isOpenMic = true;
        isOpenMic_true_time = millis();
      } else {
        String rcv_word = (char*)payload;
        Serial.printf(rcv_word.c_str());
      }
      break;
    case WStype_BIN:
      size_t bytes_written;
      Serial.printf("Received binary data, length: %d\n", length);

      if (isOpenSpk) {
        i2s_write(I2S_SPK_NUM, payload, length, &bytes_written, portMAX_DELAY);
      }
      break;
  }
}

// * Initialize the I2S microphone
void init_MIC() {
  i2s_driver_install(I2S_MIC_NUM, &i2s_config_mic, 0, NULL);
  i2s_set_pin(I2S_MIC_NUM, &pin_config_mic);
  i2s_zero_dma_buffer(I2S_MIC_NUM);
}

// * Initialize the I2S speaker
void init_SPK() {
  i2s_driver_install(I2S_SPK_NUM, &i2s_config_spk, 0, NULL);
  i2s_set_pin(I2S_SPK_NUM, &pin_config_spk);
  i2s_zero_dma_buffer(I2S_SPK_NUM);
}

// Record and send audio
void record_send_audio() {
  static unsigned long lastTime = 0;
  static unsigned long lastVoiceTime = 0;
  static bool isVoiceActive = false;  // Flag: whether there is voice
  unsigned long currentTime = millis();

  if (isOpenMic && millis() - isOpenMic_true_time > 10000 && !isVoiceActive) {
    Serial.print("No voice for 10 seconds, enter sleep mode. Press the boot button to wake up.");
    isOpenMic = false;
    String jsonString = createJsonString("timeout_no_stream", "");
    webSocket.sendTXT(jsonString);
  }

  // Check if it's time for the next sampling interval
  if (currentTime - lastTime >= (1000 / SAMPLE_RATE * SAMPLE_SIZE)) {
    lastTime = currentTime;

    size_t bytesRead;
    uint8_t i2sData[SAMPLE_SIZE * 2] = { 0 };  // 16-bit samples, 2 bytes per sample
    i2s_read(I2S_MIC_NUM, i2sData, SAMPLE_SIZE * 2, &bytesRead, portMAX_DELAY);

    if (bytesRead > 0 && isOpenMic) {
      // Calculate the average absolute value of the audio signal to detect voice
      int16_t* samples = (int16_t*)i2sData;
      long sum = 0;
      for (int i = 0; i < SAMPLE_SIZE; i++) {
        sum += abs(samples[i]);
      }
      float average = sum / (float)SAMPLE_SIZE;

      if (average > 200) {  // Set a threshold to determine if there is human voice
        Serial.println("Voice detected");
        lastVoiceTime = currentTime;  // Update the time of the last voice
        isVoiceActive = true;         // Set the flag
      } else {
        if (isVoiceActive && currentTime - lastVoiceTime > 2000) {
          Serial.println("No voice for 2 seconds");
          String jsonString = createJsonString("record_stream", "0x04");
          webSocket.sendTXT(jsonString);
          isVoiceActive = false;  // Reset the flag
          isOpenMic = false;      // Set isOpenMic to false
          isOpenSpk = true;
          Serial.println("Turn on the speaker");
        }
      }
      // Convert the binary audio data to Base64 encoding
      String base64Data = base64::encode(i2sData, bytesRead);
      Serial.println("Sending audio stream");
      String jsonString = createJsonString("record_stream", base64Data);
      webSocket.sendTXT(jsonString);
    }
  }
}

// Create a JSON data body
String createJsonString(const String& type, const String& data) {
  // Build a JSON object
  StaticJsonDocument<300> jsonDoc;
  // Create a nested JSON structure
  jsonDoc["event"] = type;
  jsonDoc["mac_address"] = macAddress;
  jsonDoc["data"] = data;  // Use data as a sub-item under macAddress
  // Serialize the JSON object into a string
  String jsonString;
  serializeJson(jsonDoc, jsonString);
  return jsonString;  // Return the serialized JSON string
}

// Change status logic
void change_status() {
if (!isOpenMic && !isOpenSpk) {
    TFT.pushImage(0, 0, 320, 240, (uint16_t*)img);//(starting x-coordinate, starting y-coordinate, image length, image width). The image length and image width can be viewed by right-clicking on the image and selecting "Properties"
  } else if (isOpenMic && !isOpenSpk) {
    TFT.pushImage(79, 32, 163, 177, (uint16_t*)listen);
  } else if (!isOpenMic && isOpenSpk) {
    TFT.pushImage(79, 32, 163, 177, (uint16_t*)speak);
  }
}