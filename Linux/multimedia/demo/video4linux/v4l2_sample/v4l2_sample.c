/*
 * Copyright 2004-2012 Freescale Semiconductor, Inc. All rights reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*
 * @file v4l2_sample.c
 *
 * @brief V4L2 application based on FSL Mxc Video For Linux 2 driver unit test
 *
 */

#ifdef __cplusplus
extern "C"{
#endif

/*=======================================================================
                                        INCLUDE FILES
=======================================================================*/
/* Standard Include Files */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

/* Verification Test Environment Include Files */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <asm/types.h>
#include <linux/videodev2.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <math.h>
#include <string.h>
#include <malloc.h>
#include <signal.h>

#define TFAIL -1
#define TPASS 0

#define ipu_fourcc(a,b,c,d)\
        (((__u32)(a)<<0)|((__u32)(b)<<8)|((__u32)(c)<<16)|((__u32)(d)<<24))

#define IPU_PIX_FMT_RGB332  ipu_fourcc('R','G','B','1') /*!<  8  RGB-3-3-2     */
#define IPU_PIX_FMT_RGB555  ipu_fourcc('R','G','B','O') /*!< 16  RGB-5-5-5     */
#define IPU_PIX_FMT_RGB565  ipu_fourcc('R','G','B','P') /*!< 16  RGB-5-6-5     */
#define IPU_PIX_FMT_RGB666  ipu_fourcc('R','G','B','6') /*!< 18  RGB-6-6-6     */
#define IPU_PIX_FMT_BGR24   ipu_fourcc('B','G','R','3') /*!< 24  BGR-8-8-8     */
#define IPU_PIX_FMT_RGB24   ipu_fourcc('R','G','B','3') /*!< 24  RGB-8-8-8     */
#define IPU_PIX_FMT_BGR32   ipu_fourcc('B','G','R','4') /*!< 32  BGR-8-8-8-8   */
#define IPU_PIX_FMT_BGRA32  ipu_fourcc('B','G','R','A') /*!< 32  BGR-8-8-8-8   */
#define IPU_PIX_FMT_RGB32   ipu_fourcc('R','G','B','4') /*!< 32  RGB-8-8-8-8   */
#define IPU_PIX_FMT_RGBA32  ipu_fourcc('R','G','B','A') /*!< 32  RGB-8-8-8-8   */
#define IPU_PIX_FMT_ABGR32  ipu_fourcc('A','B','G','R') /*!< 32  ABGR-8-8-8-8  */


char v4l_device[100] = "/dev/video0";
int fd_v4l = 0;
int g_sensor_width = 720;
int g_sensor_height = 248;
int g_sensor_top = 0;
int g_sensor_left = 0;
int g_display_width = 480;
int g_display_height = 272;
int g_display_top = 0;
int g_display_left = 0;
int g_rotate = 0;
int g_timeout = 0;
int g_display_lcd = 0;
int g_overlay = 0;
int g_camera_color = 0;
int g_camera_framerate = 30;
int g_capture_mode = 0;
int ctrl_c_rev = 0;
int g_fd_fb_fg = 0;

void ctrl_c_handler(int signum, siginfo_t *info, void *myact)
{
	ctrl_c_rev = 1;
	return;
}

int
mxc_v4l_overlay_test(int timeout)
{
	int i;
	int overlay = 1;
	int retval = 0;
	struct v4l2_control ctl;
	char fb_device_0[100] = "/dev/fb0";
	int fd_graphic_fb = 0;
	struct fb_var_screeninfo fb0_var;

	if (ioctl(fd_v4l, VIDIOC_OVERLAY, &overlay) < 0) {
		printf("VIDIOC_OVERLAY start failed\n");
		retval = TFAIL;
		goto out1;
	}

	if (g_camera_color == 1) {
			ctl.id = V4L2_CID_BRIGHTNESS;
			for (i = 0; i < 0xff; i+=0x20) {
				ctl.value = i;
				printf("change the brightness %d\n", i);
				ioctl(fd_v4l, VIDIOC_S_CTRL, &ctl);
				sleep(1);
			}
	}
	else if (g_camera_color == 2) {
			ctl.id = V4L2_CID_SATURATION;
			for (i = 25; i < 150; i+= 25) {
				ctl.value = i;
				printf("change the color saturation %d\n", i);
				ioctl(fd_v4l, VIDIOC_S_CTRL, &ctl);
				sleep(5);
			}
	}
	else if (g_camera_color == 3) {
			ctl.id = V4L2_CID_RED_BALANCE;
			for (i = 0; i < 0xff; i+=0x20) {
				ctl.value = i;
				printf("change the red balance %d\n", i);
				ioctl(fd_v4l, VIDIOC_S_CTRL, &ctl);
				sleep(1);
			}
	}
	else if (g_camera_color == 4) {
			ctl.id = V4L2_CID_BLUE_BALANCE;
			for (i = 0; i < 0xff; i+=0x20) {
				ctl.value = i;
				printf("change the blue balance %d\n", i);
				ioctl(fd_v4l, VIDIOC_S_CTRL, &ctl);
				sleep(1);
			}
	}
	else if (g_camera_color == 5) {
			ctl.id = V4L2_CID_BLACK_LEVEL;
			for (i = 0; i < 4; i++) {
				ctl.value = i;
				printf("change the black balance %d\n", i);
				ioctl(fd_v4l, VIDIOC_S_CTRL, &ctl);
				sleep(5);
			}
	}
	else {

			if (timeout > 0) {
				sleep(timeout);
			} else {
				getchar();
			}
	}

out2:
	overlay = 0;
	if (ioctl(fd_v4l, VIDIOC_OVERLAY, &overlay) < 0)
	{
		printf("VIDIOC_OVERLAY stop failed\n");
		retval = TFAIL;
		goto out1;
	}
out1:
	close(fd_graphic_fb);
	return retval;
}

int mxc_v4l_overlay_setup(struct v4l2_format *fmt)
{
	struct v4l2_streamparm parm;
	v4l2_std_id id;
	struct v4l2_control ctl;
	struct v4l2_crop crop;
	struct v4l2_frmsizeenum fsize;
	struct v4l2_fmtdesc ffmt;

	fsize.index = g_capture_mode;
	if (ioctl(fd_v4l, VIDIOC_ENUM_FRAMESIZES, &fsize) < 0)
	{
	        printf("VIDIOC_ENUM_FRAMESIZES failed\n");
	        return TFAIL;
	}

	printf("sensor frame size is %dx%d\n", fsize.discrete.width,
					       fsize.discrete.height);

	ffmt.index = g_capture_mode;
	if (ioctl(fd_v4l, VIDIOC_ENUM_FMT, &ffmt) < 0)
	{
	        printf("VIDIOC_ENUM_FMT failed\n");
	        return TFAIL;
	}
	printf("ffmt.pixelformat = 0x%04x\n",ffmt.pixelformat);


	if (ffmt.pixelformat == V4L2_PIX_FMT_YUYV)
		printf("sensor frame format is YUYV\n");
	else if (ffmt.pixelformat == V4L2_PIX_FMT_UYVY)
		printf("sensor frame format is UYVY\n");
	else if (ffmt.pixelformat == V4L2_PIX_FMT_YUV420)
		printf("sensor frame format is YUV420\n");

	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	parm.parm.capture.timeperframe.numerator = 1;
	parm.parm.capture.timeperframe.denominator = g_camera_framerate;
	parm.parm.capture.capturemode = g_capture_mode;

	if (ioctl(fd_v4l, VIDIOC_S_PARM, &parm) < 0)
	{
		printf("VIDIOC_S_PARM failed\n");
		return TFAIL;
	}

	parm.parm.capture.timeperframe.numerator = 0;
	parm.parm.capture.timeperframe.denominator = 0;

	if (ioctl(fd_v4l, VIDIOC_G_PARM, &parm) < 0)
	{
		printf("get frame rate failed\n");
		return TFAIL;
	}

	printf("frame_rate is %d\n",parm.parm.capture.timeperframe.denominator);

	ctl.id = V4L2_CID_PRIVATE_BASE + 2;
	ctl.value = g_rotate;
	if (ioctl(fd_v4l, VIDIOC_S_CTRL, &ctl) < 0)
	{
		printf("set control failed\n");
		return TFAIL;
	}

	crop.type = V4L2_BUF_TYPE_VIDEO_OVERLAY;
	crop.c.left = g_sensor_left;
	crop.c.top = g_sensor_top;
	crop.c.width = g_sensor_width;
	crop.c.height = g_sensor_height;
	if (ioctl(fd_v4l, VIDIOC_S_CROP, &crop) < 0)
	{
			printf("set cropping failed\n");
			return TFAIL;
	}

	if (ioctl(fd_v4l, VIDIOC_S_FMT, fmt) < 0)
	{
			printf("set format failed\n");
			return TFAIL;
	}

	if (ioctl(fd_v4l, VIDIOC_G_FMT, fmt) < 0)
	{
			printf("get format failed\n");
			return TFAIL;
	}

	if (ioctl(fd_v4l, VIDIOC_G_STD, &id) < 0)
	{
			printf("VIDIOC_G_STD failed\n");
			return TFAIL;
	}

	return TPASS;
}

int process_cmdline(int argc, char **argv)
{
	int i;

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-iw") == 0) {
				g_sensor_width = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-ih") == 0) {
				g_sensor_height = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-it") == 0) {
				g_sensor_top = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-il") == 0) {
				g_sensor_left = atoi(argv[++i]);
		}
		if (strcmp(argv[i], "-ow") == 0) {
				g_display_width = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-oh") == 0) {
				g_display_height = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-ot") == 0) {
				g_display_top = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-ol") == 0) {
				g_display_left = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-r") == 0) {
				g_rotate = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-t") == 0) {
				g_timeout = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-d") == 0) {
				g_display_lcd = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-fg") == 0) {
				g_overlay = 1;
		}
		else if (strcmp(argv[i], "-v") == 0) {
				g_camera_color = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-fr") == 0) {
				g_camera_framerate = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-m") == 0) {
			g_capture_mode = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-help") == 0) {
			printf("MXC Video4Linux overlay Device Test\n\n" \
				   " -iw <input width>\n -ih <input height>\n" \
				   " -it <input top>\n -il <input left>\n"	\
				   " -ow <display width>\n -oh <display height>\n" \
				   " -ot <display top>\n -ol <display left>\n"	\
				   " -r <rotate mode>\n -t <timeout>\n" \
				   " -d <output display> \n"	\
				   " -v <camera color> 1-brightness 2-saturation"
				   " 3-red 4-blue 5-black balance\n"\
				   " -m <capture mode> 0-low resolution 1-high resolution\n" \
				   " -a <alpha mode> 0-global alpha blending 1-local alpha blending\n" \
				   " -fr <frame rate> 30fps by default\n" \
				   " -fg foreground mode when -fg specified,"
				   " otherwise go to frame buffer\n");
			return -1;
		}
	}

	printf("g_display_width = %d, g_display_height = %d\n", g_display_width, g_display_height);
	printf("g_display_top = %d, g_display_left = %d\n", g_display_top, g_display_left);

	if ((g_display_width == 0) || (g_display_height == 0)) {
		return -1;
	}
	return 0;
}

