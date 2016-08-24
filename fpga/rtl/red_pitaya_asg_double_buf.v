/**
 * $Id: red_pitaya_asg.v 961 2014-01-21 11:40:39Z matej.oblak $
 *
 * @brief Red Pitaya arbitrary signal generator (ASG).
 *
 * @Author Matej Oblak
 *
 * (c) Red Pitaya  http://www.redpitaya.com
 *
 * This part of code is written in Verilog hardware description language (HDL).
 * Please visit http://en.wikipedia.org/wiki/Verilog
 * for more details on the language used herein.
 */

/**
 * GENERAL DESCRIPTION:
 *
 * Arbitrary signal generator takes data stored in buffer and sends them to DAC.
 *
 *
 *                /-----\         /--------\
 *   SW --------> | BUF | ------> | kx + o | ---> DAC CHA
 *                \-----/         \--------/
 *                   ^
 *                   |
 *                /-----\
 *   SW --------> |     |
 *                | FSM | ------> trigger notification
 *   trigger ---> |     |
 *                \-----/
 *                   |
 *                   ˇ
 *                /-----\         /--------\
 *   SW --------> | BUF | ------> | kx + o | ---> DAC CHB
 *                \-----/         \--------/ 
 *
 *
 * Buffers are filed with SW. It also sets finite state machine which take control
 * over read pointer. All registers regarding reading from buffer has additional 
 * 16 bits used as decimal points. In this way we can make better ratio betwen 
 * clock cycle and frequency of output signal. 
 *
 * Finite state machine can be set for one time sequence or continously wrapping.
 * Starting trigger can come from outside, notification trigger used to synchronize
 * with other applications (scope) is also available. Both channels are independant.
 *
 * Output data is scaled with linear transmormation.
 * 
 */

module red_pitaya_asg_double_buf (
  // DAC
  output     [ 14-1: 0] dac_a_o   ,  // DAC data CHA
  output     [ 14-1: 0] dac_b_o   ,  // DAC data CHB
  input                 dac_clk_i ,  // DAC clock
  input                 dac_rstn_i,  // DAC reset - active low
  input                 trig_a_i  ,  // starting trigger CHA
  input                 trig_b_i  ,  // starting trigger CHB
  output                trig_out_o,  // notification trigger
  // System bus
  input      [ 32-1: 0] sys_addr  ,  // bus address
  input      [ 32-1: 0] sys_wdata ,  // bus write data
  input      [  4-1: 0] sys_sel   ,  // bus write byte select
  input                 sys_wen   ,  // bus write enable
  input                 sys_ren   ,  // bus read enable
  output reg [ 32-1: 0] sys_rdata ,  // bus read data
  output reg            sys_err   ,  // bus error indicator
  output reg            sys_ack   ,  // bus acknowledge signal
   // Debug signals
  output         [16-1: 0] debug_bus
);

//---------------------------------------------------------------------------------
//
// generating signal from DAC table 

localparam RSZ = 16 ;  // RAM size 2^RSZ

reg   [  14-1: 0] set_a_amp_0  , set_b_amp_0  ;
reg   [  14-1: 0] set_a_dc_0   , set_b_dc_0   ;
reg   [RSZ+15: 0] set_a_size_0 , set_b_size_0 ;
reg   [RSZ+15: 0] set_a_step_0 , set_b_step_0 ;
reg   [RSZ+15: 0] set_a_ofs_0  , set_b_ofs_0  ;
reg   [  16-1: 0] set_a_ncyc_0 , set_b_ncyc_0 ;
reg   [  16-1: 0] set_a_rnum_0 , set_b_rnum_0 ;
reg   [  32-1: 0] set_a_rdly_0 , set_b_rdly_0 ;
reg   [  14-1: 0] set_a_amp_1  , set_b_amp_1  ;
reg   [  14-1: 0] set_a_dc_1   , set_b_dc_1   ;
reg   [RSZ+15: 0] set_a_size_1 , set_b_size_1 ;
reg   [RSZ+15: 0] set_a_step_1 , set_b_step_1 ;
reg   [RSZ+15: 0] set_a_ofs_1  , set_b_ofs_1  ;
reg   [  16-1: 0] set_a_ncyc_1 , set_b_ncyc_1 ;
reg   [  16-1: 0] set_a_rnum_1 , set_b_rnum_1 ;
reg   [  32-1: 0] set_a_rdly_1 , set_b_rdly_1 ;
reg               set_a_rst    , set_b_rst    ;
reg               set_a_once   , set_b_once   ;
reg               set_a_wrap   , set_b_wrap   ;
reg               set_a_zero   , set_b_zero   ;
reg               set_a_rgate  , set_b_rgate  ;
reg               buf_a_we     , buf_b_we     ;
reg   [ RSZ-1: 0] buf_a_addr   , buf_b_addr   ;
wire  [  14-1: 0] buf_a_rdata  , buf_b_rdata  ;
wire  [ RSZ-1: 0] buf_a_rpnt   , buf_b_rpnt   ;
reg   [  32-1: 0] buf_a_rpnt_rd, buf_b_rpnt_rd;
reg               trig_a_sw    , trig_b_sw    ;
reg   [   3-1: 0] trig_a_src   , trig_b_src   ;
wire              trig_a_done  , trig_b_done  ;
reg               trig_evt_ab                 ;
reg   [   3-1: 0] trig_evt                    ;
wire  [  15-1: 0] ch_a_debug   , ch_b_debug   ;

