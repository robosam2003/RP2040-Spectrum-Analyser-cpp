cmake_minimum_required(VERSION 3.12)

# rest of your project
add_executable(spectrum
        main.c
        )

target_link_libraries(spectrum pico_pdm_microphone)

# enable usb output, disable uart output
pico_enable_stdio_usb(spectrum 1)
pico_enable_stdio_uart(spectrum 0)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(spectrum)