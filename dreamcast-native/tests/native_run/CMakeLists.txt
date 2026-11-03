# sr-native-run: the native race tick alone on the host, from a segamod2 snapshot (native_run.c).
file(GLOB SR_RUN_RACE CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/dreamcast-native/code/race/*.c)
file(GLOB SR_RUN_HOOKS CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/dreamcast-native/code/hooks/*.c)
add_executable(sr-native-run
  native_run.c
  preview.c
  ${CMAKE_SOURCE_DIR}/dreamcast-native/code/render/course_render.c
  ${CMAKE_SOURCE_DIR}/dreamcast-native/code/render/actors.c
  ${CMAKE_SOURCE_DIR}/dreamcast-native/code/render/hud2d.c
  ${CMAKE_SOURCE_DIR}/dreamcast-native/code/render/countdown.c
  ${SR_RUN_RACE}
  ${SR_RUN_HOOKS}
  ${CMAKE_SOURCE_DIR}/dreamcast-native/code/io_board.c
  ${CMAKE_SOURCE_DIR}/dreamcast-native/code/sr_mem.c
)
target_include_directories(sr-native-run PRIVATE
  ${CMAKE_SOURCE_DIR}/dreamcast-native/include
  ${CMAKE_SOURCE_DIR}/lib/model2/include
  ${CMAKE_SOURCE_DIR}/lib/model2/host
  ${CMAKE_SOURCE_DIR}/lib/model2/tgp/include)
target_compile_options(sr-native-run PRIVATE -ffp-contract=off)
target_compile_definitions(sr-native-run PRIVATE SR_MEM_TRACE)
target_link_libraries(sr-native-run PRIVATE model2_tgp m)
set_target_properties(sr-native-run PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
