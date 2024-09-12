#ifndef TELEGRAM_H
#define TELEGRAM_H

#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

extern const char* telegramBotToken;
extern WiFiClientSecure client;
extern UniversalTelegramBot bot;
extern long botLastCheckTime;
extern const int BOT_MTBS;

void initTelegramBot();
void handleNewMessages(int numNewMessages);
void checkTelegramMessages();

#endif // TELEGRAM_H
