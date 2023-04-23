MODULE=ram_blockdev
obj-m = $(MODULE).o
KVERSION = $(shell uname -r)
CC=gcc

all:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules

load_module: $(MODULE).ko
	insmod $(MODULE).ko

unload_module:
	rmmod $(MODULE)

check_module:
	lsmod | grep $(MODULE)

create_dev:
	cd /dev
	mknod $(DEV) c $(MAJOR) $(MINOR)
	cd $(PWD)

rm_device:
	cd /dev
	rm -rf $(DEV)

print_kernel_log:
	cat /var/log/syslog

clean:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean
