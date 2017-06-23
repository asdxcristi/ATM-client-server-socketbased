CC=g++
LIBSOCKET=-lnsl
CCFLAGS=-std=c++11 -Wall -Wextra -g
SRV=server
CLT=client

all: build

build: $(SRV) $(CLT)

$(SRV):$(SRV).cpp
	$(CC) -o $(SRV) $(CCFLAGS) $(LIBSOCKET) $(SRV).cpp

$(CLT):	$(CLT).cpp
	$(CC) -o $(CLT) $(CCFLAGS) $(LIBSOCKET) $(CLT).cpp

clean:
	rm -f *.o *~
	rm -f $(SRV) $(CLT)
