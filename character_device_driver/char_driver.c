/*
 * Simple char driver.
 * Reference:
 *  Scull driver code mentioned in the book "Linux Device Drivers"
 *  by Alessandro Rubini and Jonathan Corbet, published
 *  by O'Reilly & Associates.
 */

/* linux headers */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>

/* driver hearders */
#include "char_driver.h"

/* global definitions */
int asp_major = 0;
int asp_minor = 0;
static int NUM_DEVICES = 3; /* default value of 3, unless specified during module load */
device_t *devices; /* array of devices */


static struct class *aspDriver;

/*
 * module params
 */
module_param(NUM_DEVICES, int, S_IRUGO);

/*******************
 * File operations *
 *******************/
static ssize_t
asp_driver_read (struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
  device_t *device = NULL;
  ssize_t bytes = 0; /* number of bytes read */

  device = get_device_from_file(file);
  if (!device) {
    printk("ASP:Error getting the device during read\n");
    return (-EINVAL);
  }

  if (down_interruptible(&device->sem)) {
    return (-ERESTARTSYS);
  }
  
  printk("ASP: disk data : %s", device->ramdisk);

  if (count <= 0) {
    goto done;
  }

  if (*ppos < 0) {
    goto done;
  }

  if (*ppos >= device->size) {
    printk("ASP: Error cannot read after end of file");
    goto done;
  }

  if ((*ppos + count) > device->size) {
    count = device->size - *ppos;
  }

  bytes = count - copy_to_user(buf, (device->ramdisk + *ppos), count);
  *ppos += bytes;

  printk("ASP_READ: <%s><%d> from device %d\n", buf, (int)bytes, device->dev_no);
  printk("ASP_READ: disk data : %s", device->ramdisk);
done:
  up(&device->sem);
  return (bytes);
}

static ssize_t
asp_driver_write (struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
  int nbytes = 0;
  device_t *device = NULL;

  printk("Write %s %d from %d\n", buf, (int)count, (int)*ppos);

  device = get_device_from_file(file);
  if (!device) {
    printk("ASP:Error getting device during write\n");
    return (-EINVAL);
  }

  if (down_interruptible(&device->sem)) {
    return (-ERESTARTSYS);
  }

  if (*ppos < 0) {
    goto done;
  }

  if ((*ppos + count) > device->size) {
    count = device->size - *ppos;
  }

  nbytes = count - copy_from_user((device->ramdisk + *ppos), buf, count);
  *ppos += nbytes;

  printk("ASP_WRITE: Written %d bytes to device %d new f_pos %llu\n",
         nbytes, device->dev_no, *ppos);
  printk("ASP_WRITE: disk data : %s", device->ramdisk);

done:
  up(&device->sem);
  return (nbytes);
}

static loff_t 
asp_driver_llseek (struct file *file, loff_t offset, int whence)
{
  device_t *device = NULL;
  loff_t   pos;
  char *new_disk = NULL;

  device = get_device_from_file(file);
  if (!device) {
    printk("ASP: Error getting the device during lseek\n");
    return (-EINVAL);
  }

  switch (whence) {
  case SEEK_SET:
    pos = offset;
    break;
  case SEEK_CUR:
    pos = file->f_pos + offset;
    break;
  case SEEK_END:
    pos = device->size + offset;
    break;
  default:
    printk("ASP: Invalid start position for lseek\n");
    return (-EINVAL);
  }

  if (pos < 0) {
    return (-EINVAL);
  }

  /*
   * expand the buffer if needed
   * make sure to take the lock before expanding the buffer
   */
  if (pos > device->size) {

    printk("Expanding disk size for device %d from %d to %d bytes\n", device->dev_no, device->size, (int) pos);

    /*
     * create the new disk and memset to 0
     */
    new_disk = kmalloc(pos, GFP_KERNEL);
    if (!new_disk) {
      return (-ENOMEM);
    }
    memset(new_disk, 0, pos);

    /*
     * copy to new
     * delete old
     * associate device to new disk
     */
    if (down_interruptible(&device->sem)) {
      return (-ERESTARTSYS);
    }

    memcpy(new_disk, device->ramdisk, device->size);

    free_ramdisk(device);

    device->ramdisk = new_disk;
    device->size = pos;

    up(&device->sem);
  }

  file->f_pos = pos;
  return (pos);
}

static int
asp_driver_open (struct inode *inode, struct file *file)
{
  device_t *device;

  device = container_of(inode->i_cdev, device_t, cdev);
  file->private_data = device; /* to fetch device in other APIs */

  printk("ASP: Opening device %d\n", device->dev_no);
  
  return (0);
}

static int 
asp_driver_release (struct inode *inode, struct file *file)
{
  device_t *device = NULL;

  device = get_device_from_file(file);
  if (!device) {
    printk(KERN_ERR "ASP: Could not get device during release\n");
    return (-EINVAL);
  }

  printk("ASP: Releasing device %d\n", device->dev_no);
  return (0);
}

