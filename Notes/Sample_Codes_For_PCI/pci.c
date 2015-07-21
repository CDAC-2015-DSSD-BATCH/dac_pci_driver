#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/pci.h>
#include<linux/init.h>
#include<linux/version.h>
#include<linux/ioport.h>
#include<linux/device.h>
#include<linux/slab.h>
#include<linux/string.h>
#include<linux/fs.h>
#include<linux/types.h>
#include<linux/cdev.h>
#include<linux/errno.h>
#include<asm/unistd.h>
#include<asm/uaccess.h>
#include<asm/io.h>
#include<asm/fcntl.h>
#include<asm/errno.h>


MODULE_AUTHOR ("THIS_MODULE");
MODULE_LICENSE ("GPL");

static int __init init_pci(void);
static void __exit exit_pci(void);

static int __init init_pci(void)
{
	struct pci_dev *pdev = NULL;
	int i=0;
	for_each_pci_dev(pdev)
	{
		printk("Found pci device : %d \n",i++);
		printk("Device information : \n");
		printk("Geographical address : %s\n",pci_name(pdev));
		printk("Vendor id : %hu\n",pdev->vendor);
		printk("Device id : %hu\n",pdev->device);
		printk("Subvendor id : %hu\n",pdev->subsystem_vendor);
		printk("Subdevice id : %hu\n",pdev->subsystem_device);
		printk("Class : %hu\n",pdev->class);
		printk("Configuration size : %d\n",pdev->cfg_size);
		printk("Bus number : %d\n",pdev->bus->number);
		printk("Bridge controllers : %d\n",pdev->bus->bridge_ctl);
		printk("Primary bridge number : %d\n",pdev->bus->primary);
		printk("Secondary Bridge number : %d\n",pdev->bus->secondary);
		printk("Higest number of bus from bridge : %d\n",pdev->bus->subordinate);
		printk("Irq number : %d\n",pdev->irq);

		pci_dev_put(pdev);
	}
	return 0;
}
static void __exit exit_pci(void)
{
	printk("Inside exit module\n");
}

module_init(init_pci);
module_exit(exit_pci);

