/*****************************************************************************
* | File        :   LCD_1in14.C
* | Function    :   test Demo
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2021-03-16
* | Info        :   
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/

// #include <stdlib.h>		//itoa()
#include <stdio.h>
#include <stdarg.h>

#include "hardware/pwm.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "pico/time.h"

#include "lcd1n14.h"

#define SPI_PORT spi1
#define SPI_CLOCK (62 * 5000 * 1000)

#define FRAME_SIZE (LCD_WIDTH * LCD_HEIGHT)

#define PIN_RST 12
#define PIN_BL 13 // backlight

#define PIN_DCX 8 // data/command flag 0=command 1=data
#define PIN_CSX 9 // active low cs
#define PIN_SCL 10
#define PIN_SDA 11

// State attributes
static lcd_scan_dir_t lcd_scan_dir = LCD_SCAN_DIR_VERTICAL;
static uint16_t lcd_height;
static uint16_t lcd_width;
static uint8_t pwm_slice_num;

// Internal helper functions
// static void lcd_spi_write(const uint8_t data);
// static void lcd_spi_write16(const uint16_t data);
static void gpio_pin_init(uint16_t pin, uint16_t mode);
static void lcd_init_LCD_registers(void);
static void lcd_reset(void);
static void dev_module_init(void);

static bool lcd_is_horizontal(void) {
    return (lcd_scan_dir == LCD_SCAN_DIR_HORIZONTAL);
}

typedef enum lcd_cmd_t {
    LCD_CMD_NOP = 0x00,
    LCD_CMD_SWRESET = 0x01, // Software reset
    LCD_CMD_RDDID = 0x04,   // Read display ID
    LCD_CMD_RDDST = 0x09,   // Read display status
    LCD_CMD_RDDPM = 0x0A,   // Read display power mode
    LCD_CMD_RDDMADCTL = 0x0B,   // Read display MADCTL
    LCD_CMD_RDDCOLMOD = 0x0C,   // Read display pixel format
    LCD_CMD_RDDIM = 0x0D,   // Read display image mode
    LCD_CMD_RDDSM = 0x0E,   // Read display signal mode
    LCD_CMD_RDDSDR = 0x0F,  // Read display self-diagnostic
    LCD_CMD_SLPIN = 0x10,   // Sleep in
    LCD_CMD_SLPOUT = 0x11,  // Sleep out
    
    LCD_CMD_CASET = 0x2A,   // Column address set
    LCD_CMD_RASET = 0x2B,   // Row address set
    LCD_CMD_RAMWR = 0x2C,   // Memory write
    LCD_CMD_RAMRD = 0x2E,   // Memory read
    LCD_CMD_PTLAR = 0x30,   // Partial area

    LCD_CMD_MADCTL = 0x36, // Memory Data Access Control (vertical or horizontal)
    LCD_CMD_COLMOD = 0x3A, // Display interface 

    LCD_CMD_RAMCTRL = 0xB0, // RAM control
    LCD_CMD_RGBCTRL = 0xB1, // RGB interface control
    LCD_CMD_PORCTRL = 0xB2, // Porch setting
    LCD_CMD_FRCTRL1 = 0xB3, // Frame rate control 1 (partial mode/idle colors)
    LCD_CMD_PARCTRL = 0xB5, // Partial control
    LCD_CMD_GCTRL = 0xB7,   // Gate control

    LCD_CMD_VCOMS = 0xBB, // VCOM setting

    LCD_CMD_LCMCTRL = 0xC0, // LCM control
} lcd_cmd_t;

/********************************************************************************
function :	Initialize the lcd
parameter:
********************************************************************************/
void lcd_init() {
    dev_module_init();
    lcd_set_backlight_brightness(90);
    lcd_reset(); // Hardware reset
    lcd_init_LCD_registers(); // Initialization of registers
}

/******************************************************************************
function :	send command
parameter:
     Reg : Command register
******************************************************************************/
static void lcd_cmd_write(lcd_cmd_t cmd_reg, int data_len, ...) {
    va_list data_list;
    va_start(data_list, data_len);

    // write the register to send to
    gpio_put(PIN_DCX, 0);
    gpio_put(PIN_CSX, 0);
    spi_write_blocking(SPI_PORT, &cmd_reg, 1);

    if (data_len > 0) {
        gpio_put(PIN_CSX, 1);
        gpio_put(PIN_DCX, 1);
        gpio_put(PIN_CSX, 0);
        for (int i = 0; i < data_len; ++i) {
            uint8_t data = va_arg(data_list, int);
            spi_write_blocking(SPI_PORT, &data, 1);
        }
    }
    gpio_put(PIN_CSX, 1);
}

