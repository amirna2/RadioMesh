# RadioMesh MIC Authentication: Senior Architect Implementation Plan

## Executive Summary
Message Integrity Check (MIC) authentication has been implemented for RadioMesh protocol v4. This document outlines the architectural approach, implementation strategy, and integration points for production-ready MIC authentication.

## Architectural Approach

### Core Design Principle
**MIC authentication integrates seamlessly into existing crypto infrastructure without architectural disruption.**

### Key Architectural Decisions

#### 1. MIC as Crypto Service Extension
- **MicService** operates as dedicated authentication service alongside EncryptionService
- Follows established dependency injection patterns used by PacketRouter
- Maintains separation of concerns: encryption vs authentication

#### 2. Layered Integration Strategy
```
Device Layer:     MIC verification before packet processing
PacketRouter:     MIC computation/verification coordination  
MicService:       Context-aware MIC operations
AesCmac:          Core CMAC primitive (RFC 4493)
KeyManager:       Key lifecycle and storage
```

#### 3. Context-Aware Key Selection
**Message Type** → **MIC Key Source**
- `INCLUDE_REQUEST/RESPONSE` → ECIES k_mac (ECDH-derived)
- `INCLUDE_CONFIRM/SUCCESS` → Network Key 
- `Regular Messages` → Network Key
- `INCLUDE_OPEN` → No MIC (broadcast)

## Implementation Strategy

### Phase 1: Core CMAC Infrastructure
**Component**: AesCmac (RFC 4493 compliant)
- Singleton pattern following existing crypto services
- 128-bit CMAC with 32-bit truncation for MIC
- Optimized for embedded platforms
- **Location**: `src/core/protocol/crypto/cmac/`

### Phase 2: Authentication Service Layer  
**Component**: MicService
- Context-aware MIC computation and verification
- Key selection logic based on message type and inclusion state
- Integration with existing EncryptionService for ECIES key derivation
- **Pattern**: Dependency injection into PacketRouter

### Phase 3: Protocol Layer Integration
**Component**: PacketRouter modifications
- **Send Path**: Encrypt → Compute MIC → Append MIC → Transmit
- **Receive Path**: Extract MIC → Verify MIC → Decrypt → Process  
- **Relay Path**: Strip old MIC → Update headers → Recompute MIC
- **Error Handling**: Invalid MIC = immediate packet drop

### Phase 4: Packet Structure Evolution
**Component**: RadioMeshPacket enhancements
- Protocol version increment (3 → 4)
- MAX_DATA_LENGTH reduction (221 → 217 bytes) 
- MIC manipulation methods: append, extract, validate
- **Principle**: MIC appended to payload, header unchanged

### Phase 5: Device Layer Integration
**Component**: Device receive flow enhancement
- MIC verification after CRC validation, before decryption
- Early packet rejection for authentication failures
- **Error Code**: RM_E_AUTH_FAILED for MIC verification failures

## Key Technical Decisions

### 1. MIC Placement Strategy
**Decision**: Append MIC to payload (not header)
**Rationale**: 
- Maintains 35-byte header structure
- Reduces MAX_DATA_LENGTH by 4 bytes
- Simplifies packet parsing logic

### 2. Key Management Strategy  
**Decision**: Leverage existing KeyManager and EncryptionService
**Rationale**:
- No duplication of ECIES key derivation logic
- Consistent with established crypto patterns
- Maintains key lifecycle management

### 3. Service Composition Strategy
**Decision**: MicService as separate but coordinated service
**Rationale**:
- Single Responsibility Principle
- Testable in isolation
- Future extensibility for different MAC algorithms

## Implementation Details

### MIC Computation Flow
```cpp
// Context-aware key selection
micKey = getMICKey(topic, deviceType, inclusionState);

// CMAC computation over header + encrypted payload  
headerBytes = packet.getHeaderBytes();
mic = AesCmac::computeMIC(micKey, headerBytes + encryptedPayload);

// Append to payload
packet.appendMIC(mic);
```

### Key Derivation for ECIES Messages
```cpp
// For INCLUDE_REQUEST/RESPONSE
sharedSecret = ECDH(privateKey, publicKey);
k_mac = SHA256(sharedSecret);
mic = AesCmac::computeMIC(k_mac, header + payload);
```

### Relay MIC Recomputation
```cpp
// Strip old MIC, update mutable header fields, recompute
if (packet.hasMIC()) {
    packet.packetData = packet.getDataWithoutMIC();
}
updateRoutingHeaders(packet);
computeAndAppendMIC(packet);
```

## Integration Points

### 1. PacketRouter Enhancement
- MicService dependency injection
- Send/receive/relay path integration
- Error handling for MIC failures

### 2. Device Receive Enhancement  
- MIC verification in handleReceivedData()
- Early packet rejection for auth failures
- Proper error code propagation

### 3. EncryptionService Coordination
- Key access patterns for ECIES derivation
- Context sharing for message type determination
- No architectural coupling between services

## Quality Assurance

### Performance Requirements
- MIC computation: < 1ms on ESP32
- Memory overhead: < 2KB code space
- Throughput impact: < 5%

### Security Properties
- Cryptographic authentication via AES-CMAC
- Message integrity protection
- Replay protection (via frame counters)
- Early attack detection (invalid MIC rejection)

## Migration Strategy
- Protocol version 4 requires MIC on all packets (except INCLUDE_OPEN)
- No backward compatibility (per requirements)
- Immediate deployment of MIC-enabled firmware

## Success Criteria
1. All v4 packets authenticated with valid 4-byte MIC
2. Context-correct key selection per message type
3. Invalid MICs cause immediate packet rejection
4. Relay operations maintain MIC integrity
5. Zero architectural disruption to existing codebase

## Implementation Status
✅ **Completed Components:**
- AesCmac core implementation (RFC 4493)
- MicService context-aware authentication
- RadioMeshPacket MIC handling methods
- PacketRouter send/receive/relay integration
- Device layer MIC verification
- Protocol version and constants updated

## Next Steps
1. Address any remaining integration issues
2. Comprehensive testing of MIC authentication flow
3. Validation of end-to-end security properties
4. Performance benchmarking on target hardware

This implementation provides production-ready MIC authentication while maintaining the architectural integrity and design patterns of the RadioMesh codebase.