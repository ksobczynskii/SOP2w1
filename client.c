#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

#define BUF_SIZE (PIPE_BUF - sizeof(pid_t))


void usage(char *name)
{
    fprintf(stderr, "USAGE: %s fifo_file\n", name);
    exit(EXIT_FAILURE);
}
ssize_t bulk_read(int fd, char* buf, size_t nbyte)
{
    size_t count = nbyte;
    ssize_t bytes_read = 0;
    while ((bytes_read = read(fd,buf,count) > 0))
    {
        buf += bytes_read;
        count -= bytes_read;
    }
    if (bytes_read == 0)
        return nbyte - count;

    return -1;
}
void write_to_fifo(int fifo, int file)
{
    pid_t pid = getpid();
    // printf("pid == %d\n",pid);
    char buf1[PIPE_BUF];
    char *buf;
    memcpy(buf1, &pid,sizeof(pid_t));
    // *((pid_t *)buf1) = getpid();
    buf = buf1 + sizeof(pid_t);
    // printf("sizeof(pid_t) == %ld\n", sizeof(pid_t));
    ssize_t bytes_read;
    while ((bytes_read = read(file,buf,BUF_SIZE)) > 0)
    {
        // printf("Read %s from source. read %ld bytes\n", buf,bytes_read);
        if (bytes_read != BUF_SIZE)
        {
            memset(buf+bytes_read,0,BUF_SIZE - bytes_read);
        }
        if(write(fifo,buf1,PIPE_BUF) < 0)
            ERR("write");
    }
    if (bytes_read < 0)
        ERR("bulk_read");
}

int main(int argc, char** argv)
{
    if (argc != 3)
        usage(argv[0]);
    // argv[1] - fifo, argv[2] - plik
    errno = 0;
    if (mkfifo(argv[1],S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) < 0)
    {
        if (errno != EEXIST)
            ERR("mkfifo");
    }
    int fifo;
    int file;
    if ((fifo = open(argv[1],O_WRONLY)) < 0)
        ERR("open fifo");
    if ((file = open(argv[2],O_RDONLY)) < 0)
        ERR("open file");
    write_to_fifo(fifo,file);

    if (close(fifo) < 0)
        ERR("close fifo");
    if (close(file) < 0)
        ERR("close file");
    // printf("Client stops writing ... \n");
    exit(EXIT_SUCCESS);
}