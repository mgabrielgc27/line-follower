#include <Arduino.h>

class LedRGB
{
private:
  uint8_t pin_red, pin_blue, pin_green;
  uint8_t channel_red, channel_blue, channel_green;

public:
  LedRGB(uint8_t pin_red, uint8_t pin_blue, uint8_t pin_green, uint8_t channel_red, uint8_t channel_blue, uint8_t channel_green);
  ~LedRGB();
  void write(uint8_t red, uint8_t blue, uint8_t green);
};