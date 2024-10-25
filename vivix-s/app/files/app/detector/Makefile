INC_DIR = include
NETWOR_INC_DIR =../network/include
XML_INC_DIR =../xml/include
BF_INC_DIR = ../blowfish/include
OS_LIB_INC_DIR = ../os_lib/include
UTILS_INC_DIR = ../utils/include
OBJ_DIR = obj
SHA1_DIR = ../../web/cgi_src/thirdParty/sha_1
SHA1_LIB_DIR = ../../web/cgi_src/lib
DRV_INC_DIR = ../../driver/include

include $(MAK_DIR)/target.mak

DEBUG_FLAGS += \
		-D_DEBUG_TG_SERVICE\
		-D_DEBUG_CAPTURE\
		-DDEBUG_RESEND \
		-DTARGET_COM

AR_FLAGS +=

#CC_FLAGS += \
#	-I$(INC_DIR) -I$(NETWOR_INC_DIR) -I$(MXML_INC_DIR) -I$(MISC_INC_DIR) -I$(BF_INC_DIR)\
#	-I$(LOG_INC_DIR)  -I$(SHA1_DIR)/include $(DEBUG_FLAGS) -static

CC_FLAGS += \
	-I$(DRV_INC_DIR) -I$(INC_DIR) -I$(NETWOR_INC_DIR) -I$(XML_INC_DIR) -I$(BF_INC_DIR) -I$(OS_LIB_INC_DIR)\
	-I$(UTILS_INC_DIR) -I$(SYS_INC_DIR) -I$(SHA1_DIR)/include $(DEBUG_FLAGS) -D__cplus__ -static

LD_FLAGS += -Wl,-Map,vivix_detector.map

TARGET = $(BIN_DIR)/vivix_detector

OBJS = \
	$(OBJ_DIR)/vw_i2c.o \
	$(OBJ_DIR)/vw_spi.o \
	$(OBJ_DIR)/env.o \
	$(OBJ_DIR)/oled_image.o \
	$(OBJ_DIR)/aed.o \
	$(OBJ_DIR)/impact.o \
	$(OBJ_DIR)/hw_monitor.o \
	$(OBJ_DIR)/micom.o \
	$(OBJ_DIR)/fpga.o \
	$(OBJ_DIR)/fpga_prom.o \
	$(OBJ_DIR)/net_manager.o \
	$(OBJ_DIR)/vw_system.o \
	$(OBJ_DIR)/image.o \
	$(OBJ_DIR)/capture.o \
	$(OBJ_DIR)/gige_server.o \
	$(OBJ_DIR)/command.o \
	$(OBJ_DIR)/control_server.o \
	$(OBJ_DIR)/scu_server.o \
	$(OBJ_DIR)/calibration.o \
	$(OBJ_DIR)/diagnosis.o \
	$(OBJ_DIR)/web.o \
	$(OBJ_DIR)/image_process.o \
	$(OBJ_DIR)/image_retransfer.o \
	$(OBJ_DIR)/main.o \

LIBS = \
	$(LIB_DIR)/os_lib.a \
	$(LIB_DIR)/network.a \
	$(LIB_DIR)/vw_xml.a \
	$(LIB_DIR)/utils.a \
	$(SHA1_LIB_DIR)/libsha1.a \
	#$(LIB_DIR)/bf.a \

SYS_LIBS = \
	-lpthread -lm -lrt


all: $(TARGET)
	@echo

$(TARGET): $(OBJS) $(LIBS)
	@echo
	@echo Linking: $@
#	@$(LD) $(LD_FLAGS) -o $@ -Xlinker --start-group $(OBJS) $(LIBS) $(SYS_LIBS) --end-group
	@$(CXX) $(LD_FLAGS) -o $@ -Xlinker --start-group $(OBJS) $(LIBS) $(SYS_LIBS)


$(OBJ_DIR)/%.o: src/%.cpp
	@echo
	@echo Compiling: $<
	@$(CXX) $(CC_FLAGS) -o $@ $<

clean:
	@echo
	@echo Removing: $(OBJS)
	@rm -f $(OBJS)
	@echo
	@echo Removing: $(TARGET)
	@rm -f $(TARGET)
	@echo
