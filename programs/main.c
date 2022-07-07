/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This examples captures data from a PDM microphone using a sample
 * rate of 8 kHz and prints the sample values over the USB serial
 * connection.
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
#include "tusb.h"

#define LED_STRIP_PIN 16
#define NUM_LEDS 144
#define LEDS_PER_STRIP 14 // Will be 28 with the full thing // just<20cm
#define NUM_STRIPS 10 // for now...
#define nsamples 256
#define pi 3.14159265358979323846
uint divisions[10] = {141, 206, 316, 445, 703, 1078, 1570, 2320, 3281, 3840};
#define UPPER_THRESHOLD 100000
#define AVG_PERIOD 156;

#define RED urgb_u32(255, 0, 0)
#define YELLOW urgb_u32(255, 255, 0)
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

void set_strips_level(uint levels[], uint32_t colour) {// levels range from 0 to 28.
    for (int i=0;i<NUM_STRIPS;i++) {
        for (int j=0;j<levels[i];j++) {
            put_pixel(colour);
        }
        for (int j=0;j<(LEDS_PER_STRIP-levels[i]);j++) {
            put_pixel(0); // black
        }
    }
    //sleep_us(500);
}

int map(int val, int in_min, int in_max, int out_min, int out_max) {
    return (int)((val - in_min) * (out_max - out_min) / (in_max - in_min) + out_min);
}

int main() {
    stdio_init_all();
    /// PDM MIC SETUP
    // initialize stdio and wait for USB CDC connect
    stdio_init_all();
    while (!tud_cdc_connected()) {
        tight_loop_contents();
    }
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




    /// LED STRIP SETUP
    uint offset2= pio_add_program(pio1, &ws2812_program);
    uint sm2 = pio_claim_unused_sm(pio1, true);
    ws2812_program_init(pio1, sm2, offset2, LED_STRIP_PIN, 800000, false);

    clear_strip();
    sleep_ms(10);


    uint levels[NUM_STRIPS] = {}; // REMOVE THE *2 when you have 10 strips

    while (1) {
        // wait for new samples
        while (samples_read == 0) { tight_loop_contents(); }
        // store and clear the samples read from the callback
        int sample_count = samples_read;
        samples_read = 0;

        double sum = 0;
        for (int i = 0; i < nsamples; i++) { sum += sample_buffer[i]; }
        double avg = sum / nsamples;
        for (int i = 0; i < nsamples; i++) { fft_in[i] = (float) (sample_buffer[i]); }/// remove base frequency

        kiss_fftr(cfg, fft_in, fft_out);

        unsigned long long freqMag[nsamples / 2] = {};
        for (int i = 0; i < nsamples / 4; i++) {
            freqMag[i] = sqrt(pow(fft_out[i].r, 2) + pow(fft_out[i].i, 2));
            if (freqMag[i] != 0) { freqMag[i] = (freqMag[i]); } // add log scale here if needed
        }
        for (int i=0;i<3;i++) { // attenuate the first 3 signals (up to 60HZ)
            freqMag[i] = 0;
        }
        int counter = 0;
        for (int i=0;i<10;){
            for (int j=0;j<nsamples/2;j++) {
                if (j*31 < divisions[i]) {
                    levels[i] += freqMag[j];
                    counter++;
                }
                else {
                    levels[i] /= counter;
                    ++i;
                    counter = 0;
                    levels[i] += freqMag[j];
                    counter++;
                }
            }
        }

        for (int i=0;i<NUM_STRIPS;i++) {
            printf("%d ", levels[i]);
        }

        printf("\n");
//        uint avMag = 0;
//        for (int i=0;i<40;i++) {
//            avMag += freqMag[i];
//        }
//        avMag/=40;

        for(int i=0;i<NUM_STRIPS;i++) {
            double div = UPPER_THRESHOLD/LEDS_PER_STRIP;
            levels[i] /= div;
            (levels[i] > LEDS_PER_STRIP) ? levels[i]=LEDS_PER_STRIP : 0;
            (levels[i] < 0) ? levels[i] = 0 : 0; // should never occur
        }
        set_strips_level(levels, YELLOW);

        sleep_us(400);
        for (int i=0;i<NUM_STRIPS;i++) {
            levels[i] = 0;
        }

    }

    kiss_fft_free(cfg); // will never get here

}

