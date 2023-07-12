
#include "project_header.h"

extern "C" void app_main()
{
    audio_pipeline_handle_t pipeline; 
    audio_element_handle_t i2s_reader;
    audio_element_handle_t i2s_writer;
    audio_board_handle_t board_handle = audio_board_init();
    
    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set(TAG, ESP_LOG_INFO);

    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_BOTH, AUDIO_HAL_CTRL_START);
    ESP_LOGI(TAG, "1) Configured and initialised codec chip");

    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    mem_assert(pipeline);
    ESP_LOGI(TAG, "2) Initialised pipeline");

    i2s_stream_cfg_t i2s_read_cfg = I2S_STREAM_CUSTOM_READ_CFG();
    i2s_reader = i2s_stream_init(&i2s_read_cfg);
    ESP_LOGI(TAG, "3) Configured I2S stream read");

    i2s_stream_cfg_t i2s_write_cfg = I2S_STREAM_CUSTOM_WRITE_CFG();
    i2s_writer = i2s_stream_init(&i2s_write_cfg);
    ESP_LOGI(TAG, "4) Configured I2S stream write");
    
    initArduino();
    Serial.begin(115200);
    while(!Serial){;}

    audio_pipeline_register(pipeline, i2s_reader, "i2sr");
    audio_pipeline_register(pipeline, i2s_writer, "i2sw");
    ESP_LOGI(TAG, "7) Registered pipeline elements");

    const char *link_tag[2] = {"i2sr","i2sw"};
    audio_pipeline_link(pipeline, &link_tag[0], 2);
    ESP_LOGI(TAG, "8) successfully linked together [codec_chip]--> i2s_read--> codec2--> i2s_write --> [codec chip]");

    audio_event_iface_cfg_t event_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG(); // ненважно
    audio_event_iface_handle_t event = audio_event_iface_init(&event_cfg); 
    ESP_LOGI(TAG, "9) Set up  event listener");

    audio_pipeline_set_listener(pipeline, event);
    ESP_LOGI(TAG, "10) Set listener event from pipeline");

    audio_pipeline_run(pipeline);
    ESP_LOGI(TAG, "11) Started audio pipeline tasks");

    while (1) 
    {
    audio_event_iface_msg_t msg;
        static int seconds_played = 1;
        vTaskDelay(1000/portTICK_PERIOD_MS);
        ESP_LOGI(TAG,"System running for %d seconds", seconds_played);
        Serial.println("loop");
        seconds_played++;
    }
    
    ESP_LOGI(TAG, "[ 7 ] Stop audio_pipeline"); // сброс всех процессов
    audio_pipeline_stop(pipeline);
    audio_pipeline_wait_for_stop(pipeline);
    audio_pipeline_terminate(pipeline);
    audio_pipeline_unregister(pipeline, i2s_reader);
    audio_pipeline_unregister(pipeline, i2s_writer);
    /* Terminate the pipeline before removing the listener */
    audio_pipeline_remove_listener(pipeline);
    /* Stop all periph before removing the listener */
    /* Make sure audio_pipeline_remove_listener & audio_event_iface_remove_listener are called before destroying event_iface */
    audio_event_iface_destroy(event);
    /* Release all resources */
    audio_pipeline_deinit(pipeline);
    audio_element_deinit(i2s_reader); 
    audio_element_deinit(i2s_writer);
    }