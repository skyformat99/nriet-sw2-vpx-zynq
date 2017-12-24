#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "iic_1848.h"
#include "gpio.h"

int SysReset_Ctl();

extern unsigned int hlSrioGetChildSw(unsigned int parentSw,unsigned int parentPort,unsigned int *childPort);
extern void hlWriteReg(unsigned char swIndex,unsigned int offset,unsigned int data);
extern unsigned int  hlReadReg(unsigned char swIndex,unsigned int offset);
extern void hlSrioReconfigLUT(unsigned int  swIndex);
extern unsigned int hlSrioGetSwLocalID(unsigned int swIndex,unsigned int swIndexPort[]);


void disable_1848_lane(int num,unsigned char lane_num)
{
	unsigned int  tmp;
	tmp = get_1848_reg(num,0xFF8000ul + (0x100 * lane_num));
	set_1848_reg(num,0xFF8000ul + (0x100 * lane_num), tmp | 1);
}

void enable_1848_lane(int num,unsigned char lane_num)
{
	unsigned int tmp;
	tmp = get_1848_reg(num,0xFF8000ul + (0x100 * lane_num));
	set_1848_reg(num,0xFF8000ul + (0x100 * lane_num), (tmp & 0xfffffffeul) /*| (32 << 5)*/);
}

void iic_1848PortRepair(unsigned char swIndex)
{	
	int i;
	unsigned int offset,val,val2,_portNum;
	unsigned int  portCnt = 18;
	
	//repare the RIO end point
	if(swIndex==2)
	{
	    offset = 0x158+1*0x20;
	    val = get_1848_reg(swIndex,offset);

	    offset = 0x158+9*0x20;
	    val2 = get_1848_reg(swIndex,offset);

	    printf("switch 2  port 1 , value = 0x%x\n\r",val);
	    printf("switch 2  port 9 , value = 0x%x\n\r",val2);
	    while(((val&0x3)==0) || ((val2&0x3)==0)||((val&0x10100) !=0)||((val2&0x10100) !=0))
	    {
			//GpioOutputExample(XPAR_AXI_GPIO_1_DEVICE_ID, 0x1);//rio enter reset             zz zz
			//GpioOutputExample(XPAR_AXI_GPIO_1_DEVICE_ID, 0x0);//rio leave reset				zz zz
			//delay_SL(0x1fffff);
	    	usleep(10000);
			for(i=0;i<2;i++)
			{
				if( i== 0 )
					_portNum = 1;
				else
					_portNum = 9;

				//lock port
				offset = 0x15c+_portNum*0x20;
				val = get_1848_reg(swIndex,offset);
				val |= 0x2;
				set_1848_reg(swIndex,offset,val);

				//recover failed link
				offset = 0x148+_portNum*0x20;
				val = get_1848_reg(swIndex,offset);
				val |= 0x80000000;
				//delay_SL(0x1fffff);
				usleep(10000);
				set_1848_reg(swIndex,offset,val);
				//delay_SL(0x1fffff);
				usleep(10000);
				set_1848_reg(swIndex,offset,0);
				//delay_SL(0x1fffff);
				usleep(10000);

				offset = 0x140+_portNum*0x20;
				set_1848_reg(swIndex,offset,0x4);
				//unlock port
				offset = 0x15c+_portNum*0x20;
				val = get_1848_reg(swIndex,offset);
				val = val&0xfffffffd;
				set_1848_reg(swIndex,offset,val);
				//delay_SL(0x1fffff);
				usleep(10000);

				offset = 0x158+_portNum*0x20;
				val = get_1848_reg(swIndex,offset);
				set_1848_reg(swIndex,offset,val);

				printf("read port%d again, value=0x%x\n\r",_portNum,val);
			}

			offset = 0x158+1*0x20;
			val = get_1848_reg(swIndex,offset);

			offset = 0x158+9*0x20;
			val2 = get_1848_reg(swIndex,offset);
        }
	}

	printf("Switch %d********Base Address: 0x%x!\n\r",swIndex,0x158);

	//将port_rst_ctl置上1
	//这样1848端口在收到control symbol之后，
	//就仅会对端口进行复位，而不是整个1848芯片进行复位
	val = get_1848_reg(swIndex,0xf2000c);
	val |= 0x1;
	set_1848_reg(swIndex,0xf2000c,val);
	
	for( i=0; i<portCnt; i++)
	{
		offset = 0x158+i*0x20;
		val = get_1848_reg(swIndex,offset);

		if ((val & 0x30300) != 0)
		{
			printf("switch %d  port %d , value = 0x%x\n\r",swIndex,i,val);
			if( ( swIndex == 0 ) && ( i== 13 ) )	//zynq endpoint
			{
				disable_1848_lane(swIndex,6);
				//delay_SL(0x1fffff);
				usleep(10000);

				reset_1848_port(swIndex,13);
				//delay_SL(0x1fffff);
				usleep(10000);
				enable_1848_lane(swIndex,6);
				//delay_SL(0x1fffff);
				usleep(10000);

				val = get_1848_reg(swIndex,0x158+i*0x20);
				set_1848_reg(swIndex,0x158+i*0x20,val);
				//delay_SL(0x1fffff);
				usleep(10000);
				val = get_1848_reg(swIndex,0x158+i*0x20);
				printf("read port %d again, value=0x%x\n\r",i,val);
			}
			else
			{
				//如果出现input_err_stop或output_err_stop
				if((val & 0x10100) != 0)
				{
					//lock port
					offset = 0x15c+i*0x20;
					val = get_1848_reg(swIndex,offset);
					val |= 0x2;
					set_1848_reg(swIndex,offset,val);
					
					//recover failed link
					offset = 0x148+i*0x20;
					val = get_1848_reg(swIndex,offset);
					val |= 0x80000000;
					//delay_SL(0x1fffff);
					usleep(10000);
					set_1848_reg(swIndex,offset,val);
					//delay_SL(0x1fffff);
					usleep(10000);
					set_1848_reg(swIndex,offset,0);
					//delay_SL(0x1fffff);
					usleep(10000);
					
					offset = 0x140+i*0x20;
					set_1848_reg(swIndex,offset,0x4);
					
					offset = 0x15c+i*0x20;
					val = get_1848_reg(swIndex,offset);
					val = val&0xfffffffd;
					set_1848_reg(swIndex,offset,val);
					//delay_SL(0x1fffff);
					usleep(10000);
					
					offset = 0x158+i*0x20;
					val = get_1848_reg(swIndex,offset);
					set_1848_reg(swIndex,offset,val);
					//delay_SL(0x1fffff);
					usleep(10000);
					val = get_1848_reg(swIndex,offset);
					printf("read port %d again, value=0x%x\n\r",i,val);
				}
				else
				{					
					set_1848_reg(swIndex,offset,val);
					val = get_1848_reg(swIndex,offset);
					printf("read port %d again, value=0x%x\n\r",i,val);
				}
			}
		}				
	}
}


