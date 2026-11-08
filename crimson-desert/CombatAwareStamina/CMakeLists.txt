cmake_minimum_required(VERSION 3.21)
project(CombatAwareStamina LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Crimson Desert is x64 only - configure a 64-bit build.")
endif()

include(FetchContent)
set(SAFETYHOOK_FETCH_ZYDIS ON CACHE BOOL "" FORCE)
FetchContent_Declare(safetyhook
    GIT_REPOSITORY https://github.com/cursey/safetyhook.git
    GIT_TAG        v0.6.9)
FetchContent_MakeAvailable(safetyhook)

add_library(CombatAwareStamina SHARED
    src/dllmain.cpp
    src/config.cpp
    src/combat.cpp
    src/scanner.cpp
    src/log.cpp)

target_link_libraries(CombatAwareStamina PRIVATE safetyhook::safetyhook)
target_compile_definitions(CombatAwareStamina PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# Ultimate ASI Loader picks up *.asi files.
set_target_properties(CombatAwareStamina PROPERTIES SUFFIX ".asi" PREFIX "")

if(MSVC)
    target_compile_options(CombatAwareStamina PRIVATE /W4 /permissive-)
    set_property(TARGET CombatAwareStamina PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
    target_link_options(CombatAwareStamina PRIVATE -static -static-libgcc -static-libstdc++)
endif()
