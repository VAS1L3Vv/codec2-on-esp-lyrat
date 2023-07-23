#include "project_header.h"

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
    ringbuf_handle_t speech_write_buffer;
    audio_board_handle_t board_handle = audio_board_init();
    my_struct codec2_data; 
    codec2_data_init(&codec2_data);

    // configs
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    i2s_stream_cfg_t i2s_read_cfg = I2S_STREAM_CUSTOM_READ_CFG();
    i2s_stream_cfg_t i2s_write_cfg = I2S_STREAM_CUSTOM_WRITE_CFG();
    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    audio_element_cfg_t el_cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);

    
    static const char *TAG = "MONITORING";
    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set(TAG, ESP_LOG_INFO);

    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_BOTH, AUDIO_HAL_CTRL_START);
    ESP_LOGI(TAG, "\n\nConfigured and initialised codec chip \n");

    pipeline = audio_pipeline_init(&pipeline_cfg);
    mem_assert(pipeline)
    ESP_LOGI(TAG, "Initialised pipeline \n");
    i2s_reader = i2s_stream_init(&i2s_read_cfg);

    ESP_LOGI(TAG, "Configured I2S stream read \n");

    i2s_writer = i2s_stream_init(&i2s_write_cfg);
    ESP_LOGI(TAG, "Configured I2S stream write \n");

    codec2_enc = codec2_element_init(&el_cfg);
    //codec2_dec = codec2_element_init(&el_cfg);
    ESP_LOGI(TAG, "Configured element \n");
    audio_element_setdata(codec2_enc, (my_struct*) &codec2_data);
    //audio_element_setdata(codec2_dec, (my_struct*) &codec2_data);
    
    speech_read_buffer = rb_create(SPEECH_BUFFER_SIZE,1);
    enc2_frame_bits = rb_create(ENCODE_FRAME_SIZE,1);
    speech_write_buffer = rb_create(SPEECH_BUFFER_SIZE,1);
    ESP_LOGI(TAG, "Created ringbuffers \n");

    audio_element_set_output_ringbuf(i2s_reader, speech_read_buffer);
    audio_element_set_input_ringbuf(codec2_enc, speech_read_buffer);
    audio_element_set_output_ringbuf(codec2_enc, speech_write_buffer);
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

    const char *link_tag[3] = {"i2sr", "enc2", "i2sw"};
    audio_pipeline_link(pipeline, &link_tag[0], 3);
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
    audio_element_deinit(codec2_enc);
    audio_element_deinit(codec2_dec);
    codec2_destroy(codec2_data.codec2_state);
    audio_element_deinit(i2s_writer);
    esp_periph_set_destroy(set);
    }

    /* User codec2  functions. To be relocated to another file later. /

    * Don't know when, but later.
    
    * Setup and init functions. To configure or setup some stuff once. */

void codec2_data_init(my_struct* codec2_data) // to be used once
{
codec2_data->codec2_state = codec2_create(CODEC2_MODE_3200);
codec2_data->speech_in = (int16_t*)calloc(SPEECH_BUFFER_SIZE,sizeof(int16_t));
codec2_data->speech_out = (int16_t*)calloc(SPEECH_BUFFER_SIZE,sizeof(int16_t));
codec2_data->frame_bits_in = (uint8_t*)calloc(ENCODE_FRAME_SIZE,sizeof(uint8_t));
codec2_data->frame_bits_out = (uint8_t*)calloc(ENCODE_FRAME_SIZE,sizeof(uint8_t));
}

audio_element_handle_t encoder2_element_init(audio_element_cfg_t *codec2_enc_cfg)
{
    codec2_enc_cfg->task_prio = 3;
    codec2_enc_cfg->task_core = 1;
    codec2_enc_cfg->task_stack = 5*1024;
    codec2_enc_cfg->open = codec2_enc_open;
    codec2_enc_cfg->process = codec2_enc_process;
    codec2_enc_cfg->close = codec2_enc_close;
    codec2_enc_cfg->destroy = codec2_enc_destroy;
    codec2_enc_cfg->tag = "codec2_enc_tag";
    audio_element_handle_t el = audio_element_init(codec2_enc_cfg);
    return el;
}

audio_element_handle_t decoder2_element_init(audio_element_cfg_t *codec2_dec_cfg)
{
    codec2_dec_cfg->task_prio = 3;
    codec2_dec_cfg->task_core = 1;
    codec2_dec_cfg->task_stack = 5*1024;
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
    ESP_LOGD(TAG, "codec2_enc_open");

    return ESP_OK;
}

static audio_element_err_t codec2_enc_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    int in_size = audio_element_input(self, in_buffer, in_len);
    int out_len = in_size;
    static const char * TAG = audio_element_get_tag(self);
    my_struct * codec2_data = (my_struct*) audio_element_getdata(self);

    for(int i = 0; i <= out_len;i+=2) {
        if (i > out_len)
        {
            ESP_LOGE(TAG, "INPUT BUFFER OVERFLOW: BUFFER SIZE GREATER THAN speech_in BUFFER SIZE");
            return (audio_element_err_t)out_len;
        }
    codec2_data->speech_in[i] = in_buffer[i] << 8;
    codec2_data->speech_in[i] += in_buffer[i+1];
    }
    codec2_encode(codec2_data->codec2_state, codec2_data->frame_bits_out, codec2_data->speech_in);
    codec2_decode(codec2_data->codec2_state, codec2_data->speech_out, codec2_data->frame_bits_in);

    for(int i = 0;  i <= out_len; i+=2) {
        in_buffer[i] = codec2_data->speech_out[i] >> 8; 
        in_buffer[i+1] = (char)codec2_data->speech_out[i];
    }
    if (in_size > 0) {
    out_len = audio_element_output(self, in_buffer, in_size);
        if (out_len > 0) {
        audio_element_update_byte_pos(self, out_len);
        }
    }
    ESP_LOGD(TAG, "codec2_enc_processing");

    return (audio_element_err_t)out_len;
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


// Codec2 decoder element functions. Also not auto generated.
//
//


static int codec2_dec_open(audio_element_handle_t self)
{
    static const char * TAG = audio_element_get_tag(self);
    ESP_LOGD(TAG, "codec2_dec_open");

    return ESP_OK;
}

static audio_element_err_t codec2_dec_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    int in_size = audio_element_input(self, in_buffer, in_len);
    int out_len = in_size;
    static const char * TAG = audio_element_get_tag(self);
    my_struct * codec2_data = (my_struct*) audio_element_getdata(self);

    for(int i = 0; i <= out_len;i+=2) {
        if (i > out_len)
        {
            ESP_LOGE(TAG, "INPUT BUFFER OVERFLOW: BUFFER SIZE GREATER THAN speech_in BUFFER SIZE");
            return (audio_element_err_t)out_len;
        }
    codec2_data->speech_in[i] = in_buffer[i] << 8;
    codec2_data->speech_in[i] += in_buffer[i+1];
    }
    codec2_decode(codec2_data->codec2_state, codec2_data->speech_out, codec2_data->frame_bits_in);

    for(int i = 0;  i <= out_len; i+=2) {
        in_buffer[i] = codec2_data->speech_out[i] >> 8; 
        in_buffer[i+1] = (char)codec2_data->speech_out[i];
    }
    if (in_size > 0) {
    out_len = audio_element_output(self, in_buffer, in_size);
        if (out_len > 0) {
        audio_element_update_byte_pos(self, out_len);
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






















































































// I reallly hope this garbage works.