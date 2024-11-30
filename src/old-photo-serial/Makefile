all: par


par: old-photo-parallel-A.c image-lib.c image-lib.h
	gcc old-photo-parallel-A.c image-lib.c image-lib.h -g -o par -lgd

clean:
	rm par

clean_all: clean
	rm -fr ./*-dir

run_all: all
	./old-photo-serial
