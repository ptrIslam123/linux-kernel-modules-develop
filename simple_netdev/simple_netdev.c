#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/kernel.h>
#include <linux/etherdevice.h>

#define DRIVER_NAME ("virt-net")
#define DEVICE_NAME ("virt-net%d")

static struct net_device *net_dev;

static int device_open(struct net_device* dev) {
    printk(KERN_INFO "Device open\n");
    netif_start_queue(dev);
    return 0;
}

static int device_stop(struct net_device* dev) {
    printk(KERN_INFO "Device stop\n");
    netif_stop_queue(dev);
    return 0;
}

static int device_start_xmit(struct sk_buff* skb, struct net_device* dev) {
    printk(KERN_INFO "Device start xmit\n");
    dev_kfree_skb(skb);
    return 0;
}

static int device_init(struct net_device* dev) {
    printk(KERN_INFO "Device init\n");
    return 0;
};

static const struct net_device_ops net_dev_ops = {
    .ndo_init = device_init,
    .ndo_open = device_open,
    .ndo_stop = device_stop,
    .ndo_start_xmit = device_start_xmit,
};

static void device_setup(struct net_device* dev){
    dev->netdev_ops = &net_dev_ops;
    printk(KERN_INFO "Device setup\n");
}

static int __init init_module_device(void) {
    printk(KERN_INFO "Start initialization and registration of net device module=%s\n", DRIVER_NAME);

    int ret;
    net_dev = alloc_netdev(0, DEVICE_NAME, NET_NAME_UNKNOWN, device_setup);
    if((ret = register_netdev(net_dev))) {
        printk(KERN_ERR "Could not register the net device\n");
        return ret;
    }

    printk(KERN_INFO "initialized the net device successfully\n");
    return 0;
}

static void __exit deinit_module_device(void)
{
    printk(KERN_INFO "Cleaning Up the Module\n");
    unregister_netdev(net_dev);
}

module_init(init_module_device);
module_exit(deinit_module_device);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kardanov.I");
MODULE_DESCRIPTION("A simple net device");