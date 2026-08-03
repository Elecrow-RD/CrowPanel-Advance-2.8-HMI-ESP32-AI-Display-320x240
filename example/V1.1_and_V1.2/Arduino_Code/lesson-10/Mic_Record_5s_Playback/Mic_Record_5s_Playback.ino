#include <Arduino.h>
#include <ESP_I2S.h>
#include <esp_heap_caps.h>

/*
 * CrowPanel Advance 2.4/2.8inch V1.1/V1.2 audio pins.
 *
 * The original 4.3/5.0/7.0inch demo uses different audio pins and an
 * I2C audio-control device. The small 2.4/2.8inch boards route the
 * microphone through IO9/IO10, the speaker through IO13/IO11/IO12,
 * and use IO21 for amplifier mute control.
 */
constexpr int MIC_CLK = 9;
constexpr int MIC_DATA = 10;

constexpr int SPK_BCLK = 13;
constexpr int SPK_LRCLK = 11;
constexpr int SPK_DATA = 12;

constexpr int AMP_MUTE_PIN = 21;
constexpr int AUDIO_SELECT_PIN = 45;
constexpr int AUDIO_SELECT_MIC_LEVEL = HIGH;

constexpr uint32_t SAMPLE_RATE = 16000;
constexpr uint32_t RECORD_SECONDS = 5;
constexpr float PLAYBACK_GAIN = 8.0f;
constexpr size_t WAV_HEADER_SIZE = 44;

I2SClass audio;

void selectMicrophonePath() {
  pinMode(AUDIO_SELECT_PIN, OUTPUT);
  digitalWrite(AUDIO_SELECT_PIN, AUDIO_SELECT_MIC_LEVEL);
}

void muteAmplifier(bool mute) {
  pinMode(AMP_MUTE_PIN, OUTPUT);
  digitalWrite(AMP_MUTE_PIN, mute ? HIGH : LOW);
}

int16_t amplify(int16_t sample) {
  int32_t value = static_cast<int32_t>(sample * PLAYBACK_GAIN);
  if (value > INT16_MAX) value = INT16_MAX;
  if (value < INT16_MIN) value = INT16_MIN;
  return static_cast<int16_t>(value);
}

bool recordAndPlay() {
  size_t wavSize = 0;

  selectMicrophonePath();
  muteAmplifier(true);

  audio.setPinsPdmRx(MIC_CLK, MIC_DATA);
  if (!audio.begin(I2S_MODE_PDM_RX, SAMPLE_RATE,
                   I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO)) {
    Serial.println("ERROR: PDM microphone initialization failed.");
    return false;
  }

  Serial.println("Recording for 5 seconds...");
  uint8_t *wav = audio.recordWAV(RECORD_SECONDS, &wavSize);
  audio.end();

  if (wav == nullptr || wavSize <= WAV_HEADER_SIZE) {
    Serial.println("ERROR: Recording failed (check PSRAM/memory). ");
    free(wav);
    return false;
  }
  Serial.printf("Recording complete: %u bytes\n", static_cast<unsigned>(wavSize));

  // recordWAV returns mono 16-bit PCM after a standard 44-byte WAV header.
  uint8_t *monoBytes = wav + WAV_HEADER_SIZE;
  size_t monoSize = wavSize - WAV_HEADER_SIZE;
  size_t sampleCount = monoSize / sizeof(int16_t);
  size_t stereoSize = sampleCount * 2 * sizeof(int16_t);

  int16_t *stereo = static_cast<int16_t *>(
      heap_caps_malloc(stereoSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  bool stereoInPsram = stereo != nullptr;
  if (stereo == nullptr) {
    // Fall back to internal RAM if PSRAM is not exposed as a separate heap.
    stereo = static_cast<int16_t *>(malloc(stereoSize));
  }
  if (stereo == nullptr) {
    Serial.println("ERROR: Playback buffer allocation failed.");
    free(wav);
    return false;
  }

  const int16_t *mono = reinterpret_cast<const int16_t *>(monoBytes);
  for (size_t i = 0; i < sampleCount; ++i) {
    stereo[i * 2] = 0;                 // Left channel is unused.
    stereo[i * 2 + 1] = amplify(mono[i]);  // Speaker is on the right channel.
  }

  audio.setPins(SPK_BCLK, SPK_LRCLK, SPK_DATA);
  if (!audio.begin(I2S_MODE_STD, SAMPLE_RATE,
                   I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO)) {
    Serial.println("ERROR: I2S speaker initialization failed.");
    if (stereoInPsram) {
      heap_caps_free(stereo);
    } else {
      free(stereo);
    }
    free(wav);
    return false;
  }

  Serial.println("Playing the recording...");
  muteAmplifier(false);
  size_t bytesWritten = audio.write(reinterpret_cast<uint8_t *>(stereo), stereoSize);
  delay(20);              // Let the final DMA samples leave the peripheral.
  muteAmplifier(true);
  audio.end();

  if (stereoInPsram) {
    heap_caps_free(stereo);
  } else {
    free(stereo);
  }
  free(wav);

  Serial.printf("Playback complete: %u/%u bytes written.\n",
                static_cast<unsigned>(bytesWritten),
                static_cast<unsigned>(stereoSize));
  return bytesWritten == stereoSize;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  selectMicrophonePath();
  muteAmplifier(true);

  Serial.println("\n5-second microphone record/playback demo");
  Serial.println("Target board: CrowPanel Advance 2.4/2.8inch V1.1/V1.2");
  Serial.println("Send r in Serial Monitor to run it again.");
  recordAndPlay();
}

void loop() {
  if (Serial.available()) {
    char command = Serial.read();
    if (command == 'r' || command == 'R') {
      while (Serial.available()) Serial.read();
      recordAndPlay();
    }
  }
  delay(10);
}
