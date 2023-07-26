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

#define I2S_STREAM_CUSTOM_READ_CFG() {                                          \
    .type = AUDIO_STREAM_READER,                                                \
    .i2s_config = {                                                             \
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX),      \
        .sample_rate = 8000,                                                    \
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,                           \
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,                           \
        .communication_format = I2S_COMM_FORMAT_STAND_MSB,                      \
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL2 | ESP_INTR_FLAG_IRAM,          \
        .dma_buf_count = 2,                                                     \
        .dma_buf_len = 200,                                                     \
        .use_apll = true,                                                       \
        .tx_desc_auto_clear = true,                                             \
        .fixed_mclk = 0                                                         \
    },                                                                          \
    .i2s_port = I2S_NUM_0,                                                      \
    .use_alc = 0,                                                               \
    .volume = 0,                                                                \
    .out_rb_size = 0,                                                           \
    .task_stack = 4*1024,                                                       \
    .task_core = 0,                                                             \
    .task_prio = 3,                                                             \
    .stack_in_ext = false,                                                      \
    .multi_out_num = 0,                                                         \
    .uninstall_drv = true,                                                      \
    .need_expand = 0,                                                           \
    .expand_src_bits = I2S_BITS_PER_SAMPLE_16BIT,                               \
    .buffer_len = 300,                                                          \
}

#define I2S_STREAM_CUSTOM_WRITE_CFG() {                                         \
     .type = AUDIO_STREAM_WRITER,                                               \
    .i2s_config = {                                                             \
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX),      \
        .sample_rate = 8000,                                                    \
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,                           \
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,                           \
        .communication_format = I2S_COMM_FORMAT_STAND_MSB,                      \
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL2 | ESP_INTR_FLAG_IRAM,          \
        .dma_buf_count = 2,                                                     \
        .dma_buf_len = 200,                                                     \
        .use_apll = true,                                                       \
        .tx_desc_auto_clear = true,                                             \
        .fixed_mclk = 0                                                         \
    },                                                                          \
    .i2s_port = I2S_NUM_1,                                                      \
    .use_alc = 0,                                                               \
    .volume = 0,                                                               \
    .out_rb_size = 0,                                                           \
    .task_stack = 4*1024,                                                       \
    .task_core = 0,                                                             \
    .task_prio = 3,                                                             \
    .stack_in_ext = false,                                                      \
    .multi_out_num = 0,                                                         \
    .uninstall_drv = true,                                                      \
    .need_expand = 0,                                                           \
    .expand_src_bits = I2S_BITS_PER_SAMPLE_16BIT,                               \
    .buffer_len = 300,                                                          \
}

#define CDC2_INFO()                     { \
    .sample_rates = 8000,                 \
    .channels = 2,                        \
    .bits = 16,                           \
    .bps = 248000,                        \
    .byte_pos = 0,                        \
    .total_bytes = 0,                     \
    .duration = 0,                        \
    .uri = NULL,                          \
    .codec_fmt = ESP_CODEC_TYPE_UNKNOW    \
}

#define I2S_INFO()                      { \
    .sample_rates = 8000,                 \
    .channels = 2,                        \
    .bits = 16,                           \
    .bps = 248000,                        \
    .byte_pos = 0,                        \
    .total_bytes = 0,                     \
    .duration = 0,                        \
    .uri = NULL,                          \
    .codec_fmt = ESP_CODEC_TYPE_UNKNOW    \
}

typedef struct user_struct
{
struct CODEC2* codec2_state;
int mode;
int SPEECH_SIZE;
int FRAME_SIZE;
uint8_t* frame_bits_in;
uint8_t* frame_bits_out;
int16_t* speech_in;
int16_t* speech_out;
}my_struct;

void codec2_data_init(my_struct*);
void codec2_data_deinit(my_struct*);

static esp_err_t codec2_enc_open(audio_element_handle_t self);
static audio_element_err_t codec2_enc_process(audio_element_handle_t self, char *in_buffer, int in_len);
static esp_err_t codec2_enc_close(audio_element_handle_t self);
static esp_err_t codec2_enc_destroy(audio_element_handle_t self);
audio_element_handle_t encoder2_element_init(audio_element_cfg_t *codec2_enc_cfg);

static esp_err_t codec2_dec_open(audio_element_handle_t self);
static audio_element_err_t codec2_dec_process(audio_element_handle_t self, char *in_buffer, int in_len);
static esp_err_t codec2_dec_close(audio_element_handle_t self);
static esp_err_t codec2_dec_destroy(audio_element_handle_t self);
audio_element_handle_t decoder2_element_init(audio_element_cfg_t *codec2_dec_cfg);

#endif
