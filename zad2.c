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
#define MESSAGE_SIZE 16
#define MAX_N 5
#define MAX_M 10


void usage(char *name)
{
    fprintf(stderr, "USAGE: %s pipe\n", name);
    exit(EXIT_FAILURE);
}

void child_work(int reader, int sender, int M) {
    srand(getpid());
    int cards[MAX_M] = {0};
    for (int i=0; i<M; i++) {
        cards[i] = rand()%M + 1;
    }
    printf("Player '%d' started work!\n", getpid());

    char read_buf[MESSAGE_SIZE];
    char write_buf[MESSAGE_SIZE];
    ssize_t bytes;
    for (int i=0; i<M; i++) {
        if ((bytes = read(reader,read_buf,MESSAGE_SIZE)) < 0)
            ERR("read");

        if (bytes==0)
            return;
        // printf("Read %ld bytes\n", bytes);
        // printf("Received: %s\n", read_buf);
        if (strcmp("new_round",read_buf) != 0)
            ERR("strcmp - moj blad");

        int to_send = cards[M-i-1];
        printf("Proc '%d' will be sendin '%d'\n", getpid(), to_send);
        snprintf(write_buf,sizeof(int),"%d",to_send);
        memset(write_buf + sizeof(int),0,MESSAGE_SIZE - sizeof(int));

        printf("After conversion == %s\n", write_buf);

        if (write(sender,write_buf,MESSAGE_SIZE) < 0)
            ERR("write");
    }



}
void parent_work(pid_t processes[MAX_N],int writers[MAX_N],int readers[MAX_N],int M,int N)
{
    char read_buf[MESSAGE_SIZE];
    char write_buf[MESSAGE_SIZE];
    int points[MAX_N] = {0};
    // snprintf(read_buf,sizeof("new_round"),"new_round");
    char* tmp = "new_round";
    strncpy(write_buf,tmp,strlen(tmp));
    memset(write_buf+strlen(tmp),0,MESSAGE_SIZE - strlen(tmp));
    printf("From writer -- '%s'\n", write_buf);
    // memset(read_buf + sizeof("new_round"),0,MESSAGE_SIZE - sizeof("new_round"));
    for (int i=0; i<M; i++) {
        for (int j = 0; j<N; j++) {
            if (write(writers[j],write_buf,MESSAGE_SIZE) < 0)
                ERR("write");
        }

        int current_round[MAX_N] = {0};
        ssize_t bytes;
        int max = 0;
        for (int j = 0; j < N; j++) {
            if ((bytes = read(readers[j],read_buf,MESSAGE_SIZE)) < 0)
                ERR("read");
            if (bytes==0) {
                printf("Pipe '%d' for process '%d' closed!\n", j, processes[j]);
                return;
            }
            current_round[j] = atoi(read_buf);
            printf("Player '%d' with process '%d' sent '%d' in round '%d' \n",j,processes[j],current_round[j],i);
            if (current_round[j] > max) {
                max = current_round[j];
            }
        }
        int winners = 0;
        for (int j = 0; j<N; j++) {
            if (current_round[j] == max)
                winners++;
        }
        int score = N / winners;
        for (int j = 0; j<N; j++) {
            if (current_round[j] == max)
            {
                printf("Player '%d' with process '%d' won and gets '%d' points\n",j, processes[j], score);
                points[j] = points[j] + score;
            }
        }

    }
    int iMax = -1;
    int max = 0;
    for (int i=0; i<M; i++) {
        if (points[i] > max) {
            iMax = i;
            max = points[i];
        }
    }

    printf("The winner is player '%d' with procces '%d' and %d points!\n", iMax, processes[iMax], max);

}
int main(int argc, const char** argv) {
    if (argc !=3  ) {
        usage("argc");
    }
    int N = atoi(argv[1]);
    int M = atoi(argv[2]);

    if (N < 2 || N > 5 || M > 10 || M < 5)
        usage("argv");
    int writers[MAX_N] = {0};
    int readers[MAX_N] = {0};
    pid_t processes[MAX_N] = {0};
    int tmp_sender[2];
    int tmp_reader[2];

    for (int i=0; i<N; i++) {
        // printf("i is equal to '%d'\n", i);


        if (pipe(tmp_sender) < 0)
            ERR("pipe_sender");
        if (pipe(tmp_reader) < 0)
            ERR("pipe_reader");

        switch (processes[i] = fork()) {
            case -1:
                ERR("fork");
            case 0:
                printf("Fork no. '%d'. Created process '%d'\n", i, getpid());
                if (close(tmp_reader[0]) < 0)
                    ERR("close reader");
                if (close(tmp_sender[1]) < 0)
                    ERR("close sender");

                child_work(tmp_sender[0], tmp_reader[1], M);

                if (close(tmp_reader[1]) < 0)
                    ERR("close reader");
                if (close(tmp_sender[0]) < 0)
                    ERR("close sender");
                exit(EXIT_SUCCESS);
            default:
                if (close(tmp_reader[1]) < 0)
                    ERR("close reader");
                if (close(tmp_sender[0]) < 0)
                    ERR("close sender");
                writers[i] = tmp_sender[1];
                readers[i] = tmp_reader[0];
        }
    }

    parent_work(processes,writers,readers,M,N);

    for (int i=0; i<N; i++) {
        if (waitpid(processes[i],NULL,0) < 0)
            ERR("waitpid");
    }
    exit(EXIT_SUCCESS);


}