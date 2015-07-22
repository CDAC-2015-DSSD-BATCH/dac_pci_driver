#include <rtai.h>
#include <rtai_sched.h>
#include <asm/rtai.h>
#include <rtai_schedcore.h>
#include "rtai_esa-das.h"

#include <linux/pci.h>
#include <linux/pci_ids.h>
#include <linux/string.h>



static struct rtai_esadas_driver *esa_driver;

static int handler(unsigned irq, void * dev);
static int add_esa_device(struct pci_dev *dev);

static struct pci_device_id __devinitdata esa_device_id[] = {
	{PCI_DEVICE (VENDOR_ID, DEVICE_ID)},
	{0}
};

MODULE_DEVICE_TABLE(pci, esa_device_id);

static int __init rtai_esa_driver_init(void)
{
	dbg();

	int ret_val;
	struct pci_dev *pcidev;

	esa_driver=(struct rtai_esadas_driver *)rt_shm_alloc(nam2num("driver"),sizeof(struct rtai_esadas_driver),USE_GFP_KERNEL);      
	if(!esa_driver)
	        return -1;

	esa_driver->device_ptr=NULL;
	esa_driver->no_of_devices=0;

        rt_spl_init(&(esa_driver->device_list_spl)); 

	info("Searching for the devices\n");

	for (pcidev = pci_get_device(VENDOR_ID, DEVICE_ID, NULL);
		pcidev != NULL;
		pcidev = pci_get_device(VENDOR_ID, DEVICE_ID, pcidev)) {
			info("Found Card%d", esa_driver->no_of_devices);
			ret_val = add_esa_device(pcidev);
			if(ret_val != 0){
				err ("Error in add_esa_device");
				pci_dev_put(pcidev);
				return ret_val;
			}
	}

	if(esa_driver->no_of_devices != 0){
		info("%d ESA-DAS card found", esa_driver->no_of_devices);	
		return 0;
	}
	else{
		warn("No supported ESA-DAS card found");
		return -EIO;
	}
}



static void __exit rtai_esa_driver_cleanup(void)
{
	dbg();

	struct rtai_esadas_device *tmp;

	if(esa_driver->device_ptr){
		list_for_each(tmp){
			pci_dev_put(tmp->sys_dev);
			rt_sem_delete(&(tmp->in_use));
			rt_free(tmp->device_name);
			rt_free(tmp);
		}
	}

	rt_spl_delete(&(esa_driver->device_list_spl));

	rt_shm_free(nam2num("driver"));
	info("shm rtai_esadas_driver are freed");

	info("hey guys i am too sad to leave u\n");
}


static int add_esa_device(struct pci_dev *dev)
{

	dbg();

	char buf[5];
	struct rtai_esadas_device *new_device, *tmp;

	new_device = (struct rtai_esadas_device *) rt_malloc(sizeof(struct rtai_esadas_driver));
	if(!new_device){
		err ("Error in creating a new device structure.");
		return -ENOMEM;
	}

	sprintf(buf, "card%d", esa_driver->no_of_devices);
	new_device->device_name = (char *) rt_malloc(sizeof(buf));
	if(!new_device->device_name){
		err ("Error in setting name");
		return -ENOMEM;
	}
	strcpy(new_device->device_name, buf);

	rt_typed_sem_init(&(new_device->in_use),1, BIN_SEM);

	new_device->sys_dev = dev;

	new_device->pri_dev = NULL;

	if((esa_driver->device_ptr)==NULL)
		esa_driver->device_ptr = new_device;
	else{
		list_add_tail(tmp,new_device);
	}

	++(esa_driver->no_of_devices);

	info("device is added");

	return 0;

}

	

