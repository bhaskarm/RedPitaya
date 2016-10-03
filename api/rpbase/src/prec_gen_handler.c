/**
* $Id: $
*
* @brief Red Pitaya library Generate handler interface
*
* @Author Red Pitaya
*
* (c) Red Pitaya  http://www.redpitaya.com
*
* This part of code is written in C programming language.
* Please visit http://en.wikipedia.org/wiki/C_(programming_language)
* for more details on the language used herein.
*/

#include <float.h>
#include "math.h"
#include "common.h"
#include "prec_generate.h"
#include "prec_gen_handler.h"

// global variables
// TODO: should be organized into a system status structure
float         chA_amplitude            = 1, chB_amplitude            = 1;
float         chA_offset               = 0, chB_offset               = 0;
float         chA_dutyCycle            = 0, chB_dutyCycle            = 0;
float         chA_frequency               , chB_frequency               ;
float         chA_phase                = 0, chB_phase                = 0;
int           chA_burstCount           = 1, chB_burstCount           = 1;
int           chA_burstRepetition      = 1, chB_burstRepetition      = 1;
uint32_t      chA_burstPeriod          = 0, chB_burstPeriod          = 0;
prec_waveform_t chA_waveform                , chB_waveform                ;
uint32_t      chA_size     = BUFFER_LENGTH, chB_size     = BUFFER_LENGTH;
uint32_t      chA_arb_size = BUFFER_LENGTH, chB_arb_size = BUFFER_LENGTH;

float chA_arbitraryData[BUFFER_LENGTH];
float chB_arbitraryData[BUFFER_LENGTH];

int prec_gen_SetDefaultValues() {
    ECHECK(prec_gen_Disable(RP_CH_1));
    ECHECK(prec_gen_Disable(RP_CH_2));
    ECHECK(prec_gen_setFrequency(RP_CH_1, 0, 1000));
    ECHECK(prec_gen_setFrequency(RP_CH_2, 0, 1000));
    ECHECK(prec_gen_setWaveform(RP_CH_1, 0, PREC_WAVEFORM_SINE));
    ECHECK(prec_gen_setWaveform(RP_CH_2, 0, PREC_WAVEFORM_SINE));
    ECHECK(prec_gen_setOffset(RP_CH_1, 0, 0));
    ECHECK(prec_gen_setOffset(RP_CH_2, 0, 0));
    ECHECK(prec_gen_setAmplitude(RP_CH_1, 0, 1));
    ECHECK(prec_gen_setAmplitude(RP_CH_2, 0, 1));
    ECHECK(prec_gen_setBurstCount(RP_CH_1, 0, 1));
    ECHECK(prec_gen_setBurstCount(RP_CH_2, 0, 1));
    ECHECK(prec_gen_setTriggerSource(RP_CH_1, RP_GEN_TRIG_SRC_INTERNAL));
    ECHECK(prec_gen_setTriggerSource(RP_CH_2, RP_GEN_TRIG_SRC_INTERNAL));
    ECHECK(prec_gen_setPhase(RP_CH_1, 0, 0.0));
    ECHECK(prec_gen_setPhase(RP_CH_2, 0, 0.0));
    return RP_OK;
}

int prec_gen_Disable(rp_channel_t channel) {
    return prec_generate_setOutputDisable(channel, true);
}

int prec_gen_Enable(rp_channel_t channel) {
    return prec_generate_setOutputDisable(channel, false);
}

int prec_gen_IsEnable(rp_channel_t channel, bool *value) {
    return prec_generate_getOutputEnabled(channel, value);
}

int prec_gen_checkAmplitudeAndOffset(float amplitude, float offset) {
    if (fabs(amplitude) + fabs(offset) > LEVEL_MAX) {
        return RP_EOOR;
    }
    return RP_OK;
}

int prec_gen_setAmplitude(rp_channel_t channel, int buf_idx, float amplitude) {
    float offset;
    CHANNEL_ACTION(channel,
            offset = chA_offset,
            offset = chB_offset)
    ECHECK(prec_gen_checkAmplitudeAndOffset(amplitude, offset));

    CHANNEL_ACTION(channel,
            chA_amplitude = amplitude,
            chB_amplitude = amplitude)
    return prec_generate_setAmplitude(channel, buf_idx, amplitude);
}

int prec_gen_getAmplitude(rp_channel_t channel, int buf_idx, float *amplitude) {
    return prec_generate_getAmplitude(channel, buf_idx, amplitude);
}

