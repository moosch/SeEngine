CC = gcc

CFLAGS = -std=c17 -g -O2

LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

SRC = main.c

TARGET = game

game:
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

.PHONY: test clean

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
