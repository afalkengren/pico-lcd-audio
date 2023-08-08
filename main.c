/*****************************************************************************
* | File        :   EPD_Test.h
* | Function    :   test Demo
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2021-03-16
* | Info        :   
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/multicore.h"
#include "pico/sync.h"

#include "LCD_1in14.h"

#include "testwav.h"
#include "dft.h"

#define LCD_HEIGHT 135
#define LCD_WIDTH 240

#define PIXEL_SIZE 2 // RGB565 = 16-bit color 
#define FRAME_SIZE (LCD_WIDTH * LCD_HEIGHT)
#define FRAME_RATE 30u

#define COLOR_WHITE 0xFFFF
#define COLOR_BLACK 0x0000
#define COLOR_RED 0xF800

#define Y_OFFSET (LCD_HEIGHT / 2)

#define PIXEL_AT(X, Y) (X + (Y * LCD_WIDTH))

int attenuation = 1;
mutex_t mutex_atten;

unsigned iteration;
unsigned n_samples;

unsigned fft_bins;
unsigned fft_length = 1024;

uint16_t* framebuf_cur;
uint16_t* framebuf_1;
uint16_t* framebuf_2;

uint8_t* framehint_cur;
uint8_t* framehint_1;
uint8_t* framehint_2;

bool core1_finished = true;

inline void set_attenuation(const int new_val) {
    mutex_enter_blocking(&mutex_atten);
    attenuation = new_val;
    mutex_exit(&mutex_atten);
}

inline int16_t* get_y_wav(unsigned x) {
    int16_t* data = (int16_t*)&test_wav[44];
    return &data[x];
}

inline void draw_y_for_x(unsigned x) {
    unsigned n = iteration + x;
    int16_t sample = (n < n_samples) ? *get_y_wav(n) : 0;

    // If signal clips the screen, then adjust gain higher... TODO should be a windowed AGC
    int32_t target_y = (sample / attenuation) + Y_OFFSET;
    if (target_y >= LCD_HEIGHT) {
        set_attenuation(sample / Y_OFFSET);
        //printf("%d > %d (sample=%d) -> attenuation = %u\n", target_y, LCD_HEIGHT, sample, attenuation);
        target_y = LCD_HEIGHT - 1;
    } else if (target_y < 0) {
        set_attenuation(sample / -Y_OFFSET);
        //printf("%d < %d (sample=%d) -> attenuation = %u\n", target_y, -LCD_HEIGHT, sample, attenuation);
        target_y = 0;
    }
    
    framebuf_cur[PIXEL_AT(x, framehint_cur[x])] = COLOR_WHITE; // Remove the old signal using framehint
    framebuf_cur[PIXEL_AT(x, target_y)] = COLOR_BLACK; // Draw the new signal
    framehint_cur[x] = target_y; // Update framehint with new y
}

inline void dft2(unsigned x) {
    unsigned n = iteration + x;
    int16_t sample = (n < n_samples) ? *get_y_wav(n) : 0;

    // If signal clips the screen, then adjust gain higher... TODO should be a windowed AGC
    int32_t target_y = (sample / attenuation) + Y_OFFSET;
    if (target_y >= LCD_HEIGHT) {
        set_attenuation(sample / Y_OFFSET);
        //printf("%d > %d (sample=%d) -> attenuation = %u\n", target_y, LCD_HEIGHT, sample, attenuation);
        target_y = LCD_HEIGHT - 1;
    } else if (target_y < 0) {
        set_attenuation(sample / -Y_OFFSET);
        //printf("%d < %d (sample=%d) -> attenuation = %u\n", target_y, -LCD_HEIGHT, sample, attenuation);
        target_y = 0;
    }
    
    framebuf_cur[PIXEL_AT(x, framehint_cur[x])] = COLOR_WHITE; // Remove the old signal using framehint
    framebuf_cur[PIXEL_AT(x, target_y)] = COLOR_BLACK; // Draw the new signal
    framehint_cur[x] = target_y; // Update framehint with new y
}

inline void draw_next_frame_core0(void) {
    for (unsigned x = 0; x < LCD_WIDTH/2; ++x) {
        draw_y_for_x(x);
    }
}

void draw_next_frame_core1(void) {
    core1_finished = false;
    for (unsigned x = LCD_WIDTH/2; x < LCD_WIDTH; ++x) {
        draw_y_for_x(x);
    }
    core1_finished = true;
}

uint16_t* draw_next_frame(void) {
    // Each core generates half the frame
    multicore_reset_core1();
    multicore_launch_core1((void*)draw_next_frame_core1);
    draw_next_frame_core0();

    // wait until other core is finished
    while(!core1_finished);

    // Swap frame buffer and frame hint
    if (framebuf_cur == framebuf_1) {
        framebuf_cur = framebuf_2;
        framehint_cur = framehint_2;
        return framebuf_1;
    } else {
        framebuf_cur = framebuf_1;
        framehint_cur = framehint_1;
        return framebuf_2;
    }
}

int main(void) {
    DEV_Module_Init();

    LCD_1IN14_Init(HORIZONTAL);

    mutex_init(&mutex_atten);

    // Create a frame buffer and fill with white
    framebuf_1 = malloc(FRAME_SIZE * PIXEL_SIZE);
    framebuf_2 = malloc(FRAME_SIZE * PIXEL_SIZE);
    for (unsigned i = 0; i < FRAME_SIZE; ++i) {
        framebuf_1[i] = COLOR_WHITE;
        framebuf_2[i] = COLOR_WHITE;
    }

    // Create hint of last waveform so we can clear it
    framehint_1 = calloc(LCD_WIDTH, sizeof(uint8_t));
    framehint_2 = calloc(LCD_WIDTH, sizeof(uint8_t));

    const uint32_t FRAME_TIME_MS = 1000u / FRAME_RATE;
    absolute_time_t t_target = get_absolute_time(); // delayed_by_ms(t_start, FRAME_TIME_MS);
    
    // frame rate counter
    absolute_time_t t_fr = get_absolute_time();
    uint16_t frame_count = 0;

    // uint16_t* block_size = (uint16_t*)&test_wav[32];
    uint32_t* data_size = (uint32_t*)&test_wav[40];
    n_samples = *data_size / 2; // block_size is 2 as it is 16-bit (trust me)
    unsigned end_step = n_samples - LCD_WIDTH; // each pixel will read 1 block

    fft_bins = 

    framebuf_cur = framebuf_1;
    framehint_cur = framehint_1;

    iteration = 0;
    while(true) {
        if (iteration >= end_step) {
            printf("RESTARTING... BAUDRATE = %u\n", spi_get_baudrate(spi1));
            set_attenuation(1);
            iteration = 0;
        }

        uint16_t* next_frame = draw_next_frame();
        
        // Deliver next frame at target frame time
        printf("Time till next frame: %llu us\n", absolute_time_diff_us(get_absolute_time(), t_target));
        sleep_until(t_target);
        LCD_1IN14_Display(next_frame);
        // printf("SHOWING SAMPLES %d->%d OUT OF %d\n", iteration, iteration + 480, n_samples);

        if (frame_count++ >= 64) {
            uint32_t frame_time_ms = absolute_time_diff_us(t_fr, get_absolute_time()) / (64*1000);
            int frame_rate = 1000/frame_time_ms;
            printf("FRAMERATE = %dHz\n", frame_rate);
            t_fr = get_absolute_time();
            frame_count = 0;
        }

        // update targets for next frame
        iteration += 480;
        t_target = delayed_by_ms(t_target, FRAME_TIME_MS);  
    }

    free(framebuf_1);
    free(framebuf_2);
    free(framehint_1);
    free(framehint_2);
    framebuf_1 = NULL;
    framebuf_2 = NULL;
    framehint_1 = NULL;
    framehint_2 = NULL;

    DEV_Module_Exit();
    
    return 0;
}

