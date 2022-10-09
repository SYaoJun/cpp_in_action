all:
	gcc main.c -o jobthing -pedantic -Wall -std=gnu99 -I/local/courses/csse2310/include -L/local/courses/csse2310/lib -lcsse2310a3

clear:
	rm -rf jobthing

