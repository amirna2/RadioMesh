cmake_minimum_required(VERSION 3.20.0)

# On-board test runner: builds ONE suite from the repo's test/ directory
# (selected with -DSUITE=<directory name>) against the full RadioMesh library,
# with Unity and a small adapter that runs the suite and reports the result.
# Run it through `make test SUITE=<name>` (ports/zephyr/Makefile).

# Same kit overlay as the examples (single source of truth in ../../common).
# Must be set before find_package(Zephyr).
set(DTC_OVERLAY_FILE "${CMAKE_CURRENT_LIST_DIR}/../../common/xiao_esp32s3_procpu.overlay")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(radiomesh_test_runner C CXX)

set(RM_REPO ${CMAKE_CURRENT_LIST_DIR}/../../../..)
set(RM_SUITE_DIR ${RM_REPO}/test/${SUITE})
if(NOT SUITE OR NOT IS_DIRECTORY ${RM_SUITE_DIR})
  message(FATAL_ERROR "RadioMesh tests: set -DSUITE=<directory in test/>, e.g. -DSUITE=test_Packet (got '${SUITE}')")
endif()

# Unity: west project `unity`, pinned in west.yml to the version PlatformIO
# uses. Our manifest places every project next to this repo in the west topdir.
set(RM_UNITY_DIR ${RM_REPO}/../modules/test/unity/src)
if(NOT EXISTS ${RM_UNITY_DIR}/unity.c)
  message(FATAL_ERROR "RadioMesh tests: Unity not found at ${RM_UNITY_DIR}; run `west update unity`")
endif()

# Same flags as the PlatformIO test environments (pio-config/common-test.ini),
# build-wide so the library logs at the same level as on Arduino.
zephyr_compile_definitions(UNIT_TEST RM_LOG_VERBOSE)

file(GLOB RM_SUITE_SOURCES ${RM_SUITE_DIR}/*.cpp)
# Native-style suites define main(); rename it so the adapter's main() runs it.
set_source_files_properties(${RM_SUITE_SOURCES} PROPERTIES COMPILE_DEFINITIONS main=rm_test_main)

target_compile_definitions(app PRIVATE UNITY_INCLUDE_CONFIG_H)
target_include_directories(app PRIVATE inc ${RM_UNITY_DIR})
target_sources(app PRIVATE src/TestRunner.cpp ${RM_UNITY_DIR}/unity.c ${RM_SUITE_SOURCES})
