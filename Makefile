# Define the module name (without .ko)
obj-m := write.o

# Set the kernel build directory
KDIR := /lib/modules/$(shell uname -r)/build

# Default build target
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# Clean target
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

