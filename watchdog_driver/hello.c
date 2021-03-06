#include <linux/io.h>
#include <linux/io-64-nonatomic-lo-hi.h>
#include <linux/interrupt.h>


/* #include <linux/init.h> */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/watchdog.h>
#include <asm/arch_timer.h>

/* #include <linux/sched.h> */

/* #include <linux/kernel.h> */
/* #include <linux/device.h> */

/* #include <linux/list.h> */

#if 0
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
#endif


#define DRV_NAME "sbsa-gwdt"
#define WATCHDOG_NAME "SBSA Generic Watchdog"

#define SBSA_GWDT_WRR 0x000

#define SBSA_GWDT_WCS 0x000
#define SBSA_GWDT_WOR 0x008
#define SBSA_GWDT_WCV 0x010

#define SBSA_GWDT_W_IIDR 0xfcc
#define SBSA_GWDT_IDR 0xfd0

#define SBSA_GWDT_WCS_EN BIT(0)
#define SBSA_GWDT_WCS_WS0 BIT(1)
#define SBSA_GWDT_WCS_WS1 BIT(2)

struct sbsa_gwdt {
    struct watchdog_device wdd;
    u32 clk;
    void __iomem* refresh_base;
    void __iomem* control_base;
};

#define DEFAULT_TIMEOUT 10

static unsigned int timeout;
module_param(timeout, uint, 0);
MODULE_PARM_DESC(timeout,
		"Watchdog timeout in seconds. (>=0, default="
		__MODULE_STRING(DEFAULT_TIMEOUT) ")");

static int action;
module_param(action, int, 0);
MODULE_PARM_DESC(action, "after watchdog gets WS0 interrupt, do:"
		"0 = skip(*) 1 = panic");
static bool nowayout = WATCHDOG_NOWAYOUT;
module_param(nowayout, bool, S_IRUGO);
MODULE_PARM_DESC(nowayout, 
		"Watchdog cannot be stopped once started (defalt="
		__MODULE_STRING(WATCHDOG_NOWAYOUT) ")");

static int sbsa_gwdt_set_timeout(struct watchdog_device* wdd,
		unsigned int timeout)
{
    struct sbsa_gwdt* gwdt = watchdog_get_drvdata(wdd);

    wdd->timeout = timeout;

    if (action)
	writel(gwdt->clk * timeout, 
	        gwdt->control_base + SBSA_GWDT_WOR);
    else
	writel(gwdt->clk / 2 * timeout,
	        gwdt->control_base + SBSA_GWDT_WOR);

    return 0;
}


static unsigned int sbsa_gwdt_get_timeleft(struct watchdog_device* wdd)
{
    struct sbsa_gwdt* gwdt = watchdog_get_drvdata(wdd);
    u64 timeleft = 0;

    if (!action &&
	!(readl(gwdt->control_base + SBSA_GWDT_WCS) & SBSA_GWDT_WCS_WS0))
	timeleft += readl(gwdt->control_base + SBSA_GWDT_WOR);

    timeleft += lo_hi_readq(gwdt->control_base + SBSA_GWDT_WCV) -
	arch_counter_get_cntvct();

    do_div(timeleft, gwdt->clk);

    return timeleft;
}

static int sbsa_gwdt_keepalive(struct watchdog_device* wdd)
{
    struct sbsa_gwdt* gwdt = watchdog_get_drvdata(wdd);

    writel(0, gwdt->refresh_base + SBSA_GWDT_WRR);

    return 0;
}


static int sbsa_gwdt_start(struct watchdog_device* wdd)
{
    struct sbsa_gwdt* gwdt = watchdog_get_drvdata(wdd);

    writel(SBSA_GWDT_WCS_EN, gwdt->control_base + SBSA_GWDT_WCS);

    return 0;
}



static int sbsa_gwdt_stop(struct watchdog_device* wdd)
{
    struct sbsa_gwdt* gwdt = watchdog_get_drvdata(wdd);

    writel(0, gwdt->control_base + SBSA_GWDT_WCS);

    return 0;
}


static irqreturn_t sbsa_gwdt_interrupt(int irq, void* dev_id)
{
    panic(WATCHDOG_NAME " timeout ");

    return IRQ_HANDLED;
}

static const struct watchdog_info sbsa_gwdt_info = {
    .identity = WATCHDOG_NAME,
    .options = WDIOF_SETTIMEOUT |
	    WDIOF_KEEPALIVEPING |
	    WDIOF_MAGICCLOSE |
	    WDIOF_CARDRESET,
};

static const struct watchdog_ops sbsa_gwdt_ops = {
    .owner = THIS_MODULE,
    .start = sbsa_gwdt_start,
    .stop = sbsa_gwdt_stop,
    .ping = sbsa_gwdt_keepalive,
    .set_timeout = sbsa_gwdt_set_timeout,
    .get_timeleft = sbsa_gwdt_get_timeleft,
};

