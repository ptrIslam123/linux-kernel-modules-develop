#ifndef MY_IOCT_H
#define MY_IOCT_H

#include </usr/include/asm-generic/ioctl.h>

#define IOCTL_MODULE_INFO_VERSION_BUF_MAX_SIZE (256)
#define IOCTL_MODULE_INFO_NAME_BUF_MAX_SIZE (256)
#define IOCTL_MODULE_INFO_AUTHOR_NAME_BUF_MAX_SIZE (256)
#define IOCTL_MODULE_INFO_DESCR_NAME_BUF_MAX_SIZE (256)

typedef struct module_info_struct {
	char version[IOCTL_MODULE_INFO_VERSION_BUF_MAX_SIZE];
	char name[IOCTL_MODULE_INFO_NAME_BUF_MAX_SIZE];
	char author[IOCTL_MODULE_INFO_AUTHOR_NAME_BUF_MAX_SIZE];
	char description[IOCTL_MODULE_INFO_DESCR_NAME_BUF_MAX_SIZE];
} module_info_struct_t;

#define MY_IOC_MAGIC ('h')
#define MY_LIFO_CHAR_DEV_GET_MODULE_INFO (_IOR(MY_IOC_MAGIC, 1, module_info_struct_t))
#define MY_LIFO_CHAR_DEV_GET_DEV_BUF_SIZE (_IOR(MY_IOC_MAGIC, 2, int))

#endif // MY_IOCT_H
