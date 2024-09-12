#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

// Definição das variáveis do bot
const char *telegramBotToken = "bottoken"; // Substitua pelo token do seu bot
WiFiClientSecure client;
UniversalTelegramBot bot(telegramBotToken, client);
long botLastCheckTime = 0;  // Para verificar mensagens periodicamente
const int BOT_MTBS = 10000; // Milissegundos entre verificações de mensagens do bot

void initTelegramBot()
{
    client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Configura o certificado raiz do Telegram
    Serial.println("Bot do Telegram inicializado.");
    bot.sendMessage("YOUR_CHAT_ID", "Olá! O ESP32 está agora online.", ""); // Envia uma mensagem de "Olá" para o chat inicial
}

void handleNewMessages(int numNewMessages)
{
    Serial.println("Verificando novas mensagens");
    for (int i = 0; i < numNewMessages; i++)
    {
        String chat_id = String(bot.messages[i].chat_id);
        String text = bot.messages[i].text;

        if (text == "/start")
        {
            String welcome = "Bem-vindo ao bot! Aqui estão os comandos disponíveis:\n";
            welcome += "/status - Verificar status do sistema\n";
            bot.sendMessage(chat_id, welcome, "");
        }
        else if (text == "/status")
        {
            int temperature_test = 10;
            int humidity_test = 10;
            int soil_humidity_test = 10;
            String statusMessage = "Temperatura atual: " + String(temperature_test) + "\n";
            statusMessage += "Umidade atual: " + String(humidity_test) + "\n";
            statusMessage += "Umidade do solo atual: " + String(soil_humidity_test) + "\n";
            bot.sendMessage(chat_id, statusMessage, "");
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
