#ifndef TELEGRAM_H
#define TELEGRAM_H

#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

void initTelegramBot();
void handleNewMessages(int numNewMessages);
void checkTelegramMessages();
void handleUpdateCommands(String chat_id, String command);

#endif // TELEGRAM_H
