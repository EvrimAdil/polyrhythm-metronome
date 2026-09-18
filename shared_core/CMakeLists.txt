cmake_minimum_required(VERSION 3.20)
project(polyrhythm_core VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Shared Core Library (C++20)
add_library(polyrhythm_core STATIC
    src/bjorklund.cpp
    src/sound_generator.cpp
    src/trainers.cpp
    src/rhythm_layer.cpp
    src/audio_scheduler.cpp
    src/polyrhythm_engine.cpp
    src/c_bridge.cpp
)

target_include_directories(polyrhythm_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Compiler warnings and optimizations
if(MSVC)
    target_compile_options(polyrhythm_core PRIVATE /W4 /O2)
else()
    target_compile_options(polyrhythm_core PRIVATE -Wall -Wextra -Wpedantic -O3 -ffast-math)
endif()

# Unit Tests (if building standalone)
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    enable_testing()
    
    add_executable(test_scheduler_drift
        tests/test_scheduler_drift.cpp
    )
    target_link_libraries(test_scheduler_drift PRIVATE polyrhythm_core)
    add_test(NAME SchedulerDrift COMMAND test_scheduler_drift)

    add_executable(test_bjorklund
        tests/test_bjorklund.cpp
    )
    target_link_libraries(test_bjorklund PRIVATE polyrhythm_core)
    add_test(NAME Bjorklund COMMAND test_bjorklund)

    add_executable(test_polyrhythm
        tests/test_polyrhythm.cpp
    )
    target_link_libraries(test_polyrhythm PRIVATE polyrhythm_core)
    add_test(NAME Polyrhythm COMMAND test_polyrhythm)
endif()
