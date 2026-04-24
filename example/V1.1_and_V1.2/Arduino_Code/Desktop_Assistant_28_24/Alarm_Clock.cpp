// Alarm_Clock.cpp
#include "Alarm_Clock.h"
#include "ui.h"
#include <Arduino.h>

// User-selected alarm time
static int alarmHour = -1;
static int alarmMin = -1;
static bool alarmEnabled = false;

// Timing for buzzer
const int buzzerPin = 8;
static unsigned long alarmStartMillis = 0;
const unsigned long alarmDuration = 10000; // 10 seconds

void buzzer_start()
{
  analogWrite(buzzerPin, 249);
}

void buzzer_stop()
{
  analogWrite(buzzerPin, 0);
}

void initAlarmClock() {
    pinMode(buzzerPin, OUTPUT);
    // digitalWrite(buzzerPin, LOW);
    buzzer_stop();
}

void Turn_On_Off_Alarm() {
    // Toggle alarm status
    alarmEnabled = (lv_obj_get_state(ui_ClockSwitch) & LV_STATE_CHECKED);
    if (alarmEnabled) {
        // Read selected hour and minute from dropdown lists
        char buf[3];
        lv_dropdown_get_selected_str(ui_HourDropdown, buf, sizeof(buf));
        alarmHour = atoi(buf);
        lv_dropdown_get_selected_str(ui_MinDropdown, buf, sizeof(buf));
        alarmMin = atoi(buf);
        alarmStartMillis = 0; // Reset timer
        Serial.printf("Alarm set for %02d:%02d\n", alarmHour, alarmMin);
    } else {
        // Turn off alarm and stop buzzer
        // digitalWrite(buzzerPin, LOW);
        buzzer_stop();
        alarmStartMillis = 0;
        Serial.println("Alarm disabled");
    }
}

void handleAlarmClock(unsigned long currentMillis) {
    if (!alarmEnabled) return;

    // Get current time from time label
    const char *timeText = lv_label_get_text(ui_TimeLabel);
    int hour, min, sec;
    if (sscanf(timeText, "%d:%d:%d", &hour, &min, &sec) != 3) return;

    // Trigger when the set time is reached and second is 0
    if (hour == alarmHour && min == alarmMin && sec == 0 && alarmStartMillis == 0) {
        // digitalWrite(buzzerPin, HIGH);
        buzzer_start();
        alarmStartMillis = currentMillis;
        Serial.println("Alarm triggered!");
    }

    // Automatically stop after ringing for 10 seconds
    if (alarmStartMillis > 0 && currentMillis - alarmStartMillis >= alarmDuration) {
        // digitalWrite(buzzerPin, LOW);
        buzzer_stop();
        alarmStartMillis = 0;
        alarmEnabled = false; // Automatically turn off switch state
        // Update UI switch state to off
        lv_obj_clear_state(ui_ClockSwitch, LV_STATE_CHECKED);
        Serial.println("Alarm stopped");
    }
}
