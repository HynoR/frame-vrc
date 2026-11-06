# frame-vrc-panel: SteamVR dashboard panel for frame-vrc. MIT License (see LICENSE; the frameeyeosc helpers:
# src/third_party_frameeyeosc/LICENSE.frameeyeosc; OpenVR header: third_party/openvr/LICENSE).
cmake_minimum_required(VERSION 3.16)
project(frame-vrc-panel VERSION 0.7.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Where libopenvr_api.so is at link time. Nothing is vendored: on the Frame (aarch64) it is SteamVR's own copy,
# which is also used at run time (rpath below). Elsewhere pass -DOPENVR_LIB_DIR=... .
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
  set(_ovr_libdir "/opt/steamvr/bin/linuxarm64")
  set(_ovr_runtime "/opt/steamvr/bin/linuxarm64:$ORIGIN")
else()
  set(_ovr_libdir "")
  set(_ovr_runtime "$ORIGIN")
endif()
set(OPENVR_LIB_DIR "${_ovr_libdir}" CACHE PATH "dir with libopenvr_api.so")
set(OPENVR_RUNTIME_RPATH "${_ovr_runtime}" CACHE STRING "rpath for libopenvr_api.so at run time")

find_package(PkgConfig REQUIRED)
pkg_check_modules(CAIRO REQUIRED IMPORTED_TARGET cairo)
pkg_check_modules(FREETYPE REQUIRED IMPORTED_TARGET freetype2)
pkg_check_modules(VULKAN REQUIRED IMPORTED_TARGET vulkan)
find_package(Threads REQUIRED)

find_library(OPENVR_API openvr_api PATHS "${OPENVR_LIB_DIR}" NO_DEFAULT_PATH)
if(NOT OPENVR_API)
  find_library(OPENVR_API openvr_api)
endif()
if(NOT OPENVR_API)
  message(FATAL_ERROR "libopenvr_api.so not found: set -DOPENVR_LIB_DIR=... (on the Frame: /opt/steamvr/bin/linuxarm64, "
                      "i.e. SteamVR must be installed)")
endif()

add_executable(frame-vrc-panel
  src/main.cpp
  src/model.cpp
  src/monitor.cpp
  src/ui.cpp
  src/spawn.cpp
  src/vr_overlay.cpp
  src/third_party_frameeyeosc/draw.cpp
  src/third_party_frameeyeosc/icons.cpp
  src/third_party_frameeyeosc/text_layout.cpp
  src/third_party_frameeyeosc/json.cpp
  src/third_party_frameeyeosc/vk_texture.cpp)
target_include_directories(frame-vrc-panel PRIVATE src src/third_party_frameeyeosc third_party/openvr)
target_compile_definitions(frame-vrc-panel PRIVATE FRAME_VRC_PANEL_VERSION="${PROJECT_VERSION}")
target_compile_options(frame-vrc-panel PRIVATE -Wall -Wextra -Wno-unused-parameter)
target_link_libraries(frame-vrc-panel PRIVATE PkgConfig::CAIRO PkgConfig::FREETYPE PkgConfig::VULKAN
                      Threads::Threads "${OPENVR_API}")
# libstdc++/libgcc linked statically: the binary then needs only glibc, cairo, freetype, vulkan and openvr from the
# system (no dependency on the Frame's exact GCC version)
target_link_options(frame-vrc-panel PRIVATE -static-libstdc++ -static-libgcc)
set_target_properties(frame-vrc-panel PROPERTIES
  BUILD_RPATH "${OPENVR_LIB_DIR}"
  INSTALL_RPATH "${OPENVR_RUNTIME_RPATH}"
  BUILD_WITH_INSTALL_RPATH ON
  SKIP_BUILD_RPATH OFF)
