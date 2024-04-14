#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>

int main()
{
    int cdevFd = open("/dev/myv4l2-virtual-device.c", O_RDONLY);
    if (cdevFd < 0) {
        fprintf(stderr, "Could not open character device\n");
        return -1;
    }

    int vdevFd = open("/dev/video2", O_RDONLY);
    if (vdevFd < 0) {
        fprintf(stderr, "Could not video device");
        return -1;
    }

    close(cdevFd);
    close(vdevFd);
    return 0;
}
