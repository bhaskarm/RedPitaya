/**
 * $Id: red_pitaya_asg_ch.v 1271 2014-02-25 12:32:34Z matej.oblak $
 *
 * @brief Red Pitaya ASG submodule. Holds table and FSM for one channel.
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
 *   SW --------> | BUF | ------> | kx + o | ---> DAC DAT
 *          |     \-----/         \--------/
 *          |        ^
 *          |        |
 *          |     /-----\
 *          ----> |     |
 *                | FSM | ------> trigger notification
 *   trigger ---> |     |
 *                \-----/
 *
 *
 * Submodule for ASG which hold buffer data and control registers for one channel.
 * 
 */

module red_pitaya_asg_ch_double_buf #(
   parameter RSZ = 16,
   parameter N_BUF = 4
)(
   // DAC
   output reg [ 14-1: 0] dac_o           ,  //!< dac data output
   input                 dac_clk_i       ,  //!< dac clock
   input                 dac_rstn_i      ,  //!< dac reset - active low
   // trigger
   input                 trig_sw_i       ,  //!< software trigger
   input                 trig_ext_i      ,  //!< external trigger
   input      [  3-1: 0] trig_src_i      ,  //!< trigger source selector
   input      [  3-1: 0] trig_evt_i      ,  //!< trigger event condition (0: start of waveform, 1: counter wrap, 2-7: reserved)
   output                buf_done_o      ,  //!< trigger event
   output                cyc_done_o      ,  //!< trigger event
   // buffer ctrl
   input                 buf_we_i        ,  //!< buffer write enable
   input      [RSZ-1: 0] buf_addr_i      ,  //!< buffer address
   input      [ 14-1: 0] buf_wdata_i     ,  //!< buffer write data
   output reg [ 14-1: 0] buf_rdata_o     ,  //!< buffer read data
   output reg [RSZ-1: 0] buf_rpnt_o      ,  //!< buffer current read pointer
   // configuration
   input     [  (14*N_BUF)-1: 0] set_amp_all_i     ,  //!< set amplitude scale
   input     [  (14*N_BUF)-1: 0] set_dc_all_i      ,  //!< set output offset
   input     [((RSZ+16)*N_BUF)-1: 0] set_end_all_i    ,  //!< set table data end
   input     [((RSZ+16)*N_BUF)-1: 0] set_step_all_i    ,  //!< set pointer step
   input     [((RSZ+16)*N_BUF)-1: 0] set_start_all_i     ,  //!< set reset offset
   input     [  (16*N_BUF)-1: 0] set_ncyc_all_i    ,  //!< set number of cycle
   input     [  (16*N_BUF)-1: 0] set_rnum_all_i    ,  //!< set number of repetitions
   input     [  (32*N_BUF)-1: 0] set_rdly_all_i    ,  //!< set delay between repetitions
   input                 set_rst_i       ,  //!< set FSM to reset
   input                 set_zero_i      ,  //!< set output to zero
   // Debug signals
   output         [15-1: 0] debug_bus
);

//---------------------------------------------------------------------------------
//
//  DAC buffer RAM

reg   [  14-1: 0] dac_buf [0:(1<<RSZ)-1] ;
reg   [  14-1: 0] dac_rd    ;
reg   [  14-1: 0] dac_rdat  ;
reg   [ RSZ-1: 0] dac_rp    ;
reg   [RSZ+15: 0] dac_ptr   ; // read pointer
reg   [RSZ+15: 0] dac_ptrp  ; // previour read pointer
wire  [RSZ+16: 0] dac_npnt  ; // next read pointer
wire  [RSZ+16: 0] dac_npnt_sub ;
wire              dac_npnt_sub_neg;

reg   [  28-1: 0] dac_mult  ;
reg   [  15-1: 0] dac_sum   ;

wire     [  14-1: 0] set_amp_i     ;  //!< set amplitude scale
wire     [  14-1: 0] set_dc_i      ;  //!< set output offset
wire     [RSZ+15: 0] set_end_i    ;  //!< set table data end
wire     [RSZ+15: 0] set_step_i    ;  //!< set pointer step
wire     [RSZ+15: 0] set_start_i     ;  //!< set reset offset
wire     [  16-1: 0] set_ncyc_i    ;  //!< set number of cycle
wire     [  16-1: 0] set_rnum_i    ;  //!< set number of repetitions
wire     [  32-1: 0] set_rdly_i    ;  //!< set delay between repetitions
reg      [  16-1: 0] cyc_cnt      ;
reg                  trig_in_latch;
reg      [     1:0 ] current_buf  ;
reg                  cyc_done     ;
reg                  buf_done     ;
reg      [     2: 0] asg_state    ;
reg      [     2: 0] next_asg_state    ;
parameter SM_IDLE=0, SM_START_PTR=1, SM_DRIVE_DAC=2, SM_START_NEXT_BUF=3;
assign debug_bus =  { cyc_cnt[6:0], 
set_ncyc_i[4:0],
 current_buf,
 cyc_done_o,
 buf_done_o };

