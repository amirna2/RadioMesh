# Dynamic Key Exchange Implementation Plan

## Overview
This document outlines the implementation plan for replacing hardcoded encryption keys with dynamic key exchange during the device inclusion process. The goal is to enable true end-to-end encryption with keys exchanged securely during device onboarding.

## RadioMesh Security Model

### Security Architecture
RadioMesh implements a **mandatory security** model with flexible initialization timing:

#### Two-Tier Security System
1. **Hub Master Keys**: Pre-configured keys for secure inclusion protocol management
2. **Device Session Keys**: Dynamically generated and exchanged during inclusion

#### Encryption Zones
- **Inclusion Protocol**: Unencrypted (industry standard for device onboarding)
- **Application Messages**: Encrypted with AES-256 session keys

### Device Type Security Requirements

#### Hub Devices (MeshDeviceType::HUB)
- **MUST** have initial encryption configured via `withSecureMessaging()`
- Use master keys to:
  - Validate inclusion requests
  - Generate session keys for new devices
  - Encrypt session key distribution
- Master keys persist across device restarts

#### Standard Devices (MeshDeviceType::STANDARD)
- **DO NOT** require initial encryption configuration
- Receive encryption keys dynamically during inclusion:
  1. Send public key to hub during INCLUDE_REQUEST
  2. Receive encrypted session key in INCLUDE_RESPONSE
  3. Apply session key to crypto system after INCLUDE_SUCCESS
- Session keys persist across device restarts

### Message Security
- **Inclusion Messages** (topics 0x06-0x0A): Unencrypted by design
  - INCLUDE_OPEN, INCLUDE_REQUEST, INCLUDE_RESPONSE, INCLUDE_CONFIRM, INCLUDE_SUCCESS
  - Contains public keys and encrypted payloads but message headers are plain
- **Application Messages** (topics 0x10+): Encrypted with session keys
  - All user data encrypted end-to-end
  - Frame counter prevents replay attacks

### Key Lifecycle
1. **Initialization**: Hub gets master keys, devices start without crypto
2. **Inclusion**: Dynamic key exchange establishes session keys
3. **Operation**: All application traffic encrypted with session keys
4. **Persistence**: Keys stored in EEPROM, survive device restarts
5. **Revocation**: Factory reset clears all keys, requires re-inclusion

## Current State Analysis

### What We Have
1. **InclusionController** with complete key exchange mechanism:
   - ECDH key pair generation
   - Public key exchange during inclusion protocol
   - Session key generation and encryption by hub
   - KeyManager for storing keys persistently

2. **Current Limitations**:
   - Session keys are generated and stored but never used
   - Crypto system is initialized with hardcoded keys at build time
   - No mechanism to update crypto keys after inclusion
   - Examples use the same hardcoded keys for all devices

## Implementation Phases

### Phase 1: Core Infrastructure (High Priority)

#### 1.1 Add updateSecurityParams to IDevice interface
**File**: `/src/framework/interfaces/IDevice.h`
```cpp
/**
 * @brief Update the device's security parameters dynamically
 * 
 * This method allows updating the encryption keys and parameters
 * after device initialization, typically used after successful inclusion
 * to apply the negotiated session keys.
 * 
 * @param params The new security parameters to apply
 * @return RM_E_NONE on success, error code otherwise
 */
virtual int updateSecurityParams(const SecurityParams& params) = 0;
```

#### 1.2 Implement updateSecurityParams in RadioMeshDevice
**Files**: 
- `/src/framework/device/inc/Device.h` - Add method declaration
- `/src/framework/device/src/Device.cpp` - Add implementation

Implementation details:
- Check if crypto is initialized
- Call `crypto->setParams(params)` to update the AES crypto instance
- Handle error cases (no crypto, invalid params)
- Log the security update for debugging

#### 1.3 Modify InclusionController to apply session keys
**File**: `/src/framework/device/src/InclusionController.cpp`

For Standard Devices (in `handleInclusionMessage` case `INCLUDE_SUCCESS`):
```cpp
// After line 354 - state = DeviceInclusionState::INCLUDED;
// Load the session key that was received and decrypted
std::vector<byte> sessionKey;
int rc = keyManager->loadSessionKey(sessionKey);
if (rc == RM_E_NONE) {
    // Create security params with session key
    SecurityParams newParams;
    newParams.method = SecurityMethod::AES;
    newParams.key = sessionKey;
    // TODO: Handle IV properly - either derive from session key or exchange separately
    
    // Update device crypto
    device.updateSecurityParams(newParams);
    loginfo_ln("Applied session key to crypto system");
}
```

For Hub (in `sendInclusionResponse` after generating session key):
```cpp
// After line 216 - rc = keyManager->generateSessionKey(sessionKey);
// Hub should also use the session key it generated
SecurityParams newParams;
newParams.method = SecurityMethod::AES;
newParams.key = sessionKey;
device.updateSecurityParams(newParams);
```

### Phase 2: Key Persistence and Loading (High Priority)

#### 2.1 Load session keys on device startup
**File**: `/src/framework/device/src/Device.cpp`

