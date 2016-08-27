VERILOG_COMPILE=xvlog -sv
SIM_ELAB=xelab -debug all

PATH_LIB=/opt/Xilinx/Vivado/2015.4/data/verilog/src
PATH_SIP=/opt/Xilinx/Vivado/2015.4/data/secureip
PATH_RTL=../rtl
PATH_TBN=../tbn

# secure IP
LIB=$(PATH_LIB)/unisims/DNA_PORT.v \
    $(PATH_LIB)/unisims/XADC.v \
    $(PATH_LIB)/unisims/IBUFDS.v \
    $(PATH_LIB)/unisims/IOBUF.v \
    $(PATH_LIB)/unisims/BUFG.v \
    $(PATH_LIB)/unisims/PLLE2_ADV.v \
    $(PATH_LIB)/unisims/ODDR.v \
    $(PATH_LIB)/unisims/OBUFDS.v \
    $(PATH_LIB)/unisims/OSERDESE2.v \
    $(PATH_LIB)/retarget/IBUFGDS.v \
    $(PATH_LIB)/unisims/BUFIO.v \
    $(PATH_LIB)/unisims/BUFR.v \
    $(PATH_LIB)/unisims/ISERDESE2.v \
    $(PATH_LIB)/glbl.v

SIP=$(PATH_SIP)/oserdese2/oserdese2_002.vp \
    $(PATH_SIP)/iserdese2/iserdese2_002.vp

# RTL files
RTL=$(wildcard $(PATH_RTL)/*.sv)
RTL+=$(wildcard $(PATH_RTL)/*.v)

# testbench files
TBN=$(wildcard $(PATH_TBN)/*.sv)

# targets
#TGT=$(RTL:.sv=)
#TGT=$(RTL:.v=)
TGT=red_pitaya_asg_double_buf_tb

.PHONY: compile 

all: $(TGT)

compile: $(LIB) $(SIP) $(RTL) $(TBN)
	$(VERILOG_COMPILE) $(LIB) $(SIP) $(RTL) $(TBN)

%_tb: compile
	$(SIM_ELAB) -s tb_sim work.red_pitaya_asg_double_buf_tb

run_sim: $(TGT)
	xsim tb_sim -gui -wdb asg.wdb
clean:
	rm -rf work *.log *.jou *.pb xsim* *.wdb *.str *.vcd
