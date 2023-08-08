#include "hardware/spi.h"
#include "hardware/gpio.h"

#include "mcp4922.h"

#define SPI_PORT spi0
#define SPI_CLOCK (20 * 0000 * 1000) // 20 MHz

// MCP4922 uses a 3-wire serial protocol with dual latch 
#define PIN_SCK 18
#define PIN_SDI 19
#define PIN_CS 17
#define PIN_LDAC 18

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

void mcp4922_write16(mpc4922_channel_t channel, uint16_t* audio_data) {
    // add config bits
    // bit 15 (DAC selection): 0=DACa, 1=DACb
    // bit 14 (Vref input buffer): 1=buffered, 0=unbuffered
    // bit 13 (Output gain selection): 1=1x, 0=2x
    // bit 12 (Shutdown selected channel): 1=Active, 0=Shutdown
    uint16_t data = *audio_data;
    data |= (channel << 15);
    data |= (1 << 12); // set SHDN to active
    
    gpio_put(PIN_CS, 0);
    spi_write16_blocking(SPI_PORT, &data, 1);
    gpio_put(PIN_CS, 1);

    // pulse latch to write data to output buffer
    gpio_put(PIN_LDAC, 0);
    gpio_put(PIN_LDAC, 1);
}

// Writes data to both channels
void mcp4922_mono_write16(uint16_t* audio_data) {
    // add config bits
    // bit 15 (DAC selection): 0=DACa, 1=DACb
    // bit 14 (Vref input buffer): 1=buffered, 0=unbuffered
    // bit 13 (Output gain selection): 1=1x, 0=2x
    // bit 12 (Shutdown selected channel): 1=Active, 0=Shutdown
    uint16_t data = *audio_data;
    data |= (1 << 12); // set SHDN to active

    // write to channel A
    data &= ~(1 << 15); // make sure bit 15 is 0
    gpio_put(PIN_CS, 0);
    spi_write16_blocking(SPI_PORT, &data, 1);
    gpio_put(PIN_CS, 1);

    // write to channel B
    data |= (1 << 15);
    gpio_put(PIN_CS, 0);
    spi_write16_blocking(SPI_PORT, &data, 1);
    gpio_put(PIN_CS, 1);

    // pulse latch to write data to output buffer
    gpio_put(PIN_LDAC, 0);
    gpio_put(PIN_LDAC, 1);
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
