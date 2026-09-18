cmake_minimum_required(VERSION 3.22.1)
project("polyrhythm_native")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SHARED_CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../shared_core)

# Shared Core C++20 kaynakları
set(SHARED_CORE_SOURCES
    ${SHARED_CORE_DIR}/src/bjorklund.cpp
    ${SHARED_CORE_DIR}/src/sound_generator.cpp
    ${SHARED_CORE_DIR}/src/trainers.cpp
    ${SHARED_CORE_DIR}/src/rhythm_layer.cpp
    ${SHARED_CORE_DIR}/src/audio_scheduler.cpp
    ${SHARED_CORE_DIR}/src/polyrhythm_engine.cpp
    ${SHARED_CORE_DIR}/src/c_bridge.cpp
)

add_library(polyrhythm_native SHARED
    native-lib.cpp
    OboeAudioDriver.cpp
    ${SHARED_CORE_SOURCES}
)

target_include_directories(polyrhythm_native PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SHARED_CORE_DIR}/include
)

# Android platform kütüphaneleri
find_library(log-lib log)
find_library(aaudio-lib aaudio)
find_library(OpenSLES-lib OpenSLES)

target_link_libraries(polyrhythm_native
    ${log-lib}
    ${aaudio-lib}
    ${OpenSLES-lib}
)
