#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Error: Sleep takes 1 argument");
        exit(1);
    }

    int time = atoi(argv[1]);
    sleep(time);
    exit(0);
}