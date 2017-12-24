#include "xil_io.h"
#include "iic_slave.h"
#include "xadc_core.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <ctype.h>
#include <pthread.h>
#include <assert.h>

#define RioUartBase  0x83c10000
#define SWBACK_BOARD_MAX_COUNT		500
#define GPIO_BASE_ADDR		0x82000000
#define GPIO_I2C_RESET_ADDR		0x82010000
#define BUFFER_SIZE	32

#ifndef u8
#define u8 unsigned char
#endif

extern unsigned int get_1848_reg(int num,unsigned int offset);

unsigned char SendBuffer[BUFFER_SIZE];
int g_sw1848PortStatus[3] = {0};

void xil_setreg32(unsigned int addrBase,unsigned int addrOffset,unsigned int value)
{
	Xil_Out32(addrBase+addrOffset, value);
}

int xil_getreg32(unsigned int addrBase,unsigned int addrOffset)
{
	int ans=0;
	ans=Xil_In32(addrBase+addrOffset);
	return ans;
}

//reset
void GpioReset()
{
	xil_setreg32(GPIO_I2C_RESET_ADDR,0x4,0x0);
	xil_setreg32(GPIO_I2C_RESET_ADDR,0x0,0x2);
	//printf("gpio reset ok!\n");
}


void uart_setreg32(unsigned int addrBase,unsigned int addrOffset,unsigned int value)
{
	Xil_Out32(addrBase+addrOffset, value);
}

int uart_getreg32(unsigned int addrBase,unsigned int addrOffset)
{
	int ans=0;
	ans=Xil_In32(addrBase+addrOffset);
	return ans;
}


//utility functions
float conv_voltage(float input, enum EConvType conv_direction)
{
	float result=0;

	switch(conv_direction)
	{
	case EConvType_Raw_to_Scale:
		result = ((input * 3.0 * mV_mul)/multiplier);
		break;
	case EConvType_Scale_to_Raw:
		result = (input/(3.0 * mV_mul))*multiplier;
		break;
	default:
		printf("Convertion type incorrect... Doing no conversion\n");
		//  intentional no break;
	case EConvType_None:
		result = input;
		break;
	}

	return result;
}

float conv_voltage_ext_ch(float input, enum EConvType conv_direction)
{
	float result=0;

	switch(conv_direction)
	{
	case EConvType_Raw_to_Scale:
		result = ((input * mV_mul)/multiplier);
		break;
	case EConvType_Scale_to_Raw:
		result = (input/mV_mul)*multiplier;
		break;
	default:
		printf("Convertion type incorrect... Doing no conversion\n");
		//  intentional no break;
	case EConvType_None:
		result = input;
		break;
	}

	return result;
}

float conv_temperature(float input, enum EConvType conv_direction)
{
	float result=0;

	switch(conv_direction)
	{
	case EConvType_Raw_to_Scale:
		result = ((input * 503.975)/multiplier) - 273.15;
		break;
	case EConvType_Scale_to_Raw:
		result = (input + 273.15)*multiplier/503.975;
		break;
	default:
		printf("Conversion type incorrect... Doing no conversion\n");
		//  intentional no break;
	case EConvType_None:
		result = input;
		break;
	}

	return result;
}

