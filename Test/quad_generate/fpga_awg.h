/** 
 * $Id: fpga_awg.h 881 2013-12-16 05:37:34Z rp_jmenart $
 * 
 * @brief Red Pitaya Arbitrary Waveform Generator (AWG) FPGA controller.
 *
 * @Author Jure Menart <juremenart@gmail.com>
 * 
 * (c) Red Pitaya  http://www.redpitaya.com
 *
 * This part of code is written in C programming language.
 * Please visit http://en.wikipedia.org/wiki/C_(programming_language)
 * for more details on the language used herein.
 */

#ifndef _FPGA_AWG_H_
#define _FPGA_AWG_H_

#include <stdint.h>

/** Base AWG FPGA address */
#define AWG_BASE_ADDR 0x40200000
/** Offset to be subtracted from AWG base address to get OSC base address */
#define OSC_OFFSET 0x00100000
/** Base AWG FPGA core size */
#define AWG_BASE_SIZE 0xC0000
/** FPGA AWG output signal buffer length */
#define AWG_SIG_LEN   (16*1024)
/** FPGA AWG output signal 1 offset */
#define AWG_CHA_OFFSET    0x40000
/** FPGA AWG output signal 2 offset */
#define AWG_CHB_OFFSET    0x80000

/** @brief AWG FPGA registry structure.
 *
 * This structure is direct image of physical FPGA memory. When accessing it all
 * reads/writes are performed directly from/to FPGA AWG.
 */