assign debug_bus = {trig_out_o, ch_a_debug };

red_pitaya_asg_ch_double_buf  #(.RSZ (RSZ)) ch [1:0] (
  // DAC
  .dac_o           ({dac_b_o          , dac_a_o          }),  // dac data output
  .dac_clk_i       ({dac_clk_i        , dac_clk_i        }),  // dac clock
  .dac_rstn_i      ({dac_rstn_i       , dac_rstn_i       }),  // dac reset - active low
  // trigger
  .trig_sw_i       ({trig_b_sw        , trig_a_sw        }),  // software trigger
  .trig_ext_i      ({trig_b_i         , trig_a_i         }),  // external trigger
  .trig_src_i      ({trig_b_src       , trig_a_src       }),  // trigger source selector
  .trig_done_o     ({trig_b_done      , trig_a_done      }),  // trigger event
  .trig_evt_i      ({trig_evt         , trig_evt         }),  // trigger event condition (0: start of waveform, 1: counter wrap, 2-7: reserved)
  // buffer ctrl
  .buf_we_i        ({buf_b_we         , buf_a_we         }),  // buffer buffer write
  .buf_addr_i      ({buf_b_addr       , buf_a_addr       }),  // buffer address
  .buf_wdata_i     ({sys_wdata[14-1:0], sys_wdata[14-1:0]}),  // buffer write data
  .buf_rdata_o     ({buf_b_rdata      , buf_a_rdata      }),  // buffer read data
  .buf_rpnt_o      ({buf_b_rpnt       , buf_a_rpnt       }),  // buffer current read pointer
  // configuration
  .set_amp_i_0     ({set_b_amp_0      , set_a_amp_0      }),  // set amplitude scale
  .set_dc_i_0      ({set_b_dc_0       , set_a_dc_0       }),  // set output offset
  .set_size_i_0    ({set_b_size_0     , set_a_size_0     }),  // set table data size
  .set_step_i_0    ({set_b_step_0     , set_a_step_0     }),  // set pointer step
  .set_ofs_i_0     ({set_b_ofs_0      , set_a_ofs_0      }),  // set reset offset
  .set_ncyc_i_0    ({set_b_ncyc_0     , set_a_ncyc_0     }),  // set number of cycle
  .set_rnum_i_0    ({set_b_rnum_0     , set_a_rnum_0     }),  // set number of repetitions
  .set_rdly_i_0    ({set_b_rdly_0     , set_a_rdly_0     }),  // set delay between repetitions
  .set_amp_i_1     ({set_b_amp_1      , set_a_amp_1      }),  // set amplitude scale
  .set_dc_i_1      ({set_b_dc_1       , set_a_dc_1       }),  // set output offset
  .set_size_i_1    ({set_b_size_1     , set_a_size_1     }),  // set table data size
  .set_step_i_1    ({set_b_step_1     , set_a_step_1     }),  // set pointer step
  .set_ofs_i_1     ({set_b_ofs_1      , set_a_ofs_1      }),  // set reset offset
  .set_ncyc_i_1    ({set_b_ncyc_1     , set_a_ncyc_1     }),  // set number of cycle
  .set_rnum_i_1    ({set_b_rnum_1     , set_a_rnum_1     }),  // set number of repetitions
  .set_rdly_i_1    ({set_b_rdly_1     , set_a_rdly_1     }),  // set delay between repetitions
  .set_rst_i       ({set_b_rst        , set_a_rst        }),  // set FMS to reset
  .set_once_i      ({set_b_once       , set_a_once       }),  // set only once
  .set_wrap_i      ({set_b_wrap       , set_a_wrap       }),  // set wrap pointer
  .set_zero_i      ({set_b_zero       , set_a_zero       }),  // set output to zero
  .set_rgate_i     ({set_b_rgate      , set_a_rgate      }),  // set external gated repetition
  .debug_bus       ({ch_b_debug       , ch_a_debug       })
);

