# Phase 2 Architecture Review

**Date:** 2025-11-15
**Reviewer:** Claude (Anthropic AI)
**Scope:** Domain Layer Implementation (Phase 2)
**Criteria:** Correctness, Logic Accuracy, Clean Architecture Compliance

---

## Executive Summary

**Overall Grade: B+**

Phase 2 implements a solid domain layer with well-designed entities, value objects, and repository interfaces. However, there are **3 critical violations** of Clean Architecture principles that must be addressed before proceeding to Phase 3.

### Critical Issues Found: 3
### Medium Issues Found: 2
### Minor Issues Found: 4

---

## 1. Critical Issues (Must Fix)

### 🔴 CRITICAL #1: Hardware Dependencies in Domain Layer

**Severity:** HIGH
**Impact:** Violates Clean Architecture - Domain depends on Infrastructure
**Files Affected:**
- `src/domain/value_objects/Resolution.h:6`
- `src/domain/value_objects/PixelFormat.h:6`
- `src/domain/entities/CameraSettings.h:7`
- `src/domain/entities/Camera.h` (via Frame struct)

**Problem:**
```cpp
// Resolution.h - Line 6
#include "esp_camera.h"  // ❌ DOMAIN SHOULD NOT DEPEND ON HARDWARE!

// Uses hardware types
framesize_t _frameSize;  // ❌ Hardware-specific type
pixformat_t _format;     // ❌ Hardware-specific type
```

**Why This is Wrong:**
- Domain layer is the innermost layer - it should have NO external dependencies
- `esp_camera.h` is an ESP-IDF hardware library (infrastructure concern)
- If we want to port to a different platform, we'd have to modify domain code
- Violates Dependency Inversion Principle

**Solution:**
Create domain-level enums that are framework-agnostic, then map them in infrastructure layer.

```cpp
// ✅ CORRECT: Domain-level types (no hardware dependency)
// src/domain/value_objects/Resolution.h

enum class FrameSize {
    SIZE_96X96 = 0,
    SIZE_QQVGA,     // 160x120
    SIZE_QCIF,      // 176x144
    SIZE_HQVGA,     // 240x176
    SIZE_240X240,
    SIZE_QVGA,      // 320x240
    SIZE_CIF,       // 400x296
    SIZE_HVGA,      // 480x320
    SIZE_VGA,       // 640x480
    SIZE_SVGA,      // 800x600
    SIZE_XGA,       // 1024x768
    SIZE_HD,        // 1280x720
    SIZE_SXGA,      // 1280x1024
    SIZE_UXGA       // 1600x1200
};

class Resolution {
private:
    FrameSize _frameSize;  // ✅ Domain type, not hardware type
    uint16_t _width;
    uint16_t _height;
    // ...
};
```

Then in infrastructure layer, map between types:
```cpp
// ✅ Infrastructure layer maps domain to hardware
// src/infrastructure/camera/ESP32CameraDriver.cpp

framesize_t toHardwareFrameSize(FrameSize domainSize) {
    switch (domainSize) {
        case FrameSize::SIZE_96X96: return FRAMESIZE_96X96;
        case FrameSize::SIZE_QQVGA: return FRAMESIZE_QQVGA;
        // ... etc
    }
}
```

**Action Required:** Refactor value objects to use domain-level enums

---

### 🔴 CRITICAL #2: Library Dependencies in Domain Interfaces

**Severity:** HIGH
**Impact:** Couples domain to specific library implementation
**Files Affected:**
- `src/domain/repositories/IStorageRepository.h:9`

**Problem:**
```cpp
// IStorageRepository.h - Line 9
#include <ArduinoJson.h>  // ❌ DOMAIN SHOULD NOT KNOW ABOUT LIBRARIES!
```

**Why This is Wrong:**
- Domain defines the contract, infrastructure chooses the implementation
- If we want to switch from ArduinoJson to another JSON library, we'd have to modify domain code
- Interface should use domain types only (String, primitives, domain entities)

**Solution:**
Remove the include - it's not actually used in the interface!

