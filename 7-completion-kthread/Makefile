

DEBUG = y

ifeq ($(DEBUG),y)
	DEBFLAGS = -O -DDEV_DEBUG
else
	DEBFLAGS = -O2
endif

EXTRA_CFLAGS += $(DEBFLAGS)


obj-m := completion.o

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) LDDINC=$(PWD) modules
clean:
	 rm -rf *.o *.ko *mod.* *.order *.symvers  *.out main
