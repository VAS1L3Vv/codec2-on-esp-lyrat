/* Record file to SD Card

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

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

#define RECORD_TIME_SECONDS (10)
static const char *TAG = "some_stupid_shit";
void app_main(void)
{
    audio_pipeline_handle_t pipeline; 
    audio_element_handle_t i2s_stream_reader, audio_encoder,;

    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set(TAG, ESP_LOG_INFO);

    ESP_LOGI(TAG, "[ 2 ] Start codec chip");
    audio_board_handle_t board_handle = audio_board_init();
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_ENCODE, AUDIO_HAL_CTRL_START);

    ESP_LOGI(TAG, "[3.0] Create audio pipeline for recording");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    mem_assert(pipeline);

    ESP_LOGI(TAG, "[3.2] Create i2s stream to read audio data from codec chip");
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.type = AUDIO_STREAM_READER;
    
    i2s_stream_reader = i2s_stream_init(&i2s_cfg);

    ESP_LOGI(TAG, "[3.3] Create audio encoder to handle data");
    wav_encoder_cfg_t wav_cfg = DEFAULT_WAV_ENCODER_CONFIG();
    audio_encoder = wav_encoder_init(&wav_cfg);

    ESP_LOGI(TAG, "[3.4] Register all elements to audio pipeline");
    audio_pipeline_register(pipeline, i2s_stream_reader, "i2s");
    audio_pipeline_register(pipeline, audio_encoder, "wav");


    ESP_LOGI(TAG, "[3.5] Link it together [codec_chip]-->i2s_stream-->audio_encoder-->something");
    const char *link_tag[3] = {"i2s", "wav", };
    audio_pipeline_link(pipeline, &link_tag[0], 3);
        
    ESP_LOGI(TAG, "[ 4 ] Set up  event listener");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);

    ESP_LOGI(TAG, "[4.1] Listening event from pipeline");
    audio_pipeline_set_listener(pipeline, evt);

    ESP_LOGI(TAG, "[ 5 ] Start audio_pipeline");
    audio_pipeline_run(pipeline);

    ESP_LOGI(TAG, "[ 6 ] Listen for all pipeline events, record for %d Seconds", RECORD_TIME_SECONDS);
    int second_recorded = 0;
    while (1) { // Лупа, в течении которой происходит запись и передача значений по I2S
        audio_event_iface_msg_t msg;
        if (audio_event_iface_listen(evt, &msg, 1000 / portTICK_RATE_MS) != ESP_OK) {
            second_recorded ++;
            ESP_LOGI(TAG, "[ * ] Recording ... %d", second_recorded);
            if (second_recorded >= RECORD_TIME_SECONDS) {
                audio_element_set_ringbuf_done(i2s_stream_reader);
            }
            continue;
        }
        /* Stop when the last pipeline element (fatfs_stream_writer in this case) receives stop event */
        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.cmd == AEL_MSG_CMD_REPORT_STATUS && (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED) || ((int)msg.data == AEL_STATUS_ERROR_OPEN))) 
        {
            ESP_LOGW(TAG, "[ * ] Stop event received");
            break;
        }
    }

    ESP_LOGI(TAG, "[ 7 ] Stop audio_pipeline"); // сброс всех процессов
    audio_pipeline_stop(pipeline);
    audio_pipeline_wait_for_stop(pipeline);
    audio_pipeline_terminate(pipeline);
    audio_pipeline_unregister(pipeline, audio_encoder);
    audio_pipeline_unregister(pipeline, i2s_stream_reader);
    /* Terminal the pipeline before removing the listener */
    audio_pipeline_remove_listener(pipeline);
    /* Stop all periph before removing the listener */
    /* Make sure audio_pipeline_remove_listener & audio_event_iface_remove_listener are called before destroying event_iface */
    audio_event_iface_destroy(evt);
    /* Release all resources */
    audio_pipeline_deinit(pipeline);
    audio_element_deinit(i2s_stream_reader);
    audio_element_deinit(audio_encoder);
}
