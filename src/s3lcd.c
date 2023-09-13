/*
 * Modifications and additions Copyright (c) 2020-2023 Russ Hughes
 *
 * This file licensed under the MIT License and incorporates work covered by
 * the following copyright and permission notice:
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ivan Belokobylskiy
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

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"

#include "esp_lcd_panel_commands.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "soc/soc_caps.h"
#include "driver/gpio.h"
#include "driver/dedic_gpio.h"

#include "py/obj.h"
#include "py/objstr.h"
#include "py/objmodule.h"
#include "py/runtime.h"
#include "py/builtin.h"
#include "py/mphal.h"
#include "extmod/machine_spi.h"

#include "mpfile.h"
#include "s3lcd.h"
#include "s3lcd_i80_bus.h"
#include "s3lcd_spi_bus.h"

#include "jpg/tjpgd565.h"

#define PNGLE_NO_GAMMA_CORRECTION
#include "png/pngle.h"
#include "pngenc/pngenc.h"

#define TAG "S3LCD"

#define _swap_bytes(val) (((val >> 8) | (val << 8)) & 0xFFFF)
#define _swap_int16_t(a, b) \
    {                       \
        int16_t t = a;      \
        a = b;              \
        b = t;              \
    }

#define ABS(N) (((N) < 0) ? (-(N)) : (N))
#define mp_hal_delay_ms(delay) (mp_hal_delay_us(delay * 1000))

//
// Default s3lcd display orientation tables
// can be overridden during init()
//
// typedef struct _s3lcd_rotation_t {
//     uint16_t width;     // width of the display in this rotation
//     uint16_t height;    // height of the display in this rotation
//     uint16_t x_gap;     // gap on x axis, in pixels
//     uint16_t y_gap;     // gap on y axis, in pixels
//     bool swap_xy;       // set MADCTL_MV bit 0x20
//     bool mirror_x;      // set MADCTL MX bit 0x40
//     bool mirror_y;      // set MADCTL MY bit 0x80
// } s3lcd_rotation_t;
//
// { width, height, x_gap, y_gap, swap_xy, mirror_x, mirror_y }

s3lcd_rotation_t ROTATIONS_320x480[4] = {
    {320, 480, 0, 0, false, true,  false},
    {480, 320, 0, 0, true,  false, false},
    {320, 480, 0, 0, false, false, true},
    {480, 320, 0, 0, true,  true,  true}
};

s3lcd_rotation_t ROTATIONS_240x320[4] = {
    {240, 320, 0, 0, false, false, false},
    {320, 240, 0, 0, true,  true,  false},
    {240, 320, 0, 0, false, true,  true},
    {320, 240, 0, 0, true,  false, true}
};

s3lcd_rotation_t ROTATIONS_170x320[4] = {
    {170, 320, 35, 0, false, false, false},
    {320, 170, 0, 35, true,  true,  false},
    {170, 320, 35, 0, false, true,  true},
    {320, 170, 0, 35, true,  false, true}
};

s3lcd_rotation_t ROTATIONS_240x240[4] = {
    {240, 240, 0, 0, false, false, false},
    {240, 240, 0, 0, true,  true,  false},
    {240, 240, 0, 80, false, true,  true},
    {240, 240, 80, 0, true,  false, true}
};

s3lcd_rotation_t ROTATIONS_135x240[4] = {
    {135, 240, 52, 40, false, false, false},
    {240, 135, 40, 53, true,  true,  false},
    {135, 240, 53, 40, false, true,  true},
    {240, 135, 40, 52, true,  false, true}
};

s3lcd_rotation_t ROTATIONS_128x160[4] = {
    {128, 160, 0, 0, false, false, false},
    {160, 128, 0, 0, true,  true,  false},
    {128, 160, 0, 0, false, true,  true},
    {160, 128, 0, 0, true,  false, true}
};

s3lcd_rotation_t ROTATIONS_80x160[4] = {
    {80, 160, 26, 1, false, false, false},
    {160, 80, 1, 26, true, true, false},
    {80, 160, 26, 1, false, true,  true},
    {160, 80, 1, 26, true,  false, true}
};

s3lcd_rotation_t ROTATIONS_128x128[4] = {
    {128, 128, 2, 1, false, false, false},
    {128, 128, 1, 2, true,  true,  false},
    {128, 128, 2, 3, false, true,  true},
    {128, 128, 3, 2, true,  false, true}
};

s3lcd_rotation_t *ROTATIONS[] = {
    ROTATIONS_240x320,              // default if no match
    ROTATIONS_320x480,
    ROTATIONS_170x320,
    ROTATIONS_240x240,
    ROTATIONS_135x240,
    ROTATIONS_128x160,
    ROTATIONS_80x160,
    ROTATIONS_128x128,
    NULL
};

//
// flag to indicate an esp_lcd_panel_draw_bitmap operation is in progress
//

STATIC volatile bool lcd_panel_active = false;

STATIC void s3lcd_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    (void)kind;
    s3lcd_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "<s3lcd width=%u, height=%u rotation=%u color_space=%u>", self->width, self->height, self->rotation, self->color_space);
}

// STATIC int brightness(uint16_t color) {
//     uint8_t r = (color & 0xf800) >> 10;
//     uint8_t g = (color & 0x07e0) >> 4;
//     uint8_t b = (color & 0x001f) << 1;
//     return (int)sqrtf(r * .241 + g * .691 + b * .068);
// }

STATIC uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
    return (uint16_t) ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3);
}

STATIC uint16_t alpha_blend_565(uint16_t fg, uint16_t bg, uint8_t alpha) {
    if (alpha == 0) {
        return bg;
    }

    if (alpha == 255) {
        return fg;
    }

    uint8_t r = ((fg >> 11) * alpha + (bg >> 11) * (255 - alpha)) >> 8;
    uint8_t g = (((fg >> 5) & 0x3f) * alpha + ((bg >> 5) & 0x3f) * (255 - alpha)) >> 8;
    uint8_t b = ((fg & 0x1f) * alpha + (bg & 0x1f) * (255 - alpha)) >> 8;
    return (r << 11) | (g << 5) | b;
}

#define OPTIONAL_ARG(arg_num, arg_type, arg_obj_get, arg_name, arg_default) \
    arg_type arg_name = arg_default;                                        \
    if (n_args > arg_num) {                                                 \
        arg_name = arg_obj_get(args[arg_num]);                              \
    }                                                                       \

#define COPY_TO_BUFFER(self, s, d, w, h)                    \
{                                                           \
    while (h--) {                                           \
        for (size_t ww = w; ww; --ww) {                     \
                *d++ = *s++;                                \
        }                                                   \
        d += self->width - w;                               \
    }                                                       \
}

#define BLEND_TO_BUFFER(self, s, d, w, h, alpha)            \
{                                                           \
    while (h--) {                                           \
        for (size_t ww = w; ww; --ww) {                     \
            *d = alpha_blend_565(*s++, *d, alpha);          \
            d++;                                            \
        }                                                   \
        d += self->width - w;                               \
    }                                                       \
}

STATIC void _setpixel(s3lcd_obj_t *self, uint16_t x, uint16_t y, uint16_t color, uint8_t alpha) {
    if ((x < self->width) && (y < self->height)) {
        uint16_t *b = self->frame_buffer + y * self->width + x;
        if (alpha < 255) {
            color = alpha_blend_565(color, *b, alpha);
        }
        *b = color;
    }
}

// STATIC uint16_t _getpixel(s3lcd_obj_t *self, uint16_t x, uint16_t y) {
//     return *(self->frame_buffer + x + y * self->width);
// }

STATIC void _fill(s3lcd_obj_t *self, uint16_t color) {
    uint16_t *b = self->frame_buffer;
    for (size_t i = 0; i < self->width * self->height; ++i) {
        *b++ = color;
    }
}

STATIC void _fill_rect(s3lcd_obj_t *self, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color, uint8_t alpha) {

    if (x >= self->width || (y >= self->height)) {
        return;
    }

    if (x + w > self->width) {
        w = self->width - x;
    }
    if (y + h > self->height) {
        h = self->height - y;
    }

    uint16_t *b = self->frame_buffer + y * self->width + x;
    if (alpha == 255) {
        while (h--) {
            for (size_t ww = w; ww; --ww) {
                *b++ = color;
            }
            b += self->width - w;
        }
    } else {
        while (h--) {
            for (size_t ww = w; ww; --ww) {
                *b = alpha_blend_565(color, *b, alpha);
                b++;
            }
            b += self->width - w;
        }
    }
}

STATIC int mod(int x, int m) {
    int r = x % m;
    return (r < 0) ? r + m : r;
}

void draw_pixel(s3lcd_obj_t *self, int16_t x, int16_t y, uint16_t color, uint8_t alpha) {
    if ((self->options & OPTIONS_WRAP)) {
        if ((self->options & OPTIONS_WRAP_H) && ((x >= self->width) || (x < 0))) {
            x = mod(x, self->width);
        }
        if ((self->options & OPTIONS_WRAP_V) && ((y >= self->height) || (y < 0))) {
            y = mod(y, self->height);
        }
    }
    _setpixel(self, x, y, color, alpha);
}

void fast_hline(s3lcd_obj_t *self, int16_t x, int16_t y, int16_t w, uint16_t color, uint8_t alpha) {
    if ((self->options & OPTIONS_WRAP) == 0) {
        if (y >= 0 && self->width > x && self->height > y) {
            if (0 > x) {
                w += x;
                x = 0;
            }

            if (self->width < x + w) {
                w = self->width - x;
            }

            if (w > 0) {
                _fill_rect(self, x, y, w, 1, color, alpha);
            }
        }
    } else {
        for (int d = 0; d < w; d++) {
            draw_pixel(self, x + d, y, color, alpha);
        }
    }
}

STATIC void fast_vline(s3lcd_obj_t *self, int16_t x, int16_t y, int16_t h, uint16_t color, uint8_t alpha) {
    if ((self->options & OPTIONS_WRAP) == 0) {
        if (x >= 0 && self->width > x && self->height > y) {
            if (0 > y) {
                h += y;
                y = 0;
            }

            if (self->height < y + h) {
                h = self->height - y;
            }

            if (h > 0) {
                _fill_rect(self, x, y, 1, h, color, alpha);
            }
        }
    } else {
        for (int d = 0; d < h; d++) {
            draw_pixel(self, x, y + d, color, alpha);
        }
    }
}

///
/// .reset()
/// Reset the display.
///

STATIC mp_obj_t s3lcd_reset(mp_obj_t self_in) {
    s3lcd_obj_t *self = MP_OBJ_TO_PTR(self_in);
    esp_lcd_panel_reset(self->panel_handle);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(s3lcd_reset_obj, s3lcd_reset);

///
/// .inversion_mode(value)
/// Set inversion mode
/// required parameters:
/// -- value: True to enable inversion mode, False to disable.
///

STATIC mp_obj_t s3lcd_inversion_mode(mp_obj_t self_in, mp_obj_t value) {
    s3lcd_obj_t *self = MP_OBJ_TO_PTR(self_in);
    self->inversion_mode = mp_obj_is_true(value);
    esp_lcd_panel_invert_color(self->panel_handle, self->inversion_mode);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(s3lcd_inversion_mode_obj, s3lcd_inversion_mode);

///
/// .fill_rect(x, y, w, h{, color, alpha})
/// Fill a rectangle with the given color.
/// required parameters:
/// -- x: x coordinate of the top left corner
/// -- y: y coordinate of the top left corner
/// -- w: width of the rectangle
/// -- h: height of the rectangle
/// optional parameters:
/// -- color: color of the rectangle
/// -- alpha: alpha value of the rectangle
///

STATIC mp_obj_t s3lcd_fill_rect(size_t n_args, const mp_obj_t *args) {
    s3lcd_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t x = mp_obj_get_int(args[1]);
    mp_int_t y = mp_obj_get_int(args[2]);
    mp_int_t w = mp_obj_get_int(args[3]);
    mp_int_t h = mp_obj_get_int(args[4]);
    OPTIONAL_ARG(5, mp_int_t, mp_obj_get_int, color, WHITE)
    OPTIONAL_ARG(6, mp_int_t, mp_obj_get_int, alpha, 255)

    _fill_rect(self, x, y, w, h, color, alpha);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(s3lcd_fill_rect_obj, 6, 7, s3lcd_fill_rect);


///
/// .clear({ 8_bit_color})
/// Fast framebuffer clear to given color. The the high and low bytes of the
/// color will be set to given byte.
/// optional parameters:
/// -- 8_bit_color defaults to 0x00 BLACK
///

STATIC mp_obj_t s3lcd_clear(size_t n_args, const mp_obj_t *args) {
    s3lcd_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    OPTIONAL_ARG(1, mp_int_t, mp_obj_get_int, color, BLACK)
    memset(self->frame_buffer, color & 0xff , self->frame_buffer_size);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(s3lcd_clear_obj, 2, 3, s3lcd_clear);


///
/// .fill({color, alpha})
/// Fill the screen with the given color.
/// optional parameters:
/// -- color defaults to BLACK
/// -- alpha defaults to 255
///

STATIC mp_obj_t s3lcd_fill(size_t n_args, const mp_obj_t *args) {
    s3lcd_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    OPTIONAL_ARG(1, mp_int_t, mp_obj_get_int, color, BLACK)
    // OPTIONAL_ARG(2, mp_int_t, mp_obj_get_int, alpha, 255)

    _fill(self, color);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(s3lcd_fill_obj, 2, 3, s3lcd_fill);


///
/// .pixel(x, y{, color, alpha})
/// Draw a pixel at the given coordinates.
/// required parameters:
/// -- x: x coordinate of the pixel
/// -- y: y coordinate of the pixel
/// optional parameters:
/// -- color defaults to WHITE
/// -- alpha defaults to 255
///

STATIC mp_obj_t s3lcd_pixel(size_t n_args, const mp_obj_t *args) {
    s3lcd_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t x = mp_obj_get_int(args[1]);
    mp_int_t y = mp_obj_get_int(args[2]);
    OPTIONAL_ARG(3, mp_int_t, mp_obj_get_int, color, WHITE)
    OPTIONAL_ARG(4, mp_int_t, mp_obj_get_int, alpha, 255)

    draw_pixel(self, x, y, color, alpha);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(s3lcd_pixel_obj, 4, 5, s3lcd_pixel);


STATIC void line(s3lcd_obj_t *self, int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t color, uint8_t alpha) {
    bool steep = ABS(y1 - y0) > ABS(x1 - x0);
    if (steep) {
        _swap_int16_t(x0, y0);
        _swap_int16_t(x1, y1);
    }

    if (x0 > x1) {
        _swap_int16_t(x0, x1);
        _swap_int16_t(y0, y1);
    }

    int16_t dx = x1 - x0, dy = ABS(y1 - y0);
    int16_t err = dx >> 1, ystep = -1, xs = x0, dlen = 0;

    if (y0 < y1) {
        ystep = 1;
    }

    // Split into steep and not steep for FastH/V separation
    if (steep) {
        for (; x0 <= x1; x0++) {
            dlen++;
            err -= dy;
            if (err < 0) {
                err += dx;
                if (dlen == 1) {
                    draw_pixel(self, y0, xs, color, alpha);
                } else {
                    fast_vline(self, y0, xs, dlen, color, alpha);
                }
                dlen = 0;
                y0 += ystep;
                xs = x0 + 1;
            }
        }
        if (dlen) {
            fast_vline(self, y0, xs, dlen, color, alpha);
        }
    } else {
        for (; x0 <= x1; x0++) {
            dlen++;
            err -= dy;
            if (err < 0) {
                err += dx;
                if (dlen == 1) {
                    draw_pixel(self, xs, y0, color, alpha);
                } else {
                    fast_hline(self, xs, y0, dlen, color, alpha);
                }
                dlen = 0;
                y0 += ystep;
                xs = x0 + 1;
            }
        }
        if (dlen) {
            fast_hline(self, xs, y0, dlen, color, alpha);
        }
    }
}

///
/// .line(x0, y0, x1, y1{, color, alpha})
/// Draw a line from (x0, y0) to (x1, y1).
/// required parameters:
/// -- x0: x coordinate of the start of the line
/// -- y0: y coordinate of the start of the line
/// -- x1: x coordinate of the end of the line
/// -- y1: y coordinate of the end of the line
/// optional parameters:
/// -- color defaults to WHITE
/// -- alpha defaults to 255
///

STATIC mp_obj_t s3lcd_line(size_t n_args, const mp_obj_t *args) {
    s3lcd_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t x0 = mp_obj_get_int(args[1]);
    mp_int_t y0 = mp_obj_get_int(args[2]);
    mp_int_t x1 = mp_obj_get_int(args[3]);
    mp_int_t y1 = mp_obj_get_int(args[4]);
    OPTIONAL_ARG(5, mp_int_t, mp_obj_get_int, color, WHITE)
    OPTIONAL_ARG(6, mp_int_t, mp_obj_get_int, alpha, 255)

    line(self, x0, y0, x1, y1, color, alpha);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(s3lcd_line_obj, 6, 7, s3lcd_line);

///
/// .blit_buffer(buffer, x, y, width, height {,alpha})
/// Draw a buffer to the screen.
/// required parameters:
/// -- buffer: a buffer object containing the image data
/// -- x: x coordinate of the top left corner of the image
/// -- y: y coordinate of the top left corner of the image
/// -- width: width of the image
/// -- height: height of the image
/// optional parameters:
/// -- alpha defaults to 255
///

STATIC mp_obj_t s3lcd_blit_buffer(size_t n_args, const mp_obj_t *args) {
    s3lcd_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_buffer_info_t buf_info;
    mp_get_buffer_raise(args[1], &buf_info, MP_BUFFER_READ);
    mp_int_t x = mp_obj_get_int(args[2]);
    mp_int_t y = mp_obj_get_int(args[3]);
    mp_int_t w = mp_obj_get_int(args[4]);
    mp_int_t h = mp_obj_get_int(args[5]);
    OPTIONAL_ARG(6, mp_int_t, mp_obj_get_int, alpha, 255)

    uint16_t *src = buf_info.buf;
    uint16_t stride = self->width - w;
    uint16_t *dst = (uint16_t *) self->frame_buffer + (y * self->width + x);

    for (int yy = 0; yy < h; yy++) {
        for (int xx = 0; xx < w; xx++) {
            if (xx+x < 0 || xx+x >= self->width || yy+y < 0 || yy+y >= self->height) {
                src++;
                dst++;
            }
            else {
                *dst = alpha_blend_565(*src, *dst, alpha);
                src++;
                dst++;
            }
        }
        dst += stride;
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(s3lcd_blit_buffer_obj, 6, 7, s3lcd_blit_buffer);

///
/// .draw(font, string|int, x, y, {color , scale, alpha})
/// Draw a string or a single character.
/// required parameters:
/// -- font: a font module
/// -- string: a string or a single character
/// -- x: x coordinate of the top left corner of the string
/// -- y: y coordinate of the top left corner of the string
/// optional parameters:
/// -- color defaults to WHITE
/// -- scale defaults to 1
/// -- alpha defaults to 255
///

STATIC mp_obj_t s3lcd_draw(size_t n_args, const mp_obj_t *args) {
    s3lcd_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    char single_char_s[] = {0, 0};
    const char *s;

    mp_obj_module_t *hershey = MP_OBJ_TO_PTR(args[1]);

    if (mp_obj_is_int(args[2])) {
        mp_int_t c = mp_obj_get_int(args[2]);
        single_char_s[0] = c & 0xff;
        s = single_char_s;
    } else {
        s = mp_obj_str_get_str(args[2]);
    }

    mp_int_t x = mp_obj_get_int(args[3]);
    mp_int_t y = mp_obj_get_int(args[4]);
    OPTIONAL_ARG(5, mp_int_t, mp_obj_get_int, color, WHITE)

    mp_float_t scale = 1.0;
    if (n_args > 6) {
        if (mp_obj_is_float(args[6])) {
            scale = mp_obj_float_get(args[6]);
        }
        if (mp_obj_is_int(args[6])) {
            scale = (mp_float_t)mp_obj_get_int(args[6]);
        }
    }

    OPTIONAL_ARG(7, mp_int_t, mp_obj_get_int, alpha, 255)

    mp_obj_dict_t *dict = MP_OBJ_TO_PTR(hershey->globals);
    mp_obj_t *index_data_buff = mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_INDEX));
    mp_buffer_info_t index_bufinfo;
    mp_get_buffer_raise(index_data_buff, &index_bufinfo, MP_BUFFER_READ);
    uint8_t *index = index_bufinfo.buf;

    mp_obj_t *font_data_buff = mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_FONT));
    mp_buffer_info_t font_bufinfo;
    mp_get_buffer_raise(font_data_buff, &font_bufinfo, MP_BUFFER_READ);
    int8_t *font = font_bufinfo.buf;

    int16_t from_x = x;
    int16_t from_y = y;
    int16_t to_x = x;
    int16_t to_y = y;
    int16_t pos_x = x;
    int16_t pos_y = y;
    bool penup = true;
    char c;
    int16_t ii;

    while ((c = *s++)) {
        if (c >= 32 && c <= 127) {
            ii = (c - 32) * 2;

            int16_t offset = index[ii] | (index[ii + 1] << 8);
            int16_t length = font[offset++];
            int16_t left = (int)(scale * (font[offset++] - 0x52) + 0.5);
            int16_t right = (int)(scale * (font[offset++] - 0x52) + 0.5);
            int16_t width = right - left;

            if (length) {
                int16_t i;
                for (i = 0; i < length; i++) {
                    if (font[offset] == ' ') {
                        offset += 2;
                        penup = true;
                        continue;
                    }

                    int16_t vector_x = (int)(scale * (font[offset++] - 0x52) + 0.5);
                    int16_t vector_y = (int)(scale * (font[offset++] - 0x52) + 0.5);

                    if (!i || penup) {
                        from_x = pos_x + vector_x - left;
                        from_y = pos_y + vector_y;
                    } else {
                        to_x = pos_x + vector_x - left;
                        to_y = pos_y + vector_y;

                        line(self, from_x, from_y, to_x, to_y, color, alpha);
                        from_x = to_x;
                        from_y = to_y;
                    }
                    penup = false;
                }
            }
            pos_x += width;
        }
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(s3lcd_draw_obj, 5, 8, s3lcd_draw);

///
/// .draw_len(font, string|int {, scale})
/// Returns the width of the string in pixels if drawn with the given font and scale (default 1).
/// required parameters:
/// -- font: a font module
/// -- string: a string or a single character
/// optional parameters:
/// -- scale: scale of the string (default 1)
///

STATIC mp_obj_t s3lcd_draw_len(size_t n_args, const mp_obj_t *args) {
    char single_char_s[] = {0, 0};
    const char *s;

    mp_obj_module_t *hershey = MP_OBJ_TO_PTR(args[1]);

    if (mp_obj_is_int(args[2])) {
        mp_int_t c = mp_obj_get_int(args[2]);
        single_char_s[0] = c & 0xff;
        s = single_char_s;
    } else {
        s = mp_obj_str_get_str(args[2]);
    }

    mp_float_t scale = 1.0;
    if (n_args > 3) {
        if (mp_obj_is_float(args[3])) {
            scale = mp_obj_float_get(args[3]);
        }
        if (mp_obj_is_int(args[3])) {
            scale = (mp_float_t)mp_obj_get_int(args[3]);
        }
    }

    mp_obj_dict_t *dict = MP_OBJ_TO_PTR(hershey->globals);
    mp_obj_t *index_data_buff = mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_INDEX));
    mp_buffer_info_t index_bufinfo;
    mp_get_buffer_raise(index_data_buff, &index_bufinfo, MP_BUFFER_READ);
    uint8_t *index = index_bufinfo.buf;

    mp_obj_t *font_data_buff = mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_FONT));
    mp_buffer_info_t font_bufinfo;
    mp_get_buffer_raise(font_data_buff, &font_bufinfo, MP_BUFFER_READ);
    int8_t *font = font_bufinfo.buf;

    int16_t print_width = 0;
    char c;
    int16_t ii;

    while ((c = *s++)) {
        if (c >= 32 && c <= 127) {
            ii = (c - 32) * 2;

            int16_t offset = (index[ii] | (index[ii + 1] << 8)) + 1;
            int16_t left = font[offset++] - 0x52;
            int16_t right = font[offset++] - 0x52;
            int16_t width = right - left;
            print_width += width;
        }
    }
    return mp_obj_new_int((int)(print_width * scale + 0.5));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(s3lcd_draw_len_obj, 3, 4, s3lcd_draw_len);

STATIC uint32_t bs_bit = 0;
uint8_t *bitmap_data = NULL;

STATIC uint8_t get_color(uint8_t bpp) {
    uint8_t color = 0;
    int i;

    for (i = 0; i < bpp; i++) {
        color <<= 1;
        color |= (bitmap_data[bs_bit / 8] & 1 << (7 - (bs_bit % 8))) > 0;
        bs_bit++;
    }
    return color;
}

STATIC mp_obj_t dict_lookup(mp_obj_t self_in, mp_obj_t index) {
    mp_obj_dict_t *self = MP_OBJ_TO_PTR(self_in);
    mp_map_elem_t *elem = mp_map_lookup(&self->map, index, MP_MAP_LOOKUP);
    if (elem) {
        return elem->value;
    }
    return mp_const_none;
}

///
/// .write_len(font, string)
/// return the width in pixels of the string or character if written with a font.
/// required parameters:
/// -- font: a font module
/// -- string: a string or a single character
///

STATIC mp_obj_t s3lcd_write_len(size_t n_args, const mp_obj_t *args) {
    mp_obj_module_t *font = MP_OBJ_TO_PTR(args[1]);
    mp_obj_dict_t *dict = MP_OBJ_TO_PTR(font->globals);
    mp_obj_t widths_data_buff = mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_WIDTHS));
    mp_buffer_info_t widths_bufinfo;
    mp_get_buffer_raise(widths_data_buff, &widths_bufinfo, MP_BUFFER_READ);
    const uint8_t *widths_data = widths_bufinfo.buf;

    uint16_t print_width = 0;

    mp_obj_t map_obj = mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_MAP));
    GET_STR_DATA_LEN(map_obj, map_data, map_len);
    GET_STR_DATA_LEN(args[2], str_data, str_len);
    const byte *s = str_data, *top = str_data + str_len;

    while (s < top) {
        unichar ch;
        ch = utf8_get_char(s);
        s = utf8_next_char(s);

        const byte *map_s = map_data, *map_top = map_data + map_len;
        uint16_t char_index = 0;

        while (map_s < map_top) {
            unichar map_ch;
            map_ch = utf8_get_char(map_s);
            map_s = utf8_next_char(map_s);

            if (ch == map_ch) {
                print_width += widths_data[char_index];
                break;
            }
            char_index++;
        }
    }
    return mp_obj_new_int(print_width);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(s3lcd_write_len_obj, 3, 3, s3lcd_write_len);

///
///	.write(font, s, x, y {, fg, bg, alpha})
/// write a string or character to the display.
/// required parameters:
/// -- font: a font module
/// -- s: a string or a single character
/// -- x: the x position of the string or character
/// -- y: the y position of the string or character
/// optional parameters:
/// -- fg: the foreground color of the string or character
/// -- bg: the background color of the string or character
/// -- alpha: the alpha value of the string or character
///

STATIC mp_obj_t s3lcd_write(size_t n_args, const mp_obj_t *args) {
    s3lcd_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_obj_module_t *font = MP_OBJ_TO_PTR(args[1]);

    mp_int_t x = mp_obj_get_int(args[3]);
    mp_int_t y = mp_obj_get_int(args[4]);
    OPTIONAL_ARG(5, mp_int_t, mp_obj_get_int, fg_color, WHITE)
    OPTIONAL_ARG(6, mp_int_t, mp_obj_get_int, bg_color, BLACK)
    OPTIONAL_ARG(7, mp_int_t, mp_obj_get_int, alpha, 255)

    mp_obj_dict_t *dict = MP_OBJ_TO_PTR(font->globals);
    const uint8_t bpp = mp_obj_get_int(mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_BPP)));
    const uint8_t height = mp_obj_get_int(mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_HEIGHT)));
    const uint8_t offset_width = mp_obj_get_int(mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_OFFSET_WIDTH)));

    mp_obj_t widths_data_buff = mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_WIDTHS));
    mp_buffer_info_t widths_bufinfo;
    mp_get_buffer_raise(widths_data_buff, &widths_bufinfo, MP_BUFFER_READ);
    const uint8_t *widths_data = widths_bufinfo.buf;

    mp_obj_t offsets_data_buff = mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_OFFSETS));
    mp_buffer_info_t offsets_bufinfo;
    mp_get_buffer_raise(offsets_data_buff, &offsets_bufinfo, MP_BUFFER_READ);
    const uint8_t *offsets_data = offsets_bufinfo.buf;

    mp_obj_t bitmaps_data_buff = mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_BITMAPS));
    mp_buffer_info_t bitmaps_bufinfo;
    mp_get_buffer_raise(bitmaps_data_buff, &bitmaps_bufinfo, MP_BUFFER_READ);
    bitmap_data = bitmaps_bufinfo.buf;

    uint16_t print_width = 0;
    mp_obj_t map_obj = mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_MAP));
    GET_STR_DATA_LEN(map_obj, map_data, map_len);
    GET_STR_DATA_LEN(args[2], str_data, str_len);
    const byte *s = str_data, *top = str_data + str_len;
    while (s < top) {
        unichar ch;
        ch = utf8_get_char(s);
        s = utf8_next_char(s);

        const byte *map_s = map_data, *map_top = map_data + map_len;
        uint16_t char_index = 0;

        while (map_s < map_top) {
            unichar map_ch;
            map_ch = utf8_get_char(map_s);
            map_s = utf8_next_char(map_s);

            if (ch == map_ch) {
                uint8_t width = widths_data[char_index];

                bs_bit = 0;
                switch (offset_width) {
                    case 1:
                        bs_bit = offsets_data[char_index * offset_width];
                        break;

                    case 2:
                        bs_bit = (offsets_data[char_index * offset_width] << 8) +
                            (offsets_data[char_index * offset_width + 1]);
                        break;

                    case 3:
                        bs_bit = (offsets_data[char_index * offset_width] << 16) +
                            (offsets_data[char_index * offset_width + 1] << 8) +
                            (offsets_data[char_index * offset_width + 2]);
                        break;
                }

                for (int yy = 0; yy < height; yy++) {
                    uint16_t *b = &(self->frame_buffer)[x + ((y + yy) * self->width)];
                    for (int xx = 0; xx < width; xx++) {
                        if (get_color(bpp)) {
                            if (alpha == 255) {
                                *b = fg_color;
                            } else {
                                *b = alpha_blend_565(*b, fg_color, alpha);
                            }
                        } else {
                            if (bg_color != -1) {
                                if (alpha == 255) {
                                    *b = bg_color;
                                } else {
                                    *b = alpha_blend_565(*b, bg_color, alpha);
                                }
                            }
                        }
                        b++;
                    }
                }

                x += width;
                print_width += width;
                break;
            }
            char_index++;
        }
    }
    return mp_obj_new_int(print_width);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(s3lcd_write_obj, 5, 8, s3lcd_write);

///
/// .bitmap(bitmap, x, y {, alpha})
/// required parameters:
/// -- bitmap: a tuple of (width, height, data)
/// -- x: x position
/// -- y: y position
/// optional parameters:
/// -- alpha: alpha value (0-255)
///

STATIC mp_obj_t s3lcd_bitmap_from_tuple(size_t n_args, const mp_obj_t *args) {
    s3lcd_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_obj_t *bitmap_tuple = NULL;
    size_t bitmap_tuple_len = 0;
    mp_obj_tuple_get(args[1], &bitmap_tuple_len, &bitmap_tuple);
    if (bitmap_tuple_len != 3) {
        mp_raise_ValueError(MP_ERROR_TEXT("bitmap tuple must have 3 elements"));
    }

    mp_int_t x = mp_obj_get_int(args[2]);
    mp_int_t y = mp_obj_get_int(args[3]);

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(bitmap_tuple[0], &bufinfo, MP_BUFFER_READ);

    mp_int_t width = mp_obj_get_int(bitmap_tuple[1]);
    mp_int_t height = mp_obj_get_int(bitmap_tuple[2]);
    OPTIONAL_ARG(4, mp_int_t, mp_obj_get_int, alpha, 255)

    if (bufinfo.len < width * height * 2) {
        mp_raise_ValueError(MP_ERROR_TEXT("bitmap size too small for width and height"));
    }
    uint16_t *s = (uint16_t *)bufinfo.buf;
    uint16_t *d = self->frame_buffer + (y * self->width) + x;
    if (alpha == 255) {
        COPY_TO_BUFFER(self, s, d, width, height)
    } else {
        BLEND_TO_BUFFER(self, s, d, width, height, alpha)
    }
    return mp_const_none;
}

///
/// .bitmap(bitmap, x, y {, alpha, index})
/// bitmap: a bitmap module created by imgtobitmap.py utility
/// required parameters:
/// -- x: x position
/// -- y: y position
/// optional parameters:
/// -- index: index of the character in the map
/// -- alpha: alpha value (0-255)
///

STATIC mp_obj_t s3lcd_bitmap_from_module(size_t n_args, const mp_obj_t *args) {
    s3lcd_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_obj_module_t *bitmap = MP_OBJ_TO_PTR(args[1]);
    mp_int_t x = mp_obj_get_int(args[2]);
    mp_int_t y = mp_obj_get_int(args[3]);
    OPTIONAL_ARG(4, mp_int_t, mp_obj_get_int, idx, 0)
    OPTIONAL_ARG(5, mp_int_t, mp_obj_get_int, alpha, 255)

    mp_obj_dict_t *dict = MP_OBJ_TO_PTR(bitmap->globals);
    const uint16_t height = mp_obj_get_int(mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_HEIGHT)));
    const uint16_t width = mp_obj_get_int(mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_WIDTH)));
    uint16_t bitmaps = 0;
    const uint8_t bpp = mp_obj_get_int(mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_BPP)));
    mp_obj_t *palette_arg = mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_PALETTE));
    mp_obj_t *palette = NULL;
    size_t palette_len = 0;

    mp_map_elem_t *elem = dict_lookup(bitmap->globals, MP_OBJ_NEW_QSTR(MP_QSTR_BITMAPS));
    if (elem) {
        bitmaps = mp_obj_get_int(elem);
    }

    mp_obj_get_array(palette_arg, &palette_len, &palette);

    mp_obj_t *bitmap_data_buff = mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_BITMAP));
    mp_buffer_info_t bufinfo;

    mp_get_buffer_raise(bitmap_data_buff, &bufinfo, MP_BUFFER_READ);
    bitmap_data = bufinfo.buf;

    bs_bit = 0;
    if (bitmaps) {
        if (idx < bitmaps) {
            bs_bit = height * width * bpp * idx;
        } else {
            mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("index out of range"));
        }
    }

    for (int yy = 0; yy < height; yy++) {
        uint16_t *b = self->frame_buffer + (yy + y) * self->width + x;
        for (int xx = 0; xx < width; xx++) {
            int color_idx = get_color(bpp);
            uint16_t color = mp_obj_get_int(palette[color_idx]);
            if (alpha != 255) {
                color = alpha_blend_565(color, *b, alpha);
            }
            *b++ = _swap_bytes(color);
        }
    }
    return mp_const_none;
}

STATIC mp_obj_t s3lcd_bitmap(size_t n_args, const mp_obj_t *args) {

    if (mp_obj_is_type(args[1], &mp_type_tuple)) {
        return s3lcd_bitmap_from_tuple(n_args, args);
    } else {
        if (mp_obj_is_type(args[1], &mp_type_module)) {
            return s3lcd_bitmap_from_module(n_args, args);
        }
    }

    mp_raise_TypeError(MP_ERROR_TEXT("bitmap requires either module or tuple."));
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(s3lcd_bitmap_obj, 4, 6, s3lcd_bitmap);

///
/// .text(font, x, y, text {, color, background, alpha})
/// Draw text on the screen using converted font module
/// required parameters:
/// -- font: a font module created by font2bitmap.py utility
/// -- x: x position
/// -- y: y position
/// -- text: text to display
/// optional parameters:
/// -- color: text color
/// -- background: background color
/// -- alpha: alpha value (0-255)
///

STATIC mp_obj_t s3lcd_text(size_t n_args, const mp_obj_t *args) {
    uint8_t single_char_s;
    const uint8_t *source = NULL;
    size_t source_len = 0;

    // extract arguments
    s3lcd_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_obj_module_t *font = MP_OBJ_TO_PTR(args[1]);

    if (mp_obj_is_int(args[2])) {
        mp_int_t c = mp_obj_get_int(args[2]);
        single_char_s = (c & 0xff);
        source = &single_char_s;
        source_len = 1;
    } else if (mp_obj_is_str(args[2])) {
        source = (uint8_t *) mp_obj_str_get_str(args[2]);
        source_len = strlen((char *)source);
    } else if (mp_obj_is_type(args[2], &mp_type_bytes)) {
        mp_obj_t text_data_buff = args[2];
        mp_buffer_info_t text_bufinfo;
        mp_get_buffer_raise(text_data_buff, &text_bufinfo, MP_BUFFER_READ);
        source = text_bufinfo.buf;
        source_len = text_bufinfo.len;
    } else {
        mp_raise_TypeError(MP_ERROR_TEXT("text requires either int, str or bytes."));
        return mp_const_none;
    }

    mp_int_t x0 = mp_obj_get_int(args[3]);
    mp_int_t y0 = mp_obj_get_int(args[4]);

    mp_obj_dict_t *dict = MP_OBJ_TO_PTR(font->globals);
    const uint8_t width = mp_obj_get_int(mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_WIDTH)));
    const uint8_t height = mp_obj_get_int(mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_HEIGHT)));
    const uint8_t first = mp_obj_get_int(mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_FIRST)));
    const uint8_t last = mp_obj_get_int(mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_LAST)));

    mp_obj_t font_data_buff = mp_obj_dict_get(dict, MP_OBJ_NEW_QSTR(MP_QSTR_FONT));
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(font_data_buff, &bufinfo, MP_BUFFER_READ);
    const uint8_t *font_data = bufinfo.buf;

    OPTIONAL_ARG(5, mp_int_t, mp_obj_get_int, fg_color, WHITE)
    OPTIONAL_ARG(6, mp_int_t, mp_obj_get_int, bg_color, BLACK)
    OPTIONAL_ARG(7, mp_int_t, mp_obj_get_int, alpha, 255)

    uint8_t wide = width / 8;

    uint8_t chr;
    while (source_len--) {
        chr = *source++;
        if (chr >= first && chr <= last) {
            uint16_t buf_idx = 0;
            uint16_t chr_idx = (chr - first) * (height * wide);
            for (uint8_t line = 0; line < height; line++) {
                uint16_t *b = self->frame_buffer + x0 + (y0 + line) * self->width;
                for (uint8_t line_byte = 0; line_byte < wide; line_byte++) {
                    uint8_t chr_data = font_data[chr_idx];
                    for (uint8_t bit = 8; bit; bit--) {
                        if (chr_data >> (bit - 1) & 1) {
                            if (fg_color != -1) {
                                if (alpha == 255) {
                                    *b = fg_color;
                                } else {
                                    *b = alpha_blend_565(*b, fg_color, alpha);
                                }
                            }
                        } else {
                            if (bg_color != -1) {
                                if (alpha == 255) {
                                    *b = bg_color;
                                } else {
                                    *b = alpha_blend_565(*b, bg_color, alpha);
                                }
                            }
                        }
                        b++;
                        buf_idx++;
                    }
                    chr_idx++;
                }
            }
            x0 += width;
        }
    }
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(s3lcd_text_obj, 5, 8, s3lcd_text);

STATIC void set_rotation(s3lcd_obj_t *self) {

    s3lcd_rotation_t *rotation = &self->rotations[self->rotation % self->rotations_len];
    esp_lcd_panel_swap_xy(self->panel_handle, rotation->swap_xy);
    esp_lcd_panel_mirror(self->panel_handle, rotation->mirror_x, rotation->mirror_y);
    esp_lcd_panel_set_gap(self->panel_handle, rotation->x_gap, rotation->y_gap);

    self->width = rotation->width;
    self->height = rotation->height;
}

///
/// .rotation(rotation)
/// Set the display rotation.
/// required parameters:
/// -- rotation: 0=Portrait, 1=Landscape, 2=Reverse Portrait (180), 3=Reverse Landscape (180)
///

STATIC mp_obj_t s3lcd_rotation(mp_obj_t self_in, mp_obj_t value) {
    s3lcd_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_int_t rotation = mp_obj_get_int(value) % 4;

    self->rotation = rotation;
    set_rotation(self);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(s3lcd_rotation_obj, s3lcd_rotation);

///
/// .width()
/// Returns the width of the display in pixels.
///

STATIC mp_obj_t s3lcd_width(mp_obj_t self_in) {
    s3lcd_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int(self->width);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(s3lcd_width_obj, s3lcd_width);

///
/// .height()
/// Returns the height of the display in pixels.
///

STATIC mp_obj_t s3lcd_height(mp_obj_t self_in) {
    s3lcd_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int(self->height);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(s3lcd_height_obj, s3lcd_height);

///
/// .vscrdef(tfa, vsa, bfa)
/// Set the vertical scrolling definition.
/// required parameters:
/// -- tfa: Top Fixed Area line
/// -- vsa: Vertical Scrolling Area line
/// -- bfa: Bottom Fixed Area line
///

STATIC mp_obj_t s3lcd_vscrdef(size_t n_args, const mp_obj_t *args) {
    s3lcd_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t tfa = mp_obj_get_int(args[1]);
    mp_int_t vsa = mp_obj_get_int(args[2]);
    mp_int_t bfa = mp_obj_get_int(args[3]);

    uint8_t buf[6] = {(tfa) >> 8, (tfa) & 0xFF, (vsa) >> 8, (vsa) & 0xFF, (bfa) >> 8, (bfa) & 0xFF};
    esp_lcd_panel_io_tx_param(self->io_handle, LCD_CMD_VSCRDEF, buf, 6);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(s3lcd_vscrdef_obj, 4, 4, s3lcd_vscrdef);

///
/// .vscsad(vssa)
/// Set the vertical scrolling start address.
/// required parameters:
/// -- vssa: Vertical Scrolling Start Address
///

STATIC mp_obj_t s3lcd_vscsad(mp_obj_t self_in, mp_obj_t vssa_in) {
    s3lcd_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_int_t vssa = mp_obj_get_int(vssa_in);

    uint8_t buf[2] = {(vssa) >> 8, (vssa) & 0xFF};
    esp_lcd_panel_io_tx_param(self->io_handle, ST7796_VSCSAD, buf, 2);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(s3lcd_vscsad_obj, s3lcd_vscsad);

STATIC bool lcd_panel_done(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx) {
    lcd_panel_active = false;
    return false;
}

///
/// .scroll(xstep, ystep{, fill=0})
/// Scroll the framebuffer in the given direction.
/// required parameters:
/// -- xstep: Number of pixels to scroll in the x direction. Negative values scroll left, positive values scroll right.
/// -- ystep: Number of pixels to scroll in the y direction. Negative values scroll up, positive values scroll down.
/// optional parameters:
/// -- fill: Fill color for the new pixels.
///
/// Based on the MicroPython framedbuffer scroll function.

STATIC mp_obj_t s3lcd_scroll(size_t n_args, const mp_obj_t *args) {
    s3lcd_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t xstep = mp_obj_get_int(args[1]);
    mp_int_t ystep = mp_obj_get_int(args[2]);
    OPTIONAL_ARG(3, mp_int_t, mp_obj_get_int, fill, 0)

    int sx, y, xend, yend, dx, dy;
    if (xstep < 0) {
        sx = 0;
        xend = self->width + xstep;
        if (xend <= 0) {
            return mp_const_none;
        }
        dx = 1;
    } else {
        sx = self->width - 1;
        xend = xstep - 1;
        if (xend >= sx) {
            return mp_const_none;
        }
        dx = -1;
    }
    if (ystep < 0) {
        y = 0;
        yend = self->height + ystep;
        if (yend <= 0) {
            return mp_const_none;
        }
        dy = 1;
    } else {
        y = self->height - 1;
        yend = ystep - 1;
        if (yend >= y) {
            return mp_const_none;
        }
        dy = -1;
    }

    for (; y != yend; y += dy) {
        for (int x = sx; x != xend; x += dx) {
            int src_x = x - xstep;
            int src_y = y - ystep;
            if (src_x >= 0 && src_x < self->width && src_y >= 0 && src_y < self->height) {
                self->frame_buffer[y * self->width + x] = self->frame_buffer[src_y * self->width + src_x];
            } else {
                self->frame_buffer[y * self->width + x] = fill;
            }
        }
    }

    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(s3lcd_scroll_obj, 3, 4, s3lcd_scroll);

//
// run custom_init
//

STATIC void custom_init(s3lcd_obj_t *self) {
    size_t init_len;
    mp_obj_t *init_list;

    mp_obj_get_array(self->custom_init, &init_len, &init_list);
    for (int idx = 0; idx < init_len; idx++) {
        size_t init_cmd_len;
        mp_obj_t *init_cmd;
        mp_obj_get_array(init_list[idx], &init_cmd_len, &init_cmd);
        mp_buffer_info_t init_cmd_data_info;
        if (mp_get_buffer(init_cmd[0], &init_cmd_data_info, MP_BUFFER_READ)) {
            uint8_t *init_cmd_data = (uint8_t *)init_cmd_data_info.buf;
            if (init_cmd_data_info.len > 1) {
                esp_lcd_panel_io_tx_param(self->io_handle, init_cmd_data[0], &init_cmd_data[1], init_cmd_data_info.len - 1);
            } else {
                esp_lcd_panel_io_tx_param(self->io_handle, init_cmd_data[0], NULL, 0);
            }
            mp_hal_delay_ms(10);
            // check for delay
            if (init_cmd_len > 1) {
                mp_int_t delay = mp_obj_get_int(init_cmd[1]);
                if (delay > 0) {
                    mp_hal_delay_ms(delay);
                }
            }
        }
    }
}


///
/// .init()
/// Initialize the display, This method must be called before any other methods.
///

STATIC mp_obj_t s3lcd_init(mp_obj_t self_in) {
    s3lcd_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (mp_obj_is_type(self->bus, &s3lcd_i80_bus_type)) {
        ESP_LOGI(TAG, "Initialize Intel 8080 bus");
        s3lcd_i80_bus_obj_t *config = MP_OBJ_TO_PTR(self->bus);
        self->swap_color_bytes = false;
        self->bus_handle.i80 = NULL;
        esp_lcd_i80_bus_config_t bus_config = {
            .dc_gpio_num = config->dc_gpio_num,
            .wr_gpio_num = config->wr_gpio_num,
            .clk_src = LCD_CLK_SRC_DEFAULT,
            .data_gpio_nums = {
                config->data_gpio_nums[0],
                config->data_gpio_nums[1],
                config->data_gpio_nums[2],
                config->data_gpio_nums[3],
                config->data_gpio_nums[4],
                config->data_gpio_nums[5],
                config->data_gpio_nums[6],
                config->data_gpio_nums[7],
            },
            .bus_width = config->bus_width,
            .max_transfer_bytes = self->dma_buffer_size,
        };

        ESP_ERROR_CHECK(esp_lcd_new_i80_bus(&bus_config, &self->bus_handle.i80));
        esp_lcd_panel_io_handle_t io_handle = NULL;
        esp_lcd_panel_io_i80_config_t io_config = {
            .cs_gpio_num = config->cs_gpio_num,
            .pclk_hz = config->pclk_hz,
            .trans_queue_depth = 10,
            .on_color_trans_done = lcd_panel_done,
            .user_ctx = self,
            .dc_levels = {
                .dc_idle_level = config->dc_levels.dc_idle_level,
                .dc_cmd_level = config->dc_levels.dc_cmd_level,
                .dc_dummy_level = config->dc_levels.dc_dummy_level,
                .dc_data_level = config->dc_levels.dc_data_level,
            },
            .lcd_cmd_bits = config->lcd_cmd_bits,
            .lcd_param_bits = config->lcd_param_bits,
            .flags = {
                .cs_active_high = config->flags.cs_active_high,
                .reverse_color_bits = config->flags.reverse_color_bits,
                .swap_color_bytes = config->flags.swap_color_bytes,
                .pclk_active_neg = config->flags.pclk_active_neg,
                .pclk_idle_low = config->flags.pclk_idle_low,
            }
        };

        ESP_ERROR_CHECK(esp_lcd_new_panel_io_i80(self->bus_handle.i80, &io_config, &io_handle));
        self->io_handle = io_handle;
    } else if (mp_obj_is_type(self->bus, &s3lcd_spi_bus_type)) {
         s3lcd_spi_bus_obj_t *config = MP_OBJ_TO_PTR(self->bus);
        self->swap_color_bytes = config->flags.swap_color_bytes;
        spi_bus_config_t buscfg = {
            .sclk_io_num = config->sclk_io_num,
            .mosi_io_num = config->mosi_io_num,
            .miso_io_num = -1,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = self->dma_buffer_size
        };
        ESP_ERROR_CHECK(spi_bus_initialize(config->spi_host, &buscfg, SPI_DMA_CH_AUTO));
        esp_lcd_panel_io_handle_t io_handle = NULL;
        esp_lcd_panel_io_spi_config_t io_config = {
            .dc_gpio_num = config->dc_gpio_num,
            .cs_gpio_num = config->cs_gpio_num,
            .pclk_hz = config->pclk_hz,
            .spi_mode = 0,
            .trans_queue_depth = 10,
            .lcd_cmd_bits = config->lcd_cmd_bits,
            .lcd_param_bits = config->lcd_param_bits,
            .on_color_trans_done = lcd_panel_done,
            .user_ctx = self,
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
            .flags.dc_as_cmd_phase = config->flags.dc_as_cmd_phase,
#endif
            .flags.dc_low_on_data = config->flags.dc_low_on_data,
            .flags.octal_mode =config->flags.octal_mode,
            .flags.lsb_first = config->flags.lsb_first
        };

        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)config->spi_host, &io_config, &io_handle));
        self->io_handle = io_handle;
    }

    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = self->rst,
        .color_space = self->color_space,
        .bits_per_pixel = 16,
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(self->io_handle, &panel_config, &panel_handle));
    self->panel_handle = panel_handle;

    esp_lcd_panel_reset(panel_handle);
    if (self->custom_init == MP_OBJ_NULL) {
        esp_lcd_panel_init(panel_handle);
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
        esp_lcd_panel_disp_on_off(panel_handle,true); //switch lcd on, not longer a part of init
#endif
    } else {
        custom_init(self);
    }
    esp_lcd_panel_invert_color(panel_handle, self->inversion_mode);
    set_rotation(self);

    self->frame_buffer = m_malloc(self->frame_buffer_size);
    memset(self->frame_buffer, 0, self->frame_buffer_size);

    // esp_lcd_panel_io_tx_param(self->io_handle, 0x13, NULL, 0);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(s3lcd_init_obj, s3lcd_init);

///
/// .hline(x, y, w {, color, alpha})
/// Draw a horizontal line.
/// required parameters:
/// -- x: x coordinate
/// -- y: y coordinate
/// -- w: width
/// optional parameters:
/// -- color: color
/// -- alpha: alpha
///

STATIC mp_obj_t s3lcd_hline(size_t n_args, const mp_obj_t *args) {
    s3lcd_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t x = mp_obj_get_int(args[1]);
    mp_int_t y = mp_obj_get_int(args[2]);
    mp_int_t w = mp_obj_get_int(args[3]);
    OPTIONAL_ARG(4, mp_int_t, mp_obj_get_int, color, WHITE)
    OPTIONAL_ARG(5, mp_int_t, mp_obj_get_int, alpha, 255)

    fast_hline(self, x, y, w, color, alpha);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(s3lcd_hline_obj, 4, 6, s3lcd_hline);

///
/// .vline(x, y, w {, color, alpha})
/// Draw a vertical line.
/// required parameters:
/// -- x: x coordinate
/// -- y: y coordinate
/// -- w: width
/// optional parameters:
/// -- color: color
/// -- alpha: alpha
///

STATIC mp_obj_t s3lcd_vline(size_t n_args, const mp_obj_t *args) {
    s3lcd_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t x = mp_obj_get_int(args[1]);
    mp_int_t y = mp_obj_get_int(args[2]);
    mp_int_t w = mp_obj_get_int(args[3]);
    OPTIONAL_ARG(4, mp_int_t, mp_obj_get_int, color, WHITE)
    OPTIONAL_ARG(5, mp_int_t, mp_obj_get_int, alpha, 255)

    fast_vline(self, x, y, w, color, alpha);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(s3lcd_vline_obj, 4, 6, s3lcd_vline);

// Circle/Fill_Circle by https://github.com/c-logic
// https://github.com/russhughes/s3lcd_mpy/pull/46
// https://github.com/c-logic/s3lcd_mpy.git patch-1

///
/// .circle(xm, ym, r {,color, alpha}])
/// Draw a circle.
/// required parameters:
/// -- xm: x coordinate
/// -- ym: y coordinate
/// -- r: radius
/// optional parameters:
/// -- color: color
/// -- alpha: alpha
///

STATIC mp_obj_t s3lcd_circle(size_t n_args, const mp_obj_t *args) {
    s3lcd_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t xm = mp_obj_get_int(args[1]);
    mp_int_t ym = mp_obj_get_int(args[2]);
    mp_int_t r = mp_obj_get_int(args[3]);
    OPTIONAL_ARG(4, mp_int_t, mp_obj_get_int, color, WHITE)
    OPTIONAL_ARG(5, mp_int_t, mp_obj_get_int, alpha, 255)

    mp_int_t f = 1 - r;
    mp_int_t ddF_x = 1;
    mp_int_t ddF_y = -2 * r;
    mp_int_t x = 0;
    mp_int_t y = r;

    draw_pixel(self, xm, ym + r, color, alpha);
    draw_pixel(self, xm, ym - r, color, alpha);
    draw_pixel(self, xm + r, ym, color, alpha);
    draw_pixel(self, xm - r, ym, color, alpha);
    while (x < y) {
        if (f >= 0) {
            y -= 1;
            ddF_y += 2;
            f += ddF_y;
        }
        x += 1;
        ddF_x += 2;
        f += ddF_x;
        draw_pixel(self, xm + x, ym + y, color, alpha);
        draw_pixel(self, xm - x, ym + y, color, alpha);
        draw_pixel(self, xm + x, ym - y, color, alpha);
        draw_pixel(self, xm - x, ym - y, color, alpha);
        draw_pixel(self, xm + y, ym + x, color, alpha);
        draw_pixel(self, xm - y, ym + x, color, alpha);
        draw_pixel(self, xm + y, ym - x, color, alpha);
        draw_pixel(self, xm - y, ym - x, color, alpha);
    }
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(s3lcd_circle_obj, 4, 6, s3lcd_circle);

// Circle/Fill_Circle by https://github.com/c-logic
// https://github.com/russhughes/s3lcd_mpy/pull/46
// https://github.com/c-logic/s3lcd_mpy.git patch-1

///
/// .fill_circle(xm, ym, r {,color, alpha})
/// Draw a filled circle.
/// required parameters:
/// -- xm: x coordinate
/// -- ym: y coordinate
/// -- r: radius
/// optional parameters:
/// -- color: color
/// -- alpha: alpha
///

STATIC mp_obj_t s3lcd_fill_circle(size_t n_args, const mp_obj_t *args) {
    s3lcd_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t xm = mp_obj_get_int(args[1]);
    mp_int_t ym = mp_obj_get_int(args[2]);
    mp_int_t r = mp_obj_get_int(args[3]);
    OPTIONAL_ARG(4, mp_int_t, mp_obj_get_int, color, WHITE)
    OPTIONAL_ARG(5, mp_int_t, mp_obj_get_int, alpha, 255)

    mp_int_t f = 1 - r;
    mp_int_t ddF_x = 1;
    mp_int_t ddF_y = -2 * r;
    mp_int_t x = 0;
    mp_int_t y = r;

    fast_vline(self, xm, ym - y, 2 * y + 1, color, alpha);

    while (x < y) {
        if (f >= 0) {
            y -= 1;
            ddF_y += 2;
            f += ddF_y;
        }
        x += 1;
        ddF_x += 2;
        f += ddF_x;
        fast_vline(self, xm + x, ym - y, 2 * y + 1, color, alpha);
        fast_vline(self, xm + y, ym - x, 2 * x + 1, color, alpha);
        fast_vline(self, xm - x, ym - y, 2 * y + 1, color, alpha);
        fast_vline(self, xm - y, ym - x, 2 * x + 1, color, alpha);
    }
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(s3lcd_fill_circle_obj, 4, 6, s3lcd_fill_circle);

///
/// .rect(x, y, w, h {, color, alpha})
/// Draw a rectangle.
/// required parameters:
/// -- x: x coordinate
/// -- y: y coordinate
/// -- w: width
/// -- h: height
/// optional parameters:
/// -- color: color
/// -- alpha: alpha
///

STATIC mp_obj_t s3lcd_rect(size_t n_args, const mp_obj_t *args) {
    s3lcd_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t x = mp_obj_get_int(args[1]);
    mp_int_t y = mp_obj_get_int(args[2]);
    mp_int_t w = mp_obj_get_int(args[3]);
    mp_int_t h = mp_obj_get_int(args[4]);
    OPTIONAL_ARG(5, mp_int_t, mp_obj_get_int, color, WHITE)
    OPTIONAL_ARG(6, mp_int_t, mp_obj_get_int, alpha, 255)

    fast_hline(self, x, y, w, color, alpha);
    fast_vline(self, x, y, h, color, alpha);
    fast_hline(self, x, y + h - 1, w, color, alpha);
    fast_vline(self, x + w - 1, y, h, color, alpha);
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(s3lcd_rect_obj, 5, 7, s3lcd_rect);

///
/// .color565(r, g, b)
/// Convert RGB888 to RGB565.
/// required parameters:
/// -- r: red
/// -- g: green
/// -- b: blue
///

STATIC mp_obj_t s3lcd_color565(mp_obj_t r, mp_obj_t g, mp_obj_t b) {
    return MP_OBJ_NEW_SMALL_INT(color565(
        (uint8_t)mp_obj_get_int(r),
        (uint8_t)mp_obj_get_int(g),
        (uint8_t)mp_obj_get_int(b)));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(s3lcd_color565_obj, s3lcd_color565);

STATIC void map_bitarray_to_rgb565(uint8_t const *bitarray, uint16_t *buffer, int length, int width,
    uint16_t color, uint16_t bg_color) {
    int row_pos = 0;
    for (int i = 0; i < length; i++) {
        uint8_t byte = bitarray[i];
        for (int bi = 7; bi >= 0; bi--) {
            uint8_t b = byte & (1 << bi);
            uint16_t cur_color = b ? color : bg_color;
            *buffer++ = cur_color;
            row_pos++;
            if (row_pos >= width) {
                row_pos = 0;
                break;
            }
        }
    }
}

///
/// .map_bitarray_to_rgb565(bitarray, buffer, width {, color, bg_color})
/// Map a bitarray to a buffer of RGB565 pixels.
/// required parameters:
/// -- bitarray: bitarray
/// -- buffer: buffer
/// -- width: width
/// optional parameters:
/// -- color: color
/// -- bg_color: background color
///

STATIC mp_obj_t s3lcd_map_bitarray_to_rgb565(size_t n_args, const mp_obj_t *args) {
    mp_buffer_info_t bitarray_info;
    mp_buffer_info_t buffer_info;

    mp_get_buffer_raise(args[1], &bitarray_info, MP_BUFFER_READ);
    mp_get_buffer_raise(args[2], &buffer_info, MP_BUFFER_WRITE);
    mp_int_t width = mp_obj_get_int(args[3]);
    OPTIONAL_ARG(4, mp_int_t, mp_obj_get_int, color, WHITE)
    OPTIONAL_ARG(5, mp_int_t, mp_obj_get_int, bg_color, BLACK)

    map_bitarray_to_rgb565(bitarray_info.buf, buffer_info.buf, bitarray_info.len, width, color, bg_color);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(s3lcd_map_bitarray_to_rgb565_obj, 3, 7, s3lcd_map_bitarray_to_rgb565);

//
// jpg routines
//

// User defined device identifier
typedef struct {
    mp_file_t *fp;                      // File pointer for input function
    uint8_t *fbuf;                      // Pointer to the frame buffer for output function
    uint16_t wfbuf;                     // Width of the frame buffer [pix]
    uint16_t left;                      // jpg crop left column
    uint16_t top;                       // jpg crop top row
    uint16_t right;                     // jpg crop right column
    uint16_t bottom;                    // jpg crop bottom row

    s3lcd_obj_t *self;                  // display object

    uint8_t *data;                      // Pointer to the input data
    uint32_t dataIdx;                   // Index of the input data
    uint32_t dataLen;                   // Length of the input data

} IODEV;

//
// buffer input function
//

static unsigned int buffer_in_func(     // Returns number of bytes read (zero on error)
    JDEC *jd,                           // Decompression object
    uint8_t *buff,                      // Pointer to the read buffer (null to remove data)
    unsigned int nbyte) {               // Number of bytes to read/remove
    IODEV *dev = (IODEV *)jd->device;

    if (dev->dataIdx + nbyte > dev->dataLen) {
        nbyte = dev->dataLen - dev->dataIdx;
    }

    if (buff) {
        memcpy(buff, (uint8_t *)(dev->data + dev->dataIdx), nbyte);
    }

    dev->dataIdx += nbyte;
    return nbyte;
}


//
// file input function
//

STATIC unsigned int file_in_func(           // Returns number of bytes read (zero on error)
    JDEC *jd,                               // Decompression object
    uint8_t *buff,                          // Pointer to the read buffer (null to remove data)
    unsigned int nbyte) {                   // Number of bytes to read/remove
    IODEV *dev = (IODEV *)jd->device;       // Device identifier for the session (5th argument of jd_prepare function)
    unsigned int nread;

    if (buff) {                             // Read data from input stream
        nread = (unsigned int)mp_readinto(dev->fp, buff, nbyte);
        return nread;
    }

    // Remove data from input stream if buff was NULL
    mp_seek(dev->fp, nbyte, SEEK_CUR);
    return 0;
}

//
// jpg output function
//

STATIC int jpg_out(                                         // 1:Ok, 0:Aborted
    JDEC *jd,                                               // Decompression object
    void *bitmap,                                           // Bitmap data to be output
    JRECT *rect) {                                          // Rectangular region of output image
    IODEV *dev = (IODEV *)jd->device;
    uint16_t wd = dev->wfbuf;
    uint16_t ws = rect->right - rect->left + 1;
    uint16_t stride = wd - ws;

    // Copy the decompressed RGB rectangular to the frame buffer (assuming RGB565)
    uint16_t *src = (uint16_t *) bitmap;
    uint16_t *dst = (uint16_t *) dev->fbuf + ((rect->top + jd->y_offs) * dev->wfbuf + rect->left + jd->x_offs);

    for (int y = rect->top; y <= rect->bottom; y++) {
        for (int x = rect->left; x <= rect->right; x++) {
            if ( x+jd->x_offs < 0 || x+jd->x_offs >= dev->self->width || y+jd->y_offs < 0 || y+jd->y_offs >= dev->self->height) {
                src++;
                dst++;
            }
            else {
                *dst++ = *src++;
            }
        }
        dst += stride;
    }
    return 1;
}

///
/// .jpg(filename, x, y)
/// Draw jpg from a file at x, y
/// required parameters:
/// -- filename: filename
/// -- x: x
/// -- y: y

STATIC mp_obj_t s3lcd_jpg(size_t n_args, const mp_obj_t *args) {
    s3lcd_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t x = mp_obj_get_int(args[2]);
    mp_int_t y = mp_obj_get_int(args[3]);

    JRESULT res;                                    // Result code of TJpgDec API
    JDEC jdec;                                      // Decompression object
    IODEV devid;                                    // User defined device identifier
    self->work = (void *)m_malloc(3100);            // Pointer to the work area
    static unsigned int (*input_func)(JDEC *, uint8_t *, unsigned int) = NULL;

    if (mp_obj_is_type(args[1], &mp_type_bytes)) {
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(args[1], &bufinfo, MP_BUFFER_READ);
        devid.dataIdx = 0;
        devid.data = bufinfo.buf;
        devid.dataLen = bufinfo.len;
        input_func = buffer_in_func;
        self->fp = MP_OBJ_NULL;
    } else {
        const char *filename = mp_obj_str_get_str(args[1]);
        self->fp = mp_open(filename, "rb");
        devid.fp = self->fp;
        input_func = file_in_func;
        devid.data = MP_OBJ_NULL;
        devid.dataLen = 0;
    }

    if (devid.fp || devid.data) {
        jdec.x_offs = x;
        jdec.y_offs = y;
        // Prepare to decompress
        res = jd_prepare(&jdec, input_func, self->work, 3100, &devid);
        if (res == JDR_OK) {
            // Initialize output device
            devid.fbuf = (uint8_t *)self->frame_buffer;
            devid.wfbuf = self->width;
            devid.self = self;
            res = jd_decomp(&jdec, jpg_out, 0);     // Start to decompress with 1/1 scaling
            if (res != JDR_OK) {
                mp_raise_msg_varg(&mp_type_RuntimeError, MP_ERROR_TEXT("jpg decompress failed: %d."), res);
            }
        } else {
            mp_raise_msg_varg(&mp_type_RuntimeError, MP_ERROR_TEXT("jpg prepare failed: %d."), res);
        }
        if (self->fp) {
            mp_close(self->fp);
            self->fp = MP_OBJ_NULL;
        }
    }
    m_free(self->work);     // Discard work area
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(s3lcd_jpg_obj, 4, 5, s3lcd_jpg);

//
// output function for jpg_decode
//

STATIC int out_crop(                                // 1:Ok, 0:Aborted
    JDEC *jd,                                       // Decompression object
    void *bitmap,                                   // Bitmap data to be output
    JRECT *rect) {                                  // Rectangular region of output image
    IODEV *dev = (IODEV *)jd->device;

    if (dev->left <= rect->right &&
        dev->right >= rect->left &&
        dev->top <= rect->bottom &&
        dev->bottom >= rect->top) {
        uint16_t left = MAX(dev->left, rect->left);
        uint16_t top = MAX(dev->top, rect->top);
        uint16_t right = MIN(dev->right, rect->right);
        uint16_t bottom = MIN(dev->bottom, rect->bottom);
        uint16_t dev_width = dev->right - dev->left + 1;
        uint16_t rect_width = rect->right - rect->left + 1;
        uint16_t width = (right - left + 1) * 2;
        uint16_t row;

        for (row = top; row <= bottom; row++) {
            memcpy(
                (uint16_t *)dev->fbuf + ((row - dev->top) * dev_width) + left - dev->left,
                (uint16_t *)bitmap + ((row - rect->top) * rect_width) + left - rect->left,
                width);
        }
    }
    return 1;   // Continue to decompress
}

///
/// .jpg_decode(filename {, x, y, width, height})
/// Decode a jpg file and return it or a portion of it as a tuple containing a blittable buffer,
/// the width and height of the buffer.
/// required parameters:
/// -- filename: filename
/// optional parameters:
/// -- x: x
/// -- y: y
/// -- width: width
/// -- height: height
///

STATIC mp_obj_t s3lcd_jpg_decode(size_t n_args, const mp_obj_t *args) {
    s3lcd_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    const char *filename = NULL;

    if (n_args == 2 || n_args == 6) {
        OPTIONAL_ARG(2, mp_int_t, mp_obj_get_int, x, 0)
        OPTIONAL_ARG(3, mp_int_t, mp_obj_get_int, y, 0)
        OPTIONAL_ARG(4, mp_int_t, mp_obj_get_int, width, -1)
        OPTIONAL_ARG(5, mp_int_t, mp_obj_get_int, height, -1)

        self->work = (void *)m_malloc(3100);        // Pointer to the work area
        JRESULT res;                                // Result code of TJpgDec API
        JDEC jdec;                                  // Decompression object
        IODEV devid;                                // User defined device identifier
        size_t bufsize = 0;
        static unsigned int (*input_func)(JDEC *, uint8_t *, unsigned int) = NULL;

        if (mp_obj_is_type(args[1], &mp_type_bytes)) {
            mp_buffer_info_t bufinfo;
            mp_get_buffer_raise(args[1], &bufinfo, MP_BUFFER_READ);
            devid.dataIdx = 0;
            devid.data = bufinfo.buf;
            devid.dataLen = bufinfo.len;
            input_func = buffer_in_func;
            self->fp = MP_OBJ_NULL;
        } else {
            filename = mp_obj_str_get_str(args[1]);
            self->fp = mp_open(filename, "rb");
            devid.fp = self->fp;
            input_func = file_in_func;
            devid.data = MP_OBJ_NULL;
            devid.dataLen = 0;
        }

        if (devid.fp || devid.data) {
            // Prepare to decompress
            res = jd_prepare(&jdec, input_func, self->work, 3100, &devid);
            if (res == JDR_OK) {
                if (n_args < 6) {
                    x = 0;
                    y = 0;
                    width = jdec.width;
                    height = jdec.height;
                }

                if (width == -1) {
                    width = jdec.width;
                }

                if (height == -1) {
                    height = jdec.height;
                }

                // Initialize output device
                devid.left = x;
                devid.top = y;
                devid.right = x + width - 1;
                devid.bottom = y + height - 1;

                bufsize = 2 * width * height;
                self->work_buffer = m_malloc(bufsize);
                if (self->work_buffer) {
                    memset(self->work_buffer, 0, bufsize);
                } else {
                    mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("out of memory"));
                }

                devid.fbuf = (uint8_t *)self->work_buffer;
                devid.wfbuf = width;
                devid.self = self;
                res = jd_decomp(&jdec, out_crop, 0);
                if (res != JDR_OK) {
                    mp_raise_msg_varg(&mp_type_RuntimeError, MP_ERROR_TEXT("jpg decompress failed: %d."), res);
                }
            } else {
                mp_raise_msg_varg(&mp_type_RuntimeError, MP_ERROR_TEXT("jpg prepare failed: %d."), res);
            }

            if (self->fp) {
                mp_close(self->fp);
                self->fp = MP_OBJ_NULL;
            }

            m_free(self->work);                         // Discard work area

            mp_obj_t result[3] = {
                mp_obj_new_bytearray(bufsize, (mp_obj_t *)self->work_buffer),
                mp_obj_new_int(width),
                mp_obj_new_int(height)
            };

            return mp_obj_new_tuple(3, result);
            self->work_buffer = NULL;
        }
    } else {
        mp_raise_TypeError(MP_ERROR_TEXT("jpg_decode requires either 1 or 5 arguments"));
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(s3lcd_jpg_decode_obj, 2, 6, s3lcd_jpg_decode);

//
// PNG Routines using the pngle library from https://github.com/kikuchan/pngle
// licensed under the MIT License
//

typedef struct _PNG_USER_DATA {
    s3lcd_obj_t *self;
    int16_t top;                                   // draw png starting at this row
    int16_t left;                                  // draw png starting at this column
    int16_t fg_color;                              // override foreground color
} PNG_USER_DATA;

void pngle_on_draw(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t rgba[4]) {
    PNG_USER_DATA *user_data = pngle_get_user_data(pngle);
    s3lcd_obj_t *self = user_data->self;

    if ( x+user_data->left >= self->width || y+user_data->top >= self->height) {
        return;
    }

    uint16_t color = (color565(rgba[0], rgba[1], rgba[2]));
    _fill_rect(self, x + user_data->left, y + user_data->top, w, h, color, rgba[3]);
}

///
/// .png(filename, x, y)
/// Draw a PNG image on the display
/// required parameters:
/// -- filename: the name of the file to load
/// -- x: the x coordinate to draw the image
/// -- y: the y coordinate to draw the image
///

STATIC mp_obj_t s3lcd_png(size_t n_args, const mp_obj_t *args) {
    s3lcd_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    mp_int_t x = mp_obj_get_int(args[2]);
    mp_int_t y = mp_obj_get_int(args[3]);

    char *buf = (char *)self->dma_buffer; // Reuse the dma_buffer
    int len, remain = 0;

    PNG_USER_DATA user_data = {
        self, y, x
    };

    // allocate new pngle_t and store in self to protect memory from gc
    self->work = pngle_new(self);
    pngle_t *pngle = (pngle_t *)self->work;
    pngle_set_user_data(pngle, (void *)&user_data);
    pngle_set_draw_callback(pngle, pngle_on_draw);

    if (mp_obj_is_type(args[1], &mp_type_bytes)) {
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(args[1], &bufinfo, MP_BUFFER_READ);
        int fed = pngle_feed(pngle, bufinfo.buf, bufinfo.len);
        if (fed < 0) {
            mp_raise_msg_varg(&mp_type_RuntimeError, MP_ERROR_TEXT("png decompress failed: %s"), pngle_error(pngle));
        }
    } else {
        const char *filename = mp_obj_str_get_str(args[1]);
        self->fp = mp_open(filename, "rb");
        while ((len = mp_readinto(self->fp, buf + remain, self->dma_buffer_size - remain)) > 0) {
            int fed = pngle_feed(pngle, buf, remain + len);
            if (fed < 0) {
                mp_raise_msg_varg(&mp_type_RuntimeError, MP_ERROR_TEXT("png decompress failed: %s"), pngle_error(pngle));
            }
            remain = remain + len - fed;
            if (remain > 0) {
                memmove(buf, buf + fed, remain);
            }
        }
        mp_close(self->fp);
    }
    pngle_destroy(pngle);
    self->work = NULL;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(s3lcd_png_obj, 4, 4, s3lcd_png);

//
//  png_write fileio callback functions
//

int32_t pngWrite(PNGFILE *pFile, uint8_t *pBuf, int32_t iLen) {
    return mp_write((mp_file_t *) pFile->fHandle,  pBuf, iLen);
}

int32_t pngRead(PNGFILE *pFile, uint8_t *pBuf, int32_t iLen) {
    return mp_readinto((mp_file_t *) pFile->fHandle, pBuf, iLen);
}

int32_t pngSeek(PNGFILE *pFile, int32_t iPosition) {
    return mp_seek((mp_file_t *) pFile->fHandle, iPosition, SEEK_SET);
}

void *pngOpen(const char *szFilename) {
    return (void *)mp_open(szFilename, "w+b");
}

void pngClose(PNGFILE *pFile) {
    mp_close((mp_file_t *) pFile->fHandle);
}

//
// .png_write(file_name{ x, y, width, height})
//
// save the framebuffer to a png file using PNGenc from
// https://github.com/bitbank2/PNGenc
//
//  required parameters:
//  -- filename: the name of the file to save
//  optional parameters:
//  -- x: the x coordinate to start saving
//  -- y: the y coordinate to start saving
//  -- width: the width of the area to save
//  -- height: the height of the area to save
//  returns:
//  -- file size in bytes
//

STATIC mp_obj_t s3lcd_png_write(size_t n_args, const mp_obj_t *args) {
    s3lcd_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    const char *filename = mp_obj_str_get_str(args[1]);
    int data_size = 0;
    int work_buffer_size = self->width * 3 * 2;
    int rc;

    if (n_args == 2 || n_args == 6) {
        OPTIONAL_ARG(2, mp_int_t, mp_obj_get_int, x, 0)
        OPTIONAL_ARG(3, mp_int_t, mp_obj_get_int, y, 0)
        OPTIONAL_ARG(4, mp_int_t, mp_obj_get_int, width, self->width)
        OPTIONAL_ARG(5, mp_int_t, mp_obj_get_int, height, self->height)

        if (x >= 0 && x + width <= self->width &&
            y >= 0 && y + height <= self->height) {

            PNGIMAGE *pPNG = (PNGIMAGE *) m_malloc(sizeof(PNGIMAGE));
            if (pPNG == NULL) {
                mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("memory allocation failed"));
            }
            self->work = pPNG;

            self->work_buffer = (uint16_t *) m_malloc(work_buffer_size);
            if (self->work_buffer == NULL) {
                mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("memory allocation failed"));
            }

            rc = PNG_openFile(pPNG, filename, pngOpen, pngClose, pngRead, pngWrite, pngSeek);
            if (rc != PNG_SUCCESS) {
                mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Error opening output file"));
            }

            rc = PNG_encodeBegin(pPNG, width, height, PNG_PIXEL_TRUECOLOR, 24, NULL, 9);
            if (rc == PNG_SUCCESS) {
                uint16_t *p = self->frame_buffer + x + self->width * y;
                for (int row = y; row <= y+height && rc == PNG_SUCCESS; row++) {
                    rc = PNG_addRGB565Line(pPNG, p, self->work_buffer, row-y);
                    p += self->width;
                }
                data_size = PNG_close(pPNG);
            }
            m_free(pPNG);
            self->work = NULL;
            m_free(self->work_buffer);
            self->work_buffer = NULL;
        }
    } else {
        mp_raise_TypeError(MP_ERROR_TEXT("jpg_write requires either 1 or 5 arguments"));
    }

    return mp_obj_new_int(data_size);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(s3lcd_png_write_obj, 2, 6, s3lcd_png_write);

///
/// .polygon_center(polygon)
/// Return the center of a polygon as an (x, y) tuple
/// required parameters:
/// -- polygon: a list of (x, y) tuples
///

STATIC mp_obj_t s3lcd_polygon_center(size_t n_args, const mp_obj_t *args) {
    size_t poly_len;
    mp_obj_t *polygon;
    mp_obj_get_array(args[1], &poly_len, &polygon);

    mp_float_t sum = 0.0;
    int vsx = 0;
    int vsy = 0;

    if (poly_len > 0) {
        for (int idx = 0; idx < poly_len; idx++) {
            size_t point_from_poly_len;
            mp_obj_t *point_from_poly;
            mp_obj_get_array(polygon[idx], &point_from_poly_len, &point_from_poly);
            if (point_from_poly_len < 2) {
                mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Polygon data error"));
            }

            mp_int_t v1x = mp_obj_get_int(point_from_poly[0]);
            mp_int_t v1y = mp_obj_get_int(point_from_poly[1]);

            mp_obj_get_array(polygon[(idx + 1) % poly_len], &point_from_poly_len, &point_from_poly);
            if (point_from_poly_len < 2) {
                mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Polygon data error"));
            }

            mp_int_t v2x = mp_obj_get_int(point_from_poly[0]);
            mp_int_t v2y = mp_obj_get_int(point_from_poly[1]);

            mp_float_t cross = v1x * v2y - v1y * v2x;
            sum += cross;
            vsx += (int)((v1x + v2x) * cross);
            vsy += (int)((v1y + v2y) * cross);
        }

        mp_float_t z = 1.0 / (3.0 * sum);
        vsx = (int)(vsx * z);
        vsy = (int)(vsy * z);
    } else {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Polygon data error"));
    }

    mp_obj_t center[2] = {mp_obj_new_int(vsx), mp_obj_new_int(vsy)};
    return mp_obj_new_tuple(2, center);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(s3lcd_polygon_center_obj, 2, 2, s3lcd_polygon_center);

//
// RotatePolygon: Rotate a polygon around a center point angle radians
//

STATIC void RotatePolygon(Polygon *polygon, Point center, mp_float_t angle) {
    if (polygon->length == 0) {
        return;                                     /* reject null polygons */

    }
    mp_float_t cosAngle = MICROPY_FLOAT_C_FUN(cos)(angle);
    mp_float_t sinAngle = MICROPY_FLOAT_C_FUN(sin)(angle);

    for (int i = 0; i < polygon->length; i++) {
        mp_float_t dx = (polygon->points[i].x - center.x);
        mp_float_t dy = (polygon->points[i].y - center.y);

        polygon->points[i].x = center.x + (int)0.5 + (dx * cosAngle - dy * sinAngle);
        polygon->points[i].y = center.y + (int)0.5 + (dx * sinAngle + dy * cosAngle);
    }
}