void iic_SystemReset()
{
	int _sys_rst_ctl;
	_sys_rst_ctl=SysReset_Ctl();
	if(_sys_rst_ctl==1)
	{
		printf("-------------------Switch_App_System_Reseting-------------------\r\n");
		SetGpioReg(GPIO_RESET_ADDR,0x4,0x0);
		SetGpioReg(GPIO_RESET_ADDR,0x0,0x5500);
	}
}


void iic_SwPortRepair()
{
	int i;
	for( i=0; i<3; i++)
		iic_1848PortRepair(i);
}

#if 0
void iic_InterfaceRepair()
{
	int i;
	unsigned int offset,val,val2,_portNum;

	IIC1848Init(0x02<<2, IIC_SCLK_RATE);

	offset = 0x158+1*0x20;
	val = get_1848_reg(offset);

	offset = 0x158+9*0x20;
	val2 = get_1848_reg(offset);

	printf("After Enum, Switch 2  port 1 , value = 0x%x\n\r",val);
	printf("After Enum, Switch 2  port 9 , value = 0x%x\n\r",val2);

	while(((val&0x30300)!=0)||((val2&0x30300)!=0))
	{
		if(((val&0x10100)!=0)||((val2&0x10100)!=0))
		{
			for(i=0;i<2;i++)
			{
				if( i== 0 )
					_portNum = 1;
				else
					_portNum = 9;

				//lock port
				offset = 0x15c+_portNum*0x20;
				val = get_1848_reg(offset);
				val |= 0x2;
				set_1848_reg(offset,val);

				//recover failed link
				offset = 0x148+_portNum*0x20;
				val = get_1848_reg(offset);
				val |= 0x80000000;
				delay_SL(0x1fffff);
				set_1848_reg(offset,val);
				delay_SL(0x1fffff);
				set_1848_reg(offset,0);
				delay_SL(0x1fffff);

				offset = 0x140+_portNum*0x20;
				set_1848_reg(offset,0x4);

				offset = 0x15c+_portNum*0x20;
				val = get_1848_reg(offset);
				val = val&0xfffffffd;
				set_1848_reg(offset,val);
				delay_SL(0x1fffff);

				offset = 0x158+_portNum*0x20;
				val = get_1848_reg(offset);
				set_1848_reg(offset,val);

				printf("read port%d again, value=0x%x\n\r",_portNum,val);
			}

			offset = 0x158+1*0x20;
			val = get_1848_reg(offset);

			offset = 0x158+9*0x20;
			val2 = get_1848_reg(offset);
		}
		else
		{
			_portNum = 1;
			offset = 0x158+_portNum*0x20;
			val = get_1848_reg(offset);
			set_1848_reg(offset,val);

			_portNum = 9;
			offset = 0x158+_portNum*0x20;
			val = get_1848_reg(offset);
			set_1848_reg(offset,val);
			break;
		}
	}
}
#endif

