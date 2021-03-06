IISHELL = /bin/sh
CC = g++

CFLAGS = -g  -m64 -Wall -I"../.." -I"/opt/local/include" -O3 -DNDEBUG -ffast-math -Wno-deprecated-declarations 
LDFLAGS = -g -m64 -L"/opt/local/lib" -framework GLUT -framework OpenGL -framework Cocoa 

TARGET  = tinsel

SOURCES = $(wildcard src/*.cpp src/cjson/*.c)
HEADERS = $(wildcard src/*.h)
TESTS = $(wildcard src/tests/*.h)

OBJECTS = $(SOURCES:.cpp=.o) 

all: $(TARGET)
	./$(TARGET) data/transmission.tin 

$(TARGET): $(OBJECTS) makefile $(TESTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $(TARGET) 

clean:
	-rm -f $(OBJECTS)
	-rm -f $(TARGET)

%.o: %.cpp $(HEADERS) $(TESTS) makefile
	$(CC) $(CFLAGS) -c -o $@ $<


run: $(TARGET)
	./$(TARGET)

.PHONY : all clean
