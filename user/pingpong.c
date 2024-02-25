#include "kernel/types.h"
#include "user/user.h"

int 
main(int argc, char *argv[]) 
{
    if (argc > 1) {
        printf("PingPong takes no argument!");
        exit(1);
    }
    
    int pF2C[2]; /* pipe for child to read and father to write file descripter */
    int pC2F[2]; /* pipe for father to read and child write file descripter */
    pipe(pF2C);
    pipe(pC2F);

    char buff[2];

    if (fork() == 0) {
        /* child process */
        read(pF2C[0], buff, 1);
        printf("%d: received ping\n", getpid());
        write(pC2F[1], buff, 1);
        close(pC2F[1]);
        close(pF2C[0]);
    } else {
        /* father process */
        write(pF2C[1], buff, 1);
        read(pC2F[0], buff, 1);
        printf("%d: received pong\n", getpid());
        close(pF2C[1]);
        close(pC2F[0]);
    }

    exit(0);
}