static int create_esa_device(struct rtai_esadas_device *dev, unsigned int master_conf)                         // 0: success
{
	dbg();
	
	char buf[6];
	int ret_val;
	struct __rtai_esadas_device *new_device;


	ret_val = pci_enable_device (dev->sys_dev);
	if (ret_val >= 0)
		info ("The device is Enabled.");
	else{
		err ("The device cann't be Enabled.");
		return ret_val;                                                /* Pls chk the error code */
	}

	sprintf(buf, "card%d", esa_driver->no_of_devices);
	ret_val = pci_request_region (dev->sys_dev, 2, buf);
	if (ret_val < 0){
		err ("probe : Unable to reserve resource...RTAI_ESA-DAS_DRIVER\n");
		pci_disable_device(dev);
		return ret_val;
	}

	new_device = (struct __rtai_esadas_device *) rt_malloc(sizeof(struct __rtai_esadas_device));
	if(!new_device){
		err ("Error in creating a new device structure.");
		return -ENOMEM;
	}

	new_device->owner = (RT_CURRENT)->tid;

	new_device->index = (RT_CURRENT)->tid;

	new_device->BAR = pci_resource_start(dev->sys_dev, 2);
	new_device->bar_length = pci_resource_len(dev->sys_dev, 2);

	new_device->irq = dev->sys_dev->irq; 

	rt_typed_sem_init(&(new_device->adc_read),0, BIN_SEM);

	new_device->master_reg = master_conf;
	new_device->adc_mode = 0x00;
	new_device->timer_control = 0x00;
	new_device->DIO_mode = 0x00;


	SET_MASTER_REG(master_conf, new_device->BAR);
	dev->pri_dev = new_device;

	ret_val = rt_request_irq(new_device->irq, (int *)handler,(void *)(dev->pri_dev),1);
	if (ret_val != 0){ 
		err ("##### error requesting irq IRQNO %d : returned %d\n", new_device->irq,ret_val);
		rt_spl_unlock(&(esa_driver->device_list_spl));
		return -1;
	}

	rt_sem_wait(&dev->in_use);

	return 0;
}


int rtai_open_esadas(char *devicename, unsigned long flags)
{

        dbg();

	int ret_val;
	u8  master_conf;
	struct rtai_esadas_device *tmp;

    
	if(!flags)
		master_conf = ( DAC_RANGE_0$10V | ADC_RANGE_0$10V | INP_SINGLE_ENDED | INP_POL_0$10V );       /* default settings */
	else
		master_conf = flags;

	rt_spl_lock(&(esa_driver->device_list_spl));       

	list_for_each(tmp){
		if(!(strcmp(tmp->device_name,devicename))){
				if(rt_sem_count(&tmp->in_use)){
					rt_spl_unlock(&(esa_driver->device_list_spl));
					ret_val = create_esa_device(tmp, master_conf);
					if (ret_val != 0){ 
						err ("Error in create_esa_device");
						return -1;
					}
				return tmp->pri_dev->index;
				}
				else{
					rt_spl_unlock(&(esa_driver->device_list_spl));
					warn("Device is in Used....Try again");
					return -EBUSY;
				}
		}
	}


	rt_spl_unlock(&(esa_driver->device_list_spl)); 

	err ("No device present with the name %s", devicename);
  
	return -ENODEV;
}
EXPORT_SYMBOL(rtai_open_esadas);



void rtai_close_esadas(char *devicename)
{

        dbg();

	struct rtai_esadas_device *tmp;

	rt_spl_lock(&(esa_driver->device_list_spl));       

	list_for_each(tmp){
		if(!(strcmp(tmp->device_name,devicename))){
			info("Releasing the associated memories");
			pci_disable_device(tmp->sys_dev);
			pci_release_region (tmp->sys_dev, 2);
			rt_sem_signal(&(tmp->in_use));
			rt_sem_delete(&(tmp->pri_dev->adc_read));
			rt_release_irq(tmp->pri_dev->irq);
			rt_free(tmp->pri_dev);
		}
		else
			warn ("No device present with the name %s", devicename);
	}	

	rt_spl_unlock(&(esa_driver->device_list_spl));

}
EXPORT_SYMBOL(rtai_close_esadas);





/************************************************************************************************************************************
			        S E T T I N G    M A S T E R    R E G I S T E R    F U N C T I O N
************************************************************************************************************************************/


