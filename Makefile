all: shm cpy;

shm: shm.c;
	gcc -o shm.out shm.c;

cpy: cpy.c;
	gcc -o cpy.out cpy.c;