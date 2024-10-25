INC_DIR = include
OBJ_DIR = obj

include $(MAK_DIR)/target.mak

DEBUG_FLAGS += \

AR_FLAGS +=

CC_FLAGS += \
	-I$(INC_DIR) -I$(SYS_INC_DIR) -D__cplus__\
	$(DEBUG_FLAGS)

LD_FLAGS +=

TARGET = $(LIB_DIR)/vw_xml.a

OBJS = \
	$(OBJ_DIR)/mxml_attribute.o \
	$(OBJ_DIR)/mxml_document.o \
	$(OBJ_DIR)/mxml_error.o \
	$(OBJ_DIR)/mxml_file.o \
	$(OBJ_DIR)/mxml_index.o \
	$(OBJ_DIR)/mxml_iterator.o \
	$(OBJ_DIR)/mxml_node.o \
	$(OBJ_DIR)/mxml_output.o \
	$(OBJ_DIR)/mxml_path.o \
	$(OBJ_DIR)/mxml_refil.o \
	$(OBJ_DIR)/mxml_sgs.o \
	$(OBJ_DIR)/vw_xml.o \
	$(OBJ_DIR)/vw_xml_util.o \

all: $(TARGET)
	@echo

$(TARGET): $(OBJS)
	@echo
	@echo Archiving: $@
	@$(AR) $(AR_FLAGS) $@ $(OBJS) $(LIBS)

$(OBJ_DIR)/%.o: src/%.c*
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
