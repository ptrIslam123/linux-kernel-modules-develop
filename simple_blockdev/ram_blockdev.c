#include <linux/module.h>       /* Needed by all modules */
#include <linux/kernel.h>       /* Needed for KERN_INFO */
#include <linux/init.h>         /* Needed for the macros */

#include <asm/current.h>        // this header contains macros "current" - return current process (struct task_struct)
#include <linux/sched.h>        // this header contains defenition of struct task_struct
#include <linux/types.h>        // this header contains dev_t and their functions
#include <linux/errno.h>        // this header contains error codes
#include <linux/fs.h>           // this header contains declaration of functions for getting device number
#include <linux/blkdev.h>		// this header contains declaration of functions to work with block device in the linux kernel
#include <linux/hdreg.h>		// this header contains declaration of IOCTL cmd HDIO_GETGEO for getting hardware geometry of block device
#include <linux/blk_types.h>	// this header contains declaration of struct hd_geometry
#include <linux/blk-mq.h>		// this header contains declaration of struct blk_mq_tag_set and functions to create and work with blk tag set 
#include <linux/genhd.h>		// this header contains declarations of struct gendisk and functions to work with struct
#include <linux/slab.h>         // this header contains declaration of functions for work with (virtual)kernel memory
#include <linux/uaccess.h>      // this hrader contains declaration of functions to copy from user and to user space
#include <linux/string.h>
#include <linux/version.h>
#define KERNEL_SECTOR_SIZE 512

MODULE_LICENSE("GPL");
MODULE_AUTHOR("I.Kardanov");
MODULE_DESCRIPTION("This is example of simple block linux modeule");
MODULE_VERSION("0.01");

const unsigned int numberSectors = 16;
const unsigned int sectorSize = KERNEL_SECTOR_SIZE;
const unsigned int diskNameBuffSize = 32;
const unsigned int startMinor = 0;
const unsigned int minors = 1;
int major = 0;
const char* driverName = "simple_blcok_dev";
const char* deviceName = "sblock_dev";


struct SimpleBlockDev {
	int spaceSize;
	u8* data;
	short users;
	spinlock_t lock;
	struct blk_mq_tag_set tagSet;
	struct request_queue* queue;
	struct gendisk* gdisk;
};

typedef int sblkdev_cmd_t;

struct SimpleBlockDev* blockDev;

// My implementation of open device method
static int DevOpen(struct block_device* blkDev, fmode_t mode) {
	//struct SimpleBlockDev* dev = inode->i_bdev->bd_disk->private_data;
	struct SimpleBlockDev* dev = blkDev->bd_inode->i_bdev->bd_disk->private_data;

	spin_lock(&dev->lock);
	++dev->users;
	spin_unlock(&dev->lock);
	return 0;
}

// My implementation of release device method
static void DevRelease(struct gendisk* gdisk, fmode_t mode) {
	//struct SimpleBlockDev* dev = inode->i_bdev->db_disk->private_data;
	struct SimpleBlockDev* dev = gdisk->private_data;
	
	spin_lock(&dev->lock);
	--dev->users;
	spin_unlock(&dev->lock);
}

// My implementation of geom ioctl 
static int DevGetGeometry(struct block_device* blkDev, struct hd_geometry* geo) {
	//struct SimpleBlockDev* dev = inode->i_bdev->bd_disk->private_data;
	struct SimpleBlockDev* dev = blkDev->bd_inode->i_bdev->bd_disk->private_data;
	if (dev == NULL) {
		return -1;
	}

	long size = dev->spaceSize * (sectorSize / KERNEL_SECTOR_SIZE);
	geo->cylinders = (size & ~0x3f) >> 6;
	geo->heads = 4;
	geo->sectors = numberSectors;
	geo->start = 4;
	return 0;
}

// My implementation of ioctl device method
static int DevCompatIoctl(struct block_device* blkDev, fmode_t mode, unsigned int cmd, unsigned long arg) {
	struct hd_geometry geo;

	switch(cmd) {
		case HDIO_GETGEO: {
			if ((DevGetGeometry(blkDev, &geo) == 0) && (copy_to_user((void*)arg, &geo, sizeof(geo)) == 0)) {
				printk(KERN_INFO "%::DevCompatIoctl: request geometry of block device successful\n", driverName);
				return 0;
			}

			printk(KERN_INFO "%::DevCompatIoctl: request geometry of block device failed\n", driverName);
			return -EFAULT;
		}

		default: {
			return -ENOTTY;		 
		}
	}
}

// My implementation of IO request handler
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4, 19, 0)
static int DevRequest(struct request_queue* queue) {
	return 0;
}
#else
static blk_status_t DevRequest(struct blk_mq_hw_ctx* hctx, const struct blk_mq_queue_data* data) {
	return 0;
}
#endif

// this struct describe block device operations
static struct block_device_operations blockOprts = {
	.owner = THIS_MODULE,
	.open = DevOpen,
	.release = DevRelease,
	.compat_ioctl = DevCompatIoctl	
};

static struct blk_mq_ops blockMQOprts = {
	.queue_rq = DevRequest
};

