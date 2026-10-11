#include "LedRGB.h"

LedRGB::LedRGB(uint8_t pin_red, uint8_t pin_blue, uint8_t pin_green, uint8_t channel_red, uint8_t channel_blue, uint8_t channel_green)
    : pin_red{pin_red}, pin_blue{pin_blue}, pin_green{pin_green}, channel_red{channel_red}, channel_blue{channel_blue}, channel_green{channel_green}
{
  if (pin_red && channel_red)
  {
    ledcSetup(pin_red, 5000, 8);
    ledcAttachPin(pin_red, channel_red);
    ledcWrite(channel_red, 0);
  }
  if (pin_blue && channel_blue)
  {
    ledcSetup(pin_blue, 5000, 8);
    ledcAttachPin(pin_blue, channel_blue);
    ledcWrite(channel_blue, 0);
  }
  if (pin_green && channel_green)
  {
    ledcSetup(pin_green, 5000, 8);
    ledcAttachPin(pin_green, channel_green);
    ledcWrite(channel_green, 0);
  }
}

LedRGB::~LedRGB()
{
  ledcDetachPin(pin_red);
  ledcDetachPin(pin_blue);
  ledcDetachPin(pin_green);
}

void LedRGB::write(uint8_t red, uint8_t blue, uint8_t green)
{
  if (channel_red)
    ledcWrite(channel_red, red);
  if (channel_blue)
    ledcWrite(channel_blue, blue);
  if (channel_green)
    ledcWrite(channel_green, green);
}