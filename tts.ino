#include <Arduino.h>
#include <WiFi.h>
#include <WitAITTS.h>

// --- Configuration ---
const char* ssid     = "";         // Replace with your Wi-Fi Name
const char* password = "";     // Replace with your Wi-Fi Password
const char* witToken = "";  // Replace with your Server Access Token

// --- Hardware Pinout (MAX98357A I2S DAC) ---
// --- Hardware Pinout (MAX98357A I2S DAC) ---
#define I2S_BCLK     40   // Changed from 27 to 40
#define I2S_LRC      41   // Changed from 25 to 41
#define I2S_DOUT     15   // Changed from 26 to 15


WitAITTS tts(I2S_BCLK, I2S_LRC, I2S_DOUT);

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\nInitializing Wit.ai and connecting to Wi-Fi...");

    // FIXED: Passing all 3 required arguments to begin() and fixed parenthesis syntax
    if (tts.begin(ssid, password, witToken)) {
        Serial.println("Wit.ai Initialization Success and Connected to Wi-Fi!");
        
        // Configuration settings (Optional)
        tts.setVoice("wit$Remi"); // Options: wit$Remi, wit$Rebecca, wit$Colin
        tts.setSpeed(100);        // Speed percentage (Normal = 100)
        tts.setPitch(100);        // Pitch percentage (Normal = 100)
        
        // Welcome message
        tts.speak("System initialized. Type your text in the serial monitor.");
    } else {
        Serial.println("Wit.ai Initialization Failed. Check network credentials or Token.");
    }
}

void loop() {
    // CRITICAL: Feeds the audio streaming buffer continuously.
    tts.loop(); 

    // Check for incoming user input from the Serial Monitor
    if (Serial.available()) {
        String text = Serial.readStringUntil('\n');
        text.trim(); // Clean trailing spaces or newlines
        
        if (text.length() > 0) {
            Serial.print("Speaking: ");
            Serial.println(text);
            tts.speak(text); // Request streaming audio from cloud
        }
    }
}
