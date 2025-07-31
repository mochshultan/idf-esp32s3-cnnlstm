# Setup Guide - Elderly Fall Detection System

## Prerequisites

### 1. Install ESP-IDF
```bash
# Clone ESP-IDF
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf

# Install ESP-IDF
./install.sh esp32s3

# Set up environment
source export.sh
```

### 2. Install PlatformIO (Alternative)
```bash
# Install PlatformIO Core
pip install platformio

# Or install PlatformIO IDE extension in VS Code
```

## Building the Project

### Option 1: Using ESP-IDF
```bash
# Navigate to project directory
cd /path/to/your/project

# Set up ESP-IDF environment
source $HOME/esp/esp-idf/export.sh

# Build the project
idf.py build

# Flash to ESP32-S3
idf.py -p /dev/ttyUSB0 flash monitor
```

### Option 2: Using PlatformIO
```bash
# Navigate to project directory
cd /path/to/your/project

# Build the project
pio run

# Upload to ESP32-S3
pio run --target upload

# Monitor serial output
pio device monitor
```

## Hardware Setup

### MPU6050 Connection
- **SDA**: GPIO 21
- **SCL**: GPIO 22
- **VCC**: 3.3V
- **GND**: GND

### ESP32-S3 Board
- Use Freenove ESP32-S3-WROOM or compatible board
- Ensure PSRAM is available and enabled

## Current Implementation Status

### ✅ Working Features
- MPU6050 sensor driver with I2C communication
- Multi-task FreeRTOS architecture
- Real-time sensor data collection (50Hz)
- Data buffer management (301 samples)
- Comprehensive error handling and debugging
- PSRAM support for ESP32-S3
- System health monitoring

### 🔄 Placeholder Features
- TensorFlow Lite inference (placeholder implementation)
- Model loading and execution
- Actual fall detection classification

### 📋 Next Steps for Full TensorFlow Lite Integration

1. **Install ESP-IDF TensorFlow Lite Component**
   ```bash
   cd $IDF_PATH/components
   git clone https://github.com/espressif/tflite-micro-esp-examples.git
   ```

2. **Convert Your Model to TensorFlow Lite**
   ```python
   import tensorflow as tf
   
   # Load your trained model
   model = tf.keras.models.load_model('your_model.h5')
   
   # Convert to TensorFlow Lite
   converter = tf.lite.TFLiteConverter.from_keras_model(model)
   tflite_model = converter.convert()
   
   # Save the model
   with open('fall_detection_model.tflite', 'wb') as f:
       f.write(tflite_model)
   ```

3. **Generate C Array from Model**
   ```bash
   xxd -i fall_detection_model.tflite > src/fall_detection_model.h
   ```

4. **Update Implementation**
   - Replace placeholder functions in `tflite_inference.c`
   - Uncomment TensorFlow Lite includes in `tflite_inference.h`
   - Update `platformio.ini` to include TensorFlow Lite component

## Troubleshooting

### Build Errors
1. **ESP-IDF not found**: Make sure ESP-IDF is installed and environment is set up
2. **Board not found**: Check if your board is supported in ESP-IDF
3. **Memory errors**: Ensure PSRAM is properly configured

### Runtime Errors
1. **MPU6050 not detected**: Check I2C connections and power supply
2. **Memory access violation**: Verify PSRAM configuration
3. **Task crashes**: Check stack sizes and priorities

### Serial Monitor Output
Expected output after successful upload:
```
=== Elderly Fall Detection System ===
ESP32-S3 with CNN-LSTM Model
Initializing system...
I2C initialized successfully
MPU6050 found at address 0x68
MPU6050 configured successfully
TensorFlow Lite inference placeholder initialized successfully
System initialized successfully
Starting fall detection monitoring...

=== System Status ===
Uptime: 10 seconds
Free heap: 123456 bytes
Data Buffer Status:
  Index: 150/301
  Is Full: No
Last inference: Normal (0.850)
====================
```

## Performance Monitoring

### Memory Usage
- Monitor free heap: `esp_get_free_heap_size()`
- Check minimum free heap: `esp_get_minimum_free_heap_size()`
- Tensor arena usage: 1MB allocated

### Timing
- Sensor sampling: 50Hz (20ms interval)
- Inference time: ~50ms (placeholder)
- Task scheduling: 1kHz FreeRTOS tick

### Debug Information
- Enable debug logging in `config.h`
- Monitor serial output for errors
- Check task status and memory usage

## Customization

### Sensor Configuration
Edit `include/config.h`:
```c
#define MPU6050_SDA_PIN 21
#define MPU6050_SCL_PIN 22
#define SAMPLE_RATE_HZ 50
#define INPUT_SEQUENCE_LENGTH 301
```

### Task Priorities
```c
#define MPU6050_TASK_PRIORITY 5
#define INFERENCE_TASK_PRIORITY 4
#define DEBUG_TASK_PRIORITY 3
```

### Memory Configuration
Edit `sdkconfig.defaults`:
```ini
CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL=16384
CONFIG_SPIRAM_MALLOC_RESERVE_INTERNAL=32768
```

## Support

For issues and questions:
1. Check the troubleshooting section above
2. Review ESP-IDF documentation
3. Check ESP32-S3 datasheet for hardware details
4. Create an issue in the GitHub repository