################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/cps1848.c \
../src/flash.c \
../src/iic_slave.c \
../src/iic_srio1848.c \
../src/main.c \
../src/uartApp.c \
../src/uartMenu.c \
../src/udpSocketLib.c \
../src/udp_fun.c \
../src/udp_net.c \
../src/xil_io.c 

OBJS += \
./src/cps1848.o \
./src/flash.o \
./src/iic_slave.o \
./src/iic_srio1848.o \
./src/main.o \
./src/uartApp.o \
./src/uartMenu.o \
./src/udpSocketLib.o \
./src/udp_fun.o \
./src/udp_net.o \
./src/xil_io.o 

C_DEPS += \
./src/cps1848.d \
./src/flash.d \
./src/iic_slave.d \
./src/iic_srio1848.d \
./src/main.d \
./src/uartApp.d \
./src/uartMenu.d \
./src/udpSocketLib.d \
./src/udp_fun.d \
./src/udp_net.d \
./src/xil_io.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Linux gcc compiler'
	arm-xilinx-linux-gnueabi-gcc -Wall -O0 -g3 -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


