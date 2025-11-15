# ESP32-CAM Webserver v5.0

Modern ESP32-CAM webserver with Clean Architecture, multi-client streaming, and JWT authentication.

## 🎯 Features

### Core Capabilities
- ✅ **Multi-Client Streaming** - Up to 5 concurrent MJPEG streams
- ✅ **JWT Authentication** - Secure token-based auth with RBAC
- ✅ **WebSocket Support** - Real-time streaming alternative
- ✅ **RESTful API** - Complete HTTP API for all operations
- ✅ **Persistent Storage** - Settings saved to SPIFFS
- ✅ **WiFi Auto-Reconnect** - Robust connection management
- ✅ **Clean Architecture** - Maintainable, testable, extensible

### Architecture
- **Domain Layer** - Pure business logic (100% framework-agnostic)
- **Application Layer** - Use cases and services
- **Infrastructure Layer** - ESP32 hardware drivers, repositories
- **Presentation Layer** - HTTP API, WebSocket handlers

### Security
- JWT token authentication (HMAC-SHA256)
- Role-Based Access Control (GUEST, USER, ADMIN)
- Password hashing with PBKDF2
- CORS support
- Rate limiting (planned)

### Performance
- Multi-client streaming (1 → 5 clients)
- Configurable frame rate (1-60 FPS)
- PSRAM support for high resolutions
- Asynchronous operations (FreeRTOS)
- Frame buffer pooling

## 📋 Requirements

### Hardware
- ESP32-CAM module (AI-Thinker or M5Stack)
- Camera module (OV2640, OV3660, OV5640)
- PSRAM (required for UXGA resolution)

### Software
- PlatformIO
- ESP32 Arduino Core 3.3.4+
- Libraries:
  - ESPAsyncWebServer
  - AsyncTCP
  - ArduinoJson 7.2.0
  - ESP32-Camera 2.0.14+

## 🚀 Quick Start

### 1. Install PlatformIO

```bash
# Using VSCode: Install PlatformIO IDE extension
# Or CLI: pip install platformio
```

### 2. Clone and Build

```bash
git clone <repository>
cd esp32-cam-webserver
pio run
```

### 3. Upload

```bash
pio run --target upload
```

### 4. Monitor

```bash
pio device monitor
```

### 5. Access Web Interface

The device will start in AP mode if no WiFi is configured:
- **SSID:** ESP32-CAM
- **Password:** esp32cam123
- **IP:** 192.168.4.1

Or connect to your WiFi network (configure in `data/config.json`).

## 📡 API Endpoints

### Authentication

```http
POST /api/auth/login
POST /api/auth/logout
POST /api/auth/refresh
GET  /api/auth/me
GET  /api/auth/permissions
POST /api/auth/register (admin)
GET  /api/auth/users (admin)
```

### Camera

```http
GET  /api/camera/status
GET  /api/camera/settings
POST /api/camera/settings
GET  /api/camera/capture
POST /api/camera/lamp?intensity=50
POST /api/camera/optimize?type=lowlight
POST /api/camera/reset
POST /api/camera/adjust?brightness=1
```

### Streaming

```http
GET  /api/stream/mjpeg (MJPEG stream)
POST /api/stream/start
POST /api/stream/stop?id=<streamId>
GET  /api/stream/stats?id=<streamId>
POST /api/stream/framerate?id=<streamId>&fps=25
POST /api/stream/pause?id=<streamId>
POST /api/stream/resume?id=<streamId>
GET  /api/stream/list
```

### WebSocket

```
ws://<ip>/ws
```

## 🔐 Authentication

All protected endpoints require JWT token in `Authorization` header:

```http
Authorization: Bearer <token>
```

### Default Credentials
- **Username:** admin
- **Password:** admin123

⚠️ **Change immediately in production!**

### Login Example

```bash
curl -X POST http://192.168.4.1/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123"}'
```

Response:
```json
{
  "success": true,
  "message": "Login successful",
  "token": {
    "token": "eyJ...",
    "username": "admin",
    "expiresAt": 1234567890,
    "expiresIn": 3600
  },
  "user": {
    "id": "...",
    "username": "admin",
    "role": "admin",
    "loginCount": 1
  }
}
```

