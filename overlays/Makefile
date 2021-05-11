KDIR ?= /lib/modules/${KERNELRELEASE}/build
PWD := $(shell pwd)

DTC_INCLUDE := $(KDIR)/include

dtbo-y  += cm4io-fan.dtbo

targets  += $(dtbo-y)
always-y += $(dtbo-y)
clean-files := $(dtbo-y)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) DTC_INCLUDE=$(DTC_INCLUDE)

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

install: $(modules_install)
modules_install:
	cp $(dtbo-y) /boot/overlays/
