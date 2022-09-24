/*
 *
 *
*/


#include <stdio.h>
#include <string.h>
#include <complex.h>
#include "math.h"
#include "stdlib.h"
#include "kiss_fftr.h"

#include "pico/stdlib.h"
#include "pico/pdm_microphone.h"
#include "pdm_microphone.pio.h"
#include "hardware/adc.h"
#include "hardware/irq.h"
#include "tusb.h"
#include "pico/multicore.h"


#define LED_STRIP_PIN 16
#define NUM_LEDS 288
#define LEDS_PER_STRIP 24 // Will be 28 with the full thing // just<20cm
#define NUM_STRIPS 12 // for now...
#define nsamples 256
#define pi 3.14159265358979323846

#define COLOUR_BUTTON_PIN 6 // GP6
#define MODE_BUTTON_PIN 13 // GP13

int selectedColour = 0;
int selectedMode = 0; // starts on sunrise

double divisions[NUM_STRIPS] = {121,
                              147,
                              178,
                              215,
                              261,
                              316,
                              383,
                              464,
                              562,
                              699,
                              825,
                              1000}; // 100-1000Hz, recalculated later

#define LOWER_FREQ_LIMIT 100
#define UPPER_FREQ_LIMIT 2000




#define MAX_THRESHOLD 1000000
//uint thresholds[10] = {100000, 500000, 500000, 500000, 500000, 500000, 500000, 500000, 500000, 500000};
uint multipliers[12] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};


uint MALevels[12] = {};
#define MAduration 4

#define WHITE            urgb_u32(0XFF, 0xFF, 0xFF)
#define BLACK            urgb_u32(0x00, 0x00, 0x00)

#define RED              urgb_u32(0xFF, 0x00, 0x00)
#define VERMILION        urgb_u32(0xFF, 0x40, 0x00)
#define ORANGE           urgb_u32(0xFF, 0x80, 0x00)
#define YELLOW           urgb_u32(0xFF, 0xFF, 0x00)
#define YELLOWGREEN      urgb_u32(0xBF, 0xFF, 0x00)
#define CHARTREUSE       urgb_u32(0x80, 0xFF, 0x00)
#define LEAFGREEN        urgb_u32(0x40, 0xFF, 0x00)
#define GREEN            urgb_u32(0x00, 0xFF, 0x00)
#define COBALTGREEN      urgb_u32(0x00, 0xFF, 0x40)
#define EMERALD          urgb_u32(0x00, 0xFF, 0x80)
#define TURQUOISEGREEN   urgb_u32(0x00, 0xFF, 0xBF)
#define TURQUOISEBLUE    urgb_u32(0x00, 0xFF, 0xFF)
#define CERULEAN         urgb_u32(0x00, 0xBF, 0xFF)
#define AZURE            urgb_u32(0x00, 0x80, 0xFF)
#define COBALTBLUE       urgb_u32(0x00, 0x40, 0xFF)
#define BLUE             urgb_u32(0x00, 0x00, 0xFF)
#define HYACINTH         urgb_u32(0x40, 0x00, 0xFF)
#define VIOLET           urgb_u32(0x80, 0x00, 0xFF)
#define PURPLE           urgb_u32(0xBF, 0x00, 0xFF)
#define MAGENTA          urgb_u32(0xFF, 0x00, 0xFF)
#define REDPURPLE        urgb_u32(0xFF, 0x00, 0xBF)
#define CRIMSON          urgb_u32(0xFF, 0x00, 0x80)
#define CARMINE          urgb_u32(0xFF, 0x00, 0x40)






/* my attempt at an fft - failed miserably due to memory constraints - using kiss_fft instead
//complex double * fft(complex double P[], const int n) {
//    // n should be a power of two
//    if (n==1) {
//        return P;
//    }
//    complex double omega = exp(2*pi*I/n);
//    complex double Pe[n/2];
//    complex double Po[n/2];
//    for (int i=0;i<(n/2);i++) {
//        Pe[i] = P[i*2]; // even
//        Po[i] = P[i*2+1]; // odd
//    }
//    complex double* Ye = fft(Pe, n/2);
//    complex double* Yo = fft(Po, n/2);
//    complex double Y[n]; for (int i=0;i<n;i++) {Y[i] = 0;};
//    for (int j=0;j<n/2;j++) {
//        Y[j] = Ye[j] + Yo[j]* cpow(omega, j);
//        Y[j+n/2] = Ye[j] - Yo[j] * cpow(omega, j);
//    }
//    return Y;
//} */

