MODULE=simple_lifo_chrdev
obj-m = $(MODULE).o
KVERSION = $(shell uname -r)
CC=gcc
CXX=g++

TEST_OBJS=test.cpp
TEST_BIN=test

all:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules

test.o: test.cpp
	$(CXX) -o $(TEST_BIN) $(TEST_OBJS) 

reload_module:
	unload_module && load_module

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
	dmesg

clean:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean
	rm -rf *.o $(TEST_BIN)