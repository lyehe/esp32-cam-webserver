# ESP32 CAM Webserver Modernization Plan

**Target Version:** ESP32 Arduino Core 3.3.4 (based on ESP-IDF 5.5)
**Current Version:** ESP32 Arduino Core 2.0.2 (based on ESP-IDF 4.4)
**Architecture:** Clean Architecture + SOLID Principles
**Focus:** Performance, Stability, Maintainability, Security

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Target Platform Analysis](#target-platform-analysis)
3. [Clean Architecture Design](#clean-architecture-design)
4. [SOLID Principles Application](#solid-principles-application)
5. [Core System Improvements](#core-system-improvements)
6. [Performance Optimizations](#performance-optimizations)
7. [Stability Enhancements](#stability-enhancements)
8. [Security Hardening](#security-hardening)
9. [Migration Strategy](#migration-strategy)
10. [Implementation Roadmap](#implementation-roadmap)

---

## 1. Executive Summary

### Goals
- Upgrade from ESP32 Arduino Core 2.0.2 → 3.3.4 (3+ year jump)
- Redesign architecture using Clean Architecture principles
- Apply SOLID principles throughout codebase
- Eliminate global state and tight coupling
- Support multi-client streaming
- Improve performance and stability
- Add authentication and security features
- Maintain backward compatibility where possible

### Key Metrics
- **Lines of Code:** ~5,367 → Target: ~8,000 (better structure, more features)
- **Build Time:** Reduce compilation time by 30%
- **Memory Usage:** Optimize PSRAM usage, reduce heap fragmentation
- **Concurrent Clients:** 1 → 5 simultaneous streams
- **Security Score:** D → B+ (add auth, input validation, rate limiting)
- **Test Coverage:** 0% → 60% (unit + integration tests)

---

## 2. Target Platform Analysis

### 2.1 ESP32 Arduino Core 3.3.4 Features

**Base Framework:**
- ESP-IDF 5.5
- Support for ESP32, ESP32-S2, ESP32-S3, ESP32-C3, ESP32-C6, ESP32-H2
- Improved WiFi stability
- Better PSRAM management
- Enhanced security features

**Key Improvements from 2.0.2:**
- Timer API simplified (automatic divider calculation)
- LEDC API refactored (pin-based instead of channel-based)
- New I2S library
- BLE API changes (String instead of std::string)
- UART default pin changes
- Network library improvements
- Peripheral Manager for GPIO safety

### 2.2 Breaking Changes to Address

| Component | Change | Impact | Migration Action |
|-----------|--------|--------|------------------|
| **LEDC** | `ledcSetup`/`ledcAttachPin` removed | High | Replace with `ledcAttach(pin, freq, resolution)` |
| **Timer** | API simplified | Medium | Update timer initialization to use frequency only |
| **UART** | Pin defaults changed | Low | Explicitly set pins in config |
| **PSRAM** | `BOARD_HAS_PSRAM` deprecated | High | Use `psramFound()` and new memory allocation |
| **Build Flags** | Must include `-MMD -c` | Medium | Update platformio.ini and build config |

### 2.3 Known Issues to Avoid

**Arduino Core 3.3.0 Camera Bugs:**
- ❌ Stack canary errors with `esp_camera_fb_get()`
- ❌ Only JPEG pixel format works
- ✅ **Solution:** Target 3.3.4+ which has fixes

**ESPAsyncWebServer:**
- ❌ Compatibility issues with 3.1.x
- ✅ **Solution:** Use standard `esp_http_server` or fork maintained async server

**PSRAM Detection:**
- ❌ `esp_psram.h` header changes
- ✅ **Solution:** Use `psramFound()` and `ESP.getPsramSize()`

---

## 3. Clean Architecture Design

### 3.1 Layer Structure

```
┌─────────────────────────────────────────────────────────────┐
│                    Presentation Layer                        │
│  (HTTP Handlers, WebSocket, REST API, Web UI)               │
└────────────────────────┬────────────────────────────────────┘
                         │DTO (Data Transfer Objects)
┌────────────────────────▼────────────────────────────────────┐
│                   Application Layer                          │
│  (Use Cases, Business Logic, Orchestration)                 │
│  - StartStreamUseCase                                        │
│  - CaptureImageUseCase                                       │
│  - UpdateSettingsUseCase                                     │
│  - AuthenticateUserUseCase                                   │
└────────────────────────┬────────────────────────────────────┘
                         │ Interfaces (Dependency Inversion)
┌────────────────────────▼────────────────────────────────────┐
│                     Domain Layer                             │
│  (Entities, Domain Logic, Interfaces)                       │
│  - Camera (Entity)                                           │
│  - Stream (Entity)                                           │
│  - User (Entity)                                             │
│  - ICameraRepository (Interface)                             │
│  - IStreamRepository (Interface)                             │
│  - IStorageRepository (Interface)                            │
└────────────────────────┬────────────────────────────────────┘
                         │ Interface Implementation
┌────────────────────────▼────────────────────────────────────┐
│                  Infrastructure Layer                        │
│  (Hardware, Network, Storage, External Services)            │
│  - ESP32CameraDriver                                         │
│  - WiFiManager                                               │
│  - SPIFFSStorage                                             │
│  - HTTPServer                                                │
└─────────────────────────────────────────────────────────────┘
```

### 3.2 Dependency Flow

- **Outer layers depend on inner layers** (never the reverse)
- **Domain layer is independent** (no external dependencies)
- **Infrastructure implements domain interfaces**
- **Application orchestrates use cases**
- **Presentation adapts external requests to use cases**

### 3.3 Module Structure

```
src/
├── domain/
│   ├── entities/
│   │   ├── Camera.h
│   │   ├── Stream.h
│   │   ├── CameraSettings.h
│   │   └── User.h
│   ├── repositories/
│   │   ├── ICameraRepository.h
│   │   ├── IStreamRepository.h
│   │   ├── IStorageRepository.h
│   │   └── IAuthRepository.h
│   └── value_objects/
│       ├── Resolution.h
│       ├── PixelFormat.h
│       └── Credentials.h
│
├── application/
│   ├── use_cases/
│   │   ├── camera/
│   │   │   ├── InitializeCameraUseCase.h
│   │   │   ├── CaptureImageUseCase.h
│   │   │   └── UpdateCameraSettingsUseCase.h
│   │   ├── streaming/
│   │   │   ├── StartStreamUseCase.h
│   │   │   ├── StopStreamUseCase.h
│   │   │   └── ManageStreamClientsUseCase.h
│   │   ├── storage/
│   │   │   ├── SavePreferencesUseCase.h
│   │   │   └── LoadPreferencesUseCase.h
│   │   └── auth/
│   │       ├── AuthenticateUseCase.h
│   │       └── ValidateTokenUseCase.h
│   ├── dto/
│   │   ├── CameraStatusDTO.h
│   │   ├── StreamConfigDTO.h
│   │   └── AuthResponseDTO.h
│   └── services/
│       ├── FrameBufferPool.h
│       └── ConnectionPool.h
│
├── infrastructure/
│   ├── camera/
│   │   ├── ESP32CameraDriver.h
│   │   ├── ESP32CameraDriver.cpp
│   │   └── CameraRepository.cpp
│   ├── network/
│   │   ├── WiFiManager.h
│   │   ├── WiFiManager.cpp
│   │   ├── HTTPServer.h
│   │   ├── HTTPServer.cpp
│   │   └── StreamServer.h
│   ├── storage/
│   │   ├── SPIFFSStorage.h
│   │   ├── SPIFFSStorage.cpp
│   │   └── StorageRepository.cpp
│   ├── security/
│   │   ├── JWTAuthenticator.h
│   │   ├── JWTAuthenticator.cpp
│   │   └── AuthRepository.cpp
│   └── hardware/
│       ├── LEDController.h
│       ├── LampController.h
│       └── PinConfig.h
│
├── presentation/
│   ├── http/
│   │   ├── handlers/
│   │   │   ├── CameraHandler.h
│   │   │   ├── StreamHandler.h
│   │   │   ├── ConfigHandler.h
│   │   │   └── AuthHandler.h
│   │   ├── middleware/
│   │   │   ├── AuthMiddleware.h
│   │   │   ├── RateLimitMiddleware.h
│   │   │   └── LoggingMiddleware.h
│   │   └── responses/
│   │       ├── JSONResponse.h
│   │       └── HTMLResponse.h
│   └── web/
│       ├── assets/
│       │   ├── index.html
│       │   ├── style.css
│       │   └── app.js
│       └── templates/
│           └── error.html
│
├── core/
│   ├── DependencyContainer.h  // DI Container
│   ├── Config.h                // Global configuration
│   ├── Logger.h                // Logging facade
│   └── Result.h                // Error handling monad
│
└── main.cpp                    // Application entry point
```

---

## 4. SOLID Principles Application

### 4.1 Single Responsibility Principle (SRP)

**Problem (Current):**
```cpp
// esp32-cam-webserver.ino - does EVERYTHING
- Camera initialization
- WiFi management
- OTA updates
- Lamp control
- Time synchronization
- HTTP server startup
```

**Solution (New):**
```cpp
// Each class has ONE reason to change

class CameraInitializer {
    Result<Camera> initialize(CameraConfig config);
};

class WiFiConnectionManager {
    Result<WiFiConnection> connect(WiFiCredentials creds);
};

class OTAUpdateService {
    Result<void> beginUpdate();
};

class LampController {
    void setIntensity(uint8_t value);
};
```

### 4.2 Open/Closed Principle (OCP)

**Problem (Current):**
```cpp
// Adding new camera model requires editing multiple files
#if defined(CAMERA_MODEL_AI_THINKER)
    // AI Thinker pins
#elif defined(CAMERA_MODEL_WROVER_KIT)
    // WROVER pins
#elif defined(CAMERA_MODEL_ESP_EYE)
    // ESP Eye pins
// ... must edit this file to add new models
#endif
```

**Solution (New):**
```cpp
// Abstract camera configuration
class ICameraConfig {
public:
    virtual PinConfig getPins() const = 0;
    virtual CameraSpecs getSpecs() const = 0;
    virtual ~ICameraConfig() = default;
};

// Concrete implementations
class AIThinkerConfig : public ICameraConfig { };
class WRoverConfig : public ICameraConfig { };
class ESPEyeConfig : public ICameraConfig { };

// Add new models without modifying existing code
class NewCameraModelConfig : public ICameraConfig { };
```

### 4.3 Liskov Substitution Principle (LSP)

**Solution:**
```cpp
class IFrameProvider {
public:
    virtual Result<Frame> getFrame() = 0;
    virtual ~IFrameProvider() = default;
};

// Can substitute any frame provider
class CameraFrameProvider : public IFrameProvider { };
class FileFrameProvider : public IFrameProvider { };  // For testing
class NetworkFrameProvider : public IFrameProvider { };  // For proxy

// Use without knowing concrete type
void streamFrames(IFrameProvider& provider) {
    auto frame = provider.getFrame();
    // ... all providers behave consistently
}
```

### 4.4 Interface Segregation Principle (ISP)

**Problem (Current):**
```cpp
// Monolithic interface forces clients to depend on unused methods
class CameraController {
    void startCamera();
    void stopCamera();
    Frame* captureFrame();
    void startStream();
    void stopStream();
    void setLamp(int value);
    void flashLED(int time);
    void saveSettings();
    void loadSettings();
    // ... clients need only some of these
};
```

**Solution (New):**
```cpp
// Segregated interfaces - clients depend only on what they use
class ICameraCapture {
    virtual Frame* capture() = 0;
};

class ICameraStreaming {
    virtual void startStream() = 0;
    virtual void stopStream() = 0;
};

class ICameraSettings {
    virtual void updateSettings(CameraSettings settings) = 0;
    virtual CameraSettings getSettings() = 0;
};

class ILampControl {
    virtual void setIntensity(uint8_t value) = 0;
};

// Clients use only needed interfaces
class SnapshotHandler {
    ICameraCapture& camera;  // Only needs capture
};

class StreamHandler {
    ICameraStreaming& camera;  // Only needs streaming
};
```

### 4.5 Dependency Inversion Principle (DIP)

**Problem (Current):**
```cpp
// High-level code depends on low-level hardware details
void capture_handler(httpd_req_t *req) {
    camera_fb_t * fb = esp_camera_fb_get();  // Direct hardware dependency
    // ... tightly coupled to ESP32 camera driver
}
```

**Solution (New):**
```cpp
// High-level policy depends on abstraction
class CaptureImageUseCase {
    ICameraRepository& cameraRepo;  // Depends on abstraction

public:
    Result<ImageDTO> execute() {
        return cameraRepo.captureFrame();  // Not coupled to hardware
    }
};

// Infrastructure implements abstraction
class ESP32CameraRepository : public ICameraRepository {
    Result<ImageDTO> captureFrame() override {
        camera_fb_t* fb = esp_camera_fb_get();  // Hardware here
        return ImageDTO::from(fb);
    }
};
```

---

## 5. Core System Improvements

### 5.1 Replace Global State with Dependency Injection

**Current Problem:**
```cpp
// app_httpd.cpp - 66 extern variables!
extern char myName[];
extern IPAddress ip;
extern bool accesspoint;
extern int lampVal;
// ... 62 more!
```

**Solution - Dependency Container:**
```cpp
class DependencyContainer {
private:
    std::shared_ptr<ICameraRepository> cameraRepo;
    std::shared_ptr<IStorageRepository> storageRepo;
    std::shared_ptr<IAuthRepository> authRepo;
    std::shared_ptr<WiFiManager> wifiManager;

public:
    // Factory methods
    static DependencyContainer& getInstance() {
        static DependencyContainer instance;
        return instance;
    }

    void initialize() {
        // Wire up dependencies
        auto cameraDriver = std::make_shared<ESP32CameraDriver>();
        cameraRepo = std::make_shared<CameraRepository>(cameraDriver);

        auto storage = std::make_shared<SPIFFSStorage>();
        storageRepo = std::make_shared<StorageRepository>(storage);

        authRepo = std::make_shared<JWTAuthRepository>();
        wifiManager = std::make_shared<WiFiManager>();
    }

    ICameraRepository& getCameraRepository() { return *cameraRepo; }
    IStorageRepository& getStorageRepository() { return *storageRepo; }
    IAuthRepository& getAuthRepository() { return *authRepo; }
    WiFiManager& getWiFiManager() { return *wifiManager; }
};

// Usage in handlers
class CameraHandler {
    ICameraRepository& camera;

public:
    CameraHandler(ICameraRepository& cam) : camera(cam) {}

    void handleCapture(httpd_req_t* req) {
        auto result = camera.captureFrame();
        // ... no globals!
    }
};
```

### 5.2 Replace Custom JSON Parser with ArduinoJson

**Current:**
```cpp
// Custom, fragile JSON parser
String jsonExtract(String json, String name) {
    // 50 lines of manual parsing...
}
```

**Solution:**
```cpp
#include <ArduinoJson.h>

class PreferencesSerializer {
public:
    static String serialize(const CameraSettings& settings) {
        StaticJsonDocument<1024> doc;
        doc["lamp"] = settings.lampValue;
        doc["framesize"] = settings.frameSize;
        doc["quality"] = settings.quality;
        // ... type-safe, validated

        String output;
        serializeJson(doc, output);
        return output;
    }

    static Result<CameraSettings> deserialize(const String& json) {
        StaticJsonDocument<1024> doc;
        DeserializationError error = deserializeJson(doc, json);

        if (error) {
            return Result<CameraSettings>::error("Invalid JSON");
        }

        CameraSettings settings;
        settings.lampValue = doc["lamp"] | 0;
        settings.frameSize = doc["framesize"] | FRAMESIZE_SVGA;
        // ... with defaults and validation

        return Result<CameraSettings>::ok(settings);
    }
};
```

### 5.3 Eliminate Buffer Overflow Risks

**Current:**
```cpp
static char json_response[1024];
char * p = json_response;
*p++ = '{';
p+=sprintf(p, "\"lamp\":%d,", lampVal);  // No bounds checking!
// ... potential overflow
```

**Solution:**
```cpp
class JSONResponseBuilder {
    StaticJsonDocument<2048> doc;  // Auto size management

public:
    void addField(const char* key, int value) {
        doc[key] = value;  // Safe
    }

    String build() {
        String output;
        serializeJson(doc, output);
        return output;
    }
};
```

### 5.4 Multi-Client Stream Support

**Current Limitation:**
```cpp
int8_t streamCount;  // Only 0 or 1
if (streamCount > 0) return;  // Reject new clients
```

**Solution - Connection Pool:**
```cpp
class StreamConnectionPool {
private:
    struct StreamClient {
        httpd_req_t* request;
        uint32_t lastFrameTime;
        uint8_t quality;
        bool active;
    };

    static constexpr size_t MAX_CLIENTS = 5;
    StreamClient clients[MAX_CLIENTS];
    SemaphoreHandle_t mutex;

public:
    Result<size_t> addClient(httpd_req_t* req, uint8_t quality) {
        xSemaphoreTake(mutex, portMAX_DELAY);

        for (size_t i = 0; i < MAX_CLIENTS; i++) {
            if (!clients[i].active) {
                clients[i] = {req, 0, quality, true};
                xSemaphoreGive(mutex);
                return Result<size_t>::ok(i);
            }
        }

        xSemaphoreGive(mutex);
        return Result<size_t>::error("Pool full");
    }

    void broadcastFrame(const Frame& frame) {
        xSemaphoreTake(mutex, portMAX_DELAY);

        for (auto& client : clients) {
            if (client.active) {
                sendFrameToClient(client, frame);
            }
        }

        xSemaphoreGive(mutex);
    }

    void removeClient(size_t id) {
        xSemaphoreTake(mutex, portMAX_DELAY);
        if (id < MAX_CLIENTS) {
            clients[id].active = false;
        }
        xSemaphoreGive(mutex);
    }
};
```

### 5.5 Result Type for Error Handling

**Current:**
```cpp
esp_err_t res = ESP_OK;
if (something_failed) {
    httpd_resp_send_500(req);
    return ESP_FAIL;
}
// ... error handling scattered
```

**Solution - Result Monad:**
```cpp
template<typename T>
class Result {
private:
    bool success;
    T value;
    String errorMessage;

public:
    static Result<T> ok(T val) {
        return Result(true, val, "");
    }

    static Result<T> error(String msg) {
        return Result(false, T(), msg);
    }

    bool isOk() const { return success; }
    bool isError() const { return !success; }

    T getValue() const { return value; }
    String getError() const { return errorMessage; }

    // Monadic operations
    template<typename U>
    Result<U> map(std::function<U(T)> fn) {
        if (isOk()) {
            return Result<U>::ok(fn(value));
        }
        return Result<U>::error(errorMessage);
    }

    template<typename U>
    Result<U> flatMap(std::function<Result<U>(T)> fn) {
        if (isOk()) {
            return fn(value);
        }
        return Result<U>::error(errorMessage);
    }
};

// Usage
Result<Frame> captureFrame() {
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        return Result<Frame>::error("Camera capture failed");
    }
    return Result<Frame>::ok(Frame(fb));
}

// Chain operations
auto result = captureFrame()
    .map([](Frame f) { return compressFrame(f); })
    .flatMap([](CompressedFrame cf) { return saveFrame(cf); });

if (result.isError()) {
    Serial.println(result.getError());
}
```

---

## 6. Performance Optimizations

### 6.1 Frame Buffer Pool with PSRAM

**Current:**
```cpp
camera_fb_t * fb = esp_camera_fb_get();
// ... use frame
esp_camera_fb_return(fb);
// No pre-allocation, frequent allocation overhead
```

**Solution:**
```cpp
class FrameBufferPool {
private:
    struct FrameBuffer {
        uint8_t* buffer;
        size_t size;
        bool inUse;
    };

    static constexpr size_t POOL_SIZE = 4;
    FrameBuffer buffers[POOL_SIZE];
    SemaphoreHandle_t mutex;

public:
    void initialize() {
        // Pre-allocate in PSRAM
        for (size_t i = 0; i < POOL_SIZE; i++) {
            buffers[i].size = 100 * 1024;  // 100KB per buffer
            buffers[i].buffer = (uint8_t*)ps_malloc(buffers[i].size);
            buffers[i].inUse = false;
        }
    }

    FrameBuffer* acquire() {
        xSemaphoreTake(mutex, portMAX_DELAY);

        for (auto& buf : buffers) {
            if (!buf.inUse) {
                buf.inUse = true;
                xSemaphoreGive(mutex);
                return &buf;
            }
        }

        xSemaphoreGive(mutex);
        return nullptr;  // Pool exhausted
    }

    void release(FrameBuffer* buf) {
        xSemaphoreTake(mutex, portMAX_DELAY);
        buf->inUse = false;
        xSemaphoreGive(mutex);
    }
};
```

### 6.2 Optimize String Operations

**Current:**
```cpp
while (file.available()) {
    prefs += char(file.read());  // O(n²) - reallocates on each append
}
```

**Solution:**
```cpp
String readFile(File& file) {
    size_t size = file.size();
    String content;
    content.reserve(size + 1);  // Pre-allocate

    char buffer[128];
    while (file.available()) {
        size_t bytesRead = file.readBytes(buffer, sizeof(buffer));
        content.concat(buffer, bytesRead);  // Batch append
    }

    return content;
}
```

### 6.3 Asynchronous Operations with FreeRTOS

**Solution:**
```cpp
class AsyncCameraService {
private:
    QueueHandle_t frameQueue;
    TaskHandle_t captureTask;
    TaskHandle_t processTask;

    static void captureTaskFunc(void* param) {
        auto* service = static_cast<AsyncCameraService*>(param);

        while (true) {
            camera_fb_t* fb = esp_camera_fb_get();
            if (fb) {
                xQueueSend(service->frameQueue, &fb, portMAX_DELAY);
            }
            vTaskDelay(pdMS_TO_TICKS(33));  // ~30 FPS
        }
    }

    static void processTaskFunc(void* param) {
        auto* service = static_cast<AsyncCameraService*>(param);
        camera_fb_t* fb;

        while (true) {
            if (xQueueReceive(service->frameQueue, &fb, portMAX_DELAY)) {
                // Process frame on separate core
                service->processFrame(fb);
                esp_camera_fb_return(fb);
            }
        }
    }

public:
    void start() {
        frameQueue = xQueueCreate(4, sizeof(camera_fb_t*));

        // Capture on Core 0
        xTaskCreatePinnedToCore(captureTaskFunc, "capture", 4096,
                                this, 5, &captureTask, 0);

        // Process on Core 1
        xTaskCreatePinnedToCore(processTaskFunc, "process", 4096,
                                this, 4, &processTask, 1);
    }
};
```

### 6.4 Reduce HTTP Overhead

**Current:**
```cpp
// Each frame sent individually with full headers
sprintf((char *)part_buf, _STREAM_PART, _jpg_buf_len);
res = httpd_resp_send_chunk(req, (const char *)part_buf, strlen((char *)part_buf));
```

**Solution:**
```cpp
class OptimizedStreamSender {
private:
    char headerBuffer[128];
    size_t headerLength;

public:
    void initialize() {
        // Pre-format constant parts
        headerLength = sprintf(headerBuffer,
            "\r\n--BOUNDARY\r\n"
            "Content-Type: image/jpeg\r\n"
            "Content-Length: ");
    }

    esp_err_t sendFrame(httpd_req_t* req, const uint8_t* jpeg, size_t len) {
        // Reuse pre-formatted header
        char lengthStr[16];
        sprintf(lengthStr, "%u\r\n\r\n", len);

        // Vectored I/O - single syscall
        struct iovec iov[3] = {
            {headerBuffer, headerLength},
            {lengthStr, strlen(lengthStr)},
            {(void*)jpeg, len}
        };

        return httpd_resp_send_chunk_iov(req, iov, 3);
    }
};
```

---

## 7. Stability Enhancements

### 7.1 Watchdog and Recovery System

**Solution:**
```cpp
class SystemWatchdog {
private:
    TaskHandle_t watchdogTask;
    uint32_t lastCameraActivity;
    uint32_t lastWiFiActivity;

    static void watchdogTaskFunc(void* param) {
        auto* wd = static_cast<SystemWatchdog*>(param);

        while (true) {
            uint32_t now = millis();

            // Camera health check
            if (now - wd->lastCameraActivity > 30000) {
                Serial.println("Camera timeout - reinitializing");
                wd->recoverCamera();
            }

            // WiFi health check
            if (WiFi.status() != WL_CONNECTED) {
                Serial.println("WiFi disconnected - reconnecting");
                wd->recoverWiFi();
            }

            // Heap health check
            if (ESP.getFreeHeap() < 20000) {
                Serial.println("Low memory - triggering cleanup");
                wd->cleanupMemory();
            }

            vTaskDelay(pdMS_TO_TICKS(5000));  // Check every 5s
        }
    }

    void recoverCamera() {
        esp_camera_deinit();
        vTaskDelay(pdMS_TO_TICKS(1000));
        // Reinitialize camera
        auto config = getCameraConfig();
        esp_camera_init(&config);
        lastCameraActivity = millis();
    }

    void recoverWiFi() {
        WiFi.disconnect();
        vTaskDelay(pdMS_TO_TICKS(1000));
        WiFi.reconnect();
    }

    void cleanupMemory() {
        // Force garbage collection, clear caches
    }

public:
    void start() {
        xTaskCreate(watchdogTaskFunc, "watchdog", 2048, this, 3, &watchdogTask);
    }

    void updateCameraActivity() { lastCameraActivity = millis(); }
    void updateWiFiActivity() { lastWiFiActivity = millis(); }
};
```

### 7.2 Connection State Machine

**Solution:**
```cpp
class WiFiStateMachine {
public:
    enum class State {
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
        RECONNECTING,
        AP_MODE,
        ERROR
    };

private:
    State currentState = State::DISCONNECTED;
    uint8_t reconnectAttempts = 0;
    static constexpr uint8_t MAX_RECONNECT_ATTEMPTS = 5;

public:
    void update() {
        switch (currentState) {
            case State::DISCONNECTED:
                attemptConnection();
                break;

            case State::CONNECTING:
                if (WiFi.status() == WL_CONNECTED) {
                    currentState = State::CONNECTED;
                    reconnectAttempts = 0;
                    onConnected();
                } else if (connectionTimeout()) {
                    currentState = State::RECONNECTING;
                }
                break;

            case State::CONNECTED:
                if (WiFi.status() != WL_CONNECTED) {
                    currentState = State::RECONNECTING;
                }
                break;

            case State::RECONNECTING:
                if (reconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
                    WiFi.reconnect();
                    reconnectAttempts++;
                    currentState = State::CONNECTING;
                } else {
                    currentState = State::AP_MODE;
                    enableAccessPoint();
                }
                break;

            case State::AP_MODE:
                // Stay in AP mode until manual reset
                break;

            case State::ERROR:
                // Log error, attempt recovery
                break;
        }
    }
};
```

### 7.3 Heap Fragmentation Prevention

**Solution:**
```cpp
class MemoryManager {
public:
    // Use PSRAM for large allocations
    template<typename T>
    static T* allocatePSRAM(size_t count) {
        if (!psramFound()) {
            return nullptr;
        }
        return (T*)ps_malloc(count * sizeof(T));
    }

    // Pool allocator for fixed-size objects
    template<typename T, size_t POOL_SIZE>
    class ObjectPool {
    private:
        struct Block {
            T object;
            bool inUse;
        };

        Block pool[POOL_SIZE];

    public:
        T* allocate() {
            for (auto& block : pool) {
                if (!block.inUse) {
                    block.inUse = true;
                    return &block.object;
                }
            }
            return nullptr;
        }

        void deallocate(T* ptr) {
            for (auto& block : pool) {
                if (&block.object == ptr) {
                    block.inUse = false;
                    return;
                }
            }
        }
    };

    // Monitor fragmentation
    static void printHeapStats() {
        Serial.printf("Free heap: %u\n", ESP.getFreeHeap());
        Serial.printf("Min free heap: %u\n", ESP.getMinFreeHeap());
        Serial.printf("Max alloc heap: %u\n", ESP.getMaxAllocHeap());
        Serial.printf("PSRAM free: %u\n", ESP.getFreePsram());
    }
};
```

---

## 8. Security Hardening

### 8.1 JWT-based Authentication

**Solution:**
```cpp
#include "mbedtls/md.h"
#include "mbedtls/base64.h"

class JWTAuthenticator {
private:
    String secret;
    static constexpr uint32_t TOKEN_LIFETIME = 3600;  // 1 hour

    String base64UrlEncode(const uint8_t* data, size_t len) {
        // Base64 encode with URL-safe alphabet
        char* encoded = nullptr;
        size_t encodedLen = 0;
        mbedtls_base64_encode(nullptr, 0, &encodedLen, data, len);

        encoded = new char[encodedLen + 1];
        mbedtls_base64_encode((uint8_t*)encoded, encodedLen,
                              &encodedLen, data, len);
        encoded[encodedLen] = '\0';

        String result(encoded);
        delete[] encoded;

        // Replace +/= with -/_/nothing for URL safety
        result.replace('+', '-');
        result.replace('/', '_');
        result.replace("=", "");

        return result;
    }

    String hmacSHA256(const String& message) {
        uint8_t hmac[32];
        mbedtls_md_context_t ctx;
        mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

        mbedtls_md_init(&ctx);
        mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
        mbedtls_md_hmac_starts(&ctx, (uint8_t*)secret.c_str(), secret.length());
        mbedtls_md_hmac_update(&ctx, (uint8_t*)message.c_str(), message.length());
        mbedtls_md_hmac_finish(&ctx, hmac);
        mbedtls_md_free(&ctx);

        return base64UrlEncode(hmac, 32);
    }

public:
    JWTAuthenticator(const String& secretKey) : secret(secretKey) {}

    String generateToken(const String& username) {
        // Header
        String header = R"({"alg":"HS256","typ":"JWT"})";
        String encodedHeader = base64UrlEncode(
            (uint8_t*)header.c_str(), header.length());

        // Payload
        uint32_t now = time(nullptr);
        String payload = String("{\"sub\":\"") + username +
                        "\",\"iat\":" + String(now) +
                        ",\"exp\":" + String(now + TOKEN_LIFETIME) + "}";
        String encodedPayload = base64UrlEncode(
            (uint8_t*)payload.c_str(), payload.length());

        // Signature
        String message = encodedHeader + "." + encodedPayload;
        String signature = hmacSHA256(message);

        return message + "." + signature;
    }

    Result<String> validateToken(const String& token) {
        // Parse token
        int firstDot = token.indexOf('.');
        int secondDot = token.indexOf('.', firstDot + 1);

        if (firstDot == -1 || secondDot == -1) {
            return Result<String>::error("Invalid token format");
        }

        String header = token.substring(0, firstDot);
        String payload = token.substring(firstDot + 1, secondDot);
        String signature = token.substring(secondDot + 1);

        // Verify signature
        String message = header + "." + payload;
        String expectedSignature = hmacSHA256(message);

        if (signature != expectedSignature) {
            return Result<String>::error("Invalid signature");
        }

        // Decode and check expiration
        // ... (decode base64, parse JSON, check exp claim)

        return Result<String>::ok("username");  // Extract from token
    }
};
```

### 8.2 Input Validation Middleware

**Solution:**
```cpp
class InputValidator {
public:
    static Result<int> validateInteger(const char* str, int min, int max) {
        if (!str || *str == '\0') {
            return Result<int>::error("Empty value");
        }

        char* endPtr;
        long value = strtol(str, &endPtr, 10);

        if (*endPtr != '\0') {
            return Result<int>::error("Invalid integer");
        }

        if (value < min || value > max) {
            return Result<int>::error("Out of range");
        }

        return Result<int>::ok((int)value);
    }

    static Result<String> validateString(const char* str, size_t maxLen) {
        if (!str) {
            return Result<String>::error("Null string");
        }

        size_t len = strlen(str);
        if (len == 0 || len > maxLen) {
            return Result<String>::error("Invalid length");
        }

        // Check for injection attacks
        for (size_t i = 0; i < len; i++) {
            if (str[i] == '<' || str[i] == '>' || str[i] == '&') {
                return Result<String>::error("Invalid characters");
            }
        }

        return Result<String>::ok(String(str));
    }

    static bool isValidFrameSize(int value) {
        return value >= FRAMESIZE_96X96 && value <= FRAMESIZE_QXGA;
    }
};

// Usage in handler
esp_err_t cmd_handler(httpd_req_t *req) {
    char buf[100];
    httpd_req_get_url_query_str(req, buf, sizeof(buf));

    char variable[32];
    char value[32];

    if (httpd_query_key_value(buf, "var", variable, sizeof(variable)) != ESP_OK ||
        httpd_query_key_value(buf, "val", value, sizeof(value)) != ESP_OK) {
        return httpd_resp_send_400(req);
    }

    // Validate inputs
    auto varResult = InputValidator::validateString(variable, 31);
    if (varResult.isError()) {
        return httpd_resp_send_400(req);
    }

    if (strcmp(variable, "framesize") == 0) {
        auto valResult = InputValidator::validateInteger(value, 0, 13);
        if (valResult.isError() ||
            !InputValidator::isValidFrameSize(valResult.getValue())) {
            return httpd_resp_send_400(req);
        }

        // Safe to use
        sensor->set_framesize(sensor, (framesize_t)valResult.getValue());
    }

    return ESP_OK;
}
```

### 8.3 Rate Limiting

**Solution:**
```cpp
class RateLimiter {
private:
    struct ClientRecord {
        uint32_t ipAddress;
        uint32_t requestCount;
        uint32_t windowStart;
    };

    static constexpr size_t MAX_CLIENTS = 32;
    static constexpr uint32_t WINDOW_MS = 60000;  // 1 minute
    static constexpr uint32_t MAX_REQUESTS = 100;  // 100 req/min

    ClientRecord clients[MAX_CLIENTS];
    size_t clientCount = 0;
    SemaphoreHandle_t mutex;

    uint32_t getClientIP(httpd_req_t* req) {
        int sockfd = httpd_req_to_sockfd(req);
        struct sockaddr_in addr;
        socklen_t addr_len = sizeof(addr);
        getpeername(sockfd, (struct sockaddr*)&addr, &addr_len);
        return addr.sin_addr.s_addr;
    }

public:
    bool checkLimit(httpd_req_t* req) {
        uint32_t ip = getClientIP(req);
        uint32_t now = millis();

        xSemaphoreTake(mutex, portMAX_DELAY);

        // Find or create client record
        ClientRecord* record = nullptr;
        for (size_t i = 0; i < clientCount; i++) {
            if (clients[i].ipAddress == ip) {
                record = &clients[i];
                break;
            }
        }

        if (!record && clientCount < MAX_CLIENTS) {
            record = &clients[clientCount++];
            record->ipAddress = ip;
            record->requestCount = 0;
            record->windowStart = now;
        }

        if (!record) {
            xSemaphoreGive(mutex);
            return false;  // Too many clients
        }

        // Reset window if expired
        if (now - record->windowStart > WINDOW_MS) {
            record->requestCount = 0;
            record->windowStart = now;
        }

        // Check limit
        if (record->requestCount >= MAX_REQUESTS) {
            xSemaphoreGive(mutex);
            return false;  // Rate limit exceeded
        }

        record->requestCount++;
        xSemaphoreGive(mutex);
        return true;
    }
};

// Middleware wrapper
esp_err_t rateLimitedHandler(httpd_req_t *req,
                              esp_err_t (*handler)(httpd_req_t*)) {
    if (!rateLimiter.checkLimit(req)) {
        httpd_resp_set_status(req, "429 Too Many Requests");
        return httpd_resp_send(req, "Rate limit exceeded", -1);
    }

    return handler(req);
}
```

---

## 9. Migration Strategy

### 9.1 Phased Approach

**Phase 1: Foundation (Weeks 1-2)**
- ✅ Set up new project structure
- ✅ Configure ESP32 Arduino Core 3.3.4
- ✅ Create build system (PlatformIO + Arduino IDE)
- ✅ Implement core abstractions (Result, Logger, Config)
- ✅ Set up dependency injection container

**Phase 2: Domain Layer (Weeks 3-4)**
- ✅ Define entities (Camera, Stream, User, Settings)
- ✅ Define repository interfaces
- ✅ Create value objects (Resolution, PixelFormat, etc.)
- ✅ Write domain logic unit tests

**Phase 3: Infrastructure Layer (Weeks 5-7)**
- ✅ Implement ESP32CameraDriver (adapted for Core 3.3.4)
- ✅ Implement WiFiManager with state machine
- ✅ Implement SPIFFSStorage with ArduinoJson
- ✅ Implement HTTP/Stream servers
- ✅ Create hardware controllers (LED, Lamp)

**Phase 4: Application Layer (Weeks 8-9)**
- ✅ Implement use cases (capture, stream, settings, auth)
- ✅ Create DTOs and mappers
- ✅ Implement services (FrameBufferPool, ConnectionPool)
- ✅ Write integration tests

**Phase 5: Presentation Layer (Weeks 10-11)**
- ✅ Implement HTTP handlers
- ✅ Implement middleware (auth, rate limiting, logging)
- ✅ Create modern web UI (Vue.js or React)
- ✅ Implement WebSocket support for real-time updates

**Phase 6: Testing & Optimization (Weeks 12-13)**
- ✅ Performance profiling and optimization
- ✅ Memory leak detection and fixing
- ✅ Load testing (multi-client streaming)
- ✅ Security audit

**Phase 7: Documentation & Release (Week 14)**
- ✅ Write migration guide
- ✅ Update README and API docs
- ✅ Create example configurations
- ✅ Tag release and announce

### 9.2 Backward Compatibility

**Configuration Migration Tool:**
```cpp
class ConfigMigrator {
public:
    static Result<void> migrateFromV4(const String& oldConfigPath) {
        // Read old myconfig.h
        File oldConfig = SPIFFS.open(oldConfigPath, "r");
        if (!oldConfig) {
            return Result<void>::error("Cannot open old config");
        }

        // Parse old format (C preprocessor defines)
        String content = readFile(oldConfig);

        // Extract values
        String ssid = extractDefine(content, "WIFI_SSID");
        String password = extractDefine(content, "WIFI_PASS");
        // ... more fields

        // Create new config.json
        StaticJsonDocument<2048> newConfig;
        newConfig["wifi"]["ssid"] = ssid;
        newConfig["wifi"]["password"] = password;
        // ... migrate all settings

        File newConfigFile = SPIFFS.open("/config.json", "w");
        serializeJsonPretty(newConfig, newConfigFile);
        newConfigFile.close();

        return Result<void>::ok();
    }
};
```

### 9.3 Feature Parity Checklist

| Feature | v4.0 (Old) | v5.0 (New) | Status |
|---------|-----------|-----------|--------|
| Camera init (OV2640/OV3660) | ✅ | ✅ | Planned |
| Multiple board support | ✅ | ✅ | Planned |
| WiFi station mode | ✅ | ✅ | Planned |
| WiFi AP mode | ✅ | ✅ | Planned |
| Captive portal | ✅ | ✅ | Planned |
| mDNS | ✅ | ✅ | Planned |
| OTA updates | ✅ | ✅ | Planned |
| MJPEG streaming | ✅ | ✅ | Planned |
| Single stream | ✅ | ❌ | Removed |
| Multi-client streaming | ❌ | ✅ | **New** |
| Settings persistence | ✅ | ✅ | Improved |
| Lamp control | ✅ | ✅ | Planned |
| LED status | ✅ | ✅ | Planned |
| Web UI (full) | ✅ | ✅ | Modernized |
| Web UI (simple) | ✅ | ✅ | Modernized |
| REST API | ✅ | ✅ | Improved |
| Authentication | ❌ | ✅ | **New** |
| Rate limiting | ❌ | ✅ | **New** |
| Input validation | ❌ | ✅ | **New** |
| WebSocket support | ❌ | ✅ | **New** |
| H.264 streaming | ❌ | 🔄 | Future |
| Unit tests | ❌ | ✅ | **New** |
| Integration tests | ❌ | ✅ | **New** |

---

## 10. Implementation Roadmap

### 10.1 Repository Structure

```
esp32-cam-webserver-v5/
├── .github/
│   └── workflows/
│       ├── build.yml
│       └── test.yml
├── src/
│   ├── domain/
│   ├── application/
│   ├── infrastructure/
│   ├── presentation/
│   ├── core/
│   └── main.cpp
├── test/
│   ├── unit/
│   ├── integration/
│   └── mocks/
├── lib/
│   └── ArduinoJson/
├── data/              # SPIFFS files
│   ├── config.json
│   ├── web/
│   │   ├── index.html
│   │   ├── app.js
│   │   └── style.css
│   └── certs/
├── platformio.ini
├── CMakeLists.txt     # For ESP-IDF native
├── README.md
├── MIGRATION.md
├── API.md
├── ARCHITECTURE.md
├── LICENSE
└── examples/
    ├── basic/
    ├── multi_client/
    └── authenticated/
```

### 10.2 Build Configuration

**platformio.ini:**
```ini
[platformio]
src_dir = src
test_dir = test

[env:esp32dev]
platform = espressif32@^6.9.0  ; Arduino Core 3.3.4
framework = arduino
board = esp32dev
board_build.partitions = min_spiffs.csv
board_build.filesystem = spiffs

build_flags =
    -DBOARD_HAS_PSRAM
    -DCORE_DEBUG_LEVEL=3
    -DARDUINOJSON_USE_LONG_LONG=1
    -MMD -c

lib_deps =
    bblanchon/ArduinoJson@^7.0.0
    esp32/ESP32-Camera@^2.0.9

monitor_speed = 115200
monitor_filters = esp32_exception_decoder

; Unit testing
test_framework = unity
test_build_src = yes

[env:esp32s3]
extends = env:esp32dev
board = esp32-s3-devkitc-1
board_build.mcu = esp32s3
board_build.f_cpu = 240000000L

[env:ota]
extends = env:esp32dev
upload_protocol = espota
upload_port = esp32-cam.local
upload_flags =
    --auth=YourOTAPassword
```

### 10.3 Continuous Integration

**.github/workflows/build.yml:**
```yaml
name: Build and Test

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest

    strategy:
      matrix:
        board: [esp32dev, esp32s3]

    steps:
    - uses: actions/checkout@v3

    - name: Cache PlatformIO
      uses: actions/cache@v3
      with:
        path: ~/.platformio
        key: ${{ runner.os }}-pio

    - name: Set up Python
      uses: actions/setup-python@v4
      with:
        python-version: '3.11'

    - name: Install PlatformIO
      run: pip install platformio

    - name: Build ${{ matrix.board }}
      run: pio run -e ${{ matrix.board }}

    - name: Run tests
      run: pio test -e ${{ matrix.board }}

    - name: Upload artifact
      uses: actions/upload-artifact@v3
      with:
        name: firmware-${{ matrix.board }}
        path: .pio/build/${{ matrix.board }}/firmware.bin
```

### 10.4 Development Guidelines

**Code Style:**
- Use clang-format with Google style
- Max line length: 100 characters
- Use modern C++17 features where supported
- Prefer const and constexpr
- Use RAII for resource management

**Naming Conventions:**
- Classes: PascalCase (e.g., `CameraRepository`)
- Interfaces: IPascalCase (e.g., `ICameraRepository`)
- Methods: camelCase (e.g., `captureFrame()`)
- Constants: UPPER_SNAKE_CASE (e.g., `MAX_CLIENTS`)
- Private members: camelCase with underscore (e.g., `frameBuffer_`)

**Documentation:**
- All public APIs must have Doxygen comments
- Complex algorithms must have explanatory comments
- README for each major module

**Testing:**
- Minimum 60% code coverage
- All use cases must have tests
- Critical paths must have integration tests

---

## 11. Success Metrics

### 11.1 Performance Targets

| Metric | Current (v4.0) | Target (v5.0) | Improvement |
|--------|---------------|---------------|-------------|
| Concurrent streams | 1 | 5 | 500% |
| Frame rate (SVGA) | ~15 FPS | ~25 FPS | 67% |
| Frame rate (UXGA) | ~8 FPS | ~12 FPS | 50% |
| Heap fragmentation | High | Low | Monitored |
| OTA update time | ~60s | ~45s | 25% |
| WiFi reconnect time | ~10s | ~5s | 50% |
| First frame latency | ~2s | ~500ms | 75% |
| Memory leaks | Present | None | 100% |

### 11.2 Quality Targets

| Metric | Current | Target |
|--------|---------|--------|
| Build warnings | 15+ | 0 |
| Static analysis issues | Unknown | 0 critical, <10 minor |
| Code coverage | 0% | 60%+ |
| Documentation coverage | 40% | 90%+ |
| Security vulnerabilities | 5 high | 0 high, <3 medium |

### 11.3 Stability Targets

| Metric | Current | Target |
|--------|---------|--------|
| Uptime without reboot | ~24h | >7 days |
| WiFi disconnections/day | 3-5 | <1 |
| Camera crashes/day | 1-2 | 0 |
| Memory leaks | Yes | No |
| Watchdog resets | Occasional | Rare |

---

## 12. Risk Management

### 12.1 Technical Risks

| Risk | Probability | Impact | Mitigation |
|------|------------|--------|------------|
| ESP32 Core 3.3.4 camera bugs | Medium | High | Test thoroughly, have rollback to 3.0.7 |
| PSRAM compatibility issues | Low | High | Extensive testing on multiple boards |
| Performance degradation | Low | Medium | Continuous profiling and benchmarking |
| Breaking existing integrations | High | Medium | Provide migration tools and documentation |
| OTA failures | Low | High | Maintain failsafe boot partition |

### 12.2 Project Risks

| Risk | Probability | Impact | Mitigation |
|------|------------|--------|------------|
| Scope creep | Medium | Medium | Strict phase boundaries, feature freeze |
| Timeline overrun | Medium | Low | Buffer time in schedule, parallel work |
| Breaking changes in ESP32 core | Low | High | Pin specific version, monitor releases |
| Community resistance to changes | Medium | Medium | Clear communication, migration guides |

---

## 13. Next Steps

1. **Review and approve this plan** with stakeholders
2. **Set up new repository** with Clean Architecture structure
3. **Configure development environment** with ESP32 Core 3.3.4
4. **Begin Phase 1** implementation (Foundation)
5. **Establish CI/CD pipeline**
6. **Start documentation** concurrently with development

---

## Conclusion

This modernization plan transforms the ESP32 CAM webserver from a legacy, tightly-coupled application to a modern, maintainable, and scalable system. By applying Clean Architecture and SOLID principles, we create a codebase that is:

- **Testable**: Clear boundaries enable comprehensive testing
- **Maintainable**: Separation of concerns makes changes isolated
- **Scalable**: Support for multiple clients and new features
- **Secure**: Authentication, validation, and rate limiting
- **Stable**: Watchdog, recovery, and proper state management
- **Performant**: Optimized for ESP32 hardware capabilities

The phased approach ensures we can deliver incremental value while maintaining quality and stability throughout the modernization process.
