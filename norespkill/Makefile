	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)
	obj-m := norespkill.o
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
