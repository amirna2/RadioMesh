# Multi-Device Registry Design for RadioMesh Hub

## Overview

This document outlines the design for enabling RadioMesh hubs to track and manage multiple devices simultaneously. **The current implementation can only track ONE device at a time post inclusion** - when a second device joins the network, the first device becomes unreachable because the hub overwrites the first device's key information.

## Problem Analysis

### CRITICAL ISSUE: Hub Loses Previously Included Devices

**The core problem**: The hub can only track ONE device at a time due to single-device key storage.

**Current behavior (BROKEN for multiple devices)**:
1. ✅ Device A joins successfully via inclusion protocol
2. ✅ Device A can communicate with hub normally
3. ❌ Device B joins - **OVERWRITES Device A's key in hub memory**
4. ❌ Device A can no longer communicate with hub (key lost)
5. ✅ Device B can communicate with hub
6. ❌ Hub has "forgotten" Device A exists

**REQUIREMENT**: Inclusion must remain sequential (one device at a time) - this is correct and must not be changed.

### Current Limitations

1. **Single Device Key Storage**: Hub only stores one device's public key at a time
2. **Key Overwriting**: Each new inclusion overwrites the previous device's key
3. **Lost Device Tracking**: Previously included devices become unreachable
4. **No Frame Counter Tracking**: No per-device replay protection

### Evidence from Current Implementation

From `InclusionController.cpp:287-290`:
```cpp
// This OVERWRITES any previously stored device key
if (device.getEncryptionService()) {
    device.getEncryptionService()->setTempDevicePublicKey(devicePublicKey);
    logdbg_ln("Configured EncryptionService with device public key for INCLUDE_RESPONSE");
}
```

**Problem**: The hub overwrites the previous device's public key with each new inclusion, making previously included devices unreachable.

## Proposed Solution: Minimal In-Memory Device Registry

### Design Principles

1. **Minimal Invasive Changes**: Only modify hub-side logic, preserve all existing protocols
2. **No EEPROM Complexity**: Start with in-memory registry to avoid storage overhead
3. **Frame Counter Tracking**: Implement per-device replay protection
4. **Backward Compatibility**: Standard devices remain unchanged

### Core Components

#### 1. DeviceRecord Structure

```cpp
struct DeviceRecord {
    std::array<byte, RM_ID_LENGTH> deviceId;  // 4 bytes - unique device identifier
    std::vector<byte> publicKey;               // 32 bytes - device's public key for encryption
    uint32_t lastFrameCounter;                 // 4 bytes - highest frame counter seen (replay protection)
    uint32_t lastSeen;                         // 4 bytes - millis() timestamp of last message
};
```

**Rationale for Fields**:
- `deviceId`: Essential for device identification and registry lookup
- `publicKey`: Required for encrypting INCLUDE_RESPONSE messages to specific devices
- `lastFrameCounter`: Critical for preventing replay attacks (from packet `fcounter` field)
- `lastSeen`: Useful for device health monitoring and cleanup

#### 2. DeviceRegistry Integration

**Location**: Add to `InclusionController` class (hub devices only)

```cpp
class InclusionController {
private:
    // Existing members...

    // Device registry (hub only)
    std::map<std::string, DeviceRecord> deviceRegistry;
    static constexpr size_t MAX_DEVICES = 50;  // Configurable limit

public:
    // Registry management methods
    bool addDevice(const std::array<byte, RM_ID_LENGTH>& deviceId,
                   const std::vector<byte>& publicKey);
    bool getDevicePublicKey(const std::array<byte, RM_ID_LENGTH>& deviceId,
                           std::vector<byte>& publicKey);
    void updateDeviceLastSeen(const std::array<byte, RM_ID_LENGTH>& deviceId);
    bool validateFrameCounter(const std::array<byte, RM_ID_LENGTH>& deviceId,
                             uint32_t frameCounter);
    size_t getDeviceCount() const;
};
```

