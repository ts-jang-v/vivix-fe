BINDIR:=./bin

LOGIN=vieworks.cgi
MSGSND=msgsnd.cgi
MSGRCV=msgrcv.cgi

CFLAGS= -g -I./thirdParty/qdecoder/include -I./thirdParty/sha_1/include -I$(SYS_INC_DIR)

LDFLAGS= ./lib/libsha1.a ./lib/libqdecoder.a -lm

all : $(LOGIN)

$(LOGIN) : 
	$(CC) -o $(BINDIR)/$(LOGIN) src/login.c  $(CFLAGS) $(LDFLAGS)  
	cp -f ./bin/$(LOGIN) ../www_dir/cgi-bin/

clean :
	rm -rf $(BINDIR)/$(LOGIN)
	rm -rf $(BINDIR)/$(MSGSND)
	rm -rf $(BINDIR)/$(MSGRCV)
