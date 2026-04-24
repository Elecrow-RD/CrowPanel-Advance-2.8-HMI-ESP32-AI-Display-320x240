#include <WiFi.h>              // WiFi library for connecting and managing WiFi networks
#include <WiFiManager.h>       // Used to simplify the WiFi configuration process
#include <WebSocketsClient.h>  // It is used to implement WebSocket client functions
#include <base64.h>            // Used for Base64 encoding and decoding
#include <ArduinoJson.h>       // Used to process JSON data

#include "ESP_I2S.h"

#include <Button.h>
#include "ChatDialog.h" 
#include "Arduino.h"

// ======================= 全局变量 =======================
I2SClass i2s;  // 全局 I2S 实例
ChatDialog chatDialog;

#define BUTTON_PIN 0        // Button connected to pin 0
Button button(BUTTON_PIN);  // Create a Button object

// Custom initialization variables
const char* websocket_server = "192.168.2.116";  // Server ip address----Please change to your computer's IP address

const int websocket_port = 8765;
const char* websocket_path = "/";
String macAddress;           // Define global variables
WebSocketsClient webSocket;  // Define the WebSocket client

// I2S configuration
#define SAMPLE_RATE 16000  // Sampling rate
#define SAMPLE_SIZE 1024   // Number of samples per transfer

bool isOpenMic = false;                        // Initialize the definition of microphone on
bool isOpenSpk = false;                        // Initialize the definition of microphone off
unsigned long isOpenMic_true_time = millis();  // Current time when the microphone is turned on

//-----------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);  // Initialize the serial port

  // Activate the audio recording signal control.
  pinMode(45, OUTPUT);
  digitalWrite(45, HIGH);// Pin 45 is used to control the wireless module and the microphone. High: microphone; Low: wireless module.
  pinMode(21, OUTPUT);
  digitalWrite(21, LOW);  // mute
  
  init_screen();         // Initialize the screen
  wifiServer();          // Connect to the Wi-Fi network
  connect_ws();          // Connect to the WebSocket server
  button.begin();        // Initialize button
}


void loop() {
  buttonWakeUpTask();
  change_status();
  webSocket.loop();     // Start the WebSocket event loop
  record_send_audio();  // Record and send audio
}

//----------------------------------------------------------------------------- Screen display ----------------------------------------------------------
// Function to initialize the screen and related components
void init_screen() {
  pinMode(38, OUTPUT);
  digitalWrite(38, HIGH);     //enable the backlight
  chatDialog.begin();
}
//----------------------------------------------------------------------------- Screen display ----------------------------------------------------------

// Button wake-up event
void buttonWakeUpTask() {
  static unsigned long interruptStartTime = 0;  // Record interrupt start time
  static bool waitingToOpenMic = false;         // Mark whether you are waiting to turn on the microphone

  button.read();  // Read button status

  if (button.pressed()) {
    if (isOpenSpk && !isOpenMic) {
      Serial.print("Interrupt!");
      isOpenSpk = false;
      i2s.end();

      interruptStartTime = millis();  // Record current time
      waitingToOpenMic = true;        // Flag start wait
      String jsonString_i = createJsonString("interrupt_audio", "");
      webSocket.sendTXT(jsonString_i);

    } else if (!isOpenSpk && !isOpenMic) {
      Serial.print("Wake up!");
      // Send wake up signal
      String jsonString_i = createJsonString("wake_up", "");
      webSocket.sendTXT(jsonString_i);
      isOpenSpk = true;
      init_SPK();
    }
  }

  // Check if you have waited 1 second
  if (waitingToOpenMic && (millis() - interruptStartTime >= 1000)) {
    isOpenMic = true;
    isOpenMic_true_time = millis();
    waitingToOpenMic = false;  // Cancel wait
    init_MIC();
  }
}

// * Connect to the Wi-Fi network
void wifiServer() {
  WiFiManager manager;
  // Get the MAC address of the ESP and assign it to the global variable
  macAddress = WiFi.macAddress();
  // Remove the colons
  macAddress.replace(":", "");
  Serial.print("The address is:");
  Serial.println(macAddress);
  // manager.resetSettings();  // Reset the WiFi settings
  String APName = "AI-" + macAddress;
  manager.autoConnect(APName.c_str());
  // Connect to the Wi-Fi
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("WiFi is connected");
}

