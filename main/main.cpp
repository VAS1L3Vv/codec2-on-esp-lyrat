#include "project_header.h"

// initArduino();
    // Serial.begin(115200);
    // while(!Serial){;}
    // ESP_LOGI(TAG, "Initialised Arduino \n");

extern "C" void app_main()
{
    audio_element_handle_t i2s_reader;
    audio_element_handle_t i2s_writer;
    audio_element_handle_t codec2_enc;
    audio_board_handle_t board_handle = audio_board_init();
    my_struct cdc2; 
    cdc2.mode = CODEC2_MODE_3200;

    timer_config_t timer;
    timer.alarm_en = TIMER_ALARM_DIS;
    timer.auto_reload = TIMER_AUTORELOAD_DIS;
    timer.counter_dir = TIMER_COUNT_UP;
    timer.counter_en = TIMER_PAUSE;
    timer.intr_type = TIMER_INTR_LEVEL;
    timer.divider = 80;
    // timer_init(timer);
    //init config
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    i2s_stream_cfg_t i2s_read_cfg = I2S_STREAM_CUSTOM_READ_CFG();
    i2s_stream_cfg_t i2s_write_cfg = I2S_STREAM_CUSTOM_WRITE_CFG();
    printf("read type %i %i \n read port num %i %i \n", i2s_read_cfg.type,AUDIO_STREAM_READER,  i2s_read_cfg.i2s_port, I2S_NUM_0);
    printf("write type %i %i \n write port num %i %i \n", i2s_write_cfg.type,AUDIO_STREAM_WRITER, i2s_write_cfg.i2s_port, I2S_NUM_1);
    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    audio_element_cfg_t cdc2_cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    audio_event_iface_cfg_t event_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);
    
    static const char *TAG = "STARTED MAIN";
    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set(TAG, ESP_LOG_INFO);

    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_BOTH, AUDIO_HAL_CTRL_START); // CHECKED
    ESP_LOGI(TAG, "\n\nConfigured and initialised codec chip \n");

    i2s_reader = i2s_stream_init(&i2s_read_cfg); // SET UP IN HEADER, CHECKED
    i2s_writer = i2s_stream_init(&i2s_write_cfg); // SET UP IN HEADER, CHECKED
    codec2_data_init(&cdc2);                 // CHECKED 
    codec2_enc = encoder2_element_init(&cdc2_cfg);
    audio_element_setdata(codec2_enc, (my_struct*) &cdc2);
    int element_number = 160;
    short * speech_in = (short*)calloc(80000,sizeof(short));

    while(1) {
        for(int secs = 3; secs > 0; secs--) {
        ESP_LOGI(TAG,"RECORDING IN %u",secs);
        vTaskDelay(1000);
        }
        ESP_LOGI(TAG,"RECORDING");
        static size_t bytes_read = 0;
        i2s_read(I2S_NUM_0, (short*)speech_in, 160000, &bytes_read, portMAX_DELAY);
        ESP_LOGI(TAG,"HAVE RECORDED %u BYTES, %u SAMPLES", bytes_read, bytes_read/sizeof(short));
        vTaskDelay(2000);
        ESP_LOGI(TAG,"WRITING");
        static size_t bytes_written = 0;
        i2s_write(I2S_NUM_1, (short*)speech_in, 160000, &bytes_written, portMAX_DELAY);
        ESP_LOGI(TAG,"HAVE WRITTEN %u BYTES, %u SAMPLES", bytes_written, bytes_written/sizeof(short));
        ESP_LOGI(TAG,"FINISHED. REPEATING IN 3 SECONDS.");
        vTaskDelay(3000);
    }

    audio_element_deinit(i2s_reader);
    audio_element_deinit(codec2_enc);
    codec2_data_deinit(&cdc2);
    audio_element_deinit(i2s_writer);
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

