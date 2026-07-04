#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "wifi ssid";
const char* password = "wifi password";
const char* apiKey = "gemini 2.5 api";

// Using the updated gemini-2.5-flash endpoint
const char* url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent?key=";

void setup() {
  Serial.begin(115200);
  delay(2000); 
  
  Serial.println("\n====================================");
  Serial.println("ESP32 Gemini AI Terminal Initialized");
  Serial.println("====================================");
  
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected successfully!");
  Serial.println("\n---> Type your prompt in the box above and press Enter <---");
}

void loop() {
  // Check if you have typed something and pressed Enter
  if (Serial.available() > 0) {
    // Read the string until a newline character is received
    String userPrompt = Serial.readStringUntil('\n');
    userPrompt.trim(); // Remove any accidental invisible spaces/newlines

    if (userPrompt.length() > 0) {
      Serial.print("\nYou asked: ");
      Serial.println(userPrompt);
      
      // Send your custom message to Gemini
      sendGeminiRequest(userPrompt);
      
      Serial.println("\n---> Type another prompt and press Enter <---");
    }
  }
}

void sendGeminiRequest(String prompt) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String fullUrl = String(url) + apiKey;
    
    http.begin(fullUrl); 
    http.addHeader("Content-Type", "application/json");

    // Construct the payload safely
    String jsonPayload = "{\"contents\": [{\"parts\":[{\"text\": \"" + prompt + "\"}]}]}";

    Serial.println("Thinking...");
    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode == 200) {
      String response = http.getString();
      JsonDocument doc; 
      DeserializationError error = deserializeJson(doc, response);

      if (!error) {
        const char* replyText = doc["candidates"][0]["content"]["parts"][0]["text"];
        Serial.println("\n[Gemini Response]:");
        if (replyText) {
          Serial.println(replyText);
        } else {
          Serial.println("Error: Text block not found in API response.");
        }
      } else {
        Serial.print("JSON Parsing failed: ");
        Serial.println(error.c_str());
      }
    } else {
      Serial.print("HTTP Error Code: ");
      Serial.println(httpResponseCode);
      if (httpResponseCode > 0) {
        Serial.println(http.getString()); 
      }
    }
    http.end();
  } else {
    Serial.println("WiFi Link Down. Cannot send.");
  }
}