//
// public-domain code by Darel Rex Finley, 2007
// https://alienryderflex.com/polygon_fill/
//

#define MAX_POLY_CORNERS 32
STATIC void PolygonFill(s3lcd_obj_t *self, Polygon *polygon, Point location, uint16_t color, uint8_t alpha) {
    int nodes, nodeX[MAX_POLY_CORNERS], pixelY, i, j, swap;

    int minX = INT_MAX;
    int maxX = INT_MIN;
    int minY = INT_MAX;
    int maxY = INT_MIN;

    for (i = 0; i < polygon->length; i++) {
        if (polygon->points[i].x < minX) {
            minX = (int)polygon->points[i].x;
        }

        if (polygon->points[i].x > maxX) {
            maxX = (int)polygon->points[i].x;
        }

        if (polygon->points[i].y < minY) {
            minY = (int)polygon->points[i].y;
        }

        if (polygon->points[i].y > maxY) {
            maxY = (int)polygon->points[i].y;
        }
    }

    // Loop through the rows
    for (pixelY = minY; pixelY < maxY; pixelY++) {
        // Build a list of nodes.
        nodes = 0;
        j = polygon->length - 1;
        for (i = 0; i < polygon->length; i++) {
            if ((polygon->points[i].y < pixelY && polygon->points[j].y >= pixelY) ||
                (polygon->points[j].y < pixelY && polygon->points[i].y >= pixelY)) {
                if (nodes < MAX_POLY_CORNERS) {
                    nodeX[nodes++] = (int)(polygon->points[i].x +
                        (pixelY - polygon->points[i].y) /
                        (polygon->points[j].y - polygon->points[i].y) *
                        (polygon->points[j].x - polygon->points[i].x));
                } else {
                    mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Polygon too complex increase MAX_POLY_CORNERS."));
                }
            }
            j = i;
        }

        // Sort the nodes, via a simple “Bubble” sort.
        i = 0;
        while (i < nodes - 1) {
            if (nodeX[i] > nodeX[i + 1]) {
                swap = nodeX[i];
                nodeX[i] = nodeX[i + 1];
                nodeX[i + 1] = swap;
                if (i) {
                    i--;
                }
            } else {
                i++;
            }
        }

        // Fill the pixels between node pairs.
        for (i = 0; i < nodes; i += 2) {
            if (nodeX[i] >= maxX) {
                break;
            }

            if (nodeX[i + 1] > minX) {
                if (nodeX[i] < minX) {
                    nodeX[i] = minX;
                }

                if (nodeX[i + 1] > maxX) {
                    nodeX[i + 1] = maxX;
                }

                fast_hline(self, (int)location.x + nodeX[i], (int)location.y + pixelY, nodeX[i + 1] - nodeX[i] + 1, color, alpha);
            }
        }
    }
}