float get_temp(int num)
{
	int number;
	int fd = -1;
	char upset[20];
	float raw_data=0;
	float true_data=0;
	int offset=0;
	char value=0;
	float max_temp=0;
	number=num;
	if(number==0)
	{
		fd = open(FPGA_TEMP, O_RDWR );
		offset=0;
		while(offset<5)
		{
			lseek(fd,offset,SEEK_SET);
			read(fd,&value,sizeof(char));
			upset[offset]=value;
			offset++;
		}
		upset[offset]='\0';
		raw_data=atoi(upset);
		true_data=conv_temperature(raw_data, EConvType_Raw_to_Scale);
		//printf("FPGA temp is %f cent\n",true_data);
		close(fd);
		return true_data;
	}
	else
	{
		fd = open(TEMP1848_1, O_RDWR );
		offset=0;
		while(offset<5)
		{
			lseek(fd,offset,SEEK_SET);
			read(fd,&value,sizeof(char));
			upset[offset]=value;
			offset++;
		}
		upset[offset]='\0';
		raw_data=atoi(upset);
		true_data=(conv_voltage(raw_data, EConvType_Raw_to_Scale)*1000/(3*2.2)-273.15)/10000;
		max_temp=true_data;
		//printf("1848-1 temp is %f cent\n",true_data);
		close(fd);

		fd = open(TEMP1848_2, O_RDWR );
		offset=0;
		while(offset<5)
		{
			lseek(fd,offset,SEEK_SET);
			read(fd,&value,sizeof(char));
			upset[offset]=value;
			offset++;
		}
		upset[offset]='\0';
		raw_data=atoi(upset);
		true_data=(conv_voltage(raw_data, EConvType_Raw_to_Scale)*1000/(3*2.2)-273.15)/10000;
		if(max_temp<true_data)
		{
			max_temp=true_data;
		}
		//printf("1848-2 temp is %f cent\n",true_data);
		close(fd);

		fd = open(TEMP1848_3, O_RDWR );
		offset=0;
		while(offset<5)
		{
			lseek(fd,offset,SEEK_SET);
			read(fd,&value,sizeof(char));
			upset[offset]=value;
			offset++;
		}
		upset[offset]='\0';
		raw_data=atoi(upset);
		true_data=(conv_voltage(raw_data, EConvType_Raw_to_Scale)*1000/(3*2.2)-273.15)/10000;
		if(max_temp<true_data)
		{
					max_temp=true_data;
		}
		//printf("1848-3 temp is %f cent\n",true_data);
		close(fd);

		//printf("1848-max temp is %f cent\n",max_temp);
		return max_temp;
	}

}

float get_vcc(int num)
{
	int number;
	int fd = -1;
	char upset[20];
	float raw_data=0;
	float true_data=0;
	int offset=0;
	char value=0;
	number=num;
	if(number==0)
	{
		fd = open(VCC_1, O_RDWR );
		offset=0;
		while(offset<5)
		{
			lseek(fd,offset,SEEK_SET);
			read(fd,&value,sizeof(char));
			upset[offset]=value;
			offset++;
		}
		upset[offset]='\0';
		raw_data=atoi(upset);
		true_data=conv_voltage(raw_data, EConvType_Raw_to_Scale);
		//printf("vcc 1 is %f mv\n",true_data);
		close(fd);
	}
	else if(number==1)
	{
		fd = open(VCC_1V8, O_RDWR );
		offset=0;
		while(offset<5)
		{
			lseek(fd,offset,SEEK_SET);
			read(fd,&value,sizeof(char));
			upset[offset]=value;
			offset++;
		}
		upset[offset]='\0';
		raw_data=atoi(upset);
		true_data=conv_voltage(raw_data, EConvType_Raw_to_Scale);
		//printf("vcc 1v8 is %f mv\n",true_data);
		close(fd);

	}
	else
	{
		fd = open(VCC_3V3, O_RDWR );
		offset=0;
		while(offset<5)
		{
			lseek(fd,offset,SEEK_SET);
			read(fd,&value,sizeof(char));
			upset[offset]=value;
			offset++;
		}
		upset[offset]='\0';
		raw_data=atoi(upset);
		true_data=(conv_voltage(raw_data, EConvType_Raw_to_Scale)/3)*5.7;
		//printf("vcc 3v3 is %f mv\n",true_data);
		close(fd);

	}
	return (true_data/0.025);
}

