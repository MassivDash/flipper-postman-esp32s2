/**
 * @file led.cpp
 * @brief LED control functions for RGB LED
 *
 * This file contains functions to initialize and control an RGB LED
 * using PWM (Pulse Width Modulation) on an ESP32 microcontroller.
 */

#include "led.h"
#include <Arduino.h>
#include <driver/ledc.h>
#include <esp_err.h>
#include <esp_log.h>


/// GPIO pin for red LED
#define LED_PIN_RED (6)
/// GPIO pin for green LED
#define LED_PIN_GREEN (5)
/// GPIO pin for blue LED
#define LED_PIN_BLUE (4)

/// LEDC speed mode
#define LEDC_MODE LEDC_LOW_SPEED_MODE

/// Tag for ESP logging
#define TAG "led"

/// Maximum PWM value
#define LED_PWM_MAX_VAL 256U

/// Maximum brightness value for red LED
#define LED_RED_MAX_VAL 20U
/// Maximum brightness value for green LED
#define LED_GREEN_MAX_VAL 20U
/// Maximum brightness value for blue LED
#define LED_BLUE_MAX_VAL 20U

/**
 * @brief Initialize LED PWM channels
 *
 * This function sets up the LEDC timer and channels for controlling the RGB LED.
 */

void led_init() {
  ESP_LOGI(TAG, "init");
  ledc_timer_config_t ledc_timer = {.speed_mode = LEDC_MODE,
                                    .duty_resolution = LEDC_TIMER_8_BIT,
                                    .timer_num = LEDC_TIMER_0,
                                    .freq_hz =
                                        5000, // Set output frequency at 5 kHz
                                    .clk_cfg = LEDC_AUTO_CLK};
  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

  ledc_channel_config_t ledc_channel_red = {
      .gpio_num = LED_PIN_RED,
      .speed_mode = LEDC_MODE,
      .channel = LEDC_CHANNEL_0,
      .intr_type = LEDC_INTR_DISABLE,
      .timer_sel = LEDC_TIMER_0,
      .duty = LED_PWM_MAX_VAL, // Set duty to 100%
      .hpoint = 0};
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_red));

  ledc_channel_config_t ledc_channel_green = {
      .gpio_num = LED_PIN_GREEN,
      .speed_mode = LEDC_MODE,
      .channel = LEDC_CHANNEL_1,
      .intr_type = LEDC_INTR_DISABLE,
      .timer_sel = LEDC_TIMER_0,
      .duty = LED_PWM_MAX_VAL, // Set duty to 100%
      .hpoint = 0};
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_green));

  ledc_channel_config_t ledc_channel_blue = {
      .gpio_num = LED_PIN_BLUE,
      .speed_mode = LEDC_MODE,
      .channel = LEDC_CHANNEL_2,
      .intr_type = LEDC_INTR_DISABLE,
      .timer_sel = LEDC_TIMER_0,
      .duty = LED_PWM_MAX_VAL, // Set duty to 100%
      .hpoint = 0};
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_blue));
  ESP_LOGI(TAG, "init done");
}

/**
 * @brief Set RGB LED color
 *
 * @param red Red component (0-255)
 * @param green Green component (0-255)
 * @param blue Blue component (0-255)
 */
void led_set(uint8_t red, uint8_t green, uint8_t blue) {
  led_set_red(red);
  led_set_green(green);
  led_set_blue(blue);
}

/**
 * @brief Set red LED brightness
 *
 * @param value Brightness value (0-255)
 */
void led_set_red(uint8_t value) {
  uint32_t pwm_value = ((uint32_t)value * LED_RED_MAX_VAL) / 255;
  ESP_ERROR_CHECK(
      ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_0, LED_PWM_MAX_VAL - pwm_value));
  ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_0));
}

/**
 * @brief Set green LED brightness
 *
 * @param value Brightness value (0-255)
 */
void led_set_green(uint8_t value) {
  uint32_t pwm_value = ((uint32_t)value * LED_GREEN_MAX_VAL) / 255;
  ESP_ERROR_CHECK(
      ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_1, LED_PWM_MAX_VAL - pwm_value));
  ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_1));
}

/**
 * @brief Set blue LED brightness
 *
 * @param value Brightness value (0-255)
 */
void led_set_blue(uint8_t value) {
  uint32_t pwm_value = ((uint32_t)value * LED_BLUE_MAX_VAL) / 255;
  ESP_ERROR_CHECK(
      ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_2, LED_PWM_MAX_VAL - pwm_value));
  ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_2));
}

/**
 * @brief Display error indication using red LED
 *
 * This function turns on the red LED for 3 second and then turns it off.
 */
void led_error() {
  led_set_red(255);
  delay(3000);
  led_set_red(0);
}
