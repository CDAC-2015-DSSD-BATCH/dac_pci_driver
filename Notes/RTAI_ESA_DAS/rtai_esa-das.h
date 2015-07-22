/************************************************************
* * E S A - D A S   P C I   D R I V E R   F O R   R T A I * *
* Start Date = 27th JULY 2009
*-------------------------------
* Team Member's Name::::
*-------------------------------
* 1. Prachi Anaspure   0015
* 2. Abhishek Singh    0016
* 3. Himanshu Panwar   0025
* 4. Vijay Belsare     0026
*-------------------------------
* DESD MARCH'09 batch
* Project Guide = Babu Sir
*************************************************************/



/* Linux header file */

#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/errno.h>

#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/stat.h>
#include <linux/ioport.h>
#include <asm/io.h>
#include <linux/version.h>


/* These header files are required for various rtai function */

#include <rtai_sem.h>
#include <rtai_scb.h>
#include <rtai_spl.h>
#include <rtai_nam2num.h>
#include <rtai_malloc.h>


#define DRIVER_NAME "ESA-DAS-Driver"

#define MODULE_NAME "RTAI_ESA-DAS_DRIVER"


/* these micro is added for debugging purpose */

#define dbg(fmt, arg...)							\
	do {									\
			rt_printk (KERN_DEBUG "%s: %s: " fmt "\n",		\
				MODULE_NAME , __FUNCTION__ , ## arg);		\
	} while (0)
#define err(format, arg...) rt_printk(KERN_ERR "%s: " format "\n" , MODULE_NAME , ## arg)
#define info(format, arg...) rt_printk(KERN_INFO "%s: " format "\n" , MODULE_NAME , ## arg)
#define warn(format, arg...) rt_printk(KERN_WARNING "%s: " format "\n" , MODULE_NAME , ## arg)


/*  adding for readability */

#define to_pri_dev(temp, member)                                                   \
	struct rtai_esadas_device *ptr;                                            \
	rt_spl_lock(&(esa_driver->device_list_spl));                               \
	list_for_each(ptr){                                                        \
		if(ptr->pri_dev->index == member)                                  \
			break;                                                     \
	}                           	                                           \
	temp = ptr->pri_dev;                                                       \
	rt_spl_unlock(&(esa_driver->device_list_spl));                             \
	if(!tmp)                                                                   \
		return -EINVAL                                                     	
	

/*       for list accessing        */

#define list_for_each(pos)                                                         \
        for (pos = esa_driver->device_ptr; pos != NULL; pos = pos->next)

#define list_add_tail(pos,new)                                                     \
	for (pos = esa_driver->device_ptr; pos->next != NULL; pos = pos->next){}   \
	pos->next = new;                                                           \
	new->next = NULL       

#define prev_in_list(pos,new)                                                      \
	for (pos = esa_driver->device_ptr; pos->next != new; pos = pos->next){}    \                                            








/************************************************************************************************************************
*  For Registration and Matching
*/

#define VENDOR_ID 0x10b5
#define DEVICE_ID 0x9050
/*     #define BAR   */
	
/***************************************************************************************************************************
*    Master register   0xE
*    BIT      7 6 5 4  3    2    1   0
*    FUNCTION X X X X DAC RANGE INP POL
*/

#define DAC_RANGE_0$10V     0x00
#define DAC_RANGE_10$10V    0x08

#define ADC_RANGE_10$10V    0x00
#define ADC_RANGE_0$10V     0x04               /* also for -5V to +5V */
#define ADC_RANGE_5$5V      0x04

#define INP_SINGLE_ENDED    0x00
#define INP_DIFF            0x02

#define INP_POL_10$10V      0x00               /* also for -5V to +5V */
#define INP_POL_5$5V        0x00
#define INP_POL_0$10V       0x01

#define GET_MASTER_REG(base)       inb(base + 0xE)
#define SET_MASTER_REG(x,base)     outb(x,base + 0xE)

/*****************************************************************************************************************************
*  ADC REGISTERS
*
*  ADC MODE register      0x9
*  BIT      7 6   5  4    3    2   1   0
*  FUNCTION X X   GAIN   CHANNEL   MODE
*/

#define GAIN_1X             0x00
#define GAIN_2X             0x10
#define GAIN_5X             0x20
#define GAIN_10X            0x30	

#define AUTO_INC_CHANNEL    0x00
#define ONE_CHANNEL         0x04
#define TWO_CHANNEL         0x08

#define POLLED_MODE    	    0x00
#define TIMER_MODE    	    0x01
#define EXTERNAL_TRIG_MODE  0x02
#define AUTO_TRIG_MODE      0x03


#define ADC_GET_MODE(base)        inb(base + 0x9)
#define ADC_SET_MODE(x,base)      outb(x,base + 0x9)

/*
*  ADC LOAD/CLEAR CHANNEL register      0x8
*  BIT          7      6 5 4  3   2   1   0
*  FUNCTION LOAD/CLEAR X X X CH3 CH2 CH1 CH0
*/

#define ADC_SET_CHANNEL(x,base)       outb(x,base + 0x8)

/*
*  ADC STATUS register            0x9
*  BIT       D7 D6 D5 D4 D3 D2 D1 D0
*  FUNCTION STS  X X  X  X  X  X  X
*/

#define ADC_GET_STATUS(base)      inb(base + 0x09)


/*  
*  ADC DATA registers    HI = 0xB   LO = 0xA    must read in order LO/HI
*                 HIGH BYTE (BADR + B)
*  BIT       D7 D6 D5     D4       D3      D2  D1  D0
*  FUNCTION  X   X X       X      DB11    DB10 DB9 DB8
*                 LOW BYTE (BADR + A)
*  BIT       D7 D6   D5    D4      D3      D2  D1   D0
*  FUNCTION  B7 DB6 DB5    DB4     DB3     DB2 DB1 DB0
*/

#define ADC_GET_DATA(data,base)		              \
	data = inb(base + 0xA);                       \
	data = ((inb(base + 0xB) << 8) + data)

/*********************************************************************************************************
*  For POLLINT register
*/

#define SET_POLLINT(x,base)      outb(x,base + 0xD) 


/*********************************************************************************************************
*  DAC REGISTERS
*  DAC DATA registers
*                HIGH BYTE (BADR + B)                 DAC Common Data         DAC 1 & 2 High Byte
*  BIT       D7 D6     D5      D4       D3      D2   D1   D0
*  FUNTION   X   X      X       X      DB11    DB10  DB9 DB8
*        LOW BYTE (BADR + A) DAC1 Data & (BADR + C) DAC 2 Data
*  BIT        D7 D6     D5      D4       D3      D2   D1   D0
*  FUNCTION  DB7 DB6   DB5      DB4     DB3     DB2   DB1 DB0
*/


#define DAC1_SET_DATA(data,base)	                  \
	outb(((data & 0xf00) >> 8 ), base + 0xB);         \
	outb((data & 0xff),base + 0xA)

#define DAC2_SET_DATA(data,base)	                  \
	outb(((data & 0xf00) >> 8 ), base + 0xB);         \
	outb((data & 0xff),base + 0xC)





/************************************************************************************************************
*  TIMER REGISTERS
*
*  The 8254 have three 16-bit timer/counter outputs.
*
*  1. COMMAND REGISTER (BADR + 3): This register is used to set the modes of
*     operation of the Timer and to select which timer is to be loaded.
*  2. TIMER 0 (BADR + 0): This register is used to load the data to Timer 0 and also to
*     read-back the status of the timer.
*  3. TIMER 1 (BADR + 1): This register is used to load the data to Timer 1 and also to
*     read-back the status of the timer.
*  4. TIMER 2 (BADR + 2): This register is used to load the data to Timer 2 and also to
*     read-back the status of the timer.
*  
*  The control word format for the timer is as shown below.
*
*  BIT                7         6       5       4      3        2        1         0
*  FUNCTION         SC1      SC0      RL1      RL0    M2       M1        M0      BCD
*
*                                    SELECT COUNTER
*           SC1                            SC0                   FUNCTION
*            0                             0                   Select Counter 0
*            0                             1                   Select Counter 1
*            1                             0                   Select Counter 2
*            1                             1                Read Back Command
*
*                                READ / WRITE
*            RL1                     RL0                      FUNCTION
*             0                       0                 Counter Latch Command
*             0                       1                  Read/Write LSB only
*             1                       0                  Read/Write MSB only
*             1                       1            Read/Write LSB first, then MSB
*  
*                              MODE                                      /*  realy dont know from manual which modes are supported  
*                 M2     M1             M0     FUNCTION                   *  only impliment only one mode that is mode 0...timer
*                 0      0              0       MODE 0                    *  generate an interrupt after expiration of count.
*                 X      1              0       MODE 0                    *  other modes may be supported by next versions of the
*                 X      1              1       MODE 0                    *  driver......chk the 8254 datasheet.
*                 0      0              0       MODE 0
*
*                         BCD
*               0  BINARY COUNTER 16 BITS.
*               1  BCD COUNTER (4 DECADES).
*
*/


#define SELECT_COUNTER0           0x00
#define SELECT_COUNTER1           0x40
#define SELECT_COUNTER2           0x80
#define TIMER_READ_BACK           0xc0

#define COUNT_LATCH_COMMAND       0x00
#define RD_WR_LSB_ONLY            0x10
#define RD_WR_MSB_ONLY            0x20
#define RD_WR_LSB_MSB             0x30

#define TIMER_MODE0               0x00

#define BINARY_TIMER              0x00
#define BCD_TIMER                 0x01                



#define SET_TIMER_CONTROL(x,base)         outb(x,base + 0x3)

#define SET_TIMER0(x,base)                outb(x,base + 0x0)
#define SET_TIMER1(x,base)                outb(x,base + 0x1)
#define SET_TIMER2(x,base)                outb(x,base + 0x2)

#define GET_TIMER0(base)                  inb(base + 0x0)
#define GET_TIMER1(base)                  inb(base + 0x1)
#define GET_TIMER2(base)                  inb(base + 0x2)

#define msec_to_count(data)               ((data * 100000/825 ) + (1/2))



/*************************************************************************************************************
*  DIGITAL I/O REGISTERS 
**************************************************************************************************************/

#define BASIC_IO_MODE           0x80
#define STROBED_IO_MODE         0xa4
#define BI_DIRECTIONAL_MODE     0xc0

#define PORTA_OUT               0x80
#define PORTA_IN                0x90

#define PORTB_OUT               0x80
#define PORTB_IN                0x82

#define PORTC_LO_OUT            0x80
#define PORTC_LO_IN             0x81
#define PORTC_UP_OUT            0x80
#define PORTC_UP_IN             0x88

#define ALL_INPUT               0x9b
#define ALL_OUTPUT              0x80


#define SET_DIO_MODE(x,base)          outb(x, base + 0x09)                     

#define WRITE_PORTA(x,base)           outb(x, base + 0x04)
#define READ_PORTA(base)              inb(base + 0x04)
#define WRITE_PORTB(x,base)           outb(x, base + 0x05)
#define READ_PORTB(base)              inb(base + 0x05)
#define WRITE_PORTC(x,base)           outb(x, base + 0x06)
#define READ_PORTC(base)              inb(base + 0x06)


#define OPTO_ONE_ON            0x10
#define OPTO_SECOND_ON         0x20
#define OPTO_BOTH_ON           0x30

#define READ_EXTERNAL_LATCH(base)     inb(base + 0x08)

/*************************************************************************************************************
*                                   G E N E R A L    M A C R O S
**************************************************************************************************************/

#define access_out(port)             ( tmp->DIO_mode & PORT##port_OUT )
#define access_in(port)              ( tmp->DIO_mode & PORT##port_IN )

/*************************************************************************************************************
*  Various structures used in the program 
**************************************************************************************************************/

struct __rtai_esadas_device{
	unsigned int                     owner;
	unsigned long int                BAR;
	unsigned long                    index;
	unsigned long int                bar_length;                           /* range of mapped I/O port region */
	u8                               master_reg;
	unsigned int                     irq;
	u8                               adc_mode;               /* mode + channel + gain */
	u8                               timer_control;
	u8                               DIO_mode;
	u8                               adc_channel;
	SEM                              adc_read;
};


struct rtai_esadas_device{
	struct rtai_esadas_device        *next;
	char                             *device_name;
	struct pci_dev                   *sys_dev;
	struct __rtai_esadas_device      *pri_dev;
	SEM                              in_use;
};


struct rtai_esadas_driver{
	struct rtai_esadas_device     *device_ptr;
	unsigned int                  no_of_devices;
	SPL                           device_list_spl;             
};


/*******************************************************************************************************************
				F U N C T I O N S    I M P L I M E N T E D 
*********************************************************************************************************************/



extern int rtai_open_esadas(char *devicename, unsigned long flags);
extern int rt_set_master_reg(unsigned long index, unsigned int flags);
extern void rtai_close_esadas(char *devicename);


extern int rt_adc_set_mode(unsigned long index, unsigned long flags);
extern int rt_adc_set_channel(unsigned long index, u8 channel_no);
extern unsigned int rt_adc_status(unsigned long index);
extern unsigned short rt_adc_read_data(unsigned long index);
extern void rt_adc_start(unsigned long index);

extern unsigned int rt_dac_write(unsigned long index, unsigned int dac_no, unsigned short data);


extern unsigned int rt_esa_init_timer(unsigned long index, unsigned long flags);
extern unsigned int rt_load_timer(unsigned long index, unsigned int timer_no, u8 data);
extern unsigned int rt_read_timer_status(unsigned long index, unsigned int timer_no);


extern int rt_init_DIO(unsigned long index, u8 flags);
extern unsigned char rt_port_read(unsigned long index, char port_name);
extern int rt_port_write(unsigned long index, char port_name, u8 data);

extern unsigned int rt_sel_line_uln(unsigned long index, unsigned int line_no);

extern unsigned int rt_set_opto(unsigned long index, unsigned int status);
extern unsigned char rt_read_opto(unsigned long index);

extern unsigned char rt_extlatch_read(unsigned long index);

