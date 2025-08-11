zephyr_library()
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

zephyr_library_sources(
  src/behaviors/behavior_twist.c
  src/processors/twist_processor.c
)