int rt_set_master_reg(unsigned long index, unsigned int flags)                   /* some checks have to added    return 0:success otherwise:failure*/
{

        dbg();

	u8  master_conf;
	struct __rtai_esadas_device *tmp;


	to_pri_dev(tmp, index);      /*match the given index and return the pointer to that structure via tmp */

	if(!flags)
		return -EINVAL;
	else
		master_conf = flags;

	SET_MASTER_REG(master_conf, tmp->BAR);

	return 0;

}
EXPORT_SYMBOL(rt_set_master_reg);




/************************************************************************************************************************************
						      A D C    F U N C T I O N S
************************************************************************************************************************************/


int rt_adc_set_mode(unsigned long index, unsigned long flags)
{

        dbg();

	struct __rtai_esadas_device *tmp;

	to_pri_dev(tmp, index);      /*match the given index and return the pointer to that structure via tmp....if not found return EINVAL*/

	if(!flags)
		flags = (GAIN_1X | ONE_CHANNEL | AUTO_TRIG_MODE);           /* default settings pass NULL for this */

	tmp->adc_mode = flags;

	ADC_SET_MODE(flags, tmp->BAR);

	return 0;

}
EXPORT_SYMBOL(rt_adc_set_mode);

int rt_adc_set_channel(unsigned long index, u8 channel_no)
{

        dbg();

	struct __rtai_esadas_device *tmp;

	to_pri_dev(tmp, index);      //match the given index and return the pointer to that structure via tmp	

	if( channel_no > 0 && channel_no < 15 ){
		tmp->adc_channel = channel_no;
		ADC_SET_CHANNEL(0x80,tmp->BAR);
		channel_no |= 0x0f;
		ADC_SET_CHANNEL(channel_no, tmp->BAR);
		return 0;
	}

	return -1;

}
EXPORT_SYMBOL(rt_adc_set_channel);



unsigned int rt_adc_status(unsigned long index)                /* 0: Ready ( Finish conversion)    1:Busy   */
{

        dbg();

	int ret_val;
	struct __rtai_esadas_device *tmp;

	to_pri_dev(tmp, index);      //match the given index and return the pointer to that structure via tmp	

	ret_val = ADC_GET_STATUS(tmp->BAR);

	if(ret_val & 0x80)
		return 1;
	else 
		return 0;

}
EXPORT_SYMBOL(rt_adc_status);



static int handler(unsigned irq, void * dev)               /* interrupt handler........only checks the adc interrupt...may add the timer int also */
{
	dbg();

	struct __rtai_esadas_device *tmp = (struct __rtai_esadas_device *) dev;

	if(ADC_GET_STATUS(tmp->BAR) & 0x80){
		rt_sem_signal(&tmp->adc_read);
		return IRQ_HANDLED;
	}
	
	return IRQ_NONE;      
}



unsigned short rt_adc_read_data(unsigned long index)
{

        dbg();

	u8 status;
/*	RTIME wait_time;      */
	unsigned short data; 
	struct __rtai_esadas_device *tmp;

	to_pri_dev(tmp, index);      //match the given index and return the pointer to that structure via tmp

	status = ADC_GET_STATUS(tmp->BAR);
    
	if(!(status & 0x80)){
/*		wait_time = nano2count(20*100);                       /* 20 usec */
/*		rt_sem_wait_timed(&(tmp->adc_read), wait_time);     */
		rt_enable_irq(tmp->irq);
		rt_sem_wait(&(tmp->adc_read));
	}

	ADC_GET_DATA(data,tmp->BAR);
	
	return data;

}
EXPORT_SYMBOL(rt_adc_read_data);



void rt_adc_start(unsigned long index)                /* start conversion in auto and polling mode */
{

        dbg();

	struct __rtai_esadas_device *tmp;

	to_pri_dev(tmp, index);      /*match the given index and return the pointer to that structure via tmp	*/

	if(!(tmp->adc_mode &= 0x03)  ||    ((tmp->adc_mode &= 0x03)==0x03))                       /* chk mode */
		SET_POLLINT(0x56,tmp->BAR);

}
EXPORT_SYMBOL(rt_adc_start);



