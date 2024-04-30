#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/sched.h>
#include <asm/io.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/poll.h>

#include "my_ioctl.h"

#define DEFAULT_DEVICE_BUFFER_SIZE (PAGE_SIZE * 1)
#define DEVICE_COUNT (1)
#define DEVICE_BASE_MINOR (0)
#define DRIVER_NAME ("my_simple_lifo_char_dev")
#define DEVICE_NAME ("my_simple_lifo_char_dev")
#define DEVICE_CLASS_NAME "lifo_char_dev"

#define POLL_DEVICE_IS_READY_FOR_READING (0)
#define POLL_DEVICE_IS_NOT_READY_FOR_READING (1)

/**
 * @brief This structure represents lifo character linux device
*/
typedef struct char_device {
    char* device_buffer;    // device mem buffer start
    int device_buffer_size; // Write size
    int device_buffer_capacity; // Device physical mem capacity
    wait_queue_head_t* wait_queue; // For blocking I/O and polling 
    spinlock_t *lock;   // Sync primitive
} char_device_t; 

static dev_t char_dev_num; // Global variable for the first device number
static struct cdev char_dev; // Global variable for the character device structure
static struct class *char_dev_class; // Global variable for the device class

static int device_open(struct inode* node, struct file* filp) {
    printk(KERN_INFO "Device opened\n");

    char_device_t* char_device = (char_device_t*)kzalloc(sizeof(char_device_t) * 1, GFP_KERNEL);
    if (!char_device) {
        printk(KERN_ERR "Could no allocate the char_device structure\n");
        return -ENOMEM;
    }

    // Allocate and initialize the memory for inner device memory buffer
    char_device->device_buffer_size = 0;
    char_device->device_buffer_capacity = DEFAULT_DEVICE_BUFFER_SIZE;
    char_device->device_buffer = kzalloc(char_device->device_buffer_capacity, GFP_KERNEL);
    if (!char_device->device_buffer) {
        kfree(char_device);
        printk(KERN_ERR "Could not init the allocated char_device structure\n");
        return -ENOMEM;
    }

    // Allocate and initialize the memory for device wait queue
    char_device->wait_queue = (wait_queue_head_t*)kzalloc(sizeof(wait_queue_head_t), GFP_KERNEL);
    if (!char_device->wait_queue) {
        printk(KERN_ERR "Could not allocate and init the device wait queue structure\n");
        return -ENOMEM;
    } else {
        init_waitqueue_head(char_device->wait_queue);
    }

    // Allocate and initialize the memory for device  wait queue spinlock
    char_device->lock = (spinlock_t*)kzalloc(sizeof(spinlock_t), GFP_KERNEL);
    if (!char_device->lock) {
        printk(KERN_ERR "Could not allocate and init the device wait queue spinlock\n");
        return -ENOMEM;
    } else {
        spin_lock_init(char_device->lock);
    }

    printk(KERN_INFO "Allocated charater device");
    filp->private_data = char_device;
    return 0;
}

static int device_release(struct inode* node, struct file* filp) {
    printk(KERN_INFO "Device closed\n");

    char_device_t* char_device = (char_device_t*)filp->private_data;
    kfree(char_device->device_buffer);
    kfree(char_device->wait_queue);
    kfree(char_device->lock);
    kfree(char_device);
    return 0;
}

