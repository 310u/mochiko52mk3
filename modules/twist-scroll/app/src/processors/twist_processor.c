// modules/twist-scroll/app/src/processors/twist_processor.c
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <zephyr/input/input.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>

#include <zmk/endpoints.h>
#include <zmk/hid.h>
#include <zmk/hid_indicators.h>
#include <zmk/logging.h>

// ★ 不在ヘッダは削除：<zmk/events/mouse_movement_state_changed.h>

// 入力プロセッサ API（プリセット dtsi を使う側では不要だが、独自実装で参照）
#include <zmk/pointing/input_listener.h>
#include <zmk/pointing/input_processor.h>

#include "twist_scroll.h"

LOG_MODULE_REGISTER(twist_processor, CONFIG_ZMK_LOG_LEVEL);

// ねじり検出のための簡易状態（実装を最小限：X/Y の相関で「捻りっぽい」判定）
static bool twist_mode_enabled;
static int32_t acc_x, acc_y;
static int64_t last_ts;

void twist_scroll_enable(bool en)
{
    twist_mode_enabled = en;
    acc_x = acc_y = 0;
    last_ts = 0;
}

// 入力イベントの処理本体
static void process_rel_event(uint16_t code, int32_t value, int64_t timestamp)
{
    if (!twist_mode_enabled) {
        // ねじりモードでないときは何もしない（素通しにしたい場合は return せず forward する）
        return;
    }

    // 累積（シンプルにしてまずは動かす）
    if (code == INPUT_REL_X) acc_x += value;
    if (code == INPUT_REL_Y) acc_y += value;

    // ざっくり「回転っぽさ」を X と Y の符号差/比率で見る
    // 最初は閾値小さめ、必要なら後で調整
    const int32_t th = 8;
    if (ABS(acc_x) + ABS(acc_y) < th) {
        return;
    }

    // X と Y が同時に動いていて、かつ比率がある程度大きい＝回転っぽいとみなす
    int32_t scroll_delta = 0;

    // Kensington SlimBlade の「ボールを捻って縦スクロール」に寄せる：
    // X を重めに見て縦スクロールへ変換
    if ((acc_x > 0 && acc_y < 0) || (acc_x < 0 && acc_y > 0)) {
        // 斜め成分（クロス）をスクロール量へ
        scroll_delta = (acc_x - acc_y) / 4; // 調整パラメータ
    } else {
        scroll_delta = (acc_x + acc_y) / 8;
    }

    // ZMK の HID スクロール出力（垂直スクロール）
    if (scroll_delta != 0) {
        struct zmk_hid_mouse_report *mr = zmk_hid_mouse_get_report();
        // 垂直スクロール（ホイール）に加算。ZMK は 8bit 単位なのでクリップ
        int16_t new_wheel = (int16_t)mr->wheel + scroll_delta;
        if (new_wheel > 127) new_wheel = 127;
        if (new_wheel < -127) new_wheel = -127;
        mr->wheel = (int8_t)new_wheel;
        zmk_hid_mouse_set_report(mr);
        zmk_endpoints_send_mouse_report();
    }

    // 使い切ったのでリセット（慣性を入れたいならここを変える）
    acc_x = acc_y = 0;
    last_ts = timestamp;
}

// ZMK の input-listener から呼ばれる想定のフック
int twist_processor_handle_event(struct input_event *ev)
{
    if (ev->type == INPUT_EV_REL &&
        (ev->code == INPUT_REL_X || ev->code == INPUT_REL_Y)) {
        process_rel_event(ev->code, ev->value, ev->timestamp);
        // 自分で消費してホストへは「スクロールだけ」送るなら 1 を返して止める手もある
        // まずは他のプロセッサ/デフォルト経路に任せるなら 0 を返す
        return 0;
    }
    return 0;
}
