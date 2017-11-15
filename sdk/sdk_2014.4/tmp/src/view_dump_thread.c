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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <termios.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>

#include "hw_base.h"
#include "VDMA.h"
#include "axi_gpio.h"
#include "vasa_udp_client.h"
#include "mem_op.h"
#include "errno.h"
#include "histogram.h"

#include "jpeg-9/jpeglib.h"
#include "jpeg-9/ProcessImage.h"

//#include <convert.h>
//#include "ResizeImage.h"
//#include <cv.h>
//#include <highgui.h>

extern int errno;
struct termios orig_termios;

void reset_terminal_mode()
{
    tcsetattr(0, TCSANOW, &orig_termios);
}

void set_conio_terminal_mode()
{
    struct termios new_termios;

    /* take two copies - one for now, one for later */
    tcgetattr(0, &orig_termios);
    memcpy(&new_termios, &orig_termios, sizeof(new_termios));

    /* register cleanup handler, and set the new terminal mode */
    atexit(reset_terminal_mode);
    cfmakeraw(&new_termios);
    tcsetattr(0, TCSANOW, &new_termios);
}

int kbhit()
{
    struct timeval tv = { 0L, 0L };
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv);
}

int getch()
{
    int r;
    unsigned char c;
    if ((r = read(0, &c, sizeof(c))) < 0) {
        return r;
    } else {
        return c;
    }
}

typedef struct
{
	uint32_t w;
	uint32_t h;
	uint8_t bpp; //bytes per pixel
} image_size_t;

typedef struct
{
	uint16_t seq_no;
	uint32_t fb_addr0;
	uint32_t fb_addr1;
	image_size_t image_size;
} image_save_t;

typedef struct
{
	uint32_t fb_addr;
	image_size_t image_size;
} image_send_t;

pthread_mutex_t saving_image_mutex;
bool saving_image;


int GetClockMs ()
{
    struct timespec ts;

    if (clock_gettime (CLOCK_MONOTONIC, &ts) == 0)
        return (uint64_t) ((ts.tv_sec * 1000000 + ts.tv_nsec / 1000000)%1000); //mod by 1000 to return ms
    else //Clock that retrieves the time, that cannot be set and represents monotonic time since an unspecified starting point
        return 0;
}

int LengthHelper(int x) { //determines the amount of numbers in the int, ex. 1 ->1, 12 ->2, 122 ->3
    if(x>=100) return 3;
    if(x>=10) return 2;
    if(x>=0) return 1;
    if(x<0) return -1;
return 0;
}

