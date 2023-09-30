#include <linux/init.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/cdev.h>

MODULE_AUTHOR("mekb-turtle");
MODULE_DESCRIPTION("Example kernel module to display \"hello world\" in a character device file");
MODULE_LICENSE("GPL");

static ssize_t custom_read(struct file* file, char __user* user_buffer, size_t count, loff_t* offset)
{
	// String to return
    const char *hello_str = "Hello World!\n";

	// Length of string
    size_t len = strlen(hello_str);

    // Check if we've reached the end of the file
    if (*offset >= len) return 0;

	// Check if we're out of bounds
	if (*offset < 0) return -ERANGE;

    // Determine how many bytes we can read (up to 'count')
    size_t bytes_to_copy = min(len - (size_t)*offset, count);

    // Copy data from the 'hello_str' to the user-space buffer
    if (copy_to_user(user_buffer, hello_str + (size_t)*offset, bytes_to_copy)) {
		// Error copying data to user-space buffer
        return -EFAULT;
    }

    // Update the file position
    *offset += bytes_to_copy;

    return bytes_to_copy;
}

static struct file_operations ops = { // file operations
	.owner = THIS_MODULE,
	.read = custom_read
};

static unsigned num_devices = 1; // number of char devices we are allocating
static dev_t dev_id; // device ID
static struct cdev cdev; // character device, contains major/minor number, operations, etc
static struct class* class; // class, like a group of devices, e.g input, block, char, net
static struct device* device; // the device itself, containing the class, device ID, and a name

static int __init module_custom_init(void)
{
	int retval;

	printk(KERN_INFO "Hello World!\n");

	// Find an available device ID
	if ((retval = alloc_chrdev_region(&dev_id, 20, num_devices, "helloworld")) < 0) {
		printk(KERN_WARNING "Failed to allocate chrdev region\n");
		return retval;
	}

	// Initialize character device
	cdev_init(&cdev, &ops);
	if ((retval = cdev_add(&cdev, dev_id, num_devices)) < 0) {
		// Unregister device ID before returning
		unregister_chrdev_region(dev_id, num_devices);
		printk(KERN_WARNING "Failed to add cdev\n");
		return retval;
	}

	// Initialize class
	class = class_create("helloworld_class");

	// Initialize device with the class, device ID, and a name
	// The name will be used as the filename, e.g /dev/helloworld
	device = device_create(class, NULL, dev_id, NULL, "helloworld");

	printk(KERN_INFO "Character device file loaded\n");

	return 0;
}

static void __exit module_custom_exit(void)
{
	printk(KERN_INFO "Goodbye\n");

	// Clean up
	device_destroy(class, dev_id);
	class_destroy(class);
	cdev_del(&cdev);
	unregister_chrdev_region(dev_id, num_devices);
}

// Init/exit functions
module_init(module_custom_init);
module_exit(module_custom_exit);

