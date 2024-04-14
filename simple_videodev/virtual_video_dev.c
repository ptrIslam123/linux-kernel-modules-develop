#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/time.h>
#include <linux/module.h>
#include <linux/videodev2.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/capability.h>
#include <linux/eventpoll.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-common.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <linux/miscdevice.h>

#define MISC_DEVICE_NAME            "myv4l2-virtual-device.c"
#define VIRTUAL_CARD_LABEL_NAME     "Dummy video device"
#define VIRTUAL_VIDEO_DEVICE_NAME   "myv4l2l-virtual-device.v"
#define LPREFX                      "MY_V4L2_MODULE: "


int v4l2_file_open(struct inode *, struct file *)
{
    printk(KERN_INFO LPREFX "Call file::open");
    return 0;
}

static long v4l2_file_ioctl(struct file *file, unsigned int cmd,
				       unsigned long parm)
{
    printk(KERN_INFO LPREFX "Call file::ioctl");
    return 0;
}

static const struct file_operations v4l2loopback_ctl_fops = {
	.owner		= THIS_MODULE,
	.open		= v4l2_file_open,
	.unlocked_ioctl	= v4l2_file_ioctl,
};

static int v4l2_video_open(struct file *)
{
    printk(KERN_INFO LPREFX "Call v4l2::open");
    return 0;
}

static int v4l2_video_close(struct file *)
{
    printk(KERN_INFO LPREFX "Call v4l2::close/release");
    return 0;
}


static const struct v4l2_file_operations v4l2_loopback_fops = {
	.owner		= THIS_MODULE,
	.open		= v4l2_video_open,
	.release	= v4l2_video_close
};

static struct miscdevice v4l2loopback_misc = {
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= MISC_DEVICE_NAME,
	.fops		= &v4l2loopback_ctl_fops,
};

struct v4l2_loopback_device {
	struct v4l2_device v4l2_dev;
	struct video_device *vdev;
    char card_label[32];
};

struct v4l2_loopback_device *dev;

static void init_vdev(struct video_device *vdev, int nr)
{
	vdev->vfl_type = VFL_TYPE_VIDEO;
	vdev->fops = &v4l2_loopback_fops;
	vdev->release = &video_device_release;
	vdev->minor = -1;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
	vdev->device_caps = V4L2_CAP_DEVICE_CAPS | V4L2_CAP_VIDEO_CAPTURE |
			    V4L2_CAP_VIDEO_OUTPUT | V4L2_CAP_READWRITE |
			    V4L2_CAP_STREAMING;
#endif
	vdev->vfl_dir = VFL_DIR_M2M;
}

static int __init v4l2_module_init(void)
{
    printk(KERN_INFO LPREFX "Start module\n");

    int err;
    // Create and register character device
    {
        err = misc_register(&v4l2loopback_misc); // This function will register my character device in "/dev/<my-character-device-name>"
        if (err < 0) {
            printk(KERN_INFO LPREFX "Could nott create misc(character) device\n");
            return err;
        } else {
            printk(KERN_INFO LPREFX "Registered misc(character) device successful\n");
        }
    }
    
    int nr = -1;
    dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
        printk(KERN_INFO LPREFX "Could not allocate v4l2_loopback_device struture\n");
        misc_deregister(&v4l2loopback_misc);
		return -ENOMEM;
    }

    snprintf(dev->card_label, sizeof(dev->card_label),
			 "%s (0x%04X)", VIRTUAL_CARD_LABEL_NAME, nr);
    snprintf(dev->v4l2_dev.name, sizeof(dev->v4l2_dev.name),
		 "%s-%03d", VIRTUAL_VIDEO_DEVICE_NAME, nr);

    err = v4l2_device_register(NULL, &dev->v4l2_dev);
	if (err) {
        printk(KERN_INFO LPREFX "Could not register video device(prob)\n");
        misc_deregister(&v4l2loopback_misc);
        v4l2_device_unregister(&dev->v4l2_dev);
        kfree(dev);
        return err;
    } else {
        printk(KERN_INFO LPREFX "Registered video device successful(prob)\n");
    }

    dev->vdev = video_device_alloc();
	if (dev->vdev == NULL) {
		printk(KERN_INFO LPREFX "Could not allocate video device");
        err = -ENOMEM;
        misc_deregister(&v4l2loopback_misc);
        v4l2_device_unregister(&dev->v4l2_dev);
        kfree(dev);
		return err;
	} 

    snprintf(dev->vdev->name, sizeof(dev->vdev->name), "%s",
		 dev->card_label);
    
    init_vdev(dev->vdev, nr);
    dev->vdev->v4l2_dev = &dev->v4l2_dev;

    if (video_register_device(dev->vdev, VFL_TYPE_VIDEO, nr) < 0) {
        err = -EFAULT;
        printk(KERN_INFO LPREFX "Could not register video device\n");
        misc_deregister(&v4l2loopback_misc);
        kfree(dev);
		return err;
	} else {
        printk(KERN_INFO LPREFX "Registered video device and module successful\n");
    } 

    return 0;
}

static void __exit v4l2_module_exit(void)
{
    misc_deregister(&v4l2loopback_misc);
    v4l2_device_unregister(&dev->v4l2_dev);
    printk(KERN_INFO LPREFX "Exit module\n");
}

module_init(v4l2_module_init);
module_exit(v4l2_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kardanov.I");
MODULE_DESCRIPTION("A simple V4L2 example driver");