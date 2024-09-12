#include <iostream>
#include <map>

#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <esp_now.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <spiffsheader.h>
#include <telegram.h>

#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

const char *ssid = "wifiname";
const char *password = "wifipass";

// REPLACE WITH YOUR RECEIVER MAC Address
uint8_t broadcastAddress[] = {0xEC, 0x64, 0xC9, 0x85, 0xA3, 0x9C}; // EC:64:C9:85:A3:9C
esp_now_peer_info_t peerInfo;

// Structure to send
typedef struct pin_message
{
  bool pin_on;
} pin_message;
pin_message pinData;
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);

void SendPinData(bool buttonState)
{
  // struct gets button state
  pinData.pin_on = buttonState;
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&pinData, sizeof(pinData));
  if (result == ESP_OK)
  {
    // Serial.print("Sent button state: ");
    // Serial.println(buttonState);
  }
  else
  {
    // Serial.println("Error sending the data");
  }
};

// Structure to receive
typedef struct sensor_message
{
  float temperature;
  float humidity;
  int soil_humidity;
} sensor_message;
sensor_message sensorData;
void OnDataReceived(const uint8_t *mac, const uint8_t *incomingData, int len);

float temperature;
float humidity;
int soil_humidity;
int active_time = 0;

int auto_temperature = 30;
int auto_humidity = 50;
int auto_soil_humidity = 4100;
int auto_active_time = 20;
bool writeNew = false; // para iniciar novo arquivo

int last_active_time = auto_active_time;
int last_humidity = auto_humidity;
int last_soil_humidity = auto_soil_humidity;
int last_temperature = auto_temperature;

SPIClass mySpi = SPIClass(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);
TFT_eSPI tft = TFT_eSPI();

int x = 320 / 2; // center of display
int y = 100;
int buttonHoriz = 100;
int buttonVetic = 50;
int fontSize = 5;

boolean ButtonOn = false;
boolean taskActive = false;
SemaphoreHandle_t spiMutex;

std::map<String, boolean> dictTela = {
    {"  ", false},
    {"Tela 2", false}};
String listTela[] = {"Tela 1", "Tela 2"};
String currentTela = "";
int currentIndex = 0;

void loop2(void *pvParameters);

void setup()
{
  Serial.begin(115200);
  SPIFFS.begin(true);

  // Verifica se o arquivo já existe
  if (!SPIFFS.exists("/config.txt") && writeNew)
  {
    Serial.println("Arquivo não encontrado. Criando novo arquivo de configuração.");
    writeVariablesToFile(auto_temperature, auto_humidity, auto_soil_humidity, auto_active_time); // Cria o arquivo e escreve as variáveis
  }
  else
  {
    Serial.println("Arquivo encontrado. Lendo variáveis do arquivo existente.");
    AutoConfig config = readVariablesFromFile(); // Lê as variáveis do arquivo existente e retorna como estrutura
    auto_active_time = config.auto_active_time;
    auto_humidity = config.auto_humidity;
    auto_soil_humidity = config.auto_soil_humidity;
    auto_temperature = config.auto_temperature;
  }

  last_active_time = auto_active_time;
  last_humidity = auto_humidity;
  last_soil_humidity = auto_soil_humidity;
  last_temperature = auto_temperature;

  ButtonOn = false;
  active_time = 0;
  spiMutex = xSemaphoreCreateMutex();

  // Inicializar a tela na "Tela 1"
  currentTela = "Tela 1";
  currentIndex = 0;

  WiFi.mode(WIFI_MODE_STA); // configura o WIFi para o Modo de estação WiFi
  // Serial.print("Endereço MAC: ");    // A0:A3:B3:AB:5F:7C
  // Serial.println(WiFi.macAddress()); // retorna o endereço MAC do dispositivo
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Conectando ao WiFi...");
  }
  Serial.println("Conectado ao WiFi");

  // Inicializa o bot do Telegram
  
  initTelegramBot();

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK)
  {
    // Serial.println("Error initializing ESP-NOW");
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
    // Serial.println("Failed to add peer");
    return;
  }

  // Start the SPI for the touch screen and init the TS library
  mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(mySpi);
  ts.setRotation(1);

  // Start the tft display and set it to black
  tft.init();
  tft.setRotation(1); // This is the display in landscape

  // Clear the screen before writing to it
  tft.fillScreen(TFT_BLACK);

  int x = 320 / 2; // center of display
  int y = 100;
  int fontSize = 2;

  tft.drawCentreString("Touch Screen to Start", x, y, fontSize);
}

