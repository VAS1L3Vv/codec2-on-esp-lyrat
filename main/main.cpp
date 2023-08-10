#include "project_header.h"

    my_struct cdc2; 
    FastAudioFIFO speech_buf;
    FastAudioFIFO frame_buf;
    void read_dma(void * arg);
    SemaphoreHandle_t mutex = xSemaphoreCreateMutex();
    esp_timer_handle_t timer;
    gpio_set_pull_mode(GPIO_NUM_26, GPIO_PULLDOWN_ONLY);
QueueHandle_t queue = xQueueCreate(3, sizeof(long));

extern "C" void app_main()
{
    static const char *TAG = "STARTED MAIN";
    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set(TAG, ESP_LOG_INFO);

    // TaskHandle_t Tx_Handle = NULL;
    // cdc2.mode = CODEC2_MODE_700B;
    audio_element_handle_t i2s_reader;
    audio_element_handle_t i2s_writer;
    audio_board_handle_t board_handle = audio_board_init();
    i2s_stream_cfg_t i2s_read_cfg = I2S_STREAM_CUSTOM_READ_CFG();
    i2s_stream_cfg_t i2s_write_cfg = I2S_STREAM_CUSTOM_WRITE_CFG();
    // esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    // audio_element_info_t i2s_info = I2S_INFO();
    i2s_pin_config_t pins;
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START); 
    esp_timer_early_init();
    i2s_reader = i2s_stream_init(&i2s_read_cfg);
    // pins.bck_io_num = 5;
    // pins.data_in_num = 26;
    // pins.ws_io_num = 25;
    // ESP_ERROR_CHECK(i2s_set_pin(I2S_NUM_0, &pins));
    i2s_writer = i2s_stream_init(&i2s_write_cfg); 
    // codec2_data_init(&cdc2);                 
    // xTaskCreatePinnedToCore(read_dma,"Read_DMA", 50*1024, NULL, 3, &Tx_Handle, 1);
    // uint8_t * frame_bits = (uint8_t*)calloc(cdc2.FRAME_SIZE,sizeof(uint8_t));
    int * speech_in = (int*)malloc(80000*sizeof(int));
    int * speech_out = (int*)malloc(80000*sizeof(int));
    size_t bytes_written;
    size_t bytes_read;
    
    while(1)
    {
        for(int i = 3; i > 0; i--)
        {
            printf("Recording in %u...\n",i);
            vTaskDelay(1000/portTICK_PERIOD_MS);
        }
            printf("RECORDNIG\n");
        i2s_read(I2S_NUM_0, (int*)speech_in, 80000*sizeof(int), &bytes_read, portMAX_DELAY);
            printf("PLAYING..\n");
            vTaskDelay(500/portTICK_PERIOD_MS);
        i2s_write(I2S_NUM_1, (int*)speech_in, 80000*sizeof(int), &bytes_written, portMAX_DELAY);
        printf("REPEAT.\n");
        vTaskDelay(500/portTICK_PERIOD_MS);
    }
    audio_element_deinit(i2s_reader);
    audio_element_deinit(i2s_writer);
    codec2_data_deinit(&cdc2);
}

// while(frame_buf.empty())
//             {
//             vTaskDelay(60/portTICK_PERIOD_MS);
//             continue;
//         }
//         vTaskDelay(1/portTICK_PERIOD_MS);
//         frame_buf.get_frame(frame_bits, cdc2.FRAME_SIZE);
//         codec2_decode(cdc2.codec2_state, speech_out, frame_bits);
//         i2s_write(I2S_NUM_1, (short*)speech_out, cdc2.SPEECH_BYTES, &bytes_written, portMAX_DELAY);

// void read_dma(void * arg)
// {
//     static const char * TAG = "READ";
//     static size_t bytes_read = 0;
//     uint8_t * frame_bits = (uint8_t*)malloc(cdc2.FRAME_SIZE*sizeof(uint8_t));
//     short * speech_in = (int16_t*)malloc(cdc2.SPEECH_SIZE*sizeof(int16_t));
//     while(1)
//     {

//     while(frame_buf.full())
//         {
//         vTaskDelay(1/portTICK_PERIOD_MS);
//         continue;
//      }
//     i2s_read(I2S_NUM_0, (short*)speech_in, cdc2.SPEECH_BYTES, &bytes_read, portMAX_DELAY);
//     vTaskDelay(2/portTICK_PERIOD_MS);
//     codec2_encode(cdc2.codec2_state, frame_bits, speech_in);
//     frame_buf.put_frame(frame_bits,cdc2.FRAME_SIZE);
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
    codec2_set_lpc_post_filter(cdc2->codec2_state, 1, 0, 0.8, 0.6);
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