///
/// .polygon(polygon, x, y {, color, alpha, angle, cx, cy})
/// Draw a polygon.
/// required parameters:
/// -- polygon: a list of points [(x1, y1), (x2, y2), ...]
/// -- x: x coordinate
/// -- y: y coordinate
/// optional parameters:
/// -- color: color
/// -- alpha: alpha
/// -- angle: angle to rotate the polygon
/// -- cx: x coordinate of the center of rotation
/// -- cy: y coordinate of the center of rotation
///

STATIC mp_obj_t s3lcd_polygon(size_t n_args, const mp_obj_t *args) {
    s3lcd_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    size_t poly_len;
    mp_obj_t *polygon;
    mp_obj_get_array(args[1], &poly_len, &polygon);

    self->work = NULL;

    if (poly_len > 0) {
        mp_int_t x = mp_obj_get_int(args[2]);
        mp_int_t y = mp_obj_get_int(args[3]);
        OPTIONAL_ARG(4, mp_int_t, mp_obj_get_int, color, WHITE)
        OPTIONAL_ARG(5, mp_int_t, mp_obj_get_int, alpha, 255)

        mp_float_t angle = 0.0;
        if (n_args > 6) {
            if (mp_obj_is_float(args[6])) {
                angle = mp_obj_float_get(args[6]);
            }
            if (mp_obj_is_int(args[6])) {
                angle = (mp_float_t)mp_obj_get_int(args[6]);
            }
        }

        OPTIONAL_ARG(7, mp_int_t, mp_obj_get_int, cx, 0)
        OPTIONAL_ARG(8, mp_int_t, mp_obj_get_int, cy, 0)

        self->work = m_malloc(poly_len * sizeof(Point));
        if (self->work) {
            Point *point = (Point *)self->work;

            for (int idx = 0; idx < poly_len; idx++) {
                size_t point_from_poly_len;
                mp_obj_t *point_from_poly;
                mp_obj_get_array(polygon[idx], &point_from_poly_len, &point_from_poly);
                if (point_from_poly_len < 2) {
                    mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Polygon data error"));
                }

                mp_int_t px = mp_obj_get_int(point_from_poly[0]);
                mp_int_t py = mp_obj_get_int(point_from_poly[1]);
                point[idx].x = px;
                point[idx].y = py;
            }

            Point center;
            center.x = cx;
            center.y = cy;

            Polygon polygon;
            polygon.length = poly_len;
            polygon.points = self->work;

            if (angle > 0) {
                RotatePolygon(&polygon, center, angle);
            }

            for (int idx = 1; idx < poly_len; idx++) {
                line(
                    self,
                    (int)point[idx - 1].x + x,
                    (int)point[idx - 1].y + y,
                    (int)point[idx].x + x,
                    (int)point[idx].y + y,
                    color,
                    alpha);
            }

            m_free(self->work);
            self->work = NULL;
        } else {
            mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Polygon data error"));
        }
    } else {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Polygon data error"));
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(s3lcd_polygon_obj, 4, 9, s3lcd_polygon);

//
//  filled convex polygon
//

///
/// .fill_polygon(polygon, x, y {, color, alpha, angle, cx, cy})
/// Draw a filled polygon.
/// required parameters:
/// -- polygon: a list of points [(x1, y1), (x2, y2), ...]
/// -- x: x coordinate
/// -- y: y coordinate
/// optional parameters:
/// -- color: color
/// -- alpha: alpha
/// -- angle: angle to rotate the polygon
/// -- cx: x coordinate of the center of rotation
/// -- cy: y coordinate of the center of rotation
///

STATIC mp_obj_t s3lcd_fill_polygon(size_t n_args, const mp_obj_t *args) {
    s3lcd_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    size_t poly_len;
    mp_obj_t *polygon;
    mp_obj_get_array(args[1], &poly_len, &polygon);

    self->work = NULL;

    if (poly_len > 0) {
        mp_int_t x = mp_obj_get_int(args[2]);
        mp_int_t y = mp_obj_get_int(args[3]);
        OPTIONAL_ARG(4, mp_int_t, mp_obj_get_int, color, WHITE)
        OPTIONAL_ARG(5, mp_int_t, mp_obj_get_int, alpha, 255)

        mp_float_t angle = 0.0;
        if (n_args > 6) {
            if (mp_obj_is_float(args[6])) {
                angle = mp_obj_float_get(args[6]);
            }
            if (mp_obj_is_int(args[6])) {
                angle = (mp_float_t)mp_obj_get_int(args[6]);
            }
        }

        OPTIONAL_ARG(7, mp_int_t, mp_obj_get_int, cx, 0)
        OPTIONAL_ARG(8, mp_int_t, mp_obj_get_int, cy, 0)

        self->work = m_malloc(poly_len * sizeof(Point));
        if (self->work) {
            Point *point = (Point *)self->work;

            for (int idx = 0; idx < poly_len; idx++) {
                size_t point_from_poly_len;
                mp_obj_t *point_from_poly;
                mp_obj_get_array(polygon[idx], &point_from_poly_len, &point_from_poly);
                if (point_from_poly_len < 2) {
                    mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Polygon data error"));
                }

                point[idx].x = mp_obj_get_int(point_from_poly[0]);
                point[idx].y = mp_obj_get_int(point_from_poly[1]);
            }

            Point center = {cx, cy};
            Polygon polygon = {poly_len, self->work};

            if (angle != 0) {
                RotatePolygon(&polygon, center, angle);
            }

            Point location = {x, y};
            PolygonFill(self, &polygon, location, color, alpha);

            m_free(self->work);
            self->work = NULL;
        } else {
            mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Polygon data error"));
        }

    } else {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Polygon data error"));
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(s3lcd_fill_polygon_obj, 4, 9, s3lcd_fill_polygon);


//
// copy rows from the framebuffer to the dma buffer and send it to the display beginning at row.
//
//  self: s3lcd object
//  src: pointer to the framebuffer
//  row: row to start at
//  rows: number of rows to send
//  len: pixel data length to send

unsigned char reverse(unsigned char b) {
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}

void s3lcd_dma_display(s3lcd_obj_t *self, uint16_t *src, uint16_t row, uint16_t rows, size_t len) {
    uint16_t *dma_buffer = self->dma_buffer;
    if (self->swap_color_bytes) {
        for (size_t i = 0; i < len; i++) {
            *dma_buffer++ = _swap_bytes(src[i]);
        }
    } else{
        memcpy(self->dma_buffer, src, len * 2);
    }

    lcd_panel_active = true;
    esp_lcd_panel_draw_bitmap(self->panel_handle, 0, row, self->width, row + rows, self->dma_buffer);
    while (lcd_panel_active) {
    }
}

///
/// .show()
/// Show the framebuffer.
///

STATIC mp_obj_t s3lcd_show(size_t n_args, const mp_obj_t *args) {
    s3lcd_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    size_t pixels = self->dma_rows * self->width;
    uint16_t *fb = self->frame_buffer;
    for (int y = 0; y < self->height - self->dma_rows + 1; y += self->dma_rows) {
        s3lcd_dma_display(self, fb, y, self->dma_rows, pixels);
        fb += pixels;
    }

    if (self->height % self->dma_rows != 0) {
        size_t remaining = self->height % self->dma_rows;
        s3lcd_dma_display(self, fb, (self->height - remaining), remaining, remaining * self->width);
    }

    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(s3lcd_show_obj, 1, 1, s3lcd_show);

///
/// .deinit()
/// Deinitialize the s3lcd object and frees allocated memory.
///

STATIC mp_obj_t s3lcd_deinit(size_t n_args, const mp_obj_t *args) {
    s3lcd_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    esp_lcd_panel_del(self->panel_handle);
    self->panel_handle = NULL;

    esp_lcd_panel_io_del(self->io_handle);
    self->io_handle = NULL;

    if (mp_obj_is_type(self->bus, &s3lcd_i80_bus_type)) {
        esp_lcd_del_i80_bus(self->bus_handle.i80);
        self->bus_handle.i80 = NULL;
    } else if (mp_obj_is_type(self->bus, &s3lcd_spi_bus_type)) {
        s3lcd_spi_bus_obj_t *config = MP_OBJ_TO_PTR(self->bus);
        spi_bus_free(config->spi_host);
        self->bus_handle.spi = NULL;
    }

    m_free(self->work);
    self->work = NULL;

    m_free(self->frame_buffer);
    self->frame_buffer = NULL;
    self->frame_buffer_size = 0;

    free(self->dma_buffer);
    self->dma_buffer = NULL;
    self->dma_buffer_size = 0;
    self->dma_rows = 0;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(s3lcd_deinit_obj, 1, 1, s3lcd_deinit);

STATIC const mp_rom_map_elem_t s3lcd_locals_dict_table[] = {
    {MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&s3lcd_write_obj)},
    {MP_ROM_QSTR(MP_QSTR_write_len), MP_ROM_PTR(&s3lcd_write_len_obj)},
    {MP_ROM_QSTR(MP_QSTR_reset), MP_ROM_PTR(&s3lcd_reset_obj)},
    {MP_ROM_QSTR(MP_QSTR_inversion_mode), MP_ROM_PTR(&s3lcd_inversion_mode_obj)},
    {MP_ROM_QSTR(MP_QSTR_map_bitarray_to_rgb565), MP_ROM_PTR(&s3lcd_map_bitarray_to_rgb565_obj)},
    {MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&s3lcd_init_obj)},
    {MP_ROM_QSTR(MP_QSTR_pixel), MP_ROM_PTR(&s3lcd_pixel_obj)},
    {MP_ROM_QSTR(MP_QSTR_line), MP_ROM_PTR(&s3lcd_line_obj)},
    {MP_ROM_QSTR(MP_QSTR_blit_buffer), MP_ROM_PTR(&s3lcd_blit_buffer_obj)},
    {MP_ROM_QSTR(MP_QSTR_draw), MP_ROM_PTR(&s3lcd_draw_obj)},
    {MP_ROM_QSTR(MP_QSTR_draw_len), MP_ROM_PTR(&s3lcd_draw_len_obj)},
    {MP_ROM_QSTR(MP_QSTR_bitmap), MP_ROM_PTR(&s3lcd_bitmap_obj)},
    {MP_ROM_QSTR(MP_QSTR_fill_rect), MP_ROM_PTR(&s3lcd_fill_rect_obj)},
    {MP_ROM_QSTR(MP_QSTR_clear), MP_ROM_PTR(&s3lcd_clear_obj)},
    {MP_ROM_QSTR(MP_QSTR_fill), MP_ROM_PTR(&s3lcd_fill_obj)},
    {MP_ROM_QSTR(MP_QSTR_hline), MP_ROM_PTR(&s3lcd_hline_obj)},
    {MP_ROM_QSTR(MP_QSTR_vline), MP_ROM_PTR(&s3lcd_vline_obj)},
    {MP_ROM_QSTR(MP_QSTR_fill_circle), MP_ROM_PTR(&s3lcd_fill_circle_obj)},
    {MP_ROM_QSTR(MP_QSTR_circle), MP_ROM_PTR(&s3lcd_circle_obj)},
    {MP_ROM_QSTR(MP_QSTR_rect), MP_ROM_PTR(&s3lcd_rect_obj)},
    {MP_ROM_QSTR(MP_QSTR_text), MP_ROM_PTR(&s3lcd_text_obj)},
    {MP_ROM_QSTR(MP_QSTR_rotation), MP_ROM_PTR(&s3lcd_rotation_obj)},
    {MP_ROM_QSTR(MP_QSTR_width), MP_ROM_PTR(&s3lcd_width_obj)},
    {MP_ROM_QSTR(MP_QSTR_height), MP_ROM_PTR(&s3lcd_height_obj)},
    {MP_ROM_QSTR(MP_QSTR_vscrdef), MP_ROM_PTR(&s3lcd_vscrdef_obj)},
    {MP_ROM_QSTR(MP_QSTR_vscsad), MP_ROM_PTR(&s3lcd_vscsad_obj)},
    {MP_ROM_QSTR(MP_QSTR_scroll), MP_ROM_PTR(&s3lcd_scroll_obj)},
    {MP_ROM_QSTR(MP_QSTR_jpg), MP_ROM_PTR(&s3lcd_jpg_obj)},
    {MP_ROM_QSTR(MP_QSTR_jpg_decode), MP_ROM_PTR(&s3lcd_jpg_decode_obj)},
    {MP_ROM_QSTR(MP_QSTR_png), MP_ROM_PTR(&s3lcd_png_obj)},
    {MP_ROM_QSTR(MP_QSTR_png_write), MP_ROM_PTR(&s3lcd_png_write_obj)},
    {MP_ROM_QSTR(MP_QSTR_polygon_center), MP_ROM_PTR(&s3lcd_polygon_center_obj)},
    {MP_ROM_QSTR(MP_QSTR_polygon), MP_ROM_PTR(&s3lcd_polygon_obj)},
    {MP_ROM_QSTR(MP_QSTR_fill_polygon), MP_ROM_PTR(&s3lcd_fill_polygon_obj)},
    {MP_ROM_QSTR(MP_QSTR_show), MP_ROM_PTR(&s3lcd_show_obj)},
    {MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&s3lcd_deinit_obj)},
};
STATIC MP_DEFINE_CONST_DICT(s3lcd_locals_dict, s3lcd_locals_dict_table);
/* methods end */

#if MICROPY_OBJ_TYPE_REPR == MICROPY_OBJ_TYPE_REPR_SLOT_INDEX

MP_DEFINE_CONST_OBJ_TYPE(
    s3lcd_type,
    MP_QSTR_ESPLCD,
    MP_TYPE_FLAG_NONE,
    print, s3lcd_print,
    make_new, s3lcd_make_new,
    locals_dict, &s3lcd_locals_dict);

#else

const mp_obj_type_t s3lcd_type = {
    {&mp_type_type},
    .name = MP_QSTR_ESPLCD,
    .print = s3lcd_print,
    .make_new = s3lcd_make_new,
    .locals_dict = (mp_obj_dict_t *)&s3lcd_locals_dict,
};

#endif

//
// Find the Rotation table for the given width and height
// return the first rotation table if no match is found.
//

s3lcd_rotation_t *set_rotations(uint16_t width, uint16_t height) {;
    for (int i=0; i < MP_ARRAY_SIZE(ROTATIONS); i++) {
        s3lcd_rotation_t *rotation;
        if ((rotation = ROTATIONS[i]) != NULL) {
            if (rotation->width == width && rotation->height == height) {
                return rotation;
            }
        }
    }
    return ROTATIONS[0];
}

///
/// .__init__(bus, width, height, reset, rotations, rotation, inversion, options)
/// required parameters:
/// -- bus: bus
/// -- width: width of the display
/// -- height: height of the display
/// optional keyword parameters:
/// -- reset: reset pin
/// -- rotations: number of rotations
/// -- rotation: rotation
/// -- inversion: inversion
/// -- options: options
///

mp_obj_t s3lcd_make_new(const mp_obj_type_t *type,
    size_t n_args,
    size_t n_kw,
    const mp_obj_t *all_args) {
    enum {
        ARG_bus,
        ARG_width,
        ARG_height,
        ARG_reset,
        ARG_rotations,
        ARG_rotation,
        ARG_custom_init,
        ARG_color_space,
        ARG_inversion_mode,
        ARG_dma_rows,
        ARG_options,
    };

    STATIC const mp_arg_t allowed_args[] = {
        {MP_QSTR_bus, MP_ARG_OBJ | MP_ARG_REQUIRED, {.u_obj = MP_OBJ_NULL}},
        {MP_QSTR_width, MP_ARG_INT | MP_ARG_REQUIRED, {.u_int = TFT_WIDTH}},
        {MP_QSTR_height, MP_ARG_INT | MP_ARG_REQUIRED, {.u_int = TFT_HEIGHT}},
        {MP_QSTR_reset, MP_ARG_INT | MP_ARG_KW_ONLY, {.u_int = -1}},
        {MP_QSTR_rotations, MP_ARG_OBJ | MP_ARG_KW_ONLY, {.u_obj = MP_OBJ_NULL}},
        {MP_QSTR_rotation, MP_ARG_INT | MP_ARG_KW_ONLY, {.u_int = 0}},
        {MP_QSTR_custom_init, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL}},
        {MP_QSTR_color_space, MP_ARG_INT | MP_ARG_KW_ONLY, {.u_int = ESP_LCD_COLOR_SPACE_RGB}},
        {MP_QSTR_inversion_mode, MP_ARG_BOOL | MP_ARG_KW_ONLY, {.u_bool = true}},
        {MP_QSTR_dma_rows, MP_ARG_INT | MP_ARG_KW_ONLY, {.u_int = 16}},
        {MP_QSTR_options, MP_ARG_INT | MP_ARG_KW_ONLY, {.u_int = 0}},
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // create new object
    s3lcd_obj_t *self = m_new_obj(s3lcd_obj_t);
    self->base.type = &s3lcd_type;
    self->bus = args[ARG_bus].u_obj;

    // set s3lcd parameters
    self->rst = args[ARG_reset].u_int;
    self->width = args[ARG_width].u_int;
    self->height = args[ARG_height].u_int;

    uint16_t longest_axis = ((self->width > self->height) ? self->width : self->height);
    self->dma_rows = args[ARG_dma_rows].u_int;
    self->dma_buffer_size = self->dma_rows * longest_axis * 2;
    self->dma_buffer = heap_caps_malloc(self->dma_buffer_size, MALLOC_CAP_DMA);
    memset(self->dma_buffer, 0, self->dma_buffer_size);
    if (self->dma_buffer == NULL) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("Failed to allocate DMA buffer"));
    }

    self->rotations = set_rotations(self->width, self->height);
    self->rotations_len = 4;

    if (args[ARG_rotations].u_obj != MP_OBJ_NULL) {
        size_t len;
        mp_obj_t *rotations_array = MP_OBJ_NULL;
        mp_obj_get_array(args[ARG_rotations].u_obj, &len, &rotations_array);
        self->rotations_len = len;
        self->rotations = m_new(s3lcd_rotation_t, self->rotations_len);
        for (int i = 0; i < self->rotations_len; i++) {
            mp_obj_t *rotation_tuple = NULL;
            size_t rotation_tuple_len = 0;

            mp_obj_tuple_get(rotations_array[i], &rotation_tuple_len, &rotation_tuple);
            if (rotation_tuple_len != 7) {
                mp_raise_ValueError(MP_ERROR_TEXT("rotations tuple must have 7 elements"));
            }
            self->rotations[i].width = mp_obj_get_int(rotation_tuple[0]);
            self->rotations[i].height = mp_obj_get_int(rotation_tuple[1]);
            self->rotations[i].x_gap = mp_obj_get_int(rotation_tuple[2]);
            self->rotations[i].y_gap = mp_obj_get_int(rotation_tuple[3]);
            self->rotations[i].swap_xy = mp_obj_is_true(rotation_tuple[4]);
            self->rotations[i].mirror_x = mp_obj_is_true(rotation_tuple[5]);
            self->rotations[i].mirror_y = mp_obj_is_true(rotation_tuple[6]);
        }
    }

    self->rotation = args[ARG_rotation].u_int % self->rotations_len;
    self->custom_init = args[ARG_custom_init].u_obj;
    self->color_space = args[ARG_color_space].u_int;
    self->inversion_mode = args[ARG_inversion_mode].u_bool;
    self->options = args[ARG_options].u_int & 0xff;
    self->frame_buffer_size = self->width * self->height * 2;
    self->frame_buffer = NULL;
    return MP_OBJ_FROM_PTR(self);
}

STATIC const mp_map_elem_t s3lcd_module_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_s3lcd)},
    {MP_ROM_QSTR(MP_QSTR_color565), (mp_obj_t)&s3lcd_color565_obj},
    {MP_ROM_QSTR(MP_QSTR_map_bitarray_to_rgb565), (mp_obj_t)&s3lcd_map_bitarray_to_rgb565_obj},
    {MP_ROM_QSTR(MP_QSTR_ESPLCD), (mp_obj_t)&s3lcd_type},
    {MP_ROM_QSTR(MP_QSTR_I80_BUS), (mp_obj_t)&s3lcd_i80_bus_type},
    {MP_ROM_QSTR(MP_QSTR_SPI_BUS), (mp_obj_t)&s3lcd_spi_bus_type},

    {MP_ROM_QSTR(MP_QSTR_RGB), MP_ROM_INT(ESP_LCD_COLOR_SPACE_RGB)},
    {MP_ROM_QSTR(MP_QSTR_BGR), MP_ROM_INT(ESP_LCD_COLOR_SPACE_BGR)},

    {MP_ROM_QSTR(MP_QSTR_BLACK), MP_ROM_INT(BLACK)},
    {MP_ROM_QSTR(MP_QSTR_BLUE), MP_ROM_INT(BLUE)},
    {MP_ROM_QSTR(MP_QSTR_RED), MP_ROM_INT(RED)},
    {MP_ROM_QSTR(MP_QSTR_GREEN), MP_ROM_INT(GREEN)},
    {MP_ROM_QSTR(MP_QSTR_CYAN), MP_ROM_INT(CYAN)},
    {MP_ROM_QSTR(MP_QSTR_MAGENTA), MP_ROM_INT(MAGENTA)},
    {MP_ROM_QSTR(MP_QSTR_YELLOW), MP_ROM_INT(YELLOW)},
    {MP_ROM_QSTR(MP_QSTR_WHITE), MP_ROM_INT(WHITE)},
    {MP_ROM_QSTR(MP_QSTR_TRANSPARENT), MP_ROM_INT(-1)},
    {MP_ROM_QSTR(MP_QSTR_WRAP), MP_ROM_INT(OPTIONS_WRAP)},
    {MP_ROM_QSTR(MP_QSTR_WRAP_H), MP_ROM_INT(OPTIONS_WRAP_H)},
    {MP_ROM_QSTR(MP_QSTR_WRAP_V), MP_ROM_INT(OPTIONS_WRAP_V)}
};

STATIC MP_DEFINE_CONST_DICT(mp_module_s3lcd_globals, s3lcd_module_globals_table);

const mp_obj_module_t mp_module_s3lcd = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&mp_module_s3lcd_globals,
};

// use the following for older versions of MicroPython

#if MICROPY_VERSION >= 0x011300                     // MicroPython 1.19 or later
MP_REGISTER_MODULE(MP_QSTR_s3lcd, mp_module_s3lcd);
#else
MP_REGISTER_MODULE(MP_QSTR_s3lcd, mp_module_s3lcd, MODULE_ESPLCD_ENABLE);
#endif
