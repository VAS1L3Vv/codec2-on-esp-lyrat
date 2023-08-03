#include "project_header.h"
    my_struct cdc2;
    FastAudioFIFO speech_buf;
    FastAudioFIFO frame_buf;
    void read_dma(void * arg);
    void write_dma(void * arg);
    SemaphoreHandle_t mutex = xSemaphoreCreateMutex();
    SemaphoreHandle_t mutex1 = xSemaphoreCreateMutex();
    TaskHandle_t Tx_Handle = NULL;
    TaskHandle_t Rx_Handle = NULL;
    QueueHandle_t queue;

extern "C" void app_main()
{
    static const char *TAG = "STARTED MAIN";
    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set(TAG, ESP_LOG_INFO);
    // initArduino();
    // Serial.begin(115200);
    // while(!Serial){;}
    // ESP_LOGI(TAG, "Initialised Arduino \n");
    cdc2.mode = CODEC2_MODE_2400;
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
    audio_element_info_t i2s_info = I2S_INFO();
    ButterworthFilter hp_filter(600, 8000, ButterworthFilter::ButterworthFilter::Highpass, 1);
    ButterworthFilter lp_filter(3500, 8000, ButterworthFilter::ButterworthFilter::Lowpass, 1);
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_BOTH, AUDIO_HAL_CTRL_START); 
    ESP_LOGI(TAG, "\n\nConfigured and initialised codec chip \n");

    i2s_reader = i2s_stream_init(&i2s_read_cfg);
    i2s_writer = i2s_stream_init(&i2s_write_cfg); 
    // audio_element_setinfo(i2s_reader,&i2s_info);
    codec2_data_init(&cdc2);                 
    xTaskCreatePinnedToCore(read_dma,"Read_DMA", 50*1024, NULL, 3, &Tx_Handle, 1);
    // xTaskCreate(write_dma,"Write_DMA", 50*1024, NULL, 4, &Rx_Handle);  
    
    // static const char * TAG = "WRITE";
    ESP_LOGD(TAG,"HAVE STARTED WRITE TASK");
    uint8_t * frame_bits = (uint8_t*)calloc(cdc2.FRAME_SIZE,sizeof(uint8_t));
    int16_t * speech_out = (int16_t*)calloc(cdc2.SPEECH_SIZE,sizeof(int16_t));
    static unsigned int i = 1;
    static size_t bytes_written = 0;
    while(1)
    {
        xSemaphoreTake(mutex1, portMAX_DELAY);
        if( xQueueReceive(queue, &frame_bits, (TickType_t)75))
    {
        // ESP_LOGD(TAG,"WRITING\n");
        codec2_decode(cdc2.codec2_state, speech_out, frame_bits);
        xSemaphoreGive(mutex1);
        // ESP_LOGD(TAG,"DECODED %u BYTE FRAME.\n",cdc2.FRAME_SIZE);
        i2s_write(I2S_NUM_1, (short*)speech_out, cdc2.SPEECH_BYTES, &bytes_written, portMAX_DELAY);
    }
        // ESP_LOGD(TAG,"HAVE WRITTEN %u BYTES, %u SAMPLES\n", bytes_written, bytes_written/sizeof(short));
        // cdc2.WRITE_FLAG = WRITING_DONE;
        // ESP_LOGD(TAG,"FINISHED CYCLE №%u\n",i);
        i++;
    }
    audio_element_deinit(i2s_reader);
    audio_element_deinit(i2s_writer);
    codec2_data_deinit(&cdc2);
    esp_periph_set_destroy(set);
}

void read_dma(void * arg)
{
    static const char * TAG = "READ";
    static size_t bytes_read = 0;
    uint8_t * frame_bits = (uint8_t*)calloc(cdc2.FRAME_SIZE,sizeof(uint8_t));
    short * speech_in = (int16_t*)calloc(cdc2.SPEECH_SIZE,sizeof(int16_t));
    queue = xQueueCreate(64, sizeof(uint8_t)); 
    ESP_LOGD(TAG,"HAVE STARTED READ TASK");
    while(1)
    {
    i2s_read(I2S_NUM_0, (short*)speech_in, cdc2.SPEECH_BYTES, &bytes_read, portMAX_DELAY);
    xSemaphoreTake(mutex, portMAX_DELAY);
    codec2_encode(cdc2.codec2_state, frame_bits, speech_in);
    xSemaphoreGive(mutex);
    if(xQueueSend(queue, (uint8_t*)frame_bits,(TickType_t)70) == pdFALSE) printf("ERROR SENDING");
    ESP_LOGD(TAG,"ENCODED %u SAMPLES.\n",cdc2.SPEECH_SIZE);
    }
}

