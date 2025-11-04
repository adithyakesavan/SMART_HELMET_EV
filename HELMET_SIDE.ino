#include <WiFi.h>
#include <WiFiUdp.h>

const char* ssid = "HEL";
const char* password = "123456789";

unsigned int localUdpPort = 4211;
const char* remoteIp = "192.168.4.1";  
const unsigned int remotePort = 4210;  

#define HELMET_PIN 32  
#define ALCOHOL_A0_PIN 34   // Using GPIO 34 for MQ-3 analog output (A0)

WiFiUDP Udp;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  pinMode(HELMET_PIN, INPUT);
  // ALCOHOL_A0_PIN is input by default, no pinMode required

  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nHelmet Connected to WiFi!");

  Udp.begin(localUdpPort);
}

void loop() {
  bool helmetWorn = digitalRead(HELMET_PIN) == LOW;   // LOW means helmet ON

  int alcoholLevel = analogRead(ALCOHOL_A0_PIN);      // Read analog alcohol sensor
  bool alcoholDetected = alcoholLevel > 900;          // Threshold for detection

  // Compose message
  char message[6];

  // If alcohol detected, send H1A1 or H0A1
  if (alcoholDetected) {
    sprintf(message, "H%dA1", helmetWorn ? 1 : 0);
  } else {
    // No alcohol detected, send normal message (A0=0)
    sprintf(message, "H%dA0", helmetWorn ? 1 : 0);
  }

  // Debug prints
  Serial.print("Helmet Sensor Raw: ");
  Serial.print(digitalRead(HELMET_PIN));
  Serial.print(" | Alcohol Level: ");
  Serial.print(alcoholLevel);
  Serial.print(" | Alcohol Detected: ");
  Serial.println(alcoholDetected ? "YES" : "NO");

  Serial.print("Transmitting Message: ");
  Serial.println(message);

  // Send UDP packet
  Udp.beginPacket(remoteIp, remotePort);
  Udp.write((uint8_t*)message, strlen(message));
  Udp.endPacket();

  delay(2000);
}
