cmake_minimum_required(VERSION 3.20.0)

# Kit-specific: force the shared XIAO ESP32-S3 + Wio-SX1262 pin overlay (single
# source of truth in ../../common). These examples target that kit only, so we
# apply the overlay explicitly rather than relying on per-board discovery.
# Must be set before find_package(Zephyr).
set(DTC_OVERLAY_FILE "${CMAKE_CURRENT_LIST_DIR}/../../common/xiao_esp32s3_procpu.overlay")

# The RadioMesh module (zephyr/module.yml) is discovered automatically when this
# repo is on the west manifest; its zephyr/CMakeLists.txt builds the full
# library when CONFIG_RADIOMESH=y (set in prj.conf).
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(radiomesh_standard CXX)

# RadioMesh log level, set build-wide so the library's own logs print too (the
# Logger macros expand per translation unit). Mirrors the Arduino
# StandardDevice example's -DRM_LOG_DEBUG.
zephyr_compile_definitions(RM_LOG_DEBUG)

target_sources(app PRIVATE src/main.cpp)