// void write_dma(void * arg)
// {
//         
//     static const char * TAG = "WRITE";
//     ESP_LOGD(TAG,"HAVE STARTED WRITE TASK");
//     uint8_t * frame_bits = (uint8_t*)calloc(cdc2.FRAME_SIZE,sizeof(uint8_t));
//     int16_t * speech_out = (int16_t*)calloc(cdc2.SPEECH_SIZE,sizeof(int16_t));
//     static unsigned int i = 1;
//     static size_t bytes_written = 0;
//     while(1)
//     {
//         while(frame_buf.empty()) {ESP_LOGD(TAG,"WAITING TO GET FRAME\n");}
//         ESP_LOGD(TAG,"WRITING\n");
//         // cdc2.WRITE_FLAG = WRITING;
//         frame_buf.get_frame(frame_bits, cdc2.FRAME_SIZE);
//         codec2_decode(cdc2.codec2_state, speech_out, frame_bits);
//         ESP_LOGD(TAG,"DECODED %u BYTE FRAME.\n",cdc2.FRAME_SIZE);
//         i2s_write(I2S_NUM_1, (short*)speech_out, cdc2.SPEECH_BYTES, &bytes_written, portMAX_DELAY);
//         ESP_LOGD(TAG,"HAVE WRITTEN %u BYTES, %u SAMPLES\n", bytes_written, bytes_written/sizeof(short));
//         // cdc2.WRITE_FLAG = WRITING_DONE;
//         ESP_LOGD(TAG,"FINISHED CYCLE №%u\n",i);
//         i++;
//     }
// }

void codec2_data_init(my_struct* cdc2) // to be used once
{
    static short mode = cdc2->mode;
    switch (mode)
    {
        case 0:
            cdc2->SPEECH_SIZE = 160;
            cdc2->FRAME_SIZE = 8;
            printf("3200 BPS MODE\n");break;
        case 1:
            cdc2->SPEECH_SIZE = 160;
            cdc2->FRAME_SIZE = 6;
            printf("2400 BPS MODE\n");break;
        case 2:
            cdc2->SPEECH_SIZE = 320;
            cdc2->FRAME_SIZE = 8;
            printf("1600 BPS MODE\n");break;
        case 3:
            cdc2->SPEECH_SIZE = 320;
            cdc2->FRAME_SIZE = 7;
            printf("1400 BPS MODE\n");break;
        case 4:
            cdc2->SPEECH_SIZE = 320;
            cdc2->FRAME_SIZE = 7;
            printf("1300 BPS MODE\n");break;
        case 5:
            cdc2->SPEECH_SIZE = 320;
            cdc2->FRAME_SIZE = 6;
            printf("1200 BPS MODE\n");break;
        case 6:
            cdc2->SPEECH_SIZE = 320;
            cdc2->FRAME_SIZE = 4;
            printf("700 BPS MODE\n");break;
        case 7:
            cdc2->SPEECH_SIZE = 320;
            cdc2->FRAME_SIZE = 4;
            printf("700B BPS MODE\n");break;
    }
    printf("SPEECH SAMPLE LENGTH %u \n\n",cdc2->SPEECH_SIZE);
    printf("ENCODE FRAME SIZE %u \n\n",cdc2->FRAME_SIZE);
    cdc2->SPEECH_BYTES = cdc2->SPEECH_SIZE*sizeof(short);
    cdc2->codec2_state = codec2_create(mode);
    codec2_set_lpc_post_filter(cdc2->codec2_state, 1, 0, 0.5, 0.5);
    cdc2->READ_FLAG = READING;
    cdc2->WRITE_FLAG = WRITING_DONE;
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

static esp_err_t i2s_mono_fix(int bits, uint8_t *sbuff, uint32_t len)
{
    static const char *TAG = "mono fix";
    if (bits == 16) {
        int16_t *temp_buf = (int16_t *)sbuff;
        int16_t temp_box;
        int k = len >> 1;
        for (int i = 0; i < k; i += 2) {
            temp_box = temp_buf[i];
            temp_buf[i] = temp_buf[i + 1];
            temp_buf[i + 1] = temp_box;
        }
    } else if (bits == 32) {
        int32_t *temp_buf = (int32_t *)sbuff;
        int32_t temp_box;
        int k = len >> 2;
        for (int i = 0; i < k; i += 4) {
            temp_box = temp_buf[i];
            temp_buf[i] = temp_buf[i + 1];
            temp_buf[i + 1] = temp_box;
        }
    } else {
        ESP_LOGI(TAG, "%s %dbits is not supported", __func__, bits);
        return ESP_FAIL;
    }
    return ESP_OK;
}