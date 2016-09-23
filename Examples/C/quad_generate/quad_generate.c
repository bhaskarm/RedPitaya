
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "fpga_awg.h"
#include "version.h"
#include "redpitaya/rp.h"

/**
 * GENERAL DESCRIPTION:
 *
 * The code below performs a function of a signal generator, which produces
 * a a signal of user-selectable pred-defined Signal shape
 * [Sine, Square, Triangle], Amplitude and Frequency on a selected Channel:
 *
 *
 *                   /-----\
 *   Signal shape -->|     | -->[data]--+-->[FPGA buf 1]--><DAC 1>
 *   Amplitude ----->| AWG |            |
 *   Frequency ----->|     |             -->[FPGA buf 2]--><DAC 2>
 *                   \-----/            ^
 *                                      |
 *   Channel ---------------------------+ 
 *
 *
 * This is achieved by first parsing the four parameters defining the 
 * signal properties from the command line, followed by synthesizing the 
 * signal in data[] buffer @ 125 MHz sample rate within the
 * generate_signal() function, depending on the Signal shape, Amplitude
 * and Frequency parameters. The data[] buffer is then transferred
 * to the specific FPGA buffer, defined by the Channel parameter -
 * within the write_signal_fpga() function.
 * The FPGA logic repeatably sends the data from both FPGA buffers to the
 * corresponding DACs @ 125 MHz, which in turn produces the synthesized
 * signal on Red Pitaya SMA output connectors labeled DAC1 & DAC2.
 *
 */

/** Maximal signal frequency [Hz] */
const double c_max_frequency = 62.5e6;

/** Minimal signal frequency [Hz] */
const double c_min_frequency = 0;

/** Maximal signal amplitude [Vpp] */
const double c_max_amplitude = 2.0;

/** AWG buffer length [samples]*/
#define MAX_NUM_SAMPLES AWG_SIG_LEN 

/** ENABLE or DISABLE the AWG trigger check **/
#define ENABLE_SCOPE_TRIGGER 0

/** AWG data buffer */
int32_t data[MAX_NUM_SAMPLES];

/** Program name */
const char *g_argv0 = NULL;

/** AWG FPGA parameters */
typedef struct {
    int32_t  offsgain;   ///< AWG offset & gain.
    uint32_t wrap;       ///< AWG buffer wrap value.
    uint32_t step;       ///< AWG step interval.
} awg_param_t;

/* Forward declarations */
void synthesize_signal(double ampl, double freq, uint32_t type,
                       int32_t *data,
                       awg_param_t *params);
void write_data_fpga(uint32_t ch,
                     const int32_t *data,
                     const awg_param_t *awg, uint32_t buf_index);

/** Print usage information */
void usage() {
    printf("generate : \n Generates an output of 2 mixed waveforms.\n");
    printf("    Options : amplitude - Max amplitude of the waveform\n");
    printf("    Example : generate 1\n");
}

