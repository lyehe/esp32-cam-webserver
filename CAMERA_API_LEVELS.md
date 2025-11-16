# Camera API Levels - Complete Guide

## Overview

The ESP32-CAM webserver provides **three levels** of camera control, from safe high-level abstractions to raw hardware access.

```
┌────────────────────────────────────────────────────┐
│  Level 1: CameraSettings (High-Level)             │
│  ✅ Recommended for most use cases                 │
│  ✅ Type-safe, validated, persistent               │
├────────────────────────────────────────────────────┤
│  Level 2: camera_config_t (Mid-Level)             │
│  ⚠️  Direct ESP32-Camera API                       │
│  ⚠️  More control, less safety                     │
├────────────────────────────────────────────────────┤
│  Level 3: sensor_t* (Low-Level)                   │
│  ⚠️  Direct sensor register access                 │
│  ⚠️  Complete control, can brick sensor!           │
└────────────────────────────────────────────────────┘
```

---

## Level 1: High-Level API (CameraSettings)

### When to Use
- ✅ **99% of use cases** - Start here!
- ✅ Production applications
- ✅ When you need validated, safe settings
- ✅ When settings should persist across reboots
- ✅ Multi-client streaming
- ✅ RESTful API control

### Features
- Type-safe value objects (Resolution, PixelFormat)
- Automatic validation
- SPIFFS persistence
- Clean Architecture compliance
- Error handling with Result<T>

### Example

```cpp
#include "application/ApplicationFacade.h"

void setup() {
    auto& app = ApplicationFacade::getInstance();
    app.initialize();

    // Get current settings
    CameraSettings settings = app.camera().getCurrentSettings();

    // Modify settings (type-safe)
    settings.setResolution(Resolution::VGA());
    settings.setQuality(12);  // 0-63
    settings.setBrightness(1);  // -2 to +2
    settings.setContrast(1);
    settings.setSaturation(0);
    settings.setSharpness(-1);
    settings.setAutoWhiteBalance(true);
    settings.setAutoExposureControl(true);
    settings.setGammaCorrection(true);
    settings.setLensCorrectionEnabled(true);

    // Apply settings (validated + persisted)
    auto result = app.camera().updateSettings(settings);

    if (result.isOk()) {
        Serial.println("✅ Settings applied successfully");
    }
}
```

### Available Controls (Level 1)

| Category | Controls | Range |
|----------|----------|-------|
| **Resolution** | QVGA → UXGA | 14 presets |
| **Format** | JPEG, RGB565, YUV422, etc. | 8 formats |
| **Quality** | JPEG compression | 0-63 |
| **Color** | Brightness, Contrast, Saturation | -2 to +2 |
| **Sharpness** | Edge enhancement | -2 to +2 |
| **Auto** | AWB, AEC, AEC DSP, AGC | On/Off |
| **Effects** | Negative, Grayscale, Sepia, etc. | 7 effects |
| **Orientation** | H-Mirror, V-Flip | On/Off |
| **Corrections** | Gamma, Lens, WPC, BPC | On/Off |
| **Advanced** | DCW, Exposure, Gain, etc. | Varies |

### Preset Optimizations

```cpp
// For low-light conditions
app.camera().optimizeForLowLight();

// For maximum speed (30+ FPS)
app.camera().optimizeForSpeed();

// For maximum quality
app.camera().optimizeForQuality();
```

---

## Level 2: Mid-Level API (camera_config_t)

### When to Use
- ⚠️ Advanced users only
- ⚠️ Custom hardware configurations
- ⚠️ Non-AI-Thinker boards
- ⚠️ Testing different XCLK frequencies
- ⚠️ Custom frame buffer strategies

### Features
- Direct access to ESP32-Camera driver
- Configure pin assignments
- Control PSRAM usage
- Modify XCLK frequency
- Advanced grab modes

### Example

```cpp
#include "infrastructure/camera/LowLevelCameraAPI.h"

void setupCustomCamera() {
    // Get base config
    camera_config_t config = LowLevelCameraAPI::getHardwareConfig();

    // Modify for custom board
    config.pin_d0 = 4;   // Different pin assignment
    config.pin_d1 = 5;
    // ... other pins ...

    // Change XCLK frequency (10-20 MHz typical)
    config.xclk_freq_hz = 15000000;  // 15 MHz for better stability

    // Force double buffering
    config.fb_count = 2;
    config.fb_location = CAMERA_FB_IN_PSRAM;

    // Latest frame grab mode (discard old frames)
    config.grab_mode = CAMERA_GRAB_LATEST;

    // Reinitialize camera
    if (LowLevelCameraAPI::reinitialize(config)) {
        Serial.println("✅ Custom config applied");
    }
}
```

