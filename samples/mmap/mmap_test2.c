#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

int main(int argc, char *argv[])
{
    int fd;

    // special case: it will map file specified in manifest and return pointer to it
    // note: so far mapped file size will not be returned
    fd = open("http://input.data", O_RDONLY);

// ###
fd = 0xfeff0000;
    printf("mapped address == %x\n", fd);
    write(STDOUT_FILENO, (void*)fd, 100);
    exit(EXIT_SUCCESS);
}
