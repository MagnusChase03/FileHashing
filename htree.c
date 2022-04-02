#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
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

        if (data.id == nthreads - 1)
        {

            blocksToRead = nblocks - ((nthreads - 1) * (nblocks / nthreads));
        }
        else
        {

            blocksToRead = nblocks / nthreads;
        }

        printf("%d, reading %d\n", data.id, blocksToRead);

        int fd = open(filename, O_RDONLY);
        lseek(fd, data.id * BSIZE * (nblocks / nthreads), SEEK_SET);

        // Create holder for hashed blocks and zero out memory
        char *hashStr = malloc(8 * blocksToRead);
        for (int i = 0; i < 8 * blocksToRead; i++)
        {

            hashStr[i] = '\0';
        }

        // Read all blocks and put into hashStr
        int hashStrIndex = 0;
        for (int i = 0; i < blocksToRead; i++)
        {

            char *buffer = malloc(BSIZE);
            read(fd, buffer, BSIZE);
            uint32_t hashed = hash(buffer, BSIZE);

            char hashStrPrt[8];
            sprintf(hashStrPrt, "%x", hashed);

            // Concatanate read block hashes
            for (int j = 0; j < 8; j++)
            {

                hashStr[hashStrIndex] = hashStrPrt[j];
                hashStrIndex += 1;
            }

            free(buffer);
        }

        printf("%d:%s\n", data.id, hashStr);

        close(fd);

        return hashStr;
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

    char *leftHash;
    char *rightHash;

    pthread_create(&leftChild, NULL, compute, &dataLeft);
    pthread_create(&rightChild, NULL, compute, &dataRight);

    pthread_join(leftChild, &leftHash);
    pthread_join(rightChild, &rightHash);

    printf("Left Hash: %s Right Hash: %s\n", leftHash, rightHash);
    return NULL;
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
    nblocks = (fileStats.st_size / BSIZE) + 1;

    close(fd);

    printf(" no. of blocks = %u \n", nblocks);

    // double start = GetTime();

    // calculate hash of the input file
    struct threadStruct data;
    data.height = height;
    data.id = 0;

    pthread_t rootThread;
    pthread_create(&rootThread, NULL, compute, &data);
    pthread_join(rootThread, NULL);

    // double end = GetTime();
    printf("hash value = %u \n", hash);
    // printf("time taken = %f \n", (end - start));

    return 0;
}