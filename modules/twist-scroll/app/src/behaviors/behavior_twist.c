#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zmk/behavior.h>
#include "twist_scroll.h"

/* トグル用ビヘイビア。押すたびにON/OFF切替 */
LOG_MODULE_REGISTER(beh_twist, LOG_LEVEL_INF);

struct behavior_twist_config {};
struct behavior_twist_data {};

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    static bool mode;
    mode = !mode;
    twist_set_mode(mode);
    LOG_INF("Twist mode: %s", mode ? "ON" : "OFF");
    return ZMK_BEHAVIOR_OPAQUE; /* 他へは流さない */
}

static const struct behavior_driver_api behavior_api = {
    .binding_pressed = on_keymap_binding_pressed,
};

static int behavior_twist_init(const struct device *dev) {
    ARG_UNUSED(dev);
    return 0;
}

/* Devicetree binding: compatible = "zmk,behavior-twist-toggle" 用 */
#define DT_DRV_COMPAT zmk_behavior_twist_toggle

#define INST(n) \
    static struct behavior_twist_data data_##n; \
    static const struct behavior_twist_config config_##n = {}; \
    DEVICE_DT_INST_DEFINE(n, behavior_twist_init, NULL, \
                          &data_##n, &config_##n, \
                          APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, \
                          &behavior_api);

DT_INST_FOREACH_STATUS_OKAY(INST)
