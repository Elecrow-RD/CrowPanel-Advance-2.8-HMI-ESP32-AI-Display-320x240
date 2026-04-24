#include "Arduino.h"
#include "WiFi.h"
#include "Audio.h"

const char *ssid = "elecrow888";        // your network SSID (name)
const char *password = "elecrow2014";   // your network password

const char *audioUrl = "http://music.163.com/song/media/outer/url?id=2086327879.mp3";

//2.4 2.8 3.5 pins init
#define BCLK 13
#define LRC 11
#define DOUT 12

//4.3 5.0 7.0 pins init
// #define BCLK 5
// #define LRC 6
// #define DOUT 4

Audio audio; // Create an instance of the Audio class

//--------methond-----------------------------------------------------------------

void connent_wifi() 
{
  // Connect to WiFi
  Serial.printf("connect to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");
}


//play httpmp3
void play_http_mp3()
{
  audio.setPinout(BCLK,LRC,DOUT); // Adjust pins according to your setup
  audio.setVolume(100);        // Set the volume level
  // Connect to the audio URL and start playback
  audio.connecttohost(audioUrl);
}


// Small size operation necessary to pull down the GPIO 14 and GPIO 21 pin functions
void pull_gpios_low()
{
  // Define the GPIO pin to operate on
  // const int GPIO_OUTPUT_IO_1 = 14;
  const int GPIO_OUTPUT_IO_2 = 21;
  // Set the pin to output mode
  // pinMode(GPIO_OUTPUT_IO_1, OUTPUT);
  pinMode(GPIO_OUTPUT_IO_2, OUTPUT);

  //Pull down pins of GPIO 14 and GPIO 21
  // digitalWrite(GPIO_OUTPUT_IO_1, LOW);
  digitalWrite(GPIO_OUTPUT_IO_2, LOW);
}

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  // Connect to WiFi
  connent_wifi();

  // Small size operation necessary to pull down the GPIO 14 and GPIO 21 pin functions
  pull_gpios_low();

  //play httpmp3
  play_http_mp3();
}


void loop()
{
  audio.loop();
  delay(10);
}


extern "C" void app_main()
{
  initArduino();
  setup();
  for (;;)
  {
    loop();
  }
}