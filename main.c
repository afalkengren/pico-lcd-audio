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
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/multicore.h"
#include "pico/sync.h"
// #include "pico/divider.h"

#include "lcd1n14.h"
#include "mcp4922.h"
#include "wavaudio.h"

#include "testwav.h"
// #include "dft.h"

#define LCD_HEIGHT LCD_1IN14_WIDTH
#define LCD_WIDTH LCD_1IN14_HEIGHT

#define PIXEL_SIZE 2 // RGB565 = 16-bit color 
#define FRAME_SIZE (LCD_WIDTH * LCD_HEIGHT)

#define FRAME_RATE 30u
#define FRAME_TIME_US (1000000u / FRAME_RATE)

#define COLOR_WHITE 0xFFFFu
#define COLOR_BLACK 0x0000u
#define COLOR_RED 0xF800u

#define Y_OFFSET (LCD_HEIGHT / 2)

#define PIXEL_AT(X, Y) (X + (Y * LCD_WIDTH))

wav_audio_t* wav_file;
unsigned n_samples;

unsigned fft_bins;
unsigned fft_length = 1024;

absolute_time_t t_frame_target;
int attenuation;
// mutex_t mutex_atten;
uint16_t* framebuf;
uint8_t* framehint;
bool frame_is_ready;

// void set_attenuation(const int new_val) {
//     mutex_enter_blocking(&mutex_atten);
//     attenuation = new_val;
//     mutex_exit(&mutex_atten);
// }

void draw_y_for_x(unsigned x) {
    int16_t sample = wav_next(wav_file).left;

    // If signal clips the screen, then adjust gain higher... TODO should be a windowed AGC
    // use div_s32s32? div_s32s32_unsafe?
    int32_t target_y = (sample / attenuation) + Y_OFFSET;
    if (target_y >= LCD_HEIGHT) {
        attenuation = (sample / Y_OFFSET);
        printf("%d > %d (sample=%d) -> attenuation = %u\n", target_y, LCD_HEIGHT, sample, attenuation);
        target_y = LCD_HEIGHT - 1;
    } else if (target_y < 0) {
        attenuation = (sample / -Y_OFFSET);
        printf("%d < %d (sample=%d) -> attenuation = %u\n", target_y, -LCD_HEIGHT, sample, attenuation);
        target_y = 0;
    }
    printf("(%d, %d) (sample=%d)\n", x, target_y, sample);
    
    framebuf[PIXEL_AT(x, framehint[x])] = COLOR_WHITE; // Remove the old signal using framehint
    framebuf[PIXEL_AT(x, target_y)] = COLOR_RED; // Draw the new signal
    framehint[x] = target_y; // Update framehint with new y
}

// void dft2(unsigned x) {
//     unsigned n = iteration + x;
//     int16_t sample = (n < n_samples) ? *get_y_wav(n) : 0;

//     // If signal clips the screen, then adjust gain higher... TODO should be a windowed AGC
//     int32_t target_y = (sample / attenuation) + Y_OFFSET;
//     if (target_y >= LCD_HEIGHT) {
//         set_attenuation(sample / Y_OFFSET);
//         //printf("%d > %d (sample=%d) -> attenuation = %u\n", target_y, LCD_HEIGHT, sample, attenuation);
//         target_y = LCD_HEIGHT - 1;
//     } else if (target_y < 0) {
//         set_attenuation(sample / -Y_OFFSET);
//         //printf("%d < %d (sample=%d) -> attenuation = %u\n", target_y, -LCD_HEIGHT, sample, attenuation);
//         target_y = 0;
//     }
    
//     framebuf_cur[PIXEL_AT(x, framehint_cur[x])] = COLOR_WHITE; // Remove the old signal using framehint
//     framebuf_cur[PIXEL_AT(x, target_y)] = COLOR_BLACK; // Draw the new signal
//     framehint_cur[x] = target_y; // Update framehint with new y
// }

// void draw_next_frame_core0(void) {
//     const uint8_t* pos = wav_get_pos(wav_file);
//     for (unsigned x = 0; x < LCD_WIDTH; ++x) {
//         draw_y_for_x(x);
//     }
//     wav_set_pos_offset(wav_file, pos, 1);
// }

void draw_next_frame_core1(void) {
    for (unsigned x = LCD_WIDTH; x < LCD_WIDTH; ++x) {
        draw_y_for_x(x);
    }
    frame_is_ready = true;
}

// typedef struct graphics_context_t {
//     absolute_time_t t_next;
//     int attenuation;
//     mutex_t mutex_atten;
    
//     // uint16_t* framebuf_cur;
//     // uint16_t* framebuf_1;
//     // uint16_t* framebuf_2;
//     // uint8_t* framehint_cur;
//     // uint8_t* framehint_1;
//     // uint8_t* framehint_2;
//     uint16_t* framebuf;
//     uint8_t* framehint;
//     bool frame_is_ready;
// } graphics_context_t;

