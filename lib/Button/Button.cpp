#include "Button.h"

Button::Button(uint8_t pin, uint8_t mode, uint32_t debounce_time_ms)
    : pin{pin}, debounce_time_ms{debounce_time_ms}
{
  pinMode(pin, mode);
}

Button::Button(uint8_t pin, uint8_t pin_mode, void (*isr)(), int isr_mode, uint32_t debounce_time_ms)
    : pin{pin}, debounce_time_ms{debounce_time_ms}
{
  pinMode(pin, pin_mode);
  attachInterrupt(pin, isr, isr_mode);
}

Button::~Button()
{
  detachInterrupt(pin);
}

int Button::read_value(bool analog)
{
  return analog ? analogRead(pin) : digitalRead(pin);
}