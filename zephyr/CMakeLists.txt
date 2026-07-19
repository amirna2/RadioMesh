# RadioMesh Zephyr module glue.
#
# D2 (shared core by co-compilation): Zephyr's build compiles the SAME physical
# src/ files as PlatformIO — no submodule, no copy. Milestone 1's packet slice
# is header-only (proven by the M0 `native` gate), so all we expose here is the
# src/ include root. Later milestones add neutral .cpp sources to a
# zephyr_library() below, in place and guarded by RM_ARDUINO_BUILD/RM_GENERIC_BUILD.

if(CONFIG_RADIOMESH)
  zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/src)

  # Later milestones (M2 routing, etc.) append neutral sources here, e.g.:
  #   zephyr_library()
  #   zephyr_library_sources(${ZEPHYR_CURRENT_MODULE_DIR}/src/common/src/Utils.cpp)
endif()