// * Connect to the WebSocket server
void connect_ws() {
  webSocket.begin(websocket_server, websocket_port, websocket_path);
  webSocket.onEvent(webSocketEvent);
}

// * WebSocket event handling function
void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.println("WebSocket disconnected");
      break;

    case WStype_CONNECTED:
      Serial.println("WebSocket connected");
      // Play opening remarks
      {
        String jsonString = createJsonString("open_word", "");
        webSocket.sendTXT(jsonString);
        Serial.println("WebSocket connected");
        Serial.println("Starting, ready to record");
        isOpenMic = true;
        isOpenMic_true_time = millis();
        init_MIC();
      }
      break;

    case WStype_TEXT: {
        Serial.printf("Received text data: %s\n", payload);
        String rcv_word = (char*)payload;  // Variable in case block

        if (rcv_word.startsWith("USER:")) {
            chatDialog.addMessage(rcv_word.substring(5), false);

        } else if (rcv_word.startsWith("AI:")) {
            chatDialog.addMessage(rcv_word.substring(3), true);

        } else if (rcv_word == "close_mic") {
            isOpenMic = false;
            i2s.end();
            Serial.println("Microphone turned off");
            isOpenSpk = true;
            init_SPK();
            Serial.println("Speaker turned on");
        } else if (rcv_word == "finish_tts") {
            delay(1000);
            String jsonString = createJsonString("re_process_audio", "");
            webSocket.sendTXT(jsonString);
            Serial.println("Starting ASR again, ready to record");
            isOpenSpk = false;
            i2s.end();

            isOpenMic = true;
            init_MIC();
            isOpenMic_true_time = millis();
        }
        break;
    }

    case WStype_BIN: {
      Serial.printf("Received PCM audio data: %d bytes\n", length);

      if (!isOpenSpk) {
        Serial.println("Speaker not initialized. Discarding audio.");
        break;
      }

      if (length % 2 != 0) length--; // Ensure 16-bit alignment

      size_t samples = length / 2;
      size_t stereo_bytes = samples * 4;

      static int16_t* stereo_buffer = nullptr;
      static size_t stereo_buf_size = 0;

      if (stereo_buf_size < stereo_bytes) {
        if (stereo_buffer) heap_caps_free(stereo_buffer);
        stereo_buffer = (int16_t*)heap_caps_malloc(stereo_bytes, MALLOC_CAP_SPIRAM);
        stereo_buf_size = stereo_bytes;
      }

      if (!stereo_buffer) {
        Serial.println("ERROR: Failed to allocate stereo buffer");
        break;
      }

      float amplification = 8.0;   // magnification times
      int16_t* input_samples = (int16_t*)payload;

      for (size_t i = 0; i < samples; i++) {
        int16_t sample = input_samples[i];
        float amplified = sample * amplification;
        if (amplified > 32767) amplified = 32767;
        if (amplified < -32768) amplified = -32768;
        int16_t new_sample = (int16_t)amplified;

        // Convert to stereo sound
        stereo_buffer[i * 2]     = new_sample; // left channel 
        stereo_buffer[i * 2 + 1] = new_sample; // right channel 
      }

      size_t bytes_written = i2s.write((uint8_t*)stereo_buffer, stereo_bytes);
      if (bytes_written != stereo_bytes) {
        Serial.printf("Warning: Only wrote %d/%d bytes\n", bytes_written, stereo_bytes);
      } else {
        Serial.printf("Played %d samples (%d bytes)\n", samples, stereo_bytes);
      }
      break;
    }
  }
}

int warmupFrames = 0;   // Used for counting the first few frames
//--------------------------------------------------------------
// Initialize MIC
void init_MIC() {
  i2s.setPinsPdmRx(9, 10);
  if (!i2s.begin(I2S_MODE_PDM_RX, SAMPLE_RATE, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO)) {
    Serial.println("Failed to init MIC!");
    return;
  }
  Serial.println("Mic initialized.");
  warmupFrames = 0;
}