void iic_write()
{
	unsigned int _data;

	memcpy((void*)&_data,&SendBuffer[0],4);
	xil_setreg32(XPAR_AXI_BRAM_CTRL_I2C_S_AXI_BASEADDR,0,_data);
	memcpy((void*)&_data,&SendBuffer[4],4);
	xil_setreg32(XPAR_AXI_BRAM_CTRL_I2C_S_AXI_BASEADDR,4,_data);
	memcpy((void*)&_data,&SendBuffer[8],4);
	xil_setreg32(XPAR_AXI_BRAM_CTRL_I2C_S_AXI_BASEADDR,8,_data);
	memcpy((void*)&_data,(void*)&SendBuffer[12],4);
	xil_setreg32(XPAR_AXI_BRAM_CTRL_I2C_S_AXI_BASEADDR,12,_data);
	memcpy((void*)&_data,(void*)&SendBuffer[16],4);
	xil_setreg32(XPAR_AXI_BRAM_CTRL_I2C_S_AXI_BASEADDR,16,_data);
	memcpy((void*)&_data,(void*)&SendBuffer[20],4);
	xil_setreg32(XPAR_AXI_BRAM_CTRL_I2C_S_AXI_BASEADDR,20,_data);
	memcpy((void*)&_data,(void*)&SendBuffer[24],4);
	xil_setreg32(XPAR_AXI_BRAM_CTRL_I2C_S_AXI_BASEADDR,24,_data);
	memcpy((void*)&_data,(void*)&SendBuffer[28],4);
	xil_setreg32(XPAR_AXI_BRAM_CTRL_I2C_S_AXI_BASEADDR,28,_data);
}

