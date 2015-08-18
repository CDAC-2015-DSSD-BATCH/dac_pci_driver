/*
 *
 *  PCI Based - DAS (Data Acquisition System) Driver for ESA PCI DAS
 *
 *  Authors: Aldonraj, Asheerudin Khilji, Deepak Kumar, Shreyas Hemachandran
 *  Start Date: 21st July 2015
 *
 *  The driver Configures itself with the DAS Card when it is inserted into
 *  the PCI Slot in the computer. It provides the output to the UserSpace
 *  using the usual 'Open','Read','Write','IOCTL' System Calls.
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
#include <linux/delay.h>

// Debug Flag
#define DEBUG

// Probing - Device Detection
#define VENDOR_ID 0x10b5 
#define DEVICE_ID 0x9050
#define SUBVENDOR_ID 0x4441
#define SUBDEVICE_ID 0x4144

// IOCTL
#define MAGIC_NUMBER 'x'
#define READ_DSA 	_IO(MAGIC_NUMBER, 0)

// DSA
#define SET_MASTER_REG(BASE_ADDR)   outb(0x05,(int) BASE_ADDR+0xE);

#define SET_ADC_MODE_REG(BASE_ADDR)   outb(0x04,(int) BASE_ADDR+0x9);

#define GET_ADC_STATUS_REG(BASE_ADDR)   inb((int) BASE_ADDR+9)

#define SET_CLEAR_CHNL(BASE_ADDR)     outb(0x87,(int) BASE_ADDR+0x8);
#define SET_LOAD_CHNL(BASE_ADDR)      outb(0x07,(int) BASE_ADDR+0x8);

#define SET_POLLINIT_REG(BASE_ADDR)   outb(0x56,(int) BASE_ADDR+0xD);

#define GET_LOW_BYTE_REG(BASE_ADDR)   inb((int) BASE_ADDR+0xA);
#define GET_HIGH_BYTE_REG(BASE_ADDR)  inb((int) BASE_ADDR+0xB);

// Prototypes
int DAS_probe (struct pci_dev *, const struct pci_device_id *);
void DAS_remove (struct pci_dev *); 
int DAS_open (struct inode *, struct file *);
long DAS_ioctl (struct file *, unsigned int, unsigned long);
int DAS_release (struct inode *, struct file *);

// Globals Variables
struct DAS_struct{
	dev_t device_no;
	struct cdev cdev;
	struct class *device_class;
	struct device *device;
	unsigned long base_address;
} DAS_inst;

struct file_operations file_ops={
	.owner=THIS_MODULE,
	.open=DAS_open,
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
         Clearing Channel 7

	 7 6 5 4 3 2 1 0
	 1 X X X 0 1 1 1
	 
	 0,1,2,3 bits -> Represents the Channel Number  
	 7th Bit -> Clear Bit
	*/
	SET_CLEAR_CHNL(DAS_inst.base_address);

	
	// ADC Load/Clear Registers
	/*
	 Loading Channel 7

	 7 6 5 4 3 2 1 0
         0 X X X 0 1 1 1

         0,1,2,3 bits -> Represents the Channel Number
         7th Bit -> Load Bit
	*/
	SET_LOAD_CHNL(DAS_inst.base_address);

	return 0;
}

/*
 *	Byte to Bynary Converter
 */
const char *byte_to_binary(int x)
{
   	static char b[9];
	int z;
    
  	b[0] = '\0';

   	for (z = 128; z > 0; z >>= 1){
        	strcat(b, ((x & z) == z) ? "1" : "0");
    	}

    	return b;
}