```cpp
// ✅ CORRECT: No library dependencies
// src/domain/repositories/IStorageRepository.h

#ifndef I_STORAGE_REPOSITORY_H
#define I_STORAGE_REPOSITORY_H

#include "../entities/CameraSettings.h"
#include "../entities/User.h"
#include "../../core/Result.h"
// NO ArduinoJson.h!

class IStorageRepository {
    // Interface uses only domain types
    virtual Result<CameraSettings> loadCameraSettings() = 0;
    virtual Result<void> saveCameraSettings(const CameraSettings& settings) = 0;
    // ...
};
```

**Action Required:** Remove ArduinoJson.h include from interface

---

### 🔴 CRITICAL #3: Frame Struct Hardware Dependency

**Severity:** HIGH
**Impact:** Core data structure couples domain to hardware
**Files Affected:**
- `src/domain/entities/Camera.h:10-30`

**Problem:**
```cpp
// Camera.h - Frame struct
struct Frame {
    pixformat_t format;  // ❌ Hardware-specific type from esp_camera.h
};
```

**Why This is Wrong:**
- Frame is used throughout the domain layer
- Depends on `pixformat_t` from esp_camera.h
- Cascading violation affecting multiple interfaces

**Solution:**
Use domain PixelFormat value object (after fixing it):

```cpp
// ✅ CORRECT: Use domain types
struct Frame {
    uint8_t* buffer;
    size_t length;
    uint32_t width;
    uint32_t height;
    PixelFormat format;  // ✅ Domain value object
    uint32_t timestamp;
};
```

**Action Required:** Replace pixformat_t with PixelFormat value object

---

## 2. Medium Issues (Should Fix)

### 🟡 MEDIUM #1: Weak Password Hashing

**Severity:** MEDIUM
**Impact:** Security vulnerability
**Files Affected:**
- `src/domain/value_objects/Credentials.h:71-80`

**Problem:**
```cpp
String getPasswordHash() const {
    // Simple SHA-256-like hash for demonstration
    uint32_t hash = 0;
    for (size_t i = 0; i < _password.length(); i++) {
        hash = ((hash << 5) - hash) + _password.charAt(i);
    }
    // ❌ This is NOT cryptographically secure!
}
```

**Why This is Wrong:**
- Simple hash is vulnerable to rainbow table attacks
- No salt, no key derivation function (KDF)
- ESP32 has mbedtls library - should use it

**Solution:**
Use proper PBKDF2 or bcrypt (available in mbedtls):

```cpp
// ✅ CORRECT: Use mbedtls for proper hashing
#include "mbedtls/md.h"
#include "mbedtls/pkcs5.h"

String getPasswordHash() const {
    const char* salt = "esp32cam-salt-v1";  // Or generate random salt
    uint8_t hash[32];

    mbedtls_md_context_t sha_ctx;
    mbedtls_md_init(&sha_ctx);
    mbedtls_md_setup(&sha_ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);

    // PBKDF2 with 10000 iterations
    mbedtls_pkcs5_pbkdf2_hmac(&sha_ctx,
                              (const uint8_t*)_password.c_str(), _password.length(),
                              (const uint8_t*)salt, strlen(salt),
                              10000,  // iterations
                              32,  // key length
                              hash);

    mbedtls_md_free(&sha_ctx);

    // Convert to hex string
    char hexHash[65];
    for (int i = 0; i < 32; i++) {
        sprintf(&hexHash[i*2], "%02x", hash[i]);
    }
    return String(hexHash);
}
```

**Action Required:** Implement proper password hashing using mbedtls

---

### 🟡 MEDIUM #2: Missing DependencyContainer Implementation

**Severity:** MEDIUM
**Impact:** Cannot actually use the DI container
**Files Affected:**
- `src/core/DependencyContainer.h:58`

**Problem:**
```cpp
// DependencyContainer.h - Line 58
bool initialize();  // ❌ Declared but not defined!
```

**Why This is Wrong:**
- Header-only declaration without implementation
- Cannot compile if anyone tries to use it
- Should either be in .cpp file or inline in header

**Solution:**
Will need implementation in Phase 3, but for now should have stub:

```cpp
// ✅ Add implementation (even if stub for now)
bool initialize() {
    if (_initialized) {
        LOG_WARN("DI", "Already initialized");
        return true;
    }

    LOG_INFO("DI", "Initializing dependency container");

    // TODO: Phase 3 - Create concrete implementations
    // _cameraDriver = std::make_shared<ESP32CameraDriver>();
    // _cameraRepository = std::make_shared<CameraRepository>(_cameraDriver);
    // ... etc

    _initialized = true;
    return true;
}
```

**Action Required:** Add stub implementation or move to Phase 3

---

## 3. Minor Issues (Nice to Fix)

### 🟢 MINOR #1: Arduino-Specific Dependencies

**Severity:** LOW
**Impact:** Reduces portability to non-Arduino platforms
**Files Affected:** Multiple

**Problem:**
- Uses `String`, `IPAddress`, `millis()` throughout
- Couples to Arduino framework

**Analysis:**
This is acceptable for an Arduino-specific project, but reduces portability.

**Recommendation:**
- Keep as-is for Phase 2 (Arduino is the target platform)
- If future portability needed, create thin abstraction layer
- Not critical for current goals

---

### 🟢 MINOR #2: Weak ID Generation

**Severity:** LOW
**Impact:** Potential ID collisions
**Files Affected:**
- `src/domain/entities/User.h:196`
- `src/domain/entities/Stream.h:121`

**Problem:**
```cpp
// User.h
static String generateId() {
    char id[16];
    sprintf(id, "%08lx", millis());  // ❌ Could collide if created at same ms
    return String("user_") + String(id);
}
```

**Solution:**
Use better ID generation:

```cpp
// ✅ Better ID generation
static String generateId() {
    static uint32_t counter = 0;
    char id[24];
    sprintf(id, "%08lx_%08lx", millis(), counter++);
    return String("user_") + String(id);
}
```

**Action Required:** Improve ID generation (low priority)

---

### 🟢 MINOR #3: Potential Integer Overflow

**Severity:** LOW
**Impact:** Statistics could overflow after ~49 days uptime
**Files Affected:**
- `src/domain/entities/Camera.h:20`
- `src/domain/entities/Stream.h:25`

**Problem:**
```cpp
uint32_t _framesCaptured;  // ❌ Overflows after 4.2 billion frames
uint32_t _lastFrameTime;   // ❌ millis() overflows after 49 days
```

**Solution:**
- For production, use uint64_t for frame counters
- Handle millis() rollover in duration calculations
- Not critical for proof-of-concept

---

### 🟢 MINOR #4: Magic Numbers

**Severity:** LOW
**Impact:** Reduces code readability
**Files Affected:** Multiple

**Problem:**
```cpp
// Stream.h
static constexpr size_t MAX_CLIENTS = 5;  // ✅ Good - named constant

// CameraSettings.h
_brightness = constrain(value, -2, 2);  // ❌ Magic numbers

// Credentials.h
if (username.length() < 3 || username.length() > 32) {  // ❌ Magic numbers
```

**Solution:**
Define named constants:

```cpp
static constexpr uint8_t MIN_USERNAME_LENGTH = 3;
static constexpr uint8_t MAX_USERNAME_LENGTH = 32;
static constexpr int8_t MIN_BRIGHTNESS = -2;
static constexpr int8_t MAX_BRIGHTNESS = 2;
```

---

## 4. Logic Accuracy Review

### ✅ Camera State Machine
**Status:** Correct
**Analysis:**
- 7 states defined logically
- Valid state transitions
- Error handling present

### ✅ Stream Client Management
**Status:** Correct
**Analysis:**
- Fixed array for embedded system - good choice
- Client addition/removal logic correct
- Statistics tracking accurate
- FPS calculation correct (1000.0f / elapsed)

### ✅ User RBAC Logic
**Status:** Correct
**Analysis:**
- Permission hierarchy: GUEST < USER < ADMIN
- Permission checks are logical
- Account management methods correct

### ✅ Value Object Immutability
**Status:** Mostly Correct
**Issues:**
- Resolution and PixelFormat are immutable ✅
- Credentials has validation ✅
- CameraSettings is mutable (entity, not value object) ✅

### ⚠️ Credential Validation
**Status:** Mostly Correct
**Issues:**
- Username validation: 3-32 chars, alphanumeric + underscore + hyphen ✅
- Password validation: 8-64 chars ✅
- Missing: Check for common weak passwords
- Missing: Entropy check for password strength

