INC_DIR = include
OBJ_DIR = obj
WEB_LIB_DIR = ../../lib

CC_FLAGS = -I$(INC_DIR)

TARGET = $(WEB_LIB_DIR)/libsha1.a

OBJS = \
    $(OBJ_DIR)/sha1.o \

all: $(TARGET)
	@echo

$(TARGET): $(OBJS)
	@echo
	@echo Archiving: $@
	@$(AR) rc $@ $(OBJS)

$(OBJ_DIR)/%.o: src/%.c*
	@echo
	@[ -d $(OBJ_DIR) ] || mkdir -p $(OBJ_DIR)
	@echo Compiling: $< $@
	@$(CC) $(CC_FLAGS) -c $< 
	@mv sha1.o obj 

clean:
	@echo
	@echo Removing: $(OBJS)
	@rm -f $(OBJS)
	@echo
	@echo Removing: $(TARGET)
	@rm -f $(TARGET)
	@echo

