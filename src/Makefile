# Compiler and flags
CC = gcc
CFLAGS = -O3
LDFLAGS= -lpthread -lgd

# Source files and target
SRCS = old-photo-parallel-B.c helper_f.c threads.c image-lib.c
OBJS = $(SRCS:.c=.o)
TARGET = old-photo-parallel-B

# Default target to build the program
all: $(TARGET)

# Build the main executable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Compile object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean only the executable and object files
clean:
	rm -f $(TARGET) $(OBJS)

# Clean all generated files and directories
clean_all: clean
	rm -fr ./*-dir

# Run the program
run: all
	./$(TARGET)
