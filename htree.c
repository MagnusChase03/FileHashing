#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <errno.h> // for EINTR
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define BSIZE 4096
char *filename;
uint32_t nblocks;
uint32_t nthreads;
uint32_t extraBytes;

// Struct for threads
struct threadStruct
{

    int height;
    int id;
};

// Print out the usage of the program and exit.
void Usage(char *s)
{
    fprintf(stderr, "Usage: %s filename height \n", s);
    exit(1);
}

// Hash Function
uint32_t hash(const uint8_t *key, size_t length)
{
    size_t i = 0;
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

// Compute file hash
void *compute(void *arg)
{

    struct threadStruct data = *((struct threadStruct *)arg);

    // Leaf
    if (data.height == 1)
    {

        int blocksToRead;

        if (nthreads > nblocks)
        {

            if (data.id >= nblocks)
            {

                uint32_t *nullReturn = malloc(sizeof(uint32_t));
                *nullReturn = 0;
                pthread_exit(nullReturn);
            }
            else
            {

                blocksToRead = 1;
            }
        }
        else
        {

            blocksToRead = nblocks / nthreads;
        }

        int fd = open(filename, O_RDONLY);
        lseek(fd, data.id * BSIZE * (nblocks / nthreads), SEEK_SET);

        // HASHES ALL READ BLOCKS AT ONCE
        char buffer[BSIZE * blocksToRead];
        read(fd, buffer, BSIZE * blocksToRead);

        uint32_t *fullHash = malloc(sizeof(uint32_t));
        *fullHash = hash(buffer, BSIZE * blocksToRead);

        // printf("%d: %x\n", data.id, *fullHash);

        close(fd);

        pthread_exit(fullHash);
    }

    // Create children
    struct threadStruct dataLeft;
    dataLeft.height = data.height - 1;
    dataLeft.id = (data.id * 2) + 1;

    struct threadStruct dataRight;
    dataRight.height = data.height - 1;
    dataRight.id = (data.id * 2) + 2;

    pthread_t leftChild;
    pthread_t rightChild;

    uint32_t *leftHash;
    uint32_t *rightHash;

    pthread_create(&leftChild, NULL, compute, &dataLeft);
    pthread_create(&rightChild, NULL, compute, &dataRight);

    pthread_join(leftChild, &leftHash);
    pthread_join(rightChild, &rightHash);

    // Read blocks and hash
    int blocksToRead;

    if (nthreads > nblocks)
    {

        if (data.id >= nblocks)
        {

            uint32_t *nullReturn = malloc(sizeof(uint32_t));
            *nullReturn = 0;
            pthread_exit(nullReturn);
        }
        else
        {

            blocksToRead = 1;
        }
    }
    else
    {

        blocksToRead = nblocks / nthreads;
    }

    int fd = open(filename, O_RDONLY);
    lseek(fd, data.id * BSIZE * (nblocks / nthreads), SEEK_SET);

    // HASHES ALL READ BLOCKS AT ONCE
    char buffer[BSIZE * blocksToRead];
    read(fd, buffer, BSIZE * blocksToRead);

    uint32_t *fullHash = malloc(sizeof(uint32_t));
    *fullHash = hash(buffer, BSIZE * blocksToRead);

    // printf("%d: %x\n", data.id, *fullHash);

    close(fd);

    // Combine all hashes
    char *combinedHash = malloc(24);

    char thisHashStr[8];
    sprintf(thisHashStr, "%x", *fullHash);

    char leftHashStr[8];
    sprintf(leftHashStr, "%x", *leftHash);

    char rightHashStr[8];
    sprintf(rightHashStr, "%x", *rightHash);

    for (int i = 0; i < 8; i++)
    {

        combinedHash[i] = thisHashStr[i];
    }

    for (int i = 8; i < 16; i++)
    {

        combinedHash[i] = leftHashStr[i - 8];
    }

    for (int i = 16; i < 24; i++)
    {

        combinedHash[i] = rightHashStr[i - 16];
    }

    *fullHash = hash(combinedHash, 24);

    pthread_exit(fullHash);
}

int main(int argc, char **argv)
{
    // input checking
    if (argc != 3)
        Usage(argv[0]);

    // Set nthreads
    int height = atoi(argv[2]);
    nthreads = pow(2, height) - 1;
    printf("n of threads: %d\n", nthreads);

    // use fstat to get file size
    // calculate nblocks
    filename = argv[1];
    int fd = open(filename, O_RDONLY);

    struct stat fileStats;
    fstat(fd, &fileStats);
    nblocks = fileStats.st_size / BSIZE;

    close(fd);

    printf(" no. of blocks = %u \n", nblocks);

    clock_t start = clock();

    // calculate hash of the input file
    struct threadStruct data;
    data.height = height;
    data.id = 0;

    uint32_t *fullHash = malloc(sizeof(uint32_t));

    pthread_t rootThread;
    pthread_create(&rootThread, NULL, compute, &data);
    pthread_join(rootThread, &fullHash);

    clock_t end = clock();
    printf("hash value = %x \n", *fullHash);
    printf("time taken = %f \n", ((double) (end - start) / CLOCKS_PER_SEC));

    return 0;
}