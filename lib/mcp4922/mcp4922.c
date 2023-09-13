#include <stdio.h>

#include "hardware/spi.h"
#include "hardware/gpio.h"

#include "mcp4922.h"

#define SPI_PORT spi0
#define SPI_CLOCK (62 * 5000 * 1000) // 62.5 MHz

// MCP4922 uses a 3-wire serial protocol with dual latch 
#define PIN_SCK 18
#define PIN_SDI 19
#define PIN_CS 17
#define PIN_LDAC 16

#define UINT12_MAX ((1 << 12) - 1)

static void mcp4922_write16(mcp4922_channel_t channel, uint16_t* data);
static void mcp4922_latch();

void mcp4922_init() {
    // Setup SPI at 20 MHz and 16 data bits
    spi_init(SPI_PORT, SPI_CLOCK);
    spi_set_format(SPI_PORT, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SDI, GPIO_FUNC_SPI);

    // Active low chip select, put 1.
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    // Active low latch, put 1.
    gpio_init(PIN_LDAC);
    gpio_set_dir(PIN_LDAC, GPIO_OUT);
    gpio_put(PIN_CS, 1);
}

void mcp4922_stereo_write(uint16_t* left_data, uint16_t* right_data) {
    mcp4922_write16(MCP4922_DAC_A, left_data);
    mcp4922_write16(MCP4922_DAC_B, right_data);
    mcp4922_latch();
}

// Writes data to both channels
void mcp4922_mono_write(uint16_t* audio_data) {
    mcp4922_write16(MCP4922_DAC_A, audio_data);
    mcp4922_write16(MCP4922_DAC_B, audio_data);
    mcp4922_latch();
}

void mcp4922_shutdown() {
    uint16_t data = 0x00;

    // Shutdown channel A
    gpio_put(PIN_CS, 0);
    spi_write16_blocking(SPI_PORT, &data, 1);
    gpio_put(PIN_CS, 1);

    data |= (1 << 15); // set channel B
    gpio_put(PIN_CS, 0);
    spi_write16_blocking(SPI_PORT, &data, 1);
    gpio_put(PIN_CS, 1);
}

static void mcp4922_write16(mcp4922_channel_t channel, uint16_t* data) {
    // bit 15 (DAC selection): 0=DACa, 1=DACb
    // bit 14 (Vref input buffer): 1=buffered, 0=unbuffered
    // bit 13 (Output gain selection): 1=1x, 0=2x
    // bit 12 (Shutdown selected channel): 1=Active, 0=Shutdown
    
    // copy data, scale and add config bits
    uint16_t message = 0;
    message |= (*data >> 4); // scale 16 bit data to 12 bit dynamic range
    message |= (channel << 15); // set channel
    message |= (1 << 13); // set output gain to 1x
    message |= (1 << 12); // set SHDN to active
    
    gpio_put(PIN_CS, 0);
    spi_write16_blocking(SPI_PORT, &message, 1);
    gpio_put(PIN_CS, 1);

    // printf("writing 0x%04x to channel: %d\n", message, channel);
}

// pulse latch to write data to output buffer
static void mcp4922_latch() {
    gpio_put(PIN_LDAC, 0);
    gpio_put(PIN_LDAC, 1);
}