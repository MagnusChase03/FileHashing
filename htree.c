#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <errno.h> // for EINTR
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <sys/mman.h>

// Needed Globals
char *filename;
uint height;
uint blockSize;
long int fileSize;
long int blocks;
uint threads;

// Thread struct to keep perosnal data
struct threadData
{

    int id;
    int height;
};

void Usage(char *);
uint32_t hash(const uint8_t *, size_t);
uint power(uint base, uint exp);
void *calculateHash(void *args);

int main(int argc, char **argv)
{
    // input checking
    if (argc != 3)
        Usage(argv[0]);

    filename = malloc(100);
    strcpy(filename, argv[1]);

    height = atoi(argv[2]);
    threads = power(2, height);

    // use fstat to get file size
    // calculate nblockss
    int fd = open(filename, O_RDONLY);

    struct stat fileStats;
    fstat(fd, &fileStats);

    blockSize = 4096;
    fileSize = (long int)fileStats.st_size;
    blocks = (long int)fileSize / blockSize;

    close(fd);

    printf(" block size = %u \n", blockSize);
    printf(" file size = %lu \n", fileSize);
    printf(" no. of blocks = %lu \n", blocks);
    printf(" no. of threads = %u \n", threads);
    printf(" no. of blocks / thread = %lu \n", blocks / threads);

    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);

    // calculate hash of the input file
    struct threadData data;
    data.id = 0;
    data.height = height;

    uint32_t *fileHash = malloc(sizeof(uint32_t));

    pthread_t root;
    pthread_create(&root, NULL, calculateHash, &data);
    pthread_join(root, &fileHash);

    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsedTime = (end.tv_sec - start.tv_sec) * 1e9;
    elapsedTime = (elapsedTime + (end.tv_nsec - start.tv_nsec)) * 1e-9;

    printf("hash value = %u \n", *fileHash);
    printf("time taken = %f \n", (elapsedTime));
    return 0;
}

// Hash function
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

// Print Usage
void Usage(char *s)
{
    fprintf(stderr, "Usage: %s filename height \n", s);
    exit(EXIT_FAILURE);
}

// Power function
uint power(uint base, uint exp)
{

    if (exp == 0)
    {

        return 1;
    }

    if (exp < 0)
    {

        return -1;
    }

    uint res = base;
    for (int i = 0; i < exp - 1; i++)
    {

        res *= base;
    }

    return res;
}

// Threaded function to calculate hash
void *calculateHash(void *args)
{

    struct threadData data = *((struct threadData *)args);

    // Leaf
    if (data.height <= 1)
    {

        int lastId = power(2, height - 1) - 1;
        if (data.id == lastId)
        {

            // Create Extra Left Node
            struct threadData dataLeft;
            dataLeft.height = data.height - 1;
            dataLeft.id = (data.id * 2) + 1;

            pthread_t leftChild;
            uint32_t *leftHash = malloc(sizeof(uint32_t));
            pthread_create(&leftChild, NULL, calculateHash, &dataLeft);
            pthread_join(leftChild, &leftHash);

            // Read current blocks
            int blocksToRead = blocks / threads;

            // Reads blocks and hashes
            int fd = open(filename, O_RDONLY);

            uint8_t *buffer = mmap(NULL, blockSize * blocksToRead, PROT_READ, MAP_PRIVATE, fd, data.id * blockSize * blocksToRead);

            uint32_t *fullHash = malloc(sizeof(uint32_t));
            *fullHash = hash(buffer, blockSize * blocksToRead);

            close(fd);
            munmap(buffer, blockSize * blocksToRead);

            // Combine both hashes
            char thisHashStr[10];
            sprintf(thisHashStr, "%u", *fullHash);

            char leftHashStr[10];
            sprintf(leftHashStr, "%u", *leftHash);

            char combinedHashes[20];
            strcat(combinedHashes, thisHashStr);
            strcat(combinedHashes, leftHashStr);

            // Calculate final hash and return it
            *fullHash = hash((uint8_t *)combinedHashes, strlen(combinedHashes));

            free(leftHash);

            pthread_exit(fullHash);
        }
        else
        {

            int blocksToRead = blocks / threads;

            // Reads blocks and hashes
            int fd = open(filename, O_RDONLY);

            uint8_t *buffer;

            buffer = mmap(NULL, blockSize * blocksToRead, PROT_READ, MAP_PRIVATE, fd, data.id * blockSize * blocksToRead);

            uint32_t *fullHash = malloc(sizeof(uint32_t));
            *fullHash = hash(buffer, blockSize * blocksToRead);

            close(fd);
            munmap(buffer, blockSize * blocksToRead);

            pthread_exit(fullHash);
        }
    }

    // Create children if not leaf node
    struct threadData dataLeft;
    dataLeft.height = data.height - 1;
    dataLeft.id = (data.id * 2) + 1;

    struct threadData dataRight;
    dataRight.height = data.height - 1;
    dataRight.id = (data.id * 2) + 2;

    pthread_t leftChild;
    pthread_t rightChild;

    uint32_t *leftHash = malloc(sizeof(uint32_t));
    uint32_t *rightHash = malloc(sizeof(uint32_t));

    pthread_create(&leftChild, NULL, calculateHash, &dataLeft);
    pthread_create(&rightChild, NULL, calculateHash, &dataRight);

    pthread_join(leftChild, &leftHash);
    pthread_join(rightChild, &rightHash);

    // Read blocks and hash
    int blocksToRead = blocks / threads;

    int fd = open(filename, O_RDONLY);

    uint8_t *buffer = mmap(NULL, blockSize * blocksToRead, PROT_READ, MAP_PRIVATE, fd, data.id * blockSize * blocksToRead);

    uint32_t *fullHash = malloc(sizeof(uint32_t));
    *fullHash = hash(buffer, blockSize * blocksToRead);

    close(fd);
    munmap(buffer, blockSize * blocksToRead);

    // Combine all computed hashes
    char thisHashStr[10];
    sprintf(thisHashStr, "%u", *fullHash);

    char leftHashStr[10];
    sprintf(leftHashStr, "%u", *leftHash);

    char rightHashStr[10];
    sprintf(rightHashStr, "%u", *rightHash);

    char combinedHashes[30];
    strcat(combinedHashes, thisHashStr);
    strcat(combinedHashes, leftHashStr);
    strcat(combinedHashes, rightHashStr);

    // Calculate final hash and return it
    *fullHash = hash((uint8_t *)combinedHashes, strlen(combinedHashes));

    // Free memory
    free(leftHash);
    free(rightHash);

    pthread_exit(fullHash);

    return NULL;
}
