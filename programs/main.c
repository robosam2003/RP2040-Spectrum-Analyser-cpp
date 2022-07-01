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
#include "tusb.h"

#define nsamples 128
#define pi 3.14159265358979323846

/** my attempt at an fft - failed miserably due to memory constraints - using kiss_fft instead */
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
//}

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

int main( void )
{
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

    // kiss fft setup
    kiss_fft_scalar fft_in[nsamples];
    kiss_fft_cpx fft_out[nsamples];
    kiss_fftr_cfg cfg = kiss_fftr_alloc(nsamples, false, 0, 0);


    int counter = 0;
    while (1) {
        // wait for new samples
        while (samples_read == 0) { tight_loop_contents(); }

        // store and clear the samples read from the callback
        int sample_count = samples_read;
        samples_read = 0;

        double sum=0;
        for (int i=0;i<nsamples;i++) { sum+=sample_buffer[i]; }
        double avg = sum/nsamples;
        for (int i=0;i<nsamples;i++) { fft_in[i]= (float)(sample_buffer[i]-avg); }

        kiss_fftr(cfg, fft_in, fft_out);

        double freqMag[nsamples/2] = {};
        for (int i=0;i<nsamples/2;i++) {
            freqMag[i] = sqrt(pow(fft_out[i].r, 2) + pow(fft_out[i].r, 2));
            if (freqMag[i] != 0) { freqMag[i] = (freqMag[i]); } // log scale
        }


        for (int i=0;i<nsamples/4;i=i+2) {
            printf("%lf ", freqMag[i]);
        }
        printf("\n");
    }

    kiss_fft_free(cfg); // will never get here


    return 0;
}