void iic_1848PortCountEnable(unsigned char swIndex)
{
	int i;
	unsigned int offset;
	unsigned int portCnt = 18;

	for( i=0; i<portCnt; i++)
	{
		offset = 0xf40004+i*0x100;
		set_1848_reg(swIndex,offset,0x6400000);
	}
}


void iic_SwPortCounterEnable()
{
	int i;
	for( i=0; i<3; i++)
		iic_1848PortCountEnable(i);
}


void SrioPortStatic(unsigned char swIndex)
{	
	unsigned int pdata[8];
	unsigned int offset = 0;
	int i;		
		
	printf("--------------------------------------------------SWITCH-%d TX---------------------------------------------------------------\n",swIndex);
	printf("port      pstatus         pa               pna            retrysymbol        alltx            droptx            pwidth            dropttl\n");
												
	for( i=0; i<18; i++)
	{				
		offset = 0x158+i*0x20;
		pdata[0] = get_1848_reg(swIndex,offset);
		
		offset = 0xf40010+i*0x100;
		pdata[1] = get_1848_reg(swIndex,offset);

		offset = 0xf40014+i*0x100;
		pdata[2] = get_1848_reg(swIndex,offset);

		offset = 0xf40018+i*0x100;
		pdata[3] = get_1848_reg(swIndex,offset);

		offset = 0xf4001c+i*0x100;
		pdata[4] = get_1848_reg(swIndex,offset);

		offset = 0xf40068+i*0x100;
		pdata[5] = get_1848_reg(swIndex,offset);
		
		offset = 0x15c+i*0x20;
		pdata[6] = get_1848_reg(swIndex,offset);

		offset = 0xf4006c+i*0x100;
		pdata[7] = get_1848_reg(swIndex,offset);

		printf("0x%.2x     0x%.8x      0x%.8x      0x%.8x       0x%.8x         0x%.8x       0x%.8x       0x%.8x       0x%.8x\n",i,pdata[0],pdata[1],pdata[2],pdata[3],pdata[4],pdata[5],pdata[6],pdata[7]);
	}
	
	printf("\n");
	

	printf("--------------------------------------------------SWITCH-%d RX---------------------------------------------------------------\n",swIndex);
	
	printf("port      pstatus         pa               pna            retrysymbol        allrx            droprx            pwidth            dropttl\n");
				
	for( i=0; i<18; i++)
	{				
		offset = 0x158+i*0x20;
		pdata[0] = get_1848_reg(swIndex,offset);
		
		offset = 0xf40040+i*0x100;
		pdata[1] = get_1848_reg(swIndex,offset);

		offset = 0xf40044+i*0x100;
		pdata[2] = get_1848_reg(swIndex,offset);

		offset = 0xf40048+i*0x100;
		pdata[3] = get_1848_reg(swIndex,offset);

		offset = 0xf40050+i*0x100;
		pdata[4] = get_1848_reg(swIndex,offset);

		offset = 0xf40064+i*0x100;
		pdata[5] = get_1848_reg(swIndex,offset);
		
		offset = 0x15c+i*0x20;
		pdata[6] = get_1848_reg(swIndex,offset);

		offset = 0xf4006c+i*0x100;
		pdata[7] = get_1848_reg(swIndex,offset);

		printf("0x%.2x     0x%.8x      0x%.8x      0x%.8x       0x%.8x         0x%.8x       0x%.8x       0x%.8x       0x%.8x\n",i,pdata[0],pdata[1],pdata[2],pdata[3],pdata[4],pdata[5],pdata[6],pdata[7]);
	}
	
	printf("\n");
}


