CC=g++
CFLAGS=-c -Wall
LDFLAGS=
SOURCES=serverUDP.cpp clientUDP.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=ServerUDP

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@