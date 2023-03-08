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

MODULE_LICENSE("GPL");
MODULE_AUTHOR("I.Kardanov");
MODULE_DESCRIPTION("This is example of simple character linux modeule that");
MODULE_VERSION("0.01");

//! device name string
static const char deviceName[] = "schrdev";
//! device number = {major, minor}
static dev_t device = {0};
//! character device struct pointer
static struct cdev* cdevice;
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
        printk(KERN_INFO "We try open more when one device=%s\n", deviceName);
        return -EBUSY;
    }

    printk(KERN_INFO "call open for module=%s\n", deviceName);
    return 0;
}

//! our @release[close()] function implementation
int schrdevRelease(struct inode* node, struct file* flipe) {
    if (openedCount > 0) {
        --openedCount;
    }

    printk(KERN_INFO "call release for module=%s\n", deviceName);
    return 0;
}

//! our @read function implementation
ssize_t schrdevRead(struct file* flipe, char __user* buffer, size_t size, loff_t* offset) {
    printk(KERN_INFO "Attempt to read from device=%s: userbuff=%s\tdevicebuff=%s\n", 
            deviceName, buffer, devBuffer);

    const size_t copySize = size > devBufferSize ? devBufferSize : size;
    if (copy_to_user(buffer, devBuffer, copySize) < 0) {
        printk(KERN_INFO "Attempt to copy to invalid user memory buffer\n");
        return -EACCES;
    }

    printk(KERN_INFO "Wrote bytes=%ld to user space\n", copySize);
    return copySize;
}

//! our @write function implementation
ssize_t schrdevWrite(struct file* flipe, const char __user* buffer, size_t size, loff_t* offset) {
    printk(KERN_INFO "Attempt to write to device=%s: userbuff=%s\tdevicebuff=%s\n", 
            deviceName, buffer, devBuffer);

    const size_t copySize = size > devBufferCapacity ? devBufferCapacity : size;
    if (copy_from_user(devBuffer, buffer, copySize) < 0) {
        printk(KERN_INFO "Attempt to copy from invalid user memory buffer\n");
        return -EACCES;
    }

    printk(KERN_INFO "Read bytes=%ld from user space\n", copySize);
    devBufferSize = copySize;
    return copySize; 
}

//! declaration and initialization of struct what conatins file operations table for our device 
static struct file_operations fileOprs = {
    .owner = THIS_MODULE,                   //! set pointer on our module struct
    .open = schrdevOpen,
    .release = schrdevRelease,
    .read = schrdevRead,
    .write = schrdevWrite
};

static int __init moduleLoad(void) {
    printk(KERN_INFO "Load simple character linux module(%s): the process name=\"%s\" and pid=%i\n",
            deviceName, current->comm, current->pid);

    // allocate device number for this module
    if (alloc_chrdev_region(&device, 0, 1, deviceName) != 0) {
        printk(KERN_INFO "Can`t allocate character device number for module=%s\n", deviceName);
        return -EBUSY;
    }
    
    // allocate character device struct
    cdevice = cdev_alloc();
    if (!cdevice) {
        printk(KERN_INFO "Can`t allocate character device struct\n");
        return -ENOMEM;
    }

    // initialize our new character device. Set our file operations table to this cdev struct
    cdev_init(cdevice, &fileOprs);
    // Set owner field. this field contains pointer to moduel struct what represent our module
    cdevice->owner = THIS_MODULE;

    // register our device module. 
    // This call notify kernel what all device with @major number shold be handl our file operations table.
    // This mead wthat we shoold finished our loading befare call this functtion.
    if (cdev_add(cdevice, device, 1) != 0) {
        printk(KERN_INFO "Can`t add new character device to kernel");
        return -EBUSY;
    }

    // allocation memory buffer for our device and set the buffer to 0
    devBuffer = kzalloc(devBufferCapacity, GFP_KERNEL);
    if (!devBuffer) {
        printk(KERN_INFO "Can`t allocate memory buffer for our device==%s", deviceName);
        return -ENOMEM;
    }

    printk(KERN_INFO "The module: {name=%s, major=%d, minor=%d} was successful registered in system kernel\n", 
            deviceName, MAJOR(device), MINOR(device));
    return 0;
}

static void __exit moduleUnload(void) {
    // remove our character device from system kernel 
    cdev_del(cdevice);
    // unregiister allacaited device number for this module
    unregister_chrdev_region(device, 1);
    // deallocate memory buffer of our device
    kfree(devBuffer);

    printk(KERN_INFO "Unload simple character linux module: {name=%s, major=%d, minor=%d}\n", 
            deviceName, MAJOR(device), MINOR(device));
}

module_init(moduleLoad);
module_exit(moduleUnload);