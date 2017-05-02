CC = gcc
CFLAGS = -g -Wall
LIBS = -largp

TARGET = fundamentals

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) $(LIBS) -o $(TARGET) $(TARGET).c

clean:
	$(RM) $(TARGET)
