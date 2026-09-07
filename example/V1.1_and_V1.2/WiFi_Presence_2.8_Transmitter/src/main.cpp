#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <esp_wifi.h>
#include <math.h>

#include "LovyanGFX_Driver.h"

#ifndef NODE_ROLE_SENSOR
#define NODE_ROLE_SENSOR 0
#endif
#ifndef CROWPANEL_MODEL
#define CROWPANEL_MODEL 24
#endif

namespace {

constexpr char kSsid[] = "CrowSense-S3";
constexpr char kPassword[] = "CrowSense2026";
constexpr uint8_t kWifiChannel = 6;
constexpr uint16_t kUdpPort = 3333;
constexpr uint32_t kMagic = 0x43534932;  // "CSI2"
constexpr int kBacklightPin = 38;
constexpr int kBootButtonPin = 0;
constexpr uint32_t kPacketPeriodMs = 20;
constexpr uint32_t kCalibrationMs = 10000;
constexpr uint32_t kPresenceHoldMs = 10000;
constexpr uint32_t kPeerTimeoutMs = 2500;
constexpr size_t kMaxSubcarriers = 128;

struct __attribute__((packed)) WirePacket {
    uint32_t magic;
    uint32_t sequence;
    uint8_t presence;
    uint8_t calibrating;
    uint8_t calibrationPercent;
    uint8_t reserved;
    float score;
    float threshold;
    uint32_t sampleRate;
    uint32_t totalSamples;
};

LGFX display;
LGFX_Sprite metricsSprite(&display);
WiFiUDP udp;
bool uiReady = false;
bool metricsSpriteReady = false;
int lastUiState = -1;
uint32_t lastUiMetricsMs = 0;
String lastUiHeading;

void drawUi(const char *role, const char *link, bool presence, bool calibrating,
            uint8_t calibrationPercent, float score, float threshold,
            uint32_t sampleRate, uint32_t totalSamples) {
    const int state = calibrating ? 1 : (presence ? 2 : 3);
    const uint16_t background = calibrating ? 0x0215 : (presence ? 0xA800 : 0x0320);
    const uint32_t now = millis();

    const String heading = String(role) + '|' + link;
    if (!uiReady || state != lastUiState || heading != lastUiHeading) {
        display.fillScreen(background);
        display.setTextColor(TFT_WHITE, background);
        display.setTextSize(2);
        display.drawString(role, 12, 10);
        display.setTextSize(1);
        display.drawString(link, 12, 38);
        display.drawFastHLine(12, 52, 296, 0xFFFF);
        display.setTextSize(2);
        display.drawString(calibrating ? "CALIBRATING" :
                           (presence ? "PRESENCE DETECTED" : "AREA CLEAR"), 12, 64);
        display.setTextSize(1);
        display.drawString("BOOT button / serial C: recalibrate", 12, 220);
        uiReady = true;
        lastUiState = state;
        lastUiHeading = heading;
        lastUiMetricsMs = 0;
    }

    if (now - lastUiMetricsMs < 200) {
        return;
    }
    lastUiMetricsMs = now;
    if (!metricsSpriteReady) {
        metricsSprite.setColorDepth(16);
        metricsSprite.createSprite(300, 116);
        metricsSpriteReady = true;
    }
    metricsSprite.fillSprite(background);
    metricsSprite.setTextColor(TFT_WHITE, background);
    metricsSprite.setTextSize(2);
    char line[64];
    snprintf(line, sizeof(line), "Score: %.3f", score);
    metricsSprite.drawString(line, 2, 4);
    snprintf(line, sizeof(line), "Limit: %.3f", threshold);
    metricsSprite.drawString(line, 2, 32);
    metricsSprite.setTextSize(1);
    if (calibrating) {
        snprintf(line, sizeof(line), "Empty-area learning: %u%%", calibrationPercent);
    } else {
        snprintf(line, sizeof(line), "CSI rate: %lu samples/s", (unsigned long)sampleRate);
    }
    metricsSprite.drawString(line, 2, 66);
    snprintf(line, sizeof(line), "Total CSI: %lu", (unsigned long)totalSamples);
    metricsSprite.drawString(line, 2, 84);
    metricsSprite.pushSprite(10, 96);
}

void initDisplay() {
    pinMode(kBacklightPin, OUTPUT);
    digitalWrite(kBacklightPin, HIGH);
    display.init();
    display.setRotation(1);
    drawUi("CrowPanel Wi-Fi Sense", "Starting...", false, true, 0, 0, 0, 0, 0);
}

#if NODE_ROLE_SENSOR

portMUX_TYPE csiLock = portMUX_INITIALIZER_UNLOCKED;
float baseline[kMaxSubcarriers] = {};
size_t baselineCount = 0;
bool baselineReady = false;
volatile float accumulatedScore = 0.0f;
volatile uint32_t accumulatedSamples = 0;
volatile uint32_t totalCsiSamples = 0;
uint8_t apBssid[6] = {};

float motionScore = 0.0f;
float threshold = 0.110f;
float calibrationMean = 0.0f;
float calibrationM2 = 0.0f;
uint32_t calibrationSamples = 0;
bool calibrationReported = false;
uint32_t calibrationStart = 0;
uint32_t lastMotionMs = 0;
uint8_t activeWindows = 0;
bool presence = false;
uint32_t sequenceNumber = 0;
uint32_t currentSampleRate = 0;

uint8_t calibrationPercent() {
    const uint32_t elapsed = millis() - calibrationStart;
    return static_cast<uint8_t>(min<uint32_t>(100, elapsed * 100 / kCalibrationMs));
}

void resetCalibration() {
    portENTER_CRITICAL(&csiLock);
    baselineCount = 0;
    baselineReady = false;
    accumulatedScore = 0.0f;
    accumulatedSamples = 0;
    portEXIT_CRITICAL(&csiLock);
    memset(baseline, 0, sizeof(baseline));
    calibrationMean = 0.0f;
    calibrationM2 = 0.0f;
    calibrationSamples = 0;
    calibrationStart = millis();
    motionScore = 0.0f;
    threshold = 0.110f;
    calibrationReported = false;
    lastMotionMs = 0;
    activeWindows = 0;
    presence = false;
    lastUiState = -1;
    Serial.println("CALIBRATION: keep the area empty for 10 seconds");
}

void IRAM_ATTR onCsi(void *, wifi_csi_info_t *info) {
    if (!info || !info->buf || info->len < 32 || memcmp(info->mac, apBssid, 6) != 0) {
        return;
    }
    const size_t offset = info->first_word_invalid ? 4 : 0;
    if (static_cast<size_t>(info->len) <= offset) {
        return;
    }
    const size_t complexCount = min(kMaxSubcarriers,
                                    static_cast<size_t>((info->len - offset) / 2));
    if (complexCount < 16) {
        return;
    }

    float current[kMaxSubcarriers];
    float meanAmplitude = 0.0f;
    for (size_t i = 0; i < complexCount; ++i) {
        const float imaginary = info->buf[offset + i * 2];
        const float real = info->buf[offset + i * 2 + 1];
        current[i] = sqrtf(real * real + imaginary * imaginary);
        meanAmplitude += current[i];
    }
    meanAmplitude /= complexCount;
    if (meanAmplitude < 1.0f) {
        return;
    }

    float deltaEnergy = 0.0f;
    portENTER_CRITICAL_ISR(&csiLock);
    if (!baselineReady || baselineCount != complexCount) {
        for (size_t i = 0; i < complexCount; ++i) {
            baseline[i] = current[i] / meanAmplitude;
        }
        baselineCount = complexCount;
        baselineReady = true;
        portEXIT_CRITICAL_ISR(&csiLock);
        return;
    }
    for (size_t i = 0; i < complexCount; ++i) {
        const float normalized = current[i] / meanAmplitude;
        const float delta = normalized - baseline[i];
        deltaEnergy += delta * delta;
        baseline[i] += 0.005f * delta;
    }
    accumulatedScore += sqrtf(deltaEnergy / complexCount);
    accumulatedSamples++;
    totalCsiSamples++;
    portEXIT_CRITICAL_ISR(&csiLock);
}

void enableCsi() {
    wifi_csi_config_t config = {};
    config.lltf_en = true;
    config.htltf_en = true;
    config.stbc_htltf2_en = true;
    config.ltf_merge_en = true;
    config.channel_filter_en = true;
    config.manu_scale = false;
    config.shift = 0;
    config.dump_ack_en = false;
    ESP_ERROR_CHECK(esp_wifi_set_csi_config(&config));
    ESP_ERROR_CHECK(esp_wifi_set_csi_rx_cb(onCsi, nullptr));
    ESP_ERROR_CHECK(esp_wifi_set_csi(true));
}

void updateDetector() {
    static uint32_t lastWindowMs = 0;
    static uint32_t rateSamples = 0;
    static uint32_t rateStartMs = 0;
    const uint32_t now = millis();
    if (now - lastWindowMs < 250) {
        return;
    }
    lastWindowMs = now;

    float scoreSum;
    uint32_t sampleCount;
    uint32_t totalSamples;
    portENTER_CRITICAL(&csiLock);
    scoreSum = accumulatedScore;
    sampleCount = accumulatedSamples;
    totalSamples = totalCsiSamples;
    accumulatedScore = 0.0f;
    accumulatedSamples = 0;
    portEXIT_CRITICAL(&csiLock);
    rateSamples += sampleCount;
    if (rateStartMs == 0) rateStartMs = now;
    if (now - rateStartMs >= 1000) {
        currentSampleRate = rateSamples * 1000 / (now - rateStartMs);
        rateSamples = 0;
        rateStartMs = now;
    }
    if (sampleCount == 0) {
        drawUi("CSI RECEIVER", "Wi-Fi linked - waiting CSI", false, true,
               calibrationPercent(), motionScore, threshold, currentSampleRate, totalSamples);
        return;
    }

    const float windowScore = scoreSum / sampleCount;
    motionScore = motionScore * 0.70f + windowScore * 0.30f;
    const bool calibrating = now - calibrationStart < kCalibrationMs;
    if (calibrating) {
        if (now - calibrationStart > 2000) {
            calibrationSamples++;
            const float delta = motionScore - calibrationMean;
            calibrationMean += delta / calibrationSamples;
            calibrationM2 += delta * (motionScore - calibrationMean);
        }
        presence = false;
        drawUi("CSI RECEIVER", "Wi-Fi linked", false, true, calibrationPercent(),
               motionScore, threshold, currentSampleRate, totalSamples);
        return;
    }

    if (!calibrationReported) {
        const float deviation = calibrationSamples > 1
                                    ? sqrtf(calibrationM2 / (calibrationSamples - 1))
                                    : 0.0f;
        threshold = constrain(calibrationMean + max(0.020f, 4.0f * deviation), 0.055f, 0.250f);
        calibrationReported = true;
        Serial.printf("CALIBRATION DONE: noise=%.5f deviation=%.5f threshold=%.5f\n",
                      calibrationMean, deviation, threshold);
    }

    if (motionScore > threshold) {
        if (activeWindows < 255) activeWindows++;
        if (activeWindows >= 2) {
            lastMotionMs = now;
            presence = true;
        }
    } else {
        activeWindows = 0;
    }
    if (presence && now - lastMotionMs > kPresenceHoldMs) presence = false;

    drawUi("CSI RECEIVER", "Wi-Fi linked", presence, false, 100, motionScore,
           threshold, currentSampleRate, totalSamples);
    static uint32_t lastLog = 0;
    if (now - lastLog >= 1000) {
        lastLog = now;
        Serial.printf("CSI score=%.5f threshold=%.5f rate=%lu total=%lu presence=%s\n",
                      motionScore, threshold, (unsigned long)currentSampleRate,
                      (unsigned long)totalSamples, presence ? "YES" : "NO");
    }
}

void setupSensor() {
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(true);
    drawUi("CSI RECEIVER", "Connecting to transmitter...", false, true, 0, 0, threshold, 0, 0);
    WiFi.begin(kSsid, kPassword);
    Serial.print("Connecting to CSI transmitter");
    while (WiFi.status() != WL_CONNECTED) {
        delay(250);
        Serial.print('.');
    }
    Serial.printf("\nReceiver IP: %s\n", WiFi.localIP().toString().c_str());
    wifi_ap_record_t apInfo = {};
    ESP_ERROR_CHECK(esp_wifi_sta_get_ap_info(&apInfo));
    memcpy(apBssid, apInfo.bssid, sizeof(apBssid));
    Serial.printf("Transmitter BSSID: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  apBssid[0], apBssid[1], apBssid[2], apBssid[3], apBssid[4], apBssid[5]);
    udp.begin(kUdpPort);
    enableCsi();
    resetCalibration();
}

void loopSensor() {
    static uint32_t lastPacket = 0;
    const uint32_t now = millis();
    if (now - lastPacket >= kPacketPeriodMs && WiFi.status() == WL_CONNECTED) {
        lastPacket = now;
        uint32_t totalSamples;
        portENTER_CRITICAL(&csiLock);
        totalSamples = totalCsiSamples;
        portEXIT_CRITICAL(&csiLock);
        WirePacket packet = {kMagic, sequenceNumber++, static_cast<uint8_t>(presence),
                             static_cast<uint8_t>(now - calibrationStart < kCalibrationMs),
                             calibrationPercent(), 0, motionScore, threshold,
                             currentSampleRate, totalSamples};
        const IPAddress peer = WiFi.gatewayIP();
        if (peer != IPAddress(0, 0, 0, 0)) {
            udp.beginPacket(peer, kUdpPort);
            udp.write(reinterpret_cast<const uint8_t *>(&packet), sizeof(packet));
            udp.endPacket();
        }
    }
    const int packetSize = udp.parsePacket();
    if (packetSize > 0) {
        WirePacket reply = {};
        udp.read(reinterpret_cast<uint8_t *>(&reply), min(packetSize, (int)sizeof(reply)));
    }
    updateDetector();

    static bool priorButton = true;
    const bool button = digitalRead(kBootButtonPin);
    if (priorButton && !button) resetCalibration();
    priorButton = button;
    while (Serial.available()) {
        const char command = Serial.read();
        if (command == 'c' || command == 'C') resetCalibration();
        else if (command == '+') threshold = min(0.25f, threshold * 1.15f);
        else if (command == '-') threshold = max(0.01f, threshold / 1.15f);
    }
    delay(1);
}

#else

uint32_t lastPeerPacketMs = 0;
uint32_t receivedPackets = 0;

void setupAccessPoint() {
    WiFi.mode(WIFI_AP);
    WiFi.setSleep(false);
    if (!WiFi.softAP(kSsid, kPassword, kWifiChannel, false, 1)) {
        Serial.println("Failed to start CSI access point");
        ESP.restart();
    }
    udp.begin(kUdpPort);
    drawUi("TRAFFIC TRANSMITTER", "Waiting for CSI receiver...", false, true, 0, 0, 0, 0, 0);
    Serial.printf("CSI transmitter ready: %s channel=%u IP=%s\n", kSsid, kWifiChannel,
                  WiFi.softAPIP().toString().c_str());
}

void loopAccessPoint() {
    static uint32_t lastLogMs = 0;
    const int packetSize = udp.parsePacket();
    if (packetSize >= (int)sizeof(WirePacket)) {
        WirePacket packet = {};
        udp.read(reinterpret_cast<uint8_t *>(&packet), sizeof(packet));
        if (packet.magic == kMagic) {
            lastPeerPacketMs = millis();
            receivedPackets++;
            drawUi("TRAFFIC TRANSMITTER", "CSI receiver connected", packet.presence != 0,
                   packet.calibrating != 0, packet.calibrationPercent, packet.score,
                   packet.threshold, packet.sampleRate, packet.totalSamples);
            udp.beginPacket(udp.remoteIP(), udp.remotePort());
            udp.write(reinterpret_cast<const uint8_t *>(&packet), sizeof(packet));
            udp.endPacket();
            if (millis() - lastLogMs >= 1000) {
                lastLogMs = millis();
                Serial.printf("PEER packets=%lu score=%.5f threshold=%.5f CSI_rate=%lu presence=%s calibration=%u%%\n",
                              (unsigned long)receivedPackets, packet.score, packet.threshold,
                              (unsigned long)packet.sampleRate, packet.presence ? "YES" : "NO",
                              packet.calibrationPercent);
            }
        }
    }
    if (lastPeerPacketMs == 0 || millis() - lastPeerPacketMs > kPeerTimeoutMs) {
        if (lastPeerPacketMs != 0) Serial.println("CSI receiver disconnected");
        lastPeerPacketMs = 0;
        drawUi("TRAFFIC TRANSMITTER", "Waiting for CSI receiver...", false, true, 0, 0, 0, 0, 0);
    }
    delay(1);
}

#endif

}  // namespace

void setup() {
    Serial.begin(115200);
    pinMode(kBootButtonPin, INPUT_PULLUP);
    initDisplay();
    Serial.printf("CrowPanel Advance %s V1.2: %s\n",
                  CROWPANEL_MODEL == 28 ? "2.8-inch" : "2.4-inch",
                  NODE_ROLE_SENSOR ? "CSI RECEIVER / STA" : "TRAFFIC TRANSMITTER / AP");
#if NODE_ROLE_SENSOR
    setupSensor();
#else
    setupAccessPoint();
#endif
}

void loop() {
#if NODE_ROLE_SENSOR
    loopSensor();
#else
    loopAccessPoint();
#endif
}
