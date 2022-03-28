#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

// Struct for threads
struct info
{

    char *filename;
    int height;
    int id;
};

// Hash Function
uint32_t hash(char *key, int length)
{
    int i = 0;
    uint32_t hash = 0;
    while (i != length)
    {
        hash += key[i++];
        hash += hash << 10;
        hash ^= hash >> 6;
    }
    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;
    return hash;
}

// Get block size
int getBlockSize(char *filename)
{

    int fd = open(filename, O_RDONLY);

    struct stat fileStats;
    fstat(fd, &fileStats);

    int blockSize = fileStats.st_blksize;
    return blockSize;
}

// Get number of blocks in file
int getBlocks(char *filename)
{

    int fd = open(filename, O_RDONLY);

    struct stat fileStats;
    fstat(fd, &fileStats);

    int totalSize = fileStats.st_size;
    int blockSize = fileStats.st_blksize;
    int blocks = totalSize / blockSize;

    return blocks + 1;
}

// Read blocks of file
void *run(void *args)
{

    struct info tInfo = *((struct info *)args);

    if (tInfo.height == 1)
    {

        printf("LEAF\n");
        return NULL;
    }
    else
    {

        tInfo.height -= 1;

        // Create Children
        pthread_t leftChild;
        pthread_t rightChild;

        int orginalID = tInfo.id;

        tInfo.id = (orginalID * 2) + 1;
        pthread_create(&leftChild, NULL, run, &tInfo);

        tInfo.id = (orginalID * 2) + 1;
        pthread_create(&rightChild, NULL, run, &tInfo);

        pthread_join(leftChild, NULL);
        pthread_join(rightChild, NULL);
        printf("%d height\n", tInfo.height + 1);
    }

    return NULL;
}

int main(int argc, char **argv)
{

    // Check for usage
    if (argc < 3)
    {

        printf("Usage: ./htree [file] [treeHeight]");
        return 1;
    }

    // Grab command line arguments
    char *filename = argv[1];
    int treeHeight = atoi(argv[2]);

    // Info for thread
    struct info tInfo;
    tInfo.filename = filename;
    tInfo.height = treeHeight;
    tInfo.id = 0;

    // Read blocks
    pthread_t readThread;
    pthread_create(&readThread, NULL, run, &tInfo);
    pthread_join(readThread, NULL);

    return 0;
}