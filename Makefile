# Compiler and flags
CC = gcc
CFLAGS = -O3 -std=c23 -Wall -Wextra 
LDFLAGS= -lpthread -lgd

# Source files and target
SRCS = src/photo-old.c src/helper_f.c src/threads.c src/image-lib.c
OBJS = $(SRCS:.c=.o)
TARGET = build/photo-old

# Default target to build the program
all: $(TARGET)

# Build the main executable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Compile object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean only the object files
clean:
	rm -f $(OBJS)

# Clean all the executable and object files
clean_all: clean
	rm -fr ./*-dir
