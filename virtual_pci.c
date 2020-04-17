
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