audio_element_handle_t encoder2_element_init(audio_element_cfg_t *codec2_enc_cfg) {
    codec2_enc_cfg->task_prio = 1;
    codec2_enc_cfg->task_core = 1;
    codec2_enc_cfg->task_stack = 50*1024;
    codec2_enc_cfg->open = codec2_enc_open;
    codec2_enc_cfg->process = codec2_enc_process;
    codec2_enc_cfg->close = codec2_enc_close;
    codec2_enc_cfg->destroy = codec2_enc_destroy;
    codec2_enc_cfg->tag = "codec2_enc_tag";
    audio_element_handle_t el = audio_element_init(codec2_enc_cfg); // CHECKING
    return el;
}

audio_element_handle_t decoder2_element_init(audio_element_cfg_t *codec2_dec_cfg)
{
    codec2_dec_cfg->task_prio = 1;
    codec2_dec_cfg->task_core = 1;
    codec2_dec_cfg->task_stack = 50*1024;
    codec2_dec_cfg->open = codec2_dec_open;
    codec2_dec_cfg->process = codec2_dec_process;
    codec2_dec_cfg->close = codec2_dec_close;
    codec2_dec_cfg->destroy = codec2_dec_destroy;
    codec2_dec_cfg->tag = "codec2_dec_tag";
    audio_element_handle_t el = audio_element_init(codec2_dec_cfg);
    return el;
}

// Codec 2 encoder element functions. Not auto generated. Please don't discredit the work i've done. 

static int codec2_enc_open(audio_element_handle_t self)
{
    static const char * TAG = audio_element_get_tag(self);
    my_struct * cdc2 = (my_struct*) audio_element_getdata(self);
    assert(cdc2);
    ESP_LOGI(TAG,"input speech samples size: %d bytes",cdc2->SPEECH_SIZE);
    ESP_LOGI(TAG,"output encoded frame size: %d bytes",cdc2->FRAME_SIZE);
    ESP_LOGD(TAG, "codec2_enc_open");
    return ESP_OK;
}

static audio_element_err_t codec2_enc_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    static const char * TAG = audio_element_get_tag(self);
    ESP_LOGD(TAG, "codec2_enc_processing");
    static unsigned char * frame_bits = (unsigned char*)calloc(8, sizeof(unsigned char));
    static short * speech = (short*)calloc(160, sizeof(short));
    static short * speech_out = (short*)calloc(160, sizeof(short));
    static char* out_buffer = (char*) calloc(320, sizeof(char));
    my_struct * cdc2 = (my_struct*) audio_element_getdata(self);
    in_len = 160*2;
    size_t in_size;
    i2s_read(I2S_NUM_0, speech, in_len, &in_size, portMAX_DELAY);
    // int in_size = audio_element_input(self, in_buffer, in_len);
    ESP_LOGI(TAG, "INPUT BUFFER BYTES:  %i ", in_size);
    ESP_LOGI(TAG, "INPUT BUFFER BIT RATE:  %i", (sizeof(speech[0]))*8);
    // ESP_LOGI(TAG, "SPEECH BUFFER BYTES:  %i", sizeof(speech));
    ESP_LOGI(TAG, "SPEECH BUFFER BIT RATE:  %i", (sizeof(speech[0]))*8);
    ESP_LOGI(TAG,"INIT BYTES IN FRAME: %i ", sizeof(frame_bits[0])*8);
    codec2_encode(cdc2->codec2_state, frame_bits, speech);
    int frame_size = 0;
    for(int i = 0; i <= 7; i++) {
    frame_size += sizeof(frame_bits[i]);
    }
    ESP_LOGI(TAG,"ENCODED BYTES IN FRAME: %i ", frame_size);
    codec2_decode(cdc2->codec2_state,speech_out,frame_bits);
    int speech_out_size = 0;
    for(int i = 0; i <= 160; i++) {
    speech_out_size += sizeof(speech_out[i]);
    }
    ESP_LOGI(TAG, "BYTES DECODED:  %i", speech_out_size);
    ESP_LOGI(TAG, "SPEECH OUT BIT RATE:  %i", (sizeof(speech_out[0]))*8);
    for(int i = 0; i <= sizeof(speech_out)/2;i+=2) {
        out_buffer[i] = speech_out[i] >> 8;
        out_buffer[i+1] = (char)speech_out[i];
    }
    unsigned short ret = 0;
    if (in_size > 0) {
    //  ret = audio_element_output(self, out_buffer,in_len);
     i2s_write(I2S_NUM_1, speech_out, in_len, &in_size, portMAX_DELAY);
        if (ret > 0) {
        audio_element_update_byte_pos(self, ret);
        }
    }
    ESP_LOGI(TAG, "BYTES WRITTEN TO BUFFER:  %i", ret);
    ESP_LOGI(TAG, "OUTPUT BIT RATE: %i \n", sizeof(out_buffer[0])*8);
    vTaskDelay(10);
    return (audio_element_err_t)ret;
}

