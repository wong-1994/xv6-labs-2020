#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

int
hittarget(const char *path, const char *target)
{
    char *p;
    char buf[512];
    if(strlen(path) + 1 > sizeof buf){
        fprintf(2, "find: path too long\n");
        return 0;
    }
    strcpy(buf, path);

    // Find first character after last slash.
    for(p=buf+strlen(path); p >= buf && *p != '/'; p--)
        ;
    p++;

    return (strcmp(p, target) == 0);
}

void
find(const char *path, const char *target)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if((fd = open(path, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch(st.type){
        case T_FILE:
            if (hittarget(path, target) == 1) {
                printf("%s\n", path);
            }
            break;

        case T_DIR:
            if(strlen(path) + 2 > sizeof buf){
                printf("find: path too long\n");
                break;
            }
            strcpy(buf, path);
            p = buf+strlen(buf);
            *p++ = '/';
            while(read(fd, &de, sizeof(de)) == sizeof(de)){
                if(de.inum == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
                    continue;
                memmove(p, de.name, strlen(de.name));
                p[strlen(de.name)] = 0;
                find(buf, target);
            }
            break;
    }
    close(fd);
    return;
}

int
main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(2, "find: wrong args!");
    }

    char *path = argv[1];
    char *target = argv[2];

    find(path, target);
    exit(0);
}