cmake_minimum_required(VERSION 3.12)

# rest of your project
add_executable(spectrum
        main.c
        )

#pico_generate_pio_header(spectrum ${CMAKE_CURRENT_LIST_DIR}/src/pdm_microphone.pio})

target_include_directories(spectrum PUBLIC ${CMAKE_CURRENT_LIST_DIR})

add_library(kiss_fftr kiss_fftr.c)
add_library(kiss_fft kiss_fft.c)

target_link_libraries(kiss_fftr kiss_fft)

target_link_libraries(spectrum pico_pdm_microphone
        kiss_fftr)

# enable usb output, disable uart output
pico_enable_stdio_usb(spectrum 1)
pico_enable_stdio_uart(spectrum 0)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(spectrum)