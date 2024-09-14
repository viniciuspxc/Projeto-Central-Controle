// spiffs.cpp
#include "spiffsheader.h"

int auto_temperature = 30;
int auto_humidity = 50;
int auto_soil_humidity = 4100;
int auto_active_time = 20;
bool writeNew = false; // para iniciar novo arquivo

AutoConfig readVariablesFromFile()
{
    AutoConfig config;
    File file = SPIFFS.open("/config.txt", FILE_READ);
    if (!file)
    {
        return config;
    }

    config.auto_temperature = file.parseInt();
    config.auto_humidity = file.parseInt();
    config.auto_soil_humidity = file.parseInt();
    config.auto_active_time = file.parseInt();
    file.close();
    Serial.println("Variáveis lidas do arquivo:");
    Serial.printf("Temperatura: %d, Umidade: %d, Umidade do Solo: %d, Tempo Ativo: %d\n",
                  config.auto_temperature, config.auto_humidity, config.auto_soil_humidity, config.auto_active_time);
    return config;
}

void writeVariablesToFile(int auto_temperature, int auto_humidity, int auto_soil_humidity, int auto_active_time)
{
    File file = SPIFFS.open("/config.txt", FILE_WRITE);
    if (!file)
    {
        Serial.println("Erro ao criar o arquivo!");
        return;
    }
    file.printf("%d\n%d\n%d\n%d\n", auto_temperature, auto_humidity, auto_soil_humidity, auto_active_time);
    file.close();
    Serial.println("Variáveis salvas no arquivo.");
}

void updateVariablesInFile(int auto_temperature, int auto_humidity, int auto_soil_humidity, int auto_active_time)
{
    File file = SPIFFS.open("/config.txt", FILE_WRITE);
    if (!file)
    {
        Serial.println("Erro ao abrir o arquivo para atualização!");
        return;
    }
    file.printf("%d\n%d\n%d\n%d\n", auto_temperature, auto_humidity, auto_soil_humidity, auto_active_time);
    file.close();
    Serial.println("Variáveis atualizadas no arquivo.");
}
