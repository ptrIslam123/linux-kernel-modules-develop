#include <linux/module.h>       /* Needed by all modules */
#include <linux/kernel.h>       /* Needed for KERN_INFO */
#include <linux/init.h>         /* Needed for the macros */

#include <asm/current.h>        // this header contains macros "current" - return current process (struct task_struct)
#include <linux/sched.h>        // this header contains defenition of struct task_struct

MODULE_LICENSE("GPL");
MODULE_AUTHOR("I.Kardanov");
MODULE_DESCRIPTION("This is example of simple linux modeule that just print hello world");
MODULE_VERSION("0.01");

static int __init hello_start(void) {
    current;
    printk(KERN_INFO "Hello world\n");
    printk(KERN_INFO "The process name=\"%s\" and pid=%i\n",
            current->comm, current->pid);
    return 0;
}

static void __exit hello_end(void) {
    printk(KERN_INFO "Goodbye Mr.\n");
}

module_init(hello_start);
module_exit(hello_end);