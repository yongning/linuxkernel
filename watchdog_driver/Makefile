	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)
	obj-m := dx_sbsa_watchdog.o
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
