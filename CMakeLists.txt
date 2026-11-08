cmake_minimum_required(VERSION 3.24)
project(AegisSpaceflight VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
option(AEGIS_BUILD_GAME "Build the Raylib application" ON)
option(AEGIS_BUILD_TESTS "Build the headless construction tests" ON)

add_library(aegis_engine SHARED
  src/PartCatalog.cpp src/VABSystem.cpp src/VABInteraction.cpp)
target_include_directories(aegis_engine PUBLIC include)
target_compile_definitions(aegis_engine PRIVATE AEGIS_ENGINE_EXPORTS)
set_target_properties(aegis_engine PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/engine/$<CONFIG>"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/engine/$<CONFIG>"
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/engine/$<CONFIG>")
if(WIN32)
  set_target_properties(aegis_engine PROPERTIES PREFIX "")
endif()

function(aegis_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /permissive-)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endfunction()
aegis_warnings(aegis_engine)

# On a Windows host, use a native executable path so cmd.exe does not parse
# the slash after a drive letter as an option. VERBATIM quotes paths with spaces.
if(CMAKE_HOST_WIN32)
  file(TO_NATIVE_PATH "${CMAKE_COMMAND}" AEGIS_CMAKE_EXECUTABLE)
else()
  set(AEGIS_CMAKE_EXECUTABLE "${CMAKE_COMMAND}")
endif()

# Generate a per-configuration manifest so DLL lists and paths are read by CMake,
# never interpreted as command text (notably by cmd.exe's pipe operator).
function(aegis_deploy target copy_font)
  set(manifest "${CMAKE_CURRENT_BINARY_DIR}/runtime/${target}-$<CONFIG>.cmake")
  set(contents "set(AEGIS_ENGINE [==[$<TARGET_FILE:aegis_engine>]==])\n")
  string(APPEND contents "set(AEGIS_DLLS [==[$<TARGET_RUNTIME_DLLS:${target}>]==])\n")
  set(destinations "")
  foreach(destination IN LISTS ARGN)
    list(APPEND destinations "${destination}")
  endforeach()
  string(APPEND contents "set(AEGIS_DESTINATIONS [==[${destinations}]==])\n")
  if(copy_font)
    string(APPEND contents "set(AEGIS_FONT [==[${CMAKE_CURRENT_SOURCE_DIR}/assets/DejaVuSans.ttf]==])\n")
  endif()
  file(GENERATE OUTPUT "${manifest}" CONTENT "${contents}" TARGET ${target})
  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND "${AEGIS_CMAKE_EXECUTABLE}" "-DMANIFEST=${manifest}"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CopyRuntime.cmake"
    VERBATIM)
endfunction()

if(AEGIS_BUILD_GAME)
  find_package(raylib 5.5 QUIET)
  if(NOT raylib_FOUND)
    include(FetchContent)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(raylib URL https://codeload.github.com/raysan5/raylib/tar.gz/refs/tags/5.5
      URL_HASH SHA256=aea98ecf5bc5c5e0b789a76de0083a21a70457050ea4cc2aec7566935f5e258e
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(raylib)
  endif()
  target_sources(aegis_engine PRIVATE src/Game.cpp src/Renderer.cpp src/UIComponents.cpp)
  target_link_libraries(aegis_engine PRIVATE raylib)
  add_executable(aegis_game src/main.cpp)
  set_target_properties(aegis_game PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
  target_link_libraries(aegis_game PRIVATE aegis_engine)
  aegis_warnings(aegis_game)
  aegis_deploy(aegis_game TRUE "$<TARGET_FILE_DIR:aegis_game>")
endif()

if(AEGIS_BUILD_TESTS)
  enable_testing()
  add_executable(aegis_tests tests/VABSystemTests.cpp tests/VABInteractionTests.cpp)
  # Keep parallel game/test post-build copies from writing the same DLL simultaneously.
  set_target_properties(aegis_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/$<CONFIG>")
  target_link_libraries(aegis_tests PRIVATE aegis_engine)
  aegis_warnings(aegis_tests)
  aegis_deploy(aegis_tests FALSE "$<TARGET_FILE_DIR:aegis_tests>")
  add_test(NAME construction COMMAND aegis_tests)
  add_test(NAME runtime_copy COMMAND "${CMAKE_COMMAND}"
    "-DTEST_ROOT=${CMAKE_CURRENT_BINARY_DIR}/runtime-copy-test"
    "-DCOPY_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/cmake/CopyRuntime.cmake"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/CopyRuntimeTests.cmake")
endif()

# An engine rebuild need not relink the executables. Refresh their runtime copies
# directly from the engine's post-build step as well as from each executable.
set(engine_destinations "")
if(AEGIS_BUILD_GAME)
  list(APPEND engine_destinations "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
endif()
if(AEGIS_BUILD_TESTS)
  list(APPEND engine_destinations "${CMAKE_BINARY_DIR}/tests/$<CONFIG>")
endif()
if(engine_destinations)
  aegis_deploy(aegis_engine FALSE ${engine_destinations})
endif()
