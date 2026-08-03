/*---------------------------------------------------------------
 * Serial counter output
 * Send a simple message once per second so students can verify
 * that the board, USB cable, serial port, and monitor baud rate
 * are all working correctly.
 *--------------------------------------------------------------*/

// Stores how many messages have already been sent to the serial monitor.
int counter = 0;

/**
 * @brief Configure the serial monitor connection.
 *
 * Arduino calls this function once after reset. The baud rate must match
 * the serial monitor setting, otherwise the output text may appear garbled
 * or may not be readable.
 *
 * @param None.
 * @return None.
 */
void setup() {
  Serial.begin(9600);
}

/**
 * @brief Print the message counter periodically.
 *
 * Arduino calls this function repeatedly after setup() finishes. Each pass
 * sends one line of text, then waits for one second before the next line so
 * the change is easy to observe in the serial monitor.
 *
 * @param None.
 * @return None.
 */
void loop() {
  Serial.print("Hello, World! ");
  Serial.print("--- ");
  Serial.println(counter);

  counter++;
  delay(1000);
}
