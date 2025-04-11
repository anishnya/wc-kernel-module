obj-m :=wc_ram.o

all:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build/ M=$(shell pwd) modules
clean:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build/ M=$(shell pwd) clean