/************************************************************************************************************************************************
							D A C    F U N C T I O N
***********************************************************************************************************************************************/


unsigned int rt_dac_write(unsigned long index, unsigned int dac_no, unsigned short data)    /* 0: success   1:failure  */
{

        dbg();             //chk whether a input or output port

	struct __rtai_esadas_device *tmp;

	to_pri_dev(tmp, index);      /*match the given index and return the pointer to that structure via tmp	*/

	if(dac_no == 1){
		DAC1_SET_DATA(data,tmp->BAR);
	}
	else if(dac_no == 2){
		DAC2_SET_DATA(data,tmp->BAR);
	}
	else
		return 1;
	
	return 0;

}



/************************************************************************************************************************************************
							T I M E R    F U N C T I O N S
***********************************************************************************************************************************************/

unsigned int rt_esa_init_timer(unsigned long index, unsigned long flags)                      /*  Sets operational mode for 8254 Timer. */
{

	dbg();

	u8 timer_conf;
	struct __rtai_esadas_device *tmp;

	to_pri_dev(tmp, index);      /*match the given index and return the pointer to that structure via tmp	*/

	if(!flags)                                    /* Default Settings */
		timer_conf = ( BINARY_TIMER | TIMER_MODE0 | SELECT_COUNTER0 | RD_WR_LSB_MSB );
	else
		timer_conf = flags;

	tmp->timer_control = timer_conf;

	SET_TIMER_CONTROL(timer_conf, tmp->BAR);

	return 0;

}
EXPORT_SYMBOL(rt_esa_init_timer);


/*   
*    Loads the data to the selected Timer. This data, which is to be loaded, is the divisor
*    for the Timers. The maximum value is 65535.
*/
unsigned int rt_load_timer(unsigned long index, unsigned int timer_no, u8 data)
{

	dbg();

	struct __rtai_esadas_device *tmp;

	to_pri_dev(tmp, index);      /*match the given index and return the pointer to that structure via tmp	*/	

	if( timer_no == 0 ){
		data = msec_to_count(data);
		SET_TIMER0(data,tmp->BAR);
	}
	else if( timer_no == 1 ){
		SET_TIMER0(0x01,tmp->BAR);                          /* clock for timer one will be CLK0/2  = 8.25 MHz */
		data = msec_to_count(data);
		SET_TIMER1(data,tmp->BAR);
	}
	else if( timer_no == 2 ){
		data = msec_to_count(data);
		SET_TIMER2(data,tmp->BAR);
	}
	else 
		return 1;

	return 0;

}
EXPORT_SYMBOL(rt_load_timer);


unsigned int rt_read_timer_status(unsigned long index, unsigned int timer_no)
{

	dbg();

	unsigned short count;
	struct __rtai_esadas_device *tmp;

	to_pri_dev(tmp, index);      /*match the given index and return the pointer to that structure via tmp	*/	

	if( timer_no == 0 ){
		count = GET_TIMER0(tmp->BAR);
		count &= ((GET_TIMER0(tmp->BAR)) << 8);
	}
	else if( timer_no == 1 ){
		count = GET_TIMER0(tmp->BAR);
		count &= ((GET_TIMER0(tmp->BAR)) << 8);
	}
	else if( timer_no == 2 ){
		count = GET_TIMER0(tmp->BAR);
		count &= ((GET_TIMER0(tmp->BAR)) << 8);
	}
	else
		return 1;

	return count;

}
EXPORT_SYMBOL(rt_read_timer_status);


/************************************************************************************************************************************************
						D I G I T A L    I/O   F U N T I O N S
************************************************************************************************************************************************/


int rt_init_DIO(unsigned long index, u8 flags)
{

	dbg();

	struct __rtai_esadas_device *tmp;

	to_pri_dev(tmp, index);      /*match the given index and return the pointer to that structure via tmp	*/	

	if(!flags)
		return -EINVAL;

	tmp->DIO_mode = flags;

	SET_DIO_MODE(flags, tmp->BAR);

	return 0;

}
EXPORT_SYMBOL(rt_init_DIO);