---

## 5. Clean Architecture Compliance

### Dependency Flow Analysis

```
Current (INCORRECT):
┌─────────────────────────────────────────┐
│         Domain Layer                     │
│  ┌──────────────────────────────────┐   │
│  │  Value Objects (Resolution)      │   │
│  │         ↓                        │   │
│  │    #include "esp_camera.h" ❌   │   │
│  │         ↓                        │   │
│  │  [Infrastructure/Hardware]       │   │
│  └──────────────────────────────────┘   │
└─────────────────────────────────────────┘

Correct (SHOULD BE):
┌─────────────────────────────────────────┐
│         Domain Layer                     │
│  ┌──────────────────────────────────┐   │
│  │  Domain Enums (FrameSize)        │   │
│  │  No external dependencies ✅     │   │
│  └──────────────────────────────────┘   │
└─────────────────────────────────────────┘
         ↑
         │ implements/maps
         │
┌────────┴─────────────────────────────────┐
│    Infrastructure Layer                   │
│  ┌──────────────────────────────────┐    │
│  │  ESP32CameraDriver               │    │
│  │  Maps: FrameSize → framesize_t  │    │
│  │  #include "esp_camera.h" ✅     │    │
│  └──────────────────────────────────┘    │
└───────────────────────────────────────────┘
```

### Layer Purity Scorecard

| Layer | Purity | Issues |
|-------|--------|--------|
| Core | 90% | Uses Arduino String (acceptable) |
| Domain Entities | 70% | ❌ Frame uses pixformat_t |
| Domain Value Objects | 40% | ❌ Resolution/PixelFormat use esp_camera.h |
| Domain Repositories | 95% | ❌ IStorageRepository includes ArduinoJson.h |

**Target:** 100% purity (domain has zero framework/hardware dependencies)

---

## 6. Architecture Diagrams

### 6.1 Clean Architecture Layers (Current State)

```
┌─────────────────────────────────────────────────────────────────────┐
│                         PRESENTATION LAYER                           │
│                      (Not Yet Implemented)                           │
│                  HTTP Handlers, Middleware, Web UI                   │
└────────────────────────────────┬────────────────────────────────────┘
                                 │
┌────────────────────────────────▼────────────────────────────────────┐
│                        APPLICATION LAYER                             │
│                      (Not Yet Implemented)                           │
│              Use Cases, DTOs, Application Services                   │
└────────────────────────────────┬────────────────────────────────────┘
                                 │
┌────────────────────────────────▼────────────────────────────────────┐
│                          DOMAIN LAYER ✅                             │
│  ┌──────────────────┐  ┌──────────────┐  ┌────────────────────┐    │
│  │  Value Objects   │  │  Entities    │  │  Repository        │    │
│  │                  │  │              │  │  Interfaces        │    │
│  │  - Resolution ⚠️│  │  - Camera ⚠️│  │  - ICameraRepo ✅ │    │
│  │  - PixelFormat⚠️│  │  - Settings⚠️│  │  - IStreamRepo ✅ │    │
│  │  - Credentials✅ │  │  - Stream ✅ │  │  - IStorageRepo⚠️│    │
│  │                  │  │  - User ✅   │  │  - IAuthRepo ✅   │    │
│  └──────────────────┘  └──────────────┘  └────────────────────┘    │
│                                                                      │
│  ⚠️ = Has dependency violations                                     │
└────────────────────────────────┬────────────────────────────────────┘
                                 │
┌────────────────────────────────▼────────────────────────────────────┐
│                      INFRASTRUCTURE LAYER                            │
│                      (Not Yet Implemented)                           │
│        ESP32CameraDriver, WiFiManager, SPIFFS, etc.                  │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│                            CORE LAYER ✅                             │
│              Result<T>, Logger, Config, DI Container                 │
└─────────────────────────────────────────────────────────────────────┘
```

### 6.2 Domain Entity Relationships

