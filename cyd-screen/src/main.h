#ifndef MAIN_H
#define MAIN_H

#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

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

#endif // MAIN_H