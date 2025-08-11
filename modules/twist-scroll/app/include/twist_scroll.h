// modules/twist-scroll/app/include/twist_scroll.h
#pragma once
#include <stdbool.h>

// 最小 API（内部状態の問い合わせのみ）
bool twist_scroll_is_enabled(void);

// 将来的に外部から明示的に切替えたい場合は、必要になった時点で
// 下の宣言を公開し、実装を behavior_twist.c から移す。
// void twist_scroll_enable(bool en);
