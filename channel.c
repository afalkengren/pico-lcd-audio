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

enum CHANNEL_FLAGS {
    CHANNEL_FLAG_IS_LOOP = 0,
}

typedef struct channel_t {
    wav_audio_t* src;
    channel_state_t state;
    channel_flags_t flags;
} channel_t;

typedef enum channel_state_t {
    CHANNEL_STATE_STOP = 0,
    CHANNEL_STATE_START = 1,
    CHANNEL_STATE_PLAYING = 2,
    CHANNEL_STATE_RESTART = 3,
} channel_state_t;

channel_t* channel_new() {
    channel_t* ch = (channel_t*)malloc(sizeof(channel_t));
    ch->state = CHANNEL_STATE_STOP;
    FLAG_RESET(ch->flags);
    return ch;
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

void channel_start(channel_t* ch) {
    ch->state = CHANNEL_STATE_START;
}

void channel_restart(channel_t* ch) {
    ch->state = CHANNEL_STATE_RESTART;
}

void channel_next(channel_t* ch) {
    switch(ch->state) {
        case CHANNEL_STATE_STOP:
            wav_
    }
}