assign buf_done_o = buf_done;
assign cyc_done_o = cyc_done;
// read
always @(posedge dac_clk_i)
begin
   buf_rpnt_o <= dac_ptr[RSZ+15:16];
   dac_rp     <= dac_ptr[RSZ+15:16];
   dac_rd     <= dac_buf[dac_rp] ;
   dac_rdat   <= dac_rd ;  // improve timing
end

// write
always @(posedge dac_clk_i)
if (buf_we_i)  dac_buf[buf_addr_i] <= buf_wdata_i[14-1:0] ;

// read-back
always @(posedge dac_clk_i)
buf_rdata_o <= dac_buf[buf_addr_i] ;

// scale and offset
always @(posedge dac_clk_i)
begin
   dac_mult <= $signed(dac_rdat) * $signed({1'b0,set_amp_i}) ;
   dac_sum  <= $signed(dac_mult[28-1:13]) + $signed(set_dc_i) ;

   // saturation
   if (set_zero_i)  dac_o <= 14'h0;
   else             dac_o <= ^dac_sum[15-1:15-2] ? {dac_sum[15-1], {13{~dac_sum[15-1]}}} : dac_sum[13:0];
end

//---------------------------------------------------------------------------------
//
//  read pointer & state machine

reg              trig_in      ;
wire             ext_trig_p   ;
wire             ext_trig_n   ;

assign  set_amp_i      = set_amp_all_i   [ (14*(current_buf+1))-1 +: 14];
assign  set_dc_i       = set_dc_all_i    [ (14*(current_buf+1))-1 +: 14];
assign  set_end_i     = set_end_all_i  [ ((RSZ+16)*(current_buf+1))-1 +: (RSZ+16)];
assign  set_step_i     = set_step_all_i  [ ((RSZ+16)*(current_buf+1))-1 +: (RSZ+16)];
assign  set_start_i      = set_start_all_i   [ ((RSZ+16)*(current_buf+1))-1 +: (RSZ+16)];
assign  set_ncyc_i     = set_ncyc_all_i  [ (16*(current_buf+1))-1 +: 16];
assign  set_rnum_i     = set_rnum_all_i  [ (16*(current_buf+1))-1 +: 16];
assign  set_rdly_i     = set_rdly_all_i  [ (32*(current_buf+1))-1 +: 32];

always @(posedge dac_clk_i) begin
   // Latch the dac trigger for as long as the DAC is not reset
   if (dac_rstn_i == 1'b0 || set_rst_i == 1'b1)
      trig_in_latch <= 1'b0;
   else if (trig_in)
      trig_in_latch <= 1'b1;
      
    // trigger arrived
   if (dac_rstn_i == 1'b0 || set_rst_i == 1'b1) begin
        trig_in <= 1'b0;
   end else begin
     case (trig_src_i)
        3'd1 : trig_in <= trig_sw_i   ; // sw
        3'd2 : trig_in <= ext_trig_p  ; // external positive edge
        3'd3 : trig_in <= ext_trig_n  ; // external negative edge
     default : trig_in <= 1'b0        ;
    endcase
  end
end

// state machine Sequential
always @(posedge dac_clk_i) begin
   if (dac_rstn_i == 1'b0 || set_rst_i == 1'b1) begin
      asg_state <= SM_IDLE;
   end else begin
      asg_state <= next_asg_state;
   end

   if (dac_rstn_i == 1'b0 || set_rst_i == 1'b1) begin
      current_buf <= 2'b00;
      cyc_cnt <= 16'h0000;
      dac_ptr <= {RSZ+16{1'b0}};
   end else begin
     case(asg_state)
       SM_IDLE       : begin
           current_buf <= 2'b00;
           cyc_cnt <= 16'h0000;
           dac_ptr <= {RSZ+16{1'b0}};
       end
       SM_START_PTR  : begin
           current_buf <= current_buf;
           cyc_cnt <= set_ncyc_i;
           dac_ptr <= set_start_i;
       end
       SM_DRIVE_DAC  : begin
           if (dac_ptr + set_step_i >= set_end_i) begin
             dac_ptr = set_start_i;
             if (cyc_cnt == 16'h0001) begin
               current_buf <= current_buf + 1;
               cyc_cnt <= set_ncyc_i;
             end else begin
               current_buf <= current_buf;
               cyc_cnt <= cyc_cnt - 1;
             end
           end else begin
             dac_ptr = dac_ptr + set_step_i;
             current_buf = current_buf;
             cyc_cnt = cyc_cnt;
           end
       end
       SM_START_NEXT_BUF: begin
           current_buf = current_buf;
           cyc_cnt = set_ncyc_i;
           dac_ptr = set_start_i;
       end
       default : begin
           current_buf = 2'b00;
           cyc_cnt = 16'h0000;
           dac_ptr = {RSZ+16{1'b0}};
       end
     endcase
   end
end

// state machine comb
always @(*) begin
    next_asg_state = SM_IDLE;
    cyc_done = 1'b0;
    buf_done = 1'b0;
    case (asg_state)
        SM_IDLE : begin
            if (trig_in_latch == 1'b1) 
              next_asg_state = SM_START_PTR;
            cyc_done = 1'b0;
            buf_done = 1'b0;
        end
        SM_START_PTR : begin
            next_asg_state = SM_DRIVE_DAC;
            cyc_done = 1'b0;
            buf_done = 1'b0;
        end
        SM_DRIVE_DAC : begin
            if (dac_ptr + set_step_i >= set_end_i) begin
              if (cyc_cnt == 16'h0001) begin
                next_asg_state = SM_START_NEXT_BUF;
              end else begin
                next_asg_state = SM_DRIVE_DAC;
              end
                cyc_done = 1'b1;
                buf_done = 1'b0;
            end else begin
              next_asg_state = SM_DRIVE_DAC;
              cyc_done = 1'b0;
              buf_done = 1'b0;
            end

        end
        SM_START_NEXT_BUF: begin
            next_asg_state = SM_START_PTR;
            cyc_done = 1'b0;
            buf_done = 1'b1;
        end
        default : begin
            next_asg_state = SM_IDLE;
            cyc_done = 1'b0;
            buf_done = 1'b0;
        end
    endcase
end
            
//---------------------------------------------------------------------------------
//
//  External trigger

reg  [  3-1: 0] ext_trig_in    ;
reg  [  2-1: 0] ext_trig_dp    ;
reg  [  2-1: 0] ext_trig_dn    ;
reg  [ 20-1: 0] ext_trig_debp  ;
reg  [ 20-1: 0] ext_trig_debn  ;

always @(posedge dac_clk_i) begin
   if (dac_rstn_i == 1'b0) begin
      ext_trig_in   <=  3'h0 ;
      ext_trig_dp   <=  2'h0 ;
      ext_trig_dn   <=  2'h0 ;
      ext_trig_debp <= 20'h0 ;
      ext_trig_debn <= 20'h0 ;
   end
   else begin
      //----------- External trigger
      // synchronize FFs
      ext_trig_in <= {ext_trig_in[1:0],trig_ext_i} ;

      // look for input changes
      if ((ext_trig_debp == 20'h0) && (ext_trig_in[1] && !ext_trig_in[2]))
         ext_trig_debp <= 20'd62500 ; // ~0.5ms
      else if (ext_trig_debp != 20'h0)
         ext_trig_debp <= ext_trig_debp - 20'd1 ;

      if ((ext_trig_debn == 20'h0) && (!ext_trig_in[1] && ext_trig_in[2]))
         ext_trig_debn <= 20'd62500 ; // ~0.5ms
      else if (ext_trig_debn != 20'h0)
         ext_trig_debn <= ext_trig_debn - 20'd1 ;

      // update output values
      ext_trig_dp[1] <= ext_trig_dp[0] ;
      if (ext_trig_debp == 20'h0)
         ext_trig_dp[0] <= ext_trig_in[1] ;

      ext_trig_dn[1] <= ext_trig_dn[0] ;
      if (ext_trig_debn == 20'h0)
         ext_trig_dn[0] <= ext_trig_in[1] ;
   end
end

assign ext_trig_p = (ext_trig_dp == 2'b01) ;
assign ext_trig_n = (ext_trig_dn == 2'b10) ;

endmodule
