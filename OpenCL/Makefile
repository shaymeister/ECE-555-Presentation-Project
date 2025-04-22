# Compiler and flags
CC = gcc
LDFLAGS = -lOpenCL

# Source files
SRC = imflipCL.c
OBJ = $(SRC:.c=.o)

# Output executable
EXEC = imflipCL

# OpenCL Kernel file
KERNEL_SRC = kernels.cl

# Default target
all: $(EXEC)

# Compile the C source file to an object file
$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c $(SRC)

# Link object files to create the executable
$(EXEC): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $(EXEC)

# Clean the build files
clean:
	rm -f $(OBJ) $(EXEC)

run_copy:
	./imflipCL dogL.bmp dogL_copy.bmp SimpleCopy 128

run_vflip:
	./imflipCL dogL.bmp dogL_vflip.bmp Vflip 128

run_hflip:
	./imflipCL dogL.bmp dogL_hflip.bmp Hflip 128	
