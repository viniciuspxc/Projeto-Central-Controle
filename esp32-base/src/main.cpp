#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "DHT.h"
#include <esp_wifi.h>
#include "ThingSpeak.h"

const char *ssid = "esp32connect";
const char *password = "esp32con";
#define SECRET_CH_ID 2656697                   // Número do canal ThingSpeak
#define SECRET_WRITE_APIKEY "EW1KUHIHD18QONAL" // Chave de escrita ThingSpeak

WiFiClient client;
unsigned long myChannelNumber = SECRET_CH_ID;
const char *myWriteAPIKey = SECRET_WRITE_APIKEY;

#define DPIN 15 // Pino para o sensor DHT11
#define DTYPE DHT11
DHT dht(DPIN, DTYPE);

#define soil_pin 34

#define LED 2
#define RELAY 26
#define BUTTON 13

uint8_t broadcastAddress[] = {0xA0, 0xA3, 0xB3, 0xAB, 0x5F, 0x7C}; // Endereço MAC do receptor
esp_now_peer_info_t peerInfo;

int buttonTimer = 5;

// Estrutura para receber
typedef struct pin_message
{
  bool pin_on;
} pin_message;
pin_message pinData;
void OnDataReceived(const uint8_t *mac, const uint8_t *incomingData, int len);

// Estrutura para enviar
typedef struct sensor_message
{
  float temperature;
  float humidity;
  int soil_humidity;
} sensor_message;
sensor_message sensorData;
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);

void readSensorData(void *pvParameters);
void sendSensorData(void *pvParameters);
void sendThingSpeak(void *pvParameters);

float temperature;
float humidity;
int soil_humidity;

void setup()
{
  Serial.begin(115200);
  dht.begin();

  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);

  pinMode(LED, OUTPUT);
  pinMode(RELAY, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);

  // Inicializa o ESP-NOW
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(OnDataReceived);
  esp_now_register_send_cb(OnDataSent);

  // Registra o peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Adiciona o peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Failed to add peer");
    return;
  }

  // Inicializa ThingSpeak
  ThingSpeak.begin(client);

  xTaskCreatePinnedToCore(readSensorData, "readSensorData", 8192, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(sendSensorData, "sendSensorData", 8192, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(sendThingSpeak, "sendThingSpeak", 8192, NULL, 1, NULL, 1);
}

void loop()
{
  while (buttonTimer > 0)
  {
    delay(1000);
    buttonTimer += -1;
  }
  if (buttonTimer == 0)
  {
    digitalWrite(LED, LOW);
    digitalWrite(RELAY, LOW);
  }
  delay(1000);
}

// callback para quando os dados são enviados
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Sent" : "Not Sent");
}

// callback para quando os dados são recebidos
void OnDataReceived(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  memcpy(&pinData, incomingData, sizeof(pinData));

  if (pinData.pin_on == true)
  {
    digitalWrite(LED, HIGH);
    digitalWrite(RELAY, HIGH);
    buttonTimer = 10;
  }
  else
  {
    digitalWrite(LED, LOW);
    digitalWrite(RELAY, LOW);
    buttonTimer = 0;
  }
}

void readSensorData(void *pvParameters)
{
  float last_temperature = 0;
  float last_humidity = 0;
  int last_soil_humidity = 0;
  while (true)
  {
    temperature = dht.readTemperature();   // temperatura C
    humidity = dht.readHumidity();         // Umidade
    soil_humidity = analogRead(soil_pin);  // Umidade do solo
    vTaskDelay(5000 / portTICK_PERIOD_MS); // Espera 5 segundos

    if (isnan(temperature) || isnan(humidity))
    {
      temperature = last_temperature;
      humidity = last_humidity;
    }
    else
    {
      last_temperature = temperature;
      last_humidity = humidity;
    }
  }
}

void sendSensorData(void *pvParameters)
{
  while (true)
  {
    Serial.printf("Temperatura: %.2f°C, Umidade: %.2f%%, Umidade do Solo: %d\n", temperature, humidity, soil_humidity);
    sensorData.temperature = temperature;
    sensorData.humidity = humidity;
    sensorData.soil_humidity = soil_humidity;

    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&sensorData, sizeof(sensorData));
    vTaskDelay(5000 / portTICK_PERIOD_MS); // Espera 5 segundos
  }
}

void sendThingSpeak(void *pvParameters)
{
  while (true)
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("Conectando ao Wi-Fi...");
      WiFi.begin(ssid, password);
      vTaskDelay(5000);
      continue;
    }

    if (!isnan(temperature) && !isnan(humidity))
    {
      ThingSpeak.setField(1, temperature);   // Campo 1: Temperatura
      ThingSpeak.setField(2, humidity);      // Campo 2: Umidade
      ThingSpeak.setField(3, soil_humidity); // Campo 3: Umidade do Solo

      // Envia os dados ao ThingSpeak
      int httpCode = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

      if (httpCode == 200)
      {
        Serial.println("Dados enviados com sucesso ao ThingSpeak.");
      }
      else
      {
        Serial.printf("Erro ao enviar os dados ao ThingSpeak. Código HTTP: %d\n", httpCode);
      }
      vTaskDelay(300000 / portTICK_PERIOD_MS);
    }
    else
    {
      vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
  }
}
