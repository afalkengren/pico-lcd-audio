/*****************************************************************************
 * Ch
 * 
 * 
 * 
*****************************************************************************/
#include <stdbool.h>

#include "wavaudio.h"

#define FLAG_GET(x, n) ((x >> n) & 1U)
#define FLAG_SET(x, n) (x |= (1U << n))
#define FLAG_CLEAR(x, n) (x &= ~(1U << n))
#define FLAG_FLIP(x, n) (x ^= (1U << n))
#define FLAG_RESET(x) (x = 0)

typedef uint16_t channel_flags_t;

enum CHANNEL_FLAGS_BITMASK {
    CHANNEL_FLAG_IS_LOOP = 0,
};

typedef struct channel_t {
    wav_audio_t* src;
    uint8_t pin;
    int8_t gain;
    channel_state_t state;
    channel_flags_t flags;
} channel_t;

typedef enum channel_state_t {
    CHANNEL_STATE_STOP = 0,
    CHANNEL_STATE_START = 1,
    CHANNEL_STATE_PLAYING = 2,
    //CHANNEL_STATE_RESTART = 3,
} channel_state_t;

channel_t* channel_new(uint8_t* ptr, uint8_t pin) {
    channel_t* ch = (channel_t*)malloc(sizeof(channel_t));
    
    ch->src = wav_load(ptr);
    
    ch->pin = pin;
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_up(pin);

    ch->state = CHANNEL_STATE_STOP;

    FLAG_RESET(ch->flags);
    
    return ch;
}

uint8_t channel_get_pin(channel_t* ch) {
    return ch->pin;
}

bool channel_get_looping(channel_t* ch) {
    return FLAG_GET(ch->flags, CHANNEL_FLAG_IS_LOOP);
}

void channel_set_looping(channel_t* ch, bool is_loop) {
    if (is_loop) {
        FLAG_SET(ch->flags, CHANNEL_FLAG_IS_LOOP);
    } else {
        FLAG_CLEAR(ch->flags, CHANNEL_FLAG_IS_LOOP);
    }
}

void channel_stop(channel_t* ch) {
    ch->state = CHANNEL_STATE_STOP;
}

void channel_play(channel_t* ch) {
    ch->state = CHANNEL_STATE_START;
}

// process next state for one channel (next sample or stop)
// TODO: handle WAVE and other formats
channel_error_t channel_next(channel_t* ch, wav_sample_t* sample) {
    return CHANNEL_ERROR
    switch(ch->state) {
        case CHANNEL_STATE_STOP:
            wav_reset(ch->src);
            return CHANNEL_ERROR_INVALID;
        case CHANNEL_STATE_START:
            // start should reset then following into playing
            wav_reset(ch->src);
            ch->state = CHANNEL_STATE_PLAYING;
        case CHANNEL_STATE_PLAYING:
            wav_result_t res = wav_next(ch->src, sample);
            if (res == WAV_RESULT_SUCCESS) {
                // TODO: handle gain. unknown if processing power enough
                return CHANNEL_ERROR_SUCCESS;
            }
            // if not success, stop channel and reset
            ch->state = CHANNEL_STATE_STOP;
            return CHANNEL_ERROR_INVALID;
        default:
            return CHANNEL_ERROR_UNKNOWN;
    }
}

// process all channels and blend samples
// TODO: handle different sampling rates
channel_error_t channel_blend_next(channel_t* chs, uint8_t size, wav_sample_t* sample) {
    for (uint8_t i = 0; i < size; ++i) {
        wav_sample_t sample_channel;
        if (channel_next(chs[i], &sample_channel) == WAV_RESULT_SUCCESS) {
            sample += sample_channel;
        }
    }
    return CHANNEL_ERROR_SUCCESS;
}