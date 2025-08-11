// modules/twist-scroll/app/src/processors/twist_processor.c
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>   // ★ 追加

#include <zephyr/input/input.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>

#include <zmk/endpoints.h>
#include <zmk/hid.h>
#include <zmk/hid_indicators.h>

// ★ 不在ヘッダは使わない
// #include <zmk/events/mouse_movement_state_changed.h>

#include <zmk/pointing/input_listener.h>
#include <zmk/pointing/input_processor.h>

#include "twist_scroll.h"

LOG_MODULE_REGISTER(twist_processor, CONFIG_ZMK_LOG_LEVEL);

// ねじりモードの簡易状態
static bool twist_mode_enabled;
static int32_t acc_x, acc_y;
static int64_t last_ts;

void twist_scroll_enable(bool en)
{
    twist_mode_enabled = en;
    acc_x = acc_y = 0;
    last_ts = 0;
}

static void process_rel_event(uint16_t code, int32_t value, int64_t timestamp)
{
    if (!twist_mode_enabled) return;

    if (code == INPUT_REL_X) acc_x += value;
    if (code == INPUT_REL_Y) acc_y += value;

    const int32_t th = 8;
    if (ABS(acc_x) + ABS(acc_y) < th) return;

    int32_t scroll_delta = 0;
    if ((acc_x > 0 && acc_y < 0) || (acc_x < 0 && acc_y > 0)) {
        scroll_delta = (acc_x - acc_y) / 4;   // 調整係数
    } else {
        scroll_delta = (acc_x + acc_y) / 8;   // 調整係数
    }

    if (scroll_delta != 0) {
        struct zmk_hid_mouse_report *mr = zmk_hid_mouse_get_report();
        int16_t new_wheel = (int16_t)mr->wheel + scroll_delta;
        if (new_wheel > 127) new_wheel = 127;
        if (new_wheel < -127) new_wheel = -127;
        mr->wheel = (int8_t)new_wheel;
        zmk_hid_mouse_set_report(mr);
        zmk_endpoints_send_mouse_report();
    }

    acc_x = acc_y = 0;
    last_ts = timestamp;
}

int twist_processor_handle_event(struct input_event *ev)
{
    if (ev->type == INPUT_EV_REL &&
        (ev->code == INPUT_REL_X || ev->code == INPUT_REL_Y)) {
        process_rel_event(ev->code, ev->value, ev->timestamp);
        return 0;
    }
    return 0;
}