/**
 * @brief Impl of reading operation for my lifo character linux device.
 * @details If the device mem buffer is empty then a reader process will be blocked until wrier write some bytes data in the device buffer.
 * @return read number bytes - success, otherwise - error code.
*/
static ssize_t device_read(struct file* filp, char __user * buffer, size_t size, loff_t* /*offset*/) {
    char_device_t* char_device = (char_device_t*)filp->private_data;
    spin_lock(char_device->lock); // Lock the device 

    int offset = char_device->device_buffer_size;

    printk(KERN_INFO "Device read(dev_buff_size=%d, req_size=%d, req_offset=%d)\n", 
            offset, size, offset);

    if (char_device->device_buffer_size == 0) {
        // Waiting for a writer
        printk(KERN_INFO "This task_struct(name=%s, pid=%i) will be waiting until writer write the device buffer %d bytes\n", 
                current->comm, current->pid, size);

        spin_unlock(char_device->lock); // We have to unlock before waiting for the wake up event

        wait_event_interruptible(*char_device->wait_queue, char_device->device_buffer_size > 0);
    
        spin_lock(char_device->lock); // we have to lock again 

        printk(KERN_INFO "This task_struct(name=%s, pid=%i) wake up and we are ready for reading %d bytes\n", 
                current->comm, current->pid, size);
    }

    const size_t read_size = (size > char_device->device_buffer_size ? 
            char_device->device_buffer_size : size);

    // Because this task_struct could wait until a writer write requested bytes and offset could be changed
    offset = char_device->device_buffer_size;
    if (copy_to_user(buffer, char_device->device_buffer + char_device->device_buffer_size - read_size, read_size) < 0) {
        printk(KERN_ERR "Read error occurred: permission(Attempt to use invalid user memory)\n");
        spin_unlock(char_device->lock); // Unlock device
        return -EPERM;
    }

    char_device->device_buffer_size -= read_size;

    spin_unlock(char_device->lock); // Unlock device

    printk(KERN_INFO "Pop %d bytes(real req_size=%d bytes) from the device: dev_buff_size=%d\n", 
            read_size, size, char_device->device_buffer_size);
    return read_size;
}

/**
 * @brief Impl of reading operation for my lifo character linux device.
 * @return write number bytes - success, otherwise - error code.
*/
static ssize_t device_write(struct file* filp, const char __user * buffer, size_t size, loff_t* /*offset*/) {
    char_device_t* char_device = (char_device_t*)filp->private_data;
    spin_lock(char_device->lock); // Lock device

    const int offset = char_device->device_buffer_size;

    printk(KERN_INFO "Device write operation(dev_buff_size=%d, req_size=%d, req_offset=%d)\n", 
            offset, size, offset);

    if (offset + size > char_device->device_buffer_capacity) {
        printk(KERN_ERR "Write error occurred: attempt to write out of device buffer: "
            "(offset=%d) + (size=%d) = total_write_req_size=%d\n", offset, size, offset + size);
        spin_unlock(char_device->lock); // Unlock device
        return -ENOMEM;
    }

    if (copy_from_user(char_device->device_buffer + offset, buffer, size) < 0) {
        printk(KERN_ERR "Read error occurred: permission error(Attempt to use invalid user memory)\n");
        spin_unlock(char_device->lock); // Unlock device
        return -EPERM;
    }

    char_device->device_buffer_size += size;

    // Wake up any task_struct waiting for the writer
    wake_up_interruptible(char_device->wait_queue);

    spin_unlock(char_device->lock); // Unlock device

    printk(KERN_INFO "Push %d bytes in the device: the dev_buff_size=%d\n", size, char_device->device_buffer_size);
    return size;
}

static long device_ioctl(struct file* filp, unsigned int cmd, unsigned long arg)
{
    printk(KERN_INFO "Device ioctl operation\n");

    if ((_IOC_TYPE(cmd) != MY_IOC_MAGIC)) {
		return -EINVAL;
	}

    switch (cmd) {
        case MY_LIFO_CHAR_DEV_GET_MODULE_INFO: {
            module_info_struct_t module_info = {0};
            sprintf(module_info.name, DEVICE_NAME);
            sprintf(module_info.version, "1.0.0");
            sprintf(module_info.author, "Kardanov.I");
            sprintf(module_info.description, "A simple lifo character device");
            if (copy_to_user((void*)arg, (void*)&module_info, sizeof(module_info)) < 0) {
				return -ENOMEM;
			}
            break;
        }
        case MY_LIFO_CHAR_DEV_GET_DEV_BUF_SIZE: {
            char_device_t* char_device = (char_device_t*)filp->private_data;
            spin_lock(char_device->lock); // Lock device
            
            if (copy_to_user((void*)arg, (void*)&(char_device->device_buffer_size), sizeof(char_device->device_buffer_size)) < 0) {
                spin_unlock(char_device->lock); // Unlock device
				return -ENOMEM;
			}

            spin_unlock(char_device->lock); // Unlock device
            break;
        }
        default: {
            printk(KERN_INFO "Invalid cmd=%d\n", cmd);
            return -EINVAL;
        }
    }

    return 0;
}

