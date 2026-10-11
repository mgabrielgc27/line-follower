#include <Arduino.h>

class Engine
{
private:
  const uint8_t pin_in1;
  const uint8_t pin_in2;
  const uint8_t pin_en;
  const uint8_t channel;

public:
  Engine(uint8_t pin_in1, uint8_t pin_in2, uint8_t pin_en, uint8_t channel);
  ~Engine();
  void change_speed(int16_t speed);
};
