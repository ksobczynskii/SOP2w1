#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void read_from_fifo(int fd)
{
    char c;
    ssize_t bytes;
    // int counter = 1;
    // printf("(sizeof(pid_t)) = %ld\n", sizeof(pid_t));
    while ((bytes = read(fd,&c,1) > 0))
    {
        // if (counter < sizeof(pid_t))
        // {
        //     printf("%d",c);
        //     counter++;
        //     continue;
        // }

        if (isalnum(c))
        {
            printf("%c",c);
        }
    }
    if (bytes < 0)
        ERR("read");
    // printf("\n");
}
void usage(char *name)
{
    fprintf(stderr, "USAGE: %s fifo_file\n", name);
    exit(EXIT_FAILURE);
}

int main(int argc, const char** argv)
{
    if (argc != 2)
    {
        perror("wrong amt of args");
        exit(EXIT_FAILURE);
    }

    errno = 0;
    if (mkfifo(argv[1],S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) < 0)
    {
        if (errno != EEXIST)
            ERR("mkfifo");
    }
    int fd;
    if ((fd = open(argv[1],O_RDONLY)) < 0)
        ERR("open");
    printf("Waiting for write...\n");
    read_from_fifo(fd);
    if (close(fd))
        ERR("close");
    printf("\nExiting FIFO...\n");
    // if (unlink(argv[1]) < 0)
    //     ERR("unlink");
    exit(EXIT_SUCCESS);

}