### camera_config_t Fields

| Field | Purpose | Typical Value |
|-------|---------|---------------|
| `xclk_freq_hz` | Sensor clock frequency | 10-20 MHz |
| `fb_count` | Frame buffer count | 1-2 |
| `fb_location` | PSRAM vs DRAM | `CAMERA_FB_IN_PSRAM` |
| `grab_mode` | Buffer strategy | `CAMERA_GRAB_LATEST` |
| `pixel_format` | Output format | `PIXFORMAT_JPEG` |
| `frame_size` | Resolution | `FRAMESIZE_SVGA` |
| `jpeg_quality` | Compression | 0-63 |

**Warning:** Incorrect configuration can prevent camera initialization!

---

## Level 3: Low-Level API (sensor_t*)

### When to Use
- ⚠️ **Expert users only!**
- ⚠️ Accessing undocumented features
- ⚠️ Custom image processing algorithms
- ⚠️ Research and experimentation
- ⚠️ Debugging hardware issues

### Risks
- ❌ Can brick sensor with wrong register values
- ❌ No validation or safety checks
- ❌ Settings not persisted
- ❌ Sensor-specific (OV2640 ≠ OV3660)

### Features
- Direct sensor register read/write
- Access to all hardware features
- Custom JPEG compression parameters
- Night mode, binning, test patterns
- Raw exposure/gain control

### Example 1: Night Mode

```cpp
#include "infrastructure/camera/LowLevelCameraAPI.h"

void enableNightVision() {
    sensor_t* sensor = LowLevelCameraAPI::getSensor();

    if (!sensor) {
        Serial.println("❌ Sensor not available");
        return;
    }

    // Disable auto controls
    sensor->set_exposure_ctrl(sensor, 0);  // Manual exposure
    sensor->set_gain_ctrl(sensor, 0);      // Manual gain
    sensor->set_awb_gain(sensor, 0);       // Disable AWB

    // Maximize light gathering
    LowLevelCameraAPI::setExposureRaw(1200);  // High exposure
    LowLevelCameraAPI::setGainRaw(30);        // Max gain
    LowLevelCameraAPI::setBinning(true);      // Combine pixels

    // OV2640-specific night mode
    if (LowLevelCameraAPI::getSensorModel() == "OV2640") {
        LowLevelCameraAPI::setNightMode(true);
    }

    Serial.println("🌙 Night vision enabled");
}
```

### Example 2: Register-Level Access

```cpp
void customSaturationBoost() {
    // Direct register manipulation (OV2640)
    // WARNING: Values are sensor-specific!

    LowLevelCameraAPI::setRegister(0xFF, 0x00);  // Select DSP bank
    LowLevelCameraAPI::setRegister(0x7C, 0x00);  // SDE control
    LowLevelCameraAPI::setRegister(0x7D, 0x02);  // Enable saturation
    LowLevelCameraAPI::setRegister(0x7C, 0x03);  // Saturation U
    LowLevelCameraAPI::setRegister(0x7D, 0xC0);  // Ultra-high saturation
    LowLevelCameraAPI::setRegister(0x7D, 0xC0);  // Ultra-high saturation V

    Serial.println("🎨 Ultra saturation enabled");
}
```

### Example 3: Performance Profiling

```cpp
void benchmarkCamera() {
    Serial.println("📊 Benchmarking camera performance...");

    // Test current settings
    LowLevelCameraAPI::profileCapture(100);

    // Test with different quality
    sensor_t* sensor = LowLevelCameraAPI::getSensor();

    Serial.println("\n=== Testing Quality 10 ===");
    sensor->set_quality(sensor, 10);
    LowLevelCameraAPI::profileCapture(100);

    Serial.println("\n=== Testing Quality 20 ===");
    sensor->set_quality(sensor, 20);
    LowLevelCameraAPI::profileCapture(100);

    Serial.println("\n=== Testing Quality 30 ===");
    sensor->set_quality(sensor, 30);
    LowLevelCameraAPI::profileCapture(100);

    // Restore original
    sensor->set_quality(sensor, 12);
}
```

