#ifndef MQTT_H
#define MQTT_H
#define SEND_TOPIC "/sendmessage"
#define ID_MQTT "YPMV"

#include <PubSubClient.h>
#include <WiFi.h>

extern WiFiClient espClient;
extern PubSubClient mqttClient;
extern const char *mqtt_server;
extern const int mqtt_port;

// Funções para configurar e utilizar o MQTT
void init_mqtt();
void reconnectMQTT();
void callbackMQTT(char *topic, byte *payload, unsigned int length);
void publishMessage(const char *topic, const char *message);

#endif
