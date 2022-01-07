CC=g++ -g -Wall 

client: main.cpp
	$(CC) $(CFLAGS) -o client main.cpp ../TCP/packet_utils.h ../TCP/packet_utils.cpp SWClient.cpp SWClient.h -lpthread

clean:
	rm  client