void printTouchToDisplay(TS_Point p)
{
  if (xSemaphoreTake(spiMutex, portMAX_DELAY) == pdTRUE)
  {
    // Clear screen first
    tft.fillScreen(TFT_BLACK);

    int x = 320 / 2; // center of display
    int y = 100;
    int fontSize = 4;

    tft.fillTriangle(80, 180, 20, 200, 80, 220, TFT_WHITE);
    tft.fillTriangle(240, 180, 300, 200, 240, 220, TFT_WHITE);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    // tft.drawCentreString(currentTela, x, 190, fontSize);

    if (currentTela == "Tela 1")
    {
      tft.drawCentreString("Central de Controle", x, 20, fontSize);

      String display_text = "Temperatura: " + String(temperature);
      tft.drawCentreString(display_text, x, 65, fontSize - 2);
      display_text = "Umidade do Ar: " + String(humidity);
      tft.drawCentreString(display_text, x, 90, fontSize - 2);
      display_text = "Umidade do solo: " + String(soil_humidity);
      tft.drawCentreString(display_text, x, 115, fontSize - 2);
      if (active_time > 0)
        display_text = "Tempo acionado: " + String(active_time);
      else
        display_text = "Tempo acionado: 0";
      tft.drawCentreString(display_text, x, 140, fontSize - 2);

      if (ButtonOn == false)
      {
        tft.fillRect(x - buttonHoriz / 2, y + 75, buttonHoriz, buttonVetic, TFT_RED);
        tft.setTextColor(TFT_BLACK, TFT_RED);
        tft.drawCentreString("OFF", x, y + 15 + 75, fontSize);
      }
      else
      {
        tft.fillRect(x - buttonHoriz / 2, y + 75, buttonHoriz, buttonVetic, TFT_GREEN);
        tft.setTextColor(TFT_BLACK, TFT_GREEN);
        tft.drawCentreString("ON", x, y + 15 + 75, fontSize);
      }
    }

    else
    { // Tela 2
      tft.drawCentreString("Acionamento", x, 180, fontSize);
      tft.drawCentreString("Automatico", x, 210, fontSize);

      tft.drawCentreString("Temperatura: ", 100, 30, fontSize - 2);
      tft.drawCentreString("Umidade: ", 100, 65, fontSize - 2);
      tft.drawCentreString("Umidade do Solo: ", 105, 100, fontSize - 2);
      tft.drawCentreString("Tempo Acionado: ", 105, 135, fontSize - 2);

      tft.drawCentreString("> " + String(auto_temperature) + " Celsius", 247, 30, fontSize - 2);
      tft.drawCentreString("< " + String(auto_humidity), 247, 65, fontSize - 2);
      tft.drawCentreString("< " + String(auto_soil_humidity), 247, 100, fontSize - 2);
      tft.drawCentreString(String(auto_active_time) + "Seconds", 247, 135, fontSize - 2);

      // Temperatura
      tft.fillTriangle(190, 30, 175, 40, 190, 50, TFT_WHITE);
      tft.fillTriangle(305, 30, 320, 40, 305, 50, TFT_WHITE);

      // Umidade
      tft.fillTriangle(190, 65, 175, 75, 190, 85, TFT_WHITE);
      tft.fillTriangle(305, 65, 320, 75, 305, 85, TFT_WHITE);

      // Umidade do Solo
      tft.fillTriangle(190, 100, 175, 110, 190, 120, TFT_WHITE);
      tft.fillTriangle(305, 100, 320, 110, 305, 120, TFT_WHITE);

      // Tempo Acionado
      tft.fillTriangle(190, 135, 175, 145, 190, 155, TFT_WHITE);
      tft.fillTriangle(305, 135, 320, 145, 305, 155, TFT_WHITE);
    }
    xSemaphoreGive(spiMutex);
  }

  SendPinData(ButtonOn);
  delay(1000);
}

