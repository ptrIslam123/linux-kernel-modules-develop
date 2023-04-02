#include <linux/module.h>       /* Needed by all modules */
#include <linux/kernel.h>       /* Needed for KERN_INFO */
#include <linux/init.h>         /* Needed for the macros */

#include <asm/current.h>        // this header contains macros "current" - return current process (struct task_struct)
#include <linux/sched.h>        // this header contains defenition of struct task_struct
#include <linux/types.h>        // this header contains dev_t and their functions
#include  <linux/errno.h>       // this header contains error codes
#include <linux/fs.h>           // this header contains declaration of functions for getting device number
#include <linux/cdev.h>         // this header contains declaration of functions for creatting  cvde_t struct
#include <linux/slab.h>         // this header contains declaration of functions for work with (virtual)kernel memory
#include <linux/uaccess.h>      // this hrader contains declaration of functions to copy from user and to user space
#include <linux/string.h>

#include "my_ioctl.h"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("I.Kardanov");
MODULE_DESCRIPTION("This is example of simple character linux modeule that");
MODULE_VERSION("0.01");

//! module version string
static const char moduleVersion[] = "0.01";
//! module name string
static const char moduleName[] = "my_schr_module";
//! device name string
static const char deviceName[] = "my_schr_dev";
//! device number = {major, minor}
static dev_t devNumber = {0};
//! character device struct pointer
static struct cdev* chrDevice;
//! count of opened device
static unsigned char openedCount = 0;
//! device buffer
static char* devBuffer;
//! device buffer capacity
static unsigned int devBufferCapacity = 256;
//! device buffer size 
static unsigned int devBufferSize = 0;

//! our @open function implementation
int schrdevOpen(struct inode* node, struct file*flipe) {
    ++openedCount;
    if (openedCount > 1) {
        printk(KERN_INFO "The module=%s: We try open more when one device=%s\n", 
				moduleName, deviceName);
        return -EBUSY;
    }

    printk(KERN_INFO "The module=%s: Call open device=%s\n", 
			moduleName, deviceName);
    return 0;
}

//! our @release[close()] function implementation
int schrdevRelease(struct inode* node, struct file* flipe) {
    if (openedCount > 0) {
        --openedCount;
    }

    printk(KERN_INFO "The module=%s: Call release device=%s\n", 
			moduleName, deviceName);
    return 0;
}

//! our @read function implementation
ssize_t schrdevRead(struct file* flipe, char __user* buffer, size_t size, loff_t* offset) {
    printk(KERN_INFO "The module=%s: Attempt to read from device=%s: userbuff=%s\tdevicebuff=%s\n", 
            moduleName, deviceName, buffer, devBuffer);

    const size_t copySize = size > devBufferSize ? devBufferSize : size;
    if (copy_to_user(buffer, devBuffer, copySize) < 0) {
        printk(KERN_INFO "The module=%s: Attempt to copy to invalid user memory buffer\n", 
				moduleName);
        return -EACCES;
    }

    printk(KERN_INFO "The module=%s: Wrote bytes=%ld to user space\n", 
			moduleName, copySize);
    return copySize;
}

//! our @write function implementation
ssize_t schrdevWrite(struct file* flipe, const char __user* buffer, size_t size, loff_t* offset) {
    printk(KERN_INFO "The module=%s: Attempt to write to device=%s: userbuff=%s\tdevicebuff=%s\n", 
            moduleName, deviceName, buffer, devBuffer);

    const size_t copySize = size > devBufferCapacity ? devBufferCapacity : size;
    if (copy_from_user(devBuffer, buffer, copySize) < 0) {
        printk(KERN_INFO "The module=%s: Attempt to copy from invalid user memory buffer\n", 
				moduleName);
        return -EACCES;
    }

    printk(KERN_INFO "The moduel=%s: Read bytes=%ld from user space\n", 
			moduleName, copySize);
    devBufferSize = copySize;
    return copySize; 
}

