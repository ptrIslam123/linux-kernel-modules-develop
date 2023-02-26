### Compile linux module:
`$ make`

### Clean working directory:
`$ make clean`

### Insert our modude to linux-kernel:
`$ sudo insmode <module-name>.ko`

### Remove our module from linux-kernel:
`$ sudo rmmod <module-name>`

### Check our module loading status:
`$ lsmod | grep -i "<module-name>"` 

### See linux log messages:
`$ sudo tail /var/log/message`

`$ sudo dmesg`

### See for more information about linux-modules develop:
https://www.cyberciti.biz/tips/compiling-linux-kernel-module.html

https://habr.com/ru/post/343828/