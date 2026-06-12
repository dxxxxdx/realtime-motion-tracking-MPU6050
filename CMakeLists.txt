cmake_minimum_required(VERSION 3.20)
project(realtime_motion_tracking_total)

# 告诉 CLion，我这底下有多个独立的固件方案，别漏了
add_subdirectory(ch572-ble)
add_subdirectory(ch592-ble)
add_subdirectory(STM32SLN)