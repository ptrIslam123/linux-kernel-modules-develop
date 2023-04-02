#ifndef MY_IOCT_H
#define MY_IOCT_H

#include </usr/include/asm-generic/ioctl.h>

#define IOCTL_MODULE_INFO_VERSION_BUF_MAX_SIZE 256
#define IOCTL_MODULE_INFO_NAME_BUF_MAX_SIZE 256

typedef struct MyIOctlmoduleInf {
	char version[IOCTL_MODULE_INFO_VERSION_BUF_MAX_SIZE];
	char name[IOCTL_MODULE_INFO_NAME_BUF_MAX_SIZE];
} MyIOctlModuleInfo;

#define MY_IOC_MAGIC ('h')
#define MY_IOCTL_GET_MODULE_INFO (_IOR(MY_IOC_MAGIC, 1, MyIOctlModuleInfo))

#endif // MY_IOCT_H