void *SaveImageThreadFcn(void *arg)
{
	image_save_t *im_save = arg;
	char filename0[20], filename1[20];
	//char time_str[8];
	char time_str[9];
	time_t t;
	//clock_t ms;
char add_ms[4];

	pthread_mutex_lock(&saving_image_mutex);
	saving_image = true;
	pthread_mutex_unlock(&saving_image_mutex);

//	memset(filename0, '\0', sizeof(filename0));
//	memset(filename1, '\0', sizeof(filename1));
//	time(&t);
//	strncpy(time_str, ctime(&t)+11, 8);
//	time_str[2] = '_';
//	time_str[5] = '_';
//	strncpy(filename0,time_str,8);   //copy no more than 8 char from time_str to filename0
//	strcat(filename0,"_0.bmp");      // append _0.bmp onto filename0
//	SaveImage(im_save->fb_addr0, filename0, im_save->image_size.w, im_save->image_size.h, im_save->image_size.bpp, 1); //_0 is left cam from behind
//	//printf("First image saved!!!");
//	strncpy(filename1,time_str,8);
//	strcat(filename1,"_1.bmp");
//	SaveImage(im_save->fb_addr1, filename1, im_save->image_size.w, im_save->image_size.h, im_save->image_size.bpp, 1); //_1 is right cam from behind

	memset(filename0, '\0', sizeof(filename0));
	memset(filename1, '\0', sizeof(filename1));
	time(&t);
	//ms = (clock() / (CLOCKS_PER_SEC / 1000)) % 1000;

	int res = GetClockMs ();
	//add_ms = res;
	sprintf(add_ms, "%d", abs(res)); //add int res to add_ms as char
		printf("milliseconds: %s\n",add_ms);
	//printf("milliseconds: %d\n",ms);
	strncpy(time_str, ctime(&t)+11, 9);
	time_str[2] = '_';
	time_str[5] = '_';
	time_str[8] = '_';
	int nr_el = LengthHelper(abs(res)); //dont forget to add something about if x == 0!
	printf("ELEMENTS: %d\n", nr_el);

	strncpy(filename0,time_str,9);   //copy no more than 8 char from time_str to filename0
	strncpy(filename0+9, add_ms, nr_el*sizeof(char));
	strcat(filename0,"_0.jpg");      // append _0.bmp onto filename0, THEY ARE SAVED AS BMP BUT WITH .JPG EXTENSION
	SaveImage(im_save->fb_addr0, filename0, im_save->image_size.w, im_save->image_size.h, im_save->image_size.bpp, 1); //_0 is left cam from behind

	strncpy(filename1,time_str,9);
	strncpy(filename1+9, add_ms, nr_el*sizeof(char));
	strcat(filename1,"_1.jpg");
	SaveImage(im_save->fb_addr1, filename1, im_save->image_size.w, im_save->image_size.h, im_save->image_size.bpp, 1); //_1 is right cam from behind


//	//int ret = system("cd /media/card && ./ResizeConvert filename0"); //files are saved as jpeg but the name is still .bmp
//	int ret = execl("/media/card/ResizeConvert", filename0,(char*) NULL);
//	printf("return value from system(): %d\n", ret);
//
//	  if (ret < 0){
//		  puts("system() failed");
//		  int errnum = errno; //assign the error nr
//		  fprintf(stderr, "value of errno: %d\n", errno);
//		  perror("the reason of errno");
//		  fprintf(stderr, "error binding: %s\n", strerror(errnum));
//		  //printf("bind returns: %d\n",bind_result);
//		  return 1;
//	  }
//ResizeImage(&filename0,&filename0);
	//ResizeImgFunc(filename0);

//ConvertImage(filename0);
//ConvertImage(filename1);

InitTcpServer(filename0, filename1); //send the stereo pairs to the odroid

remove(filename0);
remove(filename1);


//	sprintf(filename0, "%d_0.bmp",im_save->seq_no);
//	SaveImage(im_save->fb_addr0, filename0, im_save->image_size.w, im_save->image_size.h, im_save->image_size.bpp, 1);
//	sprintf(filename1, "%d_1.bmp",im_save->seq_no);
//	SaveImage(im_save->fb_addr1, filename1, im_save->image_size.w, im_save->image_size.h, im_save->image_size.bpp, 1);

	printf("Saving to FB: %x %x\n",im_save->fb_addr0,im_save->fb_addr1);

//	printf("Saving to FB: testing\n");

	pthread_mutex_lock(&saving_image_mutex);
	saving_image = false;
	pthread_mutex_unlock(&saving_image_mutex);

	pthread_exit(NULL);
}

void *SendImageThreadFcn(void *arg)
{
	image_send_t *im_send = arg;
	SendImage(im_send->fb_addr, im_send->image_size.w, im_send->image_size.h, im_send->image_size.bpp);//sends the udp
	pthread_exit(NULL);
}