## 🎥 Streaming Examples

### MJPEG Stream (HTTP)

```html
<img src="http://192.168.4.1/api/stream/mjpeg?token=<jwt>" />
```

### WebSocket Stream (JavaScript)

```javascript
const ws = new WebSocket('ws://192.168.4.1/ws');

ws.onmessage = (event) => {
  if (event.data instanceof Blob) {
    // Binary frame data
    const img = document.getElementById('stream');
    img.src = URL.createObjectURL(event.data);
  } else {
    // Text message (status, etc.)
    console.log(JSON.parse(event.data));
  }
};

// Control commands
ws.send('pause');
ws.send('resume');
ws.send('stop');
```

## 🛠️ Configuration

Edit `data/config.json`:

```json
{
  "wifi": {
    "ssid": "YourSSID",
    "password": "YourPassword"
  },
  "camera": {
    "frameSize": 10,
    "quality": 12,
    "brightness": 0,
    "contrast": 0
  },
  "server": {
    "httpPort": 80,
    "streamPort": 81,
    "maxClients": 5
  },
  "security": {
    "authEnabled": true,
    "jwtSecret": "<64-char-random-string>"
  }
}
```

Upload to SPIFFS:
```bash
pio run --target uploadfs
```

## 📚 Architecture

### Clean Architecture Layers

```
┌─────────────────────────────────────────┐
│      Presentation Layer (HTTP/WS)      │
├─────────────────────────────────────────┤
│   Application Layer (Use Cases/DTOs)   │
├─────────────────────────────────────────┤
│ Infrastructure Layer (Repos/Drivers)   │
├─────────────────────────────────────────┤
│     Domain Layer (Entities/Logic)      │
└─────────────────────────────────────────┘
```

### Dependency Flow

```
Presentation → Application → Infrastructure
                    ↓              ↓
                  Domain ←────────┘
```

### SOLID Principles

- **Single Responsibility** - Each class has one purpose
- **Open/Closed** - Extensible via interfaces
- **Liskov Substitution** - Implementations honor contracts
- **Interface Segregation** - Focused interfaces
- **Dependency Inversion** - Depend on abstractions

## 🔧 Development

### Project Structure

```
src/
├── core/               # Core abstractions (Result, Logger, Config, DI)
├── domain/            # Business logic (entities, value objects, interfaces)
├── infrastructure/    # Hardware drivers, repositories
├── application/       # Use cases, services, DTOs
└── presentation/      # HTTP handlers, WebSocket, middleware

test/                  # Unit tests
data/                  # SPIFFS files (config, web UI)
```

### Adding New Features

1. **Domain** - Define entities and repository interfaces
2. **Infrastructure** - Implement repository
3. **Application** - Create use case and service method
4. **Presentation** - Add HTTP handler

### Testing

```bash
pio test
```

## 📊 Performance

| Metric | Value |
|--------|-------|
| Max Concurrent Clients | 5 |
| Max Frame Rate | 30 FPS |
| Supported Resolutions | 96x96 to 1600x1200 (UXGA) |
| RAM Usage | ~200KB |
| PSRAM Usage | ~4MB (high res) |

## 🐛 Troubleshooting

### Camera Init Failed
- Check camera ribbon cable connection
- Verify camera model (OV2640 vs OV3660)
- Try lower resolution
- Check PSRAM availability

### WiFi Connection Failed
- Verify SSID/password in config
- Check signal strength
- Try AP mode fallback
- Monitor serial output for errors

### Streaming Lag
- Reduce resolution
- Lower frame rate
- Reduce client count
- Check WiFi bandwidth

## 📝 License

[Your License Here]

## 🙏 Acknowledgments

Built on:
- ESP32 Arduino Core
- ESPAsyncWebServer
- ArduinoJson
- ESP32-Camera library

---

**ESP32-CAM Webserver v5.0** - Professional-grade camera streaming with Clean Architecture