void SrioErrorDetect(unsigned char swIndex)
{
	int i;
	unsigned int pdata[10];
	unsigned int offset = 0;
	static short _errorInit = 0;
	if( _errorInit == 0 )
	{
		_errorInit = 1;
		for( i=0; i<18; i++)
		{
			offset = 0x1044+i*0x40;
			set_1848_reg(swIndex,offset,0x807e8037);
		}

		for( i=0; i<48; i++)
		{
			offset = 0xff8010+i*0x100;
			set_1848_reg(swIndex,offset,0x1fff);
		}

	}

	printf("--------------------------------------------------SWITCH-%d LANE---------------------------------------------------------------\n",swIndex);
	printf("          lane0           lane1           lane2           lane3           lane4           lane5           lane6           lane7\n");

	for( i=0; i<8; i++)
	{
		offset = 0x2010+i*0x20;
		pdata[i] = get_1848_reg(swIndex,offset);
	}
	printf("0x2010    0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x\n",pdata[0],pdata[1],pdata[2],pdata[3],pdata[4],pdata[5],pdata[6],pdata[7]);

	for( i=0; i<8; i++)
	{
		offset = 0xff800c+i*0x100;
		pdata[i] = get_1848_reg(swIndex,offset);
	}
	printf("0xff800c  0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x\n\n",pdata[0],pdata[1],pdata[2],pdata[3],pdata[4],pdata[5],pdata[6],pdata[7]);

	printf("          lane8           lane9           lane10          lane11          lane12          lane13           lane14          lane15\n");

	for( i=0; i<8; i++)
	{
		offset = 0x2010+(i+8)*0x20;
		pdata[i] = get_1848_reg(swIndex,offset);
	}
	printf("0x2010    0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x\n",pdata[0],pdata[1],pdata[2],pdata[3],pdata[4],pdata[5],pdata[6],pdata[7]);

	for( i=0; i<8; i++)
	{
		offset = 0xff800c+(i+8)*0x100;
		pdata[i] = get_1848_reg(swIndex,offset);
	}
	printf("0xff800c  0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x\n\n",pdata[0],pdata[1],pdata[2],pdata[3],pdata[4],pdata[5],pdata[6],pdata[7]);

	printf("          lane16           lane17          lane18          lane19          lane20          lane21           lane22          lane23\n");
	for( i=0; i<8; i++)
	{
		offset = 0x2010+(i+16)*0x20;
		pdata[i] = get_1848_reg(swIndex,offset);
	}
	printf("0x2010    0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x\n",pdata[0],pdata[1],pdata[2],pdata[3],pdata[4],pdata[5],pdata[6],pdata[7]);

	for( i=0; i<8; i++)
	{
		offset = 0xff800c+(i+16)*0x100;
		pdata[i] = get_1848_reg(swIndex,offset);
	}
	printf("0xff800c  0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x\n\n",pdata[0],pdata[1],pdata[2],pdata[3],pdata[4],pdata[5],pdata[6],pdata[7]);

	printf("          lane24           lane25          lane26          lane27          lane28          lane29           lane30          lane31\n");
	for( i=0; i<8; i++)
	{
		offset = 0x2010+(i+24)*0x20;
		pdata[i] = get_1848_reg(swIndex,offset);
	}
	printf("0x2010    0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x\n",pdata[0],pdata[1],pdata[2],pdata[3],pdata[4],pdata[5],pdata[6],pdata[7]);

	for( i=0; i<8; i++)
	{
		offset = 0xff800c+(i+24)*0x100;
		pdata[i] = get_1848_reg(swIndex,offset);
	}
	printf("0xff800c  0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x\n\n",pdata[0],pdata[1],pdata[2],pdata[3],pdata[4],pdata[5],pdata[6],pdata[7]);


	printf("          lane32           lane33          lane34          lane35          lane36          lane37           lane38          lane39\n");
	for( i=0; i<8; i++)
	{
		offset = 0x2010+(i+32)*0x20;
		pdata[i] = get_1848_reg(swIndex,offset);
	}
	printf("0x2010    0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x\n",pdata[0],pdata[1],pdata[2],pdata[3],pdata[4],pdata[5],pdata[6],pdata[7]);

	for( i=0; i<8; i++)
	{
		offset = 0xff800c+(i+32)*0x100;
		pdata[i] = get_1848_reg(swIndex,offset);
	}
	printf("0xff800c  0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x\n\n",pdata[0],pdata[1],pdata[2],pdata[3],pdata[4],pdata[5],pdata[6],pdata[7]);


	printf("          lane40           lane41          lane42          lane43          lane44          lane45           lane46          lane47\n");
	for( i=0; i<8; i++)
	{
		offset = 0x2010+(i+40)*0x20;
		pdata[i] = get_1848_reg(swIndex,offset);
	}
	printf("0x2010    0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x\n",pdata[0],pdata[1],pdata[2],pdata[3],pdata[4],pdata[5],pdata[6],pdata[7]);

	for( i=0; i<8; i++)
	{
		offset = 0xff800c+(i+40)*0x100;
		pdata[i] = get_1848_reg(swIndex,offset);
	}
	printf("0xff800c  0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x      0x%.8x\n\n",pdata[0],pdata[1],pdata[2],pdata[3],pdata[4],pdata[5],pdata[6],pdata[7]);


	printf("\n");

	printf("--------------------------------------------------SWITCH-%d PORT---------------------------------------------------------------\n",swIndex);

	printf("port      ERRCSR(0x1040)         SPECERR(0xF40008) \n");

	for( i=0; i<18; i++)
	{
		offset = 0x1040+i*0x40;
		pdata[0] = get_1848_reg(swIndex,offset);

		offset = 0xF40008+i*0x100;
		pdata[1] = get_1848_reg(swIndex,offset);

		printf("0x%.2x     0x%.8x              0x%.8x\n",i,pdata[0],pdata[1]);
	}

}

