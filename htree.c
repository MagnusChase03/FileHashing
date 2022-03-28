#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int main(int argc, char **argv)
{

    if (argc < 3)
    {

        printf("Usage: ./htree [file] [treeHeight]");
        return 1;
    }

    return 0;
}