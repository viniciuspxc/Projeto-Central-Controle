#ifndef SPIFFS_H
#define SPIFFS_H

#include <Arduino.h>
#include <SPIFFS.h>

struct AutoConfig
{
    int auto_temperature;
    int auto_humidity;
    int auto_soil_humidity;
    int auto_active_time;
};

void initSPIFFS(bool writeNew, int auto_temperature, int auto_humidity, int auto_soil_humidity, int auto_active_time);
void writeVariablesToFile(int auto_temperature, int auto_humidity, int auto_soil_humidity, int auto_active_time);
AutoConfig readVariablesFromFile();
void updateVariablesInFile(int auto_temperature, int auto_humidity, int auto_soil_humidity, int auto_active_time);

#endif
