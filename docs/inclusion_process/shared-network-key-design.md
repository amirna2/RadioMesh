# RadioMesh Shared Network Key Protocol Design

## Overview

This document outlines the revised RadioMesh inclusion protocol using a **shared network key** approach instead of per-device session keys. This design aligns with industry standards and enables better operational characteristics including cloud recovery and mesh routing efficiency.

## Design Change Rationale

### Problems with Per-Device Session Keys
1. **Hub State Management**: Hub must store public keys and session keys for every device
2. **Cloud Recovery Complexity**: Hub replacement requires re-inclusion of all devices
3. **Mesh Routing Limitations**: Hub must mediate all inter-device communication
4. **Storage Scaling**: Memory requirements grow linearly with device count
5. **Operational Complexity**: Field service and device replacement become complex

### Benefits of Shared Network Key
1. **Stateless Hub Recovery**: Hub needs only network key + device list (cloud recoverable)
2. **Efficient Mesh Routing**: Any device can decrypt/forward packets for mesh routing
3. **Simplified Operations**: Device replacement only requires network key provisioning
4. **Industry Alignment**: Follows proven patterns from Zigbee/Z-Wave/Thread
5. **Broadcast Support**: Network-wide broadcasts work seamlessly

## Protocol Flow

### Message Sequence (Unchanged)
The 5-message inclusion flow remains identical:

```
Actor          HUB                                 NEW DEVICE
  |             |                                      |
  |--switch to->|                                      |<--Enter Discovery
  | inclusion   |                                      |    Mode
  |   mode      |----Broadcast INCLUDE_OPEN----------->|
  |             |                                      |
  |             |<---INCLUDE_REQUEST-------------------|
  |             |    (DeviceID, PublicKey,             |
  |             |     InitialCounter)                  |
  |             |                                      |
  |             |--Verify DeviceID                     |
  |             |--Encrypt Network Key                 |  ← CHANGED
  |             |--Generate Nonce                      |
  |             |                                      |
  |             |----INCLUDE_RESPONSE----------------->|
  |             |    (HubPublicKey,                    |
  |             |     EncryptedNetworkKey,             |  ← CHANGED
  |             |     EncryptedNonce,                  |
  |             |     HubInitialCounter)               |
  |             |                                      |
  |             |                              Decrypt Network Key    ← CHANGED
  |             |                              Store Hub Public Key
  |             |                              Decrypt & Verify Nonce
  |             |                              Increment Nonce
  |             |                                      |
  |             |<---INCLUDE_CONFIRM-------------------|
  |             |    (IncrementedEncryptedNonce)       |
  |             |                                      |
  |             |--Decrypt & Verify Nonce              |
  |             |                                      |
  |             |----INCLUDE_SUCCESS------------------>|
  |             |                                      |
  |             |                                      |
  |             |<========Encrypted Communication=====>|
```

### Message Topics (Unchanged)
- `INCLUDE_OPEN` (0x08): Broadcast by hub when in inclusion mode
- `INCLUDE_REQUEST` (0x06): Sent by device wanting to join
- `INCLUDE_RESPONSE` (0x07): Hub's response with encrypted network key
- `INCLUDE_CONFIRM` (0x09): Device's confirmation with incremented nonce
- `INCLUDE_SUCCESS` (0x0A): Hub's final acknowledgment

## Key Changes

### Network Key Management

#### Hub Network Key Generation
```cpp
// Hub generates network key on first startup (or from cloud)
class NetworkKeyManager {
    std::vector<byte> networkKey;      // 32-byte AES key shared by all devices
    uint32_t networkKeyVersion;       // For key rotation support

    // Generate new network key (hub startup or key rotation)
    void generateNetworkKey();

    // Get current network key for inclusion
    std::vector<byte> getCurrentNetworkKey() const;
};
```

#### Network Key Distribution (Changed)
Instead of generating unique session keys, the hub now:
1. **Encrypts the shared network key** using device's public key (ECIES)
2. **Sends encrypted network key** in INCLUDE_RESPONSE
3. **Device decrypts and stores** the network key for all future communication

### Message Payload Changes

#### INCLUDE_RESPONSE (Hub → Device) - UPDATED
```cpp
struct IncludeResponseMessage {
    uint8_t hubPublicKey[32];          // Hub's public key
    uint8_t encryptedNetworkKey[48];   // ECIES encrypted network key ← CHANGED
    uint8_t encryptedNonce[16];        // AES encrypted nonce
    uint32_t hubInitialCounter;        // Hub's initial frame counter
    uint32_t networkKeyVersion;       // Network key version ← NEW
};
```

#### Other Messages (Unchanged)
- INCLUDE_OPEN: No payload
- INCLUDE_REQUEST: DeviceID + PublicKey + InitialCounter
- INCLUDE_CONFIRM: EncryptedNonce
- INCLUDE_SUCCESS: No payload

### Device State Changes

#### Device Storage (Updated)
```cpp
// Device persistent storage
- "is": Inclusion state (1 byte)
- "nk": Network key (32 bytes)           ← CHANGED from "sk" (session key)
- "nv": Network key version (4 bytes)    ← NEW
- "pk": Device private key (32 bytes)
- "hk": Hub public key (32 bytes)
- "mc": Message counter (4 bytes)
```

