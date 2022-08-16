cmake_minimum_required(VERSION 3.12)

# initialize pico_sdk from GIT
# (note this can come from environment, CMake cache etc)
# set(PICO_SDK_FETCH_FROM_GIT on)

# pico_sdk_import.cmake is a single file copied from this SDK
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(RP2040-Spectrum-Analyser-cpp C CXX ASM)

# initialize the Pico SDK
pico_sdk_init()

add_library(pico_pdm_microphone INTERFACE)
target_sources(pico_pdm_microphone INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/src/pdm_microphone.c
        ${CMAKE_CURRENT_LIST_DIR}/src/OpenPDM2PCM/OpenPDMFilter.cpp
)
target_include_directories(pico_pdm_microphone INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/src/include
        ${CMAKE_CURRENT_LIST_DIR}/programs
)
pico_generate_pio_header(pico_pdm_microphone ${CMAKE_CURRENT_LIST_DIR}/src/pdm_microphone.pio)
target_link_libraries(pico_pdm_microphone INTERFACE pico_stdlib hardware_dma hardware_pio hardware_adc)

add_library(pico_analog_microphone INTERFACE)
target_sources(pico_analog_microphone INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/src/analog_microphone.cpp
)
target_include_directories(pico_analog_microphone INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/src/include

)

target_link_libraries(pico_analog_microphone INTERFACE pico_stdlib hardware_adc hardware_dma)

add_subdirectory("examples/hello_analog_microphone")
add_subdirectory("examples/hello_pdm_microphone")
add_subdirectory("examples/usb_microphone")
add_subdirectory("programs")