```
┌─────────────────────────────────────────────────────────────────────┐
│                        DOMAIN MODEL                                  │
│                                                                      │
│  ┌──────────────┐                                                   │
│  │   Camera     │───────has──────┐                                  │
│  │              │                │                                  │
│  │  - state     │                ▼                                  │
│  │  - settings  │         ┌──────────────┐                          │
│  │  - stats     │         │CameraSettings│                          │
│  │  - lamp      │         │              │                          │
│  └──────┬───────┘         │ - resolution │                          │
│         │                 │ - quality    │                          │
│         │                 │ - brightness │                          │
│         │                 │ - ...        │                          │
│         │                 └──────────────┘                          │
│         │                                                            │
│         │uses                                                        │
│         │                                                            │
│         ▼                                                            │
│  ┌──────────────┐         ┌──────────────┐                          │
│  │    Frame     │─────────│  Resolution  │                          │
│  │              │contains │              │                          │
│  │  - buffer    │         │  - frameSize │                          │
│  │  - length    │         │  - width     │                          │
│  │  - width     │         │  - height    │                          │
│  │  - height    │         └──────────────┘                          │
│  │  - format◄───┼─────┐                                             │
│  └──────────────┘     │   ┌──────────────┐                          │
│                       └───│ PixelFormat  │                          │
│                           │              │                          │
│                           │  - format    │                          │
│  ┌──────────────┐         └──────────────┘                          │
│  │   Stream     │                                                    │
│  │              │                                                    │
│  │  - id        │         ┌──────────────┐                          │
│  │  - state     │─has─────│StreamClient[]│                          │
│  │  - clients[] │  0..5   │              │                          │
│  │  - stats     │         │  - id        │                          │
│  └──────────────┘         │  - ip        │                          │
│                           │  - frames    │                          │
│                           │  - fps       │                          │
│  ┌──────────────┐         └──────────────┘                          │
│  │    User      │                                                    │
│  │              │         ┌──────────────┐                          │
│  │  - id        │─has─────│  UserRole    │                          │
│  │  - username  │         │              │                          │
│  │  - hash      │         │  GUEST       │                          │
│  │  - role      │         │  USER        │                          │
│  │  - active    │         │  ADMIN       │                          │
│  └──────────────┘         └──────────────┘                          │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

### 6.3 Repository Pattern Flow

```
┌─────────────────────────────────────────────────────────────────────┐
│                     REPOSITORY PATTERN                               │
│                                                                      │
│  APPLICATION LAYER (Use Cases)                                       │
│  ┌──────────────────────────────────────────────────────────┐       │
│  │  CaptureImageUseCase                                     │       │
│  │                                                           │       │
│  │  execute() {                                             │       │
│  │    ┌─────────────────────────────────────┐               │       │
│  │    │ Result<Frame> frame =               │               │       │
│  │    │   cameraRepo.captureFrame();        │               │       │
│  │    │                                     │               │       │
│  │    │ if (frame.isOk()) {                 │               │       │
│  │    │   return ImageDTO::from(frame);     │               │       │
│  │    │ }                                   │               │       │
│  │    └─────────────────────────────────────┘               │       │
│  │  }                                                        │       │
│  └───────────────┬──────────────────────────────────────────┘       │
│                  │                                                   │
│                  │ depends on                                        │
│                  ▼                                                   │
│  DOMAIN LAYER (Interfaces)                                           │
│  ┌──────────────────────────────────────────────────────────┐       │
│  │  ICameraRepository (Interface)                           │       │
│  │  ┌────────────────────────────────────────────────┐      │       │
│  │  │ + initialize(settings): Result<void>          │      │       │
│  │  │ + captureFrame(): Result<Frame>               │      │       │
│  │  │ + releaseFrame(frame): void                   │      │       │
│  │  │ + updateSettings(settings): Result<void>      │      │       │
│  │  └────────────────────────────────────────────────┘      │       │
│  └───────────────▲──────────────────────────────────────────┘       │
│                  │                                                   │
│                  │ implements                                        │
│                  │                                                   │
│  INFRASTRUCTURE LAYER (Concrete Implementations)                     │
│  ┌───────────────┴──────────────────────────────────────────┐       │
│  │  CameraRepository                                        │       │
│  │                                                           │       │
│  │  - driver: ESP32CameraDriver                             │       │
│  │                                                           │       │
│  │  + captureFrame(): Result<Frame> {                       │       │
│  │      camera_fb_t* fb = driver.getFrameBuffer();          │       │
│  │      return Frame::from(fb);                             │       │
│  │  }                                                        │       │
│  └──────────────────────────────────────────────────────────┘       │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

