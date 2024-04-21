CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11
TARGET = is-it-rarjpeg.exe
SOURCE_FILES = main.c
OBJECTS = $(SOURCE_FILES:.c=.o)
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

run: $(TARGET)
	./$(TARGET)

test: $(TARGET)
	./$(TARGET) ./03_types_homework-12926-ac0783/zippng.png

clean:
	rm -f $(OBJECTS) $(TARGET)
