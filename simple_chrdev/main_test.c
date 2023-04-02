#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#include "my_ioctl.h"

void readFrom(int fd) {
	char buff[256] = {0};
	if (read(fd, buff, 256) < 0) {
		fprintf(stderr, "read op ws failed\n");
		exit(-1);
	}

	printf("%s\n", buff);
}

void writeTo(int fd, const char* msg) {
	if (write(fd, msg, strlen(msg)) < 0) {
		fprintf(stderr, "write op was failed\n");
		exit(-1);
	}
}

void ctl(int fd) {
	MyIOctlModuleInfo moduleInfo = {0};

	if (ioctl(fd, MY_IOCTL_GET_MODULE_INFO, &moduleInfo) < 0) {
		fprintf(stderr, "ioctl was failed'\n");
		exit(-1);
	}

	printf("module info={version=%s, name=%s}\n", moduleInfo.version, moduleInfo.name);
}

int main(int argc, char** argv) {
	if (argc == 2 && strcmp(argv[1], "-h") == 0) {
		printf("<file-path> option args...");
		printf("option:  -w=write to file, -r=read from file, -c=ioctl operation");
		printf("args: -w <string-message>, -r, -c");
		return 0;
	}
	
   	if (argc < 3) {
        fprintf(stderr, "Pass wrong args: %s <file-path> <option> <args ...>\n", argv[0]);
        return -1;
    }

    const char* filePath = argv[1];
	const char* opt = argv[2];
    const int fd = open(filePath, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Can`t open file=%s. return exit code=%d\n", filePath, fd);
        return -1;
    }


	if (strcmp(opt, "-r") == 0) {
		readFrom(fd);
	} else if (strcmp(opt, "-w") == 0) {
		writeTo(fd, argv[3]);
	} else if (strcmp(opt, "-c") == 0) {
		ctl(fd);
	}

	printf("file operation=%s was successful\n", argv[2]);

    if (close(fd) < 0) {
        fprintf(stderr, "Can`t close opened file=%s\n", filePath);
        return -1;
    }    
    return printf("the file=%s was successful closed\n", filePath);;
}