int main(int argc, char **argv)
{
	struct v4l2_format fmt;
	struct v4l2_framebuffer fb_v4l2;
	char fb_device_0[100] = "/dev/fb0";
	char *fb_device_fg;
	int fd_fb_0 = 0;
	struct fb_fix_screeninfo fb0_fix, fb_fg_fix;
	struct fb_var_screeninfo fb0_var, fb_fg_var;

	int ret = 0;
	struct sigaction act;
	struct v4l2_dbg_chip_ident chip;

	/* for ctrl-c */
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;
	act.sa_sigaction = ctrl_c_handler;

	if((ret = sigaction(SIGINT, &act, NULL)) < 0) {
		printf("install sigal error\n");
		return TFAIL;
	}

	if (process_cmdline(argc, argv) < 0) {
			return TFAIL;
	}

	if ((fd_v4l = open(v4l_device, O_RDWR, 0)) < 0) {
			printf("Unable to open %s\n", v4l_device);
			return TFAIL;
	}

	if (ioctl(fd_v4l, VIDIOC_DBG_G_CHIP_IDENT, &chip))
	{
                printf("VIDIOC_DBG_G_CHIP_IDENT failed.\n");
                return TFAIL;
	}
	printf("sensor chip is %s\n", chip.match.name);

	if ((fd_fb_0 = open(fb_device_0, O_RDWR )) < 0)	{
		printf("Unable to open frame buffer 0\n");
                return TFAIL;
	}

	if (ioctl(fd_fb_0, FBIOGET_VSCREENINFO, &fb0_var) < 0) {
			close(fd_fb_0);
			return TFAIL;
	}
	if (ioctl(fd_fb_0, FBIOGET_FSCREENINFO, &fb0_fix) < 0) {
			close(fd_fb_0);
			return TFAIL;
	}

	if (strcmp(fb0_fix.id, "DISP3 BG - DI1") == 0)
		g_display_lcd = 1;
	else if (strcmp(fb0_fix.id, "DISP3 BG") == 0)
		g_display_lcd = 0;
	printf("g_display_ldc = %d\n",g_display_lcd);
	if (ioctl(fd_v4l, VIDIOC_S_OUTPUT, &g_display_lcd) < 0)
	{
			printf("VIDIOC_S_OUTPUT failed\n");
			return TFAIL;
	}


	fmt.type = V4L2_BUF_TYPE_VIDEO_OVERLAY;
	fmt.fmt.win.w.top=  g_display_top ;
	fmt.fmt.win.w.left= g_display_left;
	fmt.fmt.win.w.width=g_display_width;
	fmt.fmt.win.w.height=g_display_height;

	if (mxc_v4l_overlay_setup(&fmt) < 0) {
			printf("Setup overlay failed.\n");
			return TFAIL;
	}

	memset(&fb_v4l2, 0, sizeof(fb_v4l2));

	if (!g_overlay) {
		fb_v4l2.fmt.width = fb0_var.xres;
		fb_v4l2.fmt.height = fb0_var.yres;
	
		if (fb0_var.bits_per_pixel == 32) {
				fb_v4l2.fmt.pixelformat = IPU_PIX_FMT_BGR32;
				fb_v4l2.fmt.bytesperline = 4 * fb_v4l2.fmt.width;
		}
		else if (fb0_var.bits_per_pixel == 24) {
				fb_v4l2.fmt.pixelformat = IPU_PIX_FMT_BGR24;
				fb_v4l2.fmt.bytesperline = 3 * fb_v4l2.fmt.width;
		}
		else if (fb0_var.bits_per_pixel == 16) {
				fb_v4l2.fmt.pixelformat = IPU_PIX_FMT_RGB565;
				fb_v4l2.fmt.bytesperline = 2 * fb_v4l2.fmt.width;
		}

		fb_v4l2.flags = V4L2_FBUF_FLAG_PRIMARY;
		fb_v4l2.base = (void *) fb0_fix.smem_start +
		fb0_fix.line_length*fb0_var.yoffset;
		printf("fb0_fix.smem_start=%08x\n",fb0_fix.smem_start);
		printf("fb0_fix.line_length=%d\n",fb0_fix.line_length);
		printf("fb0_var.yoffset=%d\n",fb0_var.yoffset);
		printf("fb_v4l2.base = %08x\n",fb_v4l2.base);

	} else {
		if (ioctl(fd_v4l, VIDIOC_G_FBUF, &fb_v4l2) < 0) {
			printf("Get framebuffer failed\n");
			close(fd_fb_0);
			close(g_fd_fb_fg);
			return TFAIL;
		}
		fb_v4l2.flags = V4L2_FBUF_FLAG_OVERLAY;
    }

	close(fd_fb_0);

	if (ioctl(fd_v4l, VIDIOC_S_FBUF, &fb_v4l2) < 0)
	{
			printf("set framebuffer failed\n");
			return TFAIL;
	}

	if (ioctl(fd_v4l, VIDIOC_G_FBUF, &fb_v4l2) < 0) {
			printf("set framebuffer failed\n");
			return TFAIL;
	}

	printf("\n frame buffer width %d, height %d, bytesperline %d\n",
			fb_v4l2.fmt.width, fb_v4l2.fmt.height, fb_v4l2.fmt.bytesperline);
	ret = mxc_v4l_overlay_test(g_timeout);
	
	ioctl(fd_v4l, VIDIOC_OVERLAY, 0);
	close(fd_v4l);

	return ret;
}

