	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)
	obj-m := hello.o
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
