class PID
{
private:
  float kp, ki, kd;
  float last_error = 0;

public:
  PID(float kp, float ki, float kd);
  float compute(float current_error, float delta);
};
