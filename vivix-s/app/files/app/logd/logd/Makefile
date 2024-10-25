VPATH= ./src
INC_DIR =-I./include \
	-I../network/include 
LIBS_DIR= ./libs
OBJS_DIR=./objs

include $(MAK_DIR)/target.mak

CCFLAGS += -W \
	   -Wall \
	   -O2 \
	   -fno-strict-aliasing \
	   -D_GNU_SOURCE \
	   -D_REENTRANT \


OBJS = $(OBJS_DIR)/log_conf.o \
	   $(OBJS_DIR)/logd.o \
	   $(OBJS_DIR)/packet_format.o \
	   $(OBJS_DIR)/main.o 

LIBS = -L$(LIBS_DIR) -lNetwork -lpthread
	   	 


TARGET = $(BIN_DIR)/logd

all: $(TARGET)
	@echo

$(TARGET): $(OBJS)
	@echo
	@echo Linking: $@
#	$(LD) $(LDFLAGS) -o $@ -Xlinker --start-group $(LIBS) $(OBJS) --end-group
	$(CXX) $(LDFLAGS) -o $@ -Xlinker --start-group $(LIBS) $(OBJS) 


$(OBJS_DIR)/%.o: %.cpp
	@echo
	@echo Compiling: $<
	$(CXX) $(CCFLAGS) $(INC_DIR) -g -c -o $@ $< 

clean:
	@echo
	@echo Removing: $(OBJS)
	@rm -f $(OBJS)
	@echo
	@echo Removing: $(TARGET)
	@rm -f $(TARGET)
	@echo

