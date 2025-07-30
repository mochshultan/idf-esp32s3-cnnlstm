#include "tflite_inference.h"
#include "fall_detection_model.h"

static const char* TAG = "TFLITE";

// Global variables
data_buffer_t g_data_buffer = {0};
inference_result_t g_last_result = {0};

// TensorFlow Lite Micro objects
static tflite::MicroErrorReporter micro_error_reporter;
static tflite::ErrorReporter* error_reporter = &micro_error_reporter;
static const tflite::Model* model = nullptr;
static tflite::MicroInterpreter* interpreter = nullptr;
static TfLiteTensor* input = nullptr;
static TfLiteTensor* output = nullptr;

// Tensor arena for model execution
static uint8_t tensor_arena[TENSOR_ARENA_SIZE] __attribute__((aligned(16)));

// Class labels
const char* CLASS_LABELS[NUM_CLASSES] = {
    "Normal",
    "Fall",
    "Near Fall",
    "Sitting",
    "Walking"
};

void* tflite_allocate_tensor_arena(size_t size) {
    if (size > TENSOR_ARENA_SIZE) {
        DEBUG_ERROR("Tensor arena size too large: %zu > %d", size, TENSOR_ARENA_SIZE);
        return nullptr;
    }
    return tensor_arena;
}

esp_err_t tflite_free_tensor_arena(void) {
    // Tensor arena is static, no need to free
    return ESP_OK;
}

esp_err_t tflite_load_model(void) {
    DEBUG_PRINT("Loading TensorFlow Lite model...");
    
    // Map the model into a usable data structure
    model = tflite::GetModel(fall_detection_model);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        DEBUG_ERROR("Model schema mismatch: %d != %d", 
                   model->version(), TFLITE_SCHEMA_VERSION);
        return ESP_ERR_INVALID_ARG;
    }
    
    DEBUG_PRINT("Model loaded successfully, size: %d bytes", fall_detection_model_len);
    return ESP_OK;
}

esp_err_t tflite_setup_interpreter(void) {
    DEBUG_PRINT("Setting up TensorFlow Lite interpreter...");
    
    // This pulls in all the operation implementations we need
    static tflite::AllOpsResolver resolver;
    
    // Build an interpreter to run the model
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, TENSOR_ARENA_SIZE, error_reporter);
    interpreter = &static_interpreter;
    
    // Allocate memory from the tensor_arena for the model's tensors
    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk) {
        DEBUG_ERROR("AllocateTensors() failed: %d", allocate_status);
        return ESP_ERR_NO_MEM;
    }
    
    // Get pointers to the model's input and output tensors
    input = interpreter->input(0);
    output = interpreter->output(0);
    
    if (input == nullptr || output == nullptr) {
        DEBUG_ERROR("Failed to get input/output tensors");
        return ESP_ERR_INVALID_ARG;
    }
    
    DEBUG_PRINT("Input tensor: %dx%dx%dx%d", 
               input->dims->data[0], input->dims->data[1], 
               input->dims->data[2], input->dims->data[3]);
    DEBUG_PRINT("Output tensor: %dx%dx%dx%d", 
               output->dims->data[0], output->dims->data[1], 
               output->dims->data[2], output->dims->data[3]);
    
    return ESP_OK;
}

