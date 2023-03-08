#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Pass wrong args: %s <file-path> <some-message>\n", argv[0]);
        return -1;
    }

    const char* filePath = argv[1];
    const int fd = open(filePath, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Can`t open file=%s. return exit code=%d\n", filePath, fd);
        return -1;
    }

    printf("the file=%s was succcessful opened, descriptor number=%d\n", filePath, fd);
    const char* msg = argv[2];
    ssize_t result = write(fd, msg, strlen(msg));
    if (result < 0) {
        fprintf(stderr, "Can`t write to file=%s, return exit code=%d\n", filePath, result);
        return -1;
    }

    printf("Write was successful. We wrote bytes=%d, message=\"%s\"\n", strlen(msg), msg);

    char buffer[256] = {0};
    result = read(fd, buffer, 256);
    if (result < 0) {
        fprintf(stderr, "Can`t read from file=%s, return exit code=%d\n");
    }

    printf("Read was successful. We read bytes=%d, message=\"%s\"\n", result, buffer);

    if (close(fd) < 0) {
        fprintf(stderr, "Can`t close opened file=%s\n", filePath);
        return -1;
    }    
    return printf("the file=%s was successful closed\n", filePath);;
}