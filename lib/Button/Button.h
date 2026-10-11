#include <Arduino.h>

class Button
{
public:
  const uint32_t debounce_time_ms;
  volatile uint32_t last_click_time = 0;

private:
  const uint8_t pin;

public:
  Button(uint8_t pin, uint8_t mode, uint32_t debounce_time_ms);
  Button(uint8_t pin, uint8_t pin_mode, void (*isr)(), int isr_mode, uint32_t debounce_time_ms);
  ~Button();
  int read_value(bool analog);
};
