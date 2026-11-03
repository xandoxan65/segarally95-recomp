# segamod2-diff: the host reference with every ported routine checked against its lifted original at each call.
# Run it like segamod2 (docs/dreamcast-port.md, "Differential checks").

# Ported routines: ported/*.list, one "lifted_name src/relative/file.c" per line ('#' comments).
# Each module keeps its own list and wrap_<module>.c, so ports of different modules do not touch shared files.
file(GLOB SR_DIFF_LISTS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ported/*.list)
set(SR_DIFF_PORTED)
foreach(list_file IN LISTS SR_DIFF_LISTS)
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${list_file})
  file(STRINGS ${list_file} lines)
  foreach(line IN LISTS lines)
    string(STRIP "${line}" line)
    if(line STREQUAL "" OR line MATCHES "^#")
      continue()
    endif()
    string(REGEX REPLACE "[ \t]+" ";" pair "${line}")
    list(APPEND SR_DIFF_PORTED ${pair})
  endforeach()
endforeach()
file(GLOB SR_DIFF_WRAPPERS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/wrap_*.c)

file(GLOB SR_NATIVE_RACE CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/dreamcast-native/code/race/*.c)
list(FILTER SR_NATIVE_RACE EXCLUDE REGEX "/(tgp|race_tick)\\.c$")

set(SR_DIFF_LIFT_SRCS)
foreach(src IN LISTS LIFT_HOST_RUNTIME LIFT_GAME_GLUE LIFT_GAME LIFT_HOST LIFT_LIBC LIFT_BOOT LIFT_IRQ)
  if(IS_ABSOLUTE "${src}")
    list(APPEND SR_DIFF_LIFT_SRCS "${src}")
  else()
    list(APPEND SR_DIFF_LIFT_SRCS "${CMAKE_SOURCE_DIR}/${src}")
  endif()
endforeach()

# Source properties are per directory, so these renames apply to this target only.
list(LENGTH SR_DIFF_PORTED n)
math(EXPR last "${n} - 1")
foreach(i RANGE 0 ${last} 2)
  math(EXPR j "${i} + 1")
  list(GET SR_DIFF_PORTED ${i} fn)
  list(GET SR_DIFF_PORTED ${j} file)
  set_property(SOURCE ${CMAKE_SOURCE_DIR}/src/${file} APPEND PROPERTY COMPILE_DEFINITIONS "${fn}=lifted_${fn}")
endforeach()

add_executable(segamod2-diff
  ${SR_DIFF_LIFT_SRCS}
  ${SR_NATIVE_RACE}
  sr_diff.c
  sr_tgp_host.c
  ${SR_DIFF_WRAPPERS}
)
target_include_directories(segamod2-diff PRIVATE
  ${CMAKE_SOURCE_DIR}/dreamcast-native/include
  ${CMAKE_CURRENT_SOURCE_DIR})
# Native gameplay builds exactly as on the Dreamcast: no contraction, no fast-math.
target_compile_options(segamod2-diff PRIVATE -ffp-contract=off)
target_link_libraries(segamod2-diff PRIVATE
  model2_geo model2_hw model2_tgp model2_snd host_compat)
set_target_properties(segamod2-diff PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