#### Device Crypto Initialization (Changed)
```cpp
// After successful inclusion
int InclusionController::applyNetworkKey() {
    std::vector<byte> networkKey;
    int rc = keyManager->loadNetworkKey(networkKey);
    if (rc == RM_E_NONE) {
        SecurityParams params;
        params.method = SecurityMethod::AES;
        params.key = networkKey;           // Same key for all devices
        params.iv = std::vector<byte>(16, 0);

        rc = device.updateSecurityParams(params);
    }
    return rc;
}
```

### Hub State Changes

#### Hub Storage (Simplified)
```cpp
// Hub persistent storage (minimal)
- Hub identity (private key, device ID)
- Network key (32 bytes)
- Network key version (4 bytes)

// Cloud-recoverable state
- Device registry (ID, public key, last seen)
- Network configuration
- Device authorization list
```

#### Hub Crypto Configuration (Changed)
```cpp
// Hub uses same network key for all application traffic
int HubDevice::initialize() {
    // Load or generate network key
    std::vector<byte> networkKey = networkKeyManager->getCurrentNetworkKey();

    SecurityParams params;
    params.method = SecurityMethod::AES;
    params.key = networkKey;              // Same key used by all devices
    params.iv = std::vector<byte>(16, 0);

    return initializeAesCrypto(params);
}
```

## Operational Advantages

### Cloud Recovery Scenario
1. **Hub Failure**: New hub deployed in field
2. **Identity Recovery**: Hub loads its private key from EEPROM
3. **Network Recovery**: Hub downloads from cloud:
   - Current network key
   - Device registry (device IDs and public keys)
   - Network configuration
4. **Immediate Operation**: Hub can communicate with all existing devices
5. **No Device Re-inclusion**: Existing devices continue working seamlessly

### Device Replacement Scenario
1. **Failed Device**: Device needs replacement
2. **New Device Inclusion**: Standard inclusion protocol
3. **Network Key Distribution**: New device receives current network key
4. **Immediate Mesh Participation**: Device can communicate with all other devices

### Mesh Routing Benefits
1. **Any-to-Any Encryption**: All devices share the same encryption key
2. **Efficient Forwarding**: Intermediate nodes can decrypt/re-encrypt for routing
3. **Broadcast Support**: Hub can send encrypted broadcasts to all devices
4. **Group Communication**: Devices can communicate directly without hub mediation

## Security Considerations

### Security Model
- **Network Perimeter**: Shared network key establishes trusted network boundary
- **Inclusion Security**: ECDH + ECIES protects network key during distribution
- **Transport Security**: All application traffic encrypted with AES-256
- **Replay Protection**: Frame counters prevent replay attacks
- **Device Authentication**: Device public keys stored for identity verification

### Security Trade-offs
- ✅ **Industry Standard**: Proven security model used by major IoT protocols
- ✅ **Operational Security**: Easier key management reduces human error
- ✅ **Network Resilience**: No single point of cryptographic failure
- ⚠️ **Device Compromise**: Compromised device exposes network traffic
- ⚠️ **Key Rotation**: Network-wide key updates more complex

### Future Security Enhancements
1. **Key Rotation**: Periodic network key updates
2. **Device Revocation**: Remove compromised devices from network
3. **Security Classes**: Different keys for different device types
4. **End-to-End Encryption**: Optional additional encryption for sensitive data

## Implementation Migration

### Phase 1: Core Protocol Update
- [ ] Update KeyManager to handle network keys instead of session keys
- [ ] Modify InclusionController to distribute network key
- [ ] Update device storage schema
- [ ] Maintain backward compatibility during transition

### Phase 2: Hub State Simplification
- [ ] Implement NetworkKeyManager class
- [ ] Reduce hub persistent storage requirements
- [ ] Design cloud recovery protocol
- [ ] Update DeviceBuilder patterns

### Phase 3: Mesh Routing Optimization
- [ ] Enable direct device-to-device communication
- [ ] Implement efficient broadcast mechanisms
- [ ] Optimize routing for shared key architecture
- [ ] Performance testing and optimization

## Compatibility Notes

### Protocol Version
- Increment protocol version to indicate shared network key support
- Maintain compatibility detection during inclusion
- Graceful fallback for mixed-version networks during migration

### Storage Migration
```cpp
// Migrate existing session key storage to network key
int migrateToNetworkKey() {
    // Check if old session key exists
    std::vector<byte> oldSessionKey;
    if (storage->loadSessionKey(oldSessionKey) == RM_E_NONE) {
        // For migration, treat session key as network key
        storage->persistNetworkKey(oldSessionKey);
        storage->removeKey("sk");  // Remove old session key
    }
}
```

## Testing Strategy

### Unit Tests
- [ ] Network key generation and management
- [ ] ECIES encryption/decryption of network key
- [ ] Device storage migration
- [ ] Hub state simplification

### Integration Tests
- [ ] Full inclusion with network key distribution
- [ ] Device-to-device communication verification
- [ ] Hub recovery scenarios
- [ ] Mixed-version network compatibility

### Performance Tests
- [ ] Memory usage comparison (per-device vs shared key)
- [ ] Mesh routing efficiency
- [ ] Large network scaling (100+ devices)
- [ ] Key rotation performance impact

## Documentation Updates Required

- [ ] Update API documentation for new storage keys
- [ ] Revise security architecture documentation
- [ ] Update example applications
- [ ] Create migration guide for existing deployments
- [ ] Update DeviceBuilder security model documentation

---

**Document Status**: Draft
**Last Updated**: 2025-01-06
**Replaces**: inclusion-protocol-design.md (per-device session key approach)
**Next Review**: After implementation phase 1 completion
