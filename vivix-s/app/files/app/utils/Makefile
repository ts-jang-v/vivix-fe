INC_DIR = include
OBJ_DIR = obj

include $(MAK_DIR)/target.mak

DEBUG_FLAGS += \

AR_FLAGS +=

CC_FLAGS += \
	-I$(INC_DIR) -I$(SYS_INC_DIR) \
	-DNANO_SECOND_SLEEP \
	$(DEBUG_FLAGS)

LD_FLAGS +=

SYS_LIBS = \
	-lrt \


TARGET = $(LIB_DIR)/utils.a

OBJS = \
	$(OBJ_DIR)/vw_file.o\
	$(OBJ_DIR)/blowfish.o\
	$(OBJ_DIR)/misc.o\
	$(OBJ_DIR)/vw_download.o\
	$(OBJ_DIR)/vw_net.o\


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