### Example 4: Diagnostic Tools

```cpp
void diagnosticSuite() {
    // Get sensor info
    String model = LowLevelCameraAPI::getSensorModel();
    Serial.println("📷 Sensor: " + model);

    // Full status
    String status = LowLevelCameraAPI::getSensorStatus();
    Serial.println(status);

    // Enable test pattern
    LowLevelCameraAPI::setTestPattern(1);  // Color bars
    delay(5000);
    LowLevelCameraAPI::setTestPattern(0);  // Disable

    // Dump all registers (for debugging)
    // LowLevelCameraAPI::dumpRegisters();  // Uncomment if needed
}
```

### sensor_t* API Reference

| Function | Purpose | Example |
|----------|---------|---------|
| `set_reg()` | Write register | `sensor->set_reg(sensor, 0xFF, 0xFF, 0x01)` |
| `get_reg()` | Read register | `sensor->get_reg(sensor, 0xFF, 0xFF)` |
| `set_exposure_ctrl()` | Auto exposure on/off | `sensor->set_exposure_ctrl(sensor, 1)` |
| `set_aec_value()` | Manual exposure | `sensor->set_aec_value(sensor, 300)` |
| `set_gain_ctrl()` | Auto gain on/off | `sensor->set_gain_ctrl(sensor, 1)` |
| `set_agc_gain()` | Manual gain | `sensor->set_agc_gain(sensor, 10)` |
| `set_binning()` | Pixel binning | `sensor->set_binning(sensor, true)` |
| `set_colorbar()` | Test pattern | `sensor->set_colorbar(sensor, 1)` |

---

## Comparison Table

| Feature | Level 1 (High) | Level 2 (Mid) | Level 3 (Low) |
|---------|----------------|---------------|---------------|
| **Safety** | ✅ Validated | ⚠️ Manual | ❌ None |
| **Persistence** | ✅ Auto-saved | ❌ No | ❌ No |
| **Type Safety** | ✅ Strict | ⚠️ Basic | ❌ None |
| **Error Handling** | ✅ Result<T> | ⚠️ errno | ❌ Can crash |
| **Portability** | ✅ Sensor-agnostic | ⚠️ Limited | ❌ Sensor-specific |
| **Learning Curve** | Easy | Medium | Hard |
| **Documentation** | ✅ Complete | ⚠️ Partial | ❌ Minimal |
| **Use Case** | Production | Testing | Research |

---

## Decision Tree

```
                    Start Here
                        │
                        ▼
        ┌───────────────────────────────┐
        │ Do you need basic camera      │
        │ controls? (brightness,        │ YES ──▶ Use Level 1
        │ resolution, quality, etc.)    │         (CameraSettings)
        └───────────────┬───────────────┘
                        │ NO
                        ▼
        ┌───────────────────────────────┐
        │ Do you need custom hardware   │
        │ pin config or XCLK frequency? │ YES ──▶ Use Level 2
        └───────────────┬───────────────┘         (camera_config_t)
                        │ NO
                        ▼
        ┌───────────────────────────────┐
        │ Do you need register-level    │
        │ access or undocumented        │ YES ──▶ Use Level 3
        │ features?                     │         (sensor_t*)
        └───────────────┬───────────────┘
                        │ NO
                        ▼
                You probably need Level 1!
```

---

## Real-World Examples

### Example 1: Time-Lapse Camera (Level 1)

```cpp
void timelapseSetup() {
    auto& app = ApplicationFacade::getInstance();

    // Optimize for quality (not speed)
    app.camera().optimizeForQuality();

    CameraSettings settings = app.camera().getCurrentSettings();
    settings.setResolution(Resolution::UXGA());  // Max resolution
    settings.setQuality(4);  // Best quality
    app.camera().updateSettings(settings);
}
```

### Example 2: Motion Detection Camera (Level 1 + Software)

```cpp
void motionDetectionSetup() {
    auto& app = ApplicationFacade::getInstance();

    // Optimize for speed
    app.camera().optimizeForSpeed();

    CameraSettings settings = app.camera().getCurrentSettings();
    settings.setResolution(Resolution::QVGA());  // Small for fast processing
    settings.setPixelFormat(PixelFormat::GRAYSCALE());  // No color needed
    app.camera().updateSettings(settings);

    // Then use software motion detection on frames
}
```

### Example 3: Security Camera with Night Vision (Level 3)

```cpp
void securityCameraSetup() {
    // Start with Level 1 for basic settings
    auto& app = ApplicationFacade::getInstance();
    app.camera().optimizeForLowLight();

    // Then use Level 3 for night mode
    sensor_t* sensor = LowLevelCameraAPI::getSensor();
    sensor->set_exposure_ctrl(sensor, 0);
    LowLevelCameraAPI::setExposureRaw(1200);
    LowLevelCameraAPI::setGainRaw(30);
    LowLevelCameraAPI::setNightMode(true);
}
```

### Example 4: Custom Board (Level 2)

```cpp
void customBoardSetup() {
    camera_config_t config = LowLevelCameraAPI::getHardwareConfig();

    // Custom pin assignment for non-AI-Thinker board
    config.pin_d0 = 17;
    config.pin_d1 = 35;
    config.pin_d2 = 34;
    config.pin_d3 = 5;
    config.pin_d4 = 39;
    config.pin_d5 = 18;
    config.pin_d6 = 36;
    config.pin_d7 = 19;
    config.pin_xclk = 27;
    config.pin_pclk = 21;
    config.pin_vsync = 22;
    config.pin_href = 26;
    config.pin_sccb_sda = 25;
    config.pin_sccb_scl = 23;
    config.pin_pwdn = -1;
    config.pin_reset = 15;

    LowLevelCameraAPI::reinitialize(config);
}
```

---

## Best Practices

### ✅ DO
1. **Start with Level 1** - Use CameraSettings for all standard use cases
2. **Test incrementally** - Apply one change at a time when using Level 3
3. **Document changes** - Keep notes on register values that work
4. **Profile performance** - Use `LowLevelCameraAPI::profileCapture()`
5. **Check sensor model** - Verify compatibility before register access

### ❌ DON'T
1. **Don't use Level 3 for production** - Unless absolutely necessary
2. **Don't modify registers blindly** - Can brick sensor
3. **Don't mix levels unnecessarily** - Pick the right abstraction
4. **Don't skip validation** - Always check return values
5. **Don't assume portability** - Register values are sensor-specific

---

## Troubleshooting

### Issue: Camera won't initialize after register changes

**Solution:**
```cpp
// Reset to defaults
esp_camera_deinit();
delay(500);

// Reinitialize with known-good config
camera_config_t config = LowLevelCameraAPI::getHardwareConfig();
esp_camera_init(&config);
```

### Issue: Settings don't persist

**Cause:** Using Level 2/3 bypasses persistence layer

**Solution:** Use Level 1 CameraSettings, or manually save:
```cpp
auto& storage = DependencyContainer::getInstance().getStorageRepository();
storage.saveCameraSettings(settings);
```

### Issue: Unknown sensor model

**Diagnosis:**
```cpp
Serial.println(LowLevelCameraAPI::getSensorModel());
Serial.println(LowLevelCameraAPI::getSensorStatus());
```

---

## Advanced Topics

### Combining Levels

You can mix levels for advanced use cases:

```cpp
// Level 1: Set base configuration
auto& app = ApplicationFacade::getInstance();
CameraSettings settings = app.camera().getCurrentSettings();
settings.setResolution(Resolution::VGA());
app.camera().updateSettings(settings);

// Level 3: Fine-tune with registers
sensor_t* sensor = LowLevelCameraAPI::getSensor();
sensor->set_exposure_ctrl(sensor, 0);
LowLevelCameraAPI::setExposureRaw(800);
```

### Register Documentation

For OV2640 register reference:
- [OV2640 Datasheet](https://www.uctronics.com/download/cam_module/OV2640DS.pdf)
- Bank selection: Register 0xFF (0x00 = DSP, 0x01 = Sensor)

For OV3660 register reference:
- [OV3660 Datasheet](https://github.com/espressif/esp32-camera/blob/master/sensors/ov3660.c)

---

## Summary

- **Level 1 (CameraSettings):** 99% of use cases ✅
- **Level 2 (camera_config_t):** Custom hardware only ⚠️
- **Level 3 (sensor_t*):** Research/debugging only ⚠️

**Recommendation:** Always start with Level 1. Only drop to lower levels when you've exhausted high-level options and understand the risks.

---

**For most users: Stick to Level 1!** 🎯
