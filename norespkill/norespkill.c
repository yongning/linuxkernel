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
#include <linux/string.h>

#include <linux/list.h>


struct timer_list norespkill_timer;
void norespkill_timer_cb(struct timer_list*);

static unsigned long noresp_scan(unsigned char* name, unsigned long control);

static int number = 0;
static int preset = 5;

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

            if (memcmp(tsk_name_tmp, "run_ltp_test", sizeof("run_ltp_test")) == 0) {
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

static int norespkill_init(void)
{
    printk(KERN_ALERT "Hello, norespkill\n");
    timer_setup(&norespkill_timer, norespkill_timer_cb, 0);
    mod_timer(&norespkill_timer, jiffies + msecs_to_jiffies(1000 * 600));


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
