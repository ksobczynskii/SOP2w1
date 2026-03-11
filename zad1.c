#define _GNU_SOURCE
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>


#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
#define MAX_BUFF 200
void usage(char *name)
{
    fprintf(stderr, "USAGE: %s pipe\n", name);
    exit(EXIT_FAILURE);
}

void child_work(int reader, int writer) {
    char buf[4];
    srand(getpid());
    while (1) {
        if (read(reader,buf,sizeof(int)) < 0)
            ERR("read");
        int x = *((int*)buf);
        printf("Process [%d] received %d\n", getpid(), x);
        if (x==0) {
            if (close(reader) < 0)
                ERR("close");
            if (close(writer) < 0)
                ERR("close");
            return;
        }
        sleep(1);
        int a = rand()%21 - 10;
        x += a;
        if (write(writer,&x,sizeof(int)) < 0)
            ERR("read");
    }
}

void parent_work(int reader, int writer) {
    int x = 1;
    if (write(writer,&x,4) < 0)
        ERR("write");
    child_work(reader, writer);
}

int main(int argc, const char** argv) {
    if (argc > 1)
        usage("argc");

    pid_t child_1;
    pid_t child_2;

    int pipe1[2];
    int pipe2[2];
    int pipe3[2];


    if (pipe(pipe1) < 0)
        ERR("pipe1");
    if (pipe(pipe2) < 0)
        ERR("pipe2");
    if (pipe(pipe3) < 0)
        ERR("pipe3");

    switch (child_1 = fork()) {
        case -1:
            ERR("fork");
        case 0:
            if (close(pipe1[1]) < 0)
                ERR("close pipe1");
            if (close(pipe2[0]) < 0)
                ERR("close pipe2");
            if (close(pipe3[0]) < 0)
                ERR("close pipe3");
            if (close(pipe3[1]) < 0)
                ERR("close pipe3");
            child_work(pipe1[0], pipe2[1]);
            exit(EXIT_SUCCESS);
        default:
            break;
    }

    switch (child_2 = fork()) {
        case -1:
            ERR("fork");
        case 0:
            if (close(pipe1[1]) < 0)
                ERR("close pipe1");
            if (close(pipe1[0]) < 0)
                ERR("close pipe1");
            if (close(pipe2[1]) < 0)
                ERR("close pipe2");
            if (close(pipe3[0]) < 0)
                ERR("close pipe3");
            child_work(pipe2[0], pipe3[1]);
            exit(EXIT_SUCCESS);
        default:
            break;
    }
    if (close(pipe1[0]) < 0)
        ERR("close pipe1");
    if (close(pipe2[0]) < 0)
        ERR("close pipe2");
    if (close(pipe2[1]) < 0)
        ERR("close pipe2");
    if (close(pipe3[1]) < 0)
        ERR("close pipe3");

    parent_work(pipe3[0], pipe1[1]);

    if (waitpid(child_1,NULL,0) < 0)
        ERR("waitpid");
    if (waitpid(child_2,NULL,0) < 0)
        ERR("waitpid");

    exit(EXIT_SUCCESS);


}