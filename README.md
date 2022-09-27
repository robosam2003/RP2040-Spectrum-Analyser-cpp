# RP2040-Spectrum-Analyser-cpp

This project uses 288 WS2812B (Neopixel) LEDs, a PDM microphone and a Raspberry Pi Pico, to create a spectrum analyser effect.

<img src = "https://github.com/robosam2003/RP2040-Spectrum-Analyser-cpp/blob/main/resources/spectrumAnalyser1.gif">

The Pico, microphone, mode and colour selection buttons, as well as a sensitivity knob are housed in a 3D printed case:
<img src = "https://github.com/robosam2003/RP2040-Spectrum-Analyser-cpp/blob/main/resources/IMG_20220924_215021.jpg" width = 400>

while the LEDS are stuck to the wall with double-sided tape /:)
<img src = "https://github.com/robosam2003/RP2040-Spectrum-Analyser-cpp/blob/main/resources/IMG_20220924_215027.jpg" width = 800>

#

Useful links for the pico:

[**RP2040 Datasheet**](https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf)

[**Pico Getting Started Guide**](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf)

## For more about the Fast Fourier Transform:
- [Reducible - The Fast Fourier Transform (FFT): Most Ingenious Algorithm Ever?](https://www.youtube.com/watch?v=h7apO7q16V0)
- [Steve Brunton - The Discrete Fourier Transform (DFT)](https://www.youtube.com/watch?v=nl9TZanwbBk)
- [Steve Brunton - The Fast Fourier Transform (FFT)](https://www.youtube.com/watch?v=E8HeD-MUrjY)
- [Steve Brunton - The Fast Fourier Transform Algorithm](https://www.youtube.com/watch?v=toj_IoCQE-4)

##

[**LEDs used**](https://www.amazon.co.uk/YUNBO-Individually-Addressable-NO-Waterproof-Flexible/dp/B07TB198W5/ref=sr_1_7?crid=37VDK3ZCKOUQC&dchild=1&keywords=addressable%2Bled%2Bstrip&qid=1631019805&sprefix=addressable%2Bled%2Caps%2C165&sr=8-7&th=1)

# 

After failing to create my own FFT algorithm due to memory constraints, I did what every good programmer does - Copied someone else. See [kissfft](https://github.com/mborgerding/kissfft)
