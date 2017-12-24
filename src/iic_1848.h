#ifndef _IIC_1848_H
#define _IIC_1848_H

void set_1848_reg(int num,unsigned int offset, unsigned int data);
unsigned int get_1848_reg(int num,unsigned int offset);
void iic_TTL_csr(unsigned int ttl_count);
void iic_SwPortCounterEnable();
void iic_SrioErrorDetect();
void iic_SwPortRepair();
void iic_SwPortClear();
void iic_SystemReset();
void iic_SwSpeedBaudSet(unsigned int speedIndex);
void disable_1848_port_rxtx(unsigned char swIndex,unsigned char portNum);
void enable_1848_port_rxtx(unsigned char swIndex,unsigned char portNum);
void reset_1848_ackid(unsigned char swIndex,unsigned char portNum);
void reset_1848_port(unsigned char swIndex,unsigned char portNum);


#endif
