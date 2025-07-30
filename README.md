# Elderly Fall Detection System - ESP32-S3

Sistem deteksi jatuh untuk lansia menggunakan ESP32-S3 dengan model CNN-LSTM TensorFlow Lite dan sensor MPU6050.

## Deskripsi

Proyek ini mengimplementasikan sistem deteksi jatuh real-time menggunakan:
- **ESP32-S3** sebagai platform utama
- **MPU6050** untuk sensor akselerometer dan giroskop
- **TensorFlow Lite Micro** untuk inferensi model CNN-LSTM
- **FreeRTOS** untuk manajemen task dan scheduling

## Model CNN-LSTM

Model yang digunakan memiliki arsitektur:
- **Input**: 301 timesteps × 6 features (ax, ay, az, gx, gy, gz)
- **Output**: 5 classes (Normal, Fall, Near Fall, Sitting, Walking)
- **Total Parameters**: 14,320 (~0.05 MB)

### Arsitektur Model:
```
Conv1D (16 filters) → BatchNorm → MaxPool → Dropout
Conv1D (32 filters) → BatchNorm → MaxPool → Dropout
LSTM (32 units) → LSTM (16 units) → Attention → Dense (32) → Dropout → Dense (5)
```

## Hardware Requirements

- **ESP32-S3 Development Board** (Freenove ESP32-S3-WROOM)
- **MPU6050 Sensor Module**
- **Kabel jumper** untuk koneksi I2C

### Pin Connections:
- **SDA**: GPIO 21
- **SCL**: GPIO 22
- **VCC**: 3.3V
- **GND**: GND

## Software Requirements

- **PlatformIO** atau **ESP-IDF**
- **Python 3.7+** (untuk development)
- **TensorFlow 2.x** (untuk model training)

## Konfigurasi Build

### PlatformIO Configuration (`platformio.ini`):
```ini
[env:freenove_esp32_s3_wroom]
platform = espressif32
board = freenove_esp32_s3_wroom
framework = espidf
monitor_speed = 115200
board_build.partitions = huge_app.csv
board_build.flash_mode = qio
board_build.flash_size = 16MB
board_build.psram_type = opi
```

### Build Flags:
- `CONFIG_TFLITE_ENABLE=1`: Enable TensorFlow Lite
- `CONFIG_MPU6050_ENABLE=1`: Enable MPU6050 support
- `CONFIG_DEBUG_ENABLE=1`: Enable debug logging
- `CONFIG_ESP32S3_SPIRAM_SUPPORT=1`: Enable PSRAM support

## Struktur Proyek

```
├── src/
│   ├── main.c                 # Main application
│   ├── mpu6050_driver.c       # MPU6050 driver implementation
│   ├── tflite_inference.c     # TensorFlow Lite inference
│   ├── fall_detection_model.h # Model data (auto-generated)
│   └── CMakeLists.txt
├── include/
│   ├── config.h              # Configuration constants
│   ├── mpu6050_driver.h      # MPU6050 driver header
│   └── tflite_inference.h    # TensorFlow Lite header
├── platformio.ini            # PlatformIO configuration
└── README.md                 # This file
```

## Fitur Utama

### 1. Real-time Sensor Data Collection
- Sampling rate: 50 Hz
- Buffer size: 301 samples (6 detik data)
- Automatic data normalization

### 2. Multi-task Architecture
- **MPU6050 Task**: Sensor data collection
- **Inference Task**: Model inference (Core 1)
- **Debug Task**: System monitoring

### 3. Robust Error Handling
- Memory access protection
- Task monitoring and restart
- Comprehensive error logging

### 4. Debug Features
- Real-time sensor data display
- Inference result logging
- System health monitoring
- Memory usage tracking

## Cara Build dan Upload

### Menggunakan PlatformIO:
```bash
# Build project
pio run

# Upload to ESP32-S3
pio run --target upload

# Monitor serial output
pio device monitor
```

### Menggunakan ESP-IDF:
```bash
# Setup environment
. $HOME/esp/esp-idf/export.sh

# Build project
idf.py build

# Flash to ESP32-S3
idf.py -p /dev/ttyUSB0 flash monitor
```

## Output Serial Monitor

Setelah upload berhasil, Anda akan melihat output seperti:
```
=== Elderly Fall Detection System ===
ESP32-S3 with CNN-LSTM Model
Initializing system...
I2C initialized successfully
MPU6050 found at address 0x68
MPU6050 configured successfully
TensorFlow Lite inference initialized successfully
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

## Troubleshooting

### 1. Memory Access Violation
- Pastikan PSRAM terkonfigurasi dengan benar
- Periksa tensor arena size
- Gunakan `CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL=16384`

### 2. MPU6050 Connection Issues
- Periksa koneksi I2C (SDA/SCL)
- Pastikan power supply stabil (3.3V)
- Cek address I2C (default: 0x68)

### 3. Model Loading Errors
- Pastikan model file ter-generate dengan benar
- Periksa model schema version
- Verifikasi tensor arena allocation

### 4. Inference Performance
- Monitor inference time
- Optimize task priorities
- Consider model quantization

## Performance Metrics

- **Sampling Rate**: 50 Hz
- **Inference Time**: ~50-100 ms
- **Memory Usage**: ~1MB tensor arena
- **Accuracy**: >90% (tergantung training data)

## Customization

### 1. Model Replacement
1. Train model baru dengan TensorFlow
2. Convert ke TensorFlow Lite
3. Generate C array dengan `xxd` command
4. Replace `fall_detection_model.h`

### 2. Sensor Configuration
- Ubah pin I2C di `config.h`
- Adjust sampling rate
- Modify normalization parameters

### 3. Threshold Tuning
- Fall detection confidence threshold
- Data buffer size
- Task priorities

## License

MIT License - lihat file LICENSE untuk detail.

## Contributing

1. Fork repository
2. Create feature branch
3. Commit changes
4. Push to branch
5. Create Pull Request

## Support

Untuk pertanyaan dan dukungan:
- Buat issue di GitHub
- Email: [your-email@domain.com]
- Dokumentasi: [link-to-docs]