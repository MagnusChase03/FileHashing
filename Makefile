run: htree
	clear
	./htree test.txt 2

htree: 
	gcc -g -o htree htree.c -lm -pthread
