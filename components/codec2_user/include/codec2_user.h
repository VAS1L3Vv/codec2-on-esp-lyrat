#ifndef _CODEC2_USER_H_
#define _CODEC2_USER_H_

#include "audio_element.h"
#include "audio_idf_version.h"
#include "esp_err.h"
#include "Arduino.h"
#include "codec2.h"
#include "LoRa.h"
#include <ButterworthFilter.h>

#ifdef __cplusplus
extern "C" {
#endif  

static esp_err_t codec2_enc_open(audio_element_handle_t self);
static int codec2_enc_process(audio_element_handle_t self, char *in_buffer, int in_len);
static esp_err_t codec2_enc_close(audio_element_handle_t self);
static esp_err_t codec2_enc_destroy(audio_element_handle_t self);
audio_element_handle_t codec2_element_init(audio_element_cfg_t *codec2_enc_cfg);

typedef struct user_struct
{
struct CODEC2* codec2_state;
uint8_t* frame_bits_in;
uint8_t* frame_bits_out;
int16_t* speech_in;
int16_t* speech_out;
}my_struct;

void codec2_data_init(my_struct*);

#ifdef __cplusplus
}
#endif

#endif