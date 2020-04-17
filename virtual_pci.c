
#include <linux/init.h>
#include <linux/module.h>
   
static int my_init(void)
{
                       return  0;
}
   
static void my_exit(void)
{
                       return;
}
   

module_init(my_init);
module_exit(my_exit);

static struct pci_device_id pci_ids[] = {
  { PCI_DEVICE(0xabcd, 0xabcd), },
  { 0, }
};


MODULE_DEVICE_TABLE(pci, pci_ids);

static dev_t devno;
static int major;


struct pci_cdev {
  int minor;
  struct pci_dev *pci_dev;
  struct cdev *cdev;
};

static struct pci_cdev pci_cdev[MAX_DEVICE];

static void pci_cdev_init(struct pci_cdev pci_cdev[], int size, int first_minor)
{
	int i;

	for(i=0; i<size; i++) {
		pci_cdev[i].minor   = first_minor++;
		pci_cdev[i].pci_dev = NULL;
		pci_cdev[i].cdev    = NULL;
	}
}

static int pci_cdev_add(struct pci_cdev pci_cdev[], int size, struct pci_dev *pdev)
{
	int i, res = -1;

	for(i=0; i<size; i++) {
		if (pci_cdev[i].pci_dev == NULL) {
			pci_cdev[i].pci_dev = pdev;
			res = pci_cdev[i].minor;
			break;
		}
	}
	
	return res;
}

static void pci_cdev_del(struct pci_cdev pci_cdev[], int size, struct pci_dev *pdev)
{
  int i;

  for(i=0; i<size; i++) {
    if (pci_cdev[i].pci_dev == pdev) {
      pci_cdev[i].pci_dev = NULL;
    }
  }
}

static struct pci_dev *pci_cdev_search_pci_dev(struct pci_cdev pci_cdev[], int size, int minor)
{
  int i;
  struct pci_dev *pdev = NULL;

  for(i=0; i<size; i++) {
    if (pci_cdev[i].minor == minor) {
      pdev = pci_cdev[i].pci_dev;
      break;
    }
  }

  return pdev;  
}

static struct cdev *pci_cdev_search_cdev(struct pci_cdev pci_cdev[], int size, int minor)
{
	int i;
	struct cdev *cdev = NULL;

	for(i=0; i<size; i++) {
		if (pci_cdev[i].minor == minor) {
			cdev = pci_cdev[i].cdev;
			break;
		}
	}

	return cdev;	
}


static int pci_cdev_search_minor(struct pci_cdev pci_cdev[], 
		int size, struct pci_dev *pdev)
{
	int i, minor = -1;

	for(i=0; i<size; i++) {
		if (pci_cdev[i].pci_dev == pdev) {
			minor = pci_cdev[i].minor;
			break;
		}
	}

	return minor;
}

static int pci_open(struct inode *inode, struct file *file)
{
  int minor = iminor(inode);
  file->private_data = (void *)pci_cdev_search_pci_dev(pci_cdev, MAX_DEVICE, minor);
  return 0;
}

static int pci_release(struct inode *inode, struct file *file)
{
  return 0;
}

static ssize_t pci_read(struct file *file,  /* see include/linux/fs.h   */
         char *buffer,  /* buffer to fill with data */
         size_t length,  /* length of the buffer     */
         loff_t * offset)
{
  int byte_read = 0;
  unsigned char value;
  struct pci_dev *pdev = (struct pci_dev *)file->private_data;
  unsigned long pci_io_addr = 0;

  pci_io_addr = pci_resource_start(pdev,BAR_IO);

  while (byte_read < length) {
    /* read a byte from the input */
    value = inb(pci_io_addr + 1);

    /* write the value in the user buffer */
    put_user(value, &buffer[byte_read]);

    byte_read++;
  }

  return byte_read;
}

