cmake_minimum_required(VERSION 3.16)
project(segamod2 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

# Match historical `build/segamod2` layout even with multi-config generators.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR})

include(CheckIncludeFile)
include(CheckSymbolExists)

# --- Optional deps (viewer / PNG dumps) ------------------------------------
set(SEGAMOD2_HAVE_SDL2 OFF)
set(SEGAMOD2_SDL_TARGET "")

find_package(SDL2 CONFIG QUIET)
if(SDL2_FOUND)
  if(TARGET SDL2::SDL2)
    set(SEGAMOD2_SDL_TARGET SDL2::SDL2)
    set(SEGAMOD2_HAVE_SDL2 ON)
  elseif(TARGET SDL2::SDL2-static)
    set(SEGAMOD2_SDL_TARGET SDL2::SDL2-static)
    set(SEGAMOD2_HAVE_SDL2 ON)
  endif()
endif()
if(NOT SEGAMOD2_HAVE_SDL2)
  find_package(PkgConfig QUIET)
  if(PkgConfig_FOUND)
    pkg_check_modules(PC_SDL2 QUIET sdl2)
    if(PC_SDL2_FOUND)
      add_library(segamod2_sdl2 INTERFACE)
      target_include_directories(segamod2_sdl2 INTERFACE ${PC_SDL2_INCLUDE_DIRS})
      target_link_libraries(segamod2_sdl2 INTERFACE ${PC_SDL2_LINK_LIBRARIES})
      target_compile_options(segamod2_sdl2 INTERFACE ${PC_SDL2_CFLAGS_OTHER})
      set(SEGAMOD2_SDL_TARGET segamod2_sdl2)
      set(SEGAMOD2_HAVE_SDL2 ON)
    endif()
  endif()
endif()
if(NOT SEGAMOD2_HAVE_SDL2)
  find_path(SDL2_INCLUDE_DIR SDL.h PATH_SUFFIXES SDL2 include/SDL2 include)
  find_library(SDL2_LIBRARY NAMES SDL2 SDL2d)
  if(SDL2_INCLUDE_DIR AND SDL2_LIBRARY)
    add_library(segamod2_sdl2 INTERFACE)
    target_include_directories(segamod2_sdl2 INTERFACE ${SDL2_INCLUDE_DIR})
    target_link_libraries(segamod2_sdl2 INTERFACE ${SDL2_LIBRARY})
    set(SEGAMOD2_SDL_TARGET segamod2_sdl2)
    set(SEGAMOD2_HAVE_SDL2 ON)
  endif()
endif()

find_package(PNG QUIET)
find_package(OpenGL QUIET)
find_package(Threads REQUIRED)
find_package(Python3 COMPONENTS Interpreter QUIET)

set(SEGAMOD2_HAVE_GL OFF)
if(OpenGL_FOUND)
  if(WIN32)
    if(SEGAMOD2_HAVE_SDL2)
      set(SEGAMOD2_HAVE_GL ON)
    endif()
  else()
    set(SEGAMOD2_HAVE_GL ON)
  endif()
endif()
# Darwin Makefile enabled GL whenever SDL2 was present (system OpenGL framework).
if(APPLE AND SEGAMOD2_HAVE_SDL2)
  set(SEGAMOD2_HAVE_GL ON)
endif()

set(HAVE_PTHREAD_H OFF)
set(HAVE_CLOCK_GETTIME OFF)
if(WIN32)
  unset(HAVE_PTHREAD_H)
  unset(HAVE_CLOCK_GETTIME)
  check_include_file("pthread.h" HAVE_PTHREAD_H)
  check_symbol_exists(clock_gettime "time.h" HAVE_CLOCK_GETTIME)
else()
  set(HAVE_PTHREAD_H ON)
  set(HAVE_CLOCK_GETTIME ON)
endif()

# --- Common flags / includes ------------------------------------------------
add_library(segamod2_common INTERFACE)
target_include_directories(segamod2_common INTERFACE
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/lib/model2/include
  ${CMAKE_SOURCE_DIR}/lib/model2/host
  ${CMAKE_SOURCE_DIR}/lib/model2/geo/include
  ${CMAKE_SOURCE_DIR}/lib/model2/hw/include
  ${CMAKE_SOURCE_DIR}/lib/model2/tgp/include
  ${CMAKE_SOURCE_DIR}/lib/model2/snd/include
  ${CMAKE_SOURCE_DIR}/lib/host_compat
)
if(WIN32 AND NOT HAVE_PTHREAD_H)
  target_include_directories(segamod2_common BEFORE INTERFACE
    ${CMAKE_SOURCE_DIR}/lib/host_compat/win32)
endif()

if(MSVC)
  target_compile_options(segamod2_common INTERFACE /W3 /wd4100 /wd4244 /wd4312 /utf-8)
  target_compile_definitions(segamod2_common INTERFACE
    _CRT_SECURE_NO_WARNINGS
    _CRT_NONSTDC_NO_WARNINGS)
else()
  target_compile_options(segamod2_common INTERFACE
    -Wall -Wextra -Wno-unused-parameter)
endif()

if(WIN32)
  if(MSVC)
    target_compile_options(segamod2_common INTERFACE
      "/FI${CMAKE_SOURCE_DIR}/lib/host_compat/host_compat.h")
  else()
    target_compile_options(segamod2_common INTERFACE
      "SHELL:-include ${CMAKE_SOURCE_DIR}/lib/host_compat/host_compat.h")
  endif()
endif()

if(HAVE_CLOCK_GETTIME)
  target_compile_definitions(segamod2_common INTERFACE HOST_COMPAT_HAVE_CLOCK_GETTIME)
endif()
if(SEGAMOD2_HAVE_SDL2)
  target_compile_definitions(segamod2_common INTERFACE
    I960_HOST_HAVE_SDL SDL_MAIN_HANDLED)
  target_link_libraries(segamod2_common INTERFACE ${SEGAMOD2_SDL_TARGET})
endif()
if(SEGAMOD2_HAVE_GL)
  target_compile_definitions(segamod2_common INTERFACE I960_HOST_HAVE_GL)
  if(TARGET OpenGL::GL)
    target_link_libraries(segamod2_common INTERFACE OpenGL::GL)
  elseif(APPLE)
    target_link_libraries(segamod2_common INTERFACE "-framework OpenGL")
  elseif(WIN32)
    target_link_libraries(segamod2_common INTERFACE opengl32)
  else()
    target_link_libraries(segamod2_common INTERFACE GL)
  endif()
endif()
if(PNG_FOUND)
  target_compile_definitions(segamod2_common INTERFACE I960_HOST_HAVE_PNG)
  target_link_libraries(segamod2_common INTERFACE PNG::PNG)
endif()
if(UNIX)
  target_link_libraries(segamod2_common INTERFACE m)
endif()
target_link_libraries(segamod2_common INTERFACE Threads::Threads)

# --- Model 2 static libs ----------------------------------------------------
file(GLOB MODEL2_GEO_SRCS CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/lib/model2/geo/*.c)
file(GLOB MODEL2_HW_SRCS  CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/lib/model2/hw/*.c)
file(GLOB MODEL2_TGP_SRCS CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/lib/model2/tgp/*.c)
file(GLOB MODEL2_SND_SRCS CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/lib/model2/snd/*.c)

add_library(model2_geo STATIC ${MODEL2_GEO_SRCS})
add_library(model2_hw  STATIC ${MODEL2_HW_SRCS})
add_library(model2_tgp STATIC ${MODEL2_TGP_SRCS})
add_library(model2_snd STATIC ${MODEL2_SND_SRCS})

foreach(lib model2_geo model2_hw model2_tgp model2_snd)
  target_link_libraries(${lib} PUBLIC segamod2_common)
endforeach()

# --- Host compat (pthread/time/GL loader) -----------------------------------
add_library(host_compat STATIC lib/host_compat/model2_gl.c)
target_link_libraries(host_compat PUBLIC segamod2_common)
if(WIN32 AND NOT HAVE_PTHREAD_H)
  target_sources(host_compat PRIVATE lib/host_compat/win32/host_pthread.c)
endif()
if(WIN32 AND NOT HAVE_CLOCK_GETTIME)
  target_sources(host_compat PRIVATE lib/host_compat/win32/host_time.c)
endif()

# --- Lift executable --------------------------------------------------------
set(LIFT_HOST_RUNTIME
  lib/model2/host/i960_regs.c
  lib/model2/host/i960_mem.c
  lib/model2/host/model2_rom.c
  lib/model2/host/model2_rom_check.c
  lib/model2/host/i960_host.c
  lib/model2/host/i960_host_scene.c
  lib/model2/host/i960_host_staging.c
  lib/model2/host/i960_host_staging_dispatch.c
  lib/model2/host/i960_host_dispatch.c
  lib/model2/host/lift_stubs.c
)
set(LIFT_GAME_GLUE
  src/i960_host_syms.c
  src/i960_host_invoke.c
  src/lift_main.c
)
file(GLOB LIFT_GAME CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/game/*.c)
file(GLOB LIFT_HOST CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/host/*.c)
file(GLOB LIFT_LIBC CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/libc/*.c)
file(GLOB LIFT_BOOT CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/boot/*.c)
file(GLOB LIFT_IRQ  CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/irq/*.c)

add_executable(segamod2
  ${LIFT_HOST_RUNTIME}
  ${LIFT_GAME_GLUE}
  ${LIFT_GAME}
  ${LIFT_HOST}
  ${LIFT_LIBC}
  ${LIFT_BOOT}
  ${LIFT_IRQ}
)
target_link_libraries(segamod2 PRIVATE
  model2_geo model2_hw model2_tgp model2_snd host_compat)

# --- ROM extract ------------------------------------------------------------
if(DEFINED ENV{SEGAMOD2_ROM_DIR} AND NOT "$ENV{SEGAMOD2_ROM_DIR}" STREQUAL "")
  set(SEGAMOD2_ROM_DIR "$ENV{SEGAMOD2_ROM_DIR}")
else()
  set(SEGAMOD2_ROM_DIR "${CMAKE_SOURCE_DIR}/ROMS/srallyc-b")
endif()

set(ROM_MAINCPU ${CMAKE_SOURCE_DIR}/out/i960/maincpu_deinterleaved.bin)
set(ROM_MAINDATA ${CMAKE_SOURCE_DIR}/out/i960/main_data_deinterleaved.bin)
set(EXTRACT_SCRIPT ${CMAKE_SOURCE_DIR}/scripts/extract_rom_blocks.py)

if(Python3_Interpreter_FOUND)
  add_custom_command(
    OUTPUT ${ROM_MAINCPU} ${ROM_MAINDATA}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/out/i960
    COMMAND ${Python3_EXECUTABLE} ${EXTRACT_SCRIPT}
            --rom-dir ${SEGAMOD2_ROM_DIR}
            --out-dir ${CMAKE_SOURCE_DIR}/out/i960
    DEPENDS ${EXTRACT_SCRIPT}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Extracting ROM blocks into out/i960"
    VERBATIM
  )
  add_custom_target(rom-blocks DEPENDS ${ROM_MAINCPU} ${ROM_MAINDATA})
else()
  add_custom_target(rom-blocks
    COMMAND ${CMAKE_COMMAND} -E echo "error: Python3 is required for rom-blocks"
    COMMAND ${CMAKE_COMMAND} -E false)
endif()

add_custom_target(lift-check DEPENDS segamod2)
# Host auto-extracts out/i960 bins on first run; rom-blocks is optional (Python).
add_custom_target(lift DEPENDS segamod2)

function(segamod2_run_target name)
  set(extra_args ${ARGN})
  add_custom_target(${name}
    COMMAND ${CMAKE_COMMAND} -E env
            SEGAMOD2_ROOT=${CMAKE_SOURCE_DIR}
            SEGAMOD2_ROM_DIR=${SEGAMOD2_ROM_DIR}
            $<TARGET_FILE:segamod2> ${extra_args}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS segamod2
    USES_TERMINAL
  )
endfunction()

segamod2_run_target(lift-boot-viewer)
segamod2_run_target(lift-boot-practice --practice)
segamod2_run_target(lift-boot-headless --headless)
add_custom_target(lift-boot-live DEPENDS lift-boot-viewer)

add_custom_target(lift-cgm-decode
  COMMAND ${CMAKE_COMMAND} -E env
          SEGAMOD2_ROOT=${CMAKE_SOURCE_DIR}
          SEGAMOD2_ROM_DIR=${SEGAMOD2_ROM_DIR}
          $<TARGET_FILE:segamod2> --decode-cgm 0x2879db0
          --palette-dump ${CMAKE_BINARY_DIR}/lift/cgm_decode
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  DEPENDS segamod2
  USES_TERMINAL
)

add_custom_target(lift-main
  COMMAND ${CMAKE_COMMAND} -E echo
          "Clone tools/ then: python -m tools.decomp.gen_lift_main (see README)")

# --- Status -----------------------------------------------------------------
message(STATUS "segamod2: SDL2=${SEGAMOD2_HAVE_SDL2} PNG=${PNG_FOUND} GL=${SEGAMOD2_HAVE_GL}")
if(NOT SEGAMOD2_HAVE_SDL2)
  message(STATUS "  live viewer compiled out — install SDL2 and reconfigure")
endif()
if(NOT PNG_FOUND)
  message(STATUS "  PNG dumps compiled out — install libpng and reconfigure")
endif()
if(NOT SEGAMOD2_HAVE_GL)
  message(STATUS "  OpenGL composite compiled out")
endif()
