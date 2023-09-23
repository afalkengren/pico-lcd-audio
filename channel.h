#ifndef __CHANNEL_H
#define __CHANNEL_H

typedef struct channel_t channel_t;

typedef enum channel_error_t {
    CHANNEL_ERROR_SUCCESS = 0,
    CHANNEL_ERROR_EOF = 1,
    CHANNEL_ERROR_INVALID = 2,
} channel_error_t;

channel_t* channel_new(uint8_t* ptr, uint8_t pin);
uint8_t channel_get_pin(channel_t* ch);
bool channel_get_looping(channel_t* ch);
void channel_set_looping(channel_t* ch, bool is_loop);
void channel_stop(channel_t* ch);
void channel_play(channel_t* ch);
channel_error_t channel_next(channel_t* ch, wav_sample_t* sample);
channel_error_t channel_blend_next(channel_t* chs, uint8_t size, wav_sample_t* sample);

#endif