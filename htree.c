#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <errno.h> // for EINTR
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

// Print out the usage of the program and exit.
void Usage(char *);
uint32_t jenkins_one_at_a_time_hash(const uint8_t *, size_t);

#define BSIZE 4096
uint32_t *htable;

int main(int argc, char **argv)
{
    int32_t fd;
    uint32_t nblocks;

    // input checking
    if (argc != 3)
        Usage(argv[0]);

    // open input file
    fd = open(argv[1], O_RDWR);
    if (fd == -1)
    {
        perror("open failed");
        exit(EXIT_FAILURE);
    }
    // use fstat to get file size
    // calculate nblocks

    printf(" no. of blocks = %u \n", nblocks);

    double start = GetTime();

    // calculate hash of the input file

    double end = GetTime();
    printf("hash value = %u \n", hash);
    printf("time taken = %f \n", (end - start));
    close(fd);
    return EXIT_SUCCESS;
}

uint32_t
jenkins_one_at_a_time_hash(const uint8_t *key, size_t length)
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

void Usage(char *s)
{
    fprintf(stderr, "Usage: %s filename height \n", s);
    exit(EXIT_FAILURE);
}