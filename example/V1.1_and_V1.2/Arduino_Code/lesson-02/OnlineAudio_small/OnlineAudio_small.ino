#include <Wire.h>
#include "Arduino.h"
#include "WiFiMulti.h"
#include "Audio.h"

/*---------------------------------------------------------------
 * I2S audio output pins
 * These pins connect the ESP32-S3 I2S peripheral to the onboard
 * amplifier, which converts the decoded audio stream into speaker
 * output.
 *--------------------------------------------------------------*/
#define I2S_DOUT 12
#define I2S_BCLK 13
#define I2S_LRC 11

// Handles MP3 stream decoding and I2S audio output.
Audio audio;

// Manages Wi-Fi connection attempts and stores the access point list.
WiFiMulti wifiMulti;

// Stores the Wi-Fi network name used by this lesson.
String ssid =     "elecrow888";

// Stores the Wi-Fi password used by this lesson.
String password = "elecrow2014";

// NetEase Cloud Music song ID. Use a non-VIP song that can be opened publicly.
String neteaseSongId = "2086327879";

// Builds the direct MP3 stream URL used by the audio decoder.
String audioUrl = String("http://music.163.com/song/media/outer/url?id=") + neteaseSongId + ".mp3";

/**
 * @brief Connect to Wi-Fi and start online audio playback.
 *
 * Arduino calls this function once after reset. It enables the amplifier,
 * joins the configured Wi-Fi network, routes the audio decoder to the I2S
 * pins, and opens the MP3 URL so playback can begin in loop().
 *
 * @param None.
 * @return None.
 */
void setup() {
  Serial.begin(115200);

  /*---------------------------------------------------------------
   * Enable the onboard audio amplifier
   * GPIO21 controls the amplifier shutdown circuit on this board.
   * Keeping it low allows the decoded stream to reach the speaker.
   *--------------------------------------------------------------*/
  pinMode(21, OUTPUT);
  // pinMode(14, OUTPUT);
  // digitalWrite(14, LOW);
  digitalWrite(21, LOW);

  /*---------------------------------------------------------------
   * Join the Wi-Fi network
   * Online playback depends on a stable network connection. A second
   * connection attempt is made if the first pass does not complete.
   *--------------------------------------------------------------*/
  delay(50);
  Serial.printf("[LINE--%d]\n", __LINE__);
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(ssid.c_str(), password.c_str());
  wifiMulti.run();
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect(true);
    wifiMulti.run();
  }
  Serial.printf("[LINE--%d]\n", __LINE__);
  Serial.println("----- WIFI_CONNECTED -----");

  /*---------------------------------------------------------------
   * Configure and open the audio stream
   * The decoder pulls MP3 data from the URL and sends samples through
   * I2S. Volume 20 is near the top of the library's 0 to 21 range.
   *--------------------------------------------------------------*/
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(20);
  digitalWrite(21, LOW);
  
  audio.connecttohost(audioUrl.c_str());
  
  // Alternative stream URL for experimenting with a different song.
  //audio.connecttohost("http://music.163.com/song/media/outer/url?id=1391891631.mp3");

  Serial.printf("[LINE--%d]\t ready to play!!\n", __LINE__);
}

/**
 * @brief Keep the decoder running and accept a replacement stream URL.
 *
 * Arduino calls this function repeatedly after setup() finishes. The audio
 * library needs frequent calls to audio.loop() so network data can be read,
 * decoded, and sent to I2S without underruns. Sending a URL through the
 * serial monitor stops the current song and starts the new stream.
 *
 * @param None.
 * @return None.
 */
void loop() {
  audio.loop();
  if (Serial.available()) {
    audio.stopSong();
    String r = Serial.readString();
    r.trim();
    if (r.length() > 5) audio.connecttohost(r.c_str());
    log_i("free heap=%i", ESP.getFreeHeap());
  }
}
