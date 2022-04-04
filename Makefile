run: htree
	clear
	./htree test.txt 2

htree: 
	gcc -o htree htree.c -lm -pthread
