htree: htree.c clean
	gcc -Wall -Werror -std=gnu99 -pthread -o htree htree.c

clean:
	rm htree