void IIC_SrioPortStaticShow()
{
	int i;

	for( i=0; i<3; i++)
	{
		SrioPortStatic(i);
	}
}


void iic_SrioErrorDetect()
{
	int i;

	for( i=0; i<3; i++)
	{
		SrioErrorDetect(i);
	}
}

void iic_1848PortClear(unsigned char swIndex)
{
	int i;
	unsigned int offset;
	unsigned short portCnt = 18;

	printf("Switch %d--------Clear port !\n\r",swIndex);

	for( i=0; i<portCnt; i++)
	{
		offset = 0x1040+i*0x40;
		set_1848_reg(swIndex,offset,0);
		usleep(10000);
		offset = 0xf40008+i*0x100;
		set_1848_reg(swIndex,offset,0);
	}

	for( i=0; i<48; i++)
	{
		offset = 0xff800c+i*0x100;
		set_1848_reg(swIndex,offset,0);
	}
}


void iic_SwPortClear()
{
	int i;
	for( i=0; i<3; i++)
		iic_1848PortClear(i);
}



//speedIndex-- 0--3.125G;	1--5.0G;		2--6.25G
void iic_ChangePLL(unsigned char swIndex,unsigned char speedIndex)
{
	unsigned int _data;
	unsigned short _portNum;
	unsigned short _laneNum;
	unsigned short _portTable[12];
	float _speed;
	int i,j;

	if( swIndex > 2 || speedIndex > 2 )
	{
		printf("----iic_ChangePLL:: index error----\r\n");
		return;
	}

	if( speedIndex == 0 )
		_speed = 3.125;
	else if( speedIndex == 1 )
		_speed = 5.0;
	else
		_speed = 6.25;

	_portTable[0] = 0;
	_portTable[1] = 7;
	_portTable[2] = 11;
	_portTable[3] = 3;
	_portTable[4] = 6;
	_portTable[5] = 10;
	_portTable[6] = 2;
	_portTable[7] = 4;
	_portTable[8] = 5;
	_portTable[9] = 8;
	_portTable[10] = 9;
	_portTable[11] = 1;		//sw0对外没有该端口


	printf( "stage 1 disable ports.\n\r" );
	for(i=0;i<12;i++)
	{
		if( swIndex == 0 && i==11 )
			continue;

		_portNum = _portTable[i];
		_data = get_1848_reg(swIndex,0x15c+_portNum*0x20);
		//xil_printf( "stage1:set before port-%d status = 0x%x.\n\r",_portNum,_data);
		set_1848_reg(swIndex,0x15c+_portNum*0x20,_data|0x800000);
		//_data = get_1848_reg(0x15c+_portNum*0x20);
		//xil_printf( "stage1:now port-%d status = 0x%x.\n\r",_portNum,_data);
	}

	printf( "stage 2 change pll selection.\n\r" );
	for(i=0;i<12;i++)
	{
		if( swIndex == 0 && i==11 )
			continue;

		_portNum = _portTable[i];
		_data = get_1848_reg(swIndex,0xff0000+_portNum*0x10);

		if( speedIndex == 1 )		//5.0G
		{
			_data = _data&0xfe;
		}
		else 		//3.125G/6.25G
		{
			_data = _data|0x1;
		}
		set_1848_reg(swIndex,0xff0000+_portNum*0x10,_data);
	}

	printf( "stage 3 set %.3fGbaud lane rate.\n\r",_speed);
	for(i=0;i<12;i++)
	{
		if( swIndex == 0 && i==11 )
			continue;

		_portNum = _portTable[i];
		for(j=0;j<4;j++)
		{
			_laneNum = _portNum*4+j;
			_data = get_1848_reg(swIndex,0xff8000+_laneNum*0x100);
			_data = _data&0xffffffe1;

			if( speedIndex == 1 || speedIndex == 2 )	//5.0G/6.25G
			{
				_data = _data|0x14;
			}
			else			//3.125G
			{
				_data = _data|0xa;
			}
			set_1848_reg(swIndex,0xff8000+_laneNum*0x100,_data);
		}
	}

	printf( "stage 4 reset port.\n\r");
	_data = 0x80000000;
	for(i=0;i<12;i++)
	{
		if( swIndex == 0 && i==11 )
			continue;

		_portNum = _portTable[i];
		_data = _data|(1<<_portNum);
		//pll_num
		_data = _data|(1<<(18+_portNum));
	}
	set_1848_reg(swIndex,0xf20300,_data);
	sleep(1);

	//_data = get_1848_reg(0xf20300);
	//xil_printf( "stage4:set 0xf20300 data = 0x%x.\n\r",_data);
	//set_1848_reg(0xf20300,_data);

	printf( "stage 5 enable ports \n\r" );

	for(i=0;i<12;i++)
	{
		if( swIndex == 0 && i==11 )
			continue;

		_portNum = _portTable[i];
		_data = get_1848_reg(swIndex,0x15c+_portNum*0x20);
		//xil_printf( "stage5:set before port-%d status = 0x%x.\n\r",_portNum,_data);
		set_1848_reg(swIndex,0x15c+_portNum*0x20,_data&0xff7fffff);
		//_data = get_1848_reg(0x15c+_portNum*0x20);
		//xil_printf( "stage5:now port-%d status = 0x%x.\n\r",_portNum,_data);
	}

	sleep(1);
}

