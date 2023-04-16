#include "esp_lcd_panel_commands.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "soc/soc_caps.h"
#include "driver/gpio.h"

#include "mphalport.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/gc.h"

#include "s3lcd_i80_bus.h"
#include <string.h>

STATIC void s3lcd_i80_bus_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    (void) kind;
    s3lcd_i80_bus_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "<I80 %s, dc=%d, wr=%d, rd=%d, cs=%d, pclk=%d, bus_width=%d, lcd_cmd_bits=%d, lcd_param_bits=%d, dc_idle_level=%d, dc_cmd_level=%d, dc_dummy_level=%d, dc_data_level=%d, cs_active_high=%d, reverse_color_bits=%d, swap_color_bytes=%d, pclk_active_neg=%d, pclk_idle_low=%d>",
        self->name,
        self->dc_gpio_num,
        self->wr_gpio_num,
        self->rd_gpio_num,
        self->cs_gpio_num,
        self->pclk_hz,
        self->bus_width,
        self->lcd_cmd_bits,
        self->lcd_param_bits,
        self->dc_levels.dc_idle_level,
        self->dc_levels.dc_cmd_level,
        self->dc_levels.dc_dummy_level,
        self->dc_levels.dc_data_level,
        self->flags.cs_active_high,
        self->flags.reverse_color_bits,
        self->flags.swap_color_bytes,
        self->flags.pclk_active_neg,
        self->flags.pclk_idle_low
    );
}

/// i80_bus
/// Configure a i8080 parallel bus.
///
/// Parameters:
///   - data: tuple list of data pins
///   - dc: data/command pin number
///   - wr: write pin number
///   - rd: read pin number
///   - cs: chip select pin number
///   - pclk: pixel clock frequency in Hz
///   - bus_width: bus width in bits
///   - lcd_cmd_bits: number of bits used to send commands to the LCD
///   - lcd_param_bits: number of bits used to send parameters to the LCD
///   - dc_idle_level: data/command pin level when idle
///   - dc_cmd_level: data/command pin level when sending commands
///   - dc_dummy_level: data/command pin level when sending dummy data
///   - dc_data_level: data/command pin level when sending data
///   - cs_active_high: chip select pin level when active
///   - reverse_color_bits: reverse the order of color bits
///   - swap_color_bytes: swap the order of color bytes
///   - pclk_active_neg: pixel clock is active negative
///   - pclk_idle_low: pixel clock is idle low
///

