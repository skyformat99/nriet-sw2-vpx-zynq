#ifndef _GPIO_H_
#define _GPIO_H_

//base addr
#define RioUartBase 0x83c10000
#define SWBACK_BOARD_MAX_COUNT 500
#define GPIO_BASE_ADDR 0x82000000
#define GPIO_RESET_ADDR 0x84200000


void SetGpioReg(unsigned int addrBase,unsigned int addrOffset,unsigned int value);
int GetGpioReg(unsigned int addrBase,unsigned int addrOffset);

#endif /* UART_H_ */