static esp_err_t codec2_enc_close(audio_element_handle_t self)
{
    static const char * TAG = audio_element_get_tag(self);
    ESP_LOGD(TAG, "codec2_enc_close");

    return ESP_OK;
}

static esp_err_t codec2_enc_destroy(audio_element_handle_t self)
{
    static const char * TAG = audio_element_get_tag(self);
    ESP_LOGD(TAG, "codec2_enc_destroy");
    
    return ESP_OK;
}
//
// Codec2 decoder element functions. Also not auto generated.
//
//
static int codec2_dec_open(audio_element_handle_t self)
{
    static const char * TAG = audio_element_get_tag(self);
    my_struct * cdc2 = (my_struct*) audio_element_getdata(self);
    assert(cdc2);
    ESP_LOGI(TAG,"input encoded frame size: %d bytes",cdc2->FRAME_SIZE);
    ESP_LOGI(TAG,"output speech samples size: %d bytes",cdc2->SPEECH_SIZE);
    ESP_LOGD(TAG, "codec2_enc_open");
    return ESP_OK;
}

static audio_element_err_t codec2_dec_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    static const char * TAG = audio_element_get_tag(self);
    my_struct * cdc2 = (my_struct*) audio_element_getdata(self);

    in_len = cdc2->FRAME_SIZE;
    int out_len = cdc2->SPEECH_SIZE;
    int in_size = audio_element_input(self, in_buffer, in_len);

    /*
    for(int i = 0; i <= in_len;i++) {
        if (i > in_size)
        {
            ESP_LOGE(TAG, "INPUT BUFFER OVERFLOW: BUFFER SIZE GREATER THAN FRAME_SIZE BUFFER SIZE");
            continue;
        }  cdc2->frame_bits_in[i] = (uint8_t)in_buffer[i];
    }
    } */
    codec2_decode(cdc2->codec2_state, cdc2->speech_out, (uint8_t*)in_buffer);
     
     /*
    for(int i = 0;  i <= out_len; i+=2) {
        in_buffer[i] = cdc2->speech_out[i] >> 8; 
        in_buffer[i+1] = (char)cdc2->speech_out[i];
    }*/
    if (in_size > 0) {
    unsigned short ret = audio_element_output(self, (char*)cdc2->speech_out, out_len);
        if (ret > 0) {
        audio_element_update_byte_pos(self, ret);
        }
    }
    ESP_LOGD(TAG, "codec2_dec_processing");

    return (audio_element_err_t)out_len;
}

static esp_err_t codec2_dec_close(audio_element_handle_t self)
{
    static const char * TAG = audio_element_get_tag(self);
    ESP_LOGD(TAG, "codec2_dec_close");

    return ESP_OK;
}

static esp_err_t codec2_dec_destroy(audio_element_handle_t self)
{
    static const char * TAG = audio_element_get_tag(self);
    ESP_LOGD(TAG, "codec2_dec_destroy");

    return ESP_OK;
}