static void lcd_cmd_write16(lcd_cmd_t cmd_reg, int data_len, ...) {
    va_list data_list;
    va_start(data_list, data_len);

    // write the register to send to
    gpio_put(PIN_DCX, 0);
    gpio_put(PIN_CSX, 0);
    spi_write_blocking(SPI_PORT, &cmd_reg, 1);
    
    if (data_len > 0) {
        gpio_put(PIN_CSX, 1);
        gpio_put(PIN_DCX, 1);
        gpio_put(PIN_CSX, 0);
        for (int i = 0; i < data_len; ++i) {
            uint16_t data = va_arg(data_list, int);
            uint8_t msb = (data >> 8) & 0xFF;
            uint8_t lsb = data & 0xFF;
            spi_write_blocking(SPI_PORT, &msb, 1);
            spi_write_blocking(SPI_PORT, &lsb, 1);
        }
    }
    gpio_put(PIN_CSX, 1);
}

/********************************************************************************
function:	Set the resolution and scanning method of the screen
parameter:
		Scan_dir:   Scan direction
********************************************************************************/
void lcd_set_scan_direction(lcd_scan_dir_t scan_dir) {
    //Get the screen scan direction
    lcd_scan_dir = scan_dir;

    // Get GRAM and LCD width and height
    // Set the read / write scan direction of the frame memory
    // D7 = MY, D6 = MX, D5 = MV, D4 = ML, D3 = RGB, D2 = MH
    // For horizontal, set MY=0 (top->bot), MX=1 (right->left), MV=1 (reverse), ML=1 (refresh top->bot)
    if(scan_dir == LCD_SCAN_DIR_HORIZONTAL) {
        lcd_height = LCD_1IN14_WIDTH;
        lcd_width = LCD_1IN14_HEIGHT;
        lcd_cmd_write(LCD_CMD_MADCTL, 1, 0b01110000); // MX, MY, RGB mode
    } else {
        lcd_height = LCD_1IN14_HEIGHT;       
        lcd_width = LCD_1IN14_WIDTH;
        lcd_cmd_write(LCD_CMD_MADCTL, 1, 0x00); // MX, MY, RGB mode
    }
}

/********************************************************************************
function:	Sets the start position and size of the display area
parameter:
		x_start 	:   X direction Start coordinates
		x_end    :   X direction end coordinates
        y_start  :   Y direction Start coordinates
		y_end    :   Y direction end coordinates
********************************************************************************/
void lcd_set_draw_area(uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end) {
    uint16_t x = (lcd_is_horizontal()) ?  40 : 52;
    uint16_t y = (lcd_is_horizontal()) ?  53 : 40;

    //set the X coordinates (column)
    lcd_cmd_write16(LCD_CMD_CASET, 2, x_start + x, x_end - 1 + x);

    //set the Y coordinates (row)
    lcd_cmd_write16(LCD_CMD_RASET, 2, y_start + y, y_end - 1 + y);
}

/******************************************************************************
function :	Clear screen
parameter:
******************************************************************************/
void lcd_clear(uint16_t color) {
    uint16_t framebuf[LCD_1IN14_SIZE];
    
    color = ((color << 8) & 0xFF00) | (color >> 8);
   
    for (uint16_t i = 0; i < LCD_1IN14_SIZE; ++i) {
        framebuf[i] = color;
    }
    
    lcd_set_draw_area(0, lcd_width, 0, lcd_height);
    lcd_draw_framebuf(framebuf);
}

/******************************************************************************
function :	Sends the image buffer in RAM to displays
parameter:
******************************************************************************/
void lcd_draw_framebuf(const uint16_t* framebuf) {
    lcd_set_draw_area(0, lcd_width, 0, lcd_height);
    // write the register to send to
    gpio_put(PIN_DCX, 0);
    gpio_put(PIN_CSX, 0);
    uint8_t cmd = LCD_CMD_RAMWR;
    spi_write_blocking(SPI_PORT, &cmd, 1);
    gpio_put(PIN_CSX, 1);

    gpio_put(PIN_DCX, 1);
    gpio_put(PIN_CSX, 0);
    for (uint16_t row = 0; row < lcd_height; ++row) {
        uint8_t* ptr_row = (uint8_t*)&framebuf[row * lcd_width];
        spi_write_blocking(SPI_PORT, ptr_row, lcd_width * 2);
    }
    gpio_put(PIN_CSX, 1);
}