static int device_mmap(struct file *filp, struct vm_area_struct *vma)
{
    printk(KERN_INFO "Device mmap operation\n");

    char_device_t* char_device = (char_device_t*)filp->private_data;
    spin_lock(char_device->lock); // Lock device

    if (vma->vm_flags & VM_READ) {
        printk(KERN_INFO "Attempt to map the memory for reading\n");
    }

    if (vma->vm_flags & VM_WRITE) {
        printk(KERN_INFO "Attempt to map the memory for writing\n");
    }

    int status = remap_pfn_range(
            vma, 
            vma->vm_start, 
            virt_to_phys(char_device->device_buffer) >> PAGE_SHIFT, // mapped character device mem buffer (!TODO: this impl doesn`t take into account user vma offset) 
            char_device->device_buffer_capacity, 
            vma->vm_page_prot
    );
    if (status) {
        printk(KERN_INFO "Failed to remap VMA\n");
        spin_unlock(char_device->lock); // Unlock device
        return -EFAULT;
    }

    spin_unlock(char_device->lock); // Unlock device
	return 0;
}

static unsigned int device_poll(struct file* filp, struct poll_table_struct* wait_table)
{
    printk(KERN_INFO "Device poll operation\n");

    char_device_t* char_device = (char_device_t*)filp->private_data;
    spin_lock(char_device->lock); // Lock device

    poll_wait(filp, char_device->wait_queue, wait_table);

    unsigned int mask = 0;
    if (char_device->device_buffer_size > 0) { 
        // If device`s lifo is not empty than a reader can read from this device without blocking
        mask |= (POLLIN | POLLRDNORM);
        printk(KERN_INFO "POLL: The device is ready for reading\n");
    }

    spin_unlock(char_device->lock); // Unlock device
    return mask;
}

static const struct file_operations char_file_ops = {
    .owner = THIS_MODULE,
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .release = device_release,
    .mmap = device_mmap,
    .poll = device_poll,
    .unlocked_ioctl = device_ioctl,
};

static int __init init_module_device(void) {
    printk(KERN_INFO "Start initialization and registration of character lifo device module=%s\n", DRIVER_NAME);

    int ret;

    //! int alloc_chrdev_region(dev_t * dev, unsigned baseminor, unsigned count, const char * name); - Allocate device numbers dynamically
    ret = alloc_chrdev_region(&char_dev_num, DEVICE_BASE_MINOR, DEVICE_COUNT, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ALERT "Failed to allocate device number\n");
        return ret;
    }

    //! void cdev_init(struct cdev *, const struct file_operations *); - Initialize the character device structure
    cdev_init(&char_dev, &char_file_ops);

    //! int cdev_add(struct cdev *, dev_t, unsigned); - Add the character device to the system
    ret = cdev_add(&char_dev, char_dev_num, DEVICE_COUNT);
    if (ret < 0) {
        unregister_chrdev_region(DEVICE_BASE_MINOR, DEVICE_COUNT);
        printk(KERN_ALERT "Failed to add device\n");
        return ret;
    }

    //! #define class_create(owner, name) - Create a class
    char_dev_class = class_create(THIS_MODULE, DEVICE_CLASS_NAME);
    if (IS_ERR(char_dev_class)) {
        cdev_del(&char_dev);
        unregister_chrdev_region(DEVICE_BASE_MINOR, DEVICE_COUNT);
        printk(KERN_ALERT "Failed to create class\n");
        return PTR_ERR(char_dev_class);
    }

    //! struct device *device_create(struct class *cls, struct device *parent, dev_t devt, void *drvdata, const char *fmt, ...); - Create a device node
    device_create(char_dev_class, NULL, char_dev_num, NULL, DEVICE_NAME);

    printk(KERN_INFO "Device registered\n");
    return 0;
}

static void __exit deinit_module_device(void) {
    //! void device_destroy(struct class *cls, dev_t devt); - Destroy a device node
    device_destroy(char_dev_class, char_dev_num);
    //! void class_destroy(struct class *cls); - Destroy a class
    class_destroy(char_dev_class);
    //! void cdev_del(struct cdev *);
    cdev_del(&char_dev);
    //! void unregister_chrdev_region(dev_t from, unsigned count); - unregister the character device from system
    unregister_chrdev_region(char_dev_num, DEVICE_COUNT);

    printk(KERN_INFO "Device unregistered\n");
}

module_init(init_module_device);
module_exit(deinit_module_device);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kardanov.I");
MODULE_DESCRIPTION("A simple lifo character device");