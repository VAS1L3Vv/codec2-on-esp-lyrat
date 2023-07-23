#include "codec2_user.h"

    void codec2_data_init(my_struct* codec2_data)
    {
    codec2_data->codec2_state = codec2_create(CODEC2_MODE_3200);
    codec2_data->speech_in = (int16_t*)calloc(SPEECH_BUFFER_SIZE,sizeof(int16_t));
    codec2_data->speech_out = (int16_t*)calloc(SPEECH_BUFFER_SIZE,sizeof(int16_t));
    codec2_data->frame_bits_in = (uint8_t*)calloc(ENCODE_FRAME_SIZE,sizeof(uint8_t));
    codec2_data->frame_bits_out = (uint8_t*)calloc(ENCODE_FRAME_SIZE,sizeof(uint8_t));
    }

static const char *TAG = "codec2_enc_tag";

    static esp_err_t codec2_enc_open(audio_element_handle_t self)
{
    ESP_LOGD(TAG, "codec2_enc_open");

    return ESP_OK;
}

static int codec2_enc_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    int in_size = audio_element_input(self, in_buffer, in_len);
    int out_len = in_size;
    my_struct * codec2_data = (my_struct*) audio_element_getdata(self);
    for(int i = 0; i <= out_len;i+=2)
    {
        if (i > out_len)
        {
            ESP_LOGE(TAG, "INPUT BUFFER OVERFLOW: BUFFER SIZE GREATER THAN speech_in BUFFER SIZE");
            return out_len;
        }
    codec2_data->speech_in[i] = in_buffer[i] << 8;
    codec2_data->speech_in[i] += in_buffer[i+1];
    }
    codec2_encode(codec2_data->codec2_state, codec2_data->frame_bits_out, codec2_data->speech_in);
    codec2_decode(codec2_data->codec2_state, codec2_data->speech_out, codec2_data->frame_bits_in);

    for(int i = 0;  i <= out_len; i+=2)
    {
        in_buffer[i] = codec2_data->speech_out[i] >> 8; 
        in_buffer[i+1] = (char)codec2_data->speech_out[i];
    }
    if (in_size > 0) 
    {
    out_len = audio_element_output(self, in_buffer, in_size);
        if (out_len > 0) {
        audio_element_update_byte_pos(self, out_len);
        }
    }
    ESP_LOGD(TAG, "codec2_enc_processing");

    return out_len;
}

static esp_err_t codec2_enc_close(audio_element_handle_t self)
{
    ESP_LOGD(TAG, "codec2_enc_close");

    return ESP_OK;
}

static esp_err_t codec2_enc_destroy(audio_element_handle_t self)
{
    ESP_LOGD(TAG, "codec2_enc_destroy");

    return ESP_OK;
}

audio_element_handle_t codec2_element_init(audio_element_cfg_t *codec2_enc_cfg)
{
      codec2_enc_cfg->open = codec2_enc_open;
    codec2_enc_cfg->process = codec2_enc_process;
    codec2_enc_cfg->close = codec2_enc_close;
    codec2_enc_cfg->destroy = codec2_enc_destroy;
    codec2_enc_cfg->tag = "codec2_enc_tag";
    audio_element_handle_t el = audio_element_init(codec2_enc_cfg);

    return el;
}