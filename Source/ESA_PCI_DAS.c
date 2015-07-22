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
int DAS_probe (struct pci_dev *dev, const struct pci_device_id *id);
void DAS_remove (struct pci_dev *dev); 

// Globals Variables
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
static int __init DAS_init(void){
	printk(KERN_ALERT "%s initialized!!",pci_driver.name);
	return pci_register_driver(&pci_driver);
}

static void __exit DAS_exit(void){
	printk(KERN_ALERT "%s Exited!!",pci_driver.name);
	pci_unregister_driver(&pci_driver);
}

int DAS_probe(struct pci_dev *dev, const struct pci_device_id *id){
	printk(KERN_ALERT "Vendor %x | Device %x -- Probing !",dev->vendor,dev->device);

	if(pci_enable_device(dev)==0){
		unsigned long BAR=0;
		printk(KERN_ALERT "Device Enabled!!");

		BAR=pci_resource_start(dev,2);
		printk(KERN_ALERT "%p - Base Address",BAR);
	}else{
		printk(KERN_ALERT "Device Not Enabled!!");
	}
	return 0;
}

void DAS_remove(struct pci_dev *dev){
	printk(KERN_ALERT "%s -- Removing!",pci_driver.name);
}


module_exit(DAS_exit);
module_init(DAS_init);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Shreyas Hemachandran, Aldonraj, Asheerudin Khilji, Deepak Kumar");
MODULE_VERSION("0.0");
