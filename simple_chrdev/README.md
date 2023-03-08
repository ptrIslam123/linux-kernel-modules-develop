### Build
`
 $ make
`

### Load and unload module
`
$ sudo make load_module 
// load module
`

`
$ sudo make unload_module
// unload module
`

### Demonstrate
`$ make
// build the module
`

`
$ sudo make load_module
// load module to kernel space
`

`
$ make check_module
// check module status. If this command will display module name it is mean that we load module successful
`

`
$ make main_test.o
// build simple program to test module
`

`
$ sudo make print_kernel_log 
// print kenel log file to see that do this module.
`

`
$ sudo mknod <DEVICE> c <MAJOR> 0
 // You need find registered major number of our module to kernel log file to create node with number 
 // create character node with our major number. Warning: you need create this node to /dev directory
`

`
$ ls -l <DEVICE>
// in /dev directory you must see your node. This node is file - door to our loaded module
`

`
$ sudo ./main_test /dev/<DEVICE> <some-message-test>
// this program open, write your message to device, read message from device and finaly close opened device
`

### Example
`
 $ make
`

`
 $ make main_test.o
`

`
 $ sudo make load_module
 `

`
 $ cd /dev && sudo mknod mydev c 244 0
 `

 `
 $ sudo ./main_test /dev/mydev hellow_world
`