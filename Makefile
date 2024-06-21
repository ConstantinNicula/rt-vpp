# Project configuration params
TARGET_NAME = video_streamer 
LIBS = -lm 
CC = gcc 
CFLAGS = -g -Wall 

# Makefile rules 
TARGET = build/$(TARGET_NAME)

C_FILES = $(wildcard src/*.c)
OBJECTS = $(patsubst src/%.c, build/%.o, $(C_FILES))

.PHONY: default all clean 

all: default 
default: build_loc $(TARGET)

build/%.o: src/%.c 
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(CFLAGS) $(LIBS) -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

build_loc: 
	mkdir -p build

clean: 
	-rm -f build/*
	