esp_err_t tflite_inference_init(void) {
    DEBUG_PRINT("Initializing TensorFlow Lite inference...");
    
    // Set up logging
    micro_error_reporter.Initialize();
    
    // Load the model
    esp_err_t ret = tflite_load_model();
    if (ret != ESP_OK) {
        DEBUG_ERROR("Failed to load model: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Set up the interpreter
    ret = tflite_setup_interpreter();
    if (ret != ESP_OK) {
        DEBUG_ERROR("Failed to setup interpreter: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Initialize data buffer
    memset(&g_data_buffer, 0, sizeof(g_data_buffer));
    memset(&g_last_result, 0, sizeof(g_last_result));
    
    DEBUG_PRINT("TensorFlow Lite inference initialized successfully");
    return ESP_OK;
}

esp_err_t tflite_init(void) {
    return tflite_inference_init();
}

esp_err_t add_sensor_data_to_buffer(const mpu6050_data_t* sensor_data) {
    if (sensor_data == NULL) {
        DEBUG_ERROR("Invalid sensor data pointer");
        return ESP_ERR_INVALID_ARG;
    }
    
    // Add sensor data to buffer in the correct order
    uint32_t base_idx = g_data_buffer.index * INPUT_FEATURES;
    if (base_idx + INPUT_FEATURES <= MODEL_INPUT_SIZE) {
        g_data_buffer.data[base_idx + 0] = sensor_data->accel_x;
        g_data_buffer.data[base_idx + 1] = sensor_data->accel_y;
        g_data_buffer.data[base_idx + 2] = sensor_data->accel_z;
        g_data_buffer.data[base_idx + 3] = sensor_data->gyro_x;
        g_data_buffer.data[base_idx + 4] = sensor_data->gyro_y;
        g_data_buffer.data[base_idx + 5] = sensor_data->gyro_z;
        
        g_data_buffer.index++;
        g_data_buffer.last_update = sensor_data->timestamp;
        
        // Check if buffer is full
        if (g_data_buffer.index >= INPUT_SEQUENCE_LENGTH) {
            g_data_buffer.is_full = true;
            g_data_buffer.index = 0;  // Reset for next cycle
            DEBUG_PRINT("Data buffer is full, ready for inference");
        }
    }
    
    return ESP_OK;
}

esp_err_t normalize_sensor_data(float* data, size_t size) {
    if (data == NULL) {
        DEBUG_ERROR("Invalid data pointer");
        return ESP_ERR_INVALID_ARG;
    }
    
    // Simple normalization: scale to [-1, 1] range
    // This should match the normalization used during training
    for (size_t i = 0; i < size; i++) {
        // Apply normalization based on typical sensor ranges
        if (i % 6 < 3) {
            // Accelerometer data: typically ±2g, normalize to ±1
            data[i] = fmaxf(-1.0f, fminf(1.0f, data[i] / 2.0f));
        } else {
            // Gyroscope data: typically ±250°/s, normalize to ±1
            data[i] = fmaxf(-1.0f, fminf(1.0f, data[i] / 250.0f));
        }
    }
    
    return ESP_OK;
}

esp_err_t prepare_input_tensor(float* input_data) {
    if (input_data == NULL) {
        DEBUG_ERROR("Invalid input data pointer");
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!g_data_buffer.is_full) {
        DEBUG_ERROR("Data buffer not full yet");
        return ESP_ERR_INVALID_STATE;
    }
    
    // Copy data from buffer to input tensor
    memcpy(input_data, g_data_buffer.data, MODEL_INPUT_SIZE * sizeof(float));
    
    // Normalize the data
    esp_err_t ret = normalize_sensor_data(input_data, MODEL_INPUT_SIZE);
    if (ret != ESP_OK) {
        DEBUG_ERROR("Failed to normalize sensor data: %s", esp_err_to_name(ret));
        return ret;
    }
    
    return ESP_OK;
}

int get_predicted_class(const float* probabilities) {
    if (probabilities == NULL) {
        DEBUG_ERROR("Invalid probabilities pointer");
        return -1;
    }
    
    int max_idx = 0;
    float max_prob = probabilities[0];
    
    for (int i = 1; i < NUM_CLASSES; i++) {
        if (probabilities[i] > max_prob) {
            max_prob = probabilities[i];
            max_idx = i;
        }
    }
    
    return max_idx;
}

float get_confidence(const float* probabilities) {
    if (probabilities == NULL) {
        DEBUG_ERROR("Invalid probabilities pointer");
        return 0.0f;
    }
    
    int max_idx = get_predicted_class(probabilities);
    if (max_idx < 0) {
        return 0.0f;
    }
    
    return probabilities[max_idx];
}

esp_err_t run_inference(inference_result_t* result) {
    if (result == NULL) {
        DEBUG_ERROR("Invalid result pointer");
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!g_data_buffer.is_full) {
        DEBUG_ERROR("Data buffer not full, cannot run inference");
        return ESP_ERR_INVALID_STATE;
    }
    
    uint64_t start_time = esp_timer_get_time();
    
    // Prepare input data
    float* input_data = (float*)input->data.f;
    esp_err_t ret = prepare_input_tensor(input_data);
    if (ret != ESP_OK) {
        DEBUG_ERROR("Failed to prepare input tensor: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Run inference
    TfLiteStatus invoke_status = interpreter->Invoke();
    if (invoke_status != kTfLiteOk) {
        DEBUG_ERROR("Invoke failed: %d", invoke_status);
        return ESP_ERR_INVALID_STATE;
    }
    
    uint64_t end_time = esp_timer_get_time();
    result->inference_time_us = end_time - start_time;
    
    // Get output probabilities
    float* output_data = (float*)output->data.f;
    memcpy(result->probabilities, output_data, NUM_CLASSES * sizeof(float));
    
    // Process results
    result->predicted_class = get_predicted_class(result->probabilities);
    result->confidence = get_confidence(result->probabilities);
    result->is_valid = true;
    
    DEBUG_PRINT("Inference completed in %llu us", result->inference_time_us);
    
    return ESP_OK;
}

esp_err_t process_inference_result(inference_result_t* result) {
    if (result == NULL || !result->is_valid) {
        DEBUG_ERROR("Invalid inference result");
        return ESP_ERR_INVALID_ARG;
    }
    
    // Store the result
    memcpy(&g_last_result, result, sizeof(inference_result_t));
    
    // Print results
    print_inference_result(result);
    
    // Check for fall detection
    if (result->predicted_class == 1 && result->confidence > 0.7f) {
        DEBUG_ERROR("FALL DETECTED! Confidence: %.3f", result->confidence);
        // Here you can add fall detection actions (alarm, notification, etc.)
    }
    
    return ESP_OK;
}

void print_inference_result(const inference_result_t* result) {
    if (result == NULL || !result->is_valid) {
        DEBUG_ERROR("Cannot print invalid inference result");
        return;
    }
    
    DEBUG_PRINT("=== Inference Result ===");
    DEBUG_PRINT("Predicted Class: %s (%d)", 
               CLASS_LABELS[result->predicted_class], result->predicted_class);
    DEBUG_PRINT("Confidence: %.3f", result->confidence);
    DEBUG_PRINT("Inference Time: %llu us", result->inference_time_us);
    
    DEBUG_PRINT("Class Probabilities:");
    for (int i = 0; i < NUM_CLASSES; i++) {
        DEBUG_PRINT("  %s: %.3f", CLASS_LABELS[i], result->probabilities[i]);
    }
    DEBUG_PRINT("========================");
}

void print_sensor_data(const mpu6050_data_t* data) {
    if (data == NULL) {
        DEBUG_ERROR("Cannot print invalid sensor data");
        return;
    }
    
    DEBUG_PRINT("Accel: X=%.3f, Y=%.3f, Z=%.3f g", 
               data->accel_x, data->accel_y, data->accel_z);
    DEBUG_PRINT("Gyro: X=%.3f, Y=%.3f, Z=%.3f deg/s", 
               data->gyro_x, data->gyro_y, data->gyro_z);
    DEBUG_PRINT("Temp: %.1f°C", data->temperature);
}

void print_data_buffer_status(void) {
    DEBUG_PRINT("Data Buffer Status:");
    DEBUG_PRINT("  Index: %u/%u", g_data_buffer.index, INPUT_SEQUENCE_LENGTH);
    DEBUG_PRINT("  Is Full: %s", g_data_buffer.is_full ? "Yes" : "No");
    DEBUG_PRINT("  Last Update: %llu", g_data_buffer.last_update);
}