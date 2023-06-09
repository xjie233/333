OBJS := server client
all: $(OBJS)

server: msg_svr.c msg.h 
	gcc -o $@ $^ -D_DEBUG

client: msg_client.c msg.h
	gcc -o $@ $^ -lpthread

clean:
	$(RM) $(OBJS)
