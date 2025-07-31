#ifndef TFLITE_INFERENCE_H
#define TFLITE_INFERENCE_H

#include "config.h"
#include "mpu6050_driver.h"

// TensorFlow Lite Micro includes (placeholder for now)
// In a full implementation, you would include the actual TensorFlow Lite headers
// #include "tensorflow/lite/micro/all_ops_resolver.h"
// #include "tensorflow/lite/micro/micro_error_reporter.h"
// #include "tensorflow/lite/micro/micro_interpreter.h"
// #include "tensorflow/lite/schema/schema_generated.h"

// Model configuration
#define TENSOR_ARENA_SIZE (1024 * 1024)  // 1MB for tensor arena
#define MAX_INFERENCE_TIME_MS 1000

// Inference result structure
typedef struct {
    float probabilities[NUM_CLASSES];
    int predicted_class;
    float confidence;
    uint64_t inference_time_us;
    bool is_valid;
} inference_result_t;

// Data buffer structure
typedef struct {
    float data[MODEL_INPUT_SIZE];
    uint32_t index;
    bool is_full;
    uint64_t last_update;
} data_buffer_t;

// Function declarations
esp_err_t tflite_init(void);
esp_err_t tflite_inference_init(void);
esp_err_t tflite_load_model(void);
esp_err_t tflite_setup_interpreter(void);

// Data processing functions
esp_err_t add_sensor_data_to_buffer(const mpu6050_data_t* sensor_data);
esp_err_t prepare_input_tensor(float* input_data);
esp_err_t normalize_sensor_data(float* data, size_t size);

// Inference functions
esp_err_t run_inference(inference_result_t* result);
esp_err_t process_inference_result(inference_result_t* result);
int get_predicted_class(const float* probabilities);
float get_confidence(const float* probabilities);

// Memory management
void* tflite_allocate_tensor_arena(size_t size);
esp_err_t tflite_free_tensor_arena(void);

// Debug functions
void print_inference_result(const inference_result_t* result);
void print_sensor_data(const mpu6050_data_t* data);
void print_data_buffer_status(void);

// Global variables
extern data_buffer_t g_data_buffer;
extern inference_result_t g_last_result;

#endif // TFLITE_INFERENCE_H