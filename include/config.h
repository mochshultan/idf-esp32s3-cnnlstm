#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <esp_system.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <driver/i2c.h>
#include <driver/gpio.h>

// Debug configuration
#define DEBUG_ENABLE 1
#define DEBUG_TAG "FALL_DETECTION"

#if DEBUG_ENABLE
#define DEBUG_PRINT(fmt, ...) ESP_LOGI(DEBUG_TAG, fmt, ##__VA_ARGS__)
#define DEBUG_ERROR(fmt, ...) ESP_LOGE(DEBUG_TAG, fmt, ##__VA_ARGS__)
#define DEBUG_WARN(fmt, ...) ESP_LOGW(DEBUG_TAG, fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...)
#define DEBUG_ERROR(fmt, ...)
#define DEBUG_WARN(fmt, ...)
#endif

// MPU6050 Configuration
#define MPU6050_I2C_ADDR 0x68
#define MPU6050_I2C_PORT I2C_NUM_0
#define MPU6050_SDA_PIN 21
#define MPU6050_SCL_PIN 22
#define MPU6050_I2C_FREQ 400000

// Model Configuration
#define INPUT_SEQUENCE_LENGTH 301
#define INPUT_FEATURES 6
#define NUM_CLASSES 5
#define MODEL_INPUT_SIZE (INPUT_SEQUENCE_LENGTH * INPUT_FEATURES)
#define MODEL_OUTPUT_SIZE NUM_CLASSES

// Data collection configuration
#define SAMPLE_RATE_HZ 50
#define SAMPLE_INTERVAL_MS (1000 / SAMPLE_RATE_HZ)
#define BUFFER_SIZE INPUT_SEQUENCE_LENGTH

// Task priorities
#define MPU6050_TASK_PRIORITY 5
#define INFERENCE_TASK_PRIORITY 4
#define DEBUG_TASK_PRIORITY 3

// Task stack sizes
#define MPU6050_TASK_STACK_SIZE 4096
#define INFERENCE_TASK_STACK_SIZE 8192
#define DEBUG_TASK_STACK_SIZE 2048

// Queue sizes
#define MPU6050_QUEUE_SIZE 10
#define INFERENCE_QUEUE_SIZE 5

// Error codes (only define if not already defined by ESP-IDF)
#ifndef ESP_OK
#define ESP_OK 0
#endif

#ifndef ESP_ERR_INVALID_ARG
#define ESP_ERR_INVALID_ARG -1
#endif

#ifndef ESP_ERR_NO_MEM
#define ESP_ERR_NO_MEM -2
#endif

#ifndef ESP_ERR_TIMEOUT
#define ESP_ERR_TIMEOUT -3
#endif

// Class labels
extern const char* CLASS_LABELS[NUM_CLASSES];

#endif // CONFIG_H