STATIC mp_obj_t s3lcd_i80_bus_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    enum {
        ARG_data,
        ARG_dc,
        ARG_wr,
        ARG_rd,
        ARG_cs,
        ARG_pclk,
        ARG_bus_width,
        ARG_lcd_cmd_bits,
        ARG_lcd_param_bits,
        ARG_dc_idle_level,
        ARG_dc_cmd_level,
        ARG_dc_dummy_level,
        ARG_dc_data_level,
        ARG_cs_active_high,
        ARG_reverse_color_bits,
        ARG_swap_color_bytes,
        ARG_pclk_active_neg,
        ARG_pclk_idle_low,
    };

    STATIC const mp_arg_t allowed_args[] = {
        { MP_QSTR_data,                 MP_ARG_OBJ  | MP_ARG_REQUIRED                      },
        { MP_QSTR_dc,                   MP_ARG_INT  | MP_ARG_REQUIRED                      },
        { MP_QSTR_wr,                   MP_ARG_INT  | MP_ARG_REQUIRED                      },
        { MP_QSTR_rd,                   MP_ARG_INT  | MP_ARG_KW_ONLY, {.u_int = -1       } },
        { MP_QSTR_cs,                   MP_ARG_INT  | MP_ARG_KW_ONLY, {.u_int = -1       } },
        { MP_QSTR_pclk,                 MP_ARG_INT  | MP_ARG_KW_ONLY, {.u_int = 10000000 } },
        { MP_QSTR_bus_width,            MP_ARG_INT  | MP_ARG_KW_ONLY, {.u_int = 8        } },
        { MP_QSTR_lcd_cmd_bits,         MP_ARG_INT  | MP_ARG_KW_ONLY, {.u_int = 8        } },
        { MP_QSTR_lcd_param_bits,       MP_ARG_INT  | MP_ARG_KW_ONLY, {.u_int = 8        } },
        { MP_QSTR_dc_idle_level,        MP_ARG_INT  | MP_ARG_KW_ONLY, {.u_int = 0        } },
        { MP_QSTR_dc_cmd_level,         MP_ARG_INT  | MP_ARG_KW_ONLY, {.u_int = 0        } },
        { MP_QSTR_dc_dummy_level,       MP_ARG_INT  | MP_ARG_KW_ONLY, {.u_int = 0        } },
        { MP_QSTR_dc_data_level,        MP_ARG_INT  | MP_ARG_KW_ONLY, {.u_int = 1        } },
        { MP_QSTR_cs_active_high,       MP_ARG_BOOL | MP_ARG_KW_ONLY, {.u_bool = false   } },
        { MP_QSTR_reverse_color_bits,   MP_ARG_BOOL | MP_ARG_KW_ONLY, {.u_bool = false   } },
        { MP_QSTR_swap_color_bytes,     MP_ARG_BOOL | MP_ARG_KW_ONLY, {.u_int = 0        } },
        { MP_QSTR_pclk_active_neg,      MP_ARG_BOOL | MP_ARG_KW_ONLY, {.u_bool = false   } },
        { MP_QSTR_pclk_idle_low,        MP_ARG_BOOL | MP_ARG_KW_ONLY, {.u_bool = false   } },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // create new i80 object
    s3lcd_i80_bus_obj_t *self = m_new_obj(s3lcd_i80_bus_obj_t);
    self->base.type = &s3lcd_i80_bus_type;
    self->name = "s3lcd_i80";

    // data bus
    mp_obj_tuple_t *t = MP_OBJ_TO_PTR(args[ARG_data].u_obj);
    if (t->len > 16) {
        mp_raise_ValueError("data bus width must be <= 16");
    }

    self->bus_width = t->len;
    for (size_t i = 0; i < t->len; i++) {
        self->data_gpio_nums[i] = mp_obj_get_int(t->items[i]);
    }

    for (size_t i = t->len; i < 24; i++) {
        self->data_gpio_nums[i] = -1;
    }

    self->dc_gpio_num = args[ARG_dc].u_int;
    self->wr_gpio_num = args[ARG_wr].u_int;
    self->rd_gpio_num = args[ARG_rd].u_int;
    self->cs_gpio_num = args[ARG_cs].u_int;
    self->pclk_hz = args[ARG_pclk].u_int;
    self->bus_width = args[ARG_bus_width].u_int;
    self->lcd_cmd_bits = args[ARG_lcd_cmd_bits].u_int;
    self->lcd_param_bits = args[ARG_lcd_param_bits].u_int;
    self->dc_levels.dc_idle_level = args[ARG_dc_idle_level].u_int;
    self->dc_levels.dc_cmd_level = args[ARG_dc_cmd_level].u_int;
    self->dc_levels.dc_dummy_level = args[ARG_dc_dummy_level].u_int;
    self->dc_levels.dc_data_level = args[ARG_dc_data_level].u_int;
    self->flags.cs_active_high = args[ARG_cs_active_high].u_bool;
    self->flags.reverse_color_bits = args[ARG_reverse_color_bits].u_bool;
    self->flags.swap_color_bytes = args[ARG_swap_color_bytes].u_bool;
    self->flags.pclk_active_neg = args[ARG_pclk_active_neg].u_bool;
    self->flags.pclk_idle_low = args[ARG_pclk_idle_low].u_bool;
    return MP_OBJ_FROM_PTR(self);
}

STATIC const mp_rom_map_elem_t s3lcd_i80_bus_locals_dict_table[] = {
};
STATIC MP_DEFINE_CONST_DICT(s3lcd_i80_bus_locals_dict, s3lcd_i80_bus_locals_dict_table);

#if MICROPY_OBJ_TYPE_REPR == MICROPY_OBJ_TYPE_REPR_SLOT_INDEX

MP_DEFINE_CONST_OBJ_TYPE(
    s3lcd_i80_bus_type,
    MP_QSTR_I80_BUS,
    MP_TYPE_FLAG_NONE,
    print, s3lcd_i80_bus_print,
    make_new, s3lcd_i80_bus_make_new,
    locals_dict, &s3lcd_i80_bus_locals_dict);

#else

const mp_obj_type_t s3lcd_type = {
    {&mp_type_type},
    .name = MP_QSTR_I80_BUS,
    .print = s3lcd_print,
    .make_new = s3lcd_make_new,
    .locals_dict = (mp_obj_dict_t *)&s3lcd_i80_bus_locals_dict,
};

#endif