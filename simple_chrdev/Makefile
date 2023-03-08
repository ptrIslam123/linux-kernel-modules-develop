MODULE=simple_chrdev
obj-m = $(MODULE).o
KVERSION = $(shell uname -r)
CC=gcc

TEST_OBJS=main_test.c
TEST_BIN=main_test

all:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules

main_test.o: main_test.c
	$(CC) -o $(TEST_BIN) $(TEST_OBJS) 

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