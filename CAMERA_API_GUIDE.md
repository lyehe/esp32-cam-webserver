# Camera API Guide - Production Ready Design

## Overview

The ESP32-CAM webserver provides **two levels** of camera control designed for production use with proper Clean Architecture principles.

```
┌────────────────────────────────────────────────────┐
│  Level 1: CameraSettings (High-Level)             │
│  ✅ Recommended for 99% of use cases               │
│  ✅ Type-safe, validated, thread-safe, persistent  │
│  ✅ 40+ camera parameters                          │
├────────────────────────────────────────────────────┤
│  Level 2: ESP32CameraDriver (Infrastructure)      │
│  ⚠️  Internal use only                             │
│  ⚠️  Accessed via CameraRepository                 │
│  ⚠️  Direct hardware control                       │
└────────────────────────────────────────────────────┘
```

**Note:** Previous documentation mentioned a "Level 3" API that provided direct register access. This has been **removed** due to critical thread safety and architectural violations. The Level 1 API provides comprehensive control (40+ parameters) that covers all production use cases.

---

## Level 1: High-Level API (CameraSettings) ✅ RECOMMENDED

### When to Use
- ✅ **ALL production applications**
- ✅ Multi-client streaming
- ✅ Web API control
- ✅ Settings persistence
- ✅ Thread-safe concurrent access

### Features
- **Comprehensive:** 40+ camera parameters
- **Type-safe:** Value objects (Resolution, PixelFormat)
- **Validated:** Automatic range checking
- **Persistent:** SPIFFS auto-save
- **Thread-safe:** FreeRTOS mutex protection
- **Clean Architecture:** Full layer isolation
- **Error handling:** Result<T> monad pattern

### Complete Parameter List

| Category | Parameters | Range/Options |
|----------|-----------|---------------|
| **Resolution** | Frame size | QVGA (320x240) → UXGA (1600x1200) |
| **Format** | Pixel format | JPEG, RGB565, YUV422, Grayscale, RAW |
| **Compression** | JPEG quality | 0-63 (lower = better quality) |
| **Color Adjustments** | Brightness | -2 to +2 |
| | Contrast | -2 to +2 |
| | Saturation | -2 to +2 |
| **Image Enhancement** | Sharpness | -2 to +2 |
| | Denoise | On/Off |
| **Auto Controls** | Auto White Balance | On/Off |
| | Auto Exposure Control | On/Off |
| | Auto Exposure DSP | On/Off |
| | Auto Gain Control | On/Off |
| **Manual Controls** | Exposure Value | 0-1200 |
| | Auto Exposure Level | -2 to +2 |
| | Manual Exposure | On/Off |
| | Gain Ceiling | 2x, 4x, 8x, 16x, 32x, 64x, 128x |
| | AGC Gain | 0-30 |
| **White Balance** | WB Mode | Auto, Sunny, Cloudy, Office, Home |
| | AWB Gain | On/Off |
| **Corrections** | Gamma Correction | On/Off |
| | Lens Correction | On/Off |
| | White Pixel Correction | On/Off |
| | Black Pixel Correction | On/Off |
| **Effects** | Special Effect | None, Negative, Grayscale, Red/Green/Blue Tint, Sepia |
| **Orientation** | Horizontal Mirror | On/Off |
| | Vertical Flip | On/Off |
| **Frame Control** | Downsize | None, 2x, 4x, 8x |
| | Color Bar | On/Off (test pattern) |
| **Power** | DCW (Downsize) | On/Off |
| **Light** | LED Intensity | 0-255 |

**Total: 40+ parameters** - Comprehensive control without unsafe register access!

### Example Usage