int prec_gen_setOffset(rp_channel_t channel, int buf_idx, float offset) {
    float amplitude;
    CHANNEL_ACTION(channel,
            amplitude = chA_amplitude,
            amplitude = chB_amplitude)
    ECHECK(prec_gen_checkAmplitudeAndOffset(amplitude, offset));

    CHANNEL_ACTION(channel,
            chA_offset = offset,
            chB_offset = offset)
    return prec_generate_setDCOffset(channel, buf_idx, offset);
}

int prec_gen_getOffset(rp_channel_t channel, int buf_idx, float *offset) {
    return prec_generate_getDCOffset(channel, buf_idx, offset);
}

int prec_gen_setFrequency(rp_channel_t channel, int buf_idx, float frequency) {
    if (frequency < FREQUENCY_MIN || frequency > FREQUENCY_MAX) {
        return RP_EOOR;
    }

    ECHECK(prec_generate_setFrequency(channel, buf_idx, frequency));
    ECHECK(prec_synthesize_signal(channel, buf_idx));
    return prec_gen_Synchronise();
}

int prec_gen_getFrequency(rp_channel_t channel, int buf_idx, float *frequency) {
    return prec_generate_getFrequency(channel, buf_idx, frequency);
}

int prec_gen_setPhase(rp_channel_t channel, int buf_idx, float phase) {
    if (phase < PHASE_MIN || phase > PHASE_MAX) {
        return RP_EOOR;
    }
    if (phase < 0) {
        phase += 360;
    }
    CHANNEL_ACTION(channel,
            chA_phase = phase,
            chB_phase = phase)

    ECHECK(prec_synthesize_signal(channel, buf_idx));
    return prec_gen_Synchronise();
}

int prec_gen_getPhase(rp_channel_t channel, int buf_idx, float *phase) {
    CHANNEL_ACTION(channel,
            *phase = chA_phase,
            *phase = chB_phase)
    return RP_OK;
}

int prec_gen_setWaveform(rp_channel_t channel, int buf_idx, prec_waveform_t type) {
    CHANNEL_ACTION(channel,
            chA_waveform = type,
            chB_waveform = type)
    if (type == PREC_WAVEFORM_ARBITRARY) {
        CHANNEL_ACTION(channel,
                chA_size = chA_arb_size,
                chB_size = chB_arb_size)
    }
    else{
        CHANNEL_ACTION(channel,
                chA_size = BUFFER_LENGTH,
                chB_size = BUFFER_LENGTH)
    }
    return prec_synthesize_signal(channel, buf_idx);
}

int prec_gen_getWaveform(rp_channel_t channel, int buf_idx, prec_waveform_t *type) {
    CHANNEL_ACTION(channel,
            *type = chA_waveform,
            *type = chB_waveform)
    return RP_OK;
}

int prec_gen_setArbWaveform(rp_channel_t channel, int buf_idx, float *data, uint32_t length) {
    // Check if data is normalized
    float min = FLT_MAX, max = -FLT_MAX; // initial values
    int i;
    for(i = 0; i < length; i++) {
        if (data[i] < min)
            min = data[i];
        if (data[i] > max)
            max = data[i];
    }
    if (min < ARBITRARY_MIN || max > ARBITRARY_MAX) {
        return RP_ENN;
    }

    // Save data
    float *pointer;
    CHANNEL_ACTION(channel,
            pointer = chA_arbitraryData,
            pointer = chB_arbitraryData)
    for(i = 0; i < length; i++) {
        pointer[i] = data[i];
    }
    for(i = length; i < BUFFER_LENGTH; i++) { // clear the rest of the buffer
        pointer[i] = 0;
    }

    if (channel == RP_CH_1) {
        chA_arb_size = length;
        if(chA_waveform==PREC_WAVEFORM_ARBITRARY){
        	return prec_synthesize_signal(channel, buf_idx);
        }
    }
    else if (channel == RP_CH_2) {
    	chA_arb_size = length;
        if(chB_waveform==PREC_WAVEFORM_ARBITRARY){
        	return prec_synthesize_signal(channel, buf_idx);
        }
    }
    else {
        return RP_EPN;
    }

    return RP_OK;
}

