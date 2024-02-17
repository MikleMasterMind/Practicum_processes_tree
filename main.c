#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdbool.h>

// macros to use write() and read() more comfortable
#define READ 0
#define WRITE 1

#define MAX(r,x,y) (r = (x > y)? x : y);


int num; // working process number
int amount_child = 0; // amount of this process children
int fd_get; // read from parent
int fd_send; // write to parent
int* fd_get_mas; // read from child
int* fd_send_mas; // write to child
int* nums; // array of process numbers
int amount_num; // amount of numbers

int compare(const void* a, const void* b) {
    return (*(int*)a > *(int*)b);
}

void create_graph() {

    int amount_proc; // amount of processes
    scanf("%d", &amount_proc);

    int fd_child[2]; // child write in this
    int fd_parent[2]; // parent write in this
    int buf = 1; // buffer
    int new_num; // number of born process
    pid_t pid;
    pid_t* pids;

    for (int i = 0; i < amount_proc; ++i) { // read line and create children
        
        new_num = 0;
        setbuf(stdin, NULL); // clean buffer
        scanf("%d", &amount_child);

        fd_get_mas = calloc(amount_child, sizeof(int));
        fd_send_mas = calloc(amount_child, sizeof(int));
        pids = calloc(amount_child, sizeof(int));

        for (int j = 0; j < amount_child; ++j) {
            
            scanf("%d", &new_num);

            pipe(fd_parent);
            pipe(fd_child);

            pid = fork();
            if (pid == -1) { // error
                perror("fork");
                exit(EXIT_FAILURE);
            } else if (pid == 0) { // child

                num = new_num;

                close(fd_parent[WRITE]);
                close(fd_child[READ]);

                fd_get = fd_parent[READ];
                fd_send = fd_child[WRITE];

                free(fd_get_mas); fd_get_mas = NULL;
                free(fd_send_mas); fd_send_mas = NULL;
                free(pids); pids = NULL;

                break;
            } else { // parent 
                close(fd_parent[READ]);
                close(fd_child[WRITE]);

                fd_send_mas[j] = fd_parent[WRITE];
                fd_get_mas[j] = fd_child[READ];
                pids[j] = pid;
            }
        }

        if (num == new_num) { // child case
            read(fd_get, &buf, sizeof(int)); // wait command from parent
        } else { // parent case
            printf("%d", getpid());
            for (int i = 0; i < amount_child; ++i) {
                printf(" %d", pids[i]);
            }
            printf("\n");
            free(pids); pids = NULL;
            break;
        }
    }

    for (int i = 0; i < amount_child; ++i) { // children work deep first
        write(fd_send_mas[i], &buf, sizeof(int)); // child begin reading
        read(fd_get_mas[i], &buf, sizeof(int)); // child finish reading
    }

    if (num != 1) { // next brother-process may work
        write(fd_send, &buf, sizeof(int));
    }

    return;
}

void read_nums() {
    
    int data; // read current number
    int buf = 0;

    if (num != 1) { // wait comand from parent
        read(fd_get, &buf, sizeof(int)); 
    }

    setbuf(stdin, NULL);
    scanf("%d", &amount_num);
    
    nums = calloc(amount_num, sizeof(int));

    for (int i = 0; i < amount_num; ++i) { // scan numbers

        scanf("%d", &data);

        nums[i] = data;
    }

    for (int i = 0; i < amount_child; ++i) { // children work deep first
        write(fd_send_mas[i], &buf, sizeof(int)); // child begin reading
        read(fd_get_mas[i], &buf, sizeof(int)); // child finish reading
    }

    if (num != 1) {  // next brother-process may work
        write(fd_send, &buf, sizeof(int));
    }

    qsort(nums, amount_num, sizeof(int), compare);

    return;
}

void get_count_send() {

    int result = 0; // result to send to parent
    int size[amount_child];
    int max_size;
    int buf;

    for (int i = 0; i < amount_child; ++i) { // close fd we don't need anymore
        close(fd_send_mas[i]);
    }
    free(fd_send_mas); fd_send_mas = NULL;
    close(fd_get);
    
    if (amount_child == 0) { // no children case
        write(fd_send, &amount_num, sizeof(int));
        for (int i = 0; i < amount_num; ++i) {
            write(fd_send, &nums[i], sizeof(int));
        }
    } else { // has chidren case
        read(fd_get_mas[0], &max_size, sizeof(int));
        for (int i = 1; i < amount_child; ++i) { // get how many nubers will be send from every child
            read(fd_get_mas[i], &size[i], sizeof(int)); 
            MAX(max_size, max_size, size[i]);
        }

        MAX(max_size, max_size, amount_num);

        write(fd_send, &max_size, sizeof(int));
        for (int k = 0; k < max_size; ++k) {
            result = 0;
            for (int i = 0; i < amount_child; ++i) { // add nums from children
                if (size[i] != 0) {
                    --size[i];
                    read(fd_get_mas[i], &buf, sizeof(int));
                } else {
                    buf = 0;
                }
                result += buf;
            }
            if (k < amount_num) { // add own num
                result += nums[k];
            } else {
                result += 0;
            }

            if (num != 1) {
                write(fd_send, &result, sizeof(int));
            } else {
                printf("%d ", result);
            } 
        }
    }

    for (int i = 0; i < amount_child; ++i) { // close fd we don't need anymore
        close(fd_get_mas[i]);
    }
    free(fd_get_mas); fd_get_mas = NULL;
    close(fd_send);

    if (num == 1) {
        printf("\n");
    }

    return;
}

int main() {

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        num = 1; // root process
        create_graph();
        read_nums();
        get_count_send();
        while(wait(NULL) != -1);
        printf("Process %d finished\n", getpid());
        exit(0);
    } else {
        wait(NULL);
    }


    return 0;
}