### 6.4 Camera State Machine

```
┌─────────────────────────────────────────────────────────────────────┐
│                    CAMERA STATE MACHINE                              │
│                                                                      │
│                     ┌──────────────┐                                 │
│           ┌─────────│UNINITIALIZED │                                 │
│           │         └──────┬───────┘                                 │
│           │                │                                         │
│           │                │ initialize()                            │
│           │                ▼                                         │
│           │         ┌──────────────┐                                 │
│           │    ┌────│ INITIALIZING │                                 │
│           │    │    └──────┬───────┘                                 │
│           │    │           │                                         │
│           │    │ error     │ success                                 │
│           │    │           ▼                                         │
│           │    │    ┌──────────────┐      captureFrame()             │
│           │    │    │    READY     │◄──────────────┐                 │
│           │    │    └──┬───────┬───┘               │                 │
│           │    │       │       │                   │                 │
│           │    │       │       │ startStream()     │                 │
│  reset()  │    │       │       ▼                   │                 │
│           │    │       │  ┌──────────────┐         │                 │
│           │    │       │  │  STREAMING   │─────────┘                 │
│           │    │       │  └──────┬───────┘                           │
│           │    │       │         │                                   │
│           │    │       │         │ stopStream()                      │
│           │    │       │         │                                   │
│           │    │       │  ┌──────▼───────┐                           │
│           │    └───────┼─►│    ERROR     │                           │
│           │            │  └──────────────┘                           │
│           │            │                                             │
│           │            │ suspend()                                   │
│           │            │                                             │
│           │            ▼                                             │
│           │     ┌──────────────┐                                     │
│           └─────│  SUSPENDED   │                                     │
│                 └──────────────┘                                     │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

### 6.5 Dependency Injection Flow

```
┌─────────────────────────────────────────────────────────────────────┐
│                  DEPENDENCY INJECTION CONTAINER                      │
│                                                                      │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │  DependencyContainer::initialize()                           │   │
│  │                                                               │   │
│  │  1. Create Hardware Drivers                                  │   │
│  │     ┌────────────────────────────────────────┐               │   │
│  │     │ _cameraDriver =                        │               │   │
│  │     │   std::make_shared<ESP32CameraDriver>()│               │   │
│  │     └────────────────────────────────────────┘               │   │
│  │                                                               │   │
│  │  2. Create Repository Implementations                        │   │
│  │     ┌────────────────────────────────────────┐               │   │
│  │     │ _cameraRepository =                    │               │   │
│  │     │   std::make_shared<CameraRepository>(  │               │   │
│  │     │     _cameraDriver                      │               │   │
│  │     │   );                                   │               │   │
│  │     └────────────────────────────────────────┘               │   │
│  │                                                               │   │
│  │  3. Inject Dependencies into Use Cases                       │   │
│  │     ┌────────────────────────────────────────┐               │   │
│  │     │ CaptureImageUseCase(                   │               │   │
│  │     │   container.getCameraRepository()      │               │   │
│  │     │ );                                     │               │   │
│  │     └────────────────────────────────────────┘               │   │
│  │                                                               │   │
│  └──────────────────────────────────────────────────────────────┘   │
│                                                                      │
│  Benefits:                                                           │
│  ✅ Loose coupling - easy to swap implementations                   │
│  ✅ Testability - inject mocks for unit tests                       │
│  ✅ Single source of truth for dependency wiring                    │
│  ✅ Lifecycle management in one place                               │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 7. Test Coverage Readiness

### Testable Components (Good!)

✅ **Camera Entity**
```cpp
// Easy to test - pure logic, no dependencies
TEST(Camera, StateTransitions) {
    Camera camera("TEST");
    ASSERT_EQ(camera.getState(), CameraState::UNINITIALIZED);

    camera.setState(CameraState::READY);
    ASSERT_TRUE(camera.isReady());
}
```

