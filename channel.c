/*****************************************************************************
 * Ch
 * 
 * 
 * 
*****************************************************************************/

typedef struct channel_t {
    uint16_t* src;
    uint16_t* ptr;
    channel_state_t state;
    bool is_loop;
} channel_t;

typedef enum channel_state_t {
    STOP = 0,
    PLAYING = 1,
    RESTART = 2,
} channel_state_t;

