/*
 *
 *  PCI Based - DAS (Data Acquisition System) Driver for ESA PCI DAS
 *
 *  Authors: Aldonraj,Asheerudin Khilji,Deepak Kumar,Shreyas Hemachandran
 *  Start Date: 21st July 2015
 *
 *  The driver Configures itself with the DAS Card when it is inserted into
 *  the PCI Slot in the computer. It provides the output to the UserSpace
 *  using the usual 'Open','Read','Write' System Calls.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation;
 *
 */

#include<linux/init.h>
#include<linux/module.h>
#include<linux/pci.h>
#include<linux/ioport.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/kdev_t.h>

#define VENDOR_ID 0x10b5 
#define DEVICE_ID 0x9050
#define SUBVENDOR_ID 0x4441
#define SUBDEVICE_ID 0x4144

#define BASE_ADDR 0x0000 // TODO Base Address yet to be discovered
#define MASTER_REG (BASE_ADDR+E)
#define LD_CL_CHANL (BASE_ADDR+8)
#define ADC_MODE_REG (BASE_ADDR+9)
#define ADC_STATUS_REG (BASE_ADDR+9)
#define LOWER_DByte (BASE_ADDR+A)
#define HIGHER_DByte (BASE_ADDR+B)
#define POLLINIT_REG (BASE_ADDR+D)

// Prototypes
int DAS_probe (struct pci_dev *, const struct pci_device_id *);
void DAS_remove (struct pci_dev *); 
int DAS_open (struct inode *, struct file *);
ssize_t DAS_read (struct file *, char __user *, size_t, loff_t *);
ssize_t DAS_write (struct file *, const char __user *, size_t, loff_t *);
long DAS_ioctl (struct file *, unsigned int, unsigned long);
int DAS_release (struct inode *, struct file *);

// Globals Variables
struct DAS_struct{
	dev_t device_no;
	struct cdev cdev;
	struct class *device_class;
	struct device *device;
} DAS_inst;

struct file_operations file_ops={
	.owner=THIS_MODULE,
	.open=DAS_open,
	.read=DAS_read,
	.write=DAS_write,
	.unlocked_ioctl=DAS_ioctl,
	.release=DAS_release,
};

struct pci_device_id DAS_id={
	.vendor=VENDOR_ID,
	.device=DEVICE_ID,
	.subvendor=SUBVENDOR_ID,
	.subdevice=SUBDEVICE_ID,
};

static struct pci_driver pci_driver={
	.name="ESA_PCI_DSA",
	.id_table=&DAS_id,
	.probe=DAS_probe,
	.remove=DAS_remove,
};

// Functions
int DAS_open (struct inode *inode, struct file *fp){
	printk(KERN_ALERT "Open Called!!");
	return 0;
}

ssize_t DAS_read (struct file *fp, char __user *uBuff, size_t size, loff_t *loff){
	printk(KERN_ALERT "Read Called!!");
	return size;	
}

ssize_t DAS_write (struct file *fp, const char __user *uBuff, size_t size, loff_t *loff){
	printk(KERN_ALERT "Write Called!!");
	return size;
}

long DAS_ioctl (struct file *fp, unsigned int cmd, unsigned long arg){
	printk(KERN_ALERT "IOCTL Called!!");
	return 0;
}

int DAS_release (struct inode *inode, struct file *fp){
	printk(KERN_ALERT "Release Called!!");	
	return 0;
}

static int __init DAS_init(void){
	int status;
	printk(KERN_ALERT "%s initialized!!",pci_driver.name);
	
	status=alloc_chrdev_region(&DAS_inst.device_no,0,1,pci_driver.name);
	if(status < 0){
		printk("Device Number Allocation Failed!");
		return 0;
	}
	
	cdev_init(&DAS_inst.cdev,&file_ops);
	status=cdev_add(&DAS_inst.cdev,DAS_inst.device_no,1);
	if(status < 0){
		printk("Cdev Not Created!!");
		return 0;
	}

	printk(KERN_ALERT "Major No: %d",MAJOR(DAS_inst.device_no));
	printk(KERN_ALERT "Minor No: %d",MINOR(DAS_inst.device_no));

	DAS_inst.device_class=class_create(THIS_MODULE,pci_driver.name);
	if(DAS_inst.device_class==NULL){
		printk(KERN_ALERT "Class Not Created!");
		return 0;
	}
	
	DAS_inst.device=device_create(DAS_inst.device_class,NULL,DAS_inst.device_no,NULL,"DAS_PCI_%d",1);
	if(DAS_inst.device==NULL){
		printk(KERN_ALERT "Device Not Created!");
		return 0;				
	}
	
	return pci_register_driver(&pci_driver);
}

static void __exit DAS_exit(void){
	printk(KERN_ALERT "%s Exited!!",pci_driver.name);
	
	cdev_del(&DAS_inst.cdev);
	device_destroy(DAS_inst.device_class,DAS_inst.device_no);
	class_destroy(DAS_inst.device_class);
	unregister_chrdev_region(DAS_inst.device_no,1);
	pci_unregister_driver(&pci_driver);
}

int DAS_probe(struct pci_dev *dev, const struct pci_device_id *id){
	printk(KERN_ALERT "Vendor %x | Device %x -- Probing !",dev->vendor,dev->device);

	if(pci_enable_device(dev)==0){
		int status;
		void __iomem *startAddress;
		unsigned long baseAddr;

		printk(KERN_ALERT "Device Enabled!!");

		status=pci_request_region(dev,2,"MyBar");
		if(status<0){
			printk(KERN_ALERT "Requested Region not Allocated!");
			pci_disable_device(dev);
			return 0;
		}

		baseAddr=pci_resource_start(dev,2);
		printk(KERN_ALERT "%p - Base Address",baseAddr);		
		printk(KERN_ALERT "%p - Flag",pci_resource_flags(dev,2));

		startAddress=pci_iomap(dev,2,0);
		printk(KERN_ALERT "%p - virtual Address",startAddress);
		printk(KERN_ALERT "%d - irq Number",dev->irq);

		/*if(dev->resource[2].flags==IORESOURCE_IO){
			printk(KERN_ALERT "IO Mapped!!");
		}else if(dev->resource[2].flags==IORESOURCE_MEM){
			printk(KERN_ALERT "Memory Mapped!!");
		}else{
			printk(KERN_ALERT " %ld NONE!!",dev->resource[2].flags);
		}*/
	}else{
		printk(KERN_ALERT "Device Not Enabled!!");
	}
	return 0;
}

void DAS_remove(struct pci_dev *dev){
	printk(KERN_ALERT "%s -- Removing!",pci_driver.name);
	pci_disable_device(dev);
	pci_release_region(dev,2);
}


module_exit(DAS_exit);
module_init(DAS_init);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Shreyas Hemachandran, Aldonraj, Asheerudin Khilji, Deepak Kumar");
MODULE_VERSION("0.0");
