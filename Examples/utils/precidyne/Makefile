##
# $Id: Makefile 1249 2014-02-22 20:21:40Z ales.bardorfer $
#
# (c) Red Pitaya  http://www.redpitaya.com
#
# Signal generator project file. To build executable for Signal generator run:
# 'make all'
#
# This project file is written for GNU/Make software. For more details please 
# visit: http://www.gnu.org/software/make/manual/make.html
# GNU Compiler Collection (GCC) tools are used for the compilation and linkage. 
# For the details about the usage and building please visit:
# http://gcc.gnu.org/onlinedocs/gcc/
#

# Versioning system
VERSION ?= 0.00-0000
REVISION ?= devbuild

TARGET=precidyne_utils

# Installation directory
INSTALL_DIR ?= .

# Makefile is composed of so called 'targets'. They give basic structure what 
# needs to be execued during various stages of the building/removing/installing
# of software package.
# Simple Makefile targets have the following structure:
# <name>: <dependencies>
#	<command1>
#       <command2>
#       ...
# The target <name> is completed in the following order:
#   - list od <dependencies> finished
#   - all <commands> in the body of targets are executed succsesfully

# Main Makefile target 'all' - it iterates over all targets listed in $(TARGET)
# variable.
all: install

# Install target - creates 'bin/' sub-directory in $(INSTALL_DIR) and copies all
# executables to that location.
install:
	mkdir -p $(INSTALL_DIR)/bin
	cp -r *.sh $(INSTALL_DIR)/bin
	mkdir -p $(INSTALL_DIR)/src/utils/$(TARGET)
	-rm -f $(TARGET) *.o
	cp -r *.sh $(INSTALL_DIR)/src/utils/$(TARGET)/
