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

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Shreyas Hemachandran, Aldonraj, Asheerudin Khilji, Deepak Kumar");
MODULE_VERSION("0.0");

static struct pci_driver pci_driver={
	.name="ESA_PCI_DSA",
};

static int __init DAS_init(void){
	printk(KERN_ALERT "%s initialized!!",pci_driver.name);
	return pci_register_driver(&pci_driver);
}

static void __exit DAS_exit(void){
	printk(KERN_ALERT "%s Exited!!",pci_driver.name);
	pci_unregister_driver(&pci_driver);
}

module_init(DAS_init);
module_exit(DAS_exit);
