INC_DIR = include
OBJ_DIR = obj

include $(MAK_DIR)/target.mak

DEBUG_FLAGS += \

AR_FLAGS +=

CC_FLAGS += \
	-I$(INC_DIR) -I$(SYS_INC_DIR)\
	-DNANO_SECOND_SLEEP \
	$(DEBUG_FLAGS)

LD_FLAGS +=

SYS_LIBS = \
	-lrt \


TARGET = $(LIB_DIR)/os_lib.a

OBJS = \
	$(OBJ_DIR)/vw_time.o\
	$(OBJ_DIR)/mutex.o\
	$(OBJ_DIR)/event.o\
	$(OBJ_DIR)/message.o\


all: $(TARGET)
	@echo

$(TARGET): $(OBJS)
	@echo
	@echo Archiving: $@
	@$(AR) $(AR_FLAGS) $@ $(OBJS)  $(LIBS) 

$(OBJ_DIR)/%.o: src/%.cpp
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