/** Signal generator main */
int main(int argc, char *argv[])
{
#if ENABLE_SCOPE_TRIGGER
    uint32_t adc_buff_size = 128;
    int16_t *adc_buff = (int16_t *)malloc(adc_buff_size * sizeof(int16_t));
#endif
    if ( argc < 2 ) {
        usage();
        return -1;
    }

#if ENABLE_SCOPE_TRIGGER
    /* Print error, if rp_Init() function failed */
    if(rp_Init() != RP_OK){
       fprintf(stderr, "Rp api init failed!\n");
    }
#endif

    /* Signal amplitude argument parsing */
    double ampl = strtod(argv[1], NULL);
    if ( (ampl < 0.0) || (ampl > c_max_amplitude) ) {
        fprintf(stderr, "Invalid amplitude: %f\n", ampl);
        usage();
        return -1;
    }

    /* Signal frequency argument parsing */
    double freq = 400000; // 40khz

    /* Check frequency limits */
    if ( (freq < c_min_frequency) || (freq > c_max_frequency ) ) {
        fprintf(stderr, "Invalid frequency: %f\n", freq);
        usage();
        return -1;
    }

    awg_param_t params;
    printf("Waveform => Sine, %f Hz, %f V\n", freq, ampl);
    fpga_awg_init();

    for (uint32_t ch=0; ch<2; ch++) {
      synthesize_signal(ampl, freq, 0, data, &params); // Build the wave first
      write_data_fpga(ch, data, &params, 0); // Load it into the buffer along with the config
     
      synthesize_signal(ampl, freq, 1, data, &params); 
      write_data_fpga(ch, data, &params, 1);
     
      synthesize_signal(ampl, freq, 2, data, &params); 
      write_data_fpga(ch, data, &params, 2);
     
      synthesize_signal(ampl, freq, 3, data, &params); 
      write_data_fpga(ch, data, &params, 3);
    }
#if ENABLE_SCOPE_TRIGGER
    // Enable the acquire module to check for cycles done trigger
    rp_AcqReset();
    rp_AcqSetDecimation(RP_DEC_8);
    rp_AcqSetTriggerLevel(0);
    rp_AcqSetTriggerDelay(0);
    rp_AcqStart();

    rp_AcqSetTriggerSrc(RP_TRIG_SRC_AWG_PE); // wait for positive edge on the ASG
    rp_GenTrigger(1); // activate the trigger for the first while loop
    rp_acq_trig_state_t state = RP_TRIG_STATE_TRIGGERED;
#endif

    printf(" \n\n");
    //g_awg_reg->state_machine_conf = 0x110011; // trigger both channels
    g_awg_reg->state_machine_conf = 0x110011;
    uint32_t trigger_counter = 0;
    while (1) {
#if ENABLE_SCOPE_TRIGGER
      rp_AcqGetTriggerState(&state);
      if(state == RP_TRIG_STATE_TRIGGERED) {
        //trigger_counter++;
        rp_AcqGetWritePointer(&trigger_counter);
        rp_AcqGetLatestDataRaw(RP_CH_1, &adc_buff_size, adc_buff);
        rp_AcqSetTriggerSrc(RP_TRIG_SRC_AWG_PE); // wait for positive edge on the ASG
        //printf("%d\n", trigger_counter);
        //g_awg_reg->state_machine_conf = 0x000011;
      }
#else 
      trigger_counter++;
      //printf("%d\n", trigger_counter);
      usleep(10);
      //g_awg_reg->state_machine_conf = 0x000011;
#endif
    }
    fpga_awg_exit();

#if ENABLE_SCOPE_TRIGGER
    rp_Release();
#endif
}

/**
 * Synthesize a desired signal.
 *
 * Generates/synthesized  a signal, based on three pre-defined signal
 * types/shapes, signal amplitude & frequency. The data[] vector of 
 * samples at 125 MHz is generated to be re-played by the FPGA AWG module.
 *
 * @param ampl  Signal amplitude [Vpp].
 * @param freq  Signal frequency [Hz].
 * @param type  Signal type/shape [Sine, Square, Triangle].
 * @param data  Returned synthesized AWG data vector.
 * @param awg   Returned AWG parameters.
 *
 */
void synthesize_signal(double ampl, double freq, uint32_t type,
                       int32_t *data,
                       awg_param_t *awg) {

    uint32_t i;

    /* Various locally used constants - HW specific parameters */
    const int dcoffs = -155;
    const int trans0 = 30;
    const int trans1 = 300;

    /* This is where frequency is used... */
    awg->offsgain = (dcoffs << 16) + 0x1fff;
    awg->step = 0x100; //round(65536lu * freq/c_awg_smpl_freq * MAX_NUM_SAMPLES);
    awg->wrap = round(65536lu * MAX_NUM_SAMPLES - 1);

    int trans = freq / 1e6 * trans1; /* 300 samples at 1 MHz */
    uint32_t amp = ampl * 4000.0;    /* 1 Vpp ==> 4000 DAC counts */
    if (amp > 8191) {
        /* Truncate to max value if needed */
        amp = 8191;
    }

    if (trans <= 10) {
        trans = trans0;
    }

    /* Fill data[] with appropriate buffer samples */
    printf(" \n\nGenerating Random \n\n");
    for(i = 0; i < MAX_NUM_SAMPLES; i++) {
        /* Sine 20Mhz*/
        if (type == 0) {
          data[i] = 0;
        }
        if (type == 1) { // 1 is a triangle
          data[i] = i/2;
          //printf(" %d ", data[i]);
        }
        if (type == 2) { // 2 is a mixed sine wave
          if (i < MAX_NUM_SAMPLES/4) {
            data[i] = round(amp * sin(4*M_PI*(double)i/(double)MAX_NUM_SAMPLES));
	  } else if (i > MAX_NUM_SAMPLES/4 && i < MAX_NUM_SAMPLES/2) {
            data[i] = round(amp * sin(4*M_PI*(double)(i-(MAX_NUM_SAMPLES/4))/(double)MAX_NUM_SAMPLES));
	  } else if (i > MAX_NUM_SAMPLES/2 && i < 3*MAX_NUM_SAMPLES/4) {
            //data[i] = round(-amp * sin(4*M_PI*(double)(i-(MAX_NUM_SAMPLES/2))/(double)MAX_NUM_SAMPLES));
            data[i] = 0;
	  } else {
            //data[i] = round(-amp * sin(4*M_PI*(double)(i-(MAX_NUM_SAMPLES*3/4))/(double)MAX_NUM_SAMPLES));
            data[i] = 0;
	  }
          //printf(" %d ", data[i]);
        }
        if (type == 3) { // 1 is a sine wave
          data[i] = round(amp * sin(2*M_PI*(double)i/(double)MAX_NUM_SAMPLES));
          //printf(" %d ", data[i]);
        }
        if (type == 4) {
          data[i] = amp/2;
        }
    }
}

/**
 * Write synthesized data[] to FPGA buffer.
 *
 * @param ch    Channel number [0, 1].
 * @param data  AWG data to write to FPGA.
 * @param awg   AWG paramters to write to FPGA.
 */
void write_data_fpga(uint32_t ch,
                     const int32_t *data,
                     const awg_param_t *awg, uint32_t buf_index) {

    uint32_t i;
    uint32_t addr_calc_temp;

    //fpga_awg_init();

    //#### DO NOT CHANGE THIS VALUE
    g_awg_reg->all_ch_trig_out_cond = 0x000002; // Important setting to keep at 2 for ping pong buffer
    if(ch == 0) {
        printf(" Channel A\n ");
        if (buf_index == 0) {
          /* Channel A */
          printf(" Buffer 0 \n ");
          g_awg_reg->cha_scale_off      = awg->offsgain;
          addr_calc_temp                = (MAX_NUM_SAMPLES - 4)<<16;
          g_awg_reg->cha_count_wrap     = addr_calc_temp;
          g_awg_reg->cha_count_step     = awg->step;
          g_awg_reg->cha_start_off      = 0;
          g_awg_reg->cha_num_cyc        = 2;
          for(i = 0; i < MAX_NUM_SAMPLES; i++) {
              g_awg_cha_mem[i] = data[i];
              //printf(" %d ", data[i]);
          }
          printf(" Buffer 0 size =%d \n ", g_awg_reg->cha_count_wrap);
        } else if (buf_index == 1) {
          printf(" Buffer 1 \n ");
          g_awg_reg->cha_scale_off_1    = awg->offsgain;
          addr_calc_temp                = (2*MAX_NUM_SAMPLES - 4)<<16;
          g_awg_reg->cha_count_wrap_1   = addr_calc_temp;
          g_awg_reg->cha_count_step_1   = awg->step;
          addr_calc_temp                = (MAX_NUM_SAMPLES)<<16;
          g_awg_reg->cha_start_off_1    = addr_calc_temp;
          g_awg_reg->cha_num_cyc_1      = 3;
          for(i = 0; i < MAX_NUM_SAMPLES; i++) {
              g_awg_cha_mem[MAX_NUM_SAMPLES+i] = data[i];
              //printf(" %d ", data[i]);
          }
          printf(" Buffer 1 size =%d \n ", g_awg_reg->cha_count_wrap_1);
        } else if (buf_index == 2) {
          printf(" Buffer 2 \n ");
          g_awg_reg->cha_scale_off_2    = awg->offsgain;
          addr_calc_temp                = (3*MAX_NUM_SAMPLES - 4)<<16;
          g_awg_reg->cha_count_wrap_2   = addr_calc_temp;
          g_awg_reg->cha_count_step_2   = awg->step;
          addr_calc_temp                = (2*MAX_NUM_SAMPLES)<<16;
          g_awg_reg->cha_start_off_2    = addr_calc_temp;
          g_awg_reg->cha_num_cyc_2      = 4;
          for(i = 0; i < MAX_NUM_SAMPLES; i++) {
              g_awg_cha_mem[2*MAX_NUM_SAMPLES+i] = data[i];
              //printf(" %d ", data[i]);
          }
          printf(" Buffer 2 size =%d \n ", g_awg_reg->cha_count_wrap_2);
        } else if (buf_index == 3) {
          printf(" Buffer 3 \n ");
          g_awg_reg->cha_scale_off_3    = awg->offsgain;
          addr_calc_temp                = (4*MAX_NUM_SAMPLES - 4)<<16;
          g_awg_reg->cha_count_wrap_3   = addr_calc_temp;
          g_awg_reg->cha_count_step_3   = awg->step;
          addr_calc_temp                = (3*MAX_NUM_SAMPLES)<<16;
          g_awg_reg->cha_start_off_3    = addr_calc_temp;
          g_awg_reg->cha_num_cyc_3      = 5;
          for(i = 0; i < MAX_NUM_SAMPLES; i++) {
              g_awg_cha_mem[3*MAX_NUM_SAMPLES+i] = data[i];
              //printf(" %d ", data[i]);
          }
          printf(" Buffer 3 size =%d \n ", g_awg_reg->cha_count_wrap_3);
        }
    } else {
        printf(" Channel B\n ");
        if (buf_index == 0) {
          /* Channel B */
          printf(" Buffer 0 \n ");
          g_awg_reg->chb_scale_off      = awg->offsgain;
          addr_calc_temp                = (MAX_NUM_SAMPLES - 4)<<16;
          g_awg_reg->chb_count_wrap     = addr_calc_temp;
          g_awg_reg->chb_count_step     = awg->step;
          g_awg_reg->chb_start_off      = 0;
          g_awg_reg->chb_num_cyc        = 2;
          for(i = 0; i < MAX_NUM_SAMPLES; i++) {
              g_awg_chb_mem[i] = data[i];
              //printf(" %d ", data[i]);
          }
          printf(" Buffer 3 size =%d \n ", g_awg_reg->chb_count_wrap);
        } else if (buf_index == 1) {
          printf(" Buffer 1 \n ");
          g_awg_reg->chb_scale_off_1    = awg->offsgain;
          addr_calc_temp                = (2*MAX_NUM_SAMPLES - 4)<<16;
          g_awg_reg->chb_count_wrap_1   = addr_calc_temp;
          g_awg_reg->chb_count_step_1   = awg->step;
          addr_calc_temp                = (MAX_NUM_SAMPLES)<<16;
          g_awg_reg->chb_start_off_1    = addr_calc_temp;
          g_awg_reg->chb_num_cyc_1      = 3;
          for(i = 0; i < MAX_NUM_SAMPLES; i++) {
              g_awg_chb_mem[MAX_NUM_SAMPLES+i] = data[i];
              //printf(" %d ", data[i]);
          }
          printf(" Buffer 3 size =%d \n ", g_awg_reg->chb_count_wrap_3);
        } else if (buf_index == 2) {
          printf(" Buffer 2 \n ");
          g_awg_reg->chb_scale_off_2    = awg->offsgain;
          addr_calc_temp                = (3*MAX_NUM_SAMPLES - 4)<<16;
          g_awg_reg->chb_count_wrap_2   = addr_calc_temp;
          g_awg_reg->chb_count_step_2   = awg->step;
          addr_calc_temp                = (2*MAX_NUM_SAMPLES)<<16;
          g_awg_reg->chb_start_off_2    = addr_calc_temp;
          g_awg_reg->chb_num_cyc_2      = 4;
          for(i = 0; i < MAX_NUM_SAMPLES; i++) {
              g_awg_chb_mem[2*MAX_NUM_SAMPLES+i] = data[i];
              //printf(" %d ", data[i]);
          }
          printf(" Buffer 3 size =%d \n ", g_awg_reg->chb_count_wrap_3);
        } else if (buf_index == 3) {
          printf(" Buffer 3 \n ");
          g_awg_reg->chb_scale_off_3    = awg->offsgain;
          addr_calc_temp                = (4*MAX_NUM_SAMPLES - 4)<<16;
          g_awg_reg->chb_count_wrap_3   = addr_calc_temp;
          g_awg_reg->chb_count_step_3   = awg->step;
          addr_calc_temp                = (3*MAX_NUM_SAMPLES)<<16;
          g_awg_reg->chb_start_off_3    = addr_calc_temp;
          g_awg_reg->chb_num_cyc_3      = 5;
          for(i = 0; i < MAX_NUM_SAMPLES; i++) {
              g_awg_chb_mem[3*MAX_NUM_SAMPLES+i] = data[i];
              //printf(" %d ", data[i]);
          }
          printf(" Buffer 3 size =%d \n ", g_awg_reg->chb_count_wrap_3);
        }
    }

    /* Enable both channels */
    /* TODO: Should this only happen for the specified channel?
     *       Otherwise, the not-to-be-affected channel is restarted as well
     *       causing unwanted disturbances on that channel.
     */
    //g_awg_reg->state_machine_conf = 0x110011;

    //fpga_awg_exit();
}
