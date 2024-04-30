#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

#include "my_ioctl.h"

#include <string>
#include <thread>
#include <chrono>
#include <iostream>

constexpr auto MMAP_SIZE = (4096);

void readFrom(int fd, std::string& buff, const int size) {
	buff.resize(size);

	if (read(fd, (void*)buff.data(), buff.size()) < 0) {
		fprintf(stderr, "read op ws failed\n");
		exit(-1);
	}
}

void writeTo(int fd, const std::string& buff) {
	if (write(fd, (void*)buff.data(), buff.size()) < 0) {
		fprintf(stderr, "write op was failed\n");
		exit(-1);
	}
}

void ctl_get_module_info(int fd)
{
	module_info_struct_t module_info = {0};
	if (ioctl(fd, MY_LIFO_CHAR_DEV_GET_MODULE_INFO, &module_info) < 0) {
		fprintf(stderr, "ioctl op was failed'\n");
		exit(-1);
	}

	std::cout << "Module{name=" << module_info.name << ", version=" << module_info.version << ", author=" << module_info.author << std::endl;
}

int ctl_get_dev_buff_size(int fd) {
	int size = 0;
	if (ioctl(fd, MY_LIFO_CHAR_DEV_GET_DEV_BUF_SIZE, &size) < 0) {
		fprintf(stderr, "ioctl op was failed'\n");
		exit(-1);
	}

	return size;
}

void test_read_write_async(int fd)
{
	std::thread writer([fd](){
		std::cout << "Started writer thread" << std::endl;
		
		std::string msg = "$";

		for (auto i = 0; i < 16; ++i) {
			writeTo(fd, msg);
			std::cout << "WRITE: " << msg[0] << std::endl;
		}

		std::cout << "End writer thread" << std::endl;
	});

	std::string msg;
	readFrom(fd, msg, 16);

	std::cout << "READ: " << msg << std::endl;

	writer.join();
}

void test_write_read(int fd)
{
	std::string msg = "Some test message for my character linux device module to test file I/O";
	writeTo(fd, msg);
	std::cout << "WRITE: " << msg << std::endl;

	int size = ctl_get_dev_buff_size(fd);
	std::cout << "Curr device buffer size=" << size << std::endl;

	msg.clear();
	readFrom(fd, msg, size);

	std::cout << "READ: " << msg << std::endl;
}

void test_mmap(int fd)
{
	std::string msg = "Some test message fro my character linux device module to test mmap I/O";
	writeTo(fd, msg);
	std::cout << "WRITE: " << msg << std::endl;

	// Memory map the file
    void* device_mem_start = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);  
    if (device_mem_start == MAP_FAILED) {
        perror("mmap op failed");
        exit(EXIT_FAILURE);
    }

	std::cout << "MMAP: " << (char*)device_mem_start << std::endl;

	// Unmap the memory
    (void)munmap(device_mem_start, 4096);
}

void test_polling(int fd) {
	std::string msg = "Some test message for my character linux device module to test poll I/O";
	std::thread writer([fd, msg]() {
		std::this_thread::sleep_for(std::chrono::seconds(2));
		std::cout << "Started writer thread" << std::endl;

		std::cout << "WRITE: " << msg << std::endl;
		writeTo(fd, msg);
		
		std::cout << "Turn on polling for reading" << std::endl;
		std::cout << "End writer thread" << std::endl;
	});

	fd_set read_fds;

	/* Initialize the file descriptor set */
    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);

	/* Call select */
	std::cout << "Main thread: do select syscall" << std::endl;

    auto ret = select(fd + 1, &read_fds, nullptr, nullptr, nullptr);
    if (ret == -1) {
        perror("select");
        exit(EXIT_FAILURE);
    }

	std::cout << "Main thread: after select syscall" << std::endl;

	if (ret > 0) {
		std::cout << "Main thread: device is ready for reading!!!" << std::endl;
		
		const auto size = msg.size();
		msg.clear();
		readFrom(fd, msg, size);

		std::cout << "READ: " << msg << std::endl;
	} else {
		std::cerr << "Main thread: device is not ready for reading!!!" << std::endl;
	}

	writer.join();
}

int main(int argc, char** argv) {
	const std::string file_path = argv[1];
	const std::string opt = argv[2];

   	if (argc < 3) {
        fprintf(stderr, "Pass wrong args: %s <file-path> <option> <args ...>\n", argv[0]);
        return -1;
    }

    const int fd = open(file_path.data(), O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Can`t open file=%s. return exit code=%d\n", file_path, fd);
        return -1;
    }

	if (opt == "--test-write-read") {
		test_write_read(fd);
	} else if (opt == "--test-read-write-async") {
		test_read_write_async(fd);
	} else if (opt == "--test-ioctl") {
		ctl_get_module_info(fd);
	} else if (opt == "--test-mmap") {
		test_mmap(fd);
	} else if (opt == "--test-polling") {
		test_polling(fd);
	} else {
		std::cerr << "Unknown operation" << opt << std::endl;
	}

    if (close(fd) < 0) {
        fprintf(stderr, "Can`t close opened file=%s\n", argv[1]);
        return -1;
    }    

    return 0;
}
