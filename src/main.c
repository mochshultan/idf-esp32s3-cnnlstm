#include "config.h"
#include "mpu6050_driver.h"
#include "tflite_inference.h"

// Task handles
static TaskHandle_t mpu6050_task_handle = NULL;
static TaskHandle_t inference_task_handle = NULL;
static TaskHandle_t debug_task_handle = NULL;

// Queue handles
static QueueHandle_t mpu6050_queue = NULL;
static QueueHandle_t inference_queue = NULL;

// Task functions
void mpu6050_task(void* pvParameters);
void inference_task(void* pvParameters);
void debug_task(void* pvParameters);

// System initialization
esp_err_t system_init(void);
esp_err_t create_tasks(void);
esp_err_t create_queues(void);

void app_main(void) {
    DEBUG_PRINT("=== Elderly Fall Detection System ===");
    DEBUG_PRINT("ESP32-S3 with CNN-LSTM Model");
    DEBUG_PRINT("Initializing system...");
    
    // Initialize system
    esp_err_t ret = system_init();
    if (ret != ESP_OK) {
        DEBUG_ERROR("System initialization failed: %s", esp_err_to_name(ret));
        esp_restart();
    }
    
    // Create queues
    ret = create_queues();
    if (ret != ESP_OK) {
        DEBUG_ERROR("Queue creation failed: %s", esp_err_to_name(ret));
        esp_restart();
    }
    
    // Create tasks
    ret = create_tasks();
    if (ret != ESP_OK) {
        DEBUG_ERROR("Task creation failed: %s", esp_err_to_name(ret));
        esp_restart();
    }
    
    DEBUG_PRINT("System initialized successfully");
    DEBUG_PRINT("Starting fall detection monitoring...");
    
    // Main loop - just keep the system running
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        
        // Check system health
        if (mpu6050_task_handle == NULL || inference_task_handle == NULL) {
            DEBUG_ERROR("Critical task died, restarting system");
            esp_restart();
        }
    }
}

