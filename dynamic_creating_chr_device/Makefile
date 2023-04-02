MODULE=dynamic_chr_dev
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

print_kernel_log:
	cat /var/log/syslog

clean:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean
