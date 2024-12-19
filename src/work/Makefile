# Target to build the parallel processing program
all: par

# Build the main executable
par: old-photo-parallel-A.c helper_f.c image-lib.c
	gcc old-photo-parallel-A.c helper_f.c threads.c image-lib.c -g -o old-photo-parallel-A -lgd -lpthread

# Clean only the executable
clean:
	rm -f par

# Clean all generated files and directories
clean_all: clean
	rm -fr ./*-dir

# Run the program
run_all: all
	./par
