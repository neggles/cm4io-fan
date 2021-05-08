KERNELRELEASE ?= `uname -r`
subdirs := emc2301
.PHONY: $(subdirs)

all: $(subdirs)
clean: $(subdirs)
modules_install: $(subdirs)
$(subdirs):
	make -C $@ $(MAKECMDGOALS) KERNELRELEASE=$(KERNELRELEASE)
