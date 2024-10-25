# target.mak for vivix 
# Author: Hong SeungKi <hongseungki@vieworks.co.kr>

#CROSS_COMPILE = arm-none-linux-gnueabi-
#CROSS_COMPILE = arm-linux-gnueabihf-
#PWD			= $(shell pwd)
#AR = $(CROSS_COMPILE)ar
#CC = $(CROSS_COMPILE)g++
#LD = $(CROSS_COMPILE)g++

TARGET_DEFS = \



TARGET_DEFS +=\

AR_FLAGS = \
	-raus elf32
CC_FLAGS = \
	-g \
	-c \
	-Wall \
	-O2 \
	-fno-strict-aliasing \
	-D_REENTRANT \
	-D_REUSE_CLIENT_MEMORY \
	-I$(PWD) \
	$(TARGET_DEFS) \

#	-DNDEBUG \

LD_FLAGS =\
#	-mfloat-abi=softfp  \

DEBUG_FLAGS = \
	-g \
	-static \
	-D_DEBUG \