/******************************************************************************
function:	Backlight brightness control
parameter:
		Value 	: 0 ~ 100
******************************************************************************/
void lcd_set_backlight_brightness(uint8_t value) {
    if (value < 0 || value > 100) {
        printf("Error! Set backlight brightness (value %d exceeds min/max)\r\n", value);
        if (value < 0) value = 0;
        else if (value > 100) value = 100;
    }
    pwm_set_chan_level(pwm_slice_num, PWM_CHAN_B, value);
}

/******************************************************************************
Internal functions
******************************************************************************/
static void gpio_pin_init(uint16_t pin, uint16_t mode) {
    gpio_init(pin);
    gpio_set_dir(pin, mode);
}

static void dev_module_init(void) {
    // SPI Config
    spi_init(SPI_PORT, SPI_CLOCK);
    // spi_set_format(SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_1, SPI_MSB_FIRST);
    gpio_set_function(PIN_SCL, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SDA, GPIO_FUNC_SPI);
    
    // RST active low
    gpio_pin_init(PIN_RST, GPIO_OUT);
    gpio_put(PIN_RST, 1);

    // DC active low
    gpio_pin_init(PIN_DCX, GPIO_OUT);
    gpio_put(PIN_DCX, 0);

    // Chip select active low
    gpio_pin_init(PIN_CSX, GPIO_OUT);
    gpio_put(PIN_CSX, 1);

    // RST active low
    gpio_pin_init(PIN_BL, GPIO_OUT);
    gpio_put(PIN_BL, 1);
    
    // PWM Config
    gpio_set_function(PIN_BL, GPIO_FUNC_PWM);
    pwm_slice_num = pwm_gpio_to_slice_num(PIN_BL);
    pwm_set_wrap(pwm_slice_num, 100);
    pwm_set_chan_level(pwm_slice_num, PWM_CHAN_B, 1);
    pwm_set_clkdiv(pwm_slice_num, 50);
    pwm_set_enabled(pwm_slice_num, true);
}

// static void lcd_spi_write(const uint8_t data) {
//     gpio_put(PIN_CSX, 0);
//     spi_write_blocking(SPI_PORT, &data, 1);
//     gpio_put(PIN_CSX, 1);
// }

// static void lcd_spi_write16(const uint16_t data) {
//     gpio_put(PIN_CSX, 0);
//     uint8_t b1 = (data >> 8) & 0xFF;
//     uint8_t b2 = data & 0xFF;
//     spi_write_blocking(SPI_PORT, &b1, 1);
//     spi_write_blocking(SPI_PORT, &b2, 1);
//     gpio_put(PIN_CSX, 1);
// }

static void lcd_init_LCD_registers(void) {
    lcd_cmd_write(LCD_CMD_COLMOD, 1, 0x05);

    lcd_cmd_write(LCD_CMD_PORCTRL, 5, 0x0C, 0x0C, 0x00, 0x33, 0x33);

    lcd_cmd_write(LCD_CMD_GCTRL, 1, 0x35);  //Gate Control
    lcd_cmd_write(LCD_CMD_VCOMS, 1, 0x19);  //VCOM Setting

    lcd_cmd_write(LCD_CMD_LCMCTRL, 1, 0x2C); // LCM Control

    lcd_cmd_write(0xC2, 1, 0x01); // VDV and VRH Command Enable
    lcd_cmd_write(0xC3, 1, 0x12); // VRH Set
    lcd_cmd_write(0xC4, 1, 0x20); // VDV Set
    lcd_cmd_write(0xC6, 1, 0x0F); // Frame Rate Control = Normal Mode
    lcd_cmd_write(0xD0, 2, 0xA4, 0xA1); // Power Control 1

    lcd_cmd_write(0xE0, 14, // Positive Voltage Gamma Control
        0xD0, 0x04, 0x0D, 0x11,
        0x13, 0x2B, 0x3F, 0x54,
        0x4C, 0x18, 0x0D, 0x0B,
        0x1F, 0x23
    );

    lcd_cmd_write(0xE1, 14, // Negative Voltage Gamma Control
        0xD0, 0x04, 0x0C, 0x11,
        0x13, 0x2C, 0x3F, 0x44,
        0x51, 0x2F, 0x1F, 0x1F,
        0x20, 0x23
    );

    lcd_cmd_write(0x21, 0);  //Display Inversion On
    lcd_cmd_write(LCD_CMD_SLPOUT, 0);  //Sleep Out
    lcd_cmd_write(0x29, 0);  //Display On
}

static void lcd_reset(void) {
    gpio_put(PIN_RST, 1);
    sleep_ms(100);
    gpio_put(PIN_RST, 0);
    sleep_ms(100);
    gpio_put(PIN_RST, 1);
    sleep_ms(100);
}