void iic_SwSpeedBaudSet(unsigned int speedIndex)
{
	unsigned int _speed;
	float _baud;
	int i;
	FILE *pFile = NULL;

	for( i=0; i<3; i++)
		iic_ChangePLL(i,speedIndex);

	if( speedIndex > 2 )
	{
		printf("----iic_SwSpeedBaudSet:: speedIndex error----\r\n");
		return;
	}

	pFile = fopen("/mnt/speed.txt","wb+");
	if( pFile == NULL )
	{
		printf("open speed.txt file failed, set default speed 3.125G.\n");
		_speed = 0;
	}
	else
	{
		fwrite((void*)&speedIndex,4,1,pFile);
	}
	fclose( pFile );

	pFile = fopen("/mnt/speed.txt","rb");
	if( pFile == NULL )
	{

	}
	else
	{
		fread((void*)&_speed,1,4,pFile);
	}
	fclose( pFile );

	if( _speed == 0 )
		_baud = 3.125;
	else if( _speed == 1 )
		_baud = 5.0;
	else if( _speed == 2 )
		_baud = 6.25;

	printf("read srio switch port speed baud from flash, speed = %d, the value is %.3fG!\n\r",_speed,_baud);
}


void iic_PLLLaneShow(unsigned char swIndex)
{
	unsigned int pdata[8];
	unsigned short _portTable[12];
	unsigned short _portNum,_laneNum;
	int i,j;

	if( swIndex > 2 )
	{
		printf("----iic_ChangePLL:: index error----\r\n");
		return;
	}

	_portTable[0] = 0;
	_portTable[1] = 7;
	_portTable[2] = 11;
	_portTable[3] = 3;
	_portTable[4] = 6;
	_portTable[5] = 10;
	_portTable[6] = 2;
	_portTable[7] = 4;
	_portTable[8] = 5;
	_portTable[9] = 8;
	_portTable[10] = 9;
	_portTable[11] = 1;		//sw0对外没有该端口

	printf("--------------------------------------------------SWITCH-%d ---------------------------------------------------------------\n",swIndex);
	printf("port      pll(0xff0000)         lane0(0xff8000)	   lane1			lane2			lane3\r\n");

	for( i=0; i<12; i++)
	{
		if( swIndex == 0 && i==11 )
			continue;

		_portNum = _portTable[i];

		pdata[0] = get_1848_reg(swIndex,0xff0000+_portNum*0x10);

		for(j=0;j<4;j++)
		{
			_laneNum = _portNum*4+j;
			pdata[j+1] = get_1848_reg(swIndex,0xff8000+_laneNum*0x100);
		}

		printf("%.2d         0x%.8x     	 0x%.8x  	    0x%.8x        0x%.8x        0x%.8x\r\n",_portNum,pdata[0],pdata[1],pdata[2],pdata[3],pdata[4]);
	}
}

void iic_SwPLLLaneShow()
{
	int i;

	for( i=0; i<3; i++)
		iic_PLLLaneShow(i);
}

void iic_TTL_csr(unsigned int ttl_count)
{
	int i=0;
	unsigned int offset = 0x102c;

	for(i=0;i<3;i++)
	{
		set_1848_reg(i,offset,ttl_count);
	}
}


int SysReset_Ctl()
{
	int i;
	unsigned int offset;
	int _index;
	unsigned int pdata1[18], pdata2[18];
	unsigned int portCnt = 18;

    for (_index=0; _index<3; _index++)
    {
	   printf("\n");
	   printf("--------------------------------------------------SWITCH-%d PORT---------------------------------------------------------------\n",_index);

	   printf("port      ERRCSR(0x1040)         SPECERR(0xF40008) \n");
	   for( i=0; i<portCnt; i++)
	   {
		   offset = 0x1040+i*0x40;
		   pdata1[i]=get_1848_reg(_index,offset);

		   offset = 0xf40008+i*0x100;
		   pdata2[i]=get_1848_reg(_index,offset);

		   printf("0x%.2x     0x%.8x              0x%.8x\n",i,pdata1[i],pdata2[i]);
	    }

	   usleep(10000);
	   for(i=0;i<portCnt;i++)
	   {
		   if((pdata1[i]!=0) || (pdata2[i]!=0))
			   return 1;
	   }
    }

	return 0;
}


