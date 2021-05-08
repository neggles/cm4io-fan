PWD := $(shell pwd)

DTC_FLAGS ?= -@
DTC_INCLUDE := /lib/modules/${KERNELRELEASE}/build/include

dtbo-y  += cm4io-fan.dtbo
targets += $(dtbo-y)

always-y	:= $(dtbo-y)
clean-files	:= *.dtbo

all:
	make -C /lib/modules/${KERNELRELEASE}/build M=$(PWD) DTC_INCLUDE=/lib/modules/${KERNELRELEASE}/build/include
	../dkms.post_build

clean:
	make -C /lib/modules/${KERNELRELEASE}/build M=$(PWD) clean

modules_install:
	cp ./$(dtbo-y) /boot/overlays/
