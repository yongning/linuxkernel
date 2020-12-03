#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/swap.h>
#include <linux/profile.h>
#include <linux/notifier.h>

#include <linux/module.h>
#include <linux/device.h>

#include <linux/timer.h>
#include <linux/jiffies.h>

#include <linux/list.h>


struct timer_list norespkill_timer;
void norespkill_timer_cb(struct timer_list*);

static unsigned long noresp_scan(unsigned char* name, unsigned long control);

void norespkill_timer_cb(struct timer_list* timer)
{

    noresp_scan(NULL, 0);

    mod_timer(&norespkill_timer, jiffies + msecs_to_jiffies(1000 * 60));

    noresp_scan(NULL, 1);

}

static unsigned long noresp_scan(unsigned char* name, unsigned long control)
{
    struct task_struct* tsk;
    /* struct task_struct* selected = NULL; */

    if (control == 1) {
        del_timer(&norespkill_timer);
        printk(KERN_ERR "==== ynf del timer ====\n");
        return 1;
    }

    rcu_read_lock();
    for_each_process(tsk) {
        struct task_struct* p;
        if (tsk->flags & PF_KTHREAD)
            continue;
        printk(KERN_ERR "========ynf tsk comm is %s ==== \n", tsk->comm);
    }
    rcu_read_unlock();

    return 0;
}

static int norespkill_init(void)
{
    printk(KERN_ALERT "Hello, norespkill\n");
    timer_setup(&norespkill_timer, norespkill_timer_cb, 0);
    mod_timer(&norespkill_timer, jiffies + msecs_to_jiffies(1000 * 60));


    /* noresp_scan(NULL, 0); */
    return 0;
}

static void norespkill_exit(void)
{
    printk(KERN_ALERT "Goodbye, norespkill\n");
}

module_init(norespkill_init);
module_exit(norespkill_exit);

MODULE_DESCRIPTION("No response thread kill for ltp test");
MODULE_AUTHOR("ynf");
MODULE_LICENSE("GPL v2");
