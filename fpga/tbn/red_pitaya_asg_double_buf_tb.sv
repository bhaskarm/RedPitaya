/**
 * $Id: red_pitaya_asg_tb.v 1271 2014-02-25 12:32:34Z matej.oblak $
 *
 * @brief Red Pitaya arbitrary signal generator testbench.
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
 * Testbench for arbitrary signal generator module.
 *
 * This testbench writes values into RAM table and sets desired configuration.
 * 
 */

`timescale 1ns / 1ps

`define CH_OFFSET_FIRST 32
`define CH_OFFSET 96
`define BUF_OFFSET 262144
module red_pitaya_asg_double_buf_tb #(
  // time period
  realtime  TP = 8.0ns,  // 125MHz
  // DUT configuration
  int unsigned DAC_DW = 14, // ADC data width
  int unsigned RSZ = 15  // RAM size is 2**RSZ
);

////////////////////////////////////////////////////////////////////////////////
// DAC signal generation
////////////////////////////////////////////////////////////////////////////////


logic               clk ;
logic               rstn;

logic [DAC_DW-1: 0] dac_a;
logic [DAC_DW-1: 0] dac_b;

logic               trig;

// DAC clock
initial        clk = 1'b0;
always #(TP/2) clk = ~clk;

// DAC reset
initial begin
  rstn = 1'b0;
  repeat(4) @(posedge clk);
  rstn = 1'b1;
end

// ADC cycle counter
int unsigned dac_cyc=0;
always_ff @ (posedge clk)
dac_cyc <= dac_cyc+1;

always begin
  trig <= 1'b0 ;
  repeat(100000) @(posedge clk);
  trig <= 1'b1 ;
  repeat(1200) @(posedge clk);
  trig <= 1'b0 ;
end

////////////////////////////////////////////////////////////////////////////////
// test sequence
////////////////////////////////////////////////////////////////////////////////

logic [ 32-1: 0] sys_addr ;
logic [ 32-1: 0] sys_wdata;
logic [  4-1: 0] sys_sel  ;
logic            sys_wen  ;
logic            sys_ren  ;
logic [ 32-1: 0] sys_rdata;
logic            sys_err  ;
logic            sys_ack  ;

logic        [ 32-1: 0] rdata;
logic signed [ 32-1: 0] rdata_blk [];

reg          [ 32-1: 0] rdata_cha;
reg          [ 32-1: 0] rdata_chb;
//---------------------------------------------------------------------------------
//
// signal generation

reg [9-1: 0] ch0_set;
reg [9-1: 0] ch1_set;

