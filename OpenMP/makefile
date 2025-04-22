# Compiler
CC = clang

# Compiler flags
CFLAGS = -Wall -Wextra -g -fopenmp

# Target executables
TARGET = main pi

# Source files
SRCS = main.c ImageStuff.c ImageFlip.c
HEADERS = ImageStuff.h ImageFlip.h

# Object files
OBJS = $(SRCS:.c=.o)

# Default target
all: main pi

# Build main executable
main: $(SRCS) $(HEADERS)
	$(CC) $(CFLAGS) $(SRCS) -o main

# Build pi executable
pi: pi.c
	$(CC) $(CFLAGS) pi.c -o pi

# Clean up build files
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean