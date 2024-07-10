#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

#define LED 2
#define BUTTON 13
// REPLACE WITH YOUR RECEIVER MAC Address
uint8_t broadcastAddress[] = {0xA0, 0xA3, 0xB3, 0xAB, 0x5F, 0x7C}; // A0:A3:B3:AB:5F:7C
esp_now_peer_info_t peerInfo;

// Structure to receive
typedef struct pin_message
{
  int pin_id;
  bool pin_on;
} pin_message;
pin_message pinData;
void OnDataReceived(const uint8_t *mac, const uint8_t *incomingData, int len);

// Structure to send
typedef struct sensor_message
{
  int sensor_id;
  float sensor_number;
} sensor_message;
sensor_message sensorData;
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);

void setup()
{
  Serial2.begin(9600);

  WiFi.mode(WIFI_MODE_STA);                  // configura o WIFi para o Modo de estação WiFi
  Serial2.print("Endereço MAC ESP32 PIN: "); // EC:64:C9:85:A3:9C
  Serial2.println(WiFi.macAddress());        // retorna o endereço MAC do dispositivo

  pinMode(LED, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK)
  {
    Serial2.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(OnDataReceived);

  // Once ESPNow is successfully Init, we will register for Send CB to get the status of Transmitted packet
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Failed to add peer");
    return;
  }
}

void loop()
{
  int buttonState = digitalRead(BUTTON);
  if (buttonState == false)
  {
    digitalWrite(LED, HIGH);
    delay(100);
    digitalWrite(LED, LOW);

    sensorData.sensor_id = 1;
    sensorData.sensor_number = 1;
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&sensorData, sizeof(sensorData));
  }
  delay(500);
}

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Sent" : "Not Sent");
}

// callback function that will be executed when data is received
void OnDataReceived(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  memcpy(&pinData, incomingData, sizeof(pinData));

  if (pinData.pin_on == true)
  {
    digitalWrite(LED, HIGH);
  }
  else
  {
    digitalWrite(LED, LOW);
  }
}