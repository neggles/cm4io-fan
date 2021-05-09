PWD := $(shell pwd)

DTC_FLAGS ?= -@
DTC_INCLUDE := /lib/modules/${kernelver}/build/include

dtbo-y  += cm4io-fan.dtbo

targets += $(dtbo-y)
always-y	:= $(dtbo-y)

all:
	make -C /lib/modules/${kernelver}/build M=$(PWD) DTC_INCLUDE=/lib/modules/${KERNELRELEASE}/build/include

clean:
	make -C /lib/modules/${kernelver}/build M=$(PWD) clean

modules_install:
	cp ./$(dtbo-y) /boot/overlays/