int IicSelfTest()
{
	int i,_swIndex;
	unsigned int _offset,_data,_waitCount,val[16];
	u32 rio_data[4];
	static unsigned int bI2cSelfTest = 0;

	rio_data[0]=0;
	rio_data[1]=0;
	rio_data[2]=0;
	rio_data[3]=0;

	if( bI2cSelfTest >= 0xfffffff0 )
		bI2cSelfTest = 1;
	else if( (bI2cSelfTest!=0) && ((bI2cSelfTest+1)%0x3AAAAAA != 0))	//差不多4秒更新一次i2c数据
	{
		bI2cSelfTest++;
		return 1;
	}


	if( bI2cSelfTest == 0 )
	{
		memset(SendBuffer,0,BUFFER_SIZE);
		memset(g_sw1848PortStatus,0,12);

		///////BIT info send
		SendBuffer[0] = 0x9;//Module Type
		SendBuffer[1] = 0x30;//FPGA Version
		SendBuffer[2] = 0;//Bootrom Version
		SendBuffer[3] = 0;//BSP Version

		for( _swIndex = 0;_swIndex<3; _swIndex++)
		{
			for( i=0; i<18; i++)
			{
				_offset = 0x158+i*0x20;
				_data = get_1848_reg(_swIndex,_offset);
				g_sw1848PortStatus[_swIndex] = (((_data&0x2)>>1)<<i)|g_sw1848PortStatus[_swIndex];
			}
		}

		//SET RIO UART CLK divisor
		_offset=0x8000000a;
		uart_setreg32(RioUartBase, 4, _offset);//clock divisor

		//srio status:ps0 (D2-P9,P5,P2,D1-P8,P4,P9,P5,P2)->{XP2[D,C,B,A],XP1[D,C,B,A]}
		//ps1(D3-P8,P4,P1,P9,P5,D2-P8,P4,P1)->{XP5[B,A],XP4[B,A],XP3[D,C,B,A]}
		//ps2(D3-P0)->XP5[C]

		//D1
		val[0] = g_sw1848PortStatus[0]&0x4;		//p2
		val[1] = g_sw1848PortStatus[0]&0x20;	//p5
		val[2] = g_sw1848PortStatus[0]&0x200;	//p9
		val[3] = g_sw1848PortStatus[0]&0x10;	//p4
		val[4] = g_sw1848PortStatus[0]&0x100;	//p8
		//D2
		val[5] = g_sw1848PortStatus[1]&0x4;		//p2
		val[6] = g_sw1848PortStatus[1]&0x20;	//p5
		val[7] = g_sw1848PortStatus[1]&0x200;	//p9
		val[8] = g_sw1848PortStatus[1]&0x2	;	//p1
		val[9] = g_sw1848PortStatus[1]&0x10;	//p4
		val[10]= g_sw1848PortStatus[1]&0x100;	//p8
		//D3
		val[11]= g_sw1848PortStatus[2]&0x20	;	//p5
		val[12]= g_sw1848PortStatus[2]&0x200;	//p9
		val[13]= g_sw1848PortStatus[2]&0x2;		//p1

		val[14]= g_sw1848PortStatus[2]&0x10;    //p4
		val[15]= g_sw1848PortStatus[2]&0x100;	//p8
		val[16]= g_sw1848PortStatus[2]&0x1;		//p0

		if(val[0] == 0x4)
			 SendBuffer[4]=(SendBuffer[4] | 1);
	    else
			 SendBuffer[4]=(SendBuffer[4] & 0xfe);

		if(val[1] == 0x20)
			 SendBuffer[4]=(SendBuffer[4] | 0x2);
		else
			 SendBuffer[4]=(SendBuffer[4] & 0xfd);

		if(val[2] == 0x200)
			 SendBuffer[4]=(SendBuffer[4] | 0x4);
		else
			 SendBuffer[4]=(SendBuffer[4] & 0xfb);

		if(val[3] == 0x10)
			 SendBuffer[4]=(SendBuffer[4] | 0x8);
		else
			 SendBuffer[4]=(SendBuffer[4] & 0xf7);

		if(val[4] == 0x100)
			 SendBuffer[4]=(SendBuffer[4] | 0x10);
		else
			 SendBuffer[4]=(SendBuffer[4] & 0xef);

		if(val[5] == 0x4)
			 SendBuffer[4]=(SendBuffer[4] | 0x20);
	    else
			 SendBuffer[4]=(SendBuffer[4] & 0xdf);

		if(val[6] == 0x20)
			 SendBuffer[4]=(SendBuffer[4] | 0x40);
	    else
			 SendBuffer[4]=(SendBuffer[4] & 0xbf);

		if(val[7] == 0x200)
			 SendBuffer[4]=(SendBuffer[4] | 0x80);
		else
			 SendBuffer[4]=(SendBuffer[4] & 0x7f);

		if(val[8] == 0x2)
			 SendBuffer[5]=(SendBuffer[5] | 0x1);
		else
			 SendBuffer[5]=(SendBuffer[5] & 0xfe);

		if(val[9] == 0x10)
			 SendBuffer[5]=(SendBuffer[5] | 0x2);
		else
			 SendBuffer[5]=(SendBuffer[5] & 0xfd);

		if(val[10] == 0x100)
			 SendBuffer[5]=(SendBuffer[5] | 0x4);
		else
			 SendBuffer[5]=(SendBuffer[5] & 0xfb);

		if(val[11] == 0x20)
			 SendBuffer[5]=(SendBuffer[5] | 0x8);
		else
			 SendBuffer[5]=(SendBuffer[5] & 0xf7);

		if(val[12] == 0x200)
			 SendBuffer[5]=(SendBuffer[5] | 0x10);
		else
			 SendBuffer[5]=(SendBuffer[5] & 0xef);

		if(val[13] == 0x2)
			 SendBuffer[5]=(SendBuffer[5] | 0x20);
		 else
			 SendBuffer[5]=(SendBuffer[5] & 0xdf);

		if(val[14] == 0x10)
			SendBuffer[5]=(SendBuffer[5] | 0x40);
		else
		    SendBuffer[5]=(SendBuffer[5] & 0xbf);

		if(val[15] == 0x100)
			SendBuffer[5]=(SendBuffer[5] | 0x80);
		else
		    SendBuffer[5]=(SendBuffer[5] & 0x7f);

		if(val[16] == 0x1)
			SendBuffer[6]=(SendBuffer[6] | 0x1);
		else
		    SendBuffer[6]=(SendBuffer[6] & 0xfe);


	}


	_offset = 0x60000000;
	uart_setreg32(RioUartBase, 0, _offset);

	//switch send 0xaa to RIO board,start the operation
	_offset=0x800000aa;
	uart_setreg32(RioUartBase, 0, _offset);

	//wait for the switch board send operation end!
	_data = uart_getreg32(RioUartBase, 0x10);

	_waitCount = 0;
	while(((_data & 0x2)==0x2) && (_waitCount <= SWBACK_BOARD_MAX_COUNT) )
	{
		_waitCount++;
		_data = uart_getreg32(RioUartBase, 0x10);
	}

	usleep(0x1fffff);
	//wait for the RIO send operation end
	_data = uart_getreg32(RioUartBase, 0x10);

	_waitCount = 0;
	while( ((_data & 0x1)==0)  && (_waitCount <= SWBACK_BOARD_MAX_COUNT) )
	{
		_waitCount++;
		_data = uart_getreg32(RioUartBase, 0x10);
	}

	if( _waitCount < SWBACK_BOARD_MAX_COUNT )
	{
		//switch read the reg data
		rio_data[0] = uart_getreg32(RioUartBase, 0x0);
		rio_data[1] = uart_getreg32(RioUartBase, 0x4);
		rio_data[2] = uart_getreg32(RioUartBase, 0x8);
		rio_data[3] = uart_getreg32(RioUartBase, 0xc);
	}

	SendBuffer[19]=(u8) (rio_data[0]>>24);//RocketIO Status(RIO)
	SendBuffer[18]=(u8) (rio_data[0]>>16);//Core Voltage(RIO)
	SendBuffer[17]=(u8) (rio_data[0]>>8);//Aux Volgate(RIO)
	SendBuffer[16]=(u8) (rio_data[0]);//RIO temp(RIO)

	SendBuffer[23]=(u8) (rio_data[1]>>24);
	SendBuffer[22]=(u8) (rio_data[1]>>16);
	SendBuffer[21]=(u8) (rio_data[1]>>8);
	SendBuffer[20]=(u8) (rio_data[1]);

	SendBuffer[27]=(u8) (rio_data[2]>>24);
	SendBuffer[26]=(u8) (rio_data[2]>>16);
	SendBuffer[25]=(u8) (rio_data[2]>>8);
	SendBuffer[24]=(u8) (rio_data[2]);

	SendBuffer[31]=(u8) (rio_data[3]>>24);
	SendBuffer[30]=(u8) (rio_data[3]>>16);
	SendBuffer[29]=(u8) (rio_data[3]>>8);
	SendBuffer[28]=(u8) (rio_data[3]);
#if 0
	xil_printf("Interface status: \r\n");
	xil_printf("reg-0x0 = 0x%02x%02x%02x%02x\r\n",SendBuffer[19],SendBuffer[18],SendBuffer[17],SendBuffer[16]);
	xil_printf("reg-0x4 = 0x%02x%02x%02x%02x\r\n",SendBuffer[23],SendBuffer[22],SendBuffer[21],SendBuffer[20]);
	xil_printf("reg-0x8 = 0x%02x%02x%02x%02x\r\n",SendBuffer[27],SendBuffer[26],SendBuffer[25],SendBuffer[24]);
	xil_printf("reg-0xc = 0x%02x%02x%02x%02x\r\n",SendBuffer[31],SendBuffer[30],SendBuffer[29],SendBuffer[28]);
#endif

	SendBuffer[7] = (u8)get_temp(0);
	SendBuffer[8] = (u8)get_temp(1);

	SendBuffer[9]=(u8)get_vcc(0);
	SendBuffer[10]=(u8)get_vcc(1);
	SendBuffer[11]=(u8)get_vcc(2);

	GpioReset();

	iic_write();

	bI2cSelfTest++;
	return 1;
}

