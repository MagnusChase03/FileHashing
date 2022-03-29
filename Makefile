htree: obj
	gcc -o htree bin/htree.o

obj:
	gcc -c -o bin/htree.o htree.c