#ifndef __CHAR_DRIVER_H__
#define __CHAR_DRIVER_H__

#define BINARY_SEMAPHORE 1
#define MYDEV_NAME "mycdrv"
#define ramdisk_size (size_t) (16 * PAGE_SIZE) /* ramdisk size */ 
#define CDRV_IOC_MAGIC 'Z'
#define ASP_CLEAR_BUF _IOW(CDRV_IOC_MAGIC, 1, int)
#define ASP_IOC_MAX_NUM 1

/*
 * device specific staructure
 * one instance represents one device
 */
typedef struct device_t_ {
	int dev_no; /* device number */
	struct semaphore sem; /* semaphore for mutual exclusion */
	struct cdev cdev; /* embedded cdev struct for this device */
        int size;
	char *ramdisk; /* memory of the device */
} device_t;

static inline device_t *
get_device_from_file (struct file *file)
{
  device_t *device = NULL;

  if (!file) {
    return (NULL);
  }

  device = file->private_data;
  if (!device) {
    return (NULL);
  }

  if (!device->ramdisk) {
    return (NULL);
  }

  return device;
}

static inline void
free_ramdisk (device_t *device)
{
  if (!device) {
    return;
  }

  kfree(device->ramdisk);
  device->size = 0;
  device->ramdisk = NULL;
}

#endif /* __CHAR_DRIVER_H__ */
