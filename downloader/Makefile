CC=g++
#FLAGS=-Wall -g -fPIC
FLAGS= -g -fPIC
LIBS= -I../sock -L../libs -lpthread -lwe

SRC_CLIENT=main_client.cpp \
	../encrypt/aria.c

SRC_SERV=main_serv.cpp \
	../encrypt/aria.c

TARGET_CLIENT=wclient
TARGET_SERV=wserver

all: client server

client:
	$(CC) $(FLAGS) -o $(TARGET_CLIENT) $(SRC_CLIENT) $(INCLUDES) $(LIBS)

server:
	$(CC) $(FLAGS) -o $(TARGET_SERV) $(SRC_SERV) $(INCLUDES) $(LIBS)

clean:
	rm -rf $(TARGET_CLIENT) $(TARGET_SERV)

test:
	./$(TARGET_SERV) 11111 &
	sleep 1
	./$(TARGET_CLIENT) 127.0.0.1 11111 &

kill:
	killall $(TARGET_CLIENT) $(TARGET_SERV)