// graphics_context_t graphics_context_new() {
//     graphics_context_t ctx; 
//     ctx.t_next = get_absolute_time();
//     ctx.attenuation = 1;
//     ctx.framebuf = malloc(FRAME_SIZE * PIXEL_SIZE);
//     ctx.framehint = calloc(LCD_WIDTH, sizeof(uint8_t));
//     ctx.frame_is_ready = false;

//     // Create a frame buffer and fill with white
//     for (unsigned i = 0; i < FRAME_SIZE; ++i) {
//         ctx.framebuf[i] = COLOR_WHITE;
//     }
//     mutex_init(&ctx.mutex_atten);
//     return ctx;
// }

void graphics_init() {
    lcd_init();
    lcd_set_scan_direction(LCD_SCAN_DIR_HORIZONTAL);
    lcd_set_draw_area(0, LCD_WIDTH, 0, LCD_HEIGHT);

    t_frame_target = get_absolute_time();
    attenuation = 1;
    framebuf = malloc(FRAME_SIZE * PIXEL_SIZE);
    framehint = calloc(LCD_WIDTH, sizeof(uint8_t));
    frame_is_ready = false;

    // Create a frame buffer and fill with white
    for (unsigned i = 0; i < FRAME_SIZE; ++i) {
        framebuf[i] = COLOR_WHITE;
    }
    // mutex_init(&mutex_atten);
}

// // returns either framebuf 1 or 2
// void draw_next_frame(void) {
//     // Each core generates half the frame
//     multicore_reset_core1();
//     multicore_launch_core1((void*)draw_next_frame_core1);
//     // draw_next_frame_core0();

//     // wait until other core is finished
//     //while(!core1_finished);

//     // Swap frame buffer and frame hint
//     // if (framebuf_cur == framebuf_1) {
//     //     framebuf_cur = framebuf_2;
//     //     framehint_cur = framehint_2;
//     // } else {
//     //     framebuf_cur = framebuf_1;
//     //     framehint_cur = framehint_1;
//     // }
// }

bool has_reached_time(absolute_time_t target) {
    absolute_time_t t_now = get_absolute_time();
    absolute_time_t t_min = absolute_time_min(target, t_now);
    return (to_us_since_boot(t_min) == to_us_since_boot(target));
}

void graphics_draw_if_valid() {
    // check if we need to deliver frame yet and frame is ready
    if (!has_reached_time(t_frame_target) || !frame_is_ready) return;
    
    frame_is_ready = false;       

    // update targets for next frame
    printf("Time till next frame: %llu us\n", absolute_time_diff_us(get_absolute_time(), t_frame_target));
    t_frame_target = delayed_by_us(t_frame_target, FRAME_TIME_US);

    // Deliver next frame at target frame time
    lcd_draw_framebuf(framebuf);

    multicore_reset_core1();
    multicore_launch_core1((void*)draw_next_frame_core1);
}

void graphics_cleanup() {
    // free(framebuf_1);
    // free(framebuf_2);
    // free(framehint_1);
    // free(framehint_2);
    // framebuf_1 = NULL;
    // framebuf_2 = NULL;
    // framehint_1 = NULL;
    // framehint_2 = NULL;
}

// Frame rate stuff
typedef struct framerate_t {
    absolute_time_t time;
    uint16_t count;
} framerate_t;

framerate_t framerate_new() {
    framerate_t fr;
    fr.time = get_absolute_time();
    fr.count = 0;
    return fr;
}

void framerate_print_ifvalid(framerate_t* fr) {
    if (fr->count++ < 64) return;
    uint32_t frame_time_ms = absolute_time_diff_us(fr->time, get_absolute_time()) / (64*1000);
    int frame_rate = 1000/frame_time_ms;
    printf("FRAMERATE = %dHz\n", frame_rate);
    fr->time = get_absolute_time();
    fr->count = 0;
}

void play_wav(uint8_t* wav) {
    wav_file = wav_load((uint8_t*)test_wav);
    
    // We need the sample period to work out how much to sleep
    uint32_t wav_period_us = wav_get_sample_period_us(wav_file); 
    absolute_time_t t_audio_target = get_absolute_time();

    while(!wav_is_eof(wav_file)) {
        // We get a signed sample, but DAC requires a positive input, so bias to half of UINT16_MAX
        uint16_t sample = wav_next(wav_file).left + (INT16_MAX + 1);
        mcp4922_mono_write(&sample);

        // Calculate the next time we should resume this loop.
        t_audio_target = delayed_by_us(t_audio_target, wav_period_us);
        // printf("Time to next sample: %llu us\n", absolute_time_diff_us(get_absolute_time(), t_audio_target));
        
        // sleep_until attempts to use low power sleep. We don't really need that.
        busy_wait_until(t_audio_target);
    }
    free(wav_file);
}

int main(void) {
    stdio_init_all();

    graphics_init();
    mcp4922_init();
    
    lcd_clear(COLOR_WHITE);

    // frame rate counter
    // framerate_t framerate = framerate_new();
    
    multicore_reset_core1();
    multicore_launch_core1((void*)play_wav);    

    //graphics_cleanup();
    
    return 0;
}

