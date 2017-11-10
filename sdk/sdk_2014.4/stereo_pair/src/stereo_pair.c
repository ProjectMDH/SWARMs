/*
 * Copyright (c) 2012 Xilinx, Inc.  All rights reserved.
 *
 * Xilinx, Inc.
 * XILINX IS PROVIDING THIS DESIGN, CODE, OR INFORMATION "AS IS" AS A
 * COURTESY TO YOU.  BY PROVIDING THIS DESIGN, CODE, OR INFORMATION AS
 * ONE POSSIBLE   IMPLEMENTATION OF THIS FEATURE, APPLICATION OR
 * STANDARD, XILINX IS MAKING NO REPRESENTATION THAT THIS IMPLEMENTATION
 * IS FREE FROM ANY CLAIMS OF INFRINGEMENT, AND YOU ARE RESPONSIBLE
 * FOR OBTAINING ANY RIGHTS YOU MAY REQUIRE FOR YOUR IMPLEMENTATION.
 * XILINX EXPRESSLY DISCLAIMS ANY WARRANTY WHATSOEVER WITH RESPECT TO
 * THE ADEQUACY OF THE IMPLEMENTATION, INCLUDING BUT NOT LIMITED TO
 * ANY WARRANTIES OR REPRESENTATIONS THAT THIS IMPLEMENTATION IS FREE
 * FROM CLAIMS OF INFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/fb.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <stdbool.h>
#include "hw_base.h"
#include "VDMA.h"
#include "axi_gpio.h"
#include "vasa_udp_client.h"
#include <time.h>
#include "mem_op.h"
#include "exposure_reg.h"
//#include <cv.h>
//#include <highgui.h>

int kbhit()
{
    // timeout structure passed into select
    struct timeval tv;
    // fd_set passed into select
    fd_set fds;
    // Set up the timeout.  here we can wait for 1 second
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    // Zero out the fd_set - make sure it's pristine
    FD_ZERO(&fds);
    // Set the FD that we want to read
    FD_SET(STDIN_FILENO, &fds); //STDIN_FILENO is 0
    // select takes the last file descriptor value + 1 in the fdset to check,
    // the fdset for reads, writes, and errors.  We are only passing in reads.
    // the last parameter is the timeout.  select will return if an FD is ready or
    // the timeout has occurred
    select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
    // return 0 if STDIN is not ready to be read.
    return FD_ISSET(STDIN_FILENO, &fds);
}

int main(int argc, char *argv[])
{
	uint32_t pin_data = 0;
	uint32_t pin_dir = 0x0;
	uint32_t w = 320;
	uint32_t h = 240;
	uint8_t bpp = 4;
	uint32_t port = VASA_FV_PORT;
	bool loop = true;
	uint32_t cnt = 1;
//	uint32_t cnt2 = 1;
	uint32_t cnt3 = 1;

	uint32_t vdma_addr0 = VDMA0_BASEADDR;
	uint32_t vdma_addr1 = VDMA2_BASEADDR;

//	time_t t;
	char filename [30];
//	char time_str[8];

	uint8_t exp_th = 127; //60; //127;
	uint32_t exp_pos;
	uint32_t exp_neg;
	expreg_set_th(exp_th);

	integration_time_limits_t itl;
	integration_times_t it;

	int size;

	image_t im0;
	im0.h = h;
	im0.w = w;
	im0.bpp = 4;
	if (im_alloc(&im0) < 0) return -1;

	image_t im1;
	im1.h = h;
	im1.w = w;
	im1.bpp = 4;
	if (im_alloc(&im1) < 0) return -1;

	bool debug = false;
	int i;


	for (i=0; i<argc; i++)
		if (strcmp(argv[i],"debug")==0)
			debug = true;


	if (debug) {
		init_vasa_address(port);
		if (init_vasa_socket_udp () == -1) return -1;
	}

	axi_gpio_init_dir();
	axi_gpio_init_val(&pin_data); //cam i2c high, resets low

	SetSize (w,h,bpp);

	InitFB(FB0_ADDR); //write DEADBEEFs
	InitFB(FB1_ADDR);

	ResetVDMA(vdma_addr0);
	SetupVDMAs2mm(vdma_addr0,FB0_ADDR,FB0_ADDR,FB0_ADDR);
	ResetVDMA(vdma_addr1);
	SetupVDMAs2mm(vdma_addr1,FB1_ADDR,FB1_ADDR,FB1_ADDR);


	cam_reset(&pin_data);
	init_cams_sync(&pin_data,&pin_dir, SIOD_0, SIOC_0, SIOD_1, SIOC_1);
	fpga_clk_reset(&pin_data);

	usleep(100);

	fpga_pipe_reset(&pin_data); //release fgpa soft reset

	get_integration_time_limits(&itl,&pin_data,&pin_dir, SIOD_0, SIOC_0);

	usleep(200000);



	do {
		// save the image
//		time(&t);
		cnt3++;
//		printf("Copy images from FB\n");
		mem_copy(im0.data, FB0_ADDR, w*h*bpp);
		mem_copy(im1.data, FB1_ADDR, w*h*bpp);

		if (cnt3 % 10 == 0)
		{
			expreg_read(&exp_pos, &exp_neg);
			printf("pos %d, neg %d\n",exp_pos,exp_neg);
			get_integration_time(&it,&pin_data,&pin_dir, SIOD_0, SIOC_0);
			print_integration_time(&it);
			calc_integration(exp_pos,exp_neg,&it,&itl);
			print_integration_time(&it);
			print_integration_time_limits(&itl);
			printf("\n");
			set_integration_time(&it,&pin_data,&pin_dir, SIOD_0, SIOC_0, SIOD_1, SIOC_1);
		}

		if (cnt3 % 30 == 0)
		{
			printf("Saving first image to SD\n");
	//		strncpy(time_str, ctime(&t)+11, 8);
			sprintf(filename, "0_%d.bmp",cnt);
			printf("%s\n",filename);
			size = SaveImage_mem(&im0,filename);
			printf("Saving second image to SD %d\n",size);
			sprintf(filename, "1_%d.bmp",cnt);
			SaveImage_mem(&im1,filename);
			cnt++;
			cnt3 = 0;
		}

		if (cnt3 % 5 == 0 && debug)
			SendImage_mem(im1.data,0);

		if (kbhit()) loop = false;
		sleep(0.2);
	} while (loop);



	im_free(&im0);
	im_free(&im1);

	StopVDMA(vdma_addr0);
	StopVDMA(vdma_addr1);
    return 0;
}