✅ **Value Objects**
```cpp
// Easy to test - immutable, validation logic
TEST(Resolution, PSRAMRequirement) {
    Resolution svga = Resolution::SVGA();
    ASSERT_FALSE(svga.requiresPSRAM());

    Resolution uxga = Resolution::UXGA();
    ASSERT_TRUE(uxga.requiresPSRAM());
}
```

✅ **User RBAC**
```cpp
// Easy to test - permission logic
TEST(User, Permissions) {
    auto result = User::create(credentials, UserRole::USER);
    User user = result.getValue();

    ASSERT_TRUE(user.canViewStream());
    ASSERT_TRUE(user.canCaptureImage());
    ASSERT_FALSE(user.canModifySettings());  // Only admin
}
```

✅ **Repository Mocking**
```cpp
// Easy to mock - pure interfaces
class MockCameraRepository : public ICameraRepository {
public:
    MOCK_METHOD(Result<Frame>, captureFrame, (), (override));
    MOCK_METHOD(Result<void>, initialize, (const CameraSettings&), (override));
};

TEST(CaptureUseCase, Success) {
    MockCameraRepository mockRepo;
    EXPECT_CALL(mockRepo, captureFrame())
        .WillOnce(Return(Result<Frame>::ok(testFrame)));

    CaptureImageUseCase useCase(mockRepo);
    auto result = useCase.execute();
    ASSERT_TRUE(result.isOk());
}
```

### Current Test Coverage Potential: **80%**
(Once critical issues are fixed)

---

## 8. Recommendations & Action Items

### Immediate Actions (Before Phase 3)

1. **Fix Critical #1** - Remove esp_camera.h from domain
   - Create domain-level enums for FrameSize and PixelFormat
   - Estimated effort: 2 hours

2. **Fix Critical #2** - Remove ArduinoJson.h from IStorageRepository
   - Simply delete the include
   - Estimated effort: 5 minutes

3. **Fix Critical #3** - Fix Frame struct to use domain types
   - Use PixelFormat value object instead of pixformat_t
   - Estimated effort: 30 minutes

### High Priority (Can defer to Phase 3 cleanup)

4. **Medium #1** - Implement proper password hashing
   - Use mbedtls PBKDF2
   - Estimated effort: 1 hour

5. **Medium #2** - Implement DependencyContainer::initialize()
   - Will be done in Phase 3 anyway
   - Estimated effort: Part of Phase 3

### Low Priority (Future improvements)

6. **Minor issues** - Various small improvements
   - Better ID generation
   - Handle millis() rollover
   - Named constants for magic numbers
   - Estimated effort: 2-3 hours total

---

## 9. Conclusion

### Summary

Phase 2 provides a **solid foundation** for the domain layer with well-designed entities, value objects, and repository interfaces. The SOLID principles are generally well-applied, and the code demonstrates good understanding of Clean Architecture.

However, there are **3 critical violations** of Clean Architecture that create coupling between domain and infrastructure layers. These must be fixed before proceeding to Phase 3.

### Grading Breakdown

| Criterion | Score | Comments |
|-----------|-------|----------|
| **Entity Design** | A | Excellent state machines, statistics, RBAC |
| **Value Object Design** | B- | Good validation but hardware dependencies |
| **Repository Interfaces** | A- | Clean contracts, one library dependency |
| **Clean Architecture** | C+ | 3 critical violations of layer independence |
| **Logic Correctness** | A | State machines, calculations all correct |
| **Testability** | A | Easy to mock, good for unit testing |
| **Security** | C | Weak password hashing |
| **Code Quality** | B+ | Well-documented, readable, some magic numbers |

### **Overall Grade: B+**

With critical issues fixed, this would be an **A** grade.

### Final Recommendation

**✅ APPROVE Phase 2 with required fixes**

Fix the 3 critical issues before Phase 3:
1. Remove esp_camera.h from domain (create domain enums)
2. Remove ArduinoJson.h from IStorageRepository
3. Fix Frame struct to use domain types

Once fixed, the domain layer will be truly framework-agnostic and ready for infrastructure implementation.

---

**Next Steps:**
1. Apply critical fixes
2. Create domain-level enums (FrameSize, PixelFormat)
3. Commit fixes
4. Proceed to Phase 3 (Infrastructure Layer)