initial begin
  repeat(10) @(posedge clk);

  // CH0 DAC settings
  bus.write(`CH_OFFSET_FIRST+32'h00004,{1'h0,-14'd500, 2'h0, 14'h2F00}  );  // DC offset, amplitude
  bus.write(`CH_OFFSET_FIRST+32'h00008,{16'h3ffc, 16'h0}          );  // table size
  bus.write(`CH_OFFSET_FIRST+32'h0000C,{16'h0, 16'h0}             );  // reset offset
  bus.write(`CH_OFFSET_FIRST+32'h00010,{16'h34, 16'h0}             );  // table step
  bus.write(`CH_OFFSET_FIRST+32'h00018,{16'h0, 16'd1}                   );  // number of cycles
  bus.write(`CH_OFFSET_FIRST+32'h0001C,{16'h0, 16'd0}                   );  // number of repetitions
  bus.write(`CH_OFFSET_FIRST+32'h00020,{32'd0}                          );  // number of 1us delay between repetitions

  bus.write(`CH_OFFSET+32'h00054,{2'h0,-14'd500, 2'h0, 14'h2F00}  );  // DC offset, amplitude
  bus.write(`CH_OFFSET+32'h00058,{16'h7ffc, 16'h0}          );  // table size
  bus.write(`CH_OFFSET+32'h0005C,{16'h4000, 16'h0}             );  // reset offset
  bus.write(`CH_OFFSET+32'h00060,{16'h34, 16'h0}             );  // table step
  bus.write(`CH_OFFSET+32'h00068,{16'h0, 16'd2}                   );  // number of cycles
  bus.write(`CH_OFFSET+32'h0006C,{16'h0, 16'd0}                   );  // number of repetitions
  bus.write(`CH_OFFSET+32'h00070,{32'd0}                          );  // number of 1us delay between repetitions

  bus.write(`CH_OFFSET+32'h00074,{2'h0,-14'd500, 2'h0, 14'h2F00}  );  // DC offset, amplitude
  bus.write(`CH_OFFSET+32'h00078,{16'hbffc, 16'h0}          );  // table size
  bus.write(`CH_OFFSET+32'h0007C,{16'h8000, 16'h0}             );  // reset offset
  bus.write(`CH_OFFSET+32'h00080,{16'h34, 16'h0}             );  // table step
  bus.write(`CH_OFFSET+32'h00088,{16'h0, 16'd3}                   );  // number of cycles
  bus.write(`CH_OFFSET+32'h0008C,{16'h0, 16'd0}                   );  // number of repetitions
  bus.write(`CH_OFFSET+32'h00090,{32'd0}                          );  // number of 1us delay between repetitions
  
  bus.write(`CH_OFFSET+32'h00094,{2'h0,-14'd500, 2'h0, 14'h2F00}  );  // DC offset, amplitude
  bus.write(`CH_OFFSET+32'h00098,{16'hfffc, 16'h0}          );  // table size
  bus.write(`CH_OFFSET+32'h0009C,{16'hc000, 16'h0}             );  // reset offset
  bus.write(`CH_OFFSET+32'h000A0,{16'h34, 16'h0}             );  // table step
  bus.write(`CH_OFFSET+32'h000A8,{16'h0, 16'd4}                   );  // number of cycles
  bus.write(`CH_OFFSET+32'h000AC,{16'h0, 16'd0}                   );  // number of repetitions
  bus.write(`CH_OFFSET+32'h000B0,{32'd0}                          );  // number of 1us delay between repetitions
  
  bus.write(32'h00044,{32'd2}                          );  // trigger output conditions

  // CH1 DAC data
  for (int k=0; k<65536/16; k++) begin
    bus.write(`BUF_OFFSET+32'h40000 + (k*4), k+3);  // write table
  end
  for (int k=0; k<40; k++) begin
    bus.write(`BUF_OFFSET+32'h50000 + (k*4), k+3);  // write table
  end
  for (int k=0; k<40; k++) begin
    bus.write(`BUF_OFFSET+32'h60000 + (k*4), k+3);  // write table
  end
  for (int k=0; k<40; k++) begin
    bus.write(`BUF_OFFSET+32'h70000 + (k*4), k+3);  // write table
  end

  ch0_set = {1'b0 ,1'b0, 1'b0, 1'b0, 1'b0,    1'b0, 3'h1} ;  // set_rgate, set_zero, set_rst, set_once(NA), set_wrap, 1'b0, trig_src

  ch1_set = {1'b0, 1'b0, 1'b0, 1'b0, 1'b0,    1'b0, 3'h1} ;  // set_rgate, set_zero, set_rst, set_once(NA), set_wrap, 1'b0, trig_src


  bus.write(32'h00000,{8'h0, ch1_set,  8'h0, ch0_set}  ); // write configuration
  bus.write(32'h00000,{8'h0, ch1_set,  8'h0, ch0_set}  ); // write configuration

  repeat(2000) @(posedge clk);

  repeat(2000) @(posedge clk);

  repeat(200) @(posedge clk);

  // CH1 table data readback
  rdata_blk = new [80];
  for (int k=0; k<80; k++) begin
    bus.read(32'h20000 + (k*4), rdata_blk [k]);  // read table
  end

  // CH1 table data readback
  for (int k=0; k<20; k++) begin
    bus.read(32'h00014, rdata);  // read read pointer
    bus.read(32'h00034, rdata);  // read read pointer
    repeat(1737) @(posedge clk);
  end

  repeat(20000) @(posedge clk);

  $finish();
end

////////////////////////////////////////////////////////////////////////////////
// module instances
////////////////////////////////////////////////////////////////////////////////

sys_bus_model bus (
  // system signals
  .clk          (clk      ),
  .rstn         (rstn     ),
  // bus protocol signals
  .sys_addr     (sys_addr ),
  .sys_wdata    (sys_wdata),
  .sys_sel      (sys_sel  ),
  .sys_wen      (sys_wen  ),
  .sys_ren      (sys_ren  ),
  .sys_rdata    (sys_rdata),
  .sys_err      (sys_err  ),
  .sys_ack      (sys_ack  ) 
);

red_pitaya_asg_double_buf asg (
  // DAC
  .dac_clk_i      (clk      ),
  .dac_rstn_i     (rstn     ),
  .dac_a_o        (dac_a    ),  // CH 1
  .dac_b_o        (dac_b    ),  // CH 2
  // trigger
  .trig_a_i       (trig     ),
  .trig_b_i       (trig     ),
  .trig_out_o     (         ),
  // System bus
  .sys_addr       (sys_addr ),
  .sys_wdata      (sys_wdata),
  .sys_sel        (sys_sel  ),
  .sys_wen        (sys_wen  ),
  .sys_ren        (sys_ren  ),
  .sys_rdata      (sys_rdata),
  .sys_err        (sys_err  ),
  .sys_ack        (sys_ack  )
);

////////////////////////////////////////////////////////////////////////////////
// waveforms
////////////////////////////////////////////////////////////////////////////////

initial begin
  $dumpfile("red_pitaya_asg_double_buf_tb.vcd");
  $dumpvars(0, red_pitaya_asg_double_buf_tb);
end

endmodule: red_pitaya_asg_double_buf_tb