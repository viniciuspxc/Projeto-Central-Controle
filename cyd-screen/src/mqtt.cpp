#include "mqtt.h"
#include "spiffsheader.h"

const char *mqtt_server = "test.mosquitto.org";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

void init_mqtt()
{
    mqttClient.setServer(mqtt_server, mqtt_port);
    mqttClient.setCallback(callbackMQTT);
}

void reconnectMQTT()
{
    // Loop até se conectar
    while (!mqttClient.connected())
    {
        Serial.print("Attempting MQTT connection...");
        // Tenta se conectar
        if (mqttClient.connect(ID_MQTT))
        {
            Serial.println("connected");
            // Inscreve-se nos tópicos
            mqttClient.subscribe("/receivemessage");
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 5 seconds");
            // Aguarda 5 segundos antes de tentar novamente
            delay(5000);
        }
    }
}

void callbackMQTT(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Message arrived on topic: ");
    Serial.print(topic);
    Serial.print(". Message: ");

    String message;
    for (int i = 0; i < length; i++)
    {
        message += (char)payload[i];
    }
    Serial.println(message);

    if (message == "status")
    {

        String statusMessage = "Auto Temperature: " + String(auto_temperature) + "\n";
        statusMessage += "Auto Humidity: " + String(auto_humidity) + "\n";
        statusMessage += "Auto Soil Humidity: " + String(auto_soil_humidity) + "\n";
        statusMessage += "Auto Active Timer: " + String(auto_active_time) + "\n";
        publishMessage("/sendmessage", statusMessage.c_str());
    }

    // Ecoa a mensagem para o tópico "sendmessage"
    // publishMessage("/sendmessage", message.c_str());
}

void publishMessage(const char *topic, const char *message)
{
    if (!mqttClient.connected())
    {
        reconnectMQTT();
    }
    mqttClient.publish(topic, message);
}
