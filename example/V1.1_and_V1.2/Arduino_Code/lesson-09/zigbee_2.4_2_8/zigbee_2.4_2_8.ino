/*---------------------------------------------------------------
 * Wireless module UART pins
 * Serial1 is connected to the wireless module, while Serial is kept
 * for the USB serial monitor on the computer.
 *--------------------------------------------------------------*/
#define SERIAL1_RX 2
#define SERIAL1_TX 1

/**
 * @brief Start both serial ports and enable the wireless module path.
 *
 * Arduino calls this function once after reset. The USB serial port is used
 * for debugging, and Serial1 is used to receive text from the wireless module
 * or its adapter board.
 *
 * @param None.
 * @return None.
 */
void setup() {
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, SERIAL1_RX, SERIAL1_TX);

  pinMode(45, OUTPUT);
  digitalWrite(45, LOW);
  Serial.println("ESP32-S3 Serial Communication Example");
}

/**
 * @brief Forward received wireless-module text to the USB serial monitor.
 *
 * Arduino calls this function repeatedly after setup() finishes. When Serial1
 * receives a line, the sketch prints it through Serial so students can verify
 * that the module UART wiring and baud rate are correct.
 *
 * @param None.
 * @return None.
 */
void loop() {
  if (Serial1.available()) {
    String message = Serial1.readStringUntil('\n');
    Serial.print("The message from serial port was received.: ");
    Serial.println(message);
  }
}