// configuration
const struct pdm_microphone_config config = {
        // GPIO pin for the PDM DAT signal
        .gpio_data = 15,
        // GPIO pin for the PDM CLK signal
        .gpio_clk = 4,
        // PIO instance to use??
        .pio = pio0,
        // PIO State Machine instance to use
        .pio_sm = 0,
        // sample rate in Hz
        .sample_rate = 8000,
        // number of samples to buffer
        .sample_buffer_size = nsamples,
};

// variables
int16_t sample_buffer[nsamples];
volatile int samples_read = 0;

void on_pdm_samples_ready()
{
    // callback from library when all the samples in the library
    // internal sample buffer are ready for reading
    samples_read = pdm_microphone_read(sample_buffer, nsamples);
}

double magnitude(complex double a) {
    return sqrt( cpow(creal(a), 2) + cpow(cimag(a), 2) );
}


void pattern_random(uint len, uint t) {
    if (t % 8)
        return;
    for (int i = 0; i < len; ++i)
        put_pixel(rand());
}

void clear_strip(){
    for (int i=0;i<NUM_LEDS;i++) {
        put_pixel(urgb_u32(0, 0, 0));
    }
}



int map(int val, int in_min, int in_max, int out_min, int out_max) {
    return (int)((val - in_min) * (out_max - out_min) / (in_max - in_min) + out_min);
}


enum modes {
    SUNRISE = 0,
    SPECTRUM_ANALYSER = 1,
    CONSTANT = 2
};

void calcDivisions() {
    for (int i=1;i<=NUM_STRIPS;) {

        double low = log10(LOWER_FREQ_LIMIT);
        double div = pow(10, (low + (i * (log10(UPPER_FREQ_LIMIT) - low)) / NUM_STRIPS    )    );
        divisions[i-1] = div;
        i++;
    }
}


unsigned long time;
const int debounce_delay = 500000; // us



void colour_button_callback() {
    selectedColour++;
    selectedColour = selectedColour % 24;
}

void mode_button_callback() {
    selectedMode++;
    selectedMode = selectedMode % 3;
}

void gpio_callback(uint gpio, uint32_t events) {
    uint64_t now = time_us_64();
    if ((now - time) > debounce_delay) {
        time = time_us_64();

        if (gpio == COLOUR_BUTTON_PIN) {
            colour_button_callback();
        } else if (gpio == MODE_BUTTON_PIN) {
            mode_button_callback();
        }
    }
}

/*void core1_entry() {
    gpio_set_irq_enabled_with_callback(MODE_BUTTON_PIN, GPIO_IRQ_EDGE_RISE, true, &mode_button_callback);
    while(1) {
        tight_loop_contents();
    }

}*/

void set_strips_level(uint levels[], uint32_t colour) {// levels range from 0 to 28.

    for (int i=0;i<NUM_STRIPS;i++) {
        if (i == 0) {
            if (levels[0] == LEDS_PER_STRIP) {
                levels[0] -= 1; // hardware issue.
            }
            for (int j = 0; j < levels[i]; j++) {
                put_pixel(colour);
            }
            for (int j = 0; j < (LEDS_PER_STRIP - 1 - levels[i]); j++) {
                put_pixel(0); // black
            }
        }
        else if (i%2 == 0) {
            for (int j = 0; j < levels[i]; j++) {
                put_pixel(colour);
            }
            for (int j = 0; j < (LEDS_PER_STRIP - levels[i]); j++) {
                put_pixel(0); // black
            }
        }
        else {
            for (int j = 0; j < (LEDS_PER_STRIP - levels[i]); j++) {
                put_pixel(0); // black
            }
            for (int j = 0; j < levels[i]; j++) {
                put_pixel(colour);
            }
        }
    }
    //sleep_us(500);
}


void set_strips_levels_colours(int stripColours[NUM_STRIPS][LEDS_PER_STRIP]) {
    for (int i=0; i<NUM_STRIPS; i++) {
        if (i==0) {
            for (int j=0; j<LEDS_PER_STRIP-1; j++) { // -1 because the first strip has one less (sigh)
                put_pixel(stripColours[i][j]);
            }
        }

        else if (i%2 == 0) {
            for (int j=0; j<LEDS_PER_STRIP; j++) {
                put_pixel(stripColours[i][j]);
            }
        }

        else {
            for (int j=0; j<LEDS_PER_STRIP; j++) {
                put_pixel(stripColours[i][LEDS_PER_STRIP-j-1]);
            }
        }

    }
}

