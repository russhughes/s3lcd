/*
 * Copyright (c) 2023 Russ Hughes
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef __s3lcd_i80_bus_H__
#define __s3lcd_i80_bus_H__

#include "mphalport.h"
#include "py/obj.h"
#include "esp_lcd_panel_io.h"

// i80 Configuration

typedef struct _s3lcd_i80_bus_obj_t {
    mp_obj_base_t base;                     // base class
    char *name;                             // name of the display
    int data_gpio_nums[24];                 // GPIOs used for data lines
    int dc_gpio_num;                        // GPIO used for D/C line
    int wr_gpio_num;                        // GPIO used for WR line
    int rd_gpio_num;                        // GPIO used for RD line, set to -1 will not read from the display
    int cs_gpio_num;                        // GPIO used for CS line, set to -1 will declaim exclusively use of I80 bus
    int reset_gpio_num;                     // GPIO used for RESET line, set to -1 will not reset the display

    unsigned int pclk_hz;                   // Frequency of pixel clock
    size_t bus_width;                       // Number of data lines, 8 or 16
    int lcd_cmd_bits;                       // Bit-width of LCD command
    int lcd_param_bits;                     // Bit-width of LCD parameter

    struct {
        unsigned int dc_idle_level: 1;      // Level of DC line in IDLE phase
        unsigned int dc_cmd_level: 1;       // Level of DC line in CMD phase
        unsigned int dc_dummy_level: 1;     // Level of DC line in DUMMY phase
        unsigned int dc_data_level: 1;      // Level of DC line in DATA phase
    } dc_levels;

    struct {
        unsigned int cs_active_high: 1;     // Whether the CS line is active on high level
        unsigned int reverse_color_bits: 1; // Reverse the data bits, D[N:0] -> D[0:N]
        unsigned int swap_color_bytes: 1;   // Swap adjacent two data bytes before sending out
        unsigned int pclk_active_neg: 1;    // The display will write data lines when there's a falling edge on WR line
        unsigned int pclk_idle_low: 1;      // The WR line keeps at low level in IDLE phase
    } flags;

} s3lcd_i80_bus_obj_t;

extern const mp_obj_type_t s3lcd_i80_bus_type;

#endif /* __i80_bus_H__ */
