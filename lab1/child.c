#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

int is_vowel(char c) {
    return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' || c == 'y' ||
           c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U' || c == 'Y';
}

int main(int argc, char **argv) {
    if (argc < 2) {
        write(2, "Error: no filename\n", 19);
        exit(1);
    }

    int fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        write(2, "Error open\n", 12);
        exit(1);
    }

    char buf[2048];
    ssize_t bytes;
    while ((bytes = read(0, buf, sizeof(buf))) > 0) {
        char out[1024];
        int j = 0;
        for (int i = 0; i < bytes; i++) {
            if (!is_vowel(buf[i])) 
                out[j++] = buf[i];
        }

        write(fd, out, j);
    }

    close(fd);
    return 0;
}