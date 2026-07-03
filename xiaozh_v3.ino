#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <driver/i2s.h>
#include <ArduinoJson.h>

// Network Credentials
const char* ssid     ="wifi ssid";
const char* password = "wifi password";

// Wit.ai SERVER Access Token 
const char* witToken = "your sever token ";

// I2S Configuration Pins (ESP32-S3 + INMP441)
#define I2S_WS   4
#define I2S_SCK  5
#define I2S_SD   6

// Audio Recording Parameters
#define SAMPLE_RATE     16000
#define RECORD_TIME_SEC 4
#define BUFFER_SIZE     (SAMPLE_RATE * 2 * RECORD_TIME_SEC) // 16-bit = 2 bytes per sample

int8_t* audioBuffer = nullptr;

void setupI2S() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT, // Left/Mono channel
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 1024,
    .use_apll = false
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
}

void recordAudio() {
  Serial.println("\n>>> Recording started... Speak now!");
  size_t bytesRead = 0;
  size_t totalBytesRead = 0;
  
  memset(audioBuffer, 0, BUFFER_SIZE);

  while (totalBytesRead < BUFFER_SIZE) {
    size_t toRead = 1024;
    if (totalBytesRead + toRead > BUFFER_SIZE) {
      toRead = BUFFER_SIZE - totalBytesRead;
    }
    
    i2s_read(I2S_NUM_0, 
             (void*)(audioBuffer + totalBytesRead), 
             toRead, 
             &bytesRead, 
             portMAX_DELAY);
             
    totalBytesRead += bytesRead;
  }
  Serial.println(">>> Recording finished. Sending to Wit.ai...");
}

void sendAudioToWitAI() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi Disconnected!");
    return;
  }

  WiFiClientSecure client;
  client.setInsecure(); // Skips SSL certificate validation for faster request handling

  HTTPClient http;
  // 'v' parameter is the versioning date required by Wit.ai
  String url = "https://api.wit.ai/speech?v=20260101"; 
  http.begin(client, url);

  // Headers for raw PCM data stream
  http.addHeader("Authorization", "Bearer " + String(witToken));
  http.addHeader("Content-Type", "audio/raw;encoding=signed-integer;bits=16;rate=16000;endian=little");

  int httpResponseCode = http.POST((uint8_t*)audioBuffer, BUFFER_SIZE);

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.print("HTTP Status Code: ");
    Serial.println(httpResponseCode);
    
    // Parsing the JSON response
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, response);
    
    if (!error) {
      const char* recognizedText = doc["text"];
      if (recognizedText) {
        Serial.print("Text Result: ");
        Serial.println(recognizedText);
      } else {
        Serial.println("Audio sent, but no text was recognized.");
      }
    } else {
      Serial.println("Error parsing JSON response.");
    }
  } else {
    Serial.print("Failed to send POST request. Error: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}

void setup() {
  Serial.begin(115200);
  
  // Allocating memory in the ESP32-S3 SRAM
  audioBuffer = (int8_t*)malloc(BUFFER_SIZE);
  if (audioBuffer == nullptr) {
    Serial.println("Critical Error: Internal memory allocation failed!");
    while (1);
  }

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected successfully!");

  setupI2S();
  delay(1000);
}

void loop() {
  // Captures audio and sends it every 15 seconds
  recordAudio();
  sendAudioToWitAI();
  delay(15000); 
}