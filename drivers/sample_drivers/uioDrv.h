#ifndef __UIODRV_H__                                                             
#define __UIODRV_H__                                                             
                                                                                 
#define DEVICENAME "XIL_AXI_COUNTER"                                             
#define DRVVERSION "0.0.1"                                                       
#define TIMERBAR  0x51001000                                                     
#define IOREGIONSIZE 28                                                          
                                                                                 
#define TC0_CS_REG      0                                                        
#define TC0_RLD_REG     4                                                        
#define TC0_TC_REG      8                                                        
#define TC1_CS_REG      0x10                                                     
#define TC1_RLD_REG     0x14                                                     
#define TC1_TC_REG      0x18                                                     
                                                                                 
#define TC_MDT0         1<<0                                                     
#define TC_UDT0         1<<1                                                     
#define TC_GENT0        1<<2                                                     
#define TC_CAPT0        1<<3                                                     
#define TC_ARHT0        1<<4                                                     
#define TC_LOAD0        1<<5                                                     
#define TC_ENIT0        1<<6                                                     
#define TC_ENT0         1<<7                                                     
#define TC_T0INT        1<<8                                                     
#define TC_PWMA0        1<<9                                                     
#define TC_ENALL        1<<10                                                    
                                                                                 
#endif
