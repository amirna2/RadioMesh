# RadioMesh Zephyr module glue.
#
# D2 (shared core by co-compilation): Zephyr's build compiles the SAME physical
# src/ files as PlatformIO — no submodule, no copy. Milestone 1's packet slice
# is header-only (proven by the M0 `native` gate), so all we expose here is the
# src/ include root. Later milestones add neutral .cpp sources to a
# zephyr_library() below, in place and guarded by RM_ARDUINO_BUILD/RM_GENERIC_BUILD.

if(CONFIG_RADIOMESH)
  zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/src)

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

  # Later milestones (M2 routing, etc.) append neutral sources here, e.g.:
  #   zephyr_library()
  #   zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/src/common/src/Utils.cpp)
endif()
