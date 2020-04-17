
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