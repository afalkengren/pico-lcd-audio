#ifndef __WAV_AUDIO_H
#define __WAV_AUDIO_H

#include <stdint.h>
#include <stdlib.h>

typedef enum wav_result_t {
    WAV_RESULT_SUCCESS = 0,
    WAV_RESULT_EOF = 1,
    WAV_RESULT_UNSUPPORTED = 2,
    WAV_RESULT_UNKNOWN = 255,
} wav_result_t;

typedef uint8_t wav_pos_t;
typedef enum wav_format_t wav_format_t;
typedef struct wav_audio_t wav_audio_t;
typedef struct wav_sample_t {
    uint16_t left;
    uint16_t right;
} wav_sample_t;

void wav_add_samples(wav_sample_t* a, wav_sample_t* b);

int wav_is_wav(const uint8_t* data);
wav_audio_t* wav_load(const uint8_t* data);
void wav_reset(wav_audio_t* wav);
int wav_is_eof(const wav_audio_t* wav);
int wav_is_stereo(const wav_audio_t* wav);
uint32_t wav_get_sample_rate(const wav_audio_t* wav);
uint32_t wav_get_sample_period_us(const wav_audio_t* wav);
uint32_t wav_get_data_size(const wav_audio_t* wav);
wav_result_t wav_next(wav_audio_t* wav_audio, wav_sample_t* sample);
const wav_pos_t* wav_get_pos(const wav_audio_t* wav);
void wav_set_pos(wav_audio_t* wav, const wav_pos_t* pos);
void wav_set_pos_offset(wav_audio_t* wav, const wav_pos_t* pos, uint8_t offset);

#endif