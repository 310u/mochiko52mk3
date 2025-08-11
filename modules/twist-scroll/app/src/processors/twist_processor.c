#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/logging/log.h>
#include <math.h>
#include <zmk/hid.h>
#include <zmk/event_manager.h>
#include <zmk/events/mouse_movement_state_changed.h>
#include "twist_scroll.h"

LOG_MODULE_REGISTER(twist_proc, LOG_LEVEL_INF);

/* ==== 調整パラメータ ==== */
#define LPF_ALPHA        0.12f   /* 平均ベクトルの滑らかさ */
#define MIN_SPEED        2.0f    /* 微小ノイズ無視 */
#define MAX_SPEED        100.0f
#define TWIST_THRESHOLD  6.0f    /* “回転っぽさ”外積しきい値 */
#define TWIST_DECAY_MS   40      /* 回転停止後の余韻(ms) */
#define SCROLL_SCALE     0.12f   /* スクロール感度 */
#define SCROLL_CLAMP     6       /* 1イベント最大段数 */

/* ==== トグル状態（ビヘイビアと共有） ==== */
static atomic_t g_twist_mode = ATOMIC_INIT(0);
void twist_set_mode(bool en) { atomic_set(&g_twist_mode, en ? 1 : 0); }

/* 既存 overlay の "trackball:" ノードだけを対象にする（他デバイス無視） */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(trackball), okay)
static const struct device *const trackball_dev = DEVICE_DT_GET(DT_NODELABEL(trackball));
#else
static const struct device *const trackball_dev = NULL;
#endif

static inline float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}
static inline int8_t clampi8(int v, int lo, int hi) {
    if (v < lo) v = lo;
    if (v > hi) v = hi;
    return (int8_t)v;
}

/* 状態（LPF平均、最終ツイスト時刻） */
static float mx = 0.0f, my = 0.0f;
static int64_t last_twist_ms = 0;

/* イベントを受けて、必要ならスクロールに置換 */
static int on_mouse_move(const zmk_mouse_movement_state_changed *ev) {
    if (!atomic_get(&g_twist_mode)) return 0;                 /* OFFなら無視 */
    if (trackball_dev && ev->device != trackball_dev) return 0; /* 他デバイス無視 */

    const float dx = (float)ev->dx;
    const float dy = (float)ev->dy;
    const float speed = sqrtf(dx*dx + dy*dy);

    /* 平均ベクトル(LPF)更新 */
    mx = (1.0f - LPF_ALPHA) * mx + LPF_ALPHA * dx;
    my = (1.0f - LPF_ALPHA) * my + LPF_ALPHA * dy;

    /* “擬似Z軸” = v × m */
    const float z = dx * my - dy * mx;

    const bool twist_like = (speed >= MIN_SPEED) && (speed <= MAX_SPEED) && (fabsf(z) >= TWIST_THRESHOLD);
    const int64_t now = k_uptime_get();
    const bool twist_active = twist_like || (now - last_twist_ms) < TWIST_DECAY_MS;
    if (twist_like) last_twist_ms = now;

    if (twist_active) {
        /* スクロール化（符号が回転方向） */
        float steps_f = clampf(z * SCROLL_SCALE, -(float)SCROLL_CLAMP, (float)SCROLL_CLAMP);
        int8_t steps = clampi8((int)lroundf(steps_f), -SCROLL_CLAMP, SCROLL_CLAMP);
        if (steps) {
            /* 垂直スクロールのみ。逆向きなら -steps に */
            zmk_hid_mouse_scroll(steps, 0);
            /* この移動は“消費”してポインタに流さない */
            return 1; /* consumed */
        }
    }
    return 0;
}

/* リスナー登録 */
ZMK_LISTENER(twist_scroll_listener, on_mouse_move);
ZMK_SUBSCRIPTION(twist_scroll_listener, mouse_movement_state_changed)