void loop()
{

  checkTelegramMessages();
  if (ButtonOn == false)
  {
    if ((temperature >= auto_temperature && !isnan(temperature) && temperature != 0) ||
        (humidity <= auto_humidity && !isnan(humidity) && humidity != 0) ||
        (soil_humidity >= auto_soil_humidity && !isnan(soil_humidity) && soil_humidity != 0))
    {
      ButtonOn = true;

      if (!taskActive)
      {
        active_time = auto_active_time;
        taskActive = true;
        xTaskCreatePinnedToCore(loop2, "loop2", 8192, NULL, 1, NULL, 0);
      }
    }
  }

  if (ts.tirqTouched() && ts.touched())
  {

    TS_Point p = ts.getPoint();
    float x_tela = p.x / 12.5;       // Para o máximo ser 320
    float y_tela = p.y / 16.6666666; // Para o máximo ser 240

    if (currentTela == "Tela 1" and x_tela > x - buttonHoriz / 2 and x_tela < x + buttonHoriz / 2 and y_tela > y + 75 and y_tela < y + buttonVetic + 75)
    {
      ButtonOn = !ButtonOn;
      if (ButtonOn)
      {
        if (!taskActive)
        {
          active_time = auto_active_time;
          taskActive = true;
          xTaskCreatePinnedToCore(loop2, "loop2", 8192, NULL, 1, NULL, 0);
        }
      }
      else
        active_time = 0;
    }

    int step = 1;
    // Temperatura
    if (currentTela == "Tela 2" and x_tela > 170 and x_tela < 195 and y_tela > 30 and y_tela < 60)
    {
      if (auto_temperature - step >= 0)
      {
        auto_temperature -= step;
      }
    }
    else if (currentTela == "Tela 2" and x_tela > 240 and x_tela < 320 and y_tela > 30 and y_tela < 60)
    {
      auto_temperature += step;
    }

    // Umidade
    if (currentTela == "Tela 2" and x_tela > 170 and x_tela < 195 and y_tela > 40 + 25 and y_tela < 50 + 45)
    {
      if (auto_humidity - step >= 0)
      {
        auto_humidity -= step;
      }
    }
    else if (currentTela == "Tela 2" and x_tela > 240 and x_tela < 320 and y_tela > 40 + 25 and y_tela < 50 + 45)
    {
      auto_humidity += step;
    }

    // Umidade do Solo
    if (currentTela == "Tela 2" and x_tela > 170 and x_tela < 195 and y_tela > 40 + 60 and y_tela < 50 + 80)
    {
      if (auto_soil_humidity - step * 100 >= 0)
      {
        auto_soil_humidity -= step * 100;
      }
    }
    else if (currentTela == "Tela 2" and x_tela > 240 and x_tela < 320 and y_tela > 40 + 60 and y_tela < 50 + 80)
    {
      auto_soil_humidity += step * 100;
    }

    // Tempo Acionado
    if (currentTela == "Tela 2" and x_tela > 170 and x_tela < 195 and y_tela > 40 + 95 and y_tela < 50 + 115)
    {
      if (auto_active_time - 1 > 0)
      {
        auto_active_time -= step;
      }
    }
    else if (currentTela == "Tela 2" and x_tela > 240 and x_tela < 320 and y_tela > 40 + 95 and y_tela < 50 + 115)
    {
      auto_active_time += step;
    }
    else if (x_tela > 20 and x_tela < 100 and y_tela > 180 and y_tela < 220)
    {
      currentIndex = currentIndex - 1;
      if (currentIndex < 0)
        currentIndex = 2;
      currentTela = listTela[currentIndex];
      delay(300);
    }
    else if (x_tela > 220 and x_tela < 300 and y_tela > 180 and y_tela < 220)
    {
      currentIndex = currentIndex + 1;
      if (currentIndex > 1)
        currentIndex = 0;
      currentTela = listTela[currentIndex];
      delay(300);
    }

    if (last_temperature != auto_temperature or last_humidity != auto_humidity or last_soil_humidity != auto_soil_humidity or last_active_time != auto_active_time)
    {
      updateVariablesInFile(auto_temperature, auto_humidity, auto_soil_humidity, auto_active_time);
      last_active_time = auto_active_time;
      last_humidity = auto_humidity;
      last_soil_humidity = auto_soil_humidity;
      last_temperature = auto_temperature;
    }
    printTouchToDisplay(p);
    delay(100);
  }
}

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  if (status == ESP_NOW_SEND_SUCCESS)
  {
    // print green icon on top right
    tft.fillCircle(310, 10, 5, TFT_GREEN);
  }
  else
  {
    // print red icon on top right
    tft.fillCircle(310, 10, 5, TFT_RED);
  }
}

// callback function that will be executed when data is received
void OnDataReceived(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  memcpy(&sensorData, incomingData, sizeof(sensorData));
  temperature = sensorData.temperature;
  humidity = sensorData.humidity;
  soil_humidity = sensorData.soil_humidity;

  printTouchToDisplay(ts.getPoint());
}

void loop2(void *pvParameters)
{
  if (ButtonOn)
  {
    TickType_t xLastWakeTime;
    const TickType_t xDelay = pdMS_TO_TICKS(1000);
    xLastWakeTime = xTaskGetTickCount();

    for (; active_time >= 1; --active_time)
    {
      if (xSemaphoreTake(spiMutex, portMAX_DELAY) == pdTRUE && currentTela == "Tela 1")
      {
        // Update display with active_time
        tft.fillRect(200, 130, 30, 30, TFT_BLACK);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        if (active_time > 0)
          tft.drawCentreString(String(active_time), 214, 140, 2);
        else
          tft.drawCentreString("0", 214, 140, 2);

        xSemaphoreGive(spiMutex); // Release the mutex after SPI operations
      }
      vTaskDelayUntil(&xLastWakeTime, xDelay);
    }
  }

  ButtonOn = false;
  taskActive = false;
  // Serial.println("signal OFF");
  vTaskDelay(200);
  vTaskDelete(NULL);
}
