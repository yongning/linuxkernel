#include <linux/module.h>
#include <linux/moduleparam.h>

#include <linux/init.h>

#include <linux/types.h>
#include <linux/fs.h>

#include <linux/ioctl.h>
#include <linux/cdev.h>

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/slab.h>

#include <linux/sched.h>
#include <linux/swap.h>
#include <linux/profile.h>
#include <linux/notifier.h>

#include <linux/device.h>

#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/string.h>

#include <asm/uaccess.h>

#include <linux/list.h>

#define NORESPKILL_IOC_MAGIC 'N'
#define NORESPKILL_IOCSTIME _IOW(NORESPKILL_IOC_MAGIC, 1, int)
#define NORESPKILL_IOCGTIME _IOR(NORESPKILL_IOC_MAGIC, 2, int)

static unsigned int noresp_minor = 0;
static unsigned int noresp_major;

struct noresp_dev* noresp_device;

struct timer_list norespkill_timer;
void norespkill_timer_cb(struct timer_list*);

static unsigned long noresp_scan(unsigned char* name, unsigned long control);

static int number = 0;
static int preset = 5;

struct noresp_dev {
    unsigned int timenumber;
    unsigned int curr;
    unsigned int timeinterval; /* min */
    struct cdev cdev;
};


void norespkill_timer_cb(struct timer_list* timer)
{
    number++;
    if (number <= preset) {
        noresp_scan(NULL, 2);

        mod_timer(&norespkill_timer, jiffies + msecs_to_jiffies(1000 * 600));
    } else {
        noresp_scan(NULL, 1);
        del_timer(&norespkill_timer);
        printk(KERN_ALERT "====ynf del timer ====\n");
    }

    // mod_timer(&norespkill_timer, jiffies + msecs_to_jiffies(1000 * 600));
    
    // noresp_scan(NULL, 2);

}

static unsigned long noresp_scan(unsigned char* name, unsigned long control)
{
    struct task_struct* tsk;
    struct task_struct* selected = NULL;
    unsigned char tsk_name_tmp[32];

    if (control == 0) {
       /*  del_timer(&norespkill_timer); */
        printk(KERN_ALERT "==== ynf tsk time is: %d ====\n", number);
        return 1;
    }

    rcu_read_lock();
    for_each_process(tsk) {
        /* struct task_struct* p; */
        if (tsk->flags & PF_KTHREAD)
            continue;
        if (control == 2) {
            printk(KERN_ALERT "==== thread name is %s ====\n", tsk->comm);
        }
 
        if (control == 1) {
            strcpy(tsk_name_tmp, tsk->comm);
            if (strncmp(tsk_name_tmp, "genload", sizeof("genload")) == 0) {
                printk(KERN_ALERT "======ynf find genload ==== \n");
                selected = tsk;
                task_lock(selected);
                send_sig(SIGKILL, selected, 0);
                task_unlock(selected);
            }
            if (strncmp(tsk_name_tmp, "ltp-pan", sizeof("ltp-pan")) == 0) {
                printk(KERN_ALERT "====== ynf find ltp-pan ==== \n");
                selected = tsk;
                task_lock(selected);
                send_sig(SIGKILL, selected, 0);
                task_unlock(selected);
            }

            if (strncmp(tsk_name_tmp, "run_ltp_test.sh", sizeof("run_ltp_test.sh")) == 0) {
                printk(KERN_ALERT "====== ynf find run_ltp_test ==== \n");
                selected = tsk;
                task_lock(selected);
                send_sig(SIGKILL, selected, 0);
                task_unlock(selected);
            }
        }
    }
    rcu_read_unlock();

    return 0;
}

ssize_t noresp_read(struct file* filp, char __user* buf, size_t count, loff_t* offp)
{
   struct noresp_dev* dev;
   int retval;

   dev = filp->private_data;
   if (copy_to_user(buf, (char *)&dev->curr, sizeof(unsigned int))) {
       retval = -EFAULT;
       return retval;
   }
   return (sizeof(unsigned int));
}

