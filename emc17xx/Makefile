KDIR ?= /lib/modules/`uname -r`/build

default:
	$(MAKE) -C $(KDIR) M=$$PWD

clean:
	$(MAKE) -C $(KDIR) M=$$PWD clean

modules_install:
	$(MAKE) -C $(KDIR) M=$$PWD modules_install