// 初始化 SPK I2S_SPK_BCLK   I2S_SPK_LRC      I2S_SPK_DOUT
void init_SPK() {
  i2s.setPins(13, 11, 12, -1);
  if (!i2s.begin(I2S_MODE_STD, 24000, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO)) {
    Serial.println("Failed to init SPK!");
    return;
  }
  Serial.println("Speaker initialized.");
}

//--------------------------------------------------------------
// Record and send (using only ZCR for judgment)
// - Voice detection: If the ZCR value is greater than 0.01 for 7 consecutive times, it is determined as the presence of human voice.
//--------------------------------------------------------------
void record_send_audio() {
  static unsigned long lastTime = 0;
  static unsigned long lastVoiceTime = 0;
  static bool isVoiceActive = false;
  static int zcrCount = 0;   // Count of consecutive satisfaction occurrences

  const int IGNORE_FRAMES = 30;   
  unsigned long currentTime = millis();

  if (isOpenMic && millis() - isOpenMic_true_time > 10000 && !isVoiceActive) {
    Serial.println("No voice 10s, sleep.");
    isOpenMic = false;
    i2s.end();
    String jsonString = createJsonString("timeout_no_stream", "");
    webSocket.sendTXT(jsonString);
    return;
  }

  if (currentTime - lastTime >= (1000.0 * SAMPLE_SIZE / SAMPLE_RATE)) {
    lastTime = currentTime;
    int16_t buffer[SAMPLE_SIZE] = {0};
    for (int i = 0; i < SAMPLE_SIZE; ++i) {
      buffer[i] = (int16_t)i2s.read();
    }

    // ---------- Calculate ZCR ----------
    int zeroCrossings = 0;
    int16_t prevSample = buffer[0];
    for (int i = 1; i < SAMPLE_SIZE; i++) {
      int16_t sample = buffer[i];
      if ((sample > 0 && prevSample < 0) || (sample < 0 && prevSample > 0)) {
        zeroCrossings++;
      }
      prevSample = sample;
    }
    float zcr = (float)zeroCrossings / SAMPLE_SIZE;

    Serial.printf("ZCR=%.3f  zcrCount=%d  VoiceActive=%d\n", zcr, zcrCount, isVoiceActive);

    if (warmupFrames < IGNORE_FRAMES) {
      warmupFrames++;
    } else {
      // ---------- Voice detection logic ----------
      if (zcr > 0.01) {
        zcrCount++;
        if (zcrCount >= 3 && !isVoiceActive) {
          isVoiceActive = true;
          lastVoiceTime = currentTime;
          Serial.println(">>> Voice detected");
        }
      } else {
        zcrCount = 0; // Once it is not met, reset to zero.
      }

      if (isVoiceActive && currentTime - lastVoiceTime > 800) {
        Serial.println(">>> Voice ended");
        String jsonString = createJsonString("record_stream", "0x04");
        webSocket.sendTXT(jsonString);
        isVoiceActive = false;
        isOpenMic = false;
        i2s.end();
        isOpenSpk = true;
        init_SPK();
      }
    }

    // ---------- Always send audio ----------
    uint8_t* bytePtr = (uint8_t*)buffer;
    String base64Data = base64::encode(bytePtr, SAMPLE_SIZE * sizeof(int16_t));
    String jsonString = createJsonString("record_stream", base64Data);
    webSocket.sendTXT(jsonString);
  }
}

// Create a JSON data body
String createJsonString(const String& type, const String& data) {
  // Build a JSON object
  StaticJsonDocument<300> jsonDoc;
  // Create a nested JSON structure
  jsonDoc["event"] = type;
  jsonDoc["mac_address"] = macAddress;
  jsonDoc["data"] = data;  // Use data as a sub-item under macAddress
  // Serialize the JSON object into a string
  String jsonString;
  serializeJson(jsonDoc, jsonString);
  return jsonString;  // Return the serialized JSON string
}

// Change status logic
void change_status() {
  if (!isOpenMic && !isOpenSpk) {
      chatDialog.showMicOrSpk(3);
//(starting x-coordinate, starting y-coordinate, image length, image width). The image length and image width can be viewed by right-clicking on the image and selecting "Properties"
  } else if (isOpenMic && !isOpenSpk) {
      chatDialog.showMicOrSpk(1);
  } else if (!isOpenMic && isOpenSpk) {
      chatDialog.showMicOrSpk(2);
  }
}
