#ifndef _PROJECT_HEADER_H_
#define _PROJECT_HEADER_H_

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
#include "audio_element.h"
#include "audio_idf_version.h"
#include "esp_err.h"
#include "Arduino.h"
#include "codec2.h"
#include "LoRa.h"
#include <ButterworthFilter.h>
#include "driver/timer.h"
#include "FastAudioFIFO.h"

#define I2S_STREAM_CUSTOM_READ_CFG() {                                          \
    .type = AUDIO_STREAM_READER,                                                \
    .i2s_config = {                                                             \
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX),      \
        .sample_rate = 8000,                                                    \
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,                           \
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,                           \
        .communication_format = I2S_COMM_FORMAT_STAND_MSB,                      \
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL2 | ESP_INTR_FLAG_IRAM,          \
        .dma_buf_count = 2,                                                     \
        .dma_buf_len = 600,                                                     \
        .use_apll = true,                                                       \
        .tx_desc_auto_clear = true,                                             \
        .fixed_mclk = 0                                                         \
    },                                                                          \
    .i2s_port = I2S_NUM_0,                                                      \
    .use_alc = 0,                                                               \
    .volume = 0,                                                                \
    .out_rb_size = 0,                                                           \
    .task_stack = 6*1024,                                                       \
    .task_core = 1,                                                             \
    .task_prio = 1,                                                             \
    .stack_in_ext = false,                                                      \
    .multi_out_num = 0,                                                         \
    .uninstall_drv = true,                                                      \
    .need_expand = 0,                                                           \
    .expand_src_bits = I2S_BITS_PER_SAMPLE_16BIT,                               \
    .buffer_len = 600,                                                          \
}

#define I2S_STREAM_CUSTOM_WRITE_CFG() {                                         \
     .type = AUDIO_STREAM_WRITER,                                               \
    .i2s_config = {                                                             \
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX),      \
        .sample_rate = 8000,                                                    \
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,                           \
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,                           \
        .communication_format = I2S_COMM_FORMAT_STAND_MSB,                      \
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL2 | ESP_INTR_FLAG_IRAM,          \
        .dma_buf_count = 2,                                                     \
        .dma_buf_len = 600,                                                     \
        .use_apll = true,                                                       \
        .tx_desc_auto_clear = true,                                             \
        .fixed_mclk = 0                                                         \
    },                                                                          \
    .i2s_port = I2S_NUM_1,                                                      \
    .use_alc = 0,                                                               \
    .volume = 0,                                                               \
    .out_rb_size = 0,                                                           \
    .task_stack = 6*1024,                                                       \
    .task_core = 1,                                                             \
    .task_prio = 1,                                                             \
    .stack_in_ext = false,                                                      \
    .multi_out_num = 0,                                                         \
    .uninstall_drv = true,                                                      \
    .need_expand = 0,                                                           \
    .expand_src_bits = I2S_BITS_PER_SAMPLE_16BIT,                               \
    .buffer_len = 600,                                                          \
}

#define I2S_INFO()    { \
    .sample_rates = 8000,                \
    .channels = 1,                        \
    .bits = 16,                           \
    .bps = 126000,                             \
    .byte_pos = 0,                        \
    .total_bytes = 0,                     \
    .duration = 0,                        \
    .uri = NULL,                          \
    .codec_fmt = ESP_CODEC_TYPE_UNKNOW    \
}

#define READING false
#define READING_DONE true
#define WRITE_LATENCY 10

typedef struct user_struct
{
struct CODEC2* codec2_state;
int mode;
int SPEECH_BYTES;
int SPEECH_SIZE;
int FRAME_SIZE;
bool READ_FLAG;
bool WRITE_FLAG;
uint8_t* frame_bits_in;
uint8_t* frame_bits_out;
int16_t* speech_in;
int16_t* speech_out;
}my_struct;

void codec2_data_init(my_struct*);
void codec2_data_deinit(my_struct*);
static esp_err_t i2s_mono_fix(int bits, uint8_t *sbuff, uint32_t len);

#endif
