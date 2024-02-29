#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

int
matchnewline(char cmd[], int m) {
    return (m > 1 && cmd[m - 1] == 'n' && cmd[m - 2] == '\\');
}

void xargs(const int argc, char *argv[], const int i, char buf[][100]) {
    if (fork() == 0) {
        int j;
        char *newargv[argc + i + 1];
        for (j = 0; j < argc - 1; ++j) {
            newargv[j] = argv[j + 1]; 
        }
        for (int k = 0; k <= i; ++k) {
            newargv[j + k] = buf[k];
        }
        newargv[argc + i] = 0;

        exec(newargv[0], newargv);
        exit(0);
    }
    wait(0);
    return;
}

int
main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(2, "xargs: insufficient args!\n");
    }

    int m, n, i;
    char buf[MAXARG][100];

    i = 0;
    m = 0;
    while((n = read(0, buf[i]+m, 1)) > 0) {
        ++m;

        /* read white space, finish write the arg to buf */
        if (m > 0 && buf[i][m - 1] == ' ') {
            buf[i][m - 1] = '\0';
            ++i;
            m = 0;
            continue;
        }

        /* read new line, excute the cmd */
        if (m > 0 && buf[i][m - 1] == '\n') {
            buf[i][m - 1] = '\0';
            xargs(argc, argv, i, buf);
            i = 0;
            m = 0;           
        }

        /* in case new line is entered as two char like '\''n' */
        if (matchnewline(buf[i], m)) {
            buf[i][m - 2] = '\0';
            xargs(argc, argv, i, buf);
            i = 0;
            m = 0;
        }
    }

    /* in case one line cmd entered with no new line char */
    if (n == 0 && m > 0) {
        buf[i][m - 2] = '\0';
        xargs(argc, argv, i, buf);
    }
    exit(0);
}