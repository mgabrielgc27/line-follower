#include <Arduino.h>
#include "config.h"
#include <LedRGB.h>
#include <Engine.h>
#include <Button.h>
#include <IRSensorsManager.h>
#include <PID.h>

// Estrutura para empacotar os dados
struct TelemetryData
{
  float error;
  float correction;
  int16_t left_speed;
  int16_t right_speed;
};

enum class State
{
  IDLE,
  RACING,
  CALIBRATING,
  TESTING_ENGINES
};

// Interrupções
void IRAM_ATTR isr_btn_toggle_race();
void IRAM_ATTR isr_btn_toggle_ir_sensors_calibration();
void IRAM_ATTR isr_btn_toggle_engines_test();

// Tasks
void vRaceTask(void *params);
void vIRSensorsCalibrationTask(void *params);
void vEnginesTestTask(void *params);
void vUpdateLedRGBTask(void *params);
void vTelemetryTask(void *params);

// Funções Auxiliares de Estado com Mutex
State get_state();
bool set_state(State new_state);

// Váriaveis Globais de Estado e Sincronização
State state = State::IDLE;
SemaphoreHandle_t xStateMutex = NULL;

// Handle da fila de dados de telemetria
QueueHandle_t xTelemetryQueue = NULL;

TaskHandle_t xRaceTaskHandle = NULL;
TaskHandle_t xIRSensorsCalibrationTaskHandle = NULL;
TaskHandle_t xEnginesTestTaskHandle = NULL;
TaskHandle_t xUpdateLedRGBTaskHandle = NULL;

// Instâncias dos Módulos
const uint8_t ir_sensors_pins[] = {PIN_IR_SENSOR0, PIN_IR_SENSOR1, PIN_IR_SENSOR2, PIN_IR_SENSOR3, PIN_IR_SENSOR4};
const int8_t ir_sensors_weights[] = {WEIGHT_IR_SENSOR0, WEIGHT_IR_SENSOR1, WEIGHT_IR_SENSOR2, WEIGHT_IR_SENSOR3, WEIGHT_IR_SENSOR4};
IRSensorsManager ir_sensors_manager{ir_sensors_pins, ir_sensors_weights};

LedRGB led_rgb{PIN_RED_LED, PIN_BLUE_LED, PIN_GREEN_LED, RED_LED_CHANNEL, BLUE_LED_CHANNEL, GREEN_LED_CHANNEL};

Engine left_engine{LEFT_ENGINE_PIN_IN1, LEFT_ENGINE_PIN_IN2, LEFT_ENGINE_PIN_EN, LEFT_ENGINE_CHANNEL};
Engine right_engine{RIGHT_ENGINE_PIN_IN1, RIGHT_ENGINE_PIN_IN2, RIGHT_ENGINE_PIN_EN, RIGHT_ENGINE_CHANNEL};

PID pid{KP, KI, KD};

Button btn_toggle_race{PIN_BTN_TOGGLE_RACE, INPUT_PULLUP, isr_btn_toggle_race, FALLING, BUTTONS_DEBOUNCE_MS};
Button btn_toggle_ir_sensors_calibration{PIN_BTN_TOGGLE_IR_SENSORS_CALIBRATION, INPUT_PULLUP, isr_btn_toggle_ir_sensors_calibration, FALLING, BUTTONS_DEBOUNCE_MS};
Button btn_toggle_engines_test{PIN_BTN_TOGGLE_ENGINES_TEST, INPUT_PULLUP, isr_btn_toggle_engines_test, FALLING, BUTTONS_DEBOUNCE_MS};

void setup()
{
  Serial.begin(11500);
  analogReadResolution(10);

  xStateMutex = xSemaphoreCreateMutex();

  xTelemetryQueue = xQueueCreate(20, sizeof(TelemetryData));

  xTaskCreatePinnedToCore(vRaceTask, "RACE", 4096, NULL, 3, &xRaceTaskHandle, 0);
  xTaskCreatePinnedToCore(vIRSensorsCalibrationTask, "CALIBRATION", 2048, NULL, 1, &xIRSensorsCalibrationTaskHandle, 1);
  xTaskCreatePinnedToCore(vEnginesTestTask, "ENGINES_TEST", 2048, NULL, 1, &xEnginesTestTaskHandle, 1);
  xTaskCreatePinnedToCore(vUpdateLedRGBTask, "UPDATE_LEDRGB", 2048, NULL, 1, &xUpdateLedRGBTaskHandle, 1);
  xTaskCreatePinnedToCore(vTelemetryTask, "TELEMETRY", 2048, NULL, 1, NULL, 1);

}

