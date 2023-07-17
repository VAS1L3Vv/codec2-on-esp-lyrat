
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


#define I2S_STREAM_CUSTOM_WRITE_CFG() {                                         \
     .type = AUDIO_STREAM_WRITER,                                               \
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
    .i2s_port = I2S_NUM_1,                                                      \
    .use_alc = 0,                                                               \
    .volume = 1,                                                                \
    .out_rb_size = I2S_STREAM_RINGBUFFER_SIZE,                                  \
    .task_stack = I2S_STREAM_TASK_STACK,                                        \
    .task_core = 1,                                                             \
    .task_prio = I2S_STREAM_TASK_PRIO,                                          \
    .stack_in_ext = false,                                                      \
    .multi_out_num = 0,                                                         \
    .uninstall_drv = true,                                                      \
    .need_expand = false,                                                       \
    .expand_src_bits = I2S_BITS_PER_SAMPLE_8BIT,                                \
    .buffer_len = 900,                                                          \
}

void print_seconds(void*);
void encode(void*);

#define SPEECH_BUFFER_SIZE 160
#define ENCODE_FRAME_BITS 64
#define ENCODE_FRAME_BYTES 8

static char *TAG = "MONITORING";
struct CODEC2* codec2_state;
int8_t speech[SPEECH_BUFFER_SIZE];
TaskHandle_t TaskHandle = NULL;
void * context_ptr = NULL;


extern "C" void app_main()
{
    // handles
    audio_pipeline_handle_t pipeline; 
    audio_element_handle_t i2s_reader;
    audio_element_handle_t codec2_enc;
    audio_element_handle_t codec2_dec;
    audio_element_handle_t i2s_writer;
    ringbuf_handle_t speech_read_buffer;
    ringbuf_handle_t enc2_frame_bits;
    ringbuf_handle_t dec2_frame_bits;
    ringbuf_handle_t speech_write_buffer;
    audio_board_handle_t board_handle = audio_board_init();

    // configs
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    i2s_stream_cfg_t i2s_read_cfg = I2S_STREAM_CUSTOM_READ_CFG();
    i2s_stream_cfg_t i2s_write_cfg = I2S_STREAM_CUSTOM_WRITE_CFG();
    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);

    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set(TAG, ESP_LOG_INFO);

    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_BOTH, AUDIO_HAL_CTRL_START);
    ESP_LOGI(TAG, "\n\nConfigured and initialised codec chip \n");

    pipeline = audio_pipeline_init(&pipeline_cfg);
    mem_assert(pipeline);
    ESP_LOGI(TAG, "Initialised pipeline \n");

    i2s_reader = i2s_stream_init(&i2s_read_cfg);
    ESP_LOGI(TAG, "Configured I2S stream read \n");

    i2s_writer = i2s_stream_init(&i2s_write_cfg);
    ESP_LOGI(TAG, "Configured I2S stream write \n");

    speech_read_buffer = rb_create(SPEECH_BUFFER_SIZE,1);
    enc2_frame_bits = rb_create(ENCODE_FRAME_BYTES,1);
    speech_write_buffer = rb_create(SPEECH_BUFFER_SIZE,1);
    ESP_LOGI(TAG, "Created ringbuffers \n");

    audio_element_set_output_ringbuf(i2s_reader, speech_read_buffer);
    audio_element_set_input_ringbuf(codec2_enc, speech_read_buffer);
    audio_element_set_output_ringbuf(codec2_enc, enc2_frame_bits);
    audio_element_set_input_ringbuf(codec2_dec, enc2_frame_bits);
     audio_element_set_output_ringbuf(codec2_dec, speech_write_buffer);
    audio_element_set_input_ringbuf(i2s_writer, speech_write_buffer);
    ESP_LOGI(TAG, "Assigned ringbuffers \n");

    initArduino();
    Serial.begin(115200);
    while(!Serial){;}
    ESP_LOGI(TAG, "Initialised Arduino \n");

    audio_pipeline_register(pipeline, i2s_reader, "i2sr");
    audio_pipeline_register(pipeline, codec2_enc, "enc2");
    audio_pipeline_register(pipeline, codec2_dec, "dec2");
    audio_pipeline_register(pipeline, i2s_writer, "i2sw");
    ESP_LOGI(TAG, "Registered pipeline elements \n");

    const char *link_tag[4] = {"i2sr", "enc2", "dec2", "i2sw"};
    audio_pipeline_link(pipeline, &link_tag[0], 4);
    ESP_LOGI(TAG, "successfully linked together [codec_chip]--> \n i2s_read--> \n codec2--> \n i2s_write --> \n [codec chip]");

    audio_event_iface_cfg_t event_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG(); // ненважно
    audio_event_iface_handle_t pipeline_event = audio_event_iface_init(&event_cfg);
    audio_event_iface_handle_t read_event = audio_event_iface_init(&event_cfg);
    audio_event_iface_handle_t write_event = audio_event_iface_init(&event_cfg);
    ESP_LOGI(TAG, "Configured event listeners \n");

    audio_pipeline_set_listener(pipeline, pipeline_event);
    audio_event_iface_set_listener(esp_periph_set_get_event_iface(set), pipeline_event);
    ESP_LOGI(TAG, "Listening to pipeline \n");
    
    audio_pipeline_run(pipeline);
    ESP_LOGI(TAG, "Started audio pipeline \n");

     while (1)
    {
        audio_event_iface_msg_t msg;
        esp_err_t ret = audio_event_iface_listen(pipeline_event, &msg, portMAX_DELAY);
    }

    ESP_LOGI(TAG, " Stopped audio_pipeline"); // сброс всех процессов
    audio_pipeline_stop(pipeline);
    audio_pipeline_wait_for_stop(pipeline);
    audio_pipeline_terminate(pipeline);
    audio_pipeline_unregister(pipeline, i2s_reader);
    audio_pipeline_unregister(pipeline, i2s_writer);
    audio_pipeline_unregister(pipeline, codec2_enc);
    audio_pipeline_unregister(pipeline, codec2_dec);
    /* Terminate the pipeline before removing the listener */
    audio_pipeline_remove_listener(pipeline);
    /* Stop all periph before removing the listener */
    esp_periph_set_stop_all(set);
    audio_event_iface_remove_listener(esp_periph_set_get_event_iface(set), pipeline_event);
    /* Make sure audio_pipeline_remove_listener & audio_event_iface_remove_listener are called before destroying event_iface */
    audio_event_iface_destroy(pipeline_event);
    /* Release all resources */
    audio_pipeline_deinit(pipeline);
    audio_element_deinit(i2s_reader); 
    audio_pipeline_deinit(pipeline, codec2_enc);
    audio_pipeline_deinit(pipeline, codec2_dec);
    audio_element_deinit(i2s_writer);
    esp_periph_set_destroy(set);
    }