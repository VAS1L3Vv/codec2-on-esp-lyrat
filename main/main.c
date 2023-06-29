/* Record file to SD Card

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

// the decoder element to decode it, and then the I2S element to output the musi
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
#include "esp_peripherals.h"
#include "i2s_stream.h"
#include "wav_encoder.h"
#include "wav_decoder.h"
#include "audio_idf_version.h"

#define I2S_STREAM_CUSTOM_READ_CFG() {                                          \
    .type = AUDIO_STREAM_READER,                                                \
    .i2s_config = {                                                             \
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX),      \
        .sample_rate = 22050,                                                   \
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,                            \
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,                            \
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,                      \
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL2 | ESP_INTR_FLAG_IRAM,          \
        .dma_buf_count = 2,                                                     \
        .dma_buf_len = 900,                                                     \
        .use_apll = true,                                                       \
        .tx_desc_auto_clear = true,                                             \
        .fixed_mclk = 0                                                         \
    },                                                                          \
    .i2s_port = I2S_NUM_0,                                                      \
    .use_alc = 0,                                                           \
    .volume = 3,                                                                \
    .out_rb_size = I2S_STREAM_RINGBUFFER_SIZE,                                  \
    .task_stack = I2S_STREAM_TASK_STACK,                                        \
    .task_core = I2S_STREAM_TASK_CORE,                                          \
    .task_prio = I2S_STREAM_TASK_PRIO,                                          \
    .stack_in_ext = false,                                                      \
    .multi_out_num = 0,                                                         \
    .uninstall_drv = true,                                                      \
    .need_expand = false,                                                       \
    .expand_src_bits = I2S_BITS_PER_SAMPLE_16BIT,                               \
    .buffer_len = 900,                                          \
}


#define I2S_STREAM_CUSTOM_WRITE_CFG() {                                         \
     .type = AUDIO_STREAM_WRITER,                                                \
    .i2s_config = {                                                             \
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX),      \
        .sample_rate = 22050,                                                   \
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,                           \
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,                           \
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,                      \
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL2 | ESP_INTR_FLAG_IRAM,          \
        .dma_buf_count = 2,                                                     \
        .dma_buf_len = 900,                                                     \
        .use_apll = true,                                                       \
        .tx_desc_auto_clear = true,                                             \
        .fixed_mclk = 0                                                         \
    },                                                                          \
    .i2s_port = I2S_NUM_1,                                                      \
    .use_alc = 0,                                                          \
    .volume = 50,                                                                \
    .out_rb_size = I2S_STREAM_RINGBUFFER_SIZE,                                  \
    .task_stack = I2S_STREAM_TASK_STACK,                                        \
    .task_core = 1,                                          \
    .task_prio = I2S_STREAM_TASK_PRIO,                                          \
    .stack_in_ext = false,                                                      \
    .multi_out_num = 0,                                                         \
    .uninstall_drv = true,                                                      \
    .need_expand = false,                                                       \
    .expand_src_bits = I2S_BITS_PER_SAMPLE_16BIT,                               \
    .buffer_len = 900,                                          \
}


static const char *TAG = "monitoring";
void app_main(void)
{
    // handles
    audio_pipeline_handle_t pipeline; 
    audio_element_handle_t i2s_reader, i2s_writer;
    //  , wav_encoder, wav_decoder;
    audio_board_handle_t board_handle = audio_board_init();

    // set log
    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set(TAG, ESP_LOG_INFO);

    // Initialize peripherals
    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);

    // codec chip config and init
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_BOTH, AUDIO_HAL_CTRL_START);
    ESP_LOGI(TAG, "1) Configured and initialised codec chip");

    // pipeline init
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    mem_assert(pipeline);
    ESP_LOGI(TAG, "2) Initialised pipeline");

    // i2s read config
    i2s_stream_cfg_t i2s_read_cfg = I2S_STREAM_CUSTOM_READ_CFG();
    i2s_read_cfg.type = AUDIO_STREAM_READER;
    i2s_read_cfg.i2s_port = I2S_NUM_0;
    i2s_reader = i2s_stream_init(&i2s_read_cfg);
    ESP_LOGI(TAG, "3) Configured I2S stream read");

    // i2s write config
    i2s_stream_cfg_t i2s_write_cfg = I2S_STREAM_CUSTOM_WRITE_CFG();
    i2s_write_cfg.type = AUDIO_STREAM_WRITER;
    i2s_write_cfg.i2s_port = I2S_NUM_1;
    i2s_writer = i2s_stream_init(&i2s_write_cfg);
    ESP_LOGI(TAG, "4) Configured I2S stream write");

    // encoder config
   /* wav_encoder_cfg_t wav_en_cfg = DEFAULT_WAV_ENCODER_CONFIG();
    wav_encoder = wav_encoder_init(&wav_en_cfg);
   ESP_LOGI(TAG, "5) Created  encoder");*/

    // decoder config
   /* wav_decoder_cfg_t  wav_dec_cfg  = DEFAULT_WAV_DECODER_CONFIG();
    wav_decoder = wav_decoder_init(&wav_dec_cfg);
     ESP_LOGI(TAG, "6) Created wav decoder"); */

    // register pipeline
    audio_pipeline_register(pipeline, i2s_reader, "i2sr");
   // audio_pipeline_register(pipeline, wav_encoder, "wavEn");
    //audio_pipeline_register(pipeline, wav_decoder, "wavDe");
    audio_pipeline_register(pipeline, i2s_writer, "i2sw");
    ESP_LOGI(TAG, "7) Registered pipeline elements");

    // link pipeline
    const char *link_tag[2] = {"i2sr","i2sw"};
    // ,"wavEn","wavDe"
    audio_pipeline_link(pipeline, &link_tag[0], 2);
    ESP_LOGI(TAG, "8) successfully linked together [codec_chip]--> i2s_read--> wav_encoder--> wav_decoder--> i2s_write --> [codec chip]");
        
    // setup event interface and listener
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);
    ESP_LOGI(TAG, "9) Set up  event listener");

    audio_pipeline_set_listener(pipeline, evt);
    ESP_LOGI(TAG, "10) Listening event from pipeline");

    audio_pipeline_run(pipeline);
    ESP_LOGI(TAG, "11) Started audio pipeline");
        audio_event_iface_msg_t msg;

    while (1) 
    {
    static int seconds_played = 1;
    vTaskDelay(1000/portTICK_PERIOD_MS);
    ESP_LOGI(TAG,"System running for %d seconds", seconds_played);
    seconds_played++;
    }
    ESP_LOGI(TAG, "[ 7 ] Stop audio_pipeline"); // сброс всех процессов
    audio_pipeline_stop(pipeline);
    audio_pipeline_wait_for_stop(pipeline);
    audio_pipeline_terminate(pipeline);
    // audio_pipeline_unregister(pipeline, wav_encoder);
    // audio_pipeline_unregister(pipeline, wav_encoder);
    audio_pipeline_unregister(pipeline, i2s_reader);
    audio_pipeline_unregister(pipeline, i2s_writer);
    /* Terminate the pipeline before removing the listener */
    audio_pipeline_remove_listener(pipeline);
    /* Stop all periph before removing the listener */
    /* Make sure audio_pipeline_remove_listener & audio_event_iface_remove_listener are called before destroying event_iface */
    audio_event_iface_destroy(evt);
    /* Release all resources */
    audio_pipeline_deinit(pipeline);
    audio_element_deinit(i2s_reader); 
    audio_element_deinit(i2s_writer);
   // audio_element_deinit(wav_encoder);
    //audio_element_deinit(wav_decoder);
    }
