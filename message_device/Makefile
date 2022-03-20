obj-m := message_slot.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
cflags-y := -std=gnu99
FLAGS += -03 -Wall -std=c11
MAJ0R_DEV_NUM := 240


all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
 
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
