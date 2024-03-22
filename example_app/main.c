/*
 * Copyright (c) 2024 Jakub Zimnol
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "hardware/gpio.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwipopts.h"

#include "pico_fota_bootloader.h"

/**
 * Basic stringify helpers.
 */
#define _STRINGIFY(ToStr) #ToStr
#define STRINGIFY(ToStr) _STRINGIFY(ToStr)

/**
 * Very poor but working logger that uses mutex for synchronizaton.
 */
#define LOG(Module, Level, ...)                               \
    do {                                                      \
        xSemaphoreTake(log_mutex, portMAX_DELAY);             \
        printf(STRINGIFY(Level) " [" STRINGIFY(Module) "] "); \
        printf(__VA_ARGS__);                                  \
        puts("");                                             \
        xSemaphoreGive(log_mutex);                            \
    } while (0)

/**
 * Max number of bytes we can receive at once.
 */
#define MAX_RECV_DATA_SIZE (4 * PFB_ALIGN_SIZE)
/**
 * Message sent to server that indicates that we are ready to receive next chunk
 * of data.
 */
#define READY_FOR_NEXT_CHUNK_MESSAGE "Ready"

/**
 * Mutex used for synchronization in the @ref LOG() macro.
 */
static SemaphoreHandle_t log_mutex;

#define DOWNLOAD_STACK_SIZE (4000U)
static StackType_t download_task_stack[DOWNLOAD_STACK_SIZE];
static StaticTask_t download_task_buffer;

#define MAIN_APP_STACK_SIZE (4000U)
static StackType_t main_app_task_stack[MAIN_APP_STACK_SIZE];
static StaticTask_t main_app_task_buffer;

/**
 * Initializes Wi-Fi. Should be called only once from one of the tasks.
 */
static void wifi_init(void) {
    if (cyw43_arch_init()) {
        LOG(wifi, ERR, "Failed to initialise CYW43 modem");
        exit(1);
    }
    cyw43_arch_enable_sta_mode();

    LOG(wifi, INF, "Connecting to \"" WIFI_SSID "\"...");
    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD,
                                              CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        const int delay_time_ms = 1000;
        LOG(wifi, WRN,
            "Failed to connect to \"" WIFI_SSID "\", retrying in %d ms",
            delay_time_ms);
        sleep_ms(delay_time_ms);
    }

    LOG(wifi, INF, "Connected to \"" WIFI_SSID "\"");
}

/**
 * Connects to the TCP server. Returns a file descriptor socket() returned or -1
 * in case of an error.
 */
static int connect_to_server(void) {
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int ret_val;
    char s[46];
    int retval;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(HOST_ADDRESS, HOST_PORT, &hints, &servinfo)) {
        LOG(download, ERR, "getaddrinfo() failed");
        freeaddrinfo(servinfo);
        return -1;
    }

    // loop trough all the results and connect to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
            == -1) {
            LOG(download, WRN, "socket() failed");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            LOG(download, WRN, "connect() failed");
            continue;
        }
        break;
    }

    if (p == NULL) {
        close(sockfd);
        freeaddrinfo(servinfo);
        LOG(download, ERR, "Failed to connect to the TCP server");
        return -1;
    }

    inet_ntop(p->ai_family, &(((struct sockaddr_in *) p->ai_addr)->sin_addr), s,
              sizeof s);
    LOG(download, INF, "Connecting to the TCP server: %s", s);

    freeaddrinfo(servinfo);

    return sockfd;
}

/**
 * Connects to the TCP server and downloads the binary file.
 */
static int download_file(size_t *out_binary_size) {
    int len_bytes;
    uint8_t buf[MAX_RECV_DATA_SIZE];
    int sockfd;
    size_t flash_offset = 0;
    int ret = 1;

    pfb_initialize_download_slot();
    while ((sockfd = connect_to_server()) == -1) {
        vTaskDelay(10000 / portTICK_PERIOD_MS);
        LOG(download, INF, "Retrying connecting to the TCP server");
    }

    int counter = 0;
    while (1) {
        if ((len_bytes = recv(sockfd, buf, MAX_RECV_DATA_SIZE, 0)) == -1) {
            LOG(download, ERR, "recv() failed");
            break;
        }
        if (len_bytes == 0) {
            LOG(download, INF, "Connection closed");
            ret = (flash_offset == 0);
            *out_binary_size = flash_offset;
            break;
        }
        if (pfb_write_to_flash_aligned_256_bytes(buf, flash_offset,
                                                 len_bytes)) {
            LOG(download, ERR, "pfb_write_to_flash_aligned_256_bytes() failed");
            break;
        }
        flash_offset += len_bytes;
        if (send(sockfd, READY_FOR_NEXT_CHUNK_MESSAGE,
                 strlen(READY_FOR_NEXT_CHUNK_MESSAGE), 0)
            == -1) {
            LOG(download, ERR, "send() failed");
            break;
        }
        if (++counter % 10 == 0) {
            LOG(download, INF, "Downloaded %zu bytes", flash_offset);
        }
    }

    close(sockfd);
    return ret;
}

/**
 * Firmware update task. Connects to the TCP server and waits for firmware
 * chunks. This task serves development purposes only and should be improved for
 * real-world applications.
 */
static void download_task(__unused void *params) {
    LOG(download, INF, "This is the download task");
    wifi_init();

    pfb_firmware_commit();

    if (pfb_is_after_firmware_update()) {
        LOG(download, INF, "#### RUNNING ON A NEW FIRMWARE ####");
    }
    if (pfb_is_after_rollback()) {
        LOG(download, WRN, "#### ROLLBACK PERFORMED ####");
    }

    size_t binary_size;
    while (download_file(&binary_size)) {
        LOG(download, ERR, "Failed to download firmware");
    }

    LOG(download, INF, "Performing update, firmware size: %zu bytes",
        binary_size);
    pfb_mark_download_slot_as_valid();
    pfb_perform_update();
}

/**
 * Main application task.
 */
static void main_app_task(__unused void *params) {
    while (1) {
        LOG(main_app, INF,
            "This is the main app, I dunno, blink LED or something");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

int main(void) {
    stdio_init_all();

    log_mutex = xSemaphoreCreateMutex();
    if (log_mutex == NULL) {
        puts("log_mutex initialization failed");
        exit(1);
    }

    xTaskCreateStatic(download_task, "DownloadTask", DOWNLOAD_STACK_SIZE, NULL,
                      tskIDLE_PRIORITY + 1UL, download_task_stack,
                      &download_task_buffer);
    xTaskCreateStatic(main_app_task, "MainAppTask", MAIN_APP_STACK_SIZE, NULL,
                      tskIDLE_PRIORITY + 2UL, main_app_task_stack,
                      &main_app_task_buffer);

    vTaskStartScheduler();
}
