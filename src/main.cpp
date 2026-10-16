#include <Arduino.h>
#include "config.h"
#include <Engine.h>
#include <IRSensorsManager.h>
#include <PID.h>
#include <LedRGB.h>

// Handle da Task principal
TaskHandle_t xRaceTaskHandle = NULL;

// Protótipo da Task
void vRaceTask(void *params);

// Instâncias dos Módulos
const uint8_t ir_sensors_pins[] = {PIN_IR_SENSOR0, PIN_IR_SENSOR1, PIN_IR_SENSOR2, PIN_IR_SENSOR3, PIN_IR_SENSOR4};
const int8_t ir_sensors_weights[] = {WEIGHT_IR_SENSOR0, WEIGHT_IR_SENSOR1, WEIGHT_IR_SENSOR2, WEIGHT_IR_SENSOR3, WEIGHT_IR_SENSOR4};
IRSensorsManager ir_sensors_manager{ir_sensors_pins, ir_sensors_weights};

LedRGB led_rgb{PIN_RED_LED, PIN_BLUE_LED, PIN_GREEN_LED, RED_LED_CHANNEL, BLUE_LED_CHANNEL, GREEN_LED_CHANNEL};

Engine left_engine{LEFT_ENGINE_PIN_IN1, LEFT_ENGINE_PIN_IN2, LEFT_ENGINE_PIN_EN, LEFT_ENGINE_CHANNEL};
Engine right_engine{RIGHT_ENGINE_PIN_IN1, RIGHT_ENGINE_PIN_IN2, RIGHT_ENGINE_PIN_EN, RIGHT_ENGINE_CHANNEL};

PID pid{KP, KI, KD};

void setup()
{
  Serial.begin(11500);
  analogReadResolution(10);

  // Criação da Task do Race (Core 0, Prioridade 3)
  xTaskCreatePinnedToCore(vRaceTask, "RACE", 4096, NULL, 3, &xRaceTaskHandle, 0);
}

void loop()
{
  vTaskDelete(NULL);
}

void vRaceTask(void *params)
{
  int16_t base_speed = ENGINE_BASE_SPEED;

  TickType_t start_calib = xTaskGetTickCount();
  TickType_t duration_calib = pdMS_TO_TICKS(5000);

  while ((xTaskGetTickCount() - start_calib) < duration_calib)
  {
    ir_sensors_manager.calibrate();
    vTaskDelay(pdMS_TO_TICKS(10)); // Lê continuamente
  }

  vTaskDelay(pdMS_TO_TICKS(3000));

  TickType_t last_time = xTaskGetTickCount();

  while (true)
  {
    // Calcula o tempo transcorrido entre as execuções (delta em ms)
    TickType_t now = xTaskGetTickCount();
    float delta = (now - last_time) * portTICK_PERIOD_MS;
    last_time = now;

    // Calcula o erro
    float error = ir_sensors_manager.calc_error();

    // Tratamento de perda extrema de linha
    if (error == 5.0f)
    {
      left_engine.change_speed(255);
      right_engine.change_speed(-200); // Puxa para trás
      vTaskDelay(pdMS_TO_TICKS(10));
      continue;
    }
    else if (error == -5.0f)
    {
      left_engine.change_speed(-200); // Puxa para trás
      right_engine.change_speed(255);
      vTaskDelay(pdMS_TO_TICKS(10));
      continue;
    }

    // Cálculo do PID e aplicação da velocidade
    float correction = pid.compute(error, delta);

    int16_t left_speed = base_speed + (int16_t)correction;
    int16_t right_speed = base_speed - (int16_t)correction;

    left_engine.change_speed(left_speed);
    right_engine.change_speed(right_speed);

    // Período do loop (~500Hz)
    vTaskDelay(pdMS_TO_TICKS(2));
  }

  vTaskDelete(NULL);
}