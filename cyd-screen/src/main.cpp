#include <iostream>
#include <map>

#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include <spiffsheader.h>
#include <telegram.h>
#include <main.h>

const int wifiChannel = 1;
const char *ssid = "esp32connect";
const char *password = "esp32con";

float temperature;
float humidity;
int soil_humidity;
int active_time = 0;

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

std::map<String, boolean> dictTela = {
    {"  ", false},
    {"Tela 2", false}};
String listTela[] = {"Tela 1", "Tela 2"};
String currentTela = "";
int currentIndex = 0;

void buttonTimerCount(void *pvParameters);

void TaskTelegram(void *pvParameters)
{
  while (true)
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("Conectando ao Wi-Fi...");
      WiFi.begin(ssid, password);
      vTaskDelay(5000);
      initTelegramBot();
    }

    checkTelegramMessages(); // Executa em uma tarefa separada
    vTaskDelay(10000);       // Aguarda entre verificações
  }
}

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

  currentTela = "Tela 1";
  currentIndex = 0;

  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);
  // Serial.print("Endereço MAC: ");    // A0:A3:B3:AB:5F:7C
  // Serial.println(WiFi.macAddress()); // retorna o endereço MAC do dispositivo

  Serial.print("Wi-Fi Channel: ");
  Serial.println(WiFi.channel());

  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Failed to add peer");
    return;
  }

  esp_now_register_recv_cb(OnDataReceived);
  esp_now_register_send_cb(OnDataSent);

  // Start the SPI for the touch screen and init the TS library
  mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(mySpi);
  ts.setRotation(1);

  // Start the tft display and set it to black
  tft.init();
  tft.setRotation(1); // This is the display in landscape

  int x = 320 / 2; // center of display
  int y = 100;
  int fontSize = 2;

  // Clear the screen before writing to it
  tft.fillScreen(TFT_BLACK);

  tft.drawCentreString("Conecting to Telegram", x, y, fontSize);
  initTelegramBot();

  tft.fillScreen(TFT_BLACK);
  tft.drawCentreString("Touch Screen to Start", x, y, fontSize);

  xTaskCreatePinnedToCore(SendPinData, "SendPinData", 8192, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(TaskTelegram, "TaskTelegram", 8192, NULL, 1, NULL, 1);
}

void printTouchToDisplay(TS_Point p)
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
    tft.drawCentreString("< " + String(auto_humidity) + " UR", 247, 65, fontSize - 2);
    tft.drawCentreString("< " + String(auto_soil_humidity), 247, 100, fontSize - 2);
    tft.drawCentreString(String(auto_active_time) + " Seconds", 247, 135, fontSize - 2);

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

  // SendPinData(ButtonOn);
  delay(1000);
}

void loop()
{
  // if (!mqttClient.connected())
  // {
  //   reconnectMQTT();
  // }
  // mqttClient.loop();

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
        xTaskCreatePinnedToCore(buttonTimerCount, "buttonTimerCount", 8192, NULL, 1, NULL, 0);
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
          xTaskCreatePinnedToCore(buttonTimerCount, "buttonTimerCount", 8192, NULL, 1, NULL, 0);
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
    delay(1000);
  }
}

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  if (status == ESP_NOW_SEND_SUCCESS)
  {
    tft.fillCircle(310, 10, 5, TFT_GREEN);
    Serial.println("Enviou!");
  }
  else
  {
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

void buttonTimerCount(void *pvParameters)
{
  if (ButtonOn)
  {
    vTaskDelay(200);
    TickType_t xLastWakeTime;
    const TickType_t xDelay = pdMS_TO_TICKS(1000);
    xLastWakeTime = xTaskGetTickCount();

    for (; active_time >= 1; --active_time)
    {
      if (currentTela == "Tela 1")
      {
        // Update display with active_time
        tft.fillRect(200, 130, 30, 30, TFT_BLACK);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        if (active_time > 0)
          tft.drawCentreString(String(active_time), 214, 140, 2);
        else
          tft.drawCentreString("0", 214, 140, 2);
      }
      vTaskDelayUntil(&xLastWakeTime, xDelay);
    }
  }

  ButtonOn = false;
  taskActive = false;
  printTouchToDisplay(ts.getPoint());

  vTaskDelay(200);
  vTaskDelete(NULL);
}