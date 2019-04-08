#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>

#include <linux/kernel.h>
#include <linux/device.h>

#include <linux/list.h>

LIST_HEAD(gHwInfo);

struct HwInfo_s {
	char HwName[8];
	int HwType;
	struct list_head list;
};

typedef struct HwInfo_s HwInfo_t; 

static struct bus_type yn_test_bus_type = {
    .name = "yn_test_bus",
};

MODULE_LICENSE("Dual BSD/GPL");

static int hello_init(void)
{
    int retval;

	HwInfo_t hwinfo = {
		.HwType = 0,
		.HwName = "hello0",
	};

	HwInfo_t hwinfo2 = {
		.HwType = 1,
		.HwName = "hello1",
	};

	INIT_LIST_HEAD(&hwinfo2.list);

	HwInfo_t* pHwtmp;

	INIT_LIST_HEAD(&hwinfo.list);

	list_add(&hwinfo.list, &gHwInfo);
	list_add_tail(&hwinfo2.list, &gHwInfo);

    pHwtmp = list_entry(&hwinfo.list, HwInfo_t, list);

	printk(KERN_ALERT "list name is %s \n", pHwtmp->HwName);

	list_for_each_entry(pHwtmp, &gHwInfo, list) {
		printk(KERN_ALERT "list for each entry name is %s \n", pHwtmp->HwName);
	}

    printk(KERN_ALERT "hello, kernel\n");
    printk(KERN_ALERT "pid is %u , name is %s\n", current->pid, current->comm);
    retval = bus_register(&yn_test_bus_type);
    if (retval) {
        printk(KERN_ALERT "bus_register error \n");
        return -1;
    }

    return 0;
}

static void hello_exit(void)
{
    printk(KERN_ALERT "Goodbye, kernel\n");
}

module_init(hello_init);
module_exit(hello_exit);