always @(posedge dac_clk_i)
begin
   buf_a_we   <= sys_wen && (sys_addr[19:RSZ+2] == 'h1);
   buf_b_we   <= sys_wen && (sys_addr[19:RSZ+2] == 'h2);
   buf_a_addr <= sys_addr[RSZ+1:2] ;  // address timing violation
   buf_b_addr <= sys_addr[RSZ+1:2] ;  // can change only synchronous to write clock
end

assign trig_out_o = trig_evt_ab ? trig_b_done : trig_a_done ;

//---------------------------------------------------------------------------------
//
//  System bus connection

reg  [3-1: 0] ren_dly ;
reg           ack_dly ;

always @(posedge dac_clk_i)
if (dac_rstn_i == 1'b0) begin
   trig_a_sw   <=  1'b0    ;
   trig_a_src  <=  3'h0    ;
   set_a_zero  <=  1'b0    ;
   set_a_rst   <=  1'b0    ;
   set_a_once  <=  1'b0    ;
   set_a_wrap  <=  1'b0    ;
   set_a_amp_0  <= 14'h2000 ;
   set_a_dc_0   <= 14'h0    ;
   set_a_size_0 <= {RSZ+16{1'b1}} ;
   set_a_ofs_0  <= {RSZ+16{1'b0}} ;
   set_a_step_0 <={{RSZ+15{1'b0}},1'b0} ;
   set_a_ncyc_0 <= 16'h0    ;
   set_a_rnum_0 <= 16'h0    ;
   set_a_rdly_0 <= 32'h0    ;
   set_a_amp_1  <= 14'h2000 ;
   set_a_dc_1   <= 14'h0    ;
   set_a_size_1 <= {RSZ+16{1'b1}} ;
   set_a_ofs_1  <= {RSZ+16{1'b0}} ;
   set_a_step_1 <={{RSZ+15{1'b0}},1'b0} ;
   set_a_ncyc_1 <= 16'h0    ;
   set_a_rnum_1 <= 16'h0    ;
   set_a_rdly_1 <= 32'h0    ;
   set_a_rgate <=  1'b0    ;
   trig_b_sw   <=  1'b0    ;
   trig_b_src  <=  3'h0    ;
   set_b_zero  <=  1'b0    ;
   set_b_rst   <=  1'b0    ;
   set_b_once  <=  1'b0    ;
   set_b_wrap  <=  1'b0    ;
   set_b_amp_0  <= 14'h2000 ;
   set_b_dc_0   <= 14'h0    ;
   set_b_size_0 <= {RSZ+16{1'b1}} ;
   set_b_ofs_0  <= {RSZ+16{1'b0}} ;
   set_b_step_0 <={{RSZ+15{1'b0}},1'b0} ;
   set_b_ncyc_0 <= 16'h0    ;
   set_b_rnum_0 <= 16'h0    ;
   set_b_rdly_0 <= 32'h0    ;
   set_b_amp_1  <= 14'h2000 ;
   set_b_dc_1   <= 14'h0    ;
   set_b_size_1 <= {RSZ+16{1'b1}} ;
   set_b_ofs_1  <= {RSZ+16{1'b0}} ;
   set_b_step_1 <={{RSZ+15{1'b0}},1'b0} ;
   set_b_ncyc_1 <= 16'h0    ;
   set_b_rnum_1 <= 16'h0    ;
   set_b_rdly_1 <= 32'h0    ;
   set_b_rgate <=  1'b0    ;
   ren_dly     <=  3'h0    ;
   ack_dly     <=  1'b0    ;
   trig_evt_ab <=  1'b0    ;
   trig_evt    <=  3'h0    ;
end else begin
   trig_a_sw  <= sys_wen && (sys_addr[19:0]==20'h0) && sys_wdata[0]  ;
   if (sys_wen && (sys_addr[19:0]==20'h0))
      trig_a_src <= sys_wdata[2:0] ;

   trig_b_sw  <= sys_wen && (sys_addr[19:0]==20'h0) && sys_wdata[16]  ;
   if (sys_wen && (sys_addr[19:0]==20'h0))
      trig_b_src <= sys_wdata[19:16] ;

   if (sys_wen) begin
      if (sys_addr[19:0]==20'h0)   {set_a_rgate, set_a_zero, set_a_rst, set_a_once, set_a_wrap} <= sys_wdata[ 8: 4] ;
      if (sys_addr[19:0]==20'h0)   {set_b_rgate, set_b_zero, set_b_rst, set_b_once, set_b_wrap} <= sys_wdata[24:20] ;

      if (sys_addr[19:0]==20'h4)   set_a_amp_0  <= sys_wdata[  0+13: 0] ;
      if (sys_addr[19:0]==20'h4)   set_a_dc_0   <= sys_wdata[ 16+13:16] ;
      if (sys_addr[19:0]==20'h8)   set_a_size_0 <= sys_wdata[RSZ+15: 0] ;
      if (sys_addr[19:0]==20'hC)   set_a_ofs_0  <= sys_wdata[RSZ+15: 0] ;
      if (sys_addr[19:0]==20'h10)  set_a_step_0 <= sys_wdata[RSZ+15: 0] ;
      if (sys_addr[19:0]==20'h18)  set_a_ncyc_0 <= sys_wdata[  16-1: 0] ;
      if (sys_addr[19:0]==20'h1C)  set_a_rnum_0 <= sys_wdata[  16-1: 0] ;
      if (sys_addr[19:0]==20'h20)  set_a_rdly_0 <= sys_wdata[  32-1: 0] ;

      if (sys_addr[19:0]==20'h24)  set_b_amp_0  <= sys_wdata[  0+13: 0] ;
      if (sys_addr[19:0]==20'h24)  set_b_dc_0   <= sys_wdata[ 16+13:16] ;
      if (sys_addr[19:0]==20'h28)  set_b_size_0 <= sys_wdata[RSZ+15: 0] ;
      if (sys_addr[19:0]==20'h2C)  set_b_ofs_0  <= sys_wdata[RSZ+15: 0] ;
      if (sys_addr[19:0]==20'h30)  set_b_step_0 <= sys_wdata[RSZ+15: 0] ;
      if (sys_addr[19:0]==20'h38)  set_b_ncyc_0 <= sys_wdata[  16-1: 0] ;
      if (sys_addr[19:0]==20'h3C)  set_b_rnum_0 <= sys_wdata[  16-1: 0] ;
      if (sys_addr[19:0]==20'h40)  set_b_rdly_0 <= sys_wdata[  32-1: 0] ;

      if (sys_addr[19:0]==20'h44)  {trig_evt_ab, trig_evt} <= sys_wdata[4-1:0] ;

      if (sys_addr[19:0]==20'h54)  set_a_amp_1  <= sys_wdata[  0+13: 0] ;
      if (sys_addr[19:0]==20'h54)  set_a_dc_1   <= sys_wdata[ 16+13:16] ;
      if (sys_addr[19:0]==20'h58)  set_a_size_1 <= sys_wdata[RSZ+15: 0] ;
      if (sys_addr[19:0]==20'h5C)  set_a_ofs_1  <= sys_wdata[RSZ+15: 0] ;
      if (sys_addr[19:0]==20'h60)  set_a_step_1 <= sys_wdata[RSZ+15: 0] ;
      if (sys_addr[19:0]==20'h68)  set_a_ncyc_1 <= sys_wdata[  16-1: 0] ;
      if (sys_addr[19:0]==20'h6C)  set_a_rnum_1 <= sys_wdata[  16-1: 0] ;
      if (sys_addr[19:0]==20'h70)  set_a_rdly_1 <= sys_wdata[  32-1: 0] ;

      if (sys_addr[19:0]==20'h74)  set_b_amp_1  <= sys_wdata[  0+13: 0] ;
      if (sys_addr[19:0]==20'h74)  set_b_dc_1   <= sys_wdata[ 16+13:16] ;
      if (sys_addr[19:0]==20'h78)  set_b_size_1 <= sys_wdata[RSZ+15: 0] ;
      if (sys_addr[19:0]==20'h7C)  set_b_ofs_1  <= sys_wdata[RSZ+15: 0] ;
      if (sys_addr[19:0]==20'h80)  set_b_step_1 <= sys_wdata[RSZ+15: 0] ;
      if (sys_addr[19:0]==20'h88)  set_b_ncyc_1 <= sys_wdata[  16-1: 0] ;
      if (sys_addr[19:0]==20'h8C)  set_b_rnum_1 <= sys_wdata[  16-1: 0] ;
      if (sys_addr[19:0]==20'h90)  set_b_rdly_1 <= sys_wdata[  32-1: 0] ;

      // Extra registers to run double buffered waveform
   end

   if (sys_ren) begin
      buf_a_rpnt_rd <= {{32-RSZ-2{1'b0}},buf_a_rpnt,2'h0};
      buf_b_rpnt_rd <= {{32-RSZ-2{1'b0}},buf_b_rpnt,2'h0};
   end

   ren_dly <= {ren_dly[3-2:0], sys_ren};
   ack_dly <=  ren_dly[3-1] || sys_wen ;
end

wire [32-1: 0] r0_rd = {7'h0,set_b_rgate, set_b_zero,set_b_rst,set_b_once,set_b_wrap, 1'b0,trig_b_src,
                        7'h0,set_a_rgate, set_a_zero,set_a_rst,set_a_once,set_a_wrap, 1'b0,trig_a_src };

wire sys_en;
assign sys_en = sys_wen | sys_ren;

always @(posedge dac_clk_i)
if (dac_rstn_i == 1'b0) begin
   sys_err <= 1'b0 ;
   sys_ack <= 1'b0 ;
end else begin
   sys_err <= 1'b0 ;

   // ##### NOTE: The registers for the second buffer have not been added to the read path
   casez (sys_addr[19:0])
     20'h00000 : begin sys_ack <= sys_en;          sys_rdata <= r0_rd                              ; end

     20'h00004 : begin sys_ack <= sys_en;          sys_rdata <= {2'h0, set_a_dc_0, 2'h0, set_a_amp_0}  ; end
     20'h00008 : begin sys_ack <= sys_en;          sys_rdata <= {{32-RSZ-16{1'b0}},set_a_size_0}     ; end
     20'h0000C : begin sys_ack <= sys_en;          sys_rdata <= {{32-RSZ-16{1'b0}},set_a_ofs_0}      ; end
     20'h00010 : begin sys_ack <= sys_en;          sys_rdata <= {{32-RSZ-16{1'b0}},set_a_step_0}     ; end
     20'h00014 : begin sys_ack <= sys_en;          sys_rdata <= buf_a_rpnt_rd                      ; end
     20'h00018 : begin sys_ack <= sys_en;          sys_rdata <= {{32-16{1'b0}},set_a_ncyc_0}         ; end
     20'h0001C : begin sys_ack <= sys_en;          sys_rdata <= {{32-16{1'b0}},set_a_rnum_0}         ; end
     20'h00020 : begin sys_ack <= sys_en;          sys_rdata <= set_a_rdly_0                         ; end

     20'h00024 : begin sys_ack <= sys_en;          sys_rdata <= {2'h0, set_b_dc_0, 2'h0, set_b_amp_0}  ; end
     20'h00028 : begin sys_ack <= sys_en;          sys_rdata <= {{32-RSZ-16{1'b0}},set_b_size_0}     ; end
     20'h0002C : begin sys_ack <= sys_en;          sys_rdata <= {{32-RSZ-16{1'b0}},set_b_ofs_0}      ; end
     20'h00030 : begin sys_ack <= sys_en;          sys_rdata <= {{32-RSZ-16{1'b0}},set_b_step_0}     ; end
     20'h00034 : begin sys_ack <= sys_en;          sys_rdata <= buf_b_rpnt_rd                      ; end
     20'h00038 : begin sys_ack <= sys_en;          sys_rdata <= {{32-16{1'b0}},set_b_ncyc_0}         ; end
     20'h0003C : begin sys_ack <= sys_en;          sys_rdata <= {{32-16{1'b0}},set_b_rnum_0}         ; end
     20'h00040 : begin sys_ack <= sys_en;          sys_rdata <= set_b_rdly_0                         ; end

     20'h00044 : begin sys_ack <= sys_en;          sys_rdata <= {{32-4{1'b0}},trig_evt_ab,trig_evt}; end

     20'h00054 : begin sys_ack <= sys_en;          sys_rdata <= {2'h0, set_a_dc_1, 2'h0, set_a_amp_1}  ; end
     20'h00058 : begin sys_ack <= sys_en;          sys_rdata <= {{32-RSZ-16{1'b0}},set_a_size_1}     ; end
     20'h0005C : begin sys_ack <= sys_en;          sys_rdata <= {{32-RSZ-16{1'b0}},set_a_ofs_1}      ; end
     20'h00060 : begin sys_ack <= sys_en;          sys_rdata <= {{32-RSZ-16{1'b0}},set_a_step_1}     ; end
     20'h00064 : begin sys_ack <= sys_en;          sys_rdata <= buf_a_rpnt_rd                      ; end
     20'h00068 : begin sys_ack <= sys_en;          sys_rdata <= {{32-16{1'b0}},set_a_ncyc_1}         ; end
     20'h0006C : begin sys_ack <= sys_en;          sys_rdata <= {{32-16{1'b0}},set_a_rnum_1}         ; end
     20'h00070 : begin sys_ack <= sys_en;          sys_rdata <= set_a_rdly_1                         ; end

     20'h00074 : begin sys_ack <= sys_en;          sys_rdata <= {2'h0, set_b_dc_1, 2'h0, set_b_amp_1}  ; end
     20'h00078 : begin sys_ack <= sys_en;          sys_rdata <= {{32-RSZ-16{1'b0}},set_b_size_1}     ; end
     20'h0007C : begin sys_ack <= sys_en;          sys_rdata <= {{32-RSZ-16{1'b0}},set_b_ofs_1}      ; end
     20'h00080 : begin sys_ack <= sys_en;          sys_rdata <= {{32-RSZ-16{1'b0}},set_b_step_1}     ; end
     20'h00084 : begin sys_ack <= sys_en;          sys_rdata <= buf_b_rpnt_rd                      ; end
     20'h00088 : begin sys_ack <= sys_en;          sys_rdata <= {{32-16{1'b0}},set_b_ncyc_1}         ; end
     20'h0008C : begin sys_ack <= sys_en;          sys_rdata <= {{32-16{1'b0}},set_b_rnum_1}         ; end
     20'h00090 : begin sys_ack <= sys_en;          sys_rdata <= set_b_rdly_1                         ; end
     // Debug registers
     // The decode below needs to change every time RSZ changes
     20'h4zzzz : begin sys_ack <= ack_dly;         sys_rdata <= {{32-14{1'b0}},buf_a_rdata}        ; end
     20'h5zzzz : begin sys_ack <= ack_dly;         sys_rdata <= {{32-14{1'b0}},buf_a_rdata}        ; end
     20'h6zzzz : begin sys_ack <= ack_dly;         sys_rdata <= {{32-14{1'b0}},buf_a_rdata}        ; end
     20'h7zzzz : begin sys_ack <= ack_dly;         sys_rdata <= {{32-14{1'b0}},buf_a_rdata}        ; end
     20'h8zzzz : begin sys_ack <= ack_dly;         sys_rdata <= {{32-14{1'b0}},buf_b_rdata}        ; end
     20'h9zzzz : begin sys_ack <= ack_dly;         sys_rdata <= {{32-14{1'b0}},buf_b_rdata}        ; end
     20'hazzzz : begin sys_ack <= ack_dly;         sys_rdata <= {{32-14{1'b0}},buf_b_rdata}        ; end
     20'hbzzzz : begin sys_ack <= ack_dly;         sys_rdata <= {{32-14{1'b0}},buf_b_rdata}        ; end

       default : begin sys_ack <= sys_en;          sys_rdata <=  32'h0                             ; end
   endcase
end

endmodule
