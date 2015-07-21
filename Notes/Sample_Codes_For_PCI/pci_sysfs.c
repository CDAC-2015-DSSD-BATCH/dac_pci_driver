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

ssize_t show_vendor_id(struct class_device *, char*);
ssize_t show_device_id(struct class_device *, char*);
ssize_t show_irq_no(struct class_device *, char*);
ssize_t show_class(struct class_device *, char*);

static struct class *pci_class;
static struct class_device *cls_dev;
dev_t dev;

CLASS_DEVICE_ATTR(vendor_id,S_IRUGO,show_vendor_id,NULL);
CLASS_DEVICE_ATTR(device_id,S_IRUGO,show_device_id,NULL);
CLASS_DEVICE_ATTR(irq_no,S_IRUGO,show_irq_no,NULL);
CLASS_DEVICE_ATTR(class,S_IRUGO,show_class,NULL);

static int __init init_pci(void);
static void __exit exit_pci(void);

ssize_t show_vendor_id(struct class_device *temp, char *buf)
{
	struct pci_dev *tmp_dev;
	tmp_dev = temp->class_data;
	return snprintf(buf,PAGE_SIZE,"Vendor id : %x\n",tmp_dev->vendor);
}
ssize_t show_device_id(struct class_device *temp, char *buf)
{
	struct pci_dev *tmp_dev;
	tmp_dev = temp->class_data;
	return snprintf(buf,PAGE_SIZE,"device id : %x\n",tmp_dev->device);
}
ssize_t show_irq_no(struct class_device *temp, char *buf)
{
	struct pci_dev *tmp_dev;
	tmp_dev = temp->class_data;
	return snprintf(buf,PAGE_SIZE,"Irq no : %d\n",tmp_dev->irq);
}
ssize_t show_class(struct class_device *temp, char *buf)
{
	struct pci_dev *tmp_dev;
	tmp_dev = temp->class_data;
	return snprintf(buf,PAGE_SIZE,"Class : %x\n",tmp_dev->class);
}


static int __init init_pci(void)
{
	struct pci_dev *pdev=NULL;
	int i=0;
	pci_class = class_create(THIS_MODULE,"my_pci_class");
	if(pci_class == NULL)
	{
		printk("error in creating class\n");
		return -1;
	}
	for_each_pci_dev(pdev)
	{
		dev = (dev_t) pdev->device;
		printk("creating %x class device \n",i++);
		cls_dev = class_device_create(pci_class,NULL,dev,NULL,"device%d",i);
		cls_dev->class_data = pdev;
		class_device_create_file(cls_dev,&class_device_attr_vendor_id);
		class_device_create_file(cls_dev,&class_device_attr_device_id);
		class_device_create_file(cls_dev,&class_device_attr_irq_no);
		class_device_create_file(cls_dev,&class_device_attr_class);
		pci_dev_put(pdev);
	}
	return 0;
}
static void __exit exit_pci(void)
{
	struct pci_dev *my_dev=NULL;
	for_each_pci_dev(my_dev)
	{
		dev = my_dev->device;
		class_device_destroy(pci_class,dev);
		pci_dev_put(my_dev);
	}
	class_destroy(pci_class);
	printk("Inside exit module\n");
}

module_init(init_pci);
module_exit(exit_pci);

