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

set(WIFI_SSID "" CACHE STRING "Wi-Fi SSID")
set(WIFI_PASSWORD "" CACHE STRING "Wi-Fi password")
if (NOT WIFI_SSID OR NOT WIFI_PASSWORD)
    message(FATAL_ERROR "WIFI_SSID or WIFI_PASSWORD not set")
endif()

set(HOST_ADDRESS "" CACHE STRING "IP address of a TCP server")
set(HOST_PORT "" CACHE STRING "Port of a TCP server")
if (NOT HOST_ADDRESS OR NOT HOST_PORT)
    message(FATAL_ERROR "HOST_ADDRESS or HOST_PORT not set")
endif()

add_executable(example_app
               main.c)
target_link_libraries(example_app PUBLIC
                      pico_stdlib
                      FreeRTOS
                      pico_fota_bootloader_lib)

pfb_compile_with_bootloader(example_app)

target_compile_definitions(example_app PRIVATE
                           WIFI_SSID=\"${WIFI_SSID}\"
                           WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
                           HOST_ADDRESS=\"${HOST_ADDRESS}\"
                           HOST_PORT=\"${HOST_PORT}\")

pico_enable_stdio_usb(example_app 1)
pico_enable_stdio_uart(example_app 0)
pico_add_extra_outputs(example_app)
