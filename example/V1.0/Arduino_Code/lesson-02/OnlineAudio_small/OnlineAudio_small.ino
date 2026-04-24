#include <Wire.h>
#include "Arduino.h"
#include "WiFiMulti.h"
#include "Audio.h"

// Built-in amplifier chip pins
#define I2S_DOUT 12
#define I2S_BCLK 13
#define I2S_LRC 11

Audio audio;
WiFiMulti wifiMulti;

String ssid = "yanfa1"; // WiFi name
String password = "1223334444yanfa"; // WiFi password

void setup() {
  Serial.begin(115200); // Set baud rate

  pinMode(21, OUTPUT); // Sound shutdown pin
  pinMode(14, OUTPUT); // MUTE pin
  digitalWrite(14, LOW);
  digitalWrite(21, HIGH); // Set pins to enable music playback

  delay(50);
  Serial.printf("[LINE--%d]\n", __LINE__);
  WiFi.mode(WIFI_STA); // Set the WiFi mode of the device to Station mode.
  wifiMulti.addAP(ssid.c_str(), password.c_str()); // Add WiFi credentials
  wifiMulti.run(); // Connect to WiFi
  if (WiFi.status() != WL_CONNECTED) { // WiFi connection failed
    WiFi.disconnect(true);
    wifiMulti.run(); // Attempt to connect again
  }
  Serial.printf("[LINE--%d]\n", __LINE__);
  Serial.println("----- WIFI_CONNECTED -----");
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT); // Initialize audio
  audio.setVolume(20);  // Volume level: 0...21
  digitalWrite(21, LOW); // Pull the mute control pin low to enable sound
  
  // Choose the URL of the music you want to play
  // audio.connecttohost("http://music.163.com/song/media/outer/url?id=2086327879.mp3"); // Flowers.mp3
  
  // This is Taylor Swift singing 'Last Christmas'
  audio.connecttohost("http://music.163.com/song/media/outer/url?id=1405259103.mp3"); // Last Christmas.mp3

  // audio.connecttohost("http://music.163.com/song/media/outer/url?id=5103312.mp3"); // Empire state of mine.mp3
  Serial.printf("[LINE--%d]\t ready to play!!\n", __LINE__);
}

void loop() {
  audio.loop(); // Play each frame of the music
  if (Serial.available()) {  // Condition to stop music, triggered when serial data is received
    audio.stopSong(); // Stop playback
    String r = Serial.readString(); // Read music data from serial
    r.trim(); // Ensure there are no extra spaces or line breaks in the received data, 
              // so that the subsequent check of r.length()>5 accurately reflects the length of valid characters.
    if (r.length() > 5) audio.connecttohost(r.c_str()); // Try connecting to the next song URL
    log_i("free heap=%i", ESP.getFreeHeap());
  }
}

// optional
void audio_info(const char *info) {
  Serial.print("info        ");
  Serial.println(info);
}

void audio_id3data(const char *info) {  //id3 metadata
  Serial.print("id3data     ");
  Serial.println(info);
}

void audio_eof_mp3(const char *info) {  //end of file
  Serial.print("eof_mp3     ");
  Serial.println(info);
}

void audio_showstation(const char *info) {
  Serial.print("station     ");
  Serial.println(info);
}

void audio_showstreamtitle(const char *info) {
  Serial.print("streamtitle ");
  Serial.println(info);
}

void audio_bitrate(const char *info) {
  Serial.print("bitrate     ");
  Serial.println(info);
}

void audio_commercial(const char *info) {  //duration in sec
  Serial.print("commercial  ");
  Serial.println(info);
}

void audio_icyurl(const char *info) {  //homepage
  Serial.print("icyurl      ");
  Serial.println(info);
}

void audio_lasthost(const char *info) {  //stream URL played
  Serial.print("lasthost    ");
  Serial.println(info);
}
