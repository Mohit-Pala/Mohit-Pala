cmake_minimum_required(VERSION 3.20)
project(TraversalMaster LANGUAGES CXX RC)

# Traversal Master: an ASI plugin for Crimson Desert that reduces or removes
# the stamina cost of sprinting, climbing, swimming, gliding and riding.
#
# This build is the probe described in private/FEASIBILITY.md. It reads the
# running game's statusinfo and skill tables and writes a report; it changes
# nothing. The file it produces is TraversalMaster.asi.

if(NOT WIN32)
    message(FATAL_ERROR "Traversal Master targets Windows only.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>") # static CRT, no redist needed

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

# MinHook is here only for its HDE64 length decoder, which farhook uses to
# measure the bytes it steals. No MinHook hook is ever created.
FetchContent_Declare(
    minhook
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(minhook)

# TraversalMaster.ini's own bytes, as a header, so a DMM install (which is the
# plugin alone) can have the documented ini written beside it on first run
# without a second copy of the text going out of step with the first.
# embed_ini.cmake reads and writes HEX on purpose: carrying the file through
# CMake as a string rewrites every LF as CRLF.
set(SM_INI_HEADER "${CMAKE_CURRENT_BINARY_DIR}/generated/ini_default.h")
add_custom_command(
    OUTPUT "${SM_INI_HEADER}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/generated"
    COMMAND ${CMAKE_COMMAND} "-DIN=${CMAKE_CURRENT_SOURCE_DIR}/TraversalMaster.ini"
                             "-DOUT=${SM_INI_HEADER}"
                             -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_ini.cmake"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/TraversalMaster.ini"
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_ini.cmake"
    COMMENT "Embedding TraversalMaster.ini"
    VERBATIM
)

add_library(TraversalMaster SHARED
    "${SM_INI_HEADER}"
    src/dllmain.cpp
    src/core/mod.cpp
    src/core/log.cpp
    src/core/paths.cpp
    src/game/mem.cpp
    src/game/farhook.cpp
    src/game/tables.cpp
    src/game/stamina.cpp
    src/resources.rc
)

# TraversalMaster.ini again, this time as the INIMETA resource (src/resources.rc);
# rebuild when the ini's ;@ directives or defaults change.
set_source_files_properties(src/resources.rc PROPERTIES OBJECT_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/TraversalMaster.ini")

target_include_directories(TraversalMaster PRIVATE src "${CMAKE_CURRENT_BINARY_DIR}/generated" ${minhook_SOURCE_DIR}/src/hde)
target_link_libraries(TraversalMaster PRIVATE minhook)
target_compile_definitions(TraversalMaster PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)
target_compile_options(TraversalMaster PRIVATE /utf-8 /W3)

set_target_properties(TraversalMaster PROPERTIES PREFIX "" SUFFIX ".asi" OUTPUT_NAME "TraversalMaster")

if(MSVC)
    target_compile_options(TraversalMaster PRIVATE /guard:cf)
    target_link_options(TraversalMaster PRIVATE /DYNAMICBASE /HIGHENTROPYVA /NXCOMPAT /guard:cf)
endif()

add_custom_command(TARGET TraversalMaster POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/dist"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:TraversalMaster>" "${CMAKE_SOURCE_DIR}/dist/"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/TraversalMaster.ini" "${CMAKE_SOURCE_DIR}/dist/"
    COMMENT "Staging dist/"
    VERBATIM
)
