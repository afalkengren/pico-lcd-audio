#ifndef __MCP4922_H
#define __MCP4922_H

#include <stdlib.h>

typedef enum mpc4922_channel_t {
    MCP4922_DAC_A = 0,
    MCP4922_DAC_B = 1,
} mpc4922_channel_t;

void mcp4922_init();
void mcp4922_write16(mpc4922_channel_t channel, uint16_t* audio_data);
void mcp4922_mono_write16(uint16_t* audio_data);
void mcp4922_shutdown();

#endif