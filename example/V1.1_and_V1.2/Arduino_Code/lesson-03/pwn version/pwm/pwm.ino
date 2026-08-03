#include <Arduino.h>
#include "LovyanGFX_Driver.h"

/*---------------------------------------------------------------
 * Visible PWM backlight test
 * The sketch draws a duty-cycle screen and progress bar while changing
 * GPIO38. This gives a visible result even if the backlight brightness
 * change is subtle on a particular board.
 *--------------------------------------------------------------*/

// Provides LCD drawing so the test result is visible.
LGFX gfx;

// Sets the PWM carrier frequency in hertz.
const int pwmFreq = 5000;

// Uses 8-bit PWM, so duty cycle values range from 0 to 255.
const int pwmResolution = 8;

// GPIO38 controls the display backlight on this lesson board.
const int pwmPin = 38;

// Records whether LEDC PWM was successfully attached to GPIO38.
bool pwmAttached = false;

/**
 * @brief Draw the current PWM duty cycle on the LCD.
 *
 * The progress bar proves the sketch is running even when the hardware
 * backlight change is hard to see by eye.
 *
 * @param dutyCycle Current PWM duty cycle, from 0 to 255.
 * @return None.
 */
void drawDutyScreen(int dutyCycle) {
  int barWidth = (dutyCycle * 200) / 255;

  gfx.fillScreen(TFT_WHITE);
  gfx.setTextColor(TFT_BLACK, TFT_WHITE);
  gfx.setTextSize(2);
  gfx.setCursor(18, 45);
  gfx.println("PWM Backlight");
  gfx.setCursor(18, 75);
  gfx.println("GPIO38");

  gfx.setCursor(18, 120);
  gfx.print("Duty: ");
  gfx.print(dutyCycle);
  gfx.println(" / 255");

  gfx.drawRect(18, 165, 204, 24, TFT_BLACK);
  gfx.fillRect(20, 167, barWidth, 20, TFT_BLUE);
}

/**
 * @brief Apply the duty cycle to GPIO38 and update the LCD display.
 *
 * If LEDC cannot attach to the pin, the fallback keeps the backlight enabled
 * and still updates the on-screen progress bar so the lesson has a result.
 *
 * @param dutyCycle Current PWM duty cycle, from 0 to 255.
 * @return None.
 */
void applyBacklightDuty(int dutyCycle) {
  if (pwmAttached) {
    ledcWrite(pwmPin, dutyCycle);
  } else {
    digitalWrite(pwmPin, HIGH);
  }

  drawDutyScreen(dutyCycle);
  Serial.print("PWM duty = ");
  Serial.println(dutyCycle);
}

/**
 * @brief Initialize the LCD and attach PWM output to the backlight pin.
 *
 * Arduino calls this function once after reset. The backlight is first forced
 * on with a normal digital output, then LEDC PWM is attached for dimming.
 *
 * @param None.
 * @return None.
 */
void setup() {
  Serial.begin(115200);

  pinMode(pwmPin, OUTPUT);
  digitalWrite(pwmPin, HIGH);

  gfx.init();
  gfx.setRotation(1);
  drawDutyScreen(255);

  pwmAttached = ledcAttach(pwmPin, pwmFreq, pwmResolution);
  if (pwmAttached) {
    ledcWrite(pwmPin, 255);
    Serial.println("PWM backlight test started.");
  } else {
    Serial.println("PWM attach failed; showing LCD duty screen only.");
  }
}

/**
 * @brief Sweep the display backlight brightness up and down.
 *
 * Arduino calls this function repeatedly after setup() finishes. The duty
 * cycle moves between dark and bright levels while the LCD displays the
 * current value.
 *
 * @param None.
 * @return None.
 */
void loop() {
  for (int dutyCycle = 0; dutyCycle <= 255; dutyCycle += 15) {
    applyBacklightDuty(dutyCycle);
    delay(120);
  }

  delay(300);

  for (int dutyCycle = 255; dutyCycle >= 0; dutyCycle -= 15) {
    applyBacklightDuty(dutyCycle);
    delay(120);
  }

  delay(300);
}