void disable_1848_port_rxtx(unsigned char swIndex,unsigned char portNum)
{
	unsigned int offset,_data;

	if( swIndex > 2 )
	{
		printf("----disable_1848_port_rxtx:: swIndex error----%d\r\n",swIndex);
		return;
	}

	if( portNum>=18 )
	{
		printf("----disable_1848_port_rxtx:: portNum error----%d\r\n",portNum);
		return;
	}

	offset = 0x15c+portNum*0x20;
	_data = get_1848_reg(swIndex,offset);

	do
	{
		_data &= 0xff9fffff;
		set_1848_reg(swIndex,offset,_data);
		_data = get_1848_reg(swIndex,offset);
		printf("disable_1848_port_rxtx::[%d][%d] status----0x%x\r\n",swIndex,portNum,_data);
	}while((_data&0x600000)!=0);

}


void enable_1848_port_rxtx(unsigned char swIndex,unsigned char portNum)
{
	unsigned int offset,_data;

	if( swIndex > 2 )
	{
		printf("----enable_1848_port_rxtx:: swIndex error----%d\r\n",swIndex);
		return;
	}

	if( portNum>=18 )
	{
		printf("----enable_1848_port_rxtx:: portNum error----%d\r\n",portNum);
		return;
	}

	offset = 0x15c+portNum*0x20;
	_data = get_1848_reg(swIndex,offset);

	do
	{
		_data |= 0x600000;
		set_1848_reg(swIndex,offset,_data);
		_data = get_1848_reg(swIndex,offset);
		printf("enable_1848_port_rxtx::[%d][%d] status----0x%x\r\n",swIndex,portNum,_data);
	}while((_data&0x600000)!=0x600000);

}


void reset_1848_ackid(unsigned char swIndex,unsigned char portNum)
{
	unsigned int offset,_data;

	offset = 0x148+portNum*0x20;
	_data = 0;
	set_1848_reg(swIndex,offset,_data);
}

void reset_1848_port(unsigned char swIndex,unsigned char portNum)
{
	unsigned int reset_ctl;
	unsigned int _data;

	if( swIndex > 2 )
	{
		printf("----disable_1848_port_rxtx:: swIndex error----%d\r\n",swIndex);
		return;
	}

	if( portNum>=18 )
	{
		printf("----disable_1848_port_rxtx:: portNum error----%d\r\n",portNum);
		return;
	}

	_data = get_1848_reg(swIndex,0xf2000c);
	_data = _data | 0x1;

	set_1848_reg(swIndex,0xf2000c,_data);

	reset_ctl = (1 << 31)|(1 << portNum);

	set_1848_reg(swIndex,0xF20300, reset_ctl);
}

void iic_SwPortErrClear(int swIndex,int port)
{
	int i;
	unsigned int _data,_offset;

	for(i=0;i<4;i++)
	{
		_offset = 0xff800c+(port*4+i)*0x100;
		set_1848_reg(swIndex,_offset,0);
	}

	_offset = 0xf40008+port*0x100;
	set_1848_reg(swIndex,_offset,0);

	_offset = 0x1040+port*0x40;
	set_1848_reg(swIndex,_offset,0);

	_offset = 0x158+port*0x20;
	_data = get_1848_reg(swIndex,_offset);
	set_1848_reg(swIndex,_offset,_data);
}

