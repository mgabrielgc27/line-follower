#include "PID.h"

PID::PID(float kp, float ki, float kd)
    : kp{kp}, ki{ki}, kd{kd}
{
}

float PID::compute(float current_error, float delta)
{
  if (delta <= 0.0001f)
    delta = 0.0001f;
  float correction = kp * current_error + kd * (current_error - last_error) / delta;
  last_error = current_error;
  return correction;
}