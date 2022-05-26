// Connor Humiston
// Loadable Kernel Module Driver
#include <linux/init.h>				//initialization
#include <linux/module.h>			//LKM support
#include <linux/fs.h>				//manipulate files
#include <linux/uaccess.h>			//access data btw user/kernel
#include <linux/slab.h>				//manage memory

MODULE_AUTHOR("Connor Humiston");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("A simple character driver");

#define BUFF_SIZE 1024
#define MAJOR_NUMBER 240			//the driver number
#define DEVICE_NAME "PA2_Character_Device"

int char_device_open(struct inode*, struct file*);
ssize_t char_driver_read(struct file*, char*, size_t, loff_t*);
ssize_t char_driver_write(struct file*, const char*, size_t, loff_t*);
loff_t char_device_seek(struct file*, loff_t offset, int whence);
int char_driver_close(struct inode*, struct file*);
static int char_driver_init(void);
void char_driver_exit(void);

char* device_buffer;				//device buffer  memory
int opened = 0;					//# of times the file has been opened
int closed = 0;					//# of times the file has been closed


/* Log number of times device opened */
int char_driver_open(struct inode* pinode, struct file* filp)
{
	opened++;				//increment every time device opened
	printk(KERN_ALERT "Device opened: %d times\n", opened);
	return 0;
}

/* Read data from device buffer to the user */
ssize_t char_driver_read(struct file* filp, char __user* buff, size_t len, loff_t* offp)
{
	int numcopied;				//number of bytes copied
	int ret;				//return value of copy_to_user
	// Error Checking
	if(len > BUFF_SIZE - *offp)		//reading after the buffer (1024+)
	{
		printk(KERN_ALERT "ERROR: Attempting to read beyond the end of the device buffer.\n");
		return -1;
	}
	else if(*offp + len < 0 || *offp < 0)	//reading before the buffer (<0)
	{
		printk(KERN_ALERT "ERROR: Attempting to read from before the device buffer.\n");
		return -1;
	}
	else if(BUFF_SIZE - *offp == 0)
	{
		printk(KERN_ALERT "Reached the end of the device buffer.\n");
	}
	// Check if user space buffer is valid
	if(access_ok(VERIFY_WRITE, buff, len) <= 0)
	{
		printk(KERN_ALERT "The given user buffer to read data into is invalid.\n");
		return -1;
	}
	// Copy data from device buffer to user starting at the current position until requested #
	ret = copy_to_user(buff, device_buffer + *offp, len);
	// Return the Number of Bytes Read
	if (ret == 0)				//in this case, all the bytes were copied
	{
		printk(KERN_ALERT "Device has read %zu bytes.\n", len);
		*offp += len;			//refleccts that you are now len bytes ahead in file
		return len;
	}
	else if(ret > 0)			//for cases where not all bytes copied
	{
		numcopied = len - ret;
		printk(KERN_ALERT "Device has read %d bytes (%d less than requested).\n", numcopied, ret);
		*offp += len;			//update position on success (kernel will update f_pos)
		return numcopied;
	}
	else					//for any other case
	{
		printk(KERN_ALERT "There was an error reading from the device buffer.\n");
		return -1;
	}
}

/* Write data given by the user into the device buffer */
ssize_t char_driver_write(struct file* filp, const char __user* buff, size_t len, loff_t* offp)
{
	int ret;				//return value of copy_from_user
	int numcopied;				//number of bytes copied
	// Error Checking
	if(*offp + len > BUFF_SIZE)		//after the file
	{
		printk(KERN_ALERT "ERROR: Attempting to write beyond the end of the device buffer.\n");
		return -1;
	}
	else if(*offp + len < 0 || *offp < 0)	//before the file
	{
		printk(KERN_ALERT "ERROR: Attempting to write before the device buffer.\n");
		return -1;
	}
	// Check if user space buffer is valid
	if(access_ok(VERIFY_READ, buff, len) <= 0)
	{
		printk(KERN_ALERT "The given user buffer with data to write is invalid.\n");
		return -1;
	}
	// Copy data from the user buffer to the device buffer starting at the current position
	ret = copy_from_user(device_buffer + *offp, buff, len);
	// Return the # of bytes written
	if(ret == 0)				//no bytes were not written
	{
		printk(KERN_ALERT "Device has written %zu bytes.\n", len);
		*offp += len;			//position only updated if successful
		return len;
	}
	else if(ret > 0)			//not all bytes written
	{
		numcopied = len - ret;
		printk(KERN_ALERT "Device has written %d bytes (%d less than requested)\n", numcopied, ret);
		*offp += len;		//position updated if successful	
		return numcopied;
	}
	else
	{
		printk(KERN_ALERT "There was an error writing to the device buffer.\n");
		return -1;
	}
}

/* Update file position according to values of offset and whence */
loff_t char_driver_seek(struct file* filp, loff_t offset, int whence)
{
	int npos;				//the desired new position
	// Position Updates
	printk(KERN_ALERT "Current Position: %lld\n", filp->f_pos);
	switch(whence)
	{
		case 0:				//SEEK_SET (beginning of file)
			npos = offset;
			break;
		case 1:				//SEEK_CUR (current position)
			npos = filp->f_pos + offset;
			break;
		case 2:				//SEEK_END (end of file)
			npos = BUFF_SIZE + offset;
			break;
		default:
			printk(KERN_ALERT "Undefined value of whence. Choose 0, 1 or 2.\n");
			return -1;
	}
	// Bounds Checking
	if(npos > BUFF_SIZE)			//after the buffer (1024+)
	{
		printk(KERN_ALERT "ERROR: Attempting to set the file position beyond the file size.\n");
		return -1;
	}
	else if(npos < 0)			//before the buffer (<0)
	{
		printk(KERN_ALERT "ERROR: Attempting to set the file position before the device file.\n");
		return -1;
	}
	filp->f_pos = npos;			//set the new position
	printk(KERN_ALERT "New Position: %lld\n", filp->f_pos);
	return npos;
}

/* Log number of times device closed */
int char_driver_close(struct inode* pinode, struct file* pfile)
{
	closed++;				// increment every time device closed
	printk(KERN_ALERT "Device closed: %d times\n", closed);
	return 0;
}

/* File Operations Structure (for kernel to reference) */
struct file_operations char_device_ops = {
	.owner 		= THIS_MODULE,
	.open		= char_driver_open,
	.release	= char_driver_close,
	.read		= char_driver_read,
	.write		= char_driver_write,
	.llseek		= char_driver_seek
};

/* Initialize & Register Driver, allocate device buffer memory */
static int char_driver_init(void)
{
	int ret;				//driver registration return value
	ret = register_chrdev(MAJOR_NUMBER, DEVICE_NAME, &char_device_ops);
	if(ret < 0)				//ensure registered correctly
	{
		printk(KERN_ALERT "Failed to register major number.\n");
		return ret;
	}
	printk(KERN_ALERT "Registered Major Number: %d (in init)\n", MAJOR_NUMBER);
	// Allocate Buffer Memory
	device_buffer = kmalloc(BUFF_SIZE, GFP_KERNEL);
	memset(device_buffer, 0, 1024);
	return 0;
}

/* Free Memory & Unregister Driver */
void char_driver_exit(void)
{
	unregister_chrdev(MAJOR_NUMBER, DEVICE_NAME);
	kfree(device_buffer);
	printk(KERN_ALERT "Goodbye from the LKM!\n");
}

module_init(char_driver_init);
module_exit(char_driver_exit);