```cpp
#include "application/ApplicationFacade.h"

void setup() {
    auto& app = ApplicationFacade::getInstance();
    app.initialize();

    // Get current settings
    CameraSettings settings = app.camera().getCurrentSettings();

    // === Basic Configuration ===
    settings.setResolution(Resolution::VGA());      // 640x480
    settings.setPixelFormat(PixelFormat::JPEG());
    settings.setQuality(12);                        // 0-63 (10-12 recommended)

    // === Color & Image Quality ===
    settings.setBrightness(1);                      // -2 to +2
    settings.setContrast(0);
    settings.setSaturation(0);
    settings.setSharpness(-1);                      // Slight blur for compression

    // === Auto Controls (Recommended) ===
    settings.setAutoWhiteBalance(true);             // Let camera adjust
    settings.setAutoExposureControl(true);          // Auto exposure
    settings.setAutoExposureDSP(true);              // DSP enhancement
    settings.setAutoGainControl(true);              // Auto gain

    // === Manual Controls (Advanced) ===
    // Only use if auto controls don't work for your use case
    settings.setAutoExposureControl(false);         // Disable auto first
    settings.setAutoExposureValue(300);             // Manual exposure
    settings.setAutoGainControl(false);
    settings.setAGCGain(10);                        // Manual gain

    // === Corrections ===
    settings.setGammaCorrection(true);              // Better color
    settings.setLensCorrectionEnabled(true);        // Fix distortion
    settings.setWhitePixelCorrectionEnabled(true);  // Fix dead pixels
    settings.setBlackPixelCorrectionEnabled(true);

    // === Orientation ===
    settings.setHorizontalMirror(false);
    settings.setVerticalFlip(false);

    // === Effects (Optional) ===
    settings.setSpecialEffect(0);                   // 0=None, 1=Negative, 2=Grayscale, etc.

    // Apply settings (validated + persisted + thread-safe)
    auto result = app.camera().updateSettings(settings);

    if (result.isOk()) {
        Serial.println("✅ Settings applied successfully");
    } else {
        Serial.println("❌ Error: " + result.getError());
    }
}
```

### Preset Optimizations

```cpp
auto& app = ApplicationFacade::getInstance();

// === Speed Preset (30+ FPS) ===
app.camera().optimizeForSpeed();
// Sets: VGA resolution, quality 20, double buffering, disabled effects

// === Quality Preset (Best Image) ===
app.camera().optimizeForQuality();
// Sets: UXGA resolution, quality 4, all corrections enabled

// === Low Light Preset (Night Mode) ===
app.camera().optimizeForLowLight();
// Sets: High exposure, high gain, denoise enabled
```

### Thread Safety Guarantee

```cpp
// Multiple tasks can safely call simultaneously:

// Task 1 (Core 0) - StreamTask
while (running) {
    app.camera().captureImage();  // Protected by mutex ✅
}

// Task 2 (Core 1) - HTTP Handler
void handleSettingsUpdate(AsyncWebServerRequest* request) {
    app.camera().updateSettings(newSettings);  // Protected by mutex ✅
}

// No race conditions, no corruption, no crashes!
```

### Error Handling

```cpp
// All operations return Result<T> for comprehensive error handling

auto result = app.camera().updateSettings(settings);

if (result.isError()) {
    Serial.println("Error code: " + String(result.getErrorCode()));
    Serial.println("Error message: " + result.getError());

    // Error types:
    // - "Camera not initialized"
    // - "Invalid resolution for current sensor"
    // - "PSRAM required for this resolution"
    // - "I2C communication failed"
    // - "Failed to acquire camera lock (timeout)"
}
```

---

## Level 2: Infrastructure Layer (ESP32CameraDriver)

### When to Use
- ⚠️ **Internal use only** - Do not call directly
- ⚠️ Accessed through CameraRepository
- ⚠️ For framework developers only

### Purpose
- Hardware abstraction for ESP32-Camera driver
- Implements `ICameraRepository` interface
- Provides mutex-protected hardware access
- Maps domain types to hardware types

### Architecture

```cpp
// ✅ CORRECT: Access through layers
ApplicationFacade → CameraService → CameraRepository → ESP32CameraDriver → Hardware

// ❌ WRONG: Direct hardware access
YourCode → ESP32CameraDriver  // Don't do this!
```

### Available Methods (Internal)

```cpp
class ESP32CameraDriver {
private:
    sensor_t* _sensor;               // Hardware pointer
    camera_config_t _config;         // Hardware config
    CameraPins _pins;                // Pin configuration
    SemaphoreHandle_t _mutex;        // Thread safety

public:
    // Initialization
    Result<void> initialize(const CameraSettings& settings);
    Result<void> deinitialize();

    // Frame capture
    Result<camera_fb_t*> captureFrame();
    void releaseFrame(camera_fb_t* fb);

    // Settings
    Result<void> updateSettings(const CameraSettings& settings);
    Result<void> applySettings(const CameraSettings& settings);

    // Hardware control
    Result<void> setLampIntensity(uint8_t intensity);
    Result<void> suspend();
    Result<void> resume();
    Result<void> reset();

    // Diagnostics
    String getSensorModel() const;
    sensor_t* getSensor() const;  // For internal use only
};
```

