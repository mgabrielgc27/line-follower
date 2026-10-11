#include <Arduino.h>

class IRSensor
{
private:
  const uint8_t pin;
  const int8_t weight;
  const bool inverted;
public:
  IRSensor(uint8_t pin, int8_t weight, bool inverted);

  uint16_t read();
  int16_t read_with_weight();
  int8_t get_weight();
};