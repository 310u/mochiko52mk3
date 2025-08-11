#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/logging/log.h>
#include <math.h>
#include "twist_scroll.h"

/* ▼ZMKのHID API：環境で名前が違う場合があります。
 *  - 代表例: zmk_hid_mouse_scroll(int8_t vertical, int8_t horizontal)
 *            zmk_hid_mouse_move(int16_t dx, int16_t dy)
 *  ビルドで未定義と言われたら、下の emit_*() の中だけ直せばOK。
 */
#include <zmk/hid.h>

LOG_MODULE_REGISTER(twist, LOG_LEVEL_INF);

/* PMW3360 から dx/dy を取得するラッパ（pmw3360_spi.c で実装） */
int pmw3360_get_dxdy(int16_t *dx, int16_t *dy);

/* パラメータ（まずはデフォルトのまま→後で微調整） */
#define POLL_HZ                1000   /* 1kHzポーリング */
#define LPF_ALPHA              0.12f  /* 低域通過フィルタ係数 */
#define MIN_SPEED              2.0f   /* 微小ノイズ除去 */
#define MAX_SPEED              100.0f
#define TWIST_THRESHOLD        6.0f   /* 回転とみなす外積しきい値 */
#define TWIST_DECAY_MS         40     /* 回転停止後の惰性(ms) */
#define SCROLL_SCALE           0.12f  /* スクロール感度 */
#define SCROLL_CLAMP           6      /* 1tick当たり最大ホイール段数 */

static atomic_t g_twist_mode = ATOMIC_INIT(0);

static inline float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}
static int8_t clampi8(int v, int lo, int hi) {
    if (v < lo) v = lo;
    if (v > hi) v = hi;
    return (int8_t)v;
}

/* ここだけZMK API名を調整すれば他は触らなくてOK */
static void emit_scroll(int8_t vertical_steps, int8_t horizontal_steps) {
    /* 垂直スクロールのみ使うなら2引数目は0でOK */
    zmk_hid_mouse_scroll(vertical_steps, horizontal_steps);
}
static void emit_move(int16_t dx, int16_t dy) {
    zmk_hid_mouse_move(dx, dy);
}

void twist_set_mode(bool enabled) {
    atomic_set(&g_twist_mode, enabled ? 1 : 0);
}

static void twist_scroll_thread(void) {
    const k_timeout_t period = K_USEC(1000000 / POLL_HZ);

    float mx = 0.0f, my = 0.0f;    /* 直近平均ベクトル(LPF) */
    int64_t last_twist_ms = 0;

    while (1) {
        int16_t dx_i16 = 0, dy_i16 = 0;
        (void)pmw3360_get_dxdy(&dx_i16, &dy_i16);

        float dx = (float)dx_i16;
        float dy = (float)dy_i16;
        float speed = sqrtf(dx * dx + dy * dy);

        /* 平均ベクトル更新（LPF） */
        mx = (1.0f - LPF_ALPHA) * mx + LPF_ALPHA * dx;
        my = (1.0f - LPF_ALPHA) * my + LPF_ALPHA * dy;

        /* 擬似Z成分（外積）: v × m */
        float z = dx * my - dy * mx;

        bool twist_mode = atomic_get(&g_twist_mode) == 1;
        bool twist_like = (speed >= MIN_SPEED) && (speed <= MAX_SPEED) && (fabsf(z) >= TWIST_THRESHOLD);

        int64_t now = k_uptime_get();
        bool twist_active = twist_like || (now - last_twist_ms) < TWIST_DECAY_MS;

        if (twist_mode && twist_active) {
            if (twist_like) last_twist_ms = now;

            /* 回転量→スクロール段数（符号で方向を決定） */
            float steps_f = clampf(z * SCROLL_SCALE, -(float)SCROLL_CLAMP, (float)SCROLL_CLAMP);
            int8_t steps = clampi8((int)lroundf(steps_f), -SCROLL_CLAMP, SCROLL_CLAMP);

            if (steps != 0) {
                emit_scroll(steps, 0);
            }
            /* ツイスト中はポインタを止める（SlimBladeっぽく） */
        } else {
            /* 通常モードはふつうにポインタ移動 */
            if (!twist_mode && (dx_i16 || dy_i16)) {
                emit_move(dx_i16, dy_i16);
            }
        }

        k_sleep(period);
    }
}

/* スレッド生成 */
K_THREAD_DEFINE(twist_scroll_tid, 2048, twist_scroll_thread, NULL, NULL, NULL,
                K_PRIO_PREEMPT(8), 0, 0);
