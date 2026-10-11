#include "IRSensorsManager.h"

IRSensorsManager::IRSensorsManager(const uint8_t pins[number_of_sensors], const int8_t weights[number_of_sensors])
{
  for (size_t i = 0; i < number_of_sensors; i++)
  {    
    sensors[i] = new IRSensor{pins[i], weights[i], false};
  }
  
}

IRSensorsManager::~IRSensorsManager()
{
  for (size_t i = 0; i < number_of_sensors; i++)
    delete sensors[i];
}

void IRSensorsManager::calibrate()
{
  uint16_t reading[number_of_sensors]{};
  read_all(reading);

  for (size_t i = 0; i < number_of_sensors; i++)
  {
    // Se a leitura for maior que o valor atual, substitui o valor atual pela leitura
    darker_values[i] = darker_values[i] > reading[i] ? reading[i] : darker_values[i];
    // Se a leitura for menor que o valor atual, substitui o valor atual pela leitura
    brighter_values[i] = brighter_values[i] < reading[i] ? reading[i] : brighter_values[i];
  }
}

float IRSensorsManager::calc_error()
{
  int sum, weighted_sum = 0;
  uint16_t reading[number_of_sensors]{};
  int16_t weighted_reading[number_of_sensors]{};
  read_all(reading);
  read_all_with_weight(weighted_reading);

  for (size_t i = 0; i < number_of_sensors; i++)
  {
    weighted_sum += weighted_reading[i];
    sum += reading[i];
  }

  return sum == 0 ? 0.0f : (float)weighted_sum / sum;
}

void IRSensorsManager::read_all(uint16_t reading[number_of_sensors])
{
  for (size_t i = 0; i < number_of_sensors; i++)
    reading[i] = sensors[i]->read();
}

void IRSensorsManager::read_all_with_weight(int16_t reading[number_of_sensors])
{
  for (size_t i = 0; i < number_of_sensors; i++)
    reading[i] = sensors[i]->read_with_weight();
}