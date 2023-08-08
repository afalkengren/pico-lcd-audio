/*****************************************************************************
* | File        :   LCD_1in14.h
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
#ifndef __LCD_1IN14_H
#define __LCD_1IN14_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define LCD_1IN14_HEIGHT 240
#define LCD_1IN14_WIDTH 135
#define LCD_1IN14_SIZE (LCD_1IN14_HEIGHT*LCD_1IN14_WIDTH)

#define LCD_1IN14_SetBacklight(Value) ; 

typedef enum lcd_scan_dir_t {
    LCD_SCAN_DIR_HORIZONTAL = 0,
    LCD_SCAN_DIR_VERTICAL = 1,
} lcd_scan_dir_t;

/********************************************************************************
function: 
    Macro definition variable name
********************************************************************************/
void lcd_init();
void lcd_clear(uint16_t color);
void lcd_draw_framebuf(const uint16_t* framebuf);
void lcd_set_scan_direction(lcd_scan_dir_t scan_dir);
void lcd_set_backlight_brightness(uint8_t value);
void lcd_set_draw_area(uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end);

#endif
