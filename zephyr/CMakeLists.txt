# RadioMesh Zephyr module glue.
#
# D2 (shared core by co-compilation): Zephyr's build compiles the SAME physical
# src/ files as PlatformIO — no submodule, no copy. The portable core
# (src/common, src/core, src/framework) is compiled as-is; the Zephyr
# implementations of the platform contract, radio and storage come from
# ports/zephyr/hal. Arduino-only code (src/hardware) is never built here.

if(CONFIG_RADIOMESH)
  # Public include roots: the core headers (src/) and the application API
  # (include/RadioMesh.h). Display and WiFi are not ported; compile them out.
  zephyr_include_directories(
    ${ZEPHYR_CURRENT_MODULE_DIR}/src
    ${ZEPHYR_CURRENT_MODULE_DIR}/include
  )
  zephyr_compile_definitions(RM_NO_DISPLAY RM_NO_WIFI)

  # Crypto library: west project `crypto`, pinned in west.yml to the commit the
  # PlatformIO build uses. Our manifest places every project next to this repo
  # in the west topdir. Only the modules the core uses are compiled; RNG.cpp is
  # never called by the core, and AESEsp32.cpp is selected by AES.h only when
  # ESP32 is defined (Arduino), so Zephyr uses the portable AES implementation.
  set(RM_CRYPTO_DIR ${ZEPHYR_CURRENT_MODULE_DIR}/../modules/lib/crypto/src)
  if(NOT EXISTS ${RM_CRYPTO_DIR}/Crypto.h)
    message(FATAL_ERROR "RadioMesh: Crypto sources not found at ${RM_CRYPTO_DIR}; run `west update crypto`")
  endif()
  zephyr_include_directories(${RM_CRYPTO_DIR})
  zephyr_library_named(radiomesh_crypto)
  zephyr_library_sources(
    ${RM_CRYPTO_DIR}/AES128.cpp
    ${RM_CRYPTO_DIR}/AES256.cpp
    ${RM_CRYPTO_DIR}/AESCommon.cpp
    ${RM_CRYPTO_DIR}/BigNumberUtil.cpp
    ${RM_CRYPTO_DIR}/BlockCipher.cpp
    ${RM_CRYPTO_DIR}/CTR.cpp
    ${RM_CRYPTO_DIR}/Cipher.cpp
    ${RM_CRYPTO_DIR}/Crypto.cpp
    ${RM_CRYPTO_DIR}/Curve25519.cpp
    ${RM_CRYPTO_DIR}/Hash.cpp
    ${RM_CRYPTO_DIR}/SHA256.cpp
  )

  # Portable core + Zephyr HAL. The HAL headers stay private to the library:
  # applications reach hardware only through the IRadio/IByteStorage interfaces.
  set(RM_SRC ${ZEPHYR_CURRENT_MODULE_DIR}/src)
  set(RM_HAL ${ZEPHYR_CURRENT_MODULE_DIR}/ports/zephyr/hal)
  zephyr_library_named(radiomesh)
  zephyr_library_include_directories(${RM_HAL}/inc)
  zephyr_library_sources(
    ${RM_SRC}/common/utils/Utils.cpp
    ${RM_SRC}/core/protocol/src/crypto/EncryptionService.cpp
    ${RM_SRC}/core/protocol/src/crypto/MicService.cpp
    ${RM_SRC}/core/protocol/src/crypto/aes/AesCrypto.cpp
    ${RM_SRC}/core/protocol/src/crypto/cmac/AesCmac.cpp
    ${RM_SRC}/core/protocol/src/routing/PacketRouter.cpp
    ${RM_SRC}/core/protocol/src/routing/RoutingTable.cpp
    ${RM_SRC}/framework/builder/src/DeviceBuilder.cpp
    ${RM_SRC}/framework/device/src/Device.cpp
    ${RM_SRC}/framework/device/src/InclusionController.cpp
    ${RM_SRC}/framework/device/src/KeyManager.cpp
    ${RM_SRC}/framework/device_portal/src/AsyncDevicePortal.cpp
    ${RM_HAL}/src/platform/ZephyrPlatform.cpp
    ${RM_HAL}/src/radio/ZephyrLoraRadio.cpp
    ${RM_HAL}/src/storage/ZephyrNVSStorage.cpp
  )
endif()