long DAS_ioctl (struct file *fp, unsigned int cmd, unsigned long arg){
	int count=10;
	char low_byte=0,high_byte=0;
	char status;
	switch(cmd){
		case READ_DSA:
#ifdef DEBUG
				printk(KERN_ALERT "IOCTL CAlled!!");
#endif
				// Setting PollInit Register
			        // As per RTAI Code
			        SET_POLLINIT_REG(DAS_inst.base_address);

				// Keep Polling till the 7th bit of ADC Status Register 
				// is set to 0. ie, 'End of Conversion' (EOC)
				do{
					status=GET_ADC_STATUS_REG(DAS_inst.base_address);
#ifdef DEBUG		
					printk(KERN_ALERT "%x - status!",status);
#endif
					status=status >> 7;
#ifdef DEBUG
					printk(KERN_ALERT "%s - Status Register",byte_to_binary(status));
#endif			
			
					udelay(20);
					printk(KERN_ALERT "Udelay in Progress!!");

					count--;
					if(count == 0){
						break;
					}
				}while(status == 1);
			
				// Aquiring Data From ADC Data Registers
				/*
				  LSB -> BaseAddress+A
				  MSB -> BaseAddress+B
				*/
				count=10;
				while(status == 0){
					udelay(20);
					low_byte=GET_LOW_BYTE_REG(DAS_inst.base_address);
					high_byte=GET_HIGH_BYTE_REG(DAS_inst.base_address);

					status=GET_ADC_STATUS_REG(DAS_inst.base_address);
					status = status >> 7;
					count--;
					if(count == 0){
						break;
					}
				}
#ifdef DEBUG
				printk(KERN_ALERT "Low Byte: %s",byte_to_binary(low_byte));
				printk(KERN_ALERT "High Byte: %s",byte_to_binary(high_byte));
#endif				
				break;
		default:
#ifdef DEBUG
			 printk(KERN_ALERT "Read Not Detected!");
#endif
	}
	return 0;
}

int DAS_release (struct inode *inode, struct file *fp){
#ifdef DEBUG
	printk(KERN_ALERT "Release Called!!");	
#endif
	
	// Clear Channel 7
	SET_CLEAR_CHNL(DAS_inst.base_address);
	
	return 0;
}

static int __init DAS_init(void){
	int status;

#ifdef DEBUG
	printk(KERN_ALERT "%s initialized!!",pci_driver.name);
#endif	
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
#ifdef DEBUG
	printk(KERN_ALERT "Major No: %d",MAJOR(DAS_inst.device_no));
	printk(KERN_ALERT "Minor No: %d",MINOR(DAS_inst.device_no));
#endif
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
#ifdef DEBUG
	printk(KERN_ALERT "%s Exited!!",pci_driver.name);
#endif	
	cdev_del(&DAS_inst.cdev);
	device_destroy(DAS_inst.device_class,DAS_inst.device_no);
	class_destroy(DAS_inst.device_class);
	unregister_chrdev_region(DAS_inst.device_no,1);
	pci_unregister_driver(&pci_driver);
}

int DAS_probe(struct pci_dev *dev, const struct pci_device_id *id){
#ifdef DEBUG
	printk(KERN_ALERT "Vendor %x | Device %x -- Probing !",dev->vendor,dev->device);
#endif
	if(pci_enable_device(dev)==0){
		int status;
		//unsigned long base_addr;// Physical Base Address
#ifdef DEBUG
		printk(KERN_ALERT "Device Enabled!!");
#endif
		status=pci_request_region(dev,2,"MyBar");
		if(status<0){
			printk(KERN_ALERT "Requested Region not Allocated!");
			pci_disable_device(dev);
			return 0;
		}

		DAS_inst.base_address=pci_resource_start(dev,2);
		DAS_inst.base_address= 0xdc41;	
		
		//DAS_inst.base_address=pci_iomap(dev,2,0);// Virtual Base Address

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
#ifdef DEBUG
		printk(KERN_ALERT "Device Not Enabled!!");
#endif
	}
	return 0;
}

void DAS_remove(struct pci_dev *dev){
#ifdef DEBUG
	printk(KERN_ALERT "%s -- Removing!",pci_driver.name);
#endif
	pci_disable_device(dev);
	pci_release_region(dev,2);
}


module_exit(DAS_exit);
module_init(DAS_init);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Shreyas Hemachandran, Aldonraj, Asheerudin Khilji, Deepak Kumar");
MODULE_VERSION("0.1");
