
#define XPAR_AXI_GPIO_I2C_MC_DEVICE_ID 2
#define XPAR_AXI_BRAM_CTRL_I2C_S_AXI_BASEADDR 0x84000000

typedef unsigned int u32;

extern void GpioReset();
extern void xil_setreg32(unsigned int addrBase,unsigned int addrOffset,unsigned int value);
extern int xil_getreg32(unsigned int addrBase,unsigned int addrOffset);
extern void GpioReset();
extern void iic_write();
