
#include "Arduino.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_common.h"
#include "board.h"
#include "ringbuf.h"
#include "esp_peripherals.h"
#include "i2s_stream.h"
#include "wav_encoder.h"
#include "wav_decoder.h"
#include "audio_idf_version.h"
#include "BluetoothSerial.h"
#include "codec2.h"
#include "LoRa.h"
#include <ButterworthFilter.h>


#define I2S_STREAM_CUSTOM_READ_CFG() {                                          \
    .type = AUDIO_STREAM_READER,                                                \
    .i2s_config = {                                                             \
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX),      \
        .sample_rate = 8000,                                                    \
        .bits_per_sample = I2S_BITS_PER_SAMPLE_8BIT,                            \
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,                           \
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,                      \
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
    .out_rb_size = I2S_STREAM_RINGBUFFER_SIZE,                                  \
    .task_stack = I2S_STREAM_TASK_STACK,                                        \
    .task_core = I2S_STREAM_TASK_CORE,                                          \
    .task_prio = I2S_STREAM_TASK_PRIO,                                          \
    .stack_in_ext = false,                                                      \
    .multi_out_num = 0,                                                         \
    .uninstall_drv = true,                                                      \
    .need_expand = false,                                                       \
    .expand_src_bits = I2S_BITS_PER_SAMPLE_8BIT,                                \
    .buffer_len = 900,                                                          \
}

#define SPEECH_BUFFER_SIZE 160

static const char *TAG = "MONITORING";
int8_t speech[SPEECH_BUFFER_SIZE];
TaskHandle_t TaskHandle = NULL;
void * context_ptr = NULL;


extern "C" void app_main()
{
    audio_element_handle_t i2s_reader;
    ringbuf_handle_t speech_read_buffer = rb_create(SPEECH_BUFFER_SIZE,1);
    audio_board_handle_t board_handle = audio_board_init();

    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set(TAG, ESP_LOG_INFO);
    ESP_LOGI(TAG, "Created Handles \n");

    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_BOTH, AUDIO_HAL_CTRL_START);
    ESP_LOGI(TAG, "Init codec chip\n");

    i2s_stream_cfg_t i2s_read_cfg = I2S_STREAM_CUSTOM_READ_CFG();
    i2s_reader = i2s_stream_init(&i2s_read_cfg);
    audio_element_set_output_ringbuf(i2s_reader, speech_read_buffer);
    ESP_LOGI(TAG, "Configured I2S stream read \n");

    initArduino();
    Serial.begin(115200);
    while(!Serial){;}
    ESP_LOGI(TAG, "Init Arduino \n");

    audio_event_iface_cfg_t event_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t read_event = audio_event_iface_init(&event_cfg);
    ESP_LOGI(TAG, "Configured event listener \n");

    audio_element_msg_set_listener(i2s_reader, read_event);
    ESP_LOGI(TAG, "Set listener event from pipeline \n");

    esp_err_t i2s_init = audio_element_run(audio_element_handle_t i2s_reader);
    if (i2s_init == ESP_OK) ESP_LOGI(TAG, "I2S READER STARTED \n");
    else ESP_LOGI(TAG, "I2S READER FAIL! \n");

    while(1) 
    {
        
    }
    
    ESP_LOGI(TAG, " Stopped audio_pipeline"); // сброс всех процессов
    audio_element_stop(i2s_reader);
    audio_element_wait_for_stop(i2s_reader);
    audio_element_terminate(i2s_reader);
    audio_element_remove_listener(i2s_reader);
    audio_element_deinit(i2s_reader); 
    }