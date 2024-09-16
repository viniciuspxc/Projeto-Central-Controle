#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

// #include <main.h>
#include <telegram.h>
#include <spiffsheader.h>

// Definição das variáveis do bot
const char *telegramBotToken = "7502015348:AAFQsXYI0nTMqz6QC2zhJiIpShB0H5totIU"; // Substitua pelo token do seu bot
WiFiClientSecure client;
UniversalTelegramBot bot(telegramBotToken, client);
long botLastCheckTime = 0;  // Para verificar mensagens periodicamente
const int BOT_MTBS = 10000; // Milissegundos entre verificações de mensagens do bot
#define CHAT_ID "6772965271"

void initTelegramBot()
{
    client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Configura o certificado raiz do Telegram
    delay(200);
    bot.sendMessage(CHAT_ID, "init telegram", "");
}

void handleNewMessages(int numNewMessages)
{
    for (int i = 0; i < numNewMessages; i++)
    {
        String chat_id = String(bot.messages[i].chat_id);
        String text = bot.messages[i].text;

        if (text == "start")
        {
            String welcome = "Commands:\n";
            welcome += "status - show automatic values configuration\n";
            welcome += "sensor - show sensor values\n";
            welcome += "update <variable> <value> - update the configuration value\n";
            bot.sendMessage(chat_id, welcome, "");
        }
        else if (text == "status")
        {
            // bot.sendMessage(chat_id, text, "");
            String statusMessage = "Auto temperature: " + String(auto_temperature) + "\n";
            statusMessage += "Auto humidity: " + String(auto_humidity) + "\n";
            statusMessage += "Auto soil_humidity: " + String(auto_soil_humidity) + "\n";
            statusMessage += "Auto active_time: " + String(auto_active_time) + "\n";
            bot.sendMessage(chat_id, statusMessage, "");
        }
        else if (text == "sensor")
        {
            // bot.sendMessage(chat_id, text, "");
            String statusMessage = "Temperature: " + String(temperature) + "\n";
            statusMessage += "Humidity: " + String(humidity) + "\n";
            statusMessage += "Soil_humidity: " + String(soil_humidity) + "\n";
            bot.sendMessage(chat_id, statusMessage, "");
        }
        else if (text.startsWith("update "))
        {
            String command = text.substring(7); // Remove o prefixo "update " para obter o comando
            handleUpdateCommands(chat_id, command);
        }
    }
}

void checkTelegramMessages()
{
    if (millis() > botLastCheckTime + BOT_MTBS)
    {
        int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

        while (numNewMessages)
        {
            handleNewMessages(numNewMessages);
            numNewMessages = bot.getUpdates(bot.last_message_received + 1);
        }

        botLastCheckTime = millis();
    }
}

void handleUpdateCommands(String chat_id, String command)
{
    bot.sendMessage(chat_id, command, "");

    // Divide o comando pelo delimitador de espaço para obter a variável e o valor
    int separatorIndex = command.indexOf(' ');
    if (separatorIndex == -1)
    {
        bot.sendMessage(chat_id, "Comando inválido! Use o formato: update <variável> <valor>", "");
        return;
    }

    String variable = command.substring(0, separatorIndex);
    String valueStr = command.substring(separatorIndex + 1);
    int value = valueStr.toInt(); // Converte o valor para inteiro

    // Atualiza a variável correspondente
    if (variable == "temperature")
    {
        auto_temperature = value;
    }
    else if (variable == "humidity")
    {
        auto_humidity = value;
    }
    else if (variable == "soil_humidity")
    {
        auto_soil_humidity = value;
    }
    else if (variable == "active_time")
    {
        auto_active_time = value;
    }
    else
    {
        bot.sendMessage(chat_id, "Invalid. Use: temperature, humidity, soil_humidity, active_time", "");
        return;
    }

    updateVariablesInFile(auto_temperature, auto_humidity, auto_soil_humidity, auto_active_time);
    bot.sendMessage(chat_id, "update success", "");
}