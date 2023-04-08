#include <linux/module.h>       /* Needed by all modules */
#include <linux/kernel.h>       /* Needed for KERN_INFO */
#include <linux/init.h>         /* Needed for the macros */

#include <asm/current.h>        // this header contains macros "current" - return current process (struct task_struct)
#include <linux/sched.h>        // this header contains defenition of struct task_struct
#include <linux/types.h>        // this header contains dev_t and their functions
#include <linux/errno.h>		// this header contains error codes
#include <linux/fs.h>           // this header contains declaration of functions for getting device number
#include <linux/cdev.h>         // this header contains declaration of functions for creatting  cvde_t struct
#include <linux/slab.h>         // this header contains declaration of functions for work with (virtual)kernel memory
#include <linux/uaccess.h>      // this header contains declaration of functions to copy from user and to user space
#include <linux/string.h>
#include <linux/device.h>		// this header conatins declaration of function and structs to work ith devices
#include <linux/version.h>		// this header conatins declaration of macros to determine linux code version
#include <linux/wait.h>			// this header for work with kernel work queue. To reschedule task executing

MODULE_LICENSE("GPL");
MODULE_AUTHOR("I.Kardanov");
MODULE_DESCRIPTION("This is example of simple character linux module");
MODULE_VERSION("0.01");

//! module version string
static const char moduleVersion[] = "0.01";
//! module name string
static const char moduleName[] = "my_sblock_io_module";
//! device name string
static const char deviceName[] = "my_sblock_io_dev";
//! device number = {major, minor}
static dev_t devNumber = {0};
//! character device struct pointer
static struct cdev* chrDevice;
//! device class struct pointer
static struct class* deviceClass;
//! device struct pointer
static struct device* device;
//! device buffer
static char* devBuffer;
//! device buffer capacity
static unsigned int devBufferCapacity = 256;
//! device buffer size 
static unsigned int devBufferSize = 0;
//! our wait queue. If you want to staticly create and initialize our wait queue, just use DECLARE_WAIT_QUEUE_HEAD(myWaitQueu);
static wait_queue_head_t myWaitQueue;
//! wait queue flag
int waitQueueFlag = 0; 

//! our @open function implementation
int schrdevOpen(struct inode* node, struct file*flipe) {
    printk(KERN_INFO "The module=%s: Call open device=%s\n", 
			moduleName, deviceName);
    return 0;
}

//! our @release[close()] function implementation
int schrdevRelease(struct inode* node, struct file* flipe) {
    printk(KERN_INFO "The module=%s: Call release device=%s\n", 
			moduleName, deviceName);
    return 0;
}

//! our @read function implementation
ssize_t schrdevRead(struct file* flipe, char __user* buffer, size_t size, loff_t* offset) {
    printk(KERN_INFO "The module=%s: Attempt to read from device=%s: userbuff=%s\tdevicebuff=%s\n", 
            moduleName, deviceName, buffer, devBuffer);
	
	printk(KERN_INFO "The process with pid=%d will be wait for a certail period of time until some other process writes\n");

	waitQueueFlag = 0;
	// In this line we say the kernel that we want to sleep. 
	// This macos takes the task_struct instance and move to wait_queue untile conditions is true
	// We can use wait_event or wait_event_interruptible.  Interruptible means that the task couldn`t be interrupted signals. 
	// The first macros(wait_event) don`t allow interrupte the task
	wait_event_interruptible(myWaitQueue, waitQueueFlag != 0);
	
	const size_t copySize = size > devBufferSize ? devBufferSize : size;
	if (copy_to_user(buffer, devBuffer, copySize) < 0) {
		printk(KERN_INFO "ERROR: We can`t copy data to user space");
		return -EACCES;
	}

    return copySize;
}

//! our @write function implementation
ssize_t schrdevWrite(struct file* flipe, const char __user* buffer, size_t size, loff_t* offset) {
    printk(KERN_INFO "The module=%s: Attempt to write to device=%s: userbuff=%s\tdevicebuff=%s\n", 
            moduleName, deviceName, buffer, devBuffer);
	
	const size_t copySize = size > devBufferCapacity ? devBufferCapacity : size;
	if (copy_from_user(devBuffer, buffer, copySize) < 0) {
		printk(KERN_INFO "ERROR: We can`t copy data from user space");
		return -EACCES;
	}
	devBufferSize = copySize;

	waitQueueFlag = 1;
	// this macros just wake up all tasks that wait
	// there are two variant this macros: wake_up and wake_up_interruptible
	wake_up_interruptible(&myWaitQueue);
	printk(KERN_INFO "Another process woke up our");
   	return copySize; 
}

//! declaration and initialization of struct what conatins file operations table for our device 
static struct file_operations fileOprs = {
    .owner = THIS_MODULE,                   //! set pointer on our module struct
    .open = schrdevOpen,
    .release = schrdevRelease,
    .read = schrdevRead,
    .write = schrdevWrite,
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

	// we dynamicly create and initialize our wait queue
	init_waitqueue_head(&myWaitQueue);

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

    // Create a device to /dev directory
    deviceClass = class_create(THIS_MODULE, deviceName);
    if (deviceClass == NULL) {
            printk(KERN_INFO "The module=%s: Can`t create device class with name=%s\n", moduleName, deviceName);
	    return -1;
    }

	const int major = MAJOR(devNumber);
   	const int minor = MINOR(devNumber);
	// device_create() - creates a device and register it with sysfs.
	// At the lowest level, every device in a Linux system is represented by an instance of struct device.
	// This struct contains the information that the device model core needs to model the system 
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 26)
	// signature: 
	// struct device* device_create(struct class* cls, struct device* parent, dev_t devt, const char* fmt, ...);
	device = device_create(deviceClass, NULL, devNumber, "%s_%d", deviceName, minor);
#else
	// signature:
	// struct device* device_create(struct class* cls, struct device* parent, dev_t devt, void* drvdata, const char* fmt, ...);
	device = device_create(deviceClass, NULL, devNumber, NULL, "%s_%d", deviceName, minor);
#endif
	if (device == NULL) {
		printk(KERN_INFO "The module=%s: Can`t create and gerister characted device={name=%s, major=%d, minor=%d}\n", 
				moduleName, deviceName, major, minor);
	} else {
		printk(KERN_INFO "The module=%s: Created and geristered characted device={name=%s, major=%d, minor=%d}\n", 
				moduleName, deviceName, major, minor);
	}

    printk(KERN_INFO "The module=%s: successful register simple characted module{name=%s, major=%d, minor=%d}\n", 
            moduleName, deviceName, major, minor);
	return 0;
}

static void __exit moduleUnload(void) {
    device_destroy(deviceClass, devNumber);
	class_destroy(deviceClass);

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
