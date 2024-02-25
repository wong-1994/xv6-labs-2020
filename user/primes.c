#include "kernel/types.h"
#include "user/user.h"

static void 
pipeprimes(int *fd)
{
    close(fd[1]);
    int n;
    if (read(fd[0], &n, sizeof(n)) <= 0) {
        return;
    }
    int prime = n;
    printf("prime %d\n", prime);

    int p[2];
    pipe(p);
    if (fork() == 0) {
        pipeprimes(p);
    } else {
        for (;;) {
            if (read(fd[0], &n, sizeof(n)) <= 0) {
                break;
            }
            if (n % prime != 0) {
                write(p[1], &n, sizeof(n));
            }
        }
        close(p[1]);
        wait(0);
    }
    return;
}

int
main(int argc, char *argv[])
{
    if (argc != 1) {
        printf("Wrong Argument Numbers!");
        exit(1);
    }

    int p[2];
    pipe(p);

    if (fork() == 0) {
        pipeprimes(p);
    } else {
        close(p[0]);
        for (int i = 2; i <= 35; ++i) {
            write(p[1], &i, sizeof(i));
        }
        close(p[1]);
        wait(0);        
    }
    exit(0);
}