static ssize_t pci_read(struct file *file,	/* see include/linux/fs.h   */
			   char *buffer,	/* buffer to fill with data */
			   size_t length,	/* length of the buffer     */
			   loff_t * offset)
{
	int byte_read = 0;
	unsigned char value;
	struct pci_dev *pdev = (struct pci_dev *)file->private_data;
	unsigned long pci_io_addr = 0;

	pci_io_addr = pci_resource_start(pdev,BAR_IO);

	while (byte_read < length) {
		/* read a byte from the input */
		value = inb(pci_io_addr + 1);

		/* write the value in the user buffer */
		put_user(value, &buffer[byte_read]);

		byte_read++;
	}

	return byte_read;
}


static ssize_t pci_write(struct file *filp, const char *buffer, size_t len, loff_t * off) {
	int i;
	unsigned char value;
	struct pci_dev *pdev = (struct pci_dev *)filp->private_data;
	unsigned long pci_io_addr = 0;

	pci_io_addr = pci_resource_start(pdev,BAR_IO);
	
	for(i=0; i<len; i++) {
		/* read value on the buffer */
		value = (unsigned char)buffer[i];

		/* write data to the device */
		outb(pci_io_addr+2, value);
	}

	return len;
}
static struct file_operations pci_ops = {
  .owner    = THIS_MODULE,
  .read     = pci_read,
  .write     = pci_write,
  .open     = pci_open,
  .release   = pci_release
};


static int pci_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
  int ret, minor;
  struct cdev *cdev;
  dev_t devno;

  /* add this pci device in pci_cdev */
  if ((minor = pci_cdev_add(pci_cdev, MAX_DEVICE, dev)) < 0)
    goto error;

  /* compute major/minor number */
  devno = MKDEV(major, minor);

  /* allocate struct cdev */
  cdev = cdev_alloc();

  /* initialise struct cdev */
  cdev_init(cdev, &pci_ops);
  cdev->owner = THIS_MODULE;

  /* register cdev */
  ret = cdev_add(cdev, devno, 1);
  if (ret < 0) {
    dev_err(&(dev->dev), "Can't register character device\n");
    goto error;
  }
  pci_cdev[minor].cdev = cdev;

  dev_info(&(dev->dev), "%s The major device number is %d (%d).\n",
         "Registeration is a success", MAJOR(devno), MINOR(devno));
  dev_info(&(dev->dev), "If you want to talk to the device driver,\n");
  dev_info(&(dev->dev), "you'll have to create a device file. \n");
  dev_info(&(dev->dev), "We suggest you use:\n");
  dev_info(&(dev->dev), "mknod %s c %d %d\n", DEVICE_NAME, MAJOR(devno), MINOR(devno));
  dev_info(&(dev->dev), "The device file name is important, because\n");
  dev_info(&(dev->dev), "the ioctl program assumes that's the\n");
  dev_info(&(dev->dev), "file you'll use.\n");

  /* enable the device */
  pci_enable_device(dev);

  /* 'alloc' IO to talk with the card */
  if (pci_request_region(dev, BAR_IO, "IO-pci") == 0) {
    dev_err(&(dev->dev), "Can't request BAR2\n");
    cdev_del(cdev);
    goto error;
  }

  /* check that BAR_IO is *really* IO region */
  if ((pci_resource_flags(dev, BAR_IO) & IORESOURCE_IO) != IORESOURCE_IO) {
    dev_err(&(dev->dev), "BAR2 isn't an IO region\n");
    cdev_del(cdev);
    goto error;
  }

  return 1;

error:
  return 0;
}
static void pci_remove(struct pci_dev *dev)
{
  int minor;
  struct cdev *cdev;

  /* remove associated cdev */
  minor = pci_cdev_search_minor(pci_cdev, MAX_DEVICE, dev);
  cdev = pci_cdev_search_cdev(pci_cdev, MAX_DEVICE, minor);
  if (cdev != NULL) 
    cdev_del(cdev);
    
  /* remove this device from pci_cdev */
  pci_cdev_del(pci_cdev, MAX_DEVICE, dev);

  /* release the IO region */
  pci_release_region(dev, BAR_IO);
}


static struct pci_driver pci_driver = {
  .name     = "pci",
  .id_table   = pci_ids,
  .probe     = pci_probe,
  .remove   = pci_remove,
};