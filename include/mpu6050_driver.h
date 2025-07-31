#ifndef MPU6050_DRIVER_H
#define MPU6050_DRIVER_H

#include "config.h"
#include "esp_err.h"
#include "driver/i2c.h"
#include "esp_log.h"

// MPU6050 Register addresses
#define MPU6050_REG_PWR_MGMT_1    0x6B
#define MPU6050_REG_PWR_MGMT_2    0x6C
#define MPU6050_REG_CONFIG        0x1A
#define MPU6050_REG_GYRO_CONFIG   0x1B
#define MPU6050_REG_ACCEL_CONFIG  0x1C
#define MPU6050_REG_ACCEL_XOUT_H  0x3B
#define MPU6050_REG_GYRO_XOUT_H   0x43
#define MPU6050_REG_TEMP_OUT_H    0x41
#define MPU6050_REG_WHO_AM_I      0x75

// MPU6050 Configuration values
#define MPU6050_WHO_AM_I_VALUE    0x70
#define MPU6050_ACCEL_FS_2G       0x00
#define MPU6050_ACCEL_FS_4G       0x08
#define MPU6050_ACCEL_FS_8G       0x10
#define MPU6050_ACCEL_FS_16G      0x18
#define MPU6050_GYRO_FS_250       0x00
#define MPU6050_GYRO_FS_500       0x08
#define MPU6050_GYRO_FS_1000      0x10
#define MPU6050_GYRO_FS_2000      0x18

// Data structure for MPU6050 readings
typedef struct {
    float accel_x;
    float accel_y;
    float accel_z;
    float gyro_x;
    float gyro_y;
    float gyro_z;
    float temperature;
    uint64_t timestamp;
} mpu6050_data_t;

// Function declarations
esp_err_t mpu6050_init(void);
esp_err_t mpu6050_read_data(mpu6050_data_t* data);
esp_err_t mpu6050_configure(void);
esp_err_t mpu6050_reset(void);
esp_err_t mpu6050_wake_up(void);
esp_err_t mpu6050_sleep(void);
bool mpu6050_is_connected(void);

// I2C helper functions
esp_err_t mpu6050_i2c_init(void);
esp_err_t mpu6050_i2c_read_byte(uint8_t reg, uint8_t* data);
esp_err_t mpu6050_i2c_write_byte(uint8_t reg, uint8_t data);
esp_err_t mpu6050_i2c_read_bytes(uint8_t reg, uint8_t* data, size_t len);

#endif // MPU6050_DRIVER_H