static long
asp_driver_ioctl (struct file *file, unsigned int cmd, unsigned long arg)
{
  //int err = 0;
  int ret = 0;
  device_t *device = NULL;
  
  device = get_device_from_file(file);
  if (!device) {
    printk("ASP: Error getting the device during lseek\n");
    return (-EINVAL);
  }

  if (_IOC_TYPE(cmd) != CDRV_IOC_MAGIC)
    return (-ENOTTY);

  if (_IOC_NR(cmd) > ASP_IOC_MAX_NUM)
    return (-ENOTTY);

#if 0
  /*
   * the direction is a bitmask, and VERIFY_WRITE catches R/W
   * transfers. `Type' is user-oriented, while
   * access_ok is kernel-oriented, so the concept of "read" and
   * "write" is reversed
   */
  if (_IOC_DIR(cmd) & _IOC_READ)
    err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
  else if (_IOC_DIR(cmd) & _IOC_WRITE)
    err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
  if (err) return -EFAULT;
#endif

  switch (cmd) {
  case ASP_CLEAR_BUF:

    if (down_interruptible(&device->sem)) {
      return (-ERESTARTSYS);
    }

    memset(device->ramdisk, 0, device->size);
    file->f_pos  = 0;

    printk("ASP_IOCTL: Cleared disk for device :%d\n", device->dev_no);
    up(&device->sem);
    
    break;
  default:
    break;
  }

  return (ret);
}

struct file_operations asp_driver_fops = {
  .owner = THIS_MODULE,
  .read = asp_driver_read,
  .write = asp_driver_write,
  .llseek = asp_driver_llseek,
  .unlocked_ioctl = asp_driver_ioctl,
  .open = asp_driver_open,
  .release = asp_driver_release,
};

/*************************
 * Module init/exit APIs *
 *************************/

static void
device_cleanup (device_t *device)
{
  if (!device) {
    return;
  }
 
  device_destroy(aspDriver, MKDEV(asp_major, device->dev_no));
  cdev_del(&device->cdev);
  free_ramdisk(device);
  
  return;
}

static void __exit
asp_driver_exit (void)
{
  int i = 0;
  dev_t dev_no = 0;

  dev_no = MKDEV(asp_major, asp_minor);

  if (devices) {
    for (i = 0; i < NUM_DEVICES; i++) {
      device_cleanup(&devices[i]);
    }

    kfree(devices);
    devices = NULL;
  }

  class_destroy(aspDriver);
  unregister_chrdev_region(dev_no, NUM_DEVICES);
  printk("ASP: Exit ASP driver\n");
  return;
}


/*
 * device init
 */
static bool
device_init (device_t *device, int index, struct class *class)
{
  struct device *dev;
  int ret = 0;
  int devno = 0;

  if (!device) {
    return (false);
  }

  /*
   * associate the device number
   */
  device->dev_no = index;

  /*
   * Initialize the semaphore
   */
  sema_init(&device->sem, BINARY_SEMAPHORE);

  /*
   * create a ramdisk for the device
   */
  device->ramdisk = kmalloc(ramdisk_size, GFP_KERNEL);;
  if (!device->ramdisk) {
    printk("ASP: Could not create ramdisk for device %d\n", index);
    return (false);
  }
  device->size = ramdisk_size;

  /*
   * setup a cdev for this device
   */
  devno = MKDEV(asp_major, asp_minor + index);
  cdev_init(&device->cdev, &asp_driver_fops);
  device->cdev.owner = THIS_MODULE;
  device->cdev.ops = &asp_driver_fops;
  ret = cdev_add(&device->cdev, devno, 1);
  if (ret != 0) {
    printk("ASP: Error %d adding device %d\n", ret, index);
    free_ramdisk(device);
    return (false);
  }

  dev = device_create(class, NULL, devno, NULL, MYDEV_NAME "%d", index);

  return (true);
}

static int __init
asp_driver_init (void)
{
  int i = 0;
  int ret = 0;
  dev_t dev = 0;

  /*
   * dynamically allocate a major number for the asp driver
   */
  ret = alloc_chrdev_region(&dev, asp_minor, NUM_DEVICES, MYDEV_NAME);
  if (ret < 0) {
    printk("ASP: Could not allocate a major number\n");
    return (ret);
  }
  asp_major = MAJOR(dev);

  /* allocate memory for the devices */
  devices = kmalloc(NUM_DEVICES * sizeof(device_t), GFP_KERNEL);
  if (!devices) {
    asp_driver_exit();
    return (-ENOMEM);
  }
  memset(devices, 0, NUM_DEVICES * sizeof(device_t));

  /*
   * create a device class
   */
  aspDriver = class_create(THIS_MODULE, MYDEV_NAME);

  /*
   * Initialize each device
   */
  for (i = 0; i < NUM_DEVICES; i++) {
    device_init(&devices[i], i, aspDriver);
  }

  printk("ASP: char driver initialized\n");
  return (0);
}

module_init(asp_driver_init);
module_exit(asp_driver_exit);

MODULE_AUTHOR("Neel Kanjaria");
MODULE_LICENSE("GPL v2");
