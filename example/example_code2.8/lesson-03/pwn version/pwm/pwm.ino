// Select PWM channel
int pwmChannel = 0;  
int pwmFreq = 5000;  // 5kHz
int pwmResolution = 8; // 8-bit resolution（0-255）

int pwmPin = 38;  // Set the pins, assuming you are using GPIO38

void setup() {
  // Configure PWM channels
  ledcSetup(pwmChannel, pwmFreq, pwmResolution);
  
  // Connect the PWM signal to the specified pins
  ledcAttachPin(pwmPin, pwmChannel);
}

void loop() {

  // Adjust PWM duty cycle  (0-255)
  for (int dutyCycle = 0; dutyCycle <= 255; dutyCycle++) {
    ledcWrite(pwmChannel, dutyCycle); // Setting the Duty Cycle
    delay(100); // Delay to visualize changes
  }
}
