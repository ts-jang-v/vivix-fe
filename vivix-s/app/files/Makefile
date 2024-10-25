# app/Makefile
# Author: Hong SeungKi <hongseungki@vieworks.co.kr>
include $(MAK_DIR)/target.mak

DEBUG_FLAGS += \
	-D_DEBUG_MAIN \
	-D_DEBUG_TCP_SERVER\

AR_FLAGS +=

CC_FLAGS += \
	-I$(SYS_INC_DIR) \
	$(DEBUG_FLAGS)

LD_FLAGS +=


LIBS = \
	$(LIB_DIR)/vw_xml.a \
	$(LIB_DIR)/os_lib.a \
	$(LIB_DIR)/network.a \
	$(LIB_DIR)/utils.a \

DIRS = \
	app/os_lib \
	app/network \
	app/utils \
	app/xml \
	app/logd \
	app/detector \

.PHONY : subdirs $(DIRS)

SYS_LIBS = \
	-lpthread

all: make_cgi make_libs  
	@echo 

make_cgi:
	$(MAKE) -C web/cgi_src/thirdParty/sha_1
	$(MAKE) -C web/cgi_src/thirdParty/qdecoder/src/qdecoder-12.0.4/src
	$(MAKE) -C web/cgi_src


make_libs: subdirs $(DIRS)

subdirs : $(DIRS)

$(DIRS) :
	$(MAKE) -C $@

clean_libs:
	@for DIR in $(DIRS); \
	do \
		echo; \
		cd $$DIR; \
		make clean; \
		cd ..; \
	done

clean: clean_libs 
	@echo
	@echo Removing: $(OBJS) $(LIBS)
	@rm -f $(LIB_DIR)/*.a
	@echo
