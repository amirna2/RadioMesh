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
  |             |    (Hub PublicKey)                   |
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
    uint8_t hubPublicKey[64];          // Hub's public key
    uint8_t encryptedNetworkKey[32];   // Direct ECC encrypted network key (zero overhead)
    uint8_t encryptedNonce[4];         // Nonce for verification
};
```

#### Other Messages (Unchanged)
- INCLUDE_OPEN: Hub public key (64 bytes)
- INCLUDE_REQUEST: DeviceID + PublicKey + InitialCounter (72 bytes)
- INCLUDE_CONFIRM: EncryptedNonce (4 bytes)
- INCLUDE_SUCCESS: Empty payload (0 bytes)

### Device State Changes

#### Device Storage (Updated)
```cpp
// Device persistent storage
- "is": Inclusion state (1 byte)
- "nk": Network key (32 bytes)           ← CHANGED from "sk" (session key)
- "pk": Device private key (32 bytes)
- "hk": Hub public key (64 bytes)        ← CORRECTED: Full ECC public key
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

## Encryption Design - Simplified ECC Approach

### Inclusion Message Encryption Table

| Message          | Encrypted? | Crypto Used              | Key Used                    | Overhead | Purpose                                        |
|------------------|------------|--------------------------|----------------------------|----------|------------------------------------------------|
| INCLUDE_OPEN     | ❌ No       | —                        | —                          | 0 bytes  | Broadcast hub public key for bootstrapping     |
| INCLUDE_REQUEST  | ✅ Yes      | ECC (device → hub)       | Hub's public key           | 0 bytes  | Encrypts device info using hub's public key    |
| INCLUDE_RESPONSE | ✅ Yes      | ECC (hub → device)       | Device's public key        | 0 bytes  | Encrypts network key using device's public key |
| INCLUDE_CONFIRM  | ✅ Yes      | AES (shared key)         | Shared network key         | 0 bytes  | Encrypts incremented nonce                     |
| INCLUDE_SUCCESS  | ✅ Yes      | AES (shared key)         | Shared network key         | 0 bytes  | Final acknowledgment                           |

### Key Management Strategy

#### Device Key Generation
- **When**: Device initialization (after EEPROM initialization)
- **Storage**: Device stores own private key and hub's public key (from INCLUDE_OPEN)
- **Lifecycle**: Keys persist across inclusion attempts for efficiency

#### Hub Key Management
- **Storage**: Hub does NOT permanently store device public keys
- **Usage**: Hub uses device public key only during active inclusion session
- **Lifecycle**: Device public keys discarded after inclusion completion

#### Industry Alignment
This approach follows proven IoT protocols:
- **Z-Wave S2**: Direct ECC encryption without ECIES overhead
- **Zigbee 3.0**: Direct AES encryption after simple key exchange
- **Thread/Matter**: Direct ECDH without ephemeral key overhead

### Benefits of Simplified Approach
- ✅ **Zero Encryption Overhead**: No ephemeral keys, no bandwidth waste
- ✅ **Industry Proven**: Same approach as Z-Wave, Zigbee, Thread
- ✅ **Simpler Implementation**: Direct ECC encryption/decryption
- ✅ **Battery Friendly**: Less radio transmission overhead
- ✅ **Secure**: Fresh keys per device, no replay attacks across sessions

## Security Considerations

### Security Model
- **Network Perimeter**: Shared network key establishes trusted network boundary
- **Inclusion Security**: Direct ECC encryption protects sensitive data during distribution
- **Transport Security**: All application traffic encrypted with AES-256
- **Replay Protection**: Frame counters prevent replay attacks
- **Device Authentication**: Device public keys validate identity during inclusion

### Security Trade-offs
- ✅ **Industry Standard**: Proven security model used by major IoT protocols
- ✅ **Operational Security**: Easier key management reduces human error
- ✅ **Network Resilience**: No single point of cryptographic failure
- ⚠️ **Device Compromise**: Compromised device exposes network traffic
- ⚠️ **Key Rotation**: Network-wide key updates more complex

### Future Security Enhancements
1. **Install Code Authentication**: Optional Z-Wave S2 style device authentication
   - Install codes printed on device labels during manufacturing
   - User enters install code into hub during inclusion
   - Only devices with matching install codes can join network
   - Prevents unauthorized devices from joining during inclusion mode
2. **Key Rotation**: Periodic network key updates
3. **Device Revocation**: Remove compromised devices from network
4. **Security Classes**: Different keys for different device types
5. **End-to-End Encryption**: Optional additional encryption for sensitive data

## Implementation Migration

### Phase 1: Core Protocol Update
- [x] Update Device initialization to generate device key pairs (ECC - Use ESP32 chipID() as seed)
- [x] Replace ECIES with direct ECC encryption/decryption
- [x] Update KeyManager to handle network keys instead of session keys
- [x] Modify InclusionController to distribute network key
- [x] Generate device key pairs during initialization
- [x] Update device storage schema
- [x] Remove ephemeral key overhead from all inclusion messages

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
