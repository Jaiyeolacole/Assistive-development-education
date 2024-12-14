#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

// Wi-Fi credentials
const char* ssid = "Your_SSID";  
const char* password = "Your_PASSWORD";

// Host and API Key for LLaMA server (replace with actual)
const char* host = "your-llama-api-host.com"; // Your LLaMA API host URL
const char* apiKey = "YOUR_API_KEY";  // API Key for accessing LLaMA model

WiFiClientSecure client;

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to Wi-Fi");

  if (client.connect(host, 443)) {
    Serial.println("Connected to LLaMA API");
    sendRequestToLlama();
  } else {
    Serial.println("Failed to connect to LLaMA API");
  }
}

void loop() {
  // This can be used for periodic updates or other tasks
}

// Send text input to LLaMA API and get response
void sendRequestToLlama() {
  String textPrompt = "Hello, LLaMA! Can you help me with a task?";
  String postData = "{";
  postData += "\"prompt\": \"" + textPrompt + "\",";
  postData += "\"temperature\": 0.7,";  // Adjust model creativity
  postData += "\"max_tokens\": 100";
  postData += "}";

  // Send HTTP POST request to LLaMA API
  HTTPClient http;
  http.begin(client, "https://" + String(host) + "/v1/generate");  // Endpoint URL for generating text
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + String(apiKey));  // Include API Key

  // Send POST request with the prompt
  int httpResponseCode = http.POST(postData);

  if (httpResponseCode > 0) {
    String response = http.getString();  // Get the response from the server
    Serial.println("Response from LLaMA API:");
    Serial.println(response);
  } else {
    Serial.print("Error in HTTP request, Code: ");
    Serial.println(httpResponseCode);
  }

  http.end();  // Close the HTTP connection
}
