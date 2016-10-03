/**
 * $Id: $
 *
 * @brief Precidyne library Generate handler interface
 *
 * @Author Bhaskar Mukherjee
 *
 *
 * This part of code is written in C programming language.
 * Please visit http://en.wikipedia.org/wiki/C_(programming_language)
 * for more details on the language used herein.
 */

#ifndef PREC_GENERATE_HANDLER_H_
#define PREC_GENERATE_HANDLER_H_


#include "redpitaya/rp.h"

int prec_gen_SetDefaultValues();
int prec_gen_Disable(rp_channel_t chanel);
int prec_gen_Enable(rp_channel_t chanel);
int prec_gen_IsEnable(rp_channel_t channel, bool *value);
int prec_gen_setAmplitude(rp_channel_t channel, int buf_idx, float amplitude);
int prec_gen_getAmplitude(rp_channel_t channel, int buf_idx, float *amplitude);
int prec_gen_setOffset(rp_channel_t channel, int buf_idx, float offset) ;
int prec_gen_getOffset(rp_channel_t channel, int buf_idx, float *offset) ;
int prec_gen_setFrequency(rp_channel_t channel, int buf_idx, float frequency);
int prec_gen_getFrequency(rp_channel_t channel, int buf_idx, float *frequency);
int prec_gen_setPhase(rp_channel_t channel, int buf_idx, float phase);
int prec_gen_getPhase(rp_channel_t channel, int buf_idx, float *phase);
int prec_gen_setWaveform(rp_channel_t channel, int buf_idx, prec_waveform_t type);
int prec_gen_getWaveform(rp_channel_t channel, int buf_idx, prec_waveform_t *type);
int prec_gen_setArbWaveform(rp_channel_t channel, int buf_idx, float *data, uint32_t length);
int prec_gen_getArbWaveform(rp_channel_t channel, int buf_idx, float *data, uint32_t *length);
int prec_gen_setBurstCount(rp_channel_t channel, int buf_idx, int num);
int prec_gen_getBurstCount(rp_channel_t channel, int buf_idx, int *num);
int prec_gen_setTriggerSource(rp_channel_t chanel, rp_trig_src_t src);
int prec_gen_getTriggerSource(rp_channel_t chanel, rp_trig_src_t *src);
int prec_gen_setTriggerEventCondition(rp_trig_evt_t evt);
int prec_gen_getTriggerEventCondition(rp_trig_evt_t *evt);
int prec_gen_Trigger(uint32_t channel);
int prec_gen_Synchronise();
int prec_triggerIfInternal(rp_channel_t channel);

int prec_synthesize_signal(rp_channel_t channel, int buf_idx);
int prec_synthesis_sin(float *data_out);
int prec_synthesis_triangle(float *data_out);
int prec_synthesis_arbitrary(rp_channel_t channel, float *data_out, uint32_t * size);
int prec_synthesis_square(float frequency, float *data_out);
int prec_synthesis_rampUp(float *data_out);
int prec_synthesis_rampDown(float *data_out);
int prec_synthesis_DC(float *data_out);
int prec_synthesis_PWM(float ratio, float *data_out);

#endif