ssize_t noresp_write(struct file* filp, const char __user* buf, size_t count, loff_t* f_pos)
{
    struct noresp_dev* dev;
    int retval;

    dev = filp->private_data;
    if (copy_from_user((char *)&dev->timenumber, buf, sizeof(unsigned int))) {
	retval = -EFAULT;
	return retval;
    }
    
    return (sizeof(unsigned int));
}

int noresp_release(struct inode* inode, struct file* filp)
{
    return 0;
}

int noresp_open(struct inode* inode, struct file* filp)
{
    struct noresp_dev* dev;
    
    dev = container_of(inode->i_cdev, struct noresp_dev, cdev);
    dev->timenumber = 0;
    dev->curr = 0;
    dev->timeinterval = 0;

    filp->private_data = dev;

    timer_setup(&norespkill_timer, norespkill_timer_cb, 0);
    
    return 0;
}

long noresp_ioctl(struct file* filp, unsigned int cmd, unsigned long arg)
{
    int err = 0, ret = 0;
    struct noresp_dev* dev;

    printk(KERN_WARNING "====noresp === ioctl enter \n");
    if (_IOC_DIR(cmd & _IOC_READ))
	err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
    else if (_IOC_DIR(cmd & _IOC_WRITE))
        err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
    if (err)
        return -EFAULT;

    dev = filp->private_data;
    
    switch (cmd) {
    case NORESPKILL_IOCSTIME:
        ret = __get_user(dev->timenumber, (int __user *)arg);
	preset = dev->timenumber;
        mod_timer(&norespkill_timer, jiffies + msecs_to_jiffies(1000 * 600));
        break;
    case NORESPKILL_IOCGTIME:
	dev->curr = number;
        ret = __put_user(dev->curr, (int __user *)arg);
        break;
    default:
        ret = -ENOTTY;
        break;
    }

    return ret;

}

struct file_operations noresp_fops = {
    .owner = THIS_MODULE,
    .read = noresp_read,
    .write = noresp_write,
    .unlocked_ioctl = noresp_ioctl,
    .open = noresp_open,
    .release = noresp_release,
};

static void noresp_setup_cdev(struct noresp_dev* dev, int index)
{
    int err, devno;

    devno = MKDEV(noresp_major, noresp_minor + index);

    cdev_init(&dev->cdev, &noresp_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &noresp_fops;
    err = cdev_add(&dev->cdev, devno, 1);
    if (err)
	printk(KERN_WARNING "noresp error add devcie\n");
    
}

static int norespkill_init(void)
{
    int result;
    dev_t dev;
    unsigned int noresp_devcnt = 1;
    
    printk(KERN_WARNING "Hello, norespkill\n");

    result = alloc_chrdev_region(&dev, noresp_minor, noresp_devcnt, "norespkill");
    if (result < 0) {
	printk(KERN_WARNING "norespkill: can't get major dev \n");
	return result;
    }
    noresp_major = MAJOR(dev);

    noresp_device = kmalloc(sizeof(struct noresp_dev), GFP_KERNEL);
    if (!noresp_device) {
	printk(KERN_WARNING "norespkill no memory for device \n");
	result = -ENOMEM;
	goto no_mem;
    }
    memset(noresp_device, 0, sizeof(struct noresp_dev));
    noresp_setup_cdev(noresp_device, 0);
    // timer_setup(&norespkill_timer, norespkill_timer_cb, 0);
    // mod_timer(&norespkill_timer, jiffies + msecs_to_jiffies(1000 * 600));


    /* noresp_scan(NULL, 0); */
    return 0;

no_mem:
    unregister_chrdev_region(dev, noresp_devcnt);
    return result;    
}

static void norespkill_exit(void)
{
    printk(KERN_ALERT "Goodbye, norespkill\n");
    cdev_del(&noresp_device->cdev);
    kfree(noresp_device);
    unregister_chrdev_region(MKDEV(noresp_major, noresp_minor), 1);
}

module_init(norespkill_init);
module_exit(norespkill_exit);

MODULE_DESCRIPTION("No response thread kill for ltp test");
MODULE_AUTHOR("ynf");
MODULE_LICENSE("GPL v2");
