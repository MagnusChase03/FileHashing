#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <sys/mman.h>
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

                uint32_t *nullReturn = mmap(NULL, sizeof(uint32_t), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
                if (nullReturn == MAP_FAILED)
                {

                    perror("Null Return Pointer Error");
                    exit(1);
                }

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
        char *buffer = mmap(NULL, BSIZE * blocksToRead, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
        if (buffer == MAP_FAILED)
        {

            perror("Full Hash Pointer Error");
            exit(1);
        }

        read(fd, buffer, BSIZE * blocksToRead);

        uint32_t *fullHash = mmap(NULL, sizeof(uint32_t), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
        if (fullHash == MAP_FAILED)
        {

            perror("Full Hash Pointer Error");
            exit(1);
        }

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

    uint32_t *leftHash = mmap(NULL, sizeof(uint32_t), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    if (leftHash == MAP_FAILED)
    {

        perror("Left Hash Pointer Error");
        exit(1);
    }

    uint32_t *rightHash = mmap(NULL, sizeof(uint32_t), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    if (rightHash == MAP_FAILED)
    {

        perror("Left Hash Pointer Error");
        exit(1);
    }

    if (pthread_create(&leftChild, NULL, compute, &dataLeft) != 0)
    {

        perror("Thead Creation Failed");
        exit(1);
    }

    if (pthread_create(&rightChild, NULL, compute, &dataRight) != 0)
    {

        perror("Thread Creation Failed");
        exit(1);
    }

    pthread_join(leftChild, &leftHash);
    pthread_join(rightChild, &rightHash);

    // Read blocks and hash
    int blocksToRead;

    if (nthreads > nblocks)
    {

        if (data.id >= nblocks)
        {

            uint32_t *nullReturn = mmap(NULL, sizeof(uint32_t), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
            if (nullReturn == MAP_FAILED)
            {

                perror("Null Return Pointer Error");
                exit(1);
            }

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
    char *buffer = mmap(NULL, BSIZE * blocksToRead, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    if (buffer == MAP_FAILED)
    {

        perror("Full Hash Pointer Error");
        exit(1);
    }

    read(fd, buffer, BSIZE * blocksToRead);

    uint32_t *fullHash = mmap(NULL, sizeof(uint32_t), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    if (fullHash == MAP_FAILED)
    {

        perror("Full Hash Pointer Error");
        exit(1);
    }

    *fullHash = hash(buffer, BSIZE * blocksToRead);

    // printf("%d: %x\n", data.id, *fullHash);

    close(fd);

    // Combine all hashes
    char combinedHash[24];

    char thisHashStr[8];

    // Fill zeros if null hash / not read else convert hash to string
    if (*fullHash != 0)
    {

        sprintf(thisHashStr, "%x", *fullHash);
    }

    char leftHashStr[8];
    if (*leftHash != 0)
    {

        sprintf(leftHashStr, "%x", *leftHash);
    }

    char rightHashStr[8];
    if (*rightHash != 0)
    {

        sprintf(rightHashStr, "%x", *rightHash);
    }

    // Concatanate all hashes
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

    // Calculate final hash and return it
    *fullHash = hash(combinedHash, 24);

    // Free memory
    int error = munmap(leftHash, sizeof(uint32_t));
    if (error != 0)
    {

        perror("Free Left Hash Mapping Failed");
        exit(1);
    }

    error = munmap(rightHash, sizeof(uint32_t));
    if (error != 0)
    {

        perror("Free Left Hash Mapping Failed");
        exit(1);
    }

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

    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);

    // calculate hash of the input file
    struct threadStruct data;
    data.height = height;
    data.id = 0;

    uint32_t *fullHash = malloc(sizeof(uint32_t));

    pthread_t rootThread;
    pthread_create(&rootThread, NULL, compute, &data);
    pthread_join(rootThread, &fullHash);

    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsedTime = (end.tv_sec - start.tv_sec) * 1e9;
    elapsedTime = (elapsedTime + (end.tv_nsec - start.tv_nsec)) * 1e-9;

    printf("hash value = %x \n", *fullHash);
    printf("time taken = %f \n", (double)elapsedTime);

    return 0;
}