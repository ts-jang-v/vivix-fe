VPATH= ./src
OBJS_DIR=./objs
LIBS_DIR=../logd/libs
INC_DIR = ./include
TARGET=$(LIBS_DIR)/libNetwork.a

include $(MAK_DIR)/target.mak

CCFLAGS += -W \
	   -Wall \
	   -O2 \
	   -fno-strict-aliasing \
	   -D_GNU_SOURCE \
	   -D_REENTRANT \


OBJS = 	   $(OBJS_DIR)/NetworkUDP.o \
   	$(OBJS_DIR)/NetworkMISC.o \
	$(OBJS_DIR)/NetworkTCPServer.o \
	$(OBJS_DIR)/ClientInfo.o 
#	$(OBJS_DIR)/NetworkTCPClient.o \

all: $(TARGET) 

$(TARGET): $(OBJS)
	@echo
	@echo make archive: $@
	$(AR) rsvc $@ $(OBJS)

$(OBJS_DIR)/%.o: %.cpp
	@echo
	@echo Compiling: $<
	$(CC) $(CCFLAGS) -I$(INC_DIR) -g -c -o $@ $^


clean:
	@echo
	@echo Removing: $(OBJS)
	@rm -f $(OBJS)
	@echo
	@echo Removing: $(TARGET)
	@rm -f $(TARGET)
	@echo