#### 3. EncryptionService Multi-Device Support

**Current Issue**: Single `tempDevicePublicKey` member
**Solution**: Registry-based key lookup

```cpp
class EncryptionService {
private:
    // Remove: std::vector<byte> tempDevicePublicKey;
    InclusionController* inclusionController;  // Registry access

public:
    // Modified method signature
    std::vector<byte> getDevicePublicKey(const std::array<byte, RM_ID_LENGTH>& deviceId);

    // Registry integration
    void setInclusionController(InclusionController* controller);
};
```

## Implementation Strategy

### Phase 1: Core Registry Implementation

#### File: `src/framework/device/inc/InclusionController.h`
- Add `DeviceRecord` struct definition
- Add `deviceRegistry` member (hub only)
- Add registry management method declarations

#### File: `src/framework/device/src/InclusionController.cpp`
- Implement registry management methods
- Modify `sendInclusionResponse()` to store device in registry
- Add frame counter validation in message handling
- Update device last seen timestamps

### Phase 2: EncryptionService Integration

#### File: `src/core/protocol/inc/crypto/EncryptionService.h`
- Remove `tempDevicePublicKey` member
- Add `inclusionController` pointer for registry access
- Modify key lookup methods

#### File: `src/core/protocol/src/crypto/EncryptionService.cpp`
- Replace single device key logic with registry lookup
- Implement device-specific key retrieval
- Add registry-based encryption/decryption

### Phase 3: Device Class Integration

#### File: `src/framework/device/src/Device.cpp`
- Initialize registry for hub devices
- Connect EncryptionService to InclusionController
- Add device registry accessor methods

## Detailed Implementation Changes

### 1. Registry Storage in INCLUDE_REQUEST Handling

**Current Flow** (`InclusionController.cpp:255-318`):
```cpp
int InclusionController::sendInclusionResponse(const RadioMeshPacket& packet)
{
    // Extract device public key from INCLUDE_REQUEST
    std::vector<byte> devicePublicKey = decryptedPayload;

    // Keep the device public key for later use
    if (device.getEncryptionService()) {
        device.getEncryptionService()->setTempDevicePublicKey(devicePublicKey);
    }
    // ... rest of method
}
```

**Modified Flow**:
```cpp
int InclusionController::sendInclusionResponse(const RadioMeshPacket& packet)
{
    // Extract device public key from INCLUDE_REQUEST
    std::vector<byte> devicePublicKey = decryptedPayload;

    // Store device in registry for persistent tracking
    if (!addDevice(packet.sourceDevId, devicePublicKey)) {
        logerr_ln("Failed to add device to registry");
        return RM_E_STORAGE_FULL;
    }

    // ... rest of method (no EncryptionService::setTempDevicePublicKey call)
}
```

### 2. Frame Counter Tracking

**Integration Point**: Message handling in `InclusionController::handleInclusionMessage()`

```cpp
// Add to all message handlers
if (deviceType == MeshDeviceType::HUB) {
    // Validate frame counter for replay protection
    if (!validateFrameCounter(packet.sourceDevId, packet.fcounter)) {
        logwarn_ln("Frame counter validation failed for device");
        return RM_E_REPLAY_ATTACK;
    }

    // Update last seen timestamp
    updateDeviceLastSeen(packet.sourceDevId);
}
```

### 3. EncryptionService Device Lookup

**Current Method**:
```cpp
std::vector<byte> EncryptionService::getEncryptionKey(EncryptionMethod method, uint8_t topic,
                                                      MeshDeviceType deviceType) const
{
    if (method == EncryptionMethod::DIRECT_ECC) {
        return tempDevicePublicKey;  // Single device only
    }
    // ...
}
```

