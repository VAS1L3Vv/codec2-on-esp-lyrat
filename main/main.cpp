#include "project_header.h"

// initArduino();
    // Serial.begin(115200);
    // while(!Serial){;}
    // ESP_LOGI(TAG, "Initialised Arduino \n");

extern "C" void app_main()
{
    static const char *TAG = "STARTED MAIN";
    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set(TAG, ESP_LOG_INFO);
    
    my_struct cdc2; 
    cdc2.mode = CODEC2_MODE_3200;
    audio_element_handle_t i2s_reader;
    audio_element_handle_t i2s_writer;
    audio_board_handle_t board_handle = audio_board_init();
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    i2s_stream_cfg_t i2s_read_cfg = I2S_STREAM_CUSTOM_READ_CFG();
    i2s_stream_cfg_t i2s_write_cfg = I2S_STREAM_CUSTOM_WRITE_CFG();
    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    audio_element_cfg_t cdc2_cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    audio_event_iface_cfg_t event_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);

    ButterworthFilter hp_filter(200, 8000, ButterworthFilter::ButterworthFilter::Highpass, 3);
    ButterworthFilter lp_filter(4000, 8000, ButterworthFilter::ButterworthFilter::Lowpass, 3);
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_BOTH, AUDIO_HAL_CTRL_START); 
    ESP_LOGI(TAG, "\n\nConfigured and initialised codec chip \n");

    i2s_reader = i2s_stream_init(&i2s_read_cfg);
    i2s_writer = i2s_stream_init(&i2s_write_cfg); 
    i2s_alc_volume_set(i2s_reader, 50);
    i2s_alc_volume_set(i2s_writer, 50);
    codec2_data_init(&cdc2);                 
    int element_number = 160;
    short * speech_in = (short*)calloc(80000,sizeof(short));
    short * speech_in_pack = (short*)calloc(160,sizeof(short));
    short * speech_out = (short*)calloc(80000,sizeof(short));
     short * speech_out_pack = (short*)calloc(160,sizeof(short));
    unsigned char * frame_bits = (unsigned char *)calloc(8,1);
    while(1) 
    {
        ESP_LOGI(TAG,"RECORDING");
        static size_t bytes_read = 0;
        i2s_read(I2S_NUM_0, (short*)speech_in, 160000, &bytes_read, portMAX_DELAY);
        ESP_LOGI(TAG,"HAVE RECORDED %u BYTES, %u SAMPLES", bytes_read, bytes_read/sizeof(short));
        ESP_LOGI(TAG, "PASSING SPEECH THROUGH 200 Hz HIGHPASS FILTER");
        for (int i = 0; i < 80000; i++)
		speech_in[i] = (short)hp_filter.Update((float)speech_in[i]);    
        for (int i = 0; i < 80000; i++)
		speech_in[i] = (short)lp_filter.Update((float)speech_in[i]);   
        // for(int i = 0; i < 80000; i+= cdc2.SPEECH_SIZE)
        // {
        //     memcpy((short*)speech_in_pack, (short*)speech_in+i, cdc2.SPEECH_SIZE*2);
        //     codec2_encode(cdc2.codec2_state, frame_bits, speech_in_pack);
        //     codec2_decode(cdc2.codec2_state, speech_out_pack, frame_bits);
        //     memcpy((short*)speech_out+i, (short*)speech_out_pack,cdc2.SPEECH_SIZE*2);
        // }
        ESP_LOGI(TAG,"PASSED. WRITING");
        static size_t bytes_written = 0;
        i2s_write(I2S_NUM_1, (short*)speech_in, 160000, &bytes_written, portMAX_DELAY);
        ESP_LOGI(TAG,"HAVE WRITTEN %u BYTES, %u SAMPLES", bytes_written, bytes_written/sizeof(short));
        ESP_LOGI(TAG,"FINISHED.");
    }
    audio_element_deinit(i2s_reader);
    audio_element_deinit(i2s_writer);
    codec2_data_deinit(&cdc2);
    esp_periph_set_destroy(set);
}

void codec2_data_init(my_struct* cdc2) // to be used once
{
    char* codec2_mode_string = (char*)malloc(20);
    static short mode = cdc2->mode;
    switch (mode)
    {
        case 0:
            cdc2->SPEECH_SIZE = 160;
            cdc2->FRAME_SIZE = 8;
            codec2_mode_string = "3200 BPS MODE";break;
        case 1:
            cdc2->SPEECH_SIZE = 160;
            cdc2->FRAME_SIZE = 6;
            codec2_mode_string = "2400 BPS MODE";break;
        case 2:
            cdc2->SPEECH_SIZE = 320;
            cdc2->FRAME_SIZE = 8;
            codec2_mode_string = "1600 BPS MODE";break;
        case 3:
            cdc2->SPEECH_SIZE = 320;
            cdc2->FRAME_SIZE = 7;
            codec2_mode_string = "1400 BPS MODE";break;
        case 4:
            cdc2->SPEECH_SIZE = 320;
            cdc2->FRAME_SIZE = 7;
            codec2_mode_string = "1300 BPS MODE";break;
        case 5:
            cdc2->SPEECH_SIZE = 320;
            cdc2->FRAME_SIZE = 6;
            codec2_mode_string = "1200 BPS MODE";break;
        case 6:
            cdc2->SPEECH_SIZE = 320;
            cdc2->FRAME_SIZE = 4;
            codec2_mode_string = "700 BPS MODE";break;
        case 7:
            cdc2->SPEECH_SIZE = 320;
            cdc2->FRAME_SIZE = 4;
            codec2_mode_string = "700B BPS MODE";break;
    }
    printf("DETECTED CODEC2 MODE %s \n\n", codec2_mode_string);
    printf("SPEECH SAMPLE SIZE %u \n\n bytes",cdc2->SPEECH_SIZE);
    printf("ENCODE FRAME SIZE %u \n\n bytes",cdc2->FRAME_SIZE);
    cdc2->codec2_state = codec2_create(mode);
    codec2_set_lpc_post_filter(cdc2->codec2_state, 1, 0, 0.8, 0.2);
    cdc2->READ_FLAG = 0;
    cdc2->WRITE_FLAG = 0;
    cdc2->speech_in = (int16_t*)calloc(cdc2->SPEECH_SIZE,sizeof(int16_t));
    cdc2->speech_out = (int16_t*)calloc(cdc2->SPEECH_SIZE,sizeof(int16_t));
    cdc2->frame_bits_in = (uint8_t*)calloc(cdc2->FRAME_SIZE,sizeof(uint8_t));
    cdc2->frame_bits_out = (uint8_t*)calloc(cdc2->FRAME_SIZE,sizeof(uint8_t));
}

void codec2_data_deinit(my_struct * cdc2)
{
    assert(cdc2);
    free(cdc2->speech_in);
    free(cdc2->speech_out);
    free(cdc2->frame_bits_out);
    free(cdc2->frame_bits_in);
    codec2_destroy(cdc2->codec2_state);
}