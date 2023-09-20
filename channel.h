
typedef struct channel_t channel_t;

typedef enum channel_error_t {
    CHANNEL_ERROR_SUCCESS = 0,
    CHANNEL_ERROR_EOF = 1,
    CHANNEL_ERROR_INVALID = 2,
} channel_error_t;
