/**
 * $Id: $
 *
 * @brief Red Pitaya library Generate module interface
 *
 * @Author Red Pitaya
 *
 * (c) Red Pitaya  http://www.redpitaya.com
 *
 * This part of code is written in C programming language.
 * Please visit http://en.wikipedia.org/wiki/C_(programming_language)
 * for more details on the language used herein.
 */
#ifndef __PREC_GENERATE_H
#define __PREC_GENERATE_H

#include "redpitaya/rp.h"

#define LEVEL_MAX               1.0         // V
#define AMPLITUDE_MAX           1.0         // V
#define ARBITRARY_MIN          -1.0         // V
#define ARBITRARY_MAX           1.0         // V
#define OFFSET_MAX              2.0         // V
#define FREQUENCY_MIN           0           // Hz
#define FREQUENCY_MAX           62.5e6      // Hz
#define PHASE_MIN              -360         // deg
#define PHASE_MAX               360         // deg
#define DUTY_CYCLE_MIN          0           // %
#define DUTY_CYCLE_MAX          100         // %
#define BURST_COUNT_MIN        -1
#define BURST_COUNT_MAX         50000
#define DAC_FREQUENCY           125e6       // Hz

#define BUFFER_LENGTH           (16 * 1024)
#define CHA_DATA_OFFSET         0x10000
#define CHB_DATA_OFFSET         0x20000
#define DATA_BIT_LENGTH         14
#define MICRO                   1e6

// Base Generate address
#define GENERATE_BASE_ADDR      0x40200000
#define GENERATE_BASE_SIZE      0x00030000

typedef struct ch_properties {
    unsigned int amplitudeScale     :14;
    unsigned int                    :2;
    unsigned int amplitudeOffset    :14;
    unsigned int                    :2;
    uint32_t pointerEnd;
    uint32_t pointerStart;
    uint32_t pointerStep;
    unsigned int                    :2;
    uint32_t buffReadPointer        :14;
    unsigned int                    :16;
    uint32_t cyclesInOneBurst;
    uint32_t burstRepetitions;
    uint32_t delayBetweenBurstRepetitions;
} ch_properties_t;

typedef struct generate_control_s {
    unsigned int AtriggerSelector   :4;
    unsigned int ASM_WrapPointer    :1;
    unsigned int                    :1;
    unsigned int ASM_reset          :1;
    unsigned int AsetOutputTo0      :1;
    unsigned int AgatedBursts       :1;
    unsigned int                    :7;

    unsigned int BtriggerSelector   :4;
    unsigned int BSM_WrapPointer    :1;
    unsigned int                    :1;
    unsigned int BSM_reset          :1;
    unsigned int BsetOutputTo0      :1;
    unsigned int BgatedBursts       :1;
    unsigned int                    :7;

    ch_properties_t properties_chA;
    ch_properties_t properties_chA_ar[3];
    ch_properties_t properties_chB;
    ch_properties_t properties_chB_ar[3];
} generate_control_t;

int prec_generate_Init();
int prec_generate_Release();

int prec_generate_setOutputDisable(rp_channel_t channel, bool disable);
int prec_generate_getOutputEnabled(rp_channel_t channel, bool *disabled);
int prec_generate_setAmplitude(rp_channel_t channel, int buf_idx, float amplitude);
int prec_generate_getAmplitude(rp_channel_t channel, int buf_idx, float *amplitude);
int prec_generate_setDCOffset(rp_channel_t channel, int buf_idx, float offset);
int prec_generate_getDCOffset(rp_channel_t channel, int buf_idx, float *offset);
int prec_generate_setFrequency(rp_channel_t channel, int buf_idx, float frequency);
int prec_generate_getFrequency(rp_channel_t channel, int buf_idx, float *frequency);
int prec_generate_setWrapCounter(rp_channel_t channel, int buf_idx, uint32_t size);
int prec_generate_setTriggerSource(rp_channel_t channel, unsigned short value);
int prec_generate_getTriggerSource(rp_channel_t channel, uint32_t *value);
int prec_generate_setBurstCount(rp_channel_t channel, int buf_idx, uint32_t num);
int prec_generate_getBurstCount(rp_channel_t channel, int buf_idx, uint32_t *num);
int prec_generate_setTriggerEventCondition(unsigned short value);
int prec_generate_getTriggerEventCondition(uint32_t *value);

int prec_generate_simultaneousTrigger();
int prec_generate_Synchronise();

int prec_generate_writeData(rp_channel_t channel, int buf_idx, float *data, uint32_t start, uint32_t length);

#endif //__PREC_GENERATE_H
