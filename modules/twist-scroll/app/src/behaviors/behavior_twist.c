// modules/twist-scroll/app/src/behaviors/behavior_twist.c
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

// ★ これを追加（behavior_driver_api の定義元）
#include <drivers/behavior.h>

#include <zmk/behavior.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>

#include "twist_scroll.h"

LOG_MODULE_REGISTER(behavior_twist, CONFIG_ZMK_LOG_LEVEL);

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
    return 0;
}

BEHAVIOR_DT_INST_DEFINE(0, behavior_twist_init, NULL, NULL, NULL, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_api);
