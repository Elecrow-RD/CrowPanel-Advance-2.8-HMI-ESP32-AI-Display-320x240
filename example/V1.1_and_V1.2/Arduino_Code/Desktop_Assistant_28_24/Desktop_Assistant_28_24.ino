#include <Arduino.h>
#include <lvgl.h>
#include "ui.h"

#include "wifi_connect.h"
#include "weather.h"
#include "get_Time.h"
#include "display_Screen.h"
#include "Play_Music.h"
#include "Alarm_Clock.h"
#include "timer.h"
#include "Light_Control.h"

void setup() {
  Serial.begin(115200);

  initDisplay();          // Initialize screen, touch, and LVGL
  ui_init();              // Initialize UI interface
  Serial.println("----- Screen related configuration completed -----");

  connectToWiFi();        // WiFi connection
  initNTP();              // Initialize NTP
  audioInit();            // Initialize: music playback
  initAlarmClock();       // Initialize: alarm pin (buzzer)
  initTimer();            // Initialize: timer-related
  lightControlInit();     // Initialize: light control
  
  getTimeData();          // Get time
  getWeatherData();       // Get weather
  Serial.println("----- Successfully initialized -----");
}

void loop() {
  lv_timer_handler();

  unsigned long currentMillis = millis();
  handleTimeUpdate(currentMillis);        // Refresh time every second
  handleNTPUpdate(currentMillis);         // Periodically sync NTP (every 10 minutes)
  handleWeatherUpdate(currentMillis);     // Periodically update weather (every 10 minutes)
  handleAlarmClock(currentMillis);        // Handle alarm response
  
  audioLoop();                            // Music player
  handleTimer();                          // Timer
  delay(5);
}
