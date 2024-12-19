#include <linux/module.h>   // Required for all kernel modules
#include <linux/kernel.h>   // Required for KERN_INFO
#include <linux/kprobes.h>  // Required for kprobe functionalities
#include <linux/uaccess.h>  // Required for copy_to_user function
#include <linux/slab.h>     // Required for kmalloc and kfree functions
#include <linux/fs.h>       // Required for file system structures
#include <linux/dcache.h>   // Required for d_path function

MODULE_LICENSE("GPL");  // Specifies the license type for the module
MODULE_AUTHOR("Baris");  // Specifies the author of the module
MODULE_DESCRIPTION("Kernel module to intercept and manipulate file operations");  // Provides a description of the module

static char *target_path = "/data/local/tmp/log";  // Specifies the target path to intercept
module_param(target_path, charp, 0000);  // Allows setting the target path as a module parameter
MODULE_PARM_DESC(target_path, "Target path to intercept and manipulate file operations");  // Provides a description for the module parameter

static struct kprobe kp;  // Defines a kprobe structure
static asmlinkage ssize_t (*original_write)(struct file *, const char __user *, size_t, loff_t *);  // Function pointer to store the original write function

static int pre_handler(struct kprobe *p, struct pt_regs *regs) {  // Pre-handler function for the kprobe
    struct file *file;  // File structure pointer
    char __user *buf;  // User space buffer pointer
    size_t count;  // Size of the buffer
    loff_t *pos;  // File position
    char *kbuf;  // Kernel buffer pointer
    char *path_buf;  // Path buffer pointer
    char *full_path;  // Full path pointer

#ifdef CONFIG_X86_64  // Checks if the architecture is x86_64
    file = (struct file *)regs->di;  // Retrieves the file parameter from the registers
    buf = (char __user *)regs->si;  // Retrieves the buffer parameter from the registers
    count = (size_t)regs->dx;  // Retrieves the count parameter from the registers
    pos = (loff_t *)regs->cx;  // Retrieves the position parameter from the registers
#else  // If the architecture is not x86_64
    return 0;  // Return without doing anything
#endif

    path_buf = (char *)__get_free_page(GFP_KERNEL);  // Allocates a page of memory for the path buffer
    if (!path_buf) {  // Checks if the allocation failed
        printk(KERN_ERR "Failed to allocate memory for path\n");  // Prints an error message
        return 0;  // Return without doing anything
    }

    full_path = d_path(&file->f_path, path_buf, PAGE_SIZE);  // Retrieves the full path of the file
    if (IS_ERR(full_path)) {  // Checks if the path retrieval failed
        printk(KERN_ERR "Failed to get file path\n");  // Prints an error message
        free_page((unsigned long)path_buf);  // Frees the allocated page
        return 0;  // Return without doing anything
    }

    if (strcmp(full_path, target_path) == 0) {  // Checks if the full path matches the target path
        kbuf = kmalloc(5, GFP_KERNEL);  // Allocates memory for the buffer with 5 bytes
        if (!kbuf) {  // Checks if the allocation failed
            printk(KERN_ERR "Failed to allocate memory\n");  // Prints an error message
            free_page((unsigned long)path_buf);  // Frees the allocated page
            return 0;  // Return without doing anything
        }
        memset(kbuf, 'X', 5);  // Fills the buffer with 'X' characters

        if (copy_to_user(buf, kbuf, 5)) {  // Copies the modified data back to the user space
            printk(KERN_ERR "Failed to copy data to user\n");  // Prints an error message
        }

        kfree(kbuf);  // Frees the allocated buffer
        regs->dx = 5;  // Modifies the count to reflect the new data size
    }

    free_page((unsigned long)path_buf);  // Frees the allocated path buffer
    return 0;  // Return without doing anything
}

static int __init my_module_init(void) {  // Initialization function for the module
    int ret;  // Return value

    printk(KERN_INFO "Module loaded\n");  // Prints a message indicating the module is loaded

    kp.pre_handler = pre_handler;  // Sets the pre-handler function for the kprobe
    kp.symbol_name = "vfs_write";  // Sets the symbol name to intercept

    original_write = (asmlinkage ssize_t (*)(struct file *, const char __user *, size_t, loff_t *)) kallsyms_lookup_name("vfs_write");  // Retrieves the original write function address

    ret = register_kprobe(&kp);  // Registers the kprobe
    if (ret < 0) {  // Checks if the registration failed
        printk(KERN_ERR "Failed to register kprobe: %d\n", ret);  // Prints an error message with the return value
        return ret;  // Return the error code
    }

    return 0;  // Return 0 indicating success
}

static void __exit my_module_exit(void) {  // Exit function for the module
    unregister_kprobe(&kp);  // Unregisters the kprobe
    printk(KERN_INFO "Module unloaded\n");  // Prints a message indicating the module is unloaded
}

module_init(my_module_init);  // Macro to specify the initialization function
module_exit(my_module_exit);  // Macro to specify the exit function
