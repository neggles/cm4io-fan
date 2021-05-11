KDIR ?= /lib/modules/$(KERNELRELEASE)/build
obj-m += emc2301.o

default:
	$(MAKE) -C $(KDIR) M=$$PWD

clean:
	$(MAKE) -C $(KDIR) M=$$PWD clean

modules_install:
	$(MAKE) -C $(KDIR) M=$$PWD modules_install
