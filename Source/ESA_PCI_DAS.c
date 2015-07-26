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
#include<asm/io.h>
#include <linux/poll.h>

// Probing - Device Detection
#define VENDOR_ID 0x10b5 
#define DEVICE_ID 0x9050
#define SUBVENDOR_ID 0x4441
#define SUBDEVICE_ID 0x4144

// IOCTL
#define MAGIC_NUMBER 'x'
#define READ_DSA 	_IO(MAGIC_NUMBER, 0)

// DSA
#define SET_MASTER_REG(BASE_ADDR)   outb(0x5,(int) BASE_ADDR+0xE);
#define GET_MASTER_REG(BASE_ADDR)   inb(BASE_ADDR+0xE);

#define SET_ADC_MODE_REG(BASE_ADDR)   outb(0x4,(int) BASE_ADDR+0x9);
#define GET_ADC_MODE_REG(BASE_ADDR)   inb(BASE_ADDR+9)

#define SET_CLEAR_CHNL(BASE_ADDR)     outb(0x8A,(int) BASE_ADDR+0x8);
#define SET_LOAD_CHNL(BASE_ADDR)      outb(0xA,(int) BASE_ADDR+0x8);

#define SET_POLLINIT_REG(BASE_ADDR)   outb(0x56,(int) BASE_ADDR+0xD);

// Prototypes
int DAS_probe (struct pci_dev *, const struct pci_device_id *);
void DAS_remove (struct pci_dev *); 
int DAS_open (struct inode *, struct file *);
long DAS_ioctl (struct file *, unsigned int, unsigned long);
int DAS_release (struct inode *, struct file *);
unsigned int DAS_poll (struct file *, struct poll_table_struct *);


// Globals Variables
struct DAS_struct{
	dev_t device_no;
	struct cdev cdev;
	struct class *device_class;
	struct device *device;
	void __iomem *base_address;
} DAS_inst;

struct file_operations file_ops={
	.owner=THIS_MODULE,
	.open=DAS_open,
	.unlocked_ioctl=DAS_ioctl,
	.release=DAS_release,
	.poll=DAS_poll,
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
unsigned int DAS_poll (struct file *fp, struct poll_table_struct *poll_table){
	printk(KERN_ALERT "In Poll Method");
	return POLLIN;
}

int DAS_open (struct inode *inode, struct file *fp){	
	
	// Setting ADC Mode Register
	/*
	 7 6 5 4 3 2 1 0
	 X X 0 0 0 1 0 0	 

	 0th and 1st bit ->  Polled Mode
	 2nd and 3rd bit ->  One Channel
	 4ht and 5th bit ->  Gain x1
	*/
	SET_ADC_MODE_REG(DAS_inst.base_address);

	// ADC Load/Clear Registers
	/*
         Clearing Channel 10

	 7 6 5 4 3 2 1 0
	 1 X X X 1 0 1 0
	 
	 0,1,2,3 bits -> Represents the Channel Number  
	 7th Bit -> Clear Bit
	*/
	SET_CLEAR_CHNL(DAS_inst.base_address);

	
	// ADC Load/Clear Registers
	/*
	 Loading Channel 10

	 7 6 5 4 3 2 1 0
         0 X X X 1 0 1 0

         0,1,2,3 bits -> Represents the Channel Number
         7th Bit -> Load Bit
	*/
	SET_LOAD_CHNL(DAS_inst.base_address);

	// Setting PollInit Register
	// As per RTAI Code
	SET_POLLINIT_REG(DAS_inst.base_address);	
	
	return 0;
}

long DAS_ioctl (struct file *fp, unsigned int cmd, unsigned long arg){
	switch(cmd){
		case READ_DSA:
				printk(KERN_ALERT "IOCTL CAlled!!");
				// Keep Pooling till the 7th bit of ADC Status Register 
				// is set to 0. ie, 'End of Conversion' (EOC)
				
				// Aquire the Data from Lower Byte and Higher Byte Register
				
				// Clear the Channel
				break;
		default:
			 printk(KERN_ALERT "Read Not Detected!");
	}
	return 0;
}

int DAS_release (struct inode *inode, struct file *fp){
	printk(KERN_ALERT "Release Called!!");	
	
	// Clear Channel 10
	SET_CLEAR_CHNL(DAS_inst.base_address);
	
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
		unsigned long base_addr;// Physical Base Address

		printk(KERN_ALERT "Device Enabled!!");

		status=pci_request_region(dev,2,"MyBar");
		if(status<0){
			printk(KERN_ALERT "Requested Region not Allocated!");
			pci_disable_device(dev);
			return 0;
		}

		base_addr=pci_resource_start(dev,2);

		DAS_inst.base_address=pci_iomap(dev,2,0);// Virtual Base Address

		// Setting Master Register 
		/*
		 MSB Nibble - X X X X
		 LSP Nibble - 0 1 0 1
		 
		 0th bit - Polarity -> Unipolar
		 1st bit - Input type -> Single Ended
		 2nd bit - ADC Range -> 0v to 10v, +/- 5v
		*/
		SET_MASTER_REG(DAS_inst.base_address);		
	
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
