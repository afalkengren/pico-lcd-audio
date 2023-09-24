#include <stdio.h>

#include "pico/stdlib.h"

#include "wavaudio.h"

typedef enum wav_format_t {
    WAV_FORMAT_PCM = 1,
    WAV_FORMAT_IEEE_FLOAT = 3,
    WAV_FORMAT_ALAW = 6,
    WAV_FORMAT_MULAW = 7,
    // WAVE_FORMAT_EXTENSIBLE = 0xFFFE,
} wav_format_t;

typedef struct wav_audio_t {
    const uint8_t* src;
    wav_format_t format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate; // bytes per second
    uint8_t block_size; // bytes per block
    uint32_t bit_depth; // bit per sample
    uint32_t data_size;
    const uint8_t* data_head;
    const uint8_t* data_end;
    const uint8_t* data_pos;
} wav_audio_t;

static wav_result_t wav_next_pcm(wav_audio_t* wav_audio, wav_sample_t* sample, uint8_t n_bytes);
static wav_result_t wav_next_float_32(wav_audio_t* wav_audio, wav_sample_t* sample);

void wav_add_samples(wav_sample_t* a, wav_sample_t* b) {
    a->left += b->left;
    a->right += b->right;
}

int wav_is_wav(const uint8_t* data) {
    const char* format = &data[8];
    return (format == "WAVE");
}

wav_audio_t* wav_new(const uint8_t* data) {
    wav_audio_t* wav = (wav_audio_t*)malloc(sizeof(wav_audio_t));
    wav->src = data;
    return wav;
}

wav_result_t wav_load(wav_audio_t* wav) {
    // subchunk1 size at offset 16 (not incl. current offset at 20)
    uint32_t data_head = *(uint32_t*)&wav->src[16] + 20;

    wav->format = *(uint16_t*)&wav->src[20];
    wav->num_channels = *(uint16_t*)&wav->src[22];
    wav->sample_rate = *(uint32_t*)&wav->src[24];
    wav->byte_rate = *(uint32_t*)&wav->src[28];
    wav->block_size = *(uint16_t*)&wav->src[32];
    wav->bit_depth = *(uint16_t*)&wav->src[34];
    
    char* chunk2_id = (char*)&wav->src[data_head];
    if (chunk2_id != "data") {
        free(wav);
        return WAV_RESULT_LOAD_FAILED;
    }
    wav->data_size = *(uint32_t*)&wav->src[data_head + 4];
    wav->data_head = &wav->src[data_head + 8];
    wav->data_end = &wav->src[data_head + 8] + wav->data_size - 1;
    wav->data_pos = &wav->src[data_head + 8];
    return WAV_RESULT_SUCCESS;
}

void wav_reset(wav_audio_t* wav) {
    wav->data_pos = wav->data_head;
}

int wav_is_eof(const wav_audio_t* wav) {
    return wav->data_pos >= wav->data_end;
}

int wav_is_stereo(const wav_audio_t* wav) {
    return wav->num_channels == 2;
}

uint32_t wav_get_sample_rate(const wav_audio_t* wav) {
    return wav->sample_rate;
}

uint32_t wav_get_sample_period_us(const wav_audio_t* wav) {
    return 1000000/wav->sample_rate;
}

uint32_t wav_get_data_size(const wav_audio_t* wav) {
    return wav->data_size;
}

uint32_t wav_get_sample_length(const wav_audio_t* wav) {
    return wav->data_size / (wav->block_size * wav->num_channels);
}

const wav_pos_t* wav_get_pos(const wav_audio_t* wav) {
    return wav->data_pos;
}

void wav_set_pos(wav_audio_t* wav, const wav_pos_t* pos) {
    wav->data_pos = pos;
}

void wav_set_pos_offset(wav_audio_t* wav, const wav_pos_t* pos, uint8_t offset) {
    const wav_pos_t* new_pos = pos + (wav->block_size * offset);
    printf("setting wav pos. %p -> %p + %d -> %p\n", wav->data_pos, pos, offset, new_pos);
    wav_set_pos(wav, new_pos);
}

wav_result_t wav_next(wav_audio_t* wav_audio, wav_sample_t* sample) {
    // if eof then just return empty
    if (wav_is_eof(wav_audio)) return WAV_RESULT_EOF;

    wav_result_t res = WAV_RESULT_UNKNOWN;
    switch(wav_audio->bit_depth) {
        case 8:
            res = wav_next_pcm(wav_audio, sample, 1);
            break;
        case 16:
            res = wav_next_pcm(wav_audio, sample, 2);
            break;
        case 24:
            res = wav_next_pcm(wav_audio, sample, 3);
            break;
        case 32:
            res = wav_next_float_32(wav_audio, sample);
            break;
    }
    return res;
}

// static functions
static wav_result_t wav_next_pcm(wav_audio_t* wav_audio, wav_sample_t* sample, uint8_t n_bytes) {
    if (wav_audio->format != WAV_FORMAT_PCM) return WAV_RESULT_UNSUPPORTED;
    bool is_stereo = (wav_audio->num_channels == 2);
    if (n_bytes == 1) {
        // 8 byte bit depth is always unsigned as per the spec
        sample->left = (*(uint8_t*)wav_audio->data_pos) << 8;
        // if stereo, change data pointer. if not just use same ptr
        if (wav_is_stereo(wav_audio)) wav_audio->data_pos += 1;
        sample->right = (*(uint8_t*)wav_audio->data_pos) << 8;
        wav_audio->data_pos += 1;
    } else {
        // take top two bytes, will still be valid int16 with reduced precision
        // signed sample, convert to unsigned for DAC
        sample->left = *(int16_t*)wav_audio->data_pos + (INT16_MAX + 1);
        
        // if stereo, change data pointer. if not just use same ptr
        if (wav_is_stereo(wav_audio)) {
            wav_audio->data_pos += n_bytes;
            sample->right = *(int16_t*)wav_audio->data_pos + (INT16_MAX + 1);
        } else {
            sample->right = sample->left;
        }

        wav_audio->data_pos += n_bytes;
    }

    return WAV_RESULT_SUCCESS;
}

static wav_result_t wav_next_float_32(wav_audio_t* wav_audio, wav_sample_t* sample) {
    if (wav_audio->format != WAV_FORMAT_IEEE_FLOAT) return WAV_RESULT_UNSUPPORTED;
    sample->left = *(float*)wav_audio->data_pos;

    // if stereo, change data pointer. if not just use same ptr
    if (wav_audio->num_channels == 2) {
        wav_audio->data_pos += 4;
    }
    
    sample->right = *(float*)wav_audio->data_pos;
    wav_audio->data_pos += 4;
    return WAV_RESULT_SUCCESS;
}