static int __initBlockDevIORequestQueue(void) {

#if LINUX_VERSION_CODE <= KERNEL_VERSION(4, 19, 0)
	blockDev->queue = blk_init_queue(DevRequest, &blockDev->queue);
	return blockDev->queue == NULL ? -1 : 0;
#else
	blockDev->tagSet.ops = &blockMQOprts;
	blockDev->tagSet.nr_hw_queues = 1;
	blockDev->tagSet.queue_depth = 128;
	blockDev->tagSet.numa_node=  NUMA_NO_NODE;
	blockDev->tagSet.cmd_size = sizeof(sblkdev_cmd_t);
	blockDev->tagSet.flags = BLK_MQ_F_SHOULD_MERGE /* | BLK_MQ_F_SG_MERGE */;
	blockDev->tagSet.driver_data = blockDev;
	
	if (blk_mq_alloc_tag_set(&blockDev->tagSet) < 0) {
		return -1;
	}

	blockDev->queue = blk_mq_init_queue(&blockDev->tagSet);
	if (blockDev->queue == NULL) {
		return -1;
	}

	return 0;
#endif
	
}

static void __clearBlockDevIORequestQueue(void) {

#if LINUX_VERSION_CODE <= KERNEL_VERSION(4, 19, 0)

#else
	blk_cleanup_queue(blockDev->queue);
#endif

}

int __init moduleInit(void) {
	// call this functtion is optional!
	major = register_blkdev(0, deviceName);
	if (major < 0) {
		printk(KERN_INFO "%s::ModuleInit: Can`t allocate major number for block device\n", driverName);
		return -EBUSY;
	}

	printk(KERN_INFO "%s::ModuleInit: Allocated major number for our block device: major=%d\n", driverName, major);

	blockDev = kzalloc(sizeof(struct SimpleBlockDev) * 1, GFP_KERNEL);
	if (blockDev == NULL) {
		printk(KERN_INFO "%s::ModuleInit: Can`t allocate mmeory for struct SimpleBlockDev\n", driverName);
		// clear resources
		unregister_blkdev(major, deviceName);
		return -ENOMEM;
	}
	
	blockDev->spaceSize = numberSectors * sectorSize;
	blockDev->data = vmalloc(blockDev->spaceSize);
	if (blockDev->data == NULL) {
		printk(KERN_INFO "%s::Moduleinit: Can`t allocate memory for ram device\n", driverName);
		// clear resources
		kfree(blockDev);
		unregister_blkdev(major, deviceName);
		return -ENOMEM;
	}

	// Initialize spin lock for our block device
	spin_lock_init(&blockDev->lock);
	// Create block device queue for IO operations. We shold initialize spin lock of device alter call blk_init_queue()
	if (__initBlockDevIORequestQueue() < 0) {
		printk(KERN_INFO "%s::ModuleInit: Can`t init block IO device request queue\n", driverName);
		// clear resources
		// ...
		return -ENOMEM;
	}

	if (blockDev->queue == NULL) {
		printk(KERN_INFO "%s::ModuleInit: Can`t allocate IO request queue for block device\n", driverName);
		// clear resources
		unregister_blkdev(major, deviceName);
		vfree(blockDev->data);
		// spin_lock_destroy(&blockDev->lock);
		kfree(blockDev);
		return -ENOMEM;
	}

	// Allocated gendisk struct 
	blockDev->gdisk = alloc_disk(minors);
	if (blockDev->gdisk == NULL) {
		printk(KERN_INFO "%s::ModuleInit: Can`t allocate gen disk struct for device\n", driverName);
		// clear resources
		unregister_blkdev(major, deviceName);
		vfree(blockDev->data);
		// spin_lock_destroy(&blockDev->lock);
		kfree(blockDev);
		return -ENOMEM;
	}

	// set up gendisk struct alfter allocating
	blockDev->gdisk->major = major;
	blockDev->gdisk->first_minor = 0;
	blockDev->gdisk->minors = minors;
	blockDev->gdisk->queue = blockDev->queue;
	blockDev->gdisk->private_data = blockDev;
	blockDev->gdisk->fops = &blockOprts;
	snprintf(blockDev->gdisk->disk_name, diskNameBuffSize, "%s%c", deviceName, 0 + 'a');

	// notify the system about our block device sector size. 
	// The kernel works only with sector size=512 bytes, but actual hardware secotor size of device can be different.
	// We need to convert our hardware sector size to kernel sector size and back
	set_capacity(blockDev->gdisk, numberSectors * (sectorSize / KERNEL_SECTOR_SIZE));

	// Notiify system about our gendisk struct. This call made out driver available to the system.
	// We sholdn`t call add_disk() function if your device has not been really yet!
	add_disk(blockDev->gdisk);
	
	printk(KERN_INFO "%s::ModuleInit: We registered our block device with major=%d\n", driverName, major);
	return 0;
}

void __exit moduleExit(void) {
	unregister_blkdev(major, deviceName);
	del_gendisk(blockDev->gdisk);
	__clearBlockDevIORequestQueue();
	vfree(blockDev->data);
	kfree(blockDev);
}

module_init(moduleInit);
module_exit(moduleExit);