**Note:** These methods are called by `CameraRepository`. Do not call them directly unless you're modifying the framework.

---

## Comparison: Level 1 vs Direct Hardware

### Level 1 (CameraSettings) - RECOMMENDED ✅

**Advantages:**
- ✅ Thread-safe (FreeRTOS mutex)
- ✅ Validated (prevents invalid combinations)
- ✅ Persistent (auto-saves to SPIFFS)
- ✅ Type-safe (Resolution, PixelFormat enums)
- ✅ Error handling (Result<T> with messages)
- ✅ Testable (mockable interfaces)
- ✅ Maintainable (Clean Architecture)
- ✅ Comprehensive (40+ parameters)

**Disadvantages:**
- None for production use

### Direct Hardware Access (Removed) ❌

**Why It Was Removed:**
- ❌ No thread safety (can corrupt I2C bus)
- ❌ Can brick sensor (wrong register values)
- ❌ Bypasses validation (invalid settings)
- ❌ No persistence (settings lost on reboot)
- ❌ Not testable (requires real hardware)
- ❌ Violates Clean Architecture
- ❌ Changes not tracked by repository

**What You're Missing:**
- Nothing! CameraSettings already exposes all useful parameters

---

## Real-World Examples

### Example 1: Time-Lapse Camera

```cpp
void setupTimelapse() {
    auto& app = ApplicationFacade::getInstance();

    CameraSettings settings = app.camera().getCurrentSettings();

    // Maximum quality
    settings.setResolution(Resolution::UXGA());  // 1600x1200
    settings.setQuality(4);                      // Best quality
    settings.setSharpness(2);                    // Maximum sharpness
    settings.setAutoWhiteBalance(true);
    settings.setAutoExposureControl(true);
    settings.setGammaCorrection(true);
    settings.setLensCorrectionEnabled(true);

    app.camera().updateSettings(settings);

    // Capture every 5 seconds
    while (true) {
        auto result = app.camera().captureImage();
        if (result.isOk()) {
            saveToSD(result.getValue());
        }
        delay(5000);
    }
}
```

### Example 2: Security Camera (Low Light)

```cpp
void setupSecurityCamera() {
    auto& app = ApplicationFacade::getInstance();

    // Use preset for low light
    app.camera().optimizeForLowLight();

    CameraSettings settings = app.camera().getCurrentSettings();

    // Further customization
    settings.setResolution(Resolution::SVGA());  // 800x600 balance
    settings.setAutoExposureLevel(2);            // +2 for darker scenes
    settings.setDenoise(true);                   // Reduce noise
    settings.setGainCeiling(GainCeiling::X32);   // High gain for low light

    app.camera().updateSettings(settings);
}
```

### Example 3: High-Speed Streaming

```cpp
void setupHighSpeedStream() {
    auto& app = ApplicationFacade::getInstance();

    // Use speed preset
    app.camera().optimizeForSpeed();

    CameraSettings settings = app.camera().getCurrentSettings();

    // Maximum speed configuration
    settings.setResolution(Resolution::QVGA());  // 320x240 for speed
    settings.setQuality(25);                     // Lower quality = faster
    settings.setSharpness(-2);                   // Disable sharpening
    settings.setAutoWhiteBalance(false);         // Disable for speed
    settings.setGammaCorrection(false);          // Disable for speed
    settings.setLensCorrectionEnabled(false);    // Disable for speed

    app.camera().updateSettings(settings);

    // Enable streaming task (30+ FPS)
    StreamTask::start();
}
```

### Example 4: Motion Detection

```cpp
void setupMotionDetection() {
    auto& app = ApplicationFacade::getInstance();

    CameraSettings settings = app.camera().getCurrentSettings();

    // Optimize for motion detection processing
    settings.setResolution(Resolution::QVGA());       // Small for speed
    settings.setPixelFormat(PixelFormat::GRAYSCALE()); // No color needed
    settings.setQuality(30);                          // Compression not critical
    settings.setAutoExposureControl(true);            // Handle light changes

    app.camera().updateSettings(settings);

    // Capture and compare frames
    Frame previousFrame;
    while (true) {
        auto result = app.camera().captureImage();
        if (result.isOk()) {
            Frame currentFrame = result.getValue();
            if (detectMotion(previousFrame, currentFrame)) {
                triggerAlert();
            }
            previousFrame = currentFrame;
        }
        delay(100);  // 10 FPS for motion detection
    }
}
```

