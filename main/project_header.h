#ifndef _PROJECT_HEADER_H_
#define _PROJECT_HEADER_H_

#include "codec2_user.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_common.h"
#include "ringbuf.h"
#include "board.h"
#include "esp_peripherals.h"
#include "i2s_stream.h"


#define I2S_STREAM_CUSTOM_READ_CFG() {                                          \
    .type = AUDIO_STREAM_READER,                                                \
    .i2s_config = {                                                             \
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX),      \
        .sample_rate = 8000,                                                    \
        .bits_per_sample = I2S_BITS_PER_SAMPLE_8BIT,                            \
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,                           \
        .communication_format = I2S_COMM_FORMAT_STAND_PCM_SHORT,                \
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL2 | ESP_INTR_FLAG_IRAM,          \
        .dma_buf_count = 3,                                                     \
        .dma_buf_len = 900,                                                     \
        .use_apll = true,                                                       \
        .tx_desc_auto_clear = true,                                             \
        .fixed_mclk = 0                                                         \
    },                                                                          \
    .i2s_port = I2S_NUM_0,                                                      \
    .use_alc = 0,                                                               \
    .volume = 0,                                                                \
    .out_rb_size = 0,                                                           \
    .task_stack = 3072,                                                         \
    .task_core = 1,                                                             \
    .task_prio = 1,                                                             \
    .stack_in_ext = false,                                                      \
    .multi_out_num = 0,                                                         \
    .uninstall_drv = true,                                                      \
    .need_expand = false,                                                       \
    .expand_src_bits = I2S_BITS_PER_SAMPLE_8BIT,                                \
    .buffer_len = 900,                                                          \
}

#define I2S_STREAM_CUSTOM_WRITE_CFG() {                                         \
     .type = AUDIO_STREAM_WRITER,                                               \
    .i2s_config = {                                                             \
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX),      \
        .sample_rate = 8000,                                                    \
        .bits_per_sample = I2S_BITS_PER_SAMPLE_8BIT,                            \
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,                           \
        .communication_format = I2S_COMM_FORMAT_STAND_PCM_SHORT,                \
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL2 | ESP_INTR_FLAG_IRAM,          \
        .dma_buf_count = 3,                                                     \
        .dma_buf_len = 900,                                                     \
        .use_apll = true,                                                       \
        .tx_desc_auto_clear = true,                                             \
        .fixed_mclk = 0                                                         \
    },                                                                          \
    .i2s_port = I2S_NUM_1,                                                      \
    .use_alc = 0,                                                               \
    .volume = 50,                                                               \
    .out_rb_size = 0,                                                           \
    .task_stack = 3072,                                                         \
    .task_core = 1,                                                             \
    .task_prio = 1,                                                             \
    .stack_in_ext = false,                                                      \
    .multi_out_num = 0,                                                         \
    .uninstall_drv = true,                                                      \
    .need_expand = false,                                                       \
    .expand_src_bits = I2S_BITS_PER_SAMPLE_8BIT,                                \
    .buffer_len = 900,                                                          \
}

#define SPEECH_BUFFER_SIZE 160
#define ENCODE_FRAME_SIZE 8
static char *TAG = "MONITORING";
#endif
