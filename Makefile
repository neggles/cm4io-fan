# cm4io-fan
KERNELRELEASE ?= $(shell uname -r)
export kernelver ?= $(KERNELRELEASE)
export arch ?= $(shell uname -m)

DKMS := $(shell command -v dkms 2> /dev/null)

DIRS = emc2301 overlays
BUILDDIRS = $(DIRS:%=build-%)
CLEANDIRS = $(DIRS:%=clean-%)
INSTALLDIRS = $(DIRS:%=install-%)

all: check_dkms $(BUILDDIRS)
modules_install: $(BUILDDIRS)
$(DIRS): $(BUILDDIRS)
$(BUILDDIRS):
	$(MAKE) -C $(@:build-%=%) $(MAKECMDGOALS)

clean: $(CLEANDIRS) clean_dtbo
$(CLEANDIRS):
	$(MAKE) -C $(@:clean-%=%) $(MAKECMDGOALS)

install: $(INSTALLDIRS)
modules_install: $(INSTALLDIRS)
$(INSTALLDIRS):
	$(MAKE) -C $(@:install-%=%) modules_install

check_dkms:
ifndef DKMS
	$(error "DKMS is not installed, please apt install dkms")
endif
	echo "DKMS found at $(shell which dkms)"
