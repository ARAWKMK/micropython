/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Paul Sokolovsky
 * Copyright (c) 2017 Eric Poulsen
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

#include <string.h>

#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"

#include "esp_task_wdt.h"

const mp_obj_type_t machine_wdt_type;

typedef struct _machine_wdt_obj_t {
    mp_obj_base_t base;
    esp_task_wdt_user_handle_t twdt_user_handle;
} machine_wdt_obj_t;

STATIC machine_wdt_obj_t wdt_default = {
    {&machine_wdt_type}, 0
};

STATIC mp_obj_t machine_wdt_make_new(const mp_obj_type_t *type_in, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_id, ARG_timeout };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_id, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_timeout, MP_ARG_INT, {.u_int = 5000} }
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    if (args[ARG_id].u_int != 0) {
        mp_raise_ValueError(NULL);
    }

    if (args[ARG_timeout].u_int <= 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("WDT timeout too short"));
    }

    esp_task_wdt_config_t config = {
        .timeout_ms = args[ARG_timeout].u_int,
        .idle_core_mask = 0,
        .trigger_panic = true,
    };
    mp_int_t rs_code = esp_task_wdt_reconfigure(&config);
    if (rs_code != ESP_OK) {
        mp_raise_OSError(rs_code);
    }

    if (wdt_default.twdt_user_handle == NULL) {
        rs_code = esp_task_wdt_add_user("mpy_machine_wdt", &wdt_default.twdt_user_handle);
        if (rs_code != ESP_OK) {
            mp_raise_OSError(rs_code);
        }
    }

    return &wdt_default;
}

STATIC mp_obj_t machine_wdt_feed(mp_obj_t self_in) {
    (void)self_in;
    mp_int_t rs_code = esp_task_wdt_reset_user(wdt_default.twdt_user_handle);
    if (rs_code != ESP_OK) {
        mp_raise_OSError(rs_code);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_wdt_feed_obj, machine_wdt_feed);

STATIC const mp_rom_map_elem_t machine_wdt_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_feed), MP_ROM_PTR(&machine_wdt_feed_obj) },
};
STATIC MP_DEFINE_CONST_DICT(machine_wdt_locals_dict, machine_wdt_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    machine_wdt_type,
    MP_QSTR_WDT,
    MP_TYPE_FLAG_NONE,
    make_new, machine_wdt_make_new,
    locals_dict, &machine_wdt_locals_dict
    );
