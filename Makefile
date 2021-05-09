
KERNELRELEASE ?= `uname -r`
export kernelver ?= $(shell uname -r)

DIRS = emc2301 overlays
BUILDDIRS = $(DIRS:%=build-%)
CLEANDIRS = $(DIRS:%=clean-%)

DKMS := $(shell command -v dkms 2> /dev/null)

all: check_dkms $(BUILDDIRS)
modules_install: $(BUILDDIRS)
$(DIRS): $(BUILDDIRS)
$(BUILDDIRS):
	$(MAKE) -C $(@:build-%=%) $(MAKECMDGOALS) KERNELRELEASE=$(KERNELRELEASE)

clean: $(CLEANDIRS) clean_dtbo
$(CLEANDIRS):
	$(MAKE) -C $(@:clean-%=%) clean KERNELRELEASE=$(KERNELRELEASE)
clean_dtbo:
	rm -f $(KERNELRELEASE)/module/cm4io-fan.dtbo
	rmdir -p $(KERNELRELEASE)/module

check_dkms:
ifndef DKMS
	$(error "DKMS is not installed, please apt install dkms")
endif
	echo "DKMS found at $(shell which dkms)"
