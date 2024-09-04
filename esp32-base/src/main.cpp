#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "DHT.h"

#define DPIN 15 // DHT11 HW-036)
#define DTYPE DHT11
DHT dht(DPIN, DTYPE);

#define soil_pin 34

#define LED 2
#define RELAY 26
#define BUTTON 13
// REPLACE WITH YOUR RECEIVER MAC Address
uint8_t broadcastAddress[] = {0xA0, 0xA3, 0xB3, 0xAB, 0x5F, 0x7C}; // A0:A3:B3:AB:5F:7C
esp_now_peer_info_t peerInfo;

// Structure to receive
typedef struct pin_message
{
  bool pin_on;
} pin_message;
pin_message pinData;
void OnDataReceived(const uint8_t *mac, const uint8_t *incomingData, int len);

// Structure to send
typedef struct sensor_message
{
  float temperature;
  float humidity;
  int soil_humidity;
} sensor_message;
sensor_message sensorData;
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);

void setup()
{
  Serial.begin(115200);
  Serial.println("Serial Conectado");

  dht.begin();

  WiFi.mode(WIFI_MODE_STA);                 // configura o WIFi para o Modo de estação WiFi
  Serial.print("Endereço MAC ESP32 PIN: "); // EC:64:C9:85:A3:9C
  Serial.println(WiFi.macAddress());        // retorna o endereço MAC do dispositivo

  pinMode(LED, OUTPUT);
  pinMode(RELAY, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
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
  float temperature = dht.readTemperature(); // temperatura C
  // float tf = dht.readTemperature(true); // temperatura F
  float humidity = dht.readHumidity(); // Umidade

  String print_temperature = "Temperatura: " + String(temperature) + "ºC";
  Serial.println(print_temperature);
  String print_humidity = "Umidade: " + String(humidity) + "ºC";
  Serial.println(print_humidity);

  int soil_humidity = analogRead(soil_pin);

  if (soil_humidity == true)
  {
    Serial.print("\nSoil mosture: ");
    Serial.print(analogRead(soil_pin));
  }

  sensorData.temperature = temperature;
  sensorData.humidity = humidity;
  sensorData.soil_humidity = soil_humidity;
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&sensorData, sizeof(sensorData));

  delay(5000);
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
    digitalWrite(RELAY, HIGH);
  }
  else
  {
    digitalWrite(LED, LOW);
    digitalWrite(RELAY, LOW);
  }
}