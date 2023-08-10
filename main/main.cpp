#include "project_header.h"
my_struct cdc2; 
FastAudioFIFO speech_buf;
FastAudioFIFO frame_buf;
void read_dma(void * arg);
SemaphoreHandle_t mutex = xSemaphoreCreateMutex();
esp_timer_handle_t timer;
QueueHandle_t queue = xQueueCreate(3, sizeof(long));

extern "C" void app_main()
{
    esp_timer_early_init();
    audio_hal_codec_config_t es8388_config = AUDIO_CODEC_INMP441_CONFIG();
   audio_hal_codec_i2s_iface_t i2s_iface_cfg;
    i2s_iface_cfg.mode = AUDIO_HAL_MODE_SLAVE;
    i2s_iface_cfg.fmt = AUDIO_HAL_I2S_LEFT;
    i2s_iface_cfg.samples = AUDIO_HAL_08K_SAMPLES;
    i2s_iface_cfg.bits = AUDIO_HAL_BIT_LENGTH_16BITS;
    
   es_i2s_clock_t clock_cfg;
    clock_cfg.sclk_div = MCLK_DIV_1;
    clock_cfg.lclk_div = LCLK_DIV_256;
    
   es8388_init(&es8388_config);
    es8388_config_fmt(ES_MODULE_DAC, ES_I2S_LEFT);
    es8388_i2s_config_clock(clock_cfg);
    es8388_set_bits_per_sample(ES_MODULE_DAC, BIT_LENGTH_16BITS);
    es8388_config_i2s(AUDIO_HAL_CODEC_MODE_DECODE, &i2s_iface_cfg);
    es8388_set_voice_volume((int)40);
    es8388_set_mic_gain(MIC_GAIN_MIN);
    es8388_set_voice_mute(0);
    es8388_config_adc_input(ADC_INPUT_MIN);
    es8388_config_dac_output(DAC_OUTPUT_ALL);
    es8388_start(ES_MODULE_DAC);
    es8388_ctrl_state(AUDIO_HAL_CODEC_MODE_DECODE,AUDIO_HAL_CTRL_START);
    
   i2s_config_t i2sr_cfg;
    i2sr_cfg.mode = I2S_MODE_MASTER;    
    i2sr_cfg.sample_rate = 8000;
    i2sr_cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
    // i2sr_cfg.channel_format = I2S_CHANNEL_FMT_ALL_LEFT;
    i2sr_cfg.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
    i2sr_cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    i2sr_cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL2;
    i2sr_cfg.dma_buf_count = 2;
    i2sr_cfg.dma_buf_len = 300;
    i2sr_cfg.use_apll = 1;
    i2sr_cfg.tx_desc_auto_clear = 1;
    i2sr_cfg.fixed_mclk = 3072000;
    i2sr_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_DEFAULT;
    i2sr_cfg.bits_per_chan = I2S_BITS_PER_CHAN_DEFAULT;    
    
   i2s_pin_config_t pins;
    pins.mck_io_num = 0;
    pins.bck_io_num = 5;
    pins.data_in_num = 26;
    pins.ws_io_num = 25;
    pins.data_out_num = 26;
    // i2s_set_pin(I2S_NUM_0, &pins);
    i2s_driver_install(I2S_NUM_0, &i2sr_cfg, 0, NULL);
    i2s_set_sample_rates(I2S_NUM_0, 8000);
    i2s_set_clk(I2S_NUM_0, 8000, 16, I2S_CHANNEL_MONO);
    i2s_start(I2S_NUM_0);
    // codec2_data_init(&cdc2);                 
    // xTaskCreatePinnedToCore(read_dma,"Read_DMA", 50*1024, NULL, 3, &Tx_Handle, 1);
    // uint8_t * frame_bits = (uint8_t*)calloc(cdc2.FRAME_SIZE,sizeof(uint8_t));
    int16_t * speech_in = (int16_t*)malloc(40000*sizeof(int16_t));
    int16_t * speech_out = (int16_t*)malloc(40000*sizeof(int16_t));
    size_t bytes_written;
        size_t bytes_read;
    while(1)
    {
            printf("RECORDNIG\n");
            vTaskDelay(1000/portTICK_PERIOD_MS);
        i2s_read(I2S_NUM_0, (int*)speech_in, 40000*2, &bytes_read, portMAX_DELAY);
            printf("PLAYING..\n");
            vTaskDelay(500/portTICK_PERIOD_MS);
        i2s_write(I2S_NUM_0, (int*)speech_in, 40000*2, &bytes_written, portMAX_DELAY);
        printf("REPEAT.\n");
        vTaskDelay(500/portTICK_PERIOD_MS);
    }
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