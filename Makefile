
KERNELRELEASE ?= `uname -r`
export kernelver ?= $(shell uname -r)

DIRS = emc2301 overlays
BUILDDIRS = $(DIRS:%=build-%)
CLEANDIRS = $(DIRS:%=clean-%)

all: $(BUILDDIRS)
modules_install: $(BUILDDIRS)
$(DIRS): $(BUILDDIRS)
$(BUILDDIRS):
	$(MAKE) -C $(@:build-%=%) $(MAKECMDGOALS) KERNELRELEASE=$(KERNELRELEASE)

clean: $(CLEANDIRS)
$(CLEANDIRS):
	$(MAKE) -C $(@:clean-%=%) clean KERNELRELEASE=$(KERNELRELEASE)
