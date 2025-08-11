#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/gpio/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pmw3360, LOG_LEVEL_INF);

/* これは「最小読み出しの骨格」です。
 * 実機では SROM ダウンロードなど初期化手順が必要なので、
 * あなたの既存PMW3360実装があれば そちらで置き換え、
 * 最終的に pmw3360_get_dxdy() だけ提供して下さい。
 */

/* DTノード:
 * - SPIコントローラ: 例) DT_NODELABEL(spi1)
 * - PMW3360デバイス: DT_NODELABEL(pmw3360) … overlayで定義しておく
 */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(spi1), okay)
#define SPI_DEV_NODE DT_NODELABEL(spi1)
#else
#error "spi1 が有効化されていません（overlayを確認してください）"
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(pmw3360), okay)
#define PMW_NODE DT_NODELABEL(pmw3360)
#else
#error "pmw3360 ノードが見つかりません（overlayを確認してください）"
#endif

static const struct device *spi_dev;
static struct spi_cs_control cs_ctrl;
static struct spi_config spi_cfg;

static int spi_read_reg(uint8_t reg, uint8_t *val) {
    /* PMW3360: 先頭bit=0でRead */
    uint8_t tx = (uint8_t)(reg & 0x7F);
    struct spi_buf txb = { .buf = &tx, .len = 1 };
    struct spi_buf_set txs = { .buffers = &txb, .count = 1 };

    struct spi_buf rxb = { .buf = val, .len = 1 };
    struct spi_buf_set rxs = { .buffers = &rxb, .count = 1 };

    int rc = spi_transceive(spi_dev, &spi_cfg, &txs, &rxs);
    /* データシートのタイミングに合わせて適宜 wait を入れる */
    k_busy_wait(100);
    return rc;
}

struct pmw_sample { int16_t dx, dy; bool motion; };

static int pmw_read_motion(struct pmw_sample *o) {
    /* 最小読み出し：Motion→DX_L/H→DY_L/H */
    uint8_t motion, dxl, dxh, dyl, dyh;
    if (spi_read_reg(0x02, &motion)) return -EIO;          /* Motion */
    if (!(motion & 0x80)) { o->motion=false; o->dx=o->dy=0; return 0; }
    if (spi_read_reg(0x03, &dxl)) return -EIO;             /* DX_L */
    if (spi_read_reg(0x04, &dxh)) return -EIO;             /* DX_H */
    if (spi_read_reg(0x05, &dyl)) return -EIO;             /* DY_L */
    if (spi_read_reg(0x06, &dyh)) return -EIO;             /* DY_H */

    o->dx = (int16_t)((dxh << 8) | dxl);
    o->dy = (int16_t)((dyh << 8) | dyl);
    o->motion = true;
    return 0;
}

/* 外部公開: dx/dyを返す（失敗時はrc<0） */
int pmw3360_get_dxdy(int16_t *dx, int16_t *dy) {
    struct pmw_sample s;
    int rc = pmw_read_motion(&s);
    if (rc) return rc;
    *dx = s.dx; *dy = s.dy;
    return 0;
}

static int pmw_init(const struct device *dev) {
    ARG_UNUSED(dev);

    spi_dev = DEVICE_DT_GET(SPI_DEV_NODE);
    if (!device_is_ready(spi_dev)) return -ENODEV;

    /* pmw3360 ノードから CS と周波数を読み取る（overlayと一致させる） */
    const struct gpio_dt_spec cs = GPIO_DT_SPEC_GET(PMW_NODE, cs_gpios);
    cs_ctrl.gpio = cs;
    cs_ctrl.delay = 5;

    spi_cfg.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB;
    spi_cfg.frequency = DT_PROP(PMW_NODE, spi_max_frequency);
    spi_cfg.cs = &cs_ctrl;

    /* ※実機ではここで SROM ダウンロード等の初期化を実装してください。 */
    LOG_INF("PMW3360 init (stub)");
    return 0;
}

/* アプリ初期化タイミングで呼ぶ */
SYS_INIT(pmw_init, APPLICATION, 50);
