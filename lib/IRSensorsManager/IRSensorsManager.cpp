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
  int sum = 0;
  long weighted_sum = 0;
  int active_count = 0;

  uint16_t raw_reading[number_of_sensors]{};
  read_all(raw_reading);

  for (size_t i = 0; i < number_of_sensors; i++)
  {
    long filtered_value = map(raw_reading[i], darker_values[i], brighter_values[i], 0, 1000);
    filtered_value = constrain(filtered_value, 0, 1000);

    if ((filtered_value > LINE_THRESHOLD) && i != 2)
      active_count++;

    int8_t weight = sensors[i]->get_weight();
    weighted_sum += filtered_value * weight;
    sum += filtered_value;
  }

  // Perdeu a linha: Tracejado ou saída da pista
  if (active_count == 0 || sum == 0)
  {
    // if (last_error > 0.5f) return 5.0f;
    // if (last_error < -0.5f) return -5.0f;
    return 0.0f;
  }
  if (active_count == 2)
  {
    if (weighted_sum < 0)
      return -5.0;
    if (weighted_sum > 0)
      return 5.0;
  }

  last_error = (float)weighted_sum / sum;
  return last_error;
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