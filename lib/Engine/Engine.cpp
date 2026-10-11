#include "Engine.h"

Engine::Engine(uint8_t pin_in1, uint8_t pin_in2, uint8_t pin_en, uint8_t channel)
    : pin_in1{pin_in1}, pin_in2{pin_in2}, pin_en{pin_en}, channel{channel}
{
  ledcSetup(channel, 5000, 8);
  ledcAttachPin(pin_en, channel);
  pinMode(pin_in1, OUTPUT);
  pinMode(pin_in2, OUTPUT);
  ledcWrite(channel, 0);
}

Engine::~Engine()
{
  ledcDetachPin(pin_en);
}

void Engine::change_speed(int16_t speed)
{
  if (speed == 0)
  {
    digitalWrite(pin_in1, HIGH);
    digitalWrite(pin_in2, HIGH);
    ledcWrite(channel, 255);
    return;
  }
  uint8_t duty = constrain(abs(speed), 0, 255);

  if (speed > 0)
  {
    digitalWrite(pin_in1, HIGH);
    digitalWrite(pin_in2, LOW);
  }
  else
  {
    digitalWrite(pin_in1, LOW);
    digitalWrite(pin_in2, HIGH);
  }

  ledcWrite(channel, duty);
}