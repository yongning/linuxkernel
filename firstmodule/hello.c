#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>

MODULE_LICENSE("Dual BSD/GPL");

static int hello_init(void)
{
    
    printk(KERN_ALERT "hello, kernel\n");
    printk(KERN_ALERT "pid is %u , name is %s\n", current->pid, current->comm);
    return 0;
}

static void hello_exit(void)
{
    printk(KERN_ALERT "Goodbye, kernel\n");
}

module_init(hello_init);
module_exit(hello_exit);
