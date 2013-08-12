CC := gcc
LD := ld
LIBS := -lcurl -ltidy
DEBUGFLAGS_C-Compiler := -g -O0 -fno-omit-frame-pointer -pipe -Wall
SRC := main.c
OBJ := $(SRC:.c=.o)
TARGET := crawler

%.o: %.c
	$(CC) $(DEBUGFLAGS_C-Compiler) -c -fmessage-length=0 -o $@ $<

all: $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LIBS)

clean:
	rm -f $(OBJ) rm -f $(TARGET)
