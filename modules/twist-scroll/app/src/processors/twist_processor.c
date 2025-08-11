// modules/twist-scroll/app/src/processors/twist_processor.c
// まずはビルドを通すための安全な最小骨格。
// （ZMK 内部の pointing ヘッダに依存させず、将来ここに処理を足せる形）

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#include <zephyr/input/input.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>

#include <zmk/endpoints.h>
#include <zmk/hid.h>
#include <zmk/hid_indicators.h>

#include "twist_scroll.h"

LOG_MODULE_REGISTER(twist_processor, CONFIG_ZMK_LOG_LEVEL);

// 状態（将来ここで移動量→スクロール変換を行う）
static int32_t acc_x, acc_y;
static int64_t last_ts;

static void reset_acc(void)
{
    acc_x = 0;
    acc_y = 0;
    last_ts = 0;
}

// Zephyr の input_event を受け取って処理する想定の関数骨格。
// 現状は twist 有効時のみ簡単に累積し、何もしなければ即 return。
int twist_processor_handle_event(struct input_event *ev)
{
    if (!twist_scroll_is_enabled()) {
        return 0;
    }

    if (ev->type == INPUT_EV_REL) {
        if (ev->code == INPUT_REL_X) acc_x += ev->value;
        if (ev->code == INPUT_REL_Y) acc_y += ev->value;
        last_ts = ev->timestamp;

        // ここに閾値やスクロール送出処理を後で実装予定
        // 例:
        // struct zmk_hid_mouse_report *mr = zmk_hid_mouse_get_report();
        // mr->wheel = CLAMP(mr->wheel + delta, -127, 127);
        // zmk_hid_mouse_set_report(mr);
        // zmk_endpoints_send_mouse_report();
    }

    return 0;
}