typedef struct awg_reg_t {
    /** @brief Offset 0x00 - State machine configuration
     *
     * State machine configuration register (offset 0x00):
     *  bits [31:24] - Reserved
     *  bit  [   23] - Channel B output set to 0
     *  bit  [   22] - Channel B state machine reset
     *  bit  [   21] - Channel B set one time trigger
     *  bit  [   20] - Channel B state machine wrap pointer
     *  bits [19:16] - Channel B trigger selector 
     *  bits [15: 8] - Reserved
     *  bit  [    7] - Channel B output set to 0
     *  bit  [    6] - Channel B state machine reset
     *  bit  [    5] - Channel B set one time trigger
     *  bit  [    4] - Channel B state machine wrap pointer
     *  bits [ 3: 0] - Channel B trigger selector 
     *
     */
    uint32_t state_machine_conf;
    /** @brief Offset 0x04 - Channel A amplitude scale and offset 
     *
     * Channel A amplitude scale and offset register (offset 0x04) used to set the
     * amplitude and scale of output signal: out = (data * scale)/0x2000 + offset
     * bits [31:30] - Reserved
     * bits [29:16] - Amplitude offset
     * bits [15:14] - Reserved 
     * bits [13: 0] - Amplitude scale (0x2000 == multiply by 1, unsigned)
     */
    uint32_t cha_scale_off;
    /** @brief Offset 0x08 - Channel A counter wrap
     *
     * Channel A counter wrap (offset 0x08) - value at which FPGA AWG state 
     * machine will wrap the output signal buffer readout:
     * bits [31:30] - Reserved
     * bits [29: 0] - Output signal counter wrap
     */
    uint32_t cha_count_wrap;
    /** @brief Offset 0x0C - Channel A starting counter offset
     *
     * Channel A starting counter offset (offset 0x0C) - start offset when 
     * trigger arrives
     * bits [31:30] - Reserved
     * bits [29: 0] - Counter start offset
     */
    uint32_t cha_start_off;
    /** @brief Offset 0x10 - Channel A counter step
     *
     * Channel A counter step (offset 0x10) - value by which FPGA AWG state 
     * machine increment readout from output signal buffer:
     * bits [31:30] - Reserved
     * bits [29: 0] - Counter step
     */
    uint32_t cha_count_step;

    /** @brief Description missing */
    uint32_t cha_rd_ptr_ro;
    uint32_t cha_num_cyc;
    uint32_t cha_num_rpt;
    uint32_t cha_rpt_dly;

    /** @brief Description missing, second buffer for chan A */
    uint32_t cha_scale_off_1;
    uint32_t cha_count_wrap_1;
    uint32_t cha_start_off_1;
    uint32_t cha_count_step_1;
    uint32_t cha_rd_ptr_ro_1;
    uint32_t cha_num_cyc_1;
    uint32_t cha_num_rpt_1;
    uint32_t cha_rpt_dly_1;
    /** @brief Description missing, third buffer for chan A */
    uint32_t cha_scale_off_2;
    uint32_t cha_count_wrap_2;
    uint32_t cha_start_off_2;
    uint32_t cha_count_step_2;
    uint32_t cha_rd_ptr_ro_2;
    uint32_t cha_num_cyc_2;
    uint32_t cha_num_rpt_2;
    uint32_t cha_rpt_dly_2;
    /** @brief Description missing, fourth buffer for chan A */
    uint32_t cha_scale_off_3;
    uint32_t cha_count_wrap_3;
    uint32_t cha_start_off_3;
    uint32_t cha_count_step_3;
    uint32_t cha_rd_ptr_ro_3;
    uint32_t cha_num_cyc_3;
    uint32_t cha_num_rpt_3;
    uint32_t cha_rpt_dly_3;
    
    /** @brief Offset 0x24 - Channel B amplitude scale and offset 
     *
     * Channel B amplitude scale and offset register (offset 0x24) used to set the
     * amplitude and scale of output signal: out = (data * scale)/0x2000 + offset
     * bits [31:30] - Reserved
     * bits [29:16] - Amplitude offset
     * bits [15:14] - Reserved 
     * bits [13: 0] - Amplitude scale (0x2000 == multiply by 1, unsigned)
     */
    uint32_t chb_scale_off;
    /** @brief Offset 0x28 - Channel B counter wrap
     *
     * Channel B counter wrap (offset 0x28) - value at which FPGA AWG state 
     * machine will wrap the output signal buffer readout:
     * bits [31:30] - Reserved
     * bits [29: 0] - Output signal counter wrap
     */
    uint32_t chb_count_wrap;
    /** @brief Offset 0x2C - Channel B starting counter offset
     *
     * Channel B starting counter offset (offset 0x2C) - start offset when 
     * trigger arrives
     * bits [31:30] - Reserved
     * bits [29: 0] - Counter start offset
     */
    uint32_t chb_start_off;
    /** @brief Offset 0x30 - Channel B counter step
     *
     * Channel B counter step (offset 0x30) - value by which FPGA AWG state 
     * machine increment readout from output signal buffer:
     * bits [31:30] - Reserved
     * bits [29: 0] - Counter step
     */
    uint32_t chb_count_step;

    /** @brief Description missing */
    uint32_t chb_rd_ptr_ro;
    uint32_t chb_num_cyc;
    uint32_t chb_num_rpt;
    uint32_t chb_rpt_dly;

    /** @brief Description missing, second buffer for chan B */
    uint32_t chb_scale_off_1;
    uint32_t chb_count_wrap_1;
    uint32_t chb_start_off_1;
    uint32_t chb_count_step_1;
    uint32_t chb_rd_ptr_ro_1;
    uint32_t chb_num_cyc_1;
    uint32_t chb_num_rpt_1;
    uint32_t chb_rpt_dly_1;
    /** @brief Description missing, third buffer for chan B */
    uint32_t chb_scale_off_2;
    uint32_t chb_count_wrap_2;
    uint32_t chb_start_off_2;
    uint32_t chb_count_step_2;
    uint32_t chb_rd_ptr_ro_2;
    uint32_t chb_num_cyc_2;
    uint32_t chb_num_rpt_2;
    uint32_t chb_rpt_dly_2;
    /** @brief Description missing, fourth buffer for chan B */
    uint32_t chb_scale_off_3;
    uint32_t chb_count_wrap_3;
    uint32_t chb_start_off_3;
    uint32_t chb_count_step_3;
    uint32_t chb_rd_ptr_ro_3;
    uint32_t chb_num_cyc_3;
    uint32_t chb_num_rpt_3;
    uint32_t chb_rpt_dly_3;
    
    /** @brief Description missing, double buf related registers */
    uint32_t all_ch_trig_out_cond;
    uint32_t reserved_a[3];

} awg_reg_t;

/** @} */

/* Description of the following variables or function declarations is in 
 * fpga_awg.c 
 */
extern awg_reg_t    *g_awg_reg;
extern uint32_t     *g_osc_reg;
extern uint32_t     *g_awg_cha_mem;
extern uint32_t     *g_awg_chb_mem;
extern const double  c_awg_smpl_freq;

int fpga_awg_init(void);
int fpga_awg_exit(void);

#endif // _FPGA_AWG_H_