esp_err_t system_init(void) {
    DEBUG_PRINT("Initializing system components...");
    
    // Initialize MPU6050
    esp_err_t ret = mpu6050_init();
    if (ret != ESP_OK) {
        DEBUG_ERROR("MPU6050 initialization failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Initialize TensorFlow Lite
    ret = tflite_init();
    if (ret != ESP_OK) {
        DEBUG_ERROR("TensorFlow Lite initialization failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    DEBUG_PRINT("System components initialized successfully");
    return ESP_OK;
}

esp_err_t create_queues(void) {
    DEBUG_PRINT("Creating message queues...");
    
    // Create MPU6050 data queue
    mpu6050_queue = xQueueCreate(MPU6050_QUEUE_SIZE, sizeof(mpu6050_data_t));
    if (mpu6050_queue == NULL) {
        DEBUG_ERROR("Failed to create MPU6050 queue");
        return ESP_ERR_NO_MEM;
    }
    
    // Create inference result queue
    inference_queue = xQueueCreate(INFERENCE_QUEUE_SIZE, sizeof(inference_result_t));
    if (inference_queue == NULL) {
        DEBUG_ERROR("Failed to create inference queue");
        return ESP_ERR_NO_MEM;
    }
    
    DEBUG_PRINT("Message queues created successfully");
    return ESP_OK;
}

esp_err_t create_tasks(void) {
    DEBUG_PRINT("Creating FreeRTOS tasks...");
    
    // Create MPU6050 task
    BaseType_t ret = xTaskCreatePinnedToCore(
        mpu6050_task,
        "MPU6050_Task",
        MPU6050_TASK_STACK_SIZE,
        NULL,
        MPU6050_TASK_PRIORITY,
        &mpu6050_task_handle,
        0  // Run on Core 0
    );
    
    if (ret != pdPASS) {
        DEBUG_ERROR("Failed to create MPU6050 task");
        return ESP_ERR_NO_MEM;
    }
    
    // Create inference task
    ret = xTaskCreatePinnedToCore(
        inference_task,
        "Inference_Task",
        INFERENCE_TASK_STACK_SIZE,
        NULL,
        INFERENCE_TASK_PRIORITY,
        &inference_task_handle,
        1  // Run on Core 1
    );
    
    if (ret != pdPASS) {
        DEBUG_ERROR("Failed to create inference task");
        return ESP_ERR_NO_MEM;
    }
    
    // Create debug task
    ret = xTaskCreatePinnedToCore(
        debug_task,
        "Debug_Task",
        DEBUG_TASK_STACK_SIZE,
        NULL,
        DEBUG_TASK_PRIORITY,
        &debug_task_handle,
        0  // Run on Core 0
    );
    
    if (ret != pdPASS) {
        DEBUG_ERROR("Failed to create debug task");
        return ESP_ERR_NO_MEM;
    }
    
    DEBUG_PRINT("FreeRTOS tasks created successfully");
    return ESP_OK;
}

void mpu6050_task(void* pvParameters) {
    DEBUG_PRINT("MPU6050 task started");
    
    mpu6050_data_t sensor_data;
    TickType_t last_wake_time = xTaskGetTickCount();
    
    while (1) {
        // Read sensor data
        esp_err_t ret = mpu6050_read_data(&sensor_data);
        if (ret != ESP_OK) {
            DEBUG_ERROR("Failed to read MPU6050 data: %s", esp_err_to_name(ret));
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        
        // Add data to buffer for inference
        ret = add_sensor_data_to_buffer(&sensor_data);
        if (ret != ESP_OK) {
            DEBUG_ERROR("Failed to add data to buffer: %s", esp_err_to_name(ret));
        }
        
        // Send data to queue (for other tasks if needed)
        if (xQueueSend(mpu6050_queue, &sensor_data, 0) != pdTRUE) {
            DEBUG_WARN("MPU6050 queue full, dropping data");
        }
        
        // Wait for next sample
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(SAMPLE_INTERVAL_MS));
    }
}

void inference_task(void* pvParameters) {
    DEBUG_PRINT("Inference task started");
    
    inference_result_t result;
    
    while (1) {
        // Wait for data buffer to be full
        if (!g_data_buffer.is_full) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        
        // Run inference
        esp_err_t ret = run_inference(&result);
        if (ret != ESP_OK) {
            DEBUG_ERROR("Inference failed: %s", esp_err_to_name(ret));
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        
        // Process results
        ret = process_inference_result(&result);
        if (ret != ESP_OK) {
            DEBUG_ERROR("Failed to process inference result: %s", esp_err_to_name(ret));
        }
        
        // Send result to queue (for other tasks if needed)
        if (xQueueSend(inference_queue, &result, 0) != pdTRUE) {
            DEBUG_WARN("Inference queue full, dropping result");
        }
        
        // Wait a bit before next inference
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void debug_task(void* pvParameters) {
    DEBUG_PRINT("Debug task started");
    
    TickType_t last_wake_time = xTaskGetTickCount();
    uint32_t debug_counter = 0;
    
    while (1) {
        debug_counter++;
        
        // Print system status every 10 seconds
        if (debug_counter % 10 == 0) {
            DEBUG_PRINT("=== System Status ===");
            DEBUG_PRINT("Uptime: %lu seconds", debug_counter);
            DEBUG_PRINT("Free heap: %lu bytes", esp_get_free_heap_size());
            DEBUG_PRINT("Minimum free heap: %lu bytes", esp_get_minimum_free_heap_size());
            
            // Print data buffer status
            print_data_buffer_status();
            
            // Print last inference result if available
            if (g_last_result.is_valid) {
                DEBUG_PRINT("Last inference: %s (%.3f)", 
                           CLASS_LABELS[g_last_result.predicted_class], 
                           g_last_result.confidence);
            }
            
            DEBUG_PRINT("====================");
        }
        
        // Wait for next debug cycle
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(1000));
    }
}