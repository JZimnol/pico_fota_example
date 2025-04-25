# Raspberry Pi Pico W FOTA Example Application

This repository showcases an example usage of
[pico_fota_bootloader](https://github.com/JZimnol/pico_fota_bootloader.git) - a
bootloader that enables secure `Firmware Over-The-Air (FOTA)` OTA updates on
Raspberry Pi Pico W.

**NOTE:** This repository contains the most basic versions of the TCP client
and TCP server which should be improved to create a more safe application.

**NOTE:** This example assumes that this repository is in the same folder as
[FreeRTOS-Kernel](https://github.com/FreeRTOS/FreeRTOS-Kernel.git) and
[pico-sdk](https://github.com/raspberrypi/pico-sdk.git). If not, change the
`PICO_SDK_PATH` and `FREERTOS_KERNEL_PATH` variables in the `CMakeLists.txt`
file accordingly.

### Other example applications

- [Pico W HTTP client](https://github.com/jondurrant/RPIPicoW-OTA-Exp) created
  by [Jon Durrant](https://github.com/jondurrant)
- [Firmware Update LwM2M Object](https://github.com/AVSystem/Anjay-pico-client/tree/master/firmware_update)
  of
  [Anjay-pico-client](https://github.com/AVSystem/Anjay-pico-client/tree/master)

## Repository contents

The repository consists of:
- `tcp_server.py` - a simple Python script that creates a TCP server and sends
  the contents of a user-specified file to the connected device. Run `python3
  tcp_server.py --help` for more information.
- `example_app/main.c` - a basic Raspberry Pi Pico W application that operates
  with two distinct threads – the main application thread and the 'downloader'
  thread. The 'downloader' thread periodically attempts to connect to the TCP
  server and awaits the file contents.

## Compiling the application

To compile the application, run the following commands:

```shell
# these commands may vary depending on the OS
mkdir build/
cd build
cmake -DWIFI_SSID='<ssid>' -DWIFI_PASSWORD='<password>' -DHOST_ADDRESS='<address>' -DHOST_PORT='<port>' -DPFB_AES_KEY="<your_key_value>" ..
make -j
```

where:
- `<ssid>` - SSID of the Wi-Fi Raspberry Pi Pico W should connect to
- `<password>` - password of the Wi-Fi Raspberry Pi Pico W should connect to
- `<address>` - IP address of the host (your PC) Raspberry Pi Pico W should
  connect to to download the binary file
- `<port>` - port of the host (your PC) Raspberry Pi Pico W should connect to to
  download the binary file

(for more information about compilation options, see [pico_fota_bootloader
README](https://github.com/JZimnol/pico_fota_bootloader/blob/master/README.md))

You should have output similar to:

```
build/
├── deps/
|   └── pico_fota_bootloader/
|       ├── CMakeFiles
|       ├── cmake_install.cmake
|       ├── elf2uf2
|       ├── libpico_fota_bootloader_lib.a
|       ├── Makefile
|       ├── pico_fota_bootloader.bin
|       ├── pico_fota_bootloader.dis
|       ├── pico_fota_bootloader.elf
|       ├── pico_fota_bootloader.elf.map
|       ├── pico_fota_bootloader.hex
|       └── pico_fota_bootloader.uf2
└── example_app/
    ├── CMakeFiles
    ├── cmake_install.cmake
    ├── example_app.bin
    ├── example_app.dis
    ├── example_app.elf
    ├── example_app.elf.map
    ├── example_app_fota_image.bin
    ├── example_app_fota_image_encrypted.bin
    ├── example_app.hex
    ├── example_app.uf2
    └── Makefile
```

### Running the application

Set Pico W to the BOOTSEL state (by powering it up with the `BOOTSEL` button
pressed) and copy the `pico_fota_bootloader.uf2` file into it. Right now the
Pico W is flashed with the bootloader but does not have proper application in
the application FLASH memory slot. Then, set Pico W to the `BOOTSEL` state
again (if it is not already in that state) and copy the `example_app.uf2` file.
The board should reboot and start `example_app` application.

**NOTE:** you can also flash the board with `PicoProbe` using files
`pico_fota_bootloader.elf` and `example_app.elf`.

Open the serial console. The output should be similar to:

```
***********************************************************
*                                                         *
*           Raspberry Pi Pico W FOTA Bootloader           *
*             Copyright (c) 2024 Jakub Zimnol             *
*                                                         *
***********************************************************

[BOOTLOADER] Nothing to swap
[BOOTLOADER] End of execution, executing the application...

WRN [wifi] Failed to connect to "SSID", retrying in 1000 ms
INF [main_app] This is the main app, I dunno, blink LED or something
WRN [wifi] Failed to connect to "SSID", retrying in 1000 ms
INF [main_app] This is the main app, I dunno, blink LED or something
INF [main_app] This is the main app, I dunno, blink LED or something
INF [wifi] Connected to "SSID"
WRN [download] connect() failed
ERR [download] Failed to connect to the TCP server
INF [main_app] This is the main app, I dunno, blink LED or something
...
```

The connection to the host machine will fail because the TCP server is not set
up yet. Now, for example, change one of the log messages in the
`example_app/main.c` file and recompile the application. You will create a
"new" `example_app_fota_image_encrypted.bin` (or `example_app_fota_image.bin`
if compiled without image encryption) that will be sent to Raspberry Pi Pico W
after running the TCP server.

## Running the TCP server and performing the Firmware Update Over The Air

To run the TCP server and send the binary file to Raspberry Pi Pico W, you can
use the `tcp_server.py` file. Run the following command:

```shell
python3 tcp_server.py --binary-file <path> --port <port>
```
where:
- `--binary-file <path>` - path to the binary file that will be sent to
  Raspberry Pi Pico W (optional, default:
  `./build/example_app/example_app_fota_image_encrypted.bin`)
- `--port <port>` - port of the server (optional, default: `3490`)

In the simplest case, just run:

```shell
python3 tcp_server.py
```

The following prompt will appear:

```
Using binary path: ./build/example_app/example_app_fota_image_encrypted.bin
Using port: 3490
Waiting for connections...
```

After some time, Raspberry Pi Pico W should connect to the TCP server:

```
Got connection from (<pico_address>)
Press Enter to trigger update:
```

After pressing `Enter`, the TCP server will begin to send the specified file to
the Raspberry Pi Pico W. You should see the following logs in the terminal:

```
Sending 1024 bytes to (<pico_address>): sent = 1024 bytes
Sending 1024 bytes to (<pico_address>): sent = 2048 bytes
...
Sending 1024 bytes to (<pico_address>): sent = 351232 bytes
Sending 256 bytes to (<pico_address>): sent = 351488 bytes
Closing ./build/example_app/example_app_fota_image_encrypted.bin
```

And in the Raspberry Pi Pico W serial output:

```
INF [download] Connecting to the TCP server: <host_address>
INF [download] Downloaded 10240 bytes
INF [download] Downloaded 20480 bytes
...
INF [download] Downloaded 348160 bytes
INF [download] Connection closed
INF [download] SHA256 matches
INF [download] Performing update, firmware size: 351488 bytes

***********************************************************
*                                                         *
*           Raspberry Pi Pico W FOTA Bootloader           *
*             Copyright (c) 2024 Jakub Zimnol             *
*                                                         *
***********************************************************

[BOOTLOADER] Swapping images
[BOOTLOADER] End of execution, executing the application...

INF [main_app] This is the main app, I dunno, blink LED or something
INF [wifi] Connected to "SSID"
INF [download] #### RUNNING ON A NEW FIRMWARE ####
```

Right now, the Raspberry Pi Pico W is running on a new firmware downloaded via
TCP. Experiment with the `main_app_task` function, recompile the application
and perform FOTA once again.