int rt_port_write(unsigned long index, char port_name, u8 data)
{

	dbg();

	struct __rtai_esadas_device *tmp;

	to_pri_dev(tmp, index);      /*match the given index and return the pointer to that structure via tmp	*/

	if((port_name == 'A' || port_name == 'a') && (tmp->DIO_mode & PORTA_OUT))
		WRITE_PORTA(data, tmp->BAR);

	else if((port_name == 'B' || port_name == 'b') && (tmp->DIO_mode & PORTA_OUT)){
		info("Writing %x on port B with base address %x", data, tmp->BAR);
		WRITE_PORTB(data, tmp->BAR);
	}

	else
		return -EINVAL;       /* for port C only two lines can be used as output use special func of opto  */

	return 0;

}
EXPORT_SYMBOL(rt_port_write);


unsigned char rt_port_read(unsigned long index, char port_name)
{

	dbg();

	u8 data;
	struct __rtai_esadas_device *tmp;

	to_pri_dev(tmp, index);      /*match the given index and return the pointer to that structure via tmp	*/

	if((port_name == 'A' || port_name == 'a') && (tmp->DIO_mode & PORTA_IN))
		data = READ_PORTA(tmp->BAR);

	else if((port_name == 'B' || port_name == 'b') && (tmp->DIO_mode & PORTB_IN))
		data = READ_PORTB(tmp->BAR);

	else if(port_name == 'C' || port_name == 'c')           /* as only 2 opto lines are out..all other are in */
		data = READ_PORTC(tmp->BAR);

	else
		return -EINVAL;

	return data;      

}
EXPORT_SYMBOL(rt_port_read);


/* Selects any of the 7 Line Drivers (ULN 2003). These lines are connected to PortA of
*  the 8255. The outputs of the line drivers are inverted. The PortA initialization was
*  taken care in this function itself.                                              
*/

unsigned int rt_sel_line_uln(unsigned long index, unsigned int line_no)
{

	dbg();

	struct __rtai_esadas_device *tmp;

	to_pri_dev(tmp, index);      /*match the given index and return the pointer to that structure via tmp	*/

	if(line_no < 0 || line_no > 7 )
		return -EINVAL;

	if(!tmp->DIO_mode)
		SET_DIO_MODE((BASIC_IO_MODE | PORTA_OUT | PORTB_OUT | PORTC_LO_IN | PORTC_UP_OUT ), tmp->BAR);
	else
		SET_DIO_MODE((tmp->DIO_mode | PORTA_OUT), tmp->BAR);

	WRITE_PORTA(line_no, tmp->BAR);

	return 0;

}
EXPORT_SYMBOL(rt_sel_line_uln);


unsigned int rt_set_opto(unsigned long index, unsigned int status)
{

	dbg();

	struct __rtai_esadas_device *tmp;

	to_pri_dev(tmp, index);      /*match the given index and return the pointer to that structure via tmp	*/

	status &= 0x30;

	WRITE_PORTC(status, tmp->BAR);

	return 0;

}
EXPORT_SYMBOL(rt_set_opto);

unsigned char rt_read_opto(unsigned long index)
{

	dbg();

	u8 status;
	struct __rtai_esadas_device *tmp;

	to_pri_dev(tmp, index);      /*match the given index and return the pointer to that structure via tmp	*/

	status = READ_PORTC(tmp->BAR);

	status = ((status & 0x06) >> 4);

	return status;

}
EXPORT_SYMBOL(rt_read_opto);



unsigned char rt_extlatch_read(unsigned long index)
{

	dbg();

	u8 data;
	struct __rtai_esadas_device *tmp;

	to_pri_dev(tmp, index);      /*match the given index and return the pointer to that structure via tmp	*/

	data = READ_EXTERNAL_LATCH(tmp->BAR);

	return data;
}
EXPORT_SYMBOL(rt_extlatch_read);



module_init(rtai_esa_driver_init);
module_exit(rtai_esa_driver_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("One group of insane peoples. <himanshupanwar@ymail.com>");
MODULE_DESCRIPTION ("ESA-DAS PCI DRIVER v1.0");
