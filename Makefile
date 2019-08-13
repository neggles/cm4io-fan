subdirs := emc17xx emc181x pac1934
.PHONY: $(subdirs)

all: $(subdirs)
clean: $(subdirs)

$(subdirs):
	make -C $@ $(MAKECMDGOALS)
