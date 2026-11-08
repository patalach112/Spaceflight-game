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

# Copy the engine, plus any other known DLL dependencies, beside each executable.
function(aegis_copy_runtime target)
  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "$<TARGET_FILE:aegis_engine>" "$<TARGET_FILE_DIR:${target}>"
    COMMAND ${CMAKE_COMMAND}
      "-DRUNTIME_DLLS=$<JOIN:$<TARGET_RUNTIME_DLLS:${target}>,|>"
      "-DDESTINATION=$<TARGET_FILE_DIR:${target}>"
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
  # A changed DLL may not relink the executable when the import library is unchanged.
  # Copy from the engine's own post-build step too, so incremental builds never run a stale DLL.
  add_custom_command(TARGET aegis_engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:aegis_engine>" "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
    COMMAND ${CMAKE_COMMAND} "-DRUNTIME_DLLS=$<JOIN:$<TARGET_RUNTIME_DLLS:aegis_engine>,|>"
      "-DDESTINATION=${CMAKE_BINARY_DIR}/bin/$<CONFIG>" -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CopyRuntime.cmake"
    VERBATIM)
  target_link_libraries(aegis_game PRIVATE aegis_engine)
  aegis_warnings(aegis_game)
  aegis_copy_runtime(aegis_game)
  add_custom_command(TARGET aegis_game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:aegis_game>/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${CMAKE_CURRENT_SOURCE_DIR}/assets/DejaVuSans.ttf"
      "$<TARGET_FILE_DIR:aegis_game>/assets/DejaVuSans.ttf"
    VERBATIM)
endif()

if(AEGIS_BUILD_TESTS)
  enable_testing()
  add_executable(aegis_tests tests/VABSystemTests.cpp tests/VABInteractionTests.cpp)
  # Keep parallel game/test post-build copies from writing the same DLL simultaneously.
  set_target_properties(aegis_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/$<CONFIG>")
  add_custom_command(TARGET aegis_engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/tests/$<CONFIG>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:aegis_engine>" "${CMAKE_BINARY_DIR}/tests/$<CONFIG>"
    COMMAND ${CMAKE_COMMAND} "-DRUNTIME_DLLS=$<JOIN:$<TARGET_RUNTIME_DLLS:aegis_engine>,|>"
      "-DDESTINATION=${CMAKE_BINARY_DIR}/tests/$<CONFIG>" -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CopyRuntime.cmake"
    VERBATIM)
  target_link_libraries(aegis_tests PRIVATE aegis_engine)
  aegis_warnings(aegis_tests)
  aegis_copy_runtime(aegis_tests)
  add_test(NAME construction COMMAND aegis_tests)
endif()