int prec_gen_getArbWaveform(rp_channel_t channel, int buf_idx, float *data, uint32_t *length) {
    // If this data was not set, then this method will return incorrect data
    float *pointer;
    if (channel == RP_CH_1) {
        *length = chA_arb_size;
        pointer = chA_arbitraryData;
    }
    else if (channel == RP_CH_2) {
        *length = chB_arb_size;
        pointer = chB_arbitraryData;
    }
    else {
        return RP_EPN;
    }
    for (int i = 0; i < *length; ++i) {
        data[i] = pointer[i];
    }
    return RP_OK;
}

int prec_gen_setBurstCount(rp_channel_t channel, int buf_idx, int num) {
    if ((num < BURST_COUNT_MIN || num > BURST_COUNT_MAX) && num == 0) {
        return RP_EOOR;
    }
    CHANNEL_ACTION(channel,
            chA_burstCount = num,
            chB_burstCount = num)
    if (num == -1) {    // -1 represents infinity. In FPGA value 0 represents infinity
        num = 0;
    }
    ECHECK(prec_generate_setBurstCount(channel, buf_idx, (uint32_t) num));

    // trigger channel if internal trigger source
    return prec_triggerIfInternal(channel);
}

int prec_gen_getBurstCount(rp_channel_t channel, int buf_idx, int *num) {
    return prec_generate_getBurstCount(channel, buf_idx, (uint32_t *) num);
}

int prec_gen_setTriggerSource(rp_channel_t channel, rp_trig_src_t src) {
    if (src == RP_GEN_TRIG_SRC_INTERNAL) {
        return prec_generate_setTriggerSource(channel, 1);
    }
    else if (src == RP_GEN_TRIG_SRC_EXT_PE) {
        return prec_generate_setTriggerSource(channel, 2);
    }
    else if (src == RP_GEN_TRIG_SRC_EXT_NE) {
        return prec_generate_setTriggerSource(channel, 3);
    }
    else {
        return RP_EIPV;
    }
}

int prec_gen_getTriggerSource(rp_channel_t channel, rp_trig_src_t *src) {
    ECHECK(prec_generate_getTriggerSource(channel, (uint32_t *) &src));
    return RP_OK;
}

int prec_gen_setTriggerEventCondition(rp_trig_evt_t evt) {
    ECHECK(prec_generate_setTriggerEventCondition(evt));
    return RP_OK;
}

int prec_gen_getTriggerEventCondition(rp_trig_evt_t *evt) {
    uint32_t event;
    ECHECK(prec_generate_getTriggerEventCondition(&event));
    *evt = event & 0x0f;
    return RP_OK;
}

int prec_gen_Trigger(uint32_t channel) {
    switch (channel) {
        case 0:
        case 1:
            return prec_generate_setTriggerSource(channel, RP_GEN_TRIG_SRC_INTERNAL);
        case 2:
        case 3:
            return prec_generate_simultaneousTrigger();
        default:
            return RP_EOOR;
    }
}

int prec_gen_Synchronise() {
    return prec_generate_Synchronise();
}

int prec_synthesize_signal(rp_channel_t channel, int buf_idx) {
    float data[BUFFER_LENGTH];
    prec_waveform_t waveform;
    float dutyCycle, frequency;
    uint32_t size, phase;

    if (channel == RP_CH_1) {
        waveform = chA_waveform;
        dutyCycle = chA_dutyCycle;
        frequency = chA_frequency;
        size = chA_size;
        phase = (uint32_t) (chA_phase * BUFFER_LENGTH / 360.0);
    }
    else if (channel == RP_CH_2) {
        waveform = chB_waveform;
        dutyCycle = chB_dutyCycle;
        frequency = chB_frequency;
    	size = chB_size;
        phase = (uint32_t) (chB_phase * BUFFER_LENGTH / 360.0);
    }
    else{
        return RP_EPN;
    }

    switch (waveform) {
        case PREC_WAVEFORM_SINE     : prec_synthesis_sin      (data);                 break;
        case PREC_WAVEFORM_TRIANGLE : prec_synthesis_triangle (data);                 break;
        case PREC_WAVEFORM_SQUARE   : prec_synthesis_square   (frequency, data);      break;
        case PREC_WAVEFORM_RAMP_UP  : prec_synthesis_rampUp   (data);                 break;
        case PREC_WAVEFORM_RAMP_DOWN: prec_synthesis_rampDown (data);                 break;
        case PREC_WAVEFORM_DC       : prec_synthesis_DC       (data);                 break;
        case PREC_WAVEFORM_PWM      : prec_synthesis_PWM      (dutyCycle, data);      break;
        case PREC_WAVEFORM_ARBITRARY: prec_synthesis_arbitrary(channel, data, &size); break;
        default:                    return RP_EIPV;
    }
    return prec_generate_writeData(channel, buf_idx, data, phase, size);
}