void RepairLinkPartner(int swIndex,int port)
{
	unsigned int _data[2],_offset,_val,_retryCnt;
	unsigned int _childSw,_childPort,_childID[10];
	int i;

	if( (swIndex > 2) || (port > 10) )		//对外端口最大端口号是9
		return;

	//获取0x1040寄存器的值
	_data[0] = get_1848_reg(swIndex,0x1040+0x40*port);
	//获取0xff800c寄存器的值--lane寄存器，这里只需要读取一个lane的值进行判断
	_data[1] = get_1848_reg(swIndex,0xff800c+0x100*port*4);

	if( ((_data[0]&0x4) == 0x4) && ((_data[1]&0x7) == 0x7 ) )			//该LinkPartner复位过
	{
			printf("交换板内1848-%d  端口-%d 对应的模块已复位。\r\n",swIndex,port);

			disable_1848_port_rxtx(swIndex,port);
			reset_1848_port(swIndex,port);
			sleep(3);				//等待3s，以防止处理板再次复位或者同时也在做修复端口操作

			_childSw = hlSrioGetChildSw(swIndex,port,&_childPort);		//根据交换板内部交换芯片和端口号得到相连的交换ID和端口
			printf("childSw = %d childport = %d.\r\n",_childSw,_childPort);

			reset_1848_ackid(swIndex,port);
			printf("reset 1848 ackid ok.\r\n");

			_retryCnt = 0;
			do
			{
				_offset = 0x140+port*0x20;
				set_1848_reg(swIndex,_offset,4);
				_offset = 0x144+port*0x20;
				_val = get_1848_reg(swIndex,_offset);
				usleep(0x1ffff);
				_retryCnt++;
			}while(((_val&0x80000000)==0)&&(_retryCnt<=50));

			if( _retryCnt >= 50 )
			{
				printf("修复失败\r\n");
				return;
			}
			printf("link partner response ok.\r\n");

			_offset = 0x140+port*0x20;
			set_1848_reg(swIndex,_offset,3);
			//printf("reset  link partner.\r\n");
			usleep(0x1ffff);
			//recover failed link
			_offset = 0x148+port*0x20;
			_val = get_1848_reg(swIndex,_offset);
			_val |= 0x80000000;
			set_1848_reg(swIndex,_offset,_val);
			usleep(0x1ffff);
			set_1848_reg(swIndex,_offset,0);
			usleep(0x1ffff);

			_offset = 0x140+port*0x20;
			set_1848_reg(swIndex,_offset,4);
			printf("reset link partner ok.\r\n");

			//得到childSw的所有本地节点id，目的是将路由去除，否则在配置childSw节点ID时会被数据包堵死
			_val = hlSrioGetSwLocalID(_childSw,_childID);
			if( _val > 10 )
			{
				printf("!!!警告,_childSw ID 个数 = 0x%d.超过了数组最大个数10.\r\n",_val);
				_val = 10;
			}

			for( i=0; i<_val; i++)
			{
				set_1848_reg(swIndex,0x70,_childID[i]);
				set_1848_reg(swIndex,0x74,0xDF);
				printf("Discard LUT::Local ID = 0x%x.\r\n",_childID[i]);
			}
			usleep(0x1ffff);
			//需要重新配置端口模式，否则维护包会发送不过去
			_offset = 0x15c+port*0x20;
			set_1848_reg(swIndex,_offset,0xd0400001);

			//先将id为0xff的路由配置到相连重启的交换芯片上
			if( swIndex == 1 )
			{
				set_1848_reg(0,0x70,0xff);
				set_1848_reg(0,0x74,11);				//port0/port7/port11都可以
			}
			else if( swIndex == 2 )					//正常不会进入该分支，从1槽到12槽底板链路都链接在交换芯片0和交换芯片1上
			{
				set_1848_reg(0,0x70,0xff);
				set_1848_reg(0,0x74,10);				//port3/port6/port10都可以
			}

			set_1848_reg(swIndex,0x70,0xff);
			set_1848_reg(swIndex,0x74,port);
			//在配置处理板路由表时，处理板不能再次复位，否则会被挂死
			printf("开始配置处理板路由表及ID.\r\n");
			hlSrioReconfigLUT(_childSw);


			//重新配置childSw的所有本地节点路由表
			for( i=0; i<_val; i++)
			{
				set_1848_reg(swIndex,0x70,_childID[i]);
				set_1848_reg(swIndex,0x74,port);
			}

			enable_1848_port_rxtx(swIndex,port);

			do
			{
				iic_SwPortErrClear(swIndex,port);
				_data[0] = get_1848_reg(swIndex,0x1040+0x40*port);
				_data[1] = get_1848_reg(swIndex,0xff800c+0x100*port*4);
				//printf("_data[0] = 0x%x, _data[1] = 0x%x.\r\n",_data[0],_data[1]);
			}while( ((_data[0]&0x4) == 0x4) && ((_data[1]&0x7) == 0x7 ) );

	}

}

unsigned short m_swOut[3][6];
void LinkDetect()
{
	int i,_swIndex,_swPort;
	static int m_bInitLinkDetect = 0;

	if(m_bInitLinkDetect == 0 )
	{
		m_bInitLinkDetect = 1;

		m_swOut[0][0] = 2;
		m_swOut[0][1] = 4;
		m_swOut[0][2] = 5;
		m_swOut[0][3] = 8;
		m_swOut[0][4] = 9;

		m_swOut[1][0] = 1;
		m_swOut[1][1] = 2;
		m_swOut[1][2] = 4;
		m_swOut[1][3] = 5;
		m_swOut[1][4] = 8;
		m_swOut[1][5] = 9;

		m_swOut[2][0] = 0;
		m_swOut[2][1] = 1;
		m_swOut[2][2] = 4;
		m_swOut[2][3] = 5;
		m_swOut[2][4] = 8;
		m_swOut[2][5] = 9;
	}

	for(_swIndex = 0; _swIndex < 3; _swIndex++)
	{
		for(i=0;i<6;i++)
		{
			if( (_swIndex == 0) &&  (i==5 ) )
					continue;
			else
				_swPort = m_swOut[_swIndex][i];

			RepairLinkPartner(_swIndex,_swPort);
		}
	}
	sleep(3);
}

