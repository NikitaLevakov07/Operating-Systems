#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>

int main(int argc, char **argv) {
    if (argc < 4) {
        write(2, "Usage: ./parent <input.txt> <file1.txt> <file2.txt>\n", 49);
        exit(1);
    }

    srand(time(NULL));

    char progpath[1024];
    ssize_t len = readlink("/proc/self/exe", progpath, sizeof(progpath) - 1);
    if (len == -1) {
        write(2, "Error: readlink\n", 16);
        exit(1);
    }

    while (progpath[len] != '/') { --len; }
    progpath[len] = '\0';

    int input_fd = open(argv[1], O_RDONLY);
    if (input_fd == -1) {
        write(2, "Error: open input\n", 18);
        exit(1);
    }

    char *file1 = argv[2];
    char *file2 = argv[3];

    int pipe1[2], pipe2[2];
    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        write(2, "Error: pipe\n", 12);
        exit(1);
    }

    pid_t pid1 = fork();
    if (pid1 == -1) {
        write(2, "Error: fork child1\n", 19);
        exit(1);
    }

    if (pid1 == 0) {
        dup2(pipe1[0], 0);
        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[0]);
        close(pipe2[1]);

        char path[2048];
        snprintf(path, sizeof(path) - 1, "%s/child", progpath);

        char *args[] = {"./child", file1, NULL};
        execv(path, args);
        write(2, "Error: execv child1\n", 20);
        exit(1);
    }

    pid_t pid2 = fork();
    if (pid2 == -1) {
        write(2, "Error: fork child2\n", 19);
        exit(1);
    }

    if (pid2 == 0) {
        dup2(pipe2[0], 0);
        close(pipe1[0]); 
        close(pipe1[1]);
        close(pipe2[0]); 
        close(pipe2[1]);

        char path[2048];
        snprintf(path, sizeof(path) - 1, "%s/child", progpath);

        char *args[] = {"./child", file2, NULL};
        execv(path, args);
        write(2, "Error: execv child2\n", 20);
        exit(1);
    }

    close(pipe1[0]);
    close(pipe2[0]);

    char buf[2048];
    ssize_t bytes;
    int pos = 0;

    while ((bytes = read(input_fd, buf + pos, sizeof(buf) - pos - 1)) > 0) {
        bytes += pos;
        pos = 0;

        for (ssize_t i = 0; i < bytes; i++) {
            if (buf[i] == '\n') {
                int len = i + 1;

                if (rand() % 100 < 80)
                    write(pipe1[1], buf + pos, len - pos);
                else
                    write(pipe2[1], buf + pos, len - pos);

                pos = len;
            }
        }

        if (pos < bytes) {
            int remaining = bytes - pos;
            for(int i = 0; i < remaining; i++)
                buf[i] = buf[pos + i];
            pos = remaining;
        } else  
            pos = 0;
    }

    if (pos > 0) {
    if (rand() % 100 < 80)
        write(pipe1[1], buf, pos);
    else
        write(pipe2[1], buf, pos);
    }

    close(input_fd);
    close(pipe1[1]);
    close(pipe2[1]);
    wait(NULL);
    wait(NULL);
    return 0;
}