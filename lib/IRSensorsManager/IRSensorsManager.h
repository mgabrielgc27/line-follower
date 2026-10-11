#include <Arduino.h>
#include <IRSensor.h>
#include "config.h"

class IRSensorsManager
{
private:
  static const uint8_t number_of_sensors{NUMBER_OF_SENSORS};
  IRSensor *sensors[number_of_sensors];
  uint16_t darker_values[number_of_sensors]{1023, 1023, 1023, 1023, 1023};
  uint16_t brighter_values[number_of_sensors]{0, 0, 0, 0, 0};

public:
  IRSensorsManager(const uint8_t pins[number_of_sensors], const int8_t weights[number_of_sensors]);
  ~IRSensorsManager();

  void calibrate();
  float calc_error();

private:
  void read_all(uint16_t reading[number_of_sensors]);
  void read_all_with_weight(int16_t reading[number_of_sensors]);
};