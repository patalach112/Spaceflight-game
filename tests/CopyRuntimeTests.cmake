if(NOT DEFINED TEST_ROOT OR NOT DEFINED COPY_SCRIPT)
  message(FATAL_ERROR "TEST_ROOT and COPY_SCRIPT are required")
endif()
# Include spaces and shell metacharacters in the paths; these must remain data.
set(root "${TEST_ROOT}/directory with spaces & symbols")
file(REMOVE_RECURSE "${TEST_ROOT}")
file(MAKE_DIRECTORY "${root}/source")
file(WRITE "${root}/source/aegis_engine.dll" "engine v1")
file(WRITE "${root}/source/first dep.dll" "first")
file(WRITE "${root}/source/second & dep.dll" "second")
file(WRITE "${root}/source/font.ttf" "font")
set(manifest "${root}/copy manifest.cmake")
file(WRITE "${manifest}"
  "set(AEGIS_ENGINE [==[${root}/source/aegis_engine.dll]==])\n"
  "set(AEGIS_DLLS [==[${root}/source/first dep.dll;${root}/source/second & dep.dll]==])\n"
  "set(AEGIS_DESTINATIONS [==[${root}/game bin;${root}/test bin]==])\n"
  "set(AEGIS_FONT [==[${root}/source/font.ttf]==])\n")
function(run_copy)
  execute_process(COMMAND "${CMAKE_COMMAND}" "-DMANIFEST=${manifest}" -P "${COPY_SCRIPT}"
    RESULT_VARIABLE result)
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "Runtime copy failed: ${result}")
  endif()
endfunction()
run_copy()
foreach(destination IN ITEMS "${root}/game bin" "${root}/test bin")
  foreach(name IN ITEMS "aegis_engine.dll" "first dep.dll" "second & dep.dll" "assets/DejaVuSans.ttf")
    if(NOT EXISTS "${destination}/${name}")
      message(FATAL_ERROR "Missing copied file: ${destination}/${name}")
    endif()
  endforeach()
endforeach()
# An engine-only rebuild must update the copy without relying on an exe relink.
file(WRITE "${root}/source/aegis_engine.dll" "engine v2")
run_copy()
file(READ "${root}/game bin/aegis_engine.dll" actual)
if(NOT actual STREQUAL "engine v2")
  message(FATAL_ERROR "Engine runtime copy was not refreshed")
endif()
# Empty DLL lists are valid (e.g. a static Raylib build).
file(WRITE "${manifest}"
  "set(AEGIS_ENGINE [==[${root}/source/aegis_engine.dll]==])\n"
  "set(AEGIS_DLLS [==[]==])\n"
  "set(AEGIS_DESTINATIONS [==[${root}/empty deps]==])\n")
run_copy()
if(NOT EXISTS "${root}/empty deps/aegis_engine.dll")
  message(FATAL_ERROR "Empty dependency list did not copy engine")
endif()
# A missing listed dependency is a build error, not a silently incomplete distribution.
file(WRITE "${manifest}"
  "set(AEGIS_ENGINE [==[${root}/source/aegis_engine.dll]==])\n"
  "set(AEGIS_DLLS [==[${root}/source/missing.dll]==])\n"
  "set(AEGIS_DESTINATIONS [==[${root}/missing deps]==])\n")
execute_process(COMMAND "${CMAKE_COMMAND}" "-DMANIFEST=${manifest}" -P "${COPY_SCRIPT}"
  RESULT_VARIABLE missing_result OUTPUT_QUIET ERROR_QUIET)
if(missing_result EQUAL 0)
  message(FATAL_ERROR "Missing dependencies must cause the copy step to fail")
endif()
