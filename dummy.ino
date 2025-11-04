#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESP_Mail_Client.h>

// Wi-Fi AP credentials
const char* ssid = "HEL"; 
const char* password = "123456789";

// Pins
#define RELAY_PIN 5
#define BUZZER_PIN 18
#define LED_PIN 2

// UDP
WiFiUDP udp;
char incoming[16];
unsigned int localPort = 4210;

// Timing variables
unsigned long lastPacketReceived = 0;
const unsigned long timeoutMs = 5000;
unsigned long alcoholAlertStart = 0;
const unsigned long alcoholAlertDuration = 2000; // 2s buzzer

bool alcoholAlertActive = false;
bool emailSent = false;

// Email configuration (replace with your details)
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465
#define AUTHOR_EMAIL "aadii31805@gmail.com"
#define AUTHOR_PASSWORD "Sasikala@25"
#define RECIPIENT_EMAIL "adithyak3108@gmail.com"

SMTPSession smtp;

// Email status callback
void smtpCallback(SMTP_Status status) {
  if (status.success()) {
    Serial.println("Email sent successfully!");
    emailSent = true;
  } else {
    ESP_MAIL_PRINTF("Failed to send email: Status Code: %d, Error Code: %d, Reason: %s\r\n",
                    smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
    emailSent = false;
  }
}

// Email send function
void sendEmail() {
  Session_Config config;
  config.server.host_name = SMTP_HOST;
  config.server.port = SMTP_PORT;
  config.login.email = AUTHOR_EMAIL;
  config.login.password = AUTHOR_PASSWORD;
  config.secure.startTLS = true;

  SMTP_Message message;
  message.sender.name = "ESP32 Alcohol Detector";
  message.sender.email = AUTHOR_EMAIL;
  message.subject = "Alert: Alcohol Detected!";
  message.addRecipient("User", RECIPIENT_EMAIL);
  message.text.content = "Alcohol has been detected by the ESP32 Bike Monitoring system.";
  message.text.charSet = "utf-8";
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

  if (!smtp.connect(&config)) {
    ESP_MAIL_PRINTF("SMTP connection failed: Status Code: %d, Error Code: %d, Reason: %s\r\n",
                    smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
    return;
  }

  if (!MailClient.sendMail(&smtp, &message)) {
    ESP_MAIL_PRINTF("Email sending failed: Status Code: %d, Error Code: %d, Reason: %s\r\n",
                    smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
  }
}

// Setup function
void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

  WiFi.softAP(ssid, password);
  WiFi.setSleep(false);
  Serial.println("Bike AP started.");
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());

  udp.begin(localPort);

  smtp.callback(smtpCallback);
}

// Main loop
void loop() {
  int packetSize = udp.parsePacket();

  if (packetSize) {
    if (packetSize < sizeof(incoming)) {
      int len = udp.read(incoming, sizeof(incoming) - 1);
      if (len > 0) incoming[len] = 0;

      Serial.print("Received: ");
      Serial.println(incoming);

      lastPacketReceived = millis();
      digitalWrite(LED_PIN, HIGH);

      if (strcmp(incoming, "H1A1") == 0) {  // Alcohol detected
        Serial.println("Alcohol detected! Buzzer ON");
        digitalWrite(RELAY_PIN, LOW);    // Motor ON
        digitalWrite(BUZZER_PIN, HIGH);  // Buzzer ON
        alcoholAlertStart = millis();
        alcoholAlertActive = true;

        if (!emailSent) {
          sendEmail();
        }
      }
      else if (!alcoholAlertActive) {
        if (strcmp(incoming, "H1A0") == 0) {  // Helmet ON, no alcohol
          digitalWrite(RELAY_PIN, HIGH);
          digitalWrite(BUZZER_PIN, LOW);
          Serial.println("Motor ON - Helmet worn, no alcohol");
        }
        else if (strcmp(incoming, "H0A0") == 0) {  // Helmet NOT worn
          digitalWrite(RELAY_PIN, LOW);
          digitalWrite(BUZZER_PIN, LOW);
          Serial.println("Motor OFF - Helmet NOT worn");
        }
        else {
          digitalWrite(RELAY_PIN, LOW);
          digitalWrite(BUZZER_PIN, LOW);
          Serial.println("Unknown message, Motor OFF");
        }
      }
    }
  }

  if (alcoholAlertActive && (millis() - alcoholAlertStart >= alcoholAlertDuration)) {
    digitalWrite(BUZZER_PIN, LOW);   // Buzzer OFF after 2s
    digitalWrite(RELAY_PIN, HIGH);   // Motor OFF
    alcoholAlertActive = false;
    Serial.println("Alcohol alert ended, Motor OFF");
  }

  if (millis() - lastPacketReceived > timeoutMs) {
    digitalWrite(LED_PIN, LOW);
  }
}
