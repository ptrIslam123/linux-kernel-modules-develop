#include <linux/module.h>       /* Needed by all modules */
#include <linux/kernel.h>       /* Needed for KERN_INFO */
#include <linux/types.h>
#include <linux/init.h>         /* Needed for the macros */
#include <linux/version.h>		// this header contains macros LINUX_CODE_VERSION
#include <asm/current.h>        // this header contains macros "current" - return current process (struct task_struct)
#include <linux/sched.h>        // this header contains defenition of struct task_struct
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("I.Kardanov");
MODULE_DESCRIPTION("This is example of simple linux modeule that just print hello world");
MODULE_VERSION("0.01");

enum ModuleErrorCode {
	ModuleErrorCode_BadAllocChrNumberRegion = 1,
	ModuleErrorCode_BadAllocChrDeviceStruct,
	ModuleErrorCode_RegisterModuleFailed,
	ModuleErrorCode_BadAllocDeviceClass
};

static const char moduleName[] = "my_chr_dev_module";
static const char deviceClassName[] = "my_chr_dev_class";
static const char deviceName[] = "my_chr_dev";
static dev_t devNumber = {0};
static struct cdev* chrDev = NULL;
static struct class* deviceClass = NULL;
static unsigned int devMajor = 0;
static unsigned int deviceCount = 3;
static struct file_operations fileOprs;

static int __init hello_start(void) {
	if (alloc_chrdev_region(&devNumber, 0, deviceCount, moduleName) < 0) {
		printk(KERN_INFO "The module=%s: Can`t allocate number gegions for character devices\n", moduleName);
		return -ModuleErrorCode_BadAllocChrNumberRegion;
	}

	devMajor = MAJOR(devNumber);
	chrDev = cdev_alloc();
	if (chrDev == NULL) {
		printk(KERN_INFO "The module=%s: Can`t allocate character device\n", moduleName);
		return -ModuleErrorCode_BadAllocChrDeviceStruct;
	}

	cdev_init(chrDev, &fileOprs);
	chrDev->owner = THIS_MODULE;


	if (cdev_add(chrDev, devNumber, deviceCount) < 0) {
		printk(KERN_INFO "The module=%s: Can`t register our character module\n", moduleName);
		return -ModuleErrorCode_RegisterModuleFailed;
	}

	// this is used to create a struct class that can then be used in calls to device_create()
	// struct class is a higher-level view of device that abstract out low-level impldetails
	deviceClass = class_create(THIS_MODULE, deviceClassName);
	if (deviceClass == NULL) {
		printk(KERN_INFO "The module=%s: Can`t create device class with name=%s\n", moduleName, deviceClassName);
		return -ModuleErrorCode_BadAllocDeviceClass;
	}

	unsigned int i = 0;
	for (; i < deviceCount; ++i) {
		struct device* deviceStruct = NULL;
		devNumber = MKDEV(devMajor, i);

		// device_create() - creates a device and register it with sysfs.
		// At the lowest level, every device in a Linux system is represented by an instance of struct device.
		// This struct contains the information that the device model core needs to model the system 
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 26)
		// signature: 
		// struct device* device_create(struct class* cls, struct device* parent, dev_t devt, const char* fmt, ...);
		deviceStruct = device_create(deviceClass, NULL, devNumber, "%s_%d", deviceName, i);
#else
		// signature:
		// struct device* device_create(struct class* cls, struct device* parent, dev_t devt, void* drvdata, const char* fmt, ...);
		deviceStruct = device_create(deviceClass, NULL, devNumber, NULL, "%s_%d", deviceName, i);
#endif
		if (deviceStruct == NULL) {
			printk(KERN_INFO "The module=%s: Can`t create and gerister characted device={name=%s, major=%d, minor=%d}\n", 
					moduleName, deviceName, devMajor, i);
		} else {
			printk(KERN_INFO "The module=%s: Created and geristered characted device={name=%s, major=%d, minor=%d}\n", 
					moduleName, deviceName, devMajor, i);
		}
	}

	printk(KERN_INFO "The module=%s: Created devices with number={%d, 0-%d} and register our module", 
			moduleName, devMajor, i);
	return 0;
}

static void __exit hello_end(void) {
	unsigned int i = 0;
	for (; i < deviceCount; ++i) {
		devNumber = MKDEV(devMajor, i);
		device_destroy(deviceClass, devNumber);
	}

	class_destroy(deviceClass);
	cdev_del(chrDev);
	devNumber = MKDEV(devMajor, 0);
	unregister_chrdev_region(devNumber, deviceCount);
	
	printk(KERN_INFO "The module=%s: Clear allocated structes and unregister characted devices", 
			moduleName);
}

module_init(hello_start);
module_exit(hello_end);
