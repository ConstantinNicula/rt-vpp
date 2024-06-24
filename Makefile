# Project configuration params
TARGET_NAME = rt-vpp 
LIBS = -lm 
CC = gcc 
CFLAGS = -g -Wall 
# Get compiler and linker flags for gstreamer
DEPS = `pkg-config --cflags --libs gstreamer-1.0`

# Makefile rules 
TARGET = build/$(TARGET_NAME)

C_FILES = $(wildcard src/*.c)
OBJECTS = $(patsubst src/%.c, build/%.o, $(C_FILES))

.PHONY: default all clean 

all: default 
default: build_loc $(TARGET)

build/%.o: src/%.c 
	$(CC) $(CFLAGS) $(DEPS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(CFLAGS) $(LIBS) $(DEPS) -o $@  

.PRECIOUS: $(TARGET) $(OBJECTS)

build_loc: 
	mkdir -p build

clean: 
	-rm -f build/*
	
