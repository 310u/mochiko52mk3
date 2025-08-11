zephyr_library()

# このモジュール内のヘッダ
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# ソース登録（必要に応じて入れ替えOK）
zephyr_library_sources(
  src/features/twist_scroll.c
  src/behaviors/behavior_twist.c
  src/sensors/pmw3360_spi.c
)