**Modified Method**:
```cpp
std::vector<byte> EncryptionService::getEncryptionKey(EncryptionMethod method, uint8_t topic,
                                                      MeshDeviceType deviceType,
                                                      const std::array<byte, RM_ID_LENGTH>& targetDeviceId) const
{
    if (method == EncryptionMethod::DIRECT_ECC) {
        std::vector<byte> deviceKey;
        if (inclusionController && inclusionController->getDevicePublicKey(targetDeviceId, deviceKey)) {
            return deviceKey;
        }
        logerr_ln("Failed to get device public key for encryption");
        return std::vector<byte>();
    }
    // ...
}
```

## Testing Strategy

### Unit Tests
1. **Registry Operations**: Add, lookup, update, validate frame counters
2. **Concurrent Inclusions**: Multiple devices joining simultaneously
3. **Frame Counter Validation**: Replay attack prevention
4. **Registry Limits**: Maximum device capacity handling

### Integration Tests
1. **Sequential Inclusion**: Multiple devices joining one after another
2. **Post-Inclusion Communication**: Device messaging after successful inclusion
3. **Hub Restart**: Verify in-memory registry reset (expected behavior)
4. **Frame Counter Continuity**: Proper counter tracking across messages

### Performance Tests
1. **Registry Lookup Speed**: Device key retrieval performance
2. **Memory Usage**: Registry memory consumption with maximum devices
3. **Inclusion Throughput**: Time to include multiple devices

## Benefits

### Immediate Benefits
1. **Multiple Device Support**: Hub can handle concurrent device inclusions
2. **Replay Protection**: Per-device frame counter validation
3. **Device Health Monitoring**: Last seen timestamps for device status
4. **Clean Architecture**: Proper separation of device tracking concerns

### Future Enablement
1. **EEPROM Persistence**: Registry can be extended to persistent storage
2. **Cloud Backup**: Device list export/import for hub recovery
3. **Device Management**: Foundation for device removal, updates, etc.
4. **Security Enhancements**: Per-device security policies

## Operational Characteristics

### Memory Usage
- **Per Device**: 44 bytes (4 + 32 + 4 + 4)
- **50 Devices**: ~2.2 KB total registry size
- **Acceptable**: Well within ESP32 memory constraints

### Performance Impact
- **Registry Lookup**: O(log n) for std::map operations
- **Inclusion Speed**: No significant change to inclusion timing
- **Message Processing**: Minimal overhead for frame counter validation

### Limitations
1. **In-Memory Only**: Registry lost on hub restart (by design initially)
2. **No Persistence**: Device list not recoverable after power loss
3. **No Device Removal**: Registry only grows (cleanup needed for production)

## Migration Strategy

### Backward Compatibility
- **Standard Devices**: No changes required
- **Existing Hubs**: Registry starts empty, builds as devices join
- **Protocol Compatibility**: All existing message flows preserved

### Deployment Approach
1. **Phase 1**: Deploy hub-side changes only
2. **Phase 2**: Test with multiple existing devices
3. **Phase 3**: Monitor registry behavior in production
4. **Phase 4**: Add persistence layer if needed

## Future Considerations

### Persistence Layer (Future Phase)
```cpp
class DeviceRegistryStorage {
public:
    int persistRegistry(const std::map<std::string, DeviceRecord>& registry);
    int loadRegistry(std::map<std::string, DeviceRecord>& registry);
    int exportToJson(const std::map<std::string, DeviceRecord>& registry, std::string& json);
    int importFromJson(const std::string& json, std::map<std::string, DeviceRecord>& registry);
};
```

### Cloud Integration (Future Phase)
- Registry backup to MQTT/HTTP endpoint
- Device authorization validation
- Remote device management commands

### Security Enhancements (Future Phase)
- Device certificate validation
- Install code authentication (Z-Wave S2 style)
- Per-device security classes

---

**Document Status**: Design Complete
**Implementation Phase**: Ready to Begin
**Target Completion**: Single development session
**Compatibility Impact**: Hub-only changes, fully backward compatible