int prec_synthesis_sin(float *data_out) {
    for(int unsigned i = 0; i < BUFFER_LENGTH; i++) {
        data_out[i] = (float) (sin(2 * M_PI * (float) i / (float) BUFFER_LENGTH));
    }
    return RP_OK;
}

int prec_synthesis_triangle(float *data_out) {
    for(int unsigned i = 0; i < BUFFER_LENGTH; i++) {
        data_out[i] = (float) ((asin(sin(2 * M_PI * (float) i / (float) BUFFER_LENGTH)) / M_PI * 2));
    }
    return RP_OK;
}

int prec_synthesis_rampUp(float *data_out) {
    data_out[BUFFER_LENGTH -1] = 0;
    for(int unsigned i = 0; i < BUFFER_LENGTH-1; i++) {
        data_out[BUFFER_LENGTH - i-2] = (float) (-1.0 * (acos(cos(M_PI * (float) i / (float) BUFFER_LENGTH)) / M_PI - 1));
    }
    return RP_OK;
}

int prec_synthesis_rampDown(float *data_out) {
    for(int unsigned i = 0; i < BUFFER_LENGTH; i++) {
        data_out[i] = (float) (-1.0 * (acos(cos(M_PI * (float) i / (float) BUFFER_LENGTH)) / M_PI - 1));
    }
    return RP_OK;
}

int prec_synthesis_DC(float *data_out) {
    for(int unsigned i = 0; i < BUFFER_LENGTH; i++) {
        data_out[i] = 1.0;
    }
    return RP_OK;
}

int prec_synthesis_PWM(float ratio, float *data_out) {
    // calculate number of samples that need to be high
    int h = (int) (BUFFER_LENGTH/2 * ratio);

    for(int unsigned i = 0; i < BUFFER_LENGTH; i++) {
        if (i < h || i >= BUFFER_LENGTH - h) {
            data_out[i] = 1.0;
        }
        else {
            data_out[i] = (float) -1.0;
        }
    }
    return RP_OK;
}

int prec_synthesis_arbitrary(rp_channel_t channel, float *data_out, uint32_t * size) {
    float *pointer;
    CHANNEL_ACTION(channel,
            pointer = chA_arbitraryData,
            pointer = chB_arbitraryData)
    for (int unsigned i = 0; i < BUFFER_LENGTH; i++) {
        data_out[i] = pointer[i];
    }
    CHANNEL_ACTION(channel,
            *size = chA_arb_size,
            *size = chB_arb_size)
    return RP_OK;
}

int prec_synthesis_square(float frequency, float *data_out) {
    // Various locally used constants - HW specific parameters
    const int trans0 = 30;
    const int trans1 = 300;

    int trans = (int) (frequency / 1e6 * trans1); // 300 samples at 1 MHz

    if (trans <= 10)  trans = trans0;

    for(int unsigned i = 0; i < BUFFER_LENGTH; i++) {
        if      ((0 <= i                      ) && (i <  BUFFER_LENGTH/2 - trans))  data_out[i] =  1.0f;
        else if ((i >= BUFFER_LENGTH/2 - trans) && (i <  BUFFER_LENGTH/2        ))  data_out[i] =  1.0f - (2.0f / trans) * (i - (BUFFER_LENGTH/2 - trans));
        else if ((0 <= BUFFER_LENGTH/2        ) && (i <  BUFFER_LENGTH   - trans))  data_out[i] = -1.0f;
        else if ((i >= BUFFER_LENGTH   - trans) && (i <  BUFFER_LENGTH          ))  data_out[i] = -1.0f + (2.0f / trans) * (i - (BUFFER_LENGTH   - trans));
    }

    return RP_OK;
}

int prec_triggerIfInternal(rp_channel_t channel) {
    uint32_t value;
    ECHECK(prec_generate_getTriggerSource(channel, &value));
    if (value == RP_GEN_TRIG_SRC_INTERNAL) {
        ECHECK(prec_generate_setTriggerSource(channel, 1))
    }
    return RP_OK;
}
