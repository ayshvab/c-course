CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11
TARGET = convert_to_utf8.exe
SOURCE_FILES = main.c
OBJECTS = $(SOURCE_FILES:.c=.o)
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

test: $(TARGET)
	./test.sh

clean:
	rm -f $(OBJECTS) $(TARGET)
	rm ./utf8_from*.txt
