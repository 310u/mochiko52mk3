zephyr_library()

# このモジュール内のヘッダをZephyrに通す
zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# ソース登録（イベント横取り式）
zephyr_library_sources(
  src/behaviors/behavior_twist.c
  src/processors/twist_processor.c
)
