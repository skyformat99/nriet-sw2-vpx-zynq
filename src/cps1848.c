/*
 * Copyright (c) 2016 CGT Co., Ltd.
 *
 * Authors: Robin Lee <lixiangbin@china-dsp.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program;
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <memory.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <sys/socket.h>

#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>

#include <termios.h>
#include <netinet/in.h>

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * global vars
 */
#define FP_DEV_1 "/sys/class/i2c-dev/i2c-0/device/0-0002/eeprom"
#define FP_DEV_2 "/sys/class/i2c-dev/i2c-0/device/0-0004/eeprom"
#define FP_DEV_3 "/sys/class/i2c-dev/i2c-0/device/0-0008/eeprom"

static int global_fd_1 = -1;
static int global_fd_2 = -1;
static int global_fd_3 = -1;

//operations of cps1848
int fd_initial(int num)
{

	int fd = -1;

	/*
		Open modem device for reading and writing and not as controlling tty
		because we don't want to get killed if linenoise sends CTRL-C.
	*/
	if(num==0)
	{
		printf("Open '%s' .. ", FP_DEV_1);
		fd = open(FP_DEV_1, O_RDWR );
	}
	else if(num==1)
	{
		printf("Open '%s' .. ", FP_DEV_2);
		fd = open(FP_DEV_2, O_RDWR );
	}
	else if(num==2)
	{
		printf("Open '%s' .. ", FP_DEV_3);
		fd = open(FP_DEV_3, O_RDWR );
	}
	else
	{
		printf("Open error.\n");
		return -1;
	}

	if (fd <0) {
		printf("failed (err = %d)\n", fd);
		return -1;
	}

	printf("Done\n");

	return fd;
}

int fd_exit(int fd,int num)
{
	if(num==1)
	{
		printf("Close '%s' .. ", FP_DEV_1);
	}
	else if(num==2)
	{
		printf("Close '%s' .. ", FP_DEV_2);
	}
	else
	{
		printf("Close '%s' .. ", FP_DEV_3);
	}
	close(fd);

	printf("Done\n");

	return 0;
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * get_1848_reg, set_1848_reg
 */
unsigned int get_1848_reg(int num,unsigned int offset)
{
	unsigned int value = 0xffff;
	unsigned int ans=0;
	int fd=-1;
	if(num==0)
	{
		fd=global_fd_1;
	}
	else if(num==1)
	{
		fd=global_fd_2;
	}
	else if( num==2 )
	{
		fd=global_fd_3;
	}
	else
	{
		printf("get_1848_reg::Invalid param !\n");
		return 0;
	}

	if (fd<0) {
		printf("Invalid device handle !\n");
		return value;
	}

	if( lseek(fd, offset, SEEK_SET) == (off_t) -1 ) {
		printf("failed for seek to offset 0x%x !\n", offset);
		return value;
	}

	if ( read(fd, &value, sizeof(value)) != sizeof(value)) {
		printf("failed for read from offset 0x%x !\n", offset);
		return value;
	}
	ans=htonl(value);
	return ans;
}

void set_1848_reg(int num,unsigned int offset, unsigned int data)
{
	unsigned int value = htonl(data);
	int fd=-1;
	if(num==0)
	{
		fd=global_fd_1;
	}
	else if(num==1)
	{
		fd=global_fd_2;
	}
	else if( num==2 )
	{
		fd=global_fd_3;
	}
	else
	{
		printf("set_1848_reg::Invalid param !\n");
		return;
	}
	if (fd<0) {
		printf("Invalid device handle !\n");
		return ;
	}

	if( lseek(fd, offset, SEEK_SET) == (off_t) -1 ) {
		printf("failed for seek to offset 0x%x !\n", offset);
		return ;
	}

	if ( write(fd, &value, sizeof(value)) != sizeof(value)) {
		printf("failed for write from offset 0x%x !\n", offset);
		return ;
	}

	return ;
}

unsigned int regtoul(const char *str)
{
	int cbase = 10;
	if(str[0]=='0'&&(str[1]=='x'||str[1]=='X')) {
		cbase = 16;
	}
	return strtoul(str, NULL, cbase);
}

int Init_1848()
{
		global_fd_1 = fd_initial(0);
		global_fd_2 = fd_initial(1);
		global_fd_3 = fd_initial(2);
		if (global_fd_1<0) {
			printf("\n Invalid First cps1848 device ! \n");
			return -1;
		}
		if (global_fd_2<0) {
			printf("\n Invalid Second cps1848 device ! \n");
			return -1;
		}
		if (global_fd_3<0) {
			printf("\n Invalid Third cps1848 device ! \n");
			return -1;
		}

		return 0;
}