int main(int argc, char *argv[])
{
	uint32_t pin_data = 0;
	uint32_t pin_dir = 0x0;

	//const image_size_t im_size = {.w = 3840, .h = 2748, .bpp = 3};
	const image_size_t im_size = {.w = 1920, .h = 1080, .bpp = 3}; //if changing here, dont forget to change in the client too!!!
	//const image_size_t im_size = {.w = 768, .h = 432, .bpp = 3};
	//const image_size_t im_size = {.w = 640, .h = 480, .bpp = 3};
	image_save_t im_save;
	im_save.image_size = im_size;
	im_save.seq_no = 0;
	image_send_t im_send;
//	im_send.image_size.w = im_size.w/8;
//	im_send.image_size.h = im_size.h/8;
	im_send.image_size.w = im_size.w/4;///////////
	im_send.image_size.h = im_size.h/4;///////////
	im_send.image_size.bpp = im_size.bpp;

	uint32_t frame_ptr = 0x0; //first fb
	uint32_t frame_ptr_out = 0x0;
//	uint8_t cam = 2;
	uint8_t stream = 0; //turn on or off udp stream, 1 = on, 0 = off STREAM

	uint8_t quit = 0;
	uint8_t update_integration = 0;
	uint8_t dump_image = 0; // 0;
	uint8_t auto_expose = 1;

	integration_time_limits_t itl;
	integration_times_t it;

//	char filename[15];
//	char *filename_ptr = filename;
//	uint16_t seq_no = 0;

	const uint16_t GLOBAL_GAIN_VALUES[] = {0x1040, 0x1840, 0x1c40, 0x1cc0, 0x1dc0, 0x1dff}; //1,2,4,8,16,31.75
	const uint8_t GLOBAL_GAIN_INDEX_MIN = 0;
	const uint8_t GLOBAL_GAIN_INDEX_MAX = 5;
	uint8_t global_gain_index = GLOBAL_GAIN_INDEX_MIN;
	uint8_t update_gain = 0;

	float harris_corner_th = 10;
	float harris_edge_th = 10;


	pthread_t SaveImageThread, SendImageThread;
	bool saving_image_tmp;

	uint32_t hist[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
//	float hist_norm[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	float hist_mean;
	uint8_t frame_cnt = 0;
	uint32_t histogram_ctrl = HISTOGRAM_CTRL_DEFAULT;



//	if (argc>1)
//		switch(atoi(argv[1])) {
//		case 0:
//			cam = 0;
//			printf("Cam0\n");
//			break;
//		case 1:
//			cam = 1;
//			printf("Cam1\n");
//			break;
//		case 2:
//			cam = 2;
//			printf("Stereo\n");
//			break;
//		}

	set_harris_th(false, harris_edge_th);
	set_harris_th(true, harris_corner_th);


//	SetSize(w,h,bpp);
	init_broadcast_address(VASA_FV_PORT);  //floods network with udp (init i think)
	if (init_vasa_socket_udp () == -1) return -1;

	axi_gpio_init_dir();
	axi_gpio_init_val(&pin_data); //cam i2c high, resets low

	cam_reset(&pin_data);
	init_cams_sync(&pin_data,&pin_dir, SIOD_0, SIOC_0, SIOD_1, SIOC_1);
	fpga_clk_reset(&pin_data);
	usleep(100);
//	getchar();

	get_integration_time_limits(&itl,&pin_data,&pin_dir,SIOD_0,SIOC_0);
	print_integration_time_limits(&itl);
	get_integration_time(&it,&pin_data,&pin_dir,SIOD_0,SIOC_0);
	print_integration_time(&it);

	ResetVDMA(VDMA0_BASEADDR);
	ResetVDMA(VDMA1_BASEADDR);
	ResetVDMA(VDMA2_BASEADDR);
	SetupVDMAs2mm(VDMA0_BASEADDR,im_size.w,im_size.h,im_size.bpp,FB0_ADDR,FB1_ADDR,FB0_ADDR); //only two FP in use
	SetupVDMAs2mm(VDMA1_BASEADDR,im_size.w,im_size.h,im_size.bpp,FB2_ADDR,FB3_ADDR,FB2_ADDR);
	SetupVDMAs2mm(VDMA2_BASEADDR,im_send.image_size.w,im_send.image_size.h,im_send.image_size.bpp,FB4_ADDR,FB5_ADDR,FB4_ADDR);

	histogram_ctrl_set(histogram_ctrl);

	fpga_pipe_reset(&pin_data); //release fgpa soft reset
	sleep(1);


	printf("ENTERING MAIN LOOP, please enter command\n");
	while (!quit) {


		if (kbhit())
			switch (getch()) { /* consume the character */
			case 'q':
				quit = 1;
				break;
			case 'w':
				dump_image = (dump_image+1)%2;
				break;
			case 't':
				toggle_cam_stream(&pin_data);
				break;
			case 'a':
				it.coarse += 1000;
				update_integration = 1;
				break;
			case 's':
				it.coarse += 100;
				update_integration = 1;
				break;
			case 'd':
				it.coarse += 10;
				update_integration = 1;
				break;
			case 'f':
				it.coarse += 1;
				update_integration = 1;
				break;
			case 'z':
				it.coarse -= 1000;
				update_integration = 1;
				break;
			case 'x':
				it.coarse -= 100;
				update_integration = 1;
				break;
			case 'c':
				it.coarse -= 10;
				update_integration = 1;
				break;
			case 'v':
				it.coarse -= 1;
				update_integration = 1;
				break;
			case 'e':
				auto_expose = (auto_expose+1)%2;
				break;
			case 'g':
				if (global_gain_index < GLOBAL_GAIN_INDEX_MAX) {
					global_gain_index++;
					update_gain = 1;
					printf("Gain index: %d\n",global_gain_index);
				}
				else
					printf("WARNING: Gain is at MAX\n");
				break;
			case 'b':
				if (global_gain_index > GLOBAL_GAIN_INDEX_MIN) {
					global_gain_index--;
					update_gain = 1;
					printf("Gain index: %d\n",global_gain_index);
				}
				else
					printf("WARNING: Gain is at MIN\n");
				break;
			case 'h':
				set_pin_flip(&pin_data,5);
				break;
			case 'r':
				set_pin_flip(&pin_data,4);
				break;
			case 'j':
				harris_corner_th*=1.25;
				set_harris_th(true, harris_corner_th);
				printf("Harris corner th: %f\n",harris_corner_th);
				break;
			case 'm':
				harris_corner_th*=0.8;
				set_harris_th(true, harris_corner_th);
				printf("Harris corner th: %f\n",harris_corner_th);
				break;
			case 'k':
				harris_edge_th*=1.25;
				set_harris_th(false, harris_edge_th);
				printf("Harris edge th: %f\n",harris_edge_th);
				break;
			case ',':
				harris_edge_th*=0.8;
				set_harris_th(false, harris_edge_th);
				printf("Harris edge th: %f\n",harris_edge_th);
				break;
			}

		if (update_integration) {
			if (it.coarse > itl.coarse_max) {
				printf("WARNING: Coarse integration time > MAX\n");
				//it.coarse = itl.coarse_max; //uncomment for LONG exposure
			}
			if (it.coarse < itl.coarse_min) {
				printf("WARNING: Coarse integration time < MIN\n");
				//it.coarse = itl.coarse_min; ////////////////////////////////
			}
			set_integration_time_dual(&it,&pin_data,&pin_dir,SIOD_0,SIOC_0,SIOD_1,SIOC_1);
			get_integration_time(&it,&pin_data,&pin_dir,SIOD_0,SIOC_0);
			print_integration_time(&it);
			update_integration = 0;
		}

		if (update_gain) {
			printf("Updating gain\n");
			update_gain = 0;
			set_gain_register(GLOBAL_GAIN_VALUES[global_gain_index], &pin_data, &pin_dir, SIOD_0,SIOC_0,SIOD_1,SIOC_1);
		}





		if (dump_image) {
			if ((frame_ptr & 0x3F) == 0 || (frame_ptr & 0x3F) == 3)
				im_save.fb_addr0 = FB1_ADDR;
			else
				im_save.fb_addr0 = FB0_ADDR;

			if (((frame_ptr>>6) & 0x3F) == 0 || ((frame_ptr>>6) & 0x3F) == 3)
				im_save.fb_addr1 = FB3_ADDR;
			else
				im_save.fb_addr1 = FB2_ADDR;

			pthread_mutex_lock(&saving_image_mutex);
			saving_image_tmp = saving_image;
			pthread_mutex_unlock(&saving_image_mutex);
			if (!saving_image_tmp) {
				//dump_image = 0; //comment out to take pictures continuously
				im_save.seq_no++;
				//im_save.seq_no += 2; //testing this, remove if it doesnt work

				pthread_create(&SaveImageThread, NULL, SaveImageThreadFcn, &im_save);
				pthread_detach(SaveImageThread);

			}

		}

		if (stream == 1) {
			if (((frame_ptr>>12) & 0x3F) == 0 || ((frame_ptr>>12) & 0x3F) == 3)
				im_send.fb_addr = FB5_ADDR;
			else
				im_send.fb_addr = FB4_ADDR;

			pthread_create(&SendImageThread, NULL, SendImageThreadFcn, &im_send);
			pthread_join(SendImageThread, NULL);
		}

		pthread_mutex_lock(&saving_image_mutex);
		if (saving_image)
			toggle_fp(&frame_ptr,false,false,true);
		else
			toggle_fp(&frame_ptr,true,true,true);
		pthread_mutex_unlock(&saving_image_mutex);
//		toggle_fp(&frame_ptr);

		while (1) { //need to ensure that VDMAs (or actually the last one (stream)) are writing to the assigned FB, poll until OK
			write_fp(&frame_ptr);
			read_fp(&frame_ptr_out);
//			printf("frame_ptr_sw: %d %d %d %x\n",(frame_ptr>>12) & 0x3F,(frame_ptr>>6) & 0x3F,frame_ptr & 0x3F, frame_ptr);
//			printf("frame_ptr_hw: %d %d %d %x\n",(frame_ptr_out>>12) & 0x3F,(frame_ptr_out>>6) & 0x3F,frame_ptr_out & 0x3F, frame_ptr_out);
//			printf("\n");
			if (compare_fp(frame_ptr,frame_ptr_out)) break;

			sleep(0.05);

//			break;
		}

		frame_cnt++;
		if (frame_cnt==5)
		{
			frame_cnt = 0;
			if (auto_expose)
			{
				if (histogram_get(hist) == -1)
					printf("Failed to read histogram\n");
				else
				{
					hist_mean = histogram_stats(hist);
//					printf("Histogram mean: %f\n",hist_mean);
					hist_mean = histogram_exp(hist_mean);
//					printf("Histogram exp: %f\n",hist_mean);
					if (hist_mean != 1.0)
					{
						it.coarse =  (uint16_t)((float)it.coarse*hist_mean);
						update_integration = 1;
					}
				}
			}
		}
	}


	StopVDMA(VDMA0_BASEADDR);
	StopVDMA(VDMA1_BASEADDR);
	StopVDMA(VDMA2_BASEADDR);

	pthread_exit(NULL);
    return 0;
}
