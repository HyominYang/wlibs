CC=g++
FLAGS=-g -fPIC
FLAGS_TEST=-g

LIBS=-lpthread

SRC=confr.cpp

OBJ=confr.o

SLIB_NAME=libwc.a

all: lib test

lib:
	$(CC) $(FLAGS) -c $(SRC) $(INCLUDES)
	ar -r $(SLIB_NAME) $(OBJ) 

test:
	$(CC) $(FLAGS_TEST) -o confr.exe confr.cpp -L. $(LIBS) -DTEST_MAIN__

install:
	cp libwc.a ../libs

clean:
	rm -f $(SLIB_NAME)
	rm -f $(OBJ)
	rm -f confr.exe
	rm -f ../libs/libwc.a