static int sbsa_gwdt_probe(struct platform_device* pdev)
{
    void __iomem* rf_base, *cf_base;
    struct device* dev = &pdev->dev;
    struct watchdog_device* wdd;
    struct sbsa_gwdt* gwdt;
    struct resource* res;
    int ret, irq;
    u32 status;

    gwdt = devm_kzalloc(dev, sizeof(*gwdt), GFP_KERNEL);
    if (!gwdt)
	return -ENOMEM;
    platform_set_drvdata(pdev, gwdt);

    printk(KERN_ERR "\n ========yongning == watchdog probe ====\n");

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    cf_base = devm_ioremap_resource(dev, res);
    if (IS_ERR(cf_base))
	return PTR_ERR(cf_base);

    res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
    rf_base = devm_ioremap_resource(dev, res);
    if (IS_ERR(rf_base))
	return PTR_ERR(rf_base);

    gwdt->clk = arch_timer_get_cntfrq();
    gwdt->refresh_base = rf_base;
    gwdt->control_base = cf_base;

    wdd = &gwdt->wdd;
    wdd->parent = dev;
    wdd->info = &sbsa_gwdt_info;
    wdd->ops = &sbsa_gwdt_ops;
    wdd->min_timeout = 1;
    wdd->max_hw_heartbeat_ms = U32_MAX / gwdt->clk * 1000;
    wdd->timeout = DEFAULT_TIMEOUT;
    watchdog_set_drvdata(wdd, gwdt);
    watchdog_set_nowayout(wdd, nowayout);

    status = readl(cf_base + SBSA_GWDT_WCS);
    if (status & SBSA_GWDT_WCS_WS1) {
	dev_warn(dev, "System reset by WDT.\n");
	wdd->bootstatus |= WDIOF_CARDRESET;
    }
    if (status & SBSA_GWDT_WCS_EN)
	set_bit(WDOG_HW_RUNNING, &wdd->status);

    if (action) {
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
            action = 0;
	    dev_warn(dev, "unable to get ws0 interrupt.\n");
	} else {
	    writel(0, rf_base + SBSA_GWDT_WRR);
	    if (devm_request_irq(dev, irq, sbsa_gwdt_interrupt, 0,
	            pdev->name, gwdt)) {
		action = 0;
		dev_warn(dev, "unable to request IQR %d. \n", irq);
	    }
	}
	if (!action)
            dev_warn(dev, "falling back to single stage mode.\n");
    }

    if (!action)
	wdd->max_hw_heartbeat_ms *= 2;

    watchdog_init_timeout(wdd, timeout, dev);

    sbsa_gwdt_set_timeout(wdd, wdd->timeout);

    ret = watchdog_register_device(wdd);
    if (ret)
	return ret;

    dev_info(dev, "Initialized with %ds timeout @ %u Hz, action=%d.%s\n",
		wdd->timeout, gwdt->clk, action,
		status & SBSA_GWDT_WCS_EN ? " [enabled]" : "");

    return 0;
}


static void sbsa_gwdt_shutdown(struct platform_device* pdev)
{
    struct sbsa_gwdt* gwdt = platform_get_drvdata(pdev);

    sbsa_gwdt_stop(&gwdt->wdd);
}

static int sbsa_gwdt_remove(struct platform_device* pdev)
{
    struct sbsa_gwdt* gwdt = platform_get_drvdata(pdev);

    watchdog_unregister_device(&gwdt->wdd);

    return 0;
}

static int __maybe_unused sbsa_gwdt_suspend(struct device* dev)
{
    struct sbsa_gwdt* gwdt = dev_get_drvdata(dev);

    if (watchdog_active(&gwdt->wdd))
	sbsa_gwdt_stop(&gwdt->wdd);

    return 0;
}

static int __maybe_unused sbsa_gwdt_resume(struct device* dev)
{
    struct sbsa_gwdt* gwdt = dev_get_drvdata(dev);

    if (watchdog_active(&gwdt->wdd))
	sbsa_gwdt_start(&gwdt->wdd);

    return 0;
}

static const struct dev_pm_ops sbsa_gwdt_pm_ops = {
    SET_SYSTEM_SLEEP_PM_OPS(sbsa_gwdt_suspend, sbsa_gwdt_resume)
};

static const struct of_device_id sbsa_gwdt_of_match[] = {
    { .compatible = "arm,sbsa-gwdt", },
    {},
};
MODULE_DEVICE_TABLE(of, sbsa_gwdt_of_match);

static const struct platform_device_id sbsa_gwdt_pdev_match[] = {
    { .name = DRV_NAME, },
    {},
};
MODULE_DEVICE_TABLE(platform, sbsa_gwdt_pdev_match);

static struct platform_driver sbsa_gwdt_driver = {
    .driver = {
        .name = DRV_NAME,
	.pm = &sbsa_gwdt_pm_ops,
	.of_match_table = sbsa_gwdt_of_match,
    },
    .probe = sbsa_gwdt_probe,
    .remove = sbsa_gwdt_remove,
    .shutdown = sbsa_gwdt_shutdown,
    .id_table = sbsa_gwdt_pdev_match,
};

module_platform_driver(sbsa_gwdt_driver);

MODULE_DESCRIPTION("SBSA Generic Watchdog Driver");
MODULE_AUTHOR("Fan");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:" DRV_NAME);

/*
module_init(hello_init);
module_exit(hello_exit);
*/
