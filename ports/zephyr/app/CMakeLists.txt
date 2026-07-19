cmake_minimum_required(VERSION 3.20.0)

# The RadioMesh module (zephyr/module.yml) is discovered automatically when this
# repo is on the west manifest; its zephyr/CMakeLists.txt adds the src/ include
# root when CONFIG_RADIOMESH=y (set in prj.conf).
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(radiomesh_m1_bringup CXX)

target_sources(app PRIVATE src/main.cpp)
