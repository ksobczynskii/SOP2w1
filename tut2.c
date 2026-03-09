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
volatile sig_atomic_t last_sig = 0;
void usage(char *name)
{
    fprintf(stderr, "USAGE: %s fifo_file\n", name);
    exit(EXIT_FAILURE);
}
void sig_operate(int sig) {
    if (sig==SIGINT)
        last_sig = SIGINT;
}
void sig_kill_kid(int sig) {
    if (rand()%5 == 0) {
        printf("Process '%d' dies... \n", getpid());
        exit(EXIT_SUCCESS);
    }

}

void child_controller(int sig) {
    pid_t pid;
    while (1)
    {
        errno = 0;
        pid = waitpid(0, NULL, WNOHANG);

        if (pid == 0)
            continue;
        if (pid < 0)
        {
            if (errno == ECHILD)
                break;
            ERR("waitpid");
        }
    }
}
void set_handle(void (*f)(int), int sig) {
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;

    if (-1 == sigaction(sig, &act, NULL))
        ERR("sigaction");
}
void close_descriptors(int *P, int n) {
    for (int i=0; i<n; i++) {
        if (close(P[i]) < 0)
            ERR("close");
    }
}

void child_work(int P_reader, int R_writer) {
    srand(getpid());
    set_handle(sig_kill_kid,SIGINT);
    unsigned char c,s;
    char buf[MAX_BUFF];
    while (1) {
        if (TEMP_FAILURE_RETRY(read(P_reader,&c,1)) < 0)
            ERR("read");
        s = 1 + rand()%MAX_BUFF;
        buf[0] = s;
        memset(buf + 1, c, (int)s);
        if (TEMP_FAILURE_RETRY(write(R_writer,buf,s+1)) < 0)
            ERR("write");
    }
}

void make_all_kids(int* P, int R[2], int n) {
    int tmpfd[2];
    for (int i=0; i<n; i++) {
        if (pipe(tmpfd) < 0)
            ERR("pipe");
        switch (fork()) {
            case -1:
                ERR("fork");
            case 0:
                if (TEMP_FAILURE_RETRY(close(tmpfd[1])) < 0)
                    ERR("close");
                if (close(R[0]) < 0)
                    ERR("close");
                printf("Process '%d' is process no. %d\n", getpid(), i);
                child_work(tmpfd[0], R[1]); // przekazujemy reader do P[i] i writer do R
            default:
                if (TEMP_FAILURE_RETRY(close(tmpfd[0])) < 0)
                    ERR("close");
                P[i] = tmpfd[1]; // writer
                break;
        }
    }
}
void parent_work(int* P, int R, int n) {
    srand(getpid());

    unsigned char x;
    ssize_t count;
    char buf[MAX_BUFF + 1];

    while (1) {
        if (last_sig == SIGINT) {
            int receiver = rand()%n;
            int initial = receiver;
            printf("Random process is process no. %d\n", receiver);
            while (P[receiver] == 0) {
                receiver++;
                receiver = receiver % n;
                printf("Dead process! newprocess is %d\n", receiver);
                if (receiver == initial) {
                    if (TEMP_FAILURE_RETRY(close(R)) < 0)
                        ERR("close");
                    return;
                }
            }
            initial = receiver;

            char sign = rand()%('z' - 'a') + 'a';
            errno = 0;

            while (write(P[receiver],&sign,1) < 0) { // FIXME Problem - write idzie pierwszy a potem proces dziecko sie konczy wiec z poziomu parenta jest wrazenie jakby byl sukces
                if (errno == EPIPE) {
                    printf("Tried to send to a dead process. Increasing...\n");
                    errno = 0;
                    if (TEMP_FAILURE_RETRY(close(P[receiver])) < 0)
                        ERR("close");
                    P[receiver] = 0;
                    receiver++;
                    receiver = receiver% n;
                }
                else
                    ERR("write");
                if (receiver == initial)
                    exit(EXIT_SUCCESS);
            }
            last_sig = 0;

            // reading:

            errno = 0;
            count = read(R,&x,1);

            if (count < 0 && errno == EINTR)
                continue;
            if (count < 0 && errno == EPIPE) {
                if (TEMP_FAILURE_RETRY(close(R)) < 0) {
                    ERR("close");
                }
                return;
            }
            if (count == 0)
                return;
            errno = 0;
            if (TEMP_FAILURE_RETRY(read(R,buf,x)) < 0) {
                if (errno == EPIPE) {
                    if (close(R) < 0) {
                        ERR("close");
                    }
                    return;
                }
            }
            buf[(int)x] = '\0';
            printf("\n%s\n",buf);
        }
    }
}

int main (int argc, const char** argv) {

    if (argc !=2)
        usage("argc");
    int n = atoi(argv[1]);

    set_handle(sig_operate,SIGINT);
    set_handle(SIG_IGN, SIGPIPE);
    set_handle(child_controller, SIGPIPE);

    int R[2];
    if (pipe(R) < 0)
        ERR("pipe");

    int *P = malloc(sizeof(int)*n);

    memset(P,0,n*sizeof(int));
    make_all_kids(P,R,n);

    if (close(R[1]) < 0)
        ERR("close");

    // close_descriptors(P,n);
    parent_work(P,R[0],n);
    while (n--)
        if (P[n] && TEMP_FAILURE_RETRY(close(P[n])))
            ERR("close");

    free(P);
    exit(EXIT_SUCCESS);

}