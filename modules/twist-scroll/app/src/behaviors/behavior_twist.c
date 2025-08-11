// modules/twist-scroll/app/src/behaviors/behavior_twist.c
// DT の compatible: "zmk,behavior-twist" を想定
// もし binding の compatible が違う場合は、ここを合わせてください。
#define DT_DRV_COMPAT zmk_behavior_twist

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <drivers/behavior.h>

#include <zmk/behavior.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>

#include "twist_scroll.h"

LOG_MODULE_REGISTER(behavior_twist, CONFIG_ZMK_LOG_LEVEL);

// 最小実装: 押下で有効化、離しで無効化（内部フラグのみ）
static bool g_twist_enabled;

bool twist_scroll_is_enabled(void) { return g_twist_enabled; }

static void twist_scroll_enable(bool en) { g_twist_enabled = en; }

static int twist_binding_pressed(struct zmk_behavior_binding *binding,
                                 struct zmk_behavior_binding_event event)
{
    ARG_UNUSED(binding);
    ARG_UNUSED(event);
    twist_scroll_enable(true);
    return 0;
}

static int twist_binding_released(struct zmk_behavior_binding *binding,
                                  struct zmk_behavior_binding_event event)
{
    ARG_UNUSED(binding);
    ARG_UNUSED(event);
    twist_scroll_enable(false);
    return 0;
}

static const struct behavior_driver_api behavior_api = {
    .binding_pressed = twist_binding_pressed,
    .binding_released = twist_binding_released,
};

static int behavior_twist_init(const struct device *dev)
{
    ARG_UNUSED(dev);
    g_twist_enabled = false;
    return 0;
}

// インスタンス 0 を定義（必要に応じて DT 側のノード数に合わせて増やす）
BEHAVIOR_DT_INST_DEFINE(0, behavior_twist_init, NULL, NULL, NULL, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_api);
