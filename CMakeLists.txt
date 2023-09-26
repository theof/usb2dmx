cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)
project(usb2dmx)
pico_sdk_init()
include(lib/Pico-DMX/interfaceLibForPicoSDK.cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(WARN_CFLAGS -Wall -Wextra)
set(FAMILY rp2040)
set(BOARD pico_sdk)

add_executable(firmware)
target_sources(firmware PUBLIC
	${CMAKE_CURRENT_LIST_DIR}/main.cpp
	${CMAKE_CURRENT_LIST_DIR}/usb_descriptors.c
)
target_include_directories(firmware PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_compile_options(firmware PRIVATE ${WARN_CFLAGS})
target_link_libraries(firmware pico_stdlib hardware_pio hardware_dma tinyusb_device tinyusb_board picodmx pico_unique_id)
pico_add_extra_outputs(firmware)
