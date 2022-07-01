# RP2040-Spectrum-Analyser-cpp

[**RP2040 Datasheet**](https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf)

[**Pico Getting Started Guide**](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf)

## Links for the fast fourier transform:
- [Reducible - The Fast Fourier Transform (FFT): Most Ingenious Algorithm Ever?](https://www.youtube.com/watch?v=h7apO7q16V0)
- [Steve Brunton - The Discrete Fourier Transform (DFT)](https://www.youtube.com/watch?v=nl9TZanwbBk)
- [Steve Brunton - The Fast Fourier Transform (FFT)](https://www.youtube.com/watch?v=E8HeD-MUrjY)
- [Steve Brunton - The Fast Fourier Transform Algorithm](https://www.youtube.com/watch?v=toj_IoCQE-4)

##

[**LEDs used**](https://www.amazon.co.uk/YUNBO-Individually-Addressable-NO-Waterproof-Flexible/dp/B07TB198W5/ref=sr_1_7?crid=37VDK3ZCKOUQC&dchild=1&keywords=addressable%2Bled%2Bstrip&qid=1631019805&sprefix=addressable%2Bled%2Caps%2C165&sr=8-7&th=1)


# 

- Using PIO to generate a 2MHz clock signal
- Using PIO to get the PDM data at the correct rate and pass to FIFO

inspiration taken from [AlexFWulff](https://github.com/AlexFWulff/awulff-pico-playground/tree/main/adc_fft)