In the `setup()` or `initialize()` method:
```cpp
// After InclusionController is initialized
if (inclusionController->getState() == DeviceInclusionState::INCLUDED) {
    // Device was previously included, load and apply session key
    int rc = inclusionController->loadAndApplySessionKey();
    if (rc != RM_E_NONE) {
        logwarn_ln("Failed to load session key, device may need re-inclusion");
    }
}
```

#### 2.2 Add helper method to InclusionController
**Files**:
- `/src/framework/device/inc/InclusionController.h` - Add method declaration
- `/src/framework/device/src/InclusionController.cpp` - Add implementation

```cpp
int InclusionController::loadAndApplySessionKey() {
    if (state != DeviceInclusionState::INCLUDED) {
        return RM_E_INVALID_STATE;
    }
    
    std::vector<byte> sessionKey;
    int rc = keyManager->loadSessionKey(sessionKey);
    if (rc != RM_E_NONE) {
        return rc;
    }
    
    SecurityParams params;
    params.method = SecurityMethod::AES;
    params.key = sessionKey;
    // TODO: IV handling
    
    return device.updateSecurityParams(params);
}
```

### Phase 3: DeviceBuilder Flexibility (Medium Priority)

#### 3.1 Make crypto initialization optional
**File**: `/src/framework/builder/src/DeviceBuilder.cpp`

Modify the build process (around line 176):
```cpp
if (blueprint.usesCrypto) {
    build_error = device->initializeAesCrypto(securityParams);
    // ... existing error handling
} else {
    // Device starts without crypto, can be enabled later via inclusion
    loginfo_ln("Device built without initial crypto - will be configured during inclusion");
}
```

#### 3.2 Consider adding withDeferredSecurity()
**File**: `/src/framework/builder/inc/DeviceBuilder.h`

Optional enhancement:
```cpp
/**
 * @brief Configure device for deferred security setup
 * 
 * Device will initialize crypto subsystem but with temporary keys
 * that will be replaced during the inclusion process
 * 
 * @return Reference to the builder
 */
DeviceBuilder& withDeferredSecurity();
```

### Phase 4: Update Examples (Medium Priority)

#### 4.1 Remove hardcoded keys
**Files**: 
- `/examples/DeviceInclusion/Standard/device_info.h`
- `/examples/DeviceInclusion/MiniHub/device_info.h`

Remove these lines:
```cpp
// Remove hardcoded keys
// std::vector<byte> key = {0x11, 0x22, ...};
// std::vector<byte> iv = {0x11, 0x22, ...};

// Add comment instead:
// Encryption keys are now dynamically exchanged during device inclusion
// No hardcoded keys needed!
```

#### 4.2 Update device initialization
**Files**:
- `/examples/DeviceInclusion/Standard/StandardDevice.ino`
- `/examples/DeviceInclusion/MiniHub/MiniHub.ino`

Remove `withSecureMessaging()` from builder chain:
```cpp
device = DeviceBuilder()
    .start()
    .withLoraRadio(radioParams)
    // .withSecureMessaging(securityParams) // REMOVED - security via inclusion
    .withRxPacketCallback(onPacketReceived)
    .build(DEVICE_NAME, DEVICE_ID, deviceType);
```

### Phase 5: Testing and Validation (Low Priority)

#### 5.1 Unit tests to add
- Test updateSecurityParams functionality
- Test key persistence and loading
- Test inclusion with dynamic keys
- Test crypto works after inclusion

#### 5.2 Integration test scenarios
1. Fresh device inclusion with key exchange
2. Device reboot after inclusion (key persistence)
3. Factory reset and re-inclusion
4. Multiple devices with unique keys

## Implementation Order

1. **Week 1**: Phase 1 (Core Infrastructure)
   - Add interface method
   - Implement in Device class
   - Update InclusionController

2. **Week 2**: Phase 2 (Key Persistence)
   - Startup key loading
   - Helper methods

3. **Week 3**: Phase 3 & 4 (Builder & Examples)
   - Make crypto optional
   - Update examples

4. **Week 4**: Phase 5 (Testing)
   - Unit tests
   - Integration tests

## Security Considerations

1. **Key Derivation**: Consider deriving IV from session key using KDF
2. **Key Rotation**: Plan for future key rotation mechanism
3. **Backward Compatibility**: Ensure devices can still work with hardcoded keys if needed
4. **Key Storage Security**: Ensure KeyManager properly protects stored keys

## Migration Path

For existing deployments using hardcoded keys:
1. Deploy hub update first with new inclusion controller
2. Factory reset devices to trigger re-inclusion
3. New devices automatically use dynamic keys
4. Consider temporary backward compatibility mode

## Success Criteria

- [ ] No hardcoded keys in example code
- [ ] Unique session keys per device
- [ ] Keys persist across device reboots
- [ ] Inclusion process handles key exchange transparently
- [ ] All tests pass with dynamic keys
- [ ] Documentation updated

## Risks and Mitigations

1. **Risk**: Breaking existing functionality
   - **Mitigation**: Extensive testing, gradual rollout

2. **Risk**: Key exchange failure during inclusion
   - **Mitigation**: Proper error handling and retry logic

3. **Risk**: Performance impact of key operations
   - **Mitigation**: Optimize crypto operations, cache where possible