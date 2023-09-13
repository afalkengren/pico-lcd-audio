#ifndef __MCP4922_H
#define __MCP4922_H

#include <stdlib.h>

typedef enum mcp4922_channel_t {
    MCP4922_DAC_A = 0,
    MCP4922_DAC_B = 1,
} mcp4922_channel_t;

void mcp4922_init();
void mcp4922_stereo_write(uint16_t* left_data, uint16_t* right_data);
void mcp4922_mono_write(uint16_t* audio_data);
void mcp4922_shutdown();

#endif