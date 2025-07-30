#include "mpu6050_driver.h"

static const char* TAG = "MPU6050";

esp_err_t mpu6050_i2c_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = MPU6050_SDA_PIN,
        .scl_io_num = MPU6050_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = MPU6050_I2C_FREQ,
    };
    
    esp_err_t ret = i2c_param_config(MPU6050_I2C_PORT, &conf);
    if (ret != ESP_OK) {
        DEBUG_ERROR("Failed to configure I2C parameters: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = i2c_driver_install(MPU6050_I2C_PORT, conf.mode, 0, 0, 0);
    if (ret != ESP_OK) {
        DEBUG_ERROR("Failed to install I2C driver: %s", esp_err_to_name(ret));
        return ret;
    }
    
    DEBUG_PRINT("I2C initialized successfully");
    return ESP_OK;
}

esp_err_t mpu6050_i2c_read_byte(uint8_t reg, uint8_t* data) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MPU6050_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MPU6050_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, data, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(MPU6050_I2C_PORT, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    
    return ret;
}

esp_err_t mpu6050_i2c_write_byte(uint8_t reg, uint8_t data) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MPU6050_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(MPU6050_I2C_PORT, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    
    return ret;
}

esp_err_t mpu6050_i2c_read_bytes(uint8_t reg, uint8_t* data, size_t len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MPU6050_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MPU6050_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, len, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(MPU6050_I2C_PORT, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    
    return ret;
}

bool mpu6050_is_connected(void) {
    uint8_t who_am_i;
    esp_err_t ret = mpu6050_i2c_read_byte(MPU6050_REG_WHO_AM_I, &who_am_i);
    if (ret != ESP_OK) {
        DEBUG_ERROR("Failed to read WHO_AM_I register: %s", esp_err_to_name(ret));
        return false;
    }
    
    if (who_am_i != MPU6050_WHO_AM_I_VALUE) {
        DEBUG_ERROR("Invalid WHO_AM_I value: 0x%02x, expected: 0x%02x", 
                   who_am_i, MPU6050_WHO_AM_I_VALUE);
        return false;
    }
    
    DEBUG_PRINT("MPU6050 found at address 0x%02x", MPU6050_I2C_ADDR);
    return true;
}

esp_err_t mpu6050_reset(void) {
    DEBUG_PRINT("Resetting MPU6050...");
    
    // Set reset bit
    esp_err_t ret = mpu6050_i2c_write_byte(MPU6050_REG_PWR_MGMT_1, 0x80);
    if (ret != ESP_OK) {
        DEBUG_ERROR("Failed to write reset bit: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Wait for reset to complete
    vTaskDelay(pdMS_TO_TICKS(100));
    
    DEBUG_PRINT("MPU6050 reset completed");
    return ESP_OK;
}

esp_err_t mpu6050_wake_up(void) {
    DEBUG_PRINT("Waking up MPU6050...");
    
    // Clear sleep bit
    esp_err_t ret = mpu6050_i2c_write_byte(MPU6050_REG_PWR_MGMT_1, 0x00);
    if (ret != ESP_OK) {
        DEBUG_ERROR("Failed to wake up MPU6050: %s", esp_err_to_name(ret));
        return ret;
    }
    
    vTaskDelay(pdMS_TO_TICKS(10));
    DEBUG_PRINT("MPU6050 woke up successfully");
    return ESP_OK;
}

esp_err_t mpu6050_sleep(void) {
    DEBUG_PRINT("Putting MPU6050 to sleep...");
    
    // Set sleep bit
    esp_err_t ret = mpu6050_i2c_write_byte(MPU6050_REG_PWR_MGMT_1, 0x40);
    if (ret != ESP_OK) {
        DEBUG_ERROR("Failed to put MPU6050 to sleep: %s", esp_err_to_name(ret));
        return ret;
    }
    
    DEBUG_PRINT("MPU6050 is now sleeping");
    return ESP_OK;
}

esp_err_t mpu6050_configure(void) {
    DEBUG_PRINT("Configuring MPU6050...");
    
    // Configure accelerometer (±2g range)
    esp_err_t ret = mpu6050_i2c_write_byte(MPU6050_REG_ACCEL_CONFIG, MPU6050_ACCEL_FS_2G);
    if (ret != ESP_OK) {
        DEBUG_ERROR("Failed to configure accelerometer: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Configure gyroscope (±250°/s range)
    ret = mpu6050_i2c_write_byte(MPU6050_REG_GYRO_CONFIG, MPU6050_GYRO_FS_250);
    if (ret != ESP_OK) {
        DEBUG_ERROR("Failed to configure gyroscope: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Configure low pass filter (5Hz)
    ret = mpu6050_i2c_write_byte(MPU6050_REG_CONFIG, 0x06);
    if (ret != ESP_OK) {
        DEBUG_ERROR("Failed to configure low pass filter: %s", esp_err_to_name(ret));
        return ret;
    }
    
    DEBUG_PRINT("MPU6050 configured successfully");
    return ESP_OK;
}

esp_err_t mpu6050_init(void) {
    DEBUG_PRINT("Initializing MPU6050...");
    
    // Initialize I2C
    esp_err_t ret = mpu6050_i2c_init();
    if (ret != ESP_OK) {
        DEBUG_ERROR("Failed to initialize I2C: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Check if device is connected
    if (!mpu6050_is_connected()) {
        DEBUG_ERROR("MPU6050 not found");
        return ESP_ERR_INVALID_ARG;
    }
    
    // Reset device
    ret = mpu6050_reset();
    if (ret != ESP_OK) {
        DEBUG_ERROR("Failed to reset MPU6050: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Wake up device
    ret = mpu6050_wake_up();
    if (ret != ESP_OK) {
        DEBUG_ERROR("Failed to wake up MPU6050: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Configure device
    ret = mpu6050_configure();
    if (ret != ESP_OK) {
        DEBUG_ERROR("Failed to configure MPU6050: %s", esp_err_to_name(ret));
        return ret;
    }
    
    DEBUG_PRINT("MPU6050 initialized successfully");
    return ESP_OK;
}

esp_err_t mpu6050_read_data(mpu6050_data_t* data) {
    if (data == NULL) {
        DEBUG_ERROR("Invalid data pointer");
        return ESP_ERR_INVALID_ARG;
    }
    
    uint8_t raw_data[14];
    esp_err_t ret = mpu6050_i2c_read_bytes(MPU6050_REG_ACCEL_XOUT_H, raw_data, 14);
    if (ret != ESP_OK) {
        DEBUG_ERROR("Failed to read sensor data: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Convert raw data to float values
    // Accelerometer data (16-bit, ±2g range)
    int16_t accel_x = (raw_data[0] << 8) | raw_data[1];
    int16_t accel_y = (raw_data[2] << 8) | raw_data[3];
    int16_t accel_z = (raw_data[4] << 8) | raw_data[5];
    
    // Temperature data
    int16_t temp = (raw_data[6] << 8) | raw_data[7];
    
    // Gyroscope data (16-bit, ±250°/s range)
    int16_t gyro_x = (raw_data[8] << 8) | raw_data[9];
    int16_t gyro_y = (raw_data[10] << 8) | raw_data[11];
    int16_t gyro_z = (raw_data[12] << 8) | raw_data[13];
    
    // Convert to physical units
    data->accel_x = accel_x / 16384.0f;  // ±2g range
    data->accel_y = accel_y / 16384.0f;
    data->accel_z = accel_z / 16384.0f;
    data->temperature = temp / 340.0f + 36.53f;  // Temperature conversion
    data->gyro_x = gyro_x / 131.0f;  // ±250°/s range
    data->gyro_y = gyro_y / 131.0f;
    data->gyro_z = gyro_z / 131.0f;
    data->timestamp = esp_timer_get_time();
    
    return ESP_OK;
}