uint32_t change_brightness(uint32_t colour, uint32_t percentage) {
    uint8_t r, g, b;
    b = colour & 0xFF;
    r = (colour >> 8) & 0xFF;
    g = (colour >> 16) & 0xFF;
    b *= percentage / 100;
    r *= percentage / 100;
    g *= percentage / 100;
    uint32_t ret = ( ((uint32_t) (r) << 8) | ((uint32_t) (g) << 16) | (uint32_t) b );
    return ret;
}

int main() {
    time = time_us_64();
    sleep_ms(5000);
    int hueColors[24] = {RED,
                         VERMILION,
                         ORANGE,
                         YELLOW,
                         YELLOWGREEN,
                         CHARTREUSE,
                         LEAFGREEN,
                         GREEN,
                         COBALTGREEN,
                         EMERALD,
                         TURQUOISEGREEN,
                         TURQUOISEBLUE,
                         CERULEAN,
                         AZURE,
                         COBALTBLUE,
                         BLUE,
                         HYACINTH,
                         VIOLET,
                         PURPLE,
                         MAGENTA,
                         REDPURPLE,
                         CRIMSON,
                         CARMINE,
                         WHITE};

    int sunriseColours[6] = {urgb_u32(0xFF, 0x00, 0x00),
                             urgb_u32(0xFF, 0x4D, 0x00), // bottom
                             urgb_u32(0xFF, 0x67, 0x00),
                             urgb_u32(0xFF, 0x81, 0x00),
                             urgb_u32(0xFF, 0xa7, 0x00),
                             urgb_u32(0xFF, 0xE7, 0x00)}; // top

    stdio_init_all();
    // initialize stdio and wait for USB CDC connect
    stdio_init_all();
/*    while (!tud_cdc_connected()) {
        tight_loop_contents();
    }*/

    // ADC setup
    adc_init();
    adc_gpio_init(26);
    adc_select_input(0);


    //multicore_launch_core1(core1_entry);


    // INTERRUPTS
    gpio_set_irq_enabled_with_callback(COLOUR_BUTTON_PIN, GPIO_IRQ_EDGE_RISE, true, &gpio_callback);
    gpio_set_irq_enabled(MODE_BUTTON_PIN, GPIO_IRQ_EDGE_RISE, true);





    /// LED STRIP SETUP
    uint offset2 = pio_add_program(pio1, &ws2812_program);
    uint sm2 = pio_claim_unused_sm(pio1, true);
    ws2812_program_init(pio1, sm2, offset2, LED_STRIP_PIN, 800000, false);

    clear_strip();
    sleep_ms(10);



    uint levels[NUM_STRIPS] = {};
    unsigned long long loopCounter = 1;
    while (1) {
        switch (selectedMode) {
            case SUNRISE: {
                bool done = false;
                int level = 0;
                int stripsColours[NUM_STRIPS][LEDS_PER_STRIP] = {0};
                int delay_ms = 100;
                if (!done) {
                    for (int i = 0; i < 6; i++) { // colour
                        for (int k = 0; k < 4; k++) {
                            for (int j = 0; j < NUM_STRIPS; j++) {
                                //uint32_t altered = change_brightness(sunriseColours[i], 50);
                                stripsColours[j][level] = sunriseColours[i] ;
                            }
                            set_strips_levels_colours(stripsColours);
                            sleep_ms(delay_ms);

                            level++;
                        }

                    }
//                for (int i=0; i<NUM_STRIPS;i++){
//                    levels[i] = LEDS_PER_STRIP;
//                }
//                set_strips_level(levels, sunriseColours[1]);
                    loopCounter++;
                    sleep_ms(100);
                    done = true;
                }
                while(1){
                    if (selectedMode != SUNRISE) {
                        break;
                    }
                    sleep_ms(100);
                }
                break;
            }

            case CONSTANT: {
                for (int i=0; i<NUM_STRIPS;i++){
                    levels[i] = LEDS_PER_STRIP;
                }
                set_strips_level(levels, hueColors[selectedColour]);
                loopCounter++;
                sleep_ms(100);
                break;
            }

            case SPECTRUM_ANALYSER: {
                // ****** Setup *******
                calcDivisions();
                // initialize the PDM microphone
                if (pdm_microphone_init(&config) < 0) {
                    printf("PDM microphone initialization failed!\n");
                    while (1) { tight_loop_contents(); }
                }
                // set callback that is called when all the samples in the library
                // internal sample buffer are ready for reading
                pdm_microphone_set_samples_ready_handler(on_pdm_samples_ready);
                // start capturing data from the PDM microphone
                if (pdm_microphone_start() < 0) {
                    printf("PDM microphone start failed!\n");
                    while (1) { tight_loop_contents(); }
                }

                /// kiss fft setup
                kiss_fft_scalar fft_in[nsamples];
                kiss_fft_cpx fft_out[nsamples];
                kiss_fftr_cfg cfg = kiss_fftr_alloc(nsamples, false, 0, 0);

                // ******  Loop *******
                while (1) {
                    // wait for new samples
                    while (samples_read == 0) { tight_loop_contents(); }
                    // store and clear the samples read from the callback
                    int sample_count = samples_read;
                    samples_read = 0;

                    double sum = 0;
                    for (int i = 0; i < nsamples; i++) { sum += sample_buffer[i]; }
                    double avg = sum / nsamples;
                    for (int i = 0;
                         i < nsamples; i++) { fft_in[i] = (float) (sample_buffer[i]); }/// remove base frequency

                    kiss_fftr(cfg, fft_in, fft_out);

                    double freqMag[nsamples / 2] = {};
                    for (int i = 0; i < nsamples / 4; i++) {
                        freqMag[i] = sqrt(pow(fft_out[i].r, 2) + pow(fft_out[i].i, 2));
                        if (freqMag[i] !=
                            0) { freqMag[i] = (freqMag[i]); } // add log scale here if needed /// log does not work as expected
                    }
                    for (int i = 0; i < 3; i++) { // attenuate the first 3 signals (up to 60HZ)
                        freqMag[i] = 0;
                    }

                    // split frequencies into 12 divisions
                    //int counter = 0;
                    for (int i = 0; i < NUM_STRIPS;) {
                        for (int j = 0; j < nsamples / 2; j++) {
                            if (j * 31 < divisions[i]) {
                                levels[i] += freqMag[j];
                                //counter++;
                            } else {
                                //levels[i] /= counter;
                                ++i;
                                //counter = 0;
                                levels[i] += freqMag[j];
                                //counter++;
                            }
                        }
                    }
                    // ADC pot read
                    uint threshold = adc_read();
                    threshold *= MAX_THRESHOLD / 0xFFF; // ADC reads from 0 to 0xFFF(4095)


                    // find moving averages of levels and store them in MALevels
                    if (loopCounter < MAduration) {
                        for (int i = 0; i < NUM_STRIPS; i++) {
                            MALevels[i] = (MALevels[i]*(loopCounter-1) + levels[i]) / (loopCounter);
                        }
                    } else {
                        for (int i = 0; i < NUM_STRIPS; i++) {
                            MALevels[i] = (MALevels[i]*(MAduration-1) + levels[i]) / (MAduration);
                        }
                    }


                    // print the values to the serial port
                    for (int i = 0; i < NUM_STRIPS; i++) {
                        levels[i] *= multipliers[i]; // multiply by their respective levels
                        printf("%u ", MALevels[i]);
                    }
                    printf("%u", threshold);

                    printf("\n");

                    uint stripLevels[NUM_STRIPS] = {};
                    for (int i = 0; i < NUM_STRIPS; i++) {
                        stripLevels[i] = MALevels[i];
                    }
                    for (int i = 0; i < NUM_STRIPS; i++) {
                        double div = threshold / LEDS_PER_STRIP;
                        stripLevels[i] /= div;
                        (stripLevels[i] > LEDS_PER_STRIP) ? stripLevels[i] = LEDS_PER_STRIP : 0;
                        (stripLevels[i] < 0) ? stripLevels[i] = 0 : 0; // should never occur
                    }
                    set_strips_level(stripLevels, hueColors[selectedColour]);

                    sleep_us(200);
                    for (int i = 0; i < NUM_STRIPS; i++) {
                        stripLevels[i] = 0;
                        levels[i] = 0;
                    }
                    loopCounter++;
                    if (selectedMode != SPECTRUM_ANALYSER) {
                        kiss_fft_free(cfg); // will never get here
                        break;
                    }
                }
                break;
            }
//            case WHITE_FADE_UP: {
//                // needs refining.
//                int white_fade_up_brightness = 0;
//                int white_fade_up_level = 1;
//                int counter = 0;
//                while (1) {
//                    for (int i=0;i<white_fade_up_level;i++) {
//                        put_pixel(urgb_u32(white_fade_up_brightness, white_fade_up_brightness, white_fade_up_brightness));
//                    }
//                    printf("%u\n", white_fade_up_level);
//                    if (white_fade_up_level < NUM_LEDS) white_fade_up_level++;
//                    if (white_fade_up_brightness <= 100 & counter % 5 == 0) white_fade_up_brightness++;
//                    sleep_ms(5);
//                    counter++;
//                }
//                break;
//            }
            default:
                selectedMode = 0;
                break;
        }




        // Cleanup, will never get here.
    }
}