void loop() { vTaskDelete(NULL); }

void vRaceTask(void *params)
{
  int16_t base_speed = ENGINE_BASE_SPEED;
  TickType_t last_time = xTaskGetTickCount();
  while (true)
  {
    // Desbloqueia a task quando o botão é clicado
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    if (get_state() == State::IDLE)
    {
      set_state(State::RACING);

      Serial.println(">> Racing: Começa em 1s...");
      // Espera um segundo para começar a correr
      vTaskDelay(pdMS_TO_TICKS(1000));

      // Se o botão for clicado nesse 1s, a corrida é cancelada
      if (ulTaskNotifyTake(pdTRUE, 0) > 0)
      {
        Serial.println("<< Racing: Corrida cancelada durante o 1s.");
        set_state(State::IDLE);
        continue;
      }

      Serial.println(">> Racing: Começou!!");
      last_time = xTaskGetTickCount();

      while (get_state() == State::RACING)
      {
        // Cancela a corrida quando o botão for clicado
        if (ulTaskNotifyTake(pdTRUE, 0) > 0)
        {
          Serial.println("<< Racing: Botão pressionado. Parando...");
          break;
        }

        // Calcula o tempo transcorrido entre as execuções (delta em ms)
        TickType_t now = xTaskGetTickCount();
        float delta = (now - last_time) * portTICK_PERIOD_MS;
        last_time = now;

        // Calcula o erro e a correção
        float error = ir_sensors_manager.calc_error();

        if (error == 5.0f)
        {
          left_engine.change_speed(255);
          right_engine.change_speed(-200); // Puxa pra trás
          vTaskDelay(pdMS_TO_TICKS(10));
          continue; // Pula o cálculo do PID neste loop
        }
        else if (error == -5.0f)
        {
          left_engine.change_speed(-200); // Puxa pra trás
          right_engine.change_speed(255);
          vTaskDelay(pdMS_TO_TICKS(10));
          continue; // Pula o cálculo do PID neste loop
        }

        float correction = pid.compute(error, delta);

        int16_t left_speed = base_speed + (int16_t)correction;
        int16_t right_speed = base_speed - (int16_t)correction;

        // Aplica a velocidade no motor
        left_engine.change_speed(left_speed);
        right_engine.change_speed(right_speed);

        // Empacota os dados
        TelemetryData data = {error, correction, left_speed, right_speed};

        // Manda para a fila
        xQueueSend(xTelemetryQueue, &data, 0);

        // 500Hz
        vTaskDelay(pdMS_TO_TICKS(2));
      }

      left_engine.change_speed(0);
      right_engine.change_speed(0);
      set_state(State::IDLE);

      ulTaskNotifyTake(pdTRUE, 0);
    }
  }
  vTaskDelete(NULL);
}

void vIRSensorsCalibrationTask(void *params)
{
  while (true)
  {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    if (get_state() == State::IDLE)
    {
      set_state(State::CALIBRATING);
      Serial.println(">> CALIBRATING: Calibrando sensores por " + String(CALIBRATION_DURATION_MS / 1000) + "s...");

      TickType_t start_ticks = xTaskGetTickCount();
      TickType_t duration_ticks = pdMS_TO_TICKS(CALIBRATION_DURATION_MS);

      while ((xTaskGetTickCount() - start_ticks) < duration_ticks)
      {
        ir_sensors_manager.calibrate();
        vTaskDelay(pdMS_TO_TICKS(10)); // Lê os sensores continuamente
      }

      Serial.println("<< CALIBRATING: Concluído.");
      set_state(State::IDLE);
      ulTaskNotifyTake(pdTRUE, 0);
    }
  }
  vTaskDelete(NULL);
}

