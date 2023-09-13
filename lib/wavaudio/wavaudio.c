#include <stdio.h>

#include "pico/stdlib.h"

#include "wavaudio.h"

typedef enum wav_format_t {
    WAVE_FORMAT_PCM = 1,
    WAVE_FORMAT_IEEE_FLOAT = 3,
    WAVE_FORMAT_ALAW = 6,
    WAVE_FORMAT_MULAW = 7,
    // WAVE_FORMAT_EXTENSIBLE = 0xFFFE,
} wav_format_t;

typedef struct wav_audio_t {
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

static wav_sample_t wav_next8(wav_audio_t* wav);
static wav_sample_t wav_next16(wav_audio_t* wav);

int wav_is_wav(const uint8_t* data) {
    const char* format = &data[8];
    return (format == "WAVE");
}

// this assumes that subchunk1 is PCM (16 bytes)
// should use subchunk1size at offset 16...?
wav_audio_t* wav_load(const uint8_t* data) {
    wav_audio_t* wav = malloc(sizeof(wav_audio_t));
    wav->format = *(uint16_t*)&data[20];
    wav->num_channels = *(uint16_t*)&data[22];
    wav->sample_rate = *(uint32_t*)&data[24];
    wav->byte_rate = *(uint32_t*)&data[28];
    wav->block_size = *(uint16_t*)&data[32];
    wav->bit_depth = *(uint16_t*)&data[34];
    wav->data_size = *(uint32_t*)&data[40];
    wav->data_head = &data[44];
    wav->data_end = &data[44] + wav->data_size - 1;
    wav->data_pos = &data[44];
    return wav;
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

wav_sample_t wav_next(wav_audio_t* wav_audio) {
    // if eof then just return empty
    if (wav_is_eof(wav_audio)) return (wav_sample_t){0, 0};

    if (wav_audio->bit_depth == 8) {
        return wav_next8(wav_audio);
    } else if (wav_audio->bit_depth == 16) {
        return wav_next16(wav_audio);
    }

    return (wav_sample_t){0, 0}; // ??? do something...
}

// static functions
static wav_sample_t wav_next8(wav_audio_t* wav_audio) {
    wav_sample_t sample;
    sample.left = *(int8_t*)wav_audio->data_pos;

    // if stereo, change data pointer. if not just use same ptr
    if (wav_audio->num_channels == 2) {
        wav_audio->data_pos += 1;
    }

    sample.right = *(int8_t*)wav_audio->data_pos; 
    wav_audio->data_pos += 1;
    return sample;
}

static wav_sample_t wav_next16(wav_audio_t* wav_audio) {
    wav_sample_t sample;
    sample.left = *(int16_t*)wav_audio->data_pos;

    // if stereo, change data pointer. if not just use same ptr
    if (wav_audio->num_channels == 2) {
        wav_audio->data_pos += 2;
    }
    
    sample.right = *(int16_t*)wav_audio->data_pos;
    wav_audio->data_pos += 2;
    return sample;
}