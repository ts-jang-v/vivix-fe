INC_DIR = include
OS_LIB_INC_DIR =../os_lib/include
OBJ_DIR = obj

include $(MAK_DIR)/target.mak

DEBUG_FLAGS += \

AR_FLAGS +=

CC_FLAGS += \
	-I$(INC_DIR) -I$(OS_LIB_INC_DIR) -I$(SYS_INC_DIR) \
	-DNANO_SECOND_SLEEP \
	$(DEBUG_FLAGS)

LD_FLAGS +=

SYS_LIBS = \
	#-lpthread \
	#-lrt \


TARGET = $(LIB_DIR)/network.a

OBJS = \
	$(OBJ_DIR)/network.o\
	$(OBJ_DIR)/tcp_client.o\
	$(OBJ_DIR)/tcp_server.o\
	$(OBJ_DIR)/udp_server.o\
	$(OBJ_DIR)/gvcp.o\

all: $(TARGET)
	@echo

$(TARGET): $(OBJS)
	@echo
	@echo Archiving: $@
	@$(AR) $(AR_FLAGS) $@ $(OBJS)  $(LIBS) 

$(OBJ_DIR)/%.o: src/%.c*
	@echo
	@echo Compiling: $<
	@$(CXX) $(CC_FLAGS) -o $@ $< $(SYS_LIBS)

clean:
	@echo
	@echo Removing: $(OBJS)
	@rm -f $(OBJS)
	@echo
	@echo Removing: $(TARGET)
	@rm -f $(TARGET)
	@echo
