#include "IRSensor.h"

IRSensor::IRSensor(uint8_t pin, int8_t weight, bool inverted)
    : pin{pin}, weight{weight}, inverted{inverted}
{
  pinMode(pin, INPUT);
}

uint16_t IRSensor::read()
{
  return (inverted ? 1023 - analogRead(pin) : analogRead(pin));
}

int16_t IRSensor::read_with_weight()
{
  return read() * weight;
}

int8_t IRSensor::get_weight() { return weight; }