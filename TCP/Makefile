CC=g++ -g -Wall 

TCP: main.cpp
	$(CC) $(CFLAGS) -o TCP main.cpp ConnectionInstance.cpp ConnectionInstance.h packet_utils.h packet_utils.cpp Logger.h Logger.cpp -lpthread

clean:
	rm  TCP