void vEnginesTestTask(void *params)
{
  while (true)
  {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    if (get_state() == State::IDLE)
    {
      set_state(State::TESTING_ENGINES);
      Serial.println(">> TESTING_ENGINES: Testando motores (" + String(ENGINES_TEST_DURATION_MS / 1000) + "s a 100%)...");

      left_engine.change_speed(255);
      right_engine.change_speed(255);

      TickType_t start_ticks = xTaskGetTickCount();
      TickType_t duration_ticks = pdMS_TO_TICKS(ENGINES_TEST_DURATION_MS);
      bool stop_early = false;

      
      while ((xTaskGetTickCount() - start_ticks) < duration_ticks)
      {
        // Se clicar o botão novamente, cancela o teste antes dos 10s
        if (ulTaskNotifyTake(pdTRUE, 0) > 0)
        {
          stop_early = true;
          break;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
      }

      left_engine.change_speed(0);
      right_engine.change_speed(0);

      Serial.println(stop_early ? "<< TESTING_ENGINES: Interrompido pelo usuário." : "<< TESTING_ENGINES: Finalizado.");
      set_state(State::IDLE);
      ulTaskNotifyTake(pdTRUE, 0);
    }
  }
  vTaskDelete(NULL);
}

void vUpdateLedRGBTask(void *params)
{
  while (true)
  {
    State current_state = get_state();
    switch (current_state)
    {
    case State::IDLE:
      led_rgb.write(0, 0, 255);
      break;

    case State::RACING:
      led_rgb.write(0, 255, 0);
      break;

    case State::CALIBRATING:
      led_rgb.write(255, 0, 150);
      vTaskDelay(pdMS_TO_TICKS(200));
      led_rgb.write(0, 0, 0);
      break;

    case State::TESTING_ENGINES:
      led_rgb.write(255, 0, 0);
      vTaskDelay(pdMS_TO_TICKS(100));
      led_rgb.write(0, 0, 0);
      break;
    }

    vTaskDelay(pdMS_TO_TICKS(200));
  }
  vTaskDelete(NULL);
}

void vTelemetryTask(void *params)
{
  TelemetryData data;
  uint8_t print_counter = 0;

  while (true)
  {
    // Fica bloqueada aguardando dados chegarem na fila
    if (xQueueReceive(xTelemetryQueue, &data, portMAX_DELAY) == pdTRUE)
    {

      // Imprime 1 a cada 10 ciclos (50Hz) para não saturar a comunicação serial
      // Os 9 ciclos restantes são lidos da fila e descartados
      if (++print_counter >= 10)
      {
        Serial.printf("Err: %.2f | Corr: %.2f | MotE: %d | MotD: %d\n",
                      data.error, data.correction, data.left_speed, data.right_speed);
        print_counter = 0;
      }
    }
  }
  vTaskDelete(NULL);
}

void IRAM_ATTR isr_btn_toggle_race()
{
  if (xRaceTaskHandle == NULL)
    return;

  uint32_t now = esp_timer_get_time() / 1000;
  if (now - btn_toggle_race.last_click_time > btn_toggle_race.debounce_time_ms)
  {
    btn_toggle_race.last_click_time = now;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(xRaceTaskHandle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}

void IRAM_ATTR isr_btn_toggle_ir_sensors_calibration()
{
  if (xIRSensorsCalibrationTaskHandle == NULL)
    return;

  uint32_t now = esp_timer_get_time() / 1000;
  if (now - btn_toggle_ir_sensors_calibration.last_click_time > btn_toggle_ir_sensors_calibration.debounce_time_ms)
  {
    btn_toggle_ir_sensors_calibration.last_click_time = now;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(xIRSensorsCalibrationTaskHandle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}

void IRAM_ATTR isr_btn_toggle_engines_test()
{
  if (xEnginesTestTaskHandle == NULL)
    return;

  uint32_t now = esp_timer_get_time() / 1000;
  if (now - btn_toggle_engines_test.last_click_time > btn_toggle_engines_test.debounce_time_ms)
  {
    btn_toggle_engines_test.last_click_time = now;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(xEnginesTestTaskHandle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}

State get_state()
{
  State current;
  if (xSemaphoreTake(xStateMutex, portMAX_DELAY) == pdTRUE)
  {
    current = state;
    xSemaphoreGive(xStateMutex);
  }
  return current;
}

bool set_state(State new_state)
{
  bool success = false;
  if (xSemaphoreTake(xStateMutex, portMAX_DELAY) == pdTRUE)
  {
    state = new_state;
    xSemaphoreGive(xStateMutex);
    success = true;
  }
  return success;
}