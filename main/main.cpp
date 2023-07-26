#include "project_header.h"

extern "C" void app_main()
{
    // CHECKING FOR EVERYTHING THAT RELATES TO CODEC_CONFIG
    // handles & structs
    audio_pipeline_handle_t pipeline; 
    audio_element_handle_t i2s_reader;
    audio_element_handle_t codec2_enc;
    audio_element_handle_t codec2_dec;
    audio_element_handle_t i2s_writer;
    ringbuf_handle_t speech_read_buffer;
    ringbuf_handle_t enc2_frame_bits;
    ringbuf_handle_t speech_write_buffer;
    audio_board_handle_t board_handle = audio_board_init(); //SET UP CUSTOM CODEC CONFIG FOR CODEC2, CHECKED
    my_struct codec2_data; 
    codec2_data.mode = CODEC2_MODE_3200;

    //init configs
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    i2s_stream_cfg_t i2s_read_cfg = I2S_STREAM_CUSTOM_READ_CFG();
    i2s_stream_cfg_t i2s_write_cfg = I2S_STREAM_CUSTOM_WRITE_CFG();
    printf("read type %i %i \n read port num %i %i \n", i2s_read_cfg.type,AUDIO_STREAM_READER,  i2s_read_cfg.i2s_port, I2S_NUM_0);
    printf("write type %i %i \n write port num %i %i \n", i2s_write_cfg.type,AUDIO_STREAM_WRITER, i2s_write_cfg.i2s_port, I2S_NUM_1);
    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    audio_element_cfg_t cdc2_cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);
    
    //ELEMENT INFO
    audio_element_info_t cdc2_info = CDC2_INFO();
    audio_element_info_t i2s_info = I2S_INFO();
    
    static const char *TAG = "STARTED MAIN";
    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set(TAG, ESP_LOG_INFO);

    initArduino();
    Serial.begin(115200);
    while(!Serial){;}
    ESP_LOGI(TAG, "Initialised Arduino \n");

    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_BOTH, AUDIO_HAL_CTRL_START); // CHECKED
    ESP_LOGI(TAG, "\n\nConfigured and initialised codec chip \n");

    pipeline = audio_pipeline_init(&pipeline_cfg); // CHECKED
    mem_assert(pipeline)

    i2s_reader = i2s_stream_init(&i2s_read_cfg); // SET UP IN HEADER, CHECKED
    ESP_LOGI(TAG, "Initialised pipeline \n"); 
    ESP_LOGI(TAG, "Configured I2S stream read \n");

    i2s_writer = i2s_stream_init(&i2s_write_cfg); // SET UP IN HEADER, CHECKED
    ESP_LOGI(TAG, "Configured I2S stream write \n");

    codec2_data_init(&codec2_data);                 // CHECKED 
    ESP_LOGI(TAG, "Initialized codec2 user struct \n");

    codec2_enc = encoder2_element_init(&cdc2_cfg);
    codec2_dec = decoder2_element_init(&cdc2_cfg);
    ESP_LOGI(TAG, "Configured codec2 elements \n");
    // MUST SET CUSTOM ELEMENT INFO

    audio_element_setinfo(i2s_reader, &i2s_info);
    audio_element_setinfo(i2s_writer, &i2s_info);
    audio_element_setinfo(codec2_enc, &cdc2_info);
    audio_element_setinfo(codec2_dec, &cdc2_info); // CHECKED

    audio_element_setdata(codec2_enc, (my_struct*) &codec2_data);
    audio_element_setdata(codec2_dec, (my_struct*) &codec2_data);
    ESP_LOGI(TAG, "Passed on user data to encoder and decoder elements \n");
    
    speech_read_buffer = rb_create(640,1);
    enc2_frame_bits = rb_create(16,1);
    speech_write_buffer = rb_create(640,1);
    ESP_LOGI(TAG, "Created ringbuffers \n");            // CHECKED

    audio_element_set_output_ringbuf(i2s_reader, speech_read_buffer);

    audio_element_set_input_ringbuf(codec2_enc, speech_read_buffer);
    audio_element_set_output_ringbuf(codec2_enc, speech_write_buffer);

    audio_element_set_input_ringbuf(codec2_dec, enc2_frame_bits);
    audio_element_set_output_ringbuf(codec2_dec, speech_write_buffer);

    audio_element_set_input_ringbuf(i2s_writer, speech_write_buffer);
    ESP_LOGI(TAG, "Assigned ringbuffers \n");               // CHECKED

    audio_pipeline_register(pipeline, i2s_reader, "i2sr");
    audio_pipeline_register(pipeline, codec2_enc, "enc2");
    audio_pipeline_register(pipeline, codec2_dec, "dec2");
    audio_pipeline_register(pipeline, i2s_writer, "i2sw");
    ESP_LOGI(TAG, "Registered pipeline elements \n");      // CHECKED

    const char *link_tag[3] = {"i2sr",
                               "enc2", 
                             // "dec2", 
                               "i2sw"};
    audio_pipeline_link(pipeline, &link_tag[0], 3);
    ESP_LOGI(TAG, "successfully linked together [codec_chip]--> \n i2s_read--> \n codec2--> \n i2s_write --> \n [codec chip]");

    audio_event_iface_cfg_t event_cfg = AUDIO_EVENT_IFACE_LONG_CFG();
    audio_event_iface_handle_t pipeline_event = audio_event_iface_init(&event_cfg); //CHECKED
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
    audio_pipeline_remove_listener(pipeline);
    esp_periph_set_stop_all(set);
    audio_event_iface_remove_listener(esp_periph_set_get_event_iface(set), pipeline_event);
    audio_event_iface_destroy(pipeline_event);
    audio_pipeline_deinit(pipeline);
    audio_element_deinit(i2s_reader);
    audio_element_deinit(codec2_enc);
    audio_element_deinit(codec2_dec);
    codec2_data_deinit(&codec2_data);
    audio_element_deinit(i2s_writer);
    esp_periph_set_destroy(set); // DONE WITH MAIN. CONFIGURED EVERYTHING AUDIO_CODEC.
 }

    /* User codec2  functions. To be relocated to another file later. 

    * Don't know when, but later.

    * Setup and init functions. To configure or setup some stuff once. */

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