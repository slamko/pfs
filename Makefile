CC=gcc
LIBS=-lfuse
FLAGS=-D_FILE_OFFSET_BITS=64
TARGET=fss

all: $(TARGET)

$(TARGET): main.c
	$(CC) main.c $(FLAGS) $(LIBS) -o $@
