
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