---

## Best Practices

### ✅ DO

1. **Use Level 1 (CameraSettings) for everything**
   ```cpp
   auto& app = ApplicationFacade::getInstance();
   app.camera().updateSettings(settings);  // ✅ Correct
   ```

2. **Use preset optimizations as starting points**
   ```cpp
   app.camera().optimizeForSpeed();  // Then customize
   ```

3. **Check return values**
   ```cpp
   auto result = app.camera().updateSettings(settings);
   if (result.isError()) {
       Serial.println("Error: " + result.getError());
   }
   ```

4. **Test settings incrementally**
   ```cpp
   settings.setBrightness(1);
   app.camera().updateSettings(settings);  // Test

   settings.setContrast(1);
   app.camera().updateSettings(settings);  // Test
   ```

5. **Use auto controls unless you have specific needs**
   ```cpp
   settings.setAutoWhiteBalance(true);     // ✅ Let camera decide
   settings.setAutoExposureControl(true);
   ```

### ❌ DON'T

1. **Don't access ESP32CameraDriver directly**
   ```cpp
   ESP32CameraDriver driver;  // ❌ Wrong layer
   driver.captureFrame();
   ```

2. **Don't try to access hardware directly**
   ```cpp
   sensor_t* sensor = esp_camera_sensor_get();  // ❌ Bypasses architecture
   sensor->set_reg(...);                        // ❌ No thread safety
   ```

3. **Don't ignore errors**
   ```cpp
   app.camera().updateSettings(settings);  // ❌ No error check
   ```

4. **Don't use extreme values without testing**
   ```cpp
   settings.setQuality(63);     // ❌ Very low quality
   settings.setBrightness(5);   // ❌ Out of range
   ```

---

## Troubleshooting

### Issue: Camera won't initialize

**Diagnosis:**
```cpp
auto result = app.camera().initialize(settings);
if (result.isError()) {
    Serial.println(result.getError());  // Check error message
}
```

**Common causes:**
- PSRAM not detected (required for resolutions > SVGA)
- Wrong pin configuration
- I2C communication failure
- Insufficient power supply

**Solution:**
```cpp
// Check PSRAM
if (!psramFound()) {
    Serial.println("ERROR: PSRAM required!");
}

// Check sensor
String model = app.camera().getSensorModel();
Serial.println("Sensor: " + model);  // Should show OV2640/OV3660
```

---

### Issue: Poor image quality

**Diagnosis:**
```cpp
CameraSettings current = app.camera().getCurrentSettings();
Serial.println("Quality: " + String(current.getQuality()));
Serial.println("Brightness: " + String(current.getBrightness()));
```

**Solution:**
```cpp
settings.setQuality(10);           // Lower = better (10-12 recommended)
settings.setGammaCorrection(true); // Better color
settings.setLensCorrectionEnabled(true);  // Fix distortion
```

---

### Issue: Low frame rate

**Diagnosis:**
```cpp
// Check current resolution
Resolution res = current.getResolution();
Serial.println("Width: " + String(res.getWidth()));
Serial.println("Height: " + String(res.getHeight()));
```

**Solution:**
```cpp
// Use speed preset
app.camera().optimizeForSpeed();

// Or manually:
settings.setResolution(Resolution::VGA());     // 640x480
settings.setQuality(20);                       // Faster compression
settings.setAutoWhiteBalance(false);           // Disable for speed
settings.setGammaCorrection(false);
settings.setLensCorrectionEnabled(false);

// Enable streaming task
StreamTask::start();  // Dedicated FreeRTOS task
```

---

## Summary

- **Level 1 (CameraSettings):** Use for ALL production code ✅
  - 40+ parameters
  - Thread-safe
  - Validated
  - Persistent
  - Comprehensive control

- **Level 2 (ESP32CameraDriver):** Internal framework use only ⚠️
  - Do not call directly
  - Accessed through CameraRepository

- **Level 3 (Direct registers):** **REMOVED** ❌
  - Architecturally unsafe
  - Thread safety violations
  - Not needed (Level 1 is comprehensive)

**For 99.9% of use cases: Use Level 1 (CameraSettings)** 🎯

---

**Questions?** Check `CRITICAL_ISSUES_FOUND.md` for detailed architectural analysis.
