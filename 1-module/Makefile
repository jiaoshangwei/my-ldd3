

DEBUG = y

ifeq ($(DEBUG),y)
	DEBFLAGS = -O -DHELLO_DEBUG
else
	DEBFLAGS = -O2
endif

EXTRA_CFLAGS += $(DEBFLAGS)


obj-m := hello.o

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) LDDINC=$(PWD) modules

clean:
	 rm -rf *.o *.ko *mod.* *.order *.symvers 
