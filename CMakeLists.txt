# Copyright (c) 2024 Jakub Zimnol
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico_w)
set(PICO_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/../pico-sdk"
    CACHE PATH "Path to the Raspberry Pi Pico SDK")
set(FREERTOS_KERNEL_PATH "${CMAKE_CURRENT_LIST_DIR}/../FreeRTOS-Kernel"
    CACHE PATH "Path to the FreeRTOS kernel sources")

include(pico_sdk_import.cmake)
include(FreeRTOS_Kernel_import.cmake)

project(example_app)
pico_sdk_init()

file(GLOB FREERTOS_SOURCES ${FREERTOS_KERNEL_PATH}/*.c)

add_library(FreeRTOS
            ${FREERTOS_SOURCES}
            ${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/port.c
            ${CMAKE_CURRENT_LIST_DIR}/port/freertos.c)
target_include_directories(FreeRTOS PUBLIC
                           ${FREERTOS_KERNEL_PATH}/include
                           ${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/include
                           ${CMAKE_CURRENT_LIST_DIR}/port)
target_link_libraries(FreeRTOS
                      pico_cyw43_arch_lwip_sys_freertos
                      FreeRTOS-Kernel-Heap4)

add_subdirectory(pico_fota_bootloader)
add_subdirectory(example_app)