//! our @ioctl function implementation
static long schrdevIoctl(struct file* filp, unsigned int cmd, unsigned long arg) {
	printk(KERN_INFO "The module=%s: Attempt to do ioctl operation for device=%s\n", 
            moduleName, deviceName);

	if ((_IOC_TYPE(cmd) != MY_IOC_MAGIC)) {
		return -ENOTTY;
	}	

	switch(cmd) {
		case MY_IOCTL_GET_MODULE_INFO: {
			printk(KERN_INFO "The module=%s: IOCT_GET_MODULE_INFO", deviceName);
			
			MyIOctlModuleInfo moduleInfo = {0};
			strcpy(moduleInfo.version, moduleVersion);
			strcpy(moduleInfo.name, deviceName);

			if (copy_to_user((void*)arg, &moduleInfo, sizeof(moduleInfo)) < 0) {
				printk(KERN_INFO "The module=%s: ioctl cmd=%d was failed", deviceName, cmd);
				return -ENOTTY;
			}

			printk(KERN_INFO "The module=%s: ioctl cmd=%d was successful. Returned module info{version=%s, name=%s}\n",
					deviceName, cmd, moduleVersion, deviceName);
			break;						   
		}
		default: {
			printk(KERN_INFO "The module=%s: IOCT error. Unknnow cmd=%d\n", deviceName, cmd);
			return -ENOTTY;
		}
	}
	return 0;
}

//! declaration and initialization of struct what conatins file operations table for our device 
static struct file_operations fileOprs = {
    .owner = THIS_MODULE,                   //! set pointer on our module struct
    .open = schrdevOpen,
    .release = schrdevRelease,
    .read = schrdevRead,
    .write = schrdevWrite,
	.unlocked_ioctl = schrdevIoctl
};

static int __init moduleLoad(void) {
    printk(KERN_INFO "The module=%s: Load simple character module: the proces pid=%i\n",
            moduleName, deviceName, current->pid);

	// allocation memory buffer for our device and set the buffer to 0
    devBuffer = kzalloc(devBufferCapacity, GFP_KERNEL);
    if (!devBuffer) {
        printk(KERN_INFO "The module=%s: Can`t allocate memory buffer for our device==%s", 
				moduleName, deviceName);
        return -ENOMEM;
    }

    // allocate device number for this module
	// 0, 1 - is range characted devices [MAJOR, 0-1]. Now we just allocate one device with minor number=1
    if (alloc_chrdev_region(&devNumber, 0, 1, deviceName) != 0) {
        printk(KERN_INFO "The module=%s: Can`t allocate character device number for module=%s\n", 
				moduleName, deviceName);
        return -EBUSY;
    }
    
    // allocate character device struct
    chrDevice = cdev_alloc();
    if (!chrDevice) {
        printk(KERN_INFO "The module=%s: Can`t allocate character device struct\n", moduleName);
        return -ENOMEM;
    }

    // initialize our new character device. Set our file operations table to this cdev struct
    cdev_init(chrDevice, &fileOprs);
    // Set owner field. this field contains pointer to moduel struct what represent our module
    chrDevice->owner = THIS_MODULE;

    // register our device module. 
    // This call notify kernel what all device with @major number shold be handl our file operations table.
    // This mead wthat we shoold finished our loading befare call this functtion.
    if (cdev_add(chrDevice, devNumber, 1) != 0) {
        printk(KERN_INFO "Can`t add new character device to kernel");
        return -EBUSY;
    }

    printk(KERN_INFO "The module=%s: successful register simple characted module{name=%s, major=%d, minor=%d}\n", 
            moduleName, deviceName, MAJOR(devNumber), MINOR(devNumber));
    return 0;
}

static void __exit moduleUnload(void) {
    // remove our character device from system kernel 
    cdev_del(chrDevice);
    // unregiister allacaited device number for this module
	// 1 - minor number of one our allocated device
    unregister_chrdev_region(devNumber, 1);
    // deallocate memory buffer of our device
    kfree(devBuffer);

    printk(KERN_INFO "The module=%s: Unload simple character linux module{dev_name%s, =major=%d, minor=%d}\n", 
            moduleName, deviceName, MAJOR(devNumber), MINOR(devNumber));
}

module_init(moduleLoad);
module_exit(moduleUnload);
