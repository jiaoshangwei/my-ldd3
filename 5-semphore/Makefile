

DEBUG = y

ifeq ($(DEBUG),y)
	DEBFLAGS = -O -DDEV_DEBUG
else
	DEBFLAGS = -O2
endif

EXTRA_CFLAGS += $(DEBFLAGS)


obj-m := semaphore.o

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) LDDINC=$(PWD) modules
	gcc -o down down.c
	gcc -o up up.c
clean:
	 rm -rf *.o *.ko *mod.* *.order *.symvers  *.out down up
