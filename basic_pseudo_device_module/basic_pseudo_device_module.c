#include <linux/module.h>       /* Needed by all modules */
#include <linux/kernel.h>       /* Needed for KERN_INFO */
#include <linux/init.h>         /* Needed for the macros */
#include <linux/fs.h>           /* Needed for struct file_opeaions */
#include <linux/uaccess.h>      /* Needed for copy_to_use, copy_from_use */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("I.Kardanov");
MODULE_DESCRIPTION("This is example of simple linux modeule that imitates character device");
MODULE_VERSION("0.02");

#define DEVICE_BUFF_SIZE 1024

static int majorDevNumber = 0;
static int countOpenedDevice = 0;
static const char* deviceName = "primitive_ch_device";
static const int deviceBuffSize = DEVICE_BUFF_SIZE;
static char deviceBuff[DEVICE_BUFF_SIZE] = {0};

static int deviceOpen(struct inode* in, struct file* flipe) {
    if ((++countOpenedDevice) > 1) {
        printk(KERN_INFO "Opened the new decie=\"%s\"", deviceName);
        return -EBUSY; 
    } else {
        printk(KERN_ALERT "Try opened more than one device=\"%s\"", deviceName);
        return 0;
    }
}

static int deviceRelease(struct inode* in, struct file* flipe) {
    --countOpenedDevice;
    printk(KERN_INFO "Closed a device=\'%s\"", deviceName);
    return 0;
}

static ssize_t deviceRead(struct file* flipe, char* userBuff, size_t len, loff_t* offset) {
    const int size = strlen(deviceBuff);
    if (copy_to_user(userBuff, deviceBuff, size) != 0) {
        printk(KERN_ALERT "Couldn`t write device data to user buffer with invalid address");
        return -1;
    }
    return size;
}

static ssize_t deviceWrite(struct file* flipe, const char* userBuff, size_t len, loff_t* offset) {
    memset(deviceBuff, '\0', deviceBuffSize);
    const int size = (len <= deviceBuffSize) ? len : deviceBuffSize;
    if (copy_from_user(deviceBuff, userBuff, size) != 0) {
        printk(KERN_ALERT "Couldn`t write user data to device buffer from invalid user buffer address");
        return -1;
    }
    return size;
}

static struct file_operations devFileOperts = {
    .read = deviceRead,
    .write = deviceWrite,
    .open = deviceOpen,
    .release = deviceRelease
};

static int __init initModule(void) {
    majorDevNumber = register_chrdev(0, deviceName, &devFileOperts);
    if (majorDevNumber < 0) {
        printk(KERN_ALERT "Couldn`t register our character device=\"%s\" because call \"register_chrdev\" returned with code=%d",
                deviceName, majorDevNumber);
        return majorDevNumber;
    } else {
        printk(KERN_ALERT "Load and registered our device=\"%s\" and major device number=%d was successful",
                deviceName, majorDevNumber);
        return 0;
    }
}

static void __exit cleanModule(void) {
    unregister_chrdev(majorDevNumber, deviceName);
    printk(KERN_ALERT "Unload and unregister our device=\"%s\" and major device number=%d", 
            deviceName, majorDevNumber);
}

module_init(initModule);
module_exit(cleanModule);