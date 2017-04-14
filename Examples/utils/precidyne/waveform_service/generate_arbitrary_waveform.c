
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

#include "redpitaya/rp.h"

#define M_PI 3.14159265358979323846
// Size of sample buffer is 16K. Right now all 4 buffers are the same size.
#define BUF_SIZE 16384
#define SAMPLE_PERIOD 1.0 / 125.0e+6
#define SAMPLE_RATE   125e+6

int main(int argc, char **argv){

	int i;
	//FILE *fp0, *fp1;
        float  x[BUF_SIZE]; // Sine wave array
        float  dc[BUF_SIZE]; // Array to save a DC value
        float  g1[BUF_SIZE];
        float  g2[BUF_SIZE];
        float  g3[BUF_SIZE];
        float  g4[BUF_SIZE];
        float  g5[BUF_SIZE];
        double g1_delay = M_PI; // # Since g1 is a cosine waveform, shift it by half a cycle to line up the stating point at 0
        double g2_delay = 0.0;
        double g3_delay = M_PI; // # Since g3 is a cosine waveform, shift it by half a cycle to line up the stating point at 0
        double g4_delay = 0.0;
        double g5_delay = 0.0;
        double g1_time  = 0.0; // # idx are temporary storage variables
        double g2_time  = 0.0;
        double g3_time  = 0.0;
        double g4_time  = 0.0;
        double g5_time  = 0.0;
        int    g1_num_samp;
        int    g2_num_samp;
        int    g3_num_samp;
        int    g4_num_samp;
        int    g5_num_samp;
        int    result;
        int    phase_bits = 0;
        int    is_started = 0;
        double radians  = 0.0;
        rp_pinState_t start_state, stop_state;
        double baseFreq = 1.25e+6; // base frequency from which all harmonics are derived
        
	/* Print error, if rp_Init() function failed */
	if(rp_Init() != RP_OK){
	    fprintf(stderr, "Rp api init failed!\n");
	}

        // Calculate the waveform data samples
        //##################################################
        //# Waveform generator has 4 buffers that can be loaded with a different waveform each.
        //# Each wave is made up of 16k samples that are driven at a fixed rate of 125MSPS
        //# 16k / 125msps = 7.6Khz is the minimum possible frequency
        //# For higher frequency, set the sample increment to be higher than 1 so that the 16k buffer is used up sooner
        //# Each buffer's waveform can be set to repeat up to 2^16 times. There is no delay between one buffer's wave ending and the second one starting.
        //# The FPGA always starts driving the waveform starting at buffer 0 and goes up to the last buffer 3. Then it loops back to start at buffer 0 again.

        //# This for loop actually calculates the sample values for each waveform
	//fp0=fopen("./ch_0_out.dat", "w+");
	//fp1=fopen("./ch_1_out.dat", "w+");
	prec_GenReset();
        for (i=0; i<BUF_SIZE; i++) {
            radians = (2 * M_PI * (i * SAMPLE_PERIOD) * baseFreq);
            g1_time = radians + g1_delay;
            g2_time = radians + g2_delay;
            g3_time = radians + g3_delay;
            g4_time = radians + g4_delay;
            g5_time = radians + g5_delay;
            //# delay is added to each sample counter to account for starting delay of each wave. each wave needs to start at sample value zero. sine waves do not need a delay. cosine needs a half cycle delay
            //# A DC and Sine wave for debug purposes
            dc[i] = 0.0;
            x[i]  = sin(radians);
            //# These 5 functions below are the ones that need to be modified to generate a new waveform, the same fuction has to be copied without 
            //# All samples are saved as floating point values separated by commas into a long string (Example - 0.1, 0.2, 0.3.... 1.0)
            if (radians < 3.0*2.0*M_PI) { //# g1 does 3 cycles at 1x base freq
                g1[i] = 0.5*(cos(g1_time) + cos(2*g1_time));
                g1_num_samp = i;
	    }
            if (radians < 3.0*2.0*M_PI) { //# g2 does 3 cycles at 1x base freq
                g2[i] = 0.5*(sin(g2_time) + sin(3*g2_time));
                g2_num_samp = i;
	    }
            if (radians < 3.0*2.0*M_PI) { //# g3 does 3 cycles at 1x base freq
                g3[i] = (0.5*(cos(g3_time) + cos(4*g3_time)));
                g3_num_samp = i;
	    }
            if (radians < 1.5*2.0*M_PI) { //# g4 does 3 cycles at 2x base freq
                g4[i] = (1.0*(sin(2*g4_time)));
                g4_num_samp = i;
	    }
            if (radians < 0.75*2.0*M_PI) { //# g5 does 3 cycles at 4x base freq
                g5[i] = (1.0*(sin(4*g5_time)));
                g5_num_samp = i;
	    }
	}

        //printf("Start wavegen listen service\n");
        rp_DpinSetDirection(RP_DIO0_N, RP_OUT);
        rp_DpinSetDirection(RP_DIO1_N, RP_OUT);
        rp_DpinSetDirection(RP_DIO2_N, RP_IN);
        rp_DpinSetDirection(RP_DIO3_N, RP_IN);

        //# Now transmit the calculated samples to the FPGA memory one buffer at a time. It takes about a second to transmit 16k samples which fills 1 of the 4 buffers. The delay mainly comes from the SCPI receiver parser being slow
        //# Channel 1
        prec_GenArbWaveform(RP_CH_1, 0, g4, 0*BUF_SIZE, g4_num_samp);
        prec_GenArbWaveform(RP_CH_1, 1, g1, 1*BUF_SIZE, g1_num_samp);
        prec_GenArbWaveform(RP_CH_1, 2, g5, 2*BUF_SIZE, g5_num_samp);
        prec_GenArbWaveform(RP_CH_1, 3, g3, 3*BUF_SIZE, g3_num_samp);
        phase_bits = (2<<6) + (1<<4) + (0<<2) + 1;
        prec_GenPhaseBits(RP_CH_1, 0, phase_bits); // Bad design, phase bits are always sent to BUF 0

        for (int buf_idx=0; buf_idx<4; buf_idx++) {
            prec_GenWaveform(RP_CH_1, buf_idx, PREC_WAVEFORM_ARBITRARY);
            prec_GenAmp(RP_CH_1, buf_idx, 0.1);
            prec_GenOffset(RP_CH_1, buf_idx, 0.0);
            prec_GenFreq(RP_CH_1, buf_idx, 7629.39453125); // Strange value guarantees one sample step at a time (16k samples at 125msps)-> pointerStep = (uint32_t) round(65536 * (frequency / DAC_FREQUENCY) * BUFFER_LENGTH);
            prec_GenBurstCount(RP_CH_1, buf_idx, 1);
        }
        prec_GenOutEnable(RP_CH_1);
        rp_DpinSetState(RP_DIO0_N, RP_HIGH);
        //prec_GenOutDisable(RP_CH_1);
        //rp_DpinSetState(RP_DIO0_N, RP_LOW);

        //# Channel 2
        prec_GenArbWaveform(RP_CH_2, 0, g4, 0*BUF_SIZE, g4_num_samp);
        prec_GenArbWaveform(RP_CH_2, 1, g2, 1*BUF_SIZE, g2_num_samp);
        prec_GenArbWaveform(RP_CH_2, 2, g5, 2*BUF_SIZE, g5_num_samp);
        prec_GenArbWaveform(RP_CH_2, 3, g2, 3*BUF_SIZE, g2_num_samp);
        phase_bits = (2<<6) + (1<<4) + (0<<2) + 1;
        prec_GenPhaseBits(RP_CH_2, 0, phase_bits); // Bad design, phase bits are always sent to BUF 0

        for (int buf_idx=0; buf_idx<4; buf_idx++) {
            prec_GenWaveform(RP_CH_2, buf_idx, PREC_WAVEFORM_ARBITRARY);
            prec_GenAmp(RP_CH_2, buf_idx, 0.1);
            prec_GenOffset(RP_CH_2, buf_idx, 0.0);
            prec_GenFreq(RP_CH_2, buf_idx, 7629.39453125); // Strange value guarantees one sample step at a time (16k samples at 125msps)-> pointerStep = (uint32_t) round(65536 * (frequency / DAC_FREQUENCY) * BUFFER_LENGTH);
            prec_GenBurstCount(RP_CH_2, buf_idx, 1);
        }
        prec_GenOutEnable(RP_CH_2);
        rp_DpinSetState(RP_DIO1_N, RP_HIGH);
        //prec_GenOutDisable(RP_CH_2);
        //rp_DpinSetState(RP_DIO1_N, RP_LOW);

        //# Trigger logic
        start_state = RP_HIGH;
        stop_state = RP_HIGH;
        is_started = 0;
        // Flush any old values in GPIO read logic
        for (int fls_idx=0; fls_idx<10; fls_idx++) {
            usleep(10000); // Sleep for 10ms
            rp_DpinGetState(RP_DIO2_N, &start_state);
            rp_DpinGetState(RP_DIO3_N, &stop_state);
        }
        while (1) {
            usleep(10000); // Sleep for 10ms
            rp_DpinGetState(RP_DIO2_N, &start_state);
            if (start_state == RP_LOW  && is_started == 0) { // if not started
                printf("Start trigger seen\n");
                prec_GenTrigger(3); // Trigger both channels
                for (int buf_idx=0; buf_idx<4; buf_idx++) {
                    prec_GenAmp(RP_CH_1, buf_idx, 0.1);
                    prec_GenAmp(RP_CH_2, buf_idx, 0.1);
                }
                rp_DpinSetState(RP_DIO0_N, RP_HIGH);
                rp_DpinSetState(RP_DIO1_N, RP_HIGH);
                rp_DpinSetState(RP_LED0, RP_HIGH);
                rp_DpinSetState(RP_LED1, RP_HIGH);
                is_started = 1;
            }
            rp_DpinGetState(RP_DIO3_N, &stop_state);
            if (stop_state == RP_LOW  && is_started == 1) { // if started
                printf("Stop trigger \n");
                for (int buf_idx=0; buf_idx<4; buf_idx++) {
                    prec_GenAmp(RP_CH_1, buf_idx, 0.0);
                    prec_GenAmp(RP_CH_2, buf_idx, 0.0);
                }
                rp_DpinSetState(RP_DIO0_N, RP_LOW);
                rp_DpinSetState(RP_DIO1_N, RP_LOW);
                rp_DpinSetState(RP_LED0, RP_LOW);
                rp_DpinSetState(RP_LED1, RP_LOW);
                //prec_GenReset(); // Reset the Wave gen to stop it
                is_started = 0;
            }
        }
            
        printf("%d %f %f %f %d Junk output", result, x[0], dc[0], g2[0], g2_num_samp);
	rp_Release();

/*
        //# Set the maximum voltage output value for each buffer
        //########
        //######## Calibration amplitude is always positive when written over SCPI.
        //######## Change the value of the samples to be negative if a negative value is needed
        //########
*/
}
