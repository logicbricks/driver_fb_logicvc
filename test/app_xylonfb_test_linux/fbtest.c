/*
 * Xylon logiCVC IP core frame buffer driver test application
 * Designed for use with Xylon FB 3.0
 *
 * Copyright (C) 2014 Xylon d.o.o.
 * Author: Davor Joja <davor.joja@logicbricks.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "xylonfb.h"

#define LOGICVC_BASE_LAYER_CTRL_REG_ADDR 0x138

#ifndef FB_VISUAL_FOURCC
#define FB_VISUAL_FOURCC 6 /* Visual identified by a V4L2 FOURCC */
#endif
#define LOGICVC_PIX_FMT_AYUV  v4l2_fourcc('A', 'Y', 'U', 'V')
#define LOGICVC_PIX_FMT_AVUY  v4l2_fourcc('A', 'V', 'U', 'Y')

#define XILINX_TEST_IMAGE_X_OFFSET 0
#define XILINX_TEST_IMAGE_Y_OFFSET (0)
#define XYLON_TEST_IMAGE_X_OFFSET 0
#define XYLON_TEST_IMAGE_Y_OFFSET (200 + XILINX_TEST_IMAGE_Y_OFFSET)

struct fb_resolution
{
	int xres;
	int yres;
};

struct cmap_data
{
	struct fb_cmap cmap;
	unsigned short cmap_transp[3];
	unsigned short cmap_red[3];
	unsigned short cmap_green[3];
	unsigned short cmap_blue[3];
	unsigned short cmap_size;
	unsigned char cmap_stat;
};

struct fb_data
{
	struct fb_fix_screeninfo finfo;
	struct fb_var_screeninfo vinfo;
	struct xylonfb_layer_buffer layer_buffer;
	struct xylonfb_layer_color layer_color;
	struct xylonfb_layer_geometry layer_geometry;
	struct xylonfb_layer_transparency layer_transp;
	struct xylonfb_hw_access hw_access;
	struct cmap_data *cmap_data;
	unsigned char *fbp;
	unsigned long ioctl_arg;
	unsigned long ioctl_tmp;
	int fbfd;
	int arg;
	int screensize;
	short xres;
	short yres;
	unsigned char bpp;
	unsigned char expanded;
	unsigned char fb_phys_id;
	unsigned char fbid;
	bool ioctl_flag;
	unsigned int ip_ver;
};


static void app_usage_1(void)
{
	puts("Usage 1");
	puts("For listing available framebuffer devices type ");
	puts("\"fbtest -list\"");
	puts("Then choose framebuffer device.");
	puts("");
}

static void app_usage_2(void)
{
	puts("Usage 2");
	puts("For using specific framebuffer device type ");
	puts("\"fbtest -bpp N (N = 8,16,32)\"");
	puts("");
	puts("For using specific framebuffer device resloution type ");
	puts("\"fbtest -r HRESxVRES\"");
	puts("");
}

static void app_usage_general(void)
{
	puts("General");
//	puts("For using framebuffer device with extended pixels ");
//	puts("(8 bpp -> 16 bpp or 16 bpp -> 32 bpp), add \"-e\"");
//	puts("");
	puts("For starting framebuffer device resolution change test type ");
	puts("\"fbtest -res\"");
	puts("");
	puts("To get and display EDID (if available) type ");
	puts("\"fbtest -edid tout (tout - timeout period in seconds)\"");
	puts("");
	puts("To turn off console while testing in progress type ");
	puts("\"fbtest -conoff\"");
	puts("");
}

static int find_fb_device(struct fb_fix_screeninfo *finfo,
			  struct fb_var_screeninfo *vinfo,
			  int bpp, int expanded, unsigned char *fbid)
{
	unsigned int color_type;
	int fbfd;
	char fb_dev[10], fb_id;

	printf("Searching for Xylon FB device (%d bpp) ...\n", bpp);
	fb_id = 0;
	color_type = 0;
	while(1)
	{
		sprintf(fb_dev, "/dev/fb%d", fb_id);
		printf("Opening device %s\n", fb_dev);
		/* Open the file for reading and writing */
		fbfd = open(fb_dev, O_RDWR);
		if (fbfd < 0)
		{
			printf("Error opening framebuffer device %s\n", fb_dev);
			perror(NULL);
			return -errno;
		}

		/* Get fixed screen information */
		if (ioctl(fbfd, FBIOGET_FSCREENINFO, finfo))
		{
			perror("Error reading fixed information");
			continue;
		}
		/* Get variable screen information */
		if (ioctl(fbfd, FBIOGET_VSCREENINFO, vinfo))
		{
			perror("Error reading variable information");
			continue;
		}

		printf("FB driver name: %s\n", finfo->id);
		printf("FB driver color type: ");
		switch (finfo->visual)
		{
			case FB_VISUAL_MONO01:
				puts("Monochrome Black(0)/White(1)");
				goto ffd_fb_not_supported;
			case FB_VISUAL_MONO10:
				puts("Monochrome Black(1)/White(0)");
				goto ffd_fb_not_supported;
			case FB_VISUAL_TRUECOLOR:
				puts("True color");
				break;
			case FB_VISUAL_PSEUDOCOLOR:
				puts("Pseudo color (ATARI)");
				break;
			case FB_VISUAL_DIRECTCOLOR:
				puts("Direct color");
				goto ffd_fb_not_supported;
			case FB_VISUAL_STATIC_PSEUDOCOLOR:
				puts("Pseudo color (readonly)");
				goto ffd_fb_not_supported;
			case FB_VISUAL_FOURCC:
				puts("Four CC color");
				color_type = vinfo->grayscale;
				break;
			default:
				goto ffd_fb_not_supported;
		}
		printf("FB driver color format:");
		if (color_type == 0)
			printf(" ARGB : ");
		else if (color_type > 0)
			printf(" %c%c%c%c : ",
				(color_type & 0x000000FF),
				((color_type & 0x0000FF00) >> 8),
				((color_type & 0x00FF0000) >> 16),
				((color_type & 0xFF000000) >> 24));
		printf("%d%d%d%d\n",
			vinfo->transp.length,
			vinfo->red.length,
			vinfo->green.length,
			vinfo->blue.length);
		if (!strncmp("Xylon FB", finfo->id, 8))
		{
			/*
			logiCVC supports pixel formats where:
				- 8 bpp can be expanded to 16 bpp
				- 16 bpp can be expanded to 32 bpp
			Check alpha offset to find plain 8 bpp or 16 bpp.
			*/
			if (vinfo->bits_per_pixel == bpp)
			{
				if (expanded &&
				    (finfo->visual == FB_VISUAL_TRUECOLOR ||
				    vinfo->transp.length != 0))
				{
					printf("Xylon FB device with expanded %d bpp" \
						"found: %s\n", bpp, fb_dev);
					break;
				}
				else if (!expanded &&
					(finfo->visual == FB_VISUAL_PSEUDOCOLOR ||
					vinfo->transp.length == 0))
				{
					printf("Xylon FB device with %d bpp found: %s\n",
						bpp, fb_dev);
					break;
				}
			}
		}

		close(fbfd);
		fb_id++;
	}

	return fbfd;

ffd_fb_not_supported:
	puts("NOT SUPPORTED!");
	errno = EPERM;
	return -errno;
}

static int list_fb_devices(struct fb_fix_screeninfo *finfo,
			   struct fb_var_screeninfo *vinfo,
			   unsigned char *fbid)
{
	unsigned int color_type;
	int fbfd, fb_id;
	char fb_dev[10];

	puts("Detected Xylon FB devices:");

	fb_id = 0;
	color_type = 0;

	while(1)
	{
		sprintf(fb_dev, "/dev/fb%d", fb_id);
		/* Open the file for reading and writing */
		fbfd = open(fb_dev, O_RDWR);
		if (fbfd < 0)
		{
			break;
		}

		/* Get fixed screen information */
		if (ioctl(fbfd, FBIOGET_FSCREENINFO, finfo))
		{
			perror("Error reading fixed information");
			continue;
		}
		/* Get variable screen information */
		if (ioctl(fbfd, FBIOGET_VSCREENINFO, vinfo))
		{
			perror("Error reading variable information");
			continue;
		}

		printf("FB driver ID %d\n", fb_id);
		printf("FB driver name: %s\n", finfo->id);
		printf("FB driver color type: ");
		switch (finfo->visual)
		{
			case FB_VISUAL_MONO01:
				puts("Monochrome Black(0)/White(1)");
				goto lfb_fb_not_supported;
			case FB_VISUAL_MONO10:
				puts("Monochrome Black(1)/White(0)");
				goto lfb_fb_not_supported;
			case FB_VISUAL_TRUECOLOR:
				puts("True color");
				break;
			case FB_VISUAL_PSEUDOCOLOR:
				puts("Pseudo color (ATARI)");
				break;
			case FB_VISUAL_DIRECTCOLOR:
				puts("Direct color");
				goto lfb_fb_not_supported;
			case FB_VISUAL_STATIC_PSEUDOCOLOR:
				puts("Pseudo color (readonly)");
				goto lfb_fb_not_supported;
			case FB_VISUAL_FOURCC:
				puts("Four CC color");
				color_type = vinfo->grayscale;
				break;
			default:
				puts("Not supported! Exit application.");
				break;
		}
		printf("FB driver color format:");
		if (color_type == 0)
			printf(" ARGB : ");
		else if (color_type > 0)
			printf(" %c%c%c%c : ",
				(color_type & 0x000000FF),
				((color_type & 0x0000FF00) >> 8),
				((color_type & 0x00FF0000) >> 16),
				((color_type & 0xFF000000) >> 24));
		printf("%d%d%d%d\n",
			vinfo->transp.length,
			vinfo->red.length,
			vinfo->green.length,
			vinfo->blue.length);
lfb_fb_not_supported:
		printf("FB driver bits per pixel %d\n", vinfo->bits_per_pixel);

		close(fbfd);
		fb_id++;
	}

	puts("Choose Xylon FB device:");
	scanf("%d", &fb_id);
	sprintf(fb_dev, "/dev/fb%d", fb_id);
	fbfd = open(fb_dev, O_RDWR);
	if (fbfd < 0)
	{
		perror("Error opening Xylon FB device");
		return -errno;
	}
	else
	{
		*fbid = (unsigned char)fb_id;
		puts("Xylon FB device opened successfully.");
	}

	if (ioctl(fbfd, FBIOGET_FSCREENINFO, finfo))
	{
		perror("Error reading fixed information");
		return -errno;
	}
	if (ioctl(fbfd, FBIOGET_VSCREENINFO, vinfo))
	{
		perror("Error reading variable information");
		return -errno;
	}

	return fbfd;
}

static void rgb_to_yuv(unsigned int rgb1, unsigned int rgb2, unsigned int *yuv,
		       int bpp, unsigned char swap_color)
{
	static unsigned int YKr, YKg, YKb;
	static unsigned int CrKr, CrKg, CrKb;
	static unsigned int CbKr, CbKg, CbKb;
	unsigned int A, R, G, B;
	unsigned int Y, Cb, Cr;
	unsigned int yuv_pix;

	YKr  = 29900;
	YKg  = 58700;
	YKb  = 11400;
	CrKr = 49980;
	CrKg = 41850;
	CrKb = 8128;
	CbKr = 16868;
	CbKg = 33107;
	CbKb = 49970;

	A = (rgb1 & 0xFF000000) >> 24;
	R = (rgb1 & 0x00FF0000) >> 16;
	G = (rgb1 & 0x0000FF00) >> 8;
	B = (rgb1 & 0x000000FF) >> 0;

	Y = ((YKr * (R & 0xFF)) + (YKg * (G & 0xFF)) + (YKb * (B & 0xFF)))
		/ 100000;
	Cb = ((-CbKr * (R & 0xFF)) - (CbKg * (G & 0xFF)) + (CbKb * (B & 0xFF))
		+ 12800000) / 100000;
	Cr = ((CrKr * (R & 0xFF)) - (CrKg * (G & 0xFF)) - (CrKb * (B & 0xFF))
		+ 12800000) / 100000;

	if (swap_color)
		yuv_pix = (A << 24) | (Cr << 16) | (Cb << 8) | Y;
	else
		yuv_pix = (A << 24) | (Y << 16) | (Cb << 8) | Cr;

	if (bpp == 16)
	{
		R = (rgb2 & 0x00FF0000) >> 16;
		G = (rgb2 & 0x0000FF00) >> 8;
		B = (rgb2 & 0x000000FF) >> 0;
		Y = ((YKr * (R & 0xFF)) + (YKg * (G & 0xFF)) + (YKb * (B & 0xFF)))
			/ 100000;
		if (swap_color)
		{
			yuv_pix &= 0xFF;
			yuv_pix <<= 8;
			yuv_pix |= ((Y << 24) | (Cr << 16) | Cb);
		}
		else
		{
			yuv_pix >>= 16;
			yuv_pix &= 0xFF;
			yuv_pix |= ((Cr << 24) | (Y << 16) | (Cb << 8));
		}
	}
	*yuv = yuv_pix;
}

static int set_pixel_color_format(struct fb_fix_screeninfo *finfo,
				  struct fb_var_screeninfo *vinfo,
				  struct fb_cmap *cmap, void *pixel,
				  unsigned int color)
{
	int i, bpp;
	int A_off, R_off, G_off, B_off;
	int A_len, R_len, G_len, B_len;
	unsigned int tmp;
	unsigned int *pix32;
	unsigned short *pix16;
	unsigned char *pix8;

	bpp = vinfo->bits_per_pixel;

	A_off = vinfo->transp.offset;
	A_len = vinfo->transp.length;
	R_off = vinfo->red.offset;
	R_len = vinfo->red.length;
	G_off = vinfo->green.offset;
	G_len = vinfo->green.length;
	B_off = vinfo->blue.offset;
	B_len = vinfo->blue.length;

	switch (bpp)
	{
		case 8:
			if ((finfo->visual == FB_VISUAL_PSEUDOCOLOR) ||
				(finfo->visual == FB_VISUAL_FOURCC))
			{
				for (i = 0; i < cmap->len; i++)
				{
					tmp = ((cmap->transp[i] & 0xFF) << 24);
					tmp |= ((cmap->red[i] & 0xFF) << 16);
					tmp |= ((cmap->green[i] & 0xFF) << 8);
					tmp |= (cmap->blue[i] & 0xFF);
					if (tmp == color)
						break;
					/* try with ignored transparency */
					if ((tmp << 8) == (color << 8))
						break;
				}
				*(unsigned char *)pixel = i;
			}
			else if (finfo->visual == FB_VISUAL_TRUECOLOR)
			{
				pix8 = (unsigned char *)pixel;
				*pix8 = 0;
				if (A_len)
				{
					tmp = color >> 24;
					tmp >>= (8 - A_len);
					tmp <<= A_off;
					*pix8 |= tmp;
				}
				tmp = (color >> 16) & 0xFF;
				tmp >>= (8 - R_len);
				tmp <<= R_off;
				*pix8 |= tmp;
				tmp = (color >> 8) & 0xFF;
				tmp >>= (8 - G_len);
				tmp <<= G_off;
				*pix8 |= tmp;
				tmp = color & 0xFF;
				tmp >>= (8 - B_len);
				tmp <<= B_off;
				*pix8 |= tmp;
			}
			else
			{
				puts("8bpp format not supported!");
				goto spcf_fb_not_supported;
			}
			break;
		case 16:
			if (finfo->visual == FB_VISUAL_TRUECOLOR)
			{
				pix16 = (unsigned short *)pixel;
				*pix16 = 0;
				if (A_len)
				{
					tmp = color >> 24;
					tmp >>= (8 - A_len);
					tmp <<= A_off;
					*pix16 |= tmp;
				}
				tmp = (color >> 16) & 0xFF;
				tmp >>= (8 - R_len);
				tmp <<= R_off;
				*pix16 |= tmp;
				tmp = (color >> 8) & 0xFF;
				tmp >>= (8 - G_len);
				tmp <<= G_off;
				*pix16 |= tmp;
				tmp = color & 0xFF;
				tmp >>= (8 - B_len);
				tmp <<= B_off;
				*pix16 |= tmp;
			}
			else if (finfo->visual == FB_VISUAL_FOURCC)
			{
				rgb_to_yuv(color, color, (unsigned int *)pixel, bpp,
					vinfo->grayscale == V4L2_PIX_FMT_VYUY ? 0 : 1);
			}
			else
			{
				puts("16bpp format not supported!");
				goto spcf_fb_not_supported;
			}
			break;
		case 32:
			if (finfo->visual == FB_VISUAL_TRUECOLOR)
			{
				pix32 = (unsigned int *)pixel;
				*pix32 = 0;
				if (R_len == 8)
				{
					*pix32 = color;
				}
				else
				{
					if (A_len)
					{
						tmp = color >> 24;
						tmp >>= (8 - A_len);
						tmp <<= A_off;
						*pix32 |= tmp;
					}
					tmp = (color >> 16) & 0xFF;
					tmp >>= (8 - R_len);
					tmp <<= R_off;
					*pix32 |= tmp;
					tmp = (color >> 8) & 0xFF;
					tmp >>= (8 - G_len);
					tmp <<= G_off;
					*pix32 |= tmp;
					tmp = color & 0xFF;
					tmp >>= (8 - B_len);
					tmp <<= B_off;
					*pix32 |= tmp;
				}
			}
			else if (finfo->visual == FB_VISUAL_FOURCC)
			{
				rgb_to_yuv(color, 0, (unsigned int *)pixel, bpp,
					vinfo->grayscale == LOGICVC_PIX_FMT_AYUV ? 0 : 1);
			}
			else
			{
				puts("32bpp format not supported!");
				goto spcf_fb_not_supported;
			}
			break;
	}

	return 0;

spcf_fb_not_supported:
	return -1;
}

static int change_cmap(struct fb_fix_screeninfo *finfo,
		       struct fb_var_screeninfo *vinfo,
		       struct cmap_data *cmap_data, int fbfd)
{
	int i;

	if ((finfo->visual == FB_VISUAL_PSEUDOCOLOR) ||
	    ((finfo->visual == FB_VISUAL_FOURCC) &&
	    (vinfo->grayscale == LOGICVC_PIX_FMT_AYUV)))
	{
		cmap_data->cmap_stat = 1;
		cmap_data->cmap_size = 256;
	}
	else if ((finfo->visual == FB_VISUAL_TRUECOLOR) ||
		(finfo->visual == FB_VISUAL_FOURCC))
	{
		cmap_data->cmap_stat = 2;
		cmap_data->cmap_size = 16;
	}
	else
	{
		errno = EPERM;
		return -errno;
	}

	cmap_data->cmap.start = 0;
	cmap_data->cmap.len = cmap_data->cmap_size;
	cmap_data->cmap.transp = (unsigned short *)calloc(cmap_data->cmap_size,
							sizeof(unsigned short));
	cmap_data->cmap.red = (unsigned short *)calloc(cmap_data->cmap_size,
						       sizeof(unsigned short));
	cmap_data->cmap.green = (unsigned short *)calloc(cmap_data->cmap_size,
							sizeof(unsigned short));
	cmap_data->cmap.blue = (unsigned short *)calloc(cmap_data->cmap_size,
							sizeof(unsigned short));
	if (ioctl(fbfd, FBIOGETCMAP, &cmap_data->cmap))
	{
		perror("Error reading framebuffer cmap");
		cmap_data->cmap_stat = 0;
	}
	/* force opaque alpha */
	for (i = 0; i < cmap_data->cmap.len; i++)
	{
		if (cmap_data->cmap.transp[i] != 0xFF)
		{
			cmap_data->cmap.transp[i] = 0xFF;
		}
	}

//	for (i = 0; i < cmap_data->cmap.len; i++)
//		printf("ARGB 0x%X	0x%X	0x%X	0x%X\n",
//			cmap_data->cmap.transp[i],
//			cmap_data->cmap.red[i],
//			cmap_data->cmap.green[i],
//			cmap_data->cmap.blue[i]);

	if (cmap_data->cmap_stat == 1)
	{
		/* read and store original cmap values */
		for (i = 0; i < 3; i++)
		{
			cmap_data->cmap_transp[i] =
				cmap_data->cmap.transp[cmap_data->cmap_size-i-1];
			cmap_data->cmap_red[i] =
				cmap_data->cmap.red[cmap_data->cmap_size-i-1];
			cmap_data->cmap_green[i] =
				cmap_data->cmap.green[cmap_data->cmap_size-i-1];
			cmap_data->cmap_blue[i] =
				cmap_data->cmap.blue[cmap_data->cmap_size-i-1];
		}

		cmap_data->cmap.transp[cmap_data->cmap_size-1] = 0xFFFF;
		cmap_data->cmap.red[cmap_data->cmap_size-1] = 0xFFFF;
		cmap_data->cmap.green[cmap_data->cmap_size-1] = 0x0000;
		cmap_data->cmap.blue[cmap_data->cmap_size-1] = 0x0000;
		cmap_data->cmap.transp[cmap_data->cmap_size-2] = 0xFFFF;
		cmap_data->cmap.red[cmap_data->cmap_size-2] = 0x0000;
		cmap_data->cmap.green[cmap_data->cmap_size-2] = 0xFFFF;
		cmap_data->cmap.blue[cmap_data->cmap_size-2] = 0x0000;
		cmap_data->cmap.transp[cmap_data->cmap_size-3] = 0xFFFF;
		cmap_data->cmap.red[cmap_data->cmap_size-3] = 0x0000;
		cmap_data->cmap.green[cmap_data->cmap_size-3] = 0x0000;
		cmap_data->cmap.blue[cmap_data->cmap_size-3] = 0xFFFF;

		if (ioctl(fbfd, FBIOPUTCMAP, &cmap_data->cmap))
		{
			perror("Error writting framebuffer cmap");
			cmap_data->cmap_stat = 0;
		}
	}

	return 0;
}

static void restore_cmap(struct cmap_data *cmap_data, int fbfd)
{
	int i;

	if (cmap_data->cmap_stat == 1)
	{
		/* restore original cmap values */
		for (i = 0; i < 3; i++)
		{
			cmap_data->cmap.transp[cmap_data->cmap_size-i-1] =
				cmap_data->cmap_transp[i];
			cmap_data->cmap.red[cmap_data->cmap_size-i-1] =
				cmap_data->cmap_red[i];
			cmap_data->cmap.green[cmap_data->cmap_size-i-1] =
				cmap_data->cmap_green[i];
			cmap_data->cmap.blue[cmap_data->cmap_size-i-1] =
				cmap_data->cmap_blue[i];
		}
		if (ioctl(fbfd, FBIOPUTCMAP, &cmap_data->cmap))
		{
			perror("Error writting framebuffer cmap");
		}
	}

	free(cmap_data->cmap.transp);
	free(cmap_data->cmap.red);
	free(cmap_data->cmap.green);
	free(cmap_data->cmap.blue);
}

static void draw_rectangles(struct fb_fix_screeninfo *finfo,
			    struct fb_var_screeninfo *vinfo,
			    struct cmap_data *cmap_data,
			    int fbfd, unsigned char *fbp,
			    unsigned int start_color)
{
	int pix_offset, bpp, bytes_pp, bytes_pl;
	int rctg, x, y, y_end;
	unsigned int pixel, color;

	bpp = vinfo->bits_per_pixel;
	bytes_pp = bpp / 8;
	bytes_pl = finfo->line_length;

	for (rctg = 0, y = 0, y_end = vinfo->yres/3;
		rctg < 3;
		rctg++, y = y_end, y_end += vinfo->yres/3)
	{
		color = (start_color >> (rctg * 8)) | 0xFF000000;
		if (set_pixel_color_format(finfo, vinfo, &cmap_data->cmap,
			(void *)&pixel, color))
			break;

		printf("Rectangle %d Color 0x%X - ", rctg, color);
		switch (bpp)
		{
			case 8:
				printf("CLUT Index value 0x%X\n",
					(unsigned char)pixel);
				break;
			case 16:
				if (finfo->visual == FB_VISUAL_TRUECOLOR)
					printf("RGB Pixel value 0x%X\n",
						(unsigned short)pixel);
				else if (finfo->visual == FB_VISUAL_FOURCC)
					printf("YUV Pixel value 0x%X\n", pixel);
				break;
			case 32:
				if (finfo->visual == FB_VISUAL_TRUECOLOR)
					printf("Pixel value 0x%X\n", pixel);
				else if (finfo->visual == FB_VISUAL_FOURCC)
					printf("YUV Pixel value 0x%X\n", pixel);
				break;
		}

		for (; y < y_end; y++)
		{
			switch (bpp)
			{
				case 8:
					for (x = (vinfo->xres/2); x < vinfo->xres; x++)
					{
						pix_offset = (x * bytes_pp) + (y * bytes_pl);
						*((unsigned char *)(fbp + pix_offset)) = pixel;
					}
					break;
				case 16:
					if (finfo->visual == FB_VISUAL_TRUECOLOR)
					{
						for (x = (vinfo->xres/2); x < vinfo->xres; x++)
						{
							pix_offset = (x * bytes_pp) + (y * bytes_pl);
							*((unsigned short *)(fbp + pix_offset)) = pixel;
						}
					}
					else if (finfo->visual == FB_VISUAL_FOURCC)
					{
						for (x = (vinfo->xres/2); x < vinfo->xres; x += 2)
						{
							pix_offset = (x * bytes_pp) + (y * bytes_pl);
							*((unsigned int *)(fbp + pix_offset)) = pixel;
						}
					}
					break;
				case 32:
					for (x = (vinfo->xres/2); x < vinfo->xres; x++)
					{
						pix_offset = (x * bytes_pp) + (y * bytes_pl);
						*((unsigned int *)(fbp + pix_offset)) = pixel;
					}
					break;
			}
		}
	}
}

static int draw_logos(struct fb_fix_screeninfo *finfo,
		      struct fb_var_screeninfo *vinfo,
		      unsigned char *fbp)
{
	FILE *fp;
	unsigned int argb_pixel, argb_pixel_next;
	int pix_offset, bpp, bytes_pp, bytes_pl;
	int x, y;
	int A_off, R_off, G_off, B_off;
	int A_len, R_len, G_len, B_len;
	unsigned int tmp;
	unsigned int pix32;
	unsigned short pix16;
	unsigned char pix8;
	unsigned char alpha, red, green, blue;

	A_off = vinfo->transp.offset;
	A_len = vinfo->transp.length;
	R_off = vinfo->red.offset;
	R_len = vinfo->red.length;
	G_off = vinfo->green.offset;
	G_len = vinfo->green.length;
	B_off = vinfo->blue.offset;
	B_len = vinfo->blue.length;

	bpp = vinfo->bits_per_pixel;
	bytes_pp = bpp / 8;
	bytes_pl = finfo->line_length;

	/* Draw Full HD test image on visible area */
	fp = fopen("CVC_test_picture.ppm", "rb");
	if (fp != NULL)
	{
		fseek(fp, 17, SEEK_SET);
		for (y = 0; y < 1080; y++)
		{
			for (x = 0; x < 1920; x++)
			{
				fread(&argb_pixel, 3, 1, fp);
				alpha = 0xFF;
				red = argb_pixel;
				green = argb_pixel >> 8;
				blue = argb_pixel >> 16;
				argb_pixel = (alpha << 24) | (red << 16) | (green << 8) | (blue);
				pix_offset = (x * bytes_pp) + (y * bytes_pl);
				if (bpp == 8)
				{
					pix8 = 0;
					if (A_len)
					{
						tmp = argb_pixel >> 24;
						tmp >>= (8 - A_len);
						tmp <<= A_off;
						pix8 |= tmp;
					}
					tmp = (argb_pixel >> 16) & 0xFF;
					tmp >>= (8 - R_len);
					tmp <<= R_off;
					pix8 |= tmp;
					tmp = (argb_pixel >> 8) & 0xFF;
					tmp >>= (8 - G_len);
					tmp <<= G_off;
					pix8 |= tmp;
					tmp = argb_pixel & 0xFF;
					tmp >>= (8 - B_len);
					tmp <<= B_off;
					pix8 |= tmp;
					*((unsigned char *)(fbp + pix_offset)) = pix8;
				}
				else if (bpp == 16)
				{
					if (finfo->visual == FB_VISUAL_TRUECOLOR)
					{
						pix16 = 0;
						if (A_len)
						{
							tmp = argb_pixel >> 24;
							tmp >>= (8 - A_len);
							tmp <<= A_off;
							pix16 |= tmp;
						}
						tmp = (argb_pixel >> 16) & 0xFF;
						tmp >>= (8 - R_len);
						tmp <<= R_off;
						pix16 |= tmp;
						tmp = (argb_pixel >> 8) & 0xFF;
						tmp >>= (8 - G_len);
						tmp <<= G_off;
						pix16 |= tmp;
						tmp = argb_pixel & 0xFF;
						tmp >>= (8 - B_len);
						tmp <<= B_off;
						pix16 |= tmp;
						*((unsigned short *)(fbp + pix_offset)) = pix16;
					}
					else if (finfo->visual == FB_VISUAL_FOURCC)
					{
						rgb_to_yuv(argb_pixel, argb_pixel,
							(unsigned int *)(fbp + pix_offset), bpp,
							vinfo->grayscale == V4L2_PIX_FMT_VYUY ? 0 : 1);
					}
				}
				else if (bpp == 32)
				{
					if (finfo->visual == FB_VISUAL_TRUECOLOR)
					{
						pix32 = 0;
						if (R_len == 8)
						{
							pix32 = argb_pixel;
						}
						else
						{
							if (A_len)
							{
								tmp = argb_pixel >> 24;
								tmp >>= (8 - A_len);
								tmp <<= A_off;
								pix32 |= tmp;
							}
							tmp = (argb_pixel >> 16) & 0xFF;
							tmp >>= (8 - R_len);
							tmp <<= R_off;
							pix32 |= tmp;
							tmp = (argb_pixel >> 8) & 0xFF;
							tmp >>= (8 - G_len);
							tmp <<= G_off;
							pix32 |= tmp;
							tmp = argb_pixel & 0xFF;
							tmp >>= (8 - B_len);
							tmp <<= B_off;
							pix32 |= tmp;
						}
						*((unsigned int *)(fbp + pix_offset)) = pix32;
					}
					else if (finfo->visual == FB_VISUAL_FOURCC)
					{
						rgb_to_yuv(argb_pixel, 0,
							(unsigned int *)(fbp + pix_offset), bpp,
							vinfo->grayscale == LOGICVC_PIX_FMT_AYUV ? 0 : 1);
					}
				}
			}
		}
		fclose(fp);
	}
	else
	{
		puts("Unable to open full HD test image!");
	}

	/* Draw Xilinx logo at non-visible area */
	fp = fopen("xilinx-inc-logo.ppm", "rb");
	if (fp == NULL)
	{
		puts("Unable to open Xilinx logo!");
		return -errno;
	}
	fseek(fp, 15, SEEK_SET);
	for (y = vinfo->yres+XILINX_TEST_IMAGE_Y_OFFSET;
		y < (vinfo->yres+XILINX_TEST_IMAGE_Y_OFFSET+155); y++)
	{
		for (x = 0; x < 524; x++)
		{
			fread(&argb_pixel, 3, 1, fp);
			alpha = 0xFF;
			red = argb_pixel;
			green = argb_pixel >> 8;
			blue = argb_pixel >> 16;
			argb_pixel = (alpha << 24) | (red << 16) | (green << 8) | (blue);
			if ((finfo->visual == FB_VISUAL_FOURCC) && (bpp == 16))
			{
				fread(&argb_pixel_next, 3, 1, fp);
				alpha = 0xFF;
				red = argb_pixel_next;
				green = argb_pixel_next >> 8;
				blue = argb_pixel_next >> 16;
				argb_pixel_next =
					(alpha << 24) | (red << 16) | (green << 8) | (blue);
			}
			pix_offset = (x * bytes_pp) + (y * bytes_pl);
			if (bpp == 8)
			{
				pix8 = 0;
				if (A_len)
				{
					tmp = argb_pixel >> 24;
					tmp >>= (8 - A_len);
					tmp <<= A_off;
					pix8 |= tmp;
				}
				tmp = (argb_pixel >> 16) & 0xFF;
				tmp >>= (8 - R_len);
				tmp <<= R_off;
				pix8 |= tmp;
				tmp = (argb_pixel >> 8) & 0xFF;
				tmp >>= (8 - G_len);
				tmp <<= G_off;
				pix8 |= tmp;
				tmp = argb_pixel & 0xFF;
				tmp >>= (8 - B_len);
				tmp <<= B_off;
				pix8 |= tmp;
				*((unsigned char *)(fbp + pix_offset)) = pix8;
			}
			else if (bpp == 16)
			{
				if (finfo->visual == FB_VISUAL_TRUECOLOR)
				{
					pix16 = 0;
					if (A_len)
					{
						tmp = argb_pixel >> 24;
						tmp >>= (8 - A_len);
						tmp <<= A_off;
						pix16 |= tmp;
					}
					tmp = (argb_pixel >> 16) & 0xFF;
					tmp >>= (8 - R_len);
					tmp <<= R_off;
					pix16 |= tmp;
					tmp = (argb_pixel >> 8) & 0xFF;
					tmp >>= (8 - G_len);
					tmp <<= G_off;
					pix16 |= tmp;
					tmp = argb_pixel & 0xFF;
					tmp >>= (8 - B_len);
					tmp <<= B_off;
					pix16 |= tmp;
					*((unsigned short *)(fbp + pix_offset)) = pix16;
				}
				else if (finfo->visual == FB_VISUAL_FOURCC)
				{
					rgb_to_yuv(argb_pixel, argb_pixel_next,
						(unsigned int *)(fbp + pix_offset), bpp,
						vinfo->grayscale == V4L2_PIX_FMT_VYUY ? 0 : 1);
					x++;
				}
			}
			else if (bpp == 32)
			{
				if (finfo->visual == FB_VISUAL_TRUECOLOR)
				{
					pix32 = 0;
					if (R_len == 8)
					{
						pix32 = argb_pixel;
					}
					else
					{
						if (A_len)
						{
							tmp = argb_pixel >> 24;
							tmp >>= (8 - A_len);
							tmp <<= A_off;
							pix32 |= tmp;
						}
						tmp = (argb_pixel >> 16) & 0xFF;
						tmp >>= (8 - R_len);
						tmp <<= R_off;
						pix32 |= tmp;
						tmp = (argb_pixel >> 8) & 0xFF;
						tmp >>= (8 - G_len);
						tmp <<= G_off;
						pix32 |= tmp;
						tmp = argb_pixel & 0xFF;
						tmp >>= (8 - B_len);
						tmp <<= B_off;
						pix32 |= tmp;
					}
					*((unsigned int *)(fbp + pix_offset)) = pix32;
				}
				else if (finfo->visual == FB_VISUAL_FOURCC)
				{
					rgb_to_yuv(argb_pixel, 0,
						(unsigned int *)(fbp + pix_offset), bpp,
						vinfo->grayscale == LOGICVC_PIX_FMT_AYUV ? 0 : 1);
				}
			}
		}
	}
	fclose(fp);
	/* Draw Xylon logo at non-visible area */
	fp = fopen("xylon-logo.ppm", "rb");
	if (fp == NULL)
	{
		puts("Unable to open Xylon logo!");
		return -errno;
	}
	fseek(fp, 14, SEEK_SET);
	for (y = (vinfo->yres+XYLON_TEST_IMAGE_Y_OFFSET);
		y < (vinfo->yres+XYLON_TEST_IMAGE_Y_OFFSET+80);
		y++)
	{
		for (x = 0; x < 154; x++)
		{
			fread(&argb_pixel, 3, 1, fp);
			alpha = 0xFF;
			red = argb_pixel;
			green = argb_pixel >> 8;
			blue = argb_pixel >> 16;
			argb_pixel = (alpha << 24) | (red << 16) | (green << 8) | (blue);
			pix_offset = (x * bytes_pp) + (y * bytes_pl);
			if ((finfo->visual == FB_VISUAL_FOURCC) && (bpp == 16))
			{
				fread(&argb_pixel_next, 3, 1, fp);
				alpha = 0xFF;
				red = argb_pixel_next;
				green = argb_pixel_next >> 8;
				blue = argb_pixel_next >> 16;
				argb_pixel_next =
					(alpha << 24) | (red << 16) | (green << 8) | (blue);
			}
			if (bpp == 8)
			{
				pix8 = 0;
				if (A_len)
				{
					tmp = argb_pixel >> 24;
					tmp >>= (8 - A_len);
					tmp <<= A_off;
					pix8 |= tmp;
				}
				tmp = (argb_pixel >> 16) & 0xFF;
				tmp >>= (8 - R_len);
				tmp <<= R_off;
				pix8 |= tmp;
				tmp = (argb_pixel >> 8) & 0xFF;
				tmp >>= (8 - G_len);
				tmp <<= G_off;
				pix8 |= tmp;
				tmp = argb_pixel & 0xFF;
				tmp >>= (8 - B_len);
				tmp <<= B_off;
				pix8 |= tmp;
				*((unsigned char *)(fbp + pix_offset)) = pix8;
			}
			else if (bpp == 16)
			{
				if (finfo->visual == FB_VISUAL_TRUECOLOR)
				{
					pix16 = 0;
					if (A_len)
					{
						tmp = argb_pixel >> 24;
						tmp >>= (8 - A_len);
						tmp <<= A_off;
						pix16 |= tmp;
					}
					tmp = (argb_pixel >> 16) & 0xFF;
					tmp >>= (8 - R_len);
					tmp <<= R_off;
					pix16 |= tmp;
					tmp = (argb_pixel >> 8) & 0xFF;
					tmp >>= (8 - G_len);
					tmp <<= G_off;
					pix16 |= tmp;
					tmp = argb_pixel & 0xFF;
					tmp >>= (8 - B_len);
					tmp <<= B_off;
					pix16 |= tmp;
					*((unsigned short *)(fbp + pix_offset)) = pix16;
				}
				else if (finfo->visual == FB_VISUAL_FOURCC)
				{
					rgb_to_yuv(argb_pixel, argb_pixel_next,
						(unsigned int *)(fbp + pix_offset), bpp,
						vinfo->grayscale == V4L2_PIX_FMT_VYUY ? 0 : 1);
					x++;
				}
			}
			else if (bpp == 32)
			{
				if (finfo->visual == FB_VISUAL_TRUECOLOR)
				{
					pix32 = 0;
					if (R_len == 8)
					{
						pix32 = argb_pixel;
					}
					else
					{
						if (A_len)
						{
							tmp = argb_pixel >> 24;
							tmp >>= (8 - A_len);
							tmp <<= A_off;
							pix32 |= tmp;
						}
						tmp = (argb_pixel >> 16) & 0xFF;
						tmp >>= (8 - R_len);
						tmp <<= R_off;
						pix32 |= tmp;
						tmp = (argb_pixel >> 8) & 0xFF;
						tmp >>= (8 - G_len);
						tmp <<= G_off;
						pix32 |= tmp;
						tmp = argb_pixel & 0xFF;
						tmp >>= (8 - B_len);
						tmp <<= B_off;
						pix32 |= tmp;
					}
					*((unsigned int *)(fbp + pix_offset)) = pix32;
				}
				else if (finfo->visual == FB_VISUAL_FOURCC)
				{
					rgb_to_yuv(argb_pixel, 0,
						(unsigned int *)(fbp + pix_offset), bpp,
						vinfo->grayscale == LOGICVC_PIX_FMT_AYUV ? 0 : 1);
				}
			}
		}
	}
	fclose(fp);

	return 0;
}

static int resolution_test(void)
{
	struct fb_resolution fbres[] =
	{
		{640,480},
		{800,600},
		{1024,768},
		{1280,720},
		{1280,1024},
		{1680,1050},
		{1920,1080}
	};
	struct fb_resolution fbres_orig;
	struct fb_var_screeninfo vinfo;
	int fbfd;
	int i, res_num;

	fbfd = open("/dev/fb0", O_RDWR);
	if (fbfd < 0)
	{
		perror("Error opening /dev/fb0 for resolution test!\n");
		return errno;
	}

	if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo))
	{
		perror("Error reading variable information");
		goto error_resolution_test;
	}

	fbres_orig.xres = vinfo.xres;
	fbres_orig.yres = vinfo.yres;

	res_num = sizeof(fbres) / sizeof(fbres[0]);

	for (i = 0; i < res_num; i++)
	{
		vinfo.xres = fbres[i].xres;
		vinfo.yres = fbres[i].yres;
		printf("Setting %dx%d@60\n", vinfo.xres, vinfo.yres);
		if (ioctl(fbfd, FBIOPUT_VSCREENINFO, &vinfo))
		{
			perror("Error setting resolution");
			//goto error_resolution_test;
		}
		if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo))
		{
			perror("Error reading variable information");
			//goto error_resolution_test;
		}
		sleep(5);
	}

	vinfo.xres = fbres_orig.xres;
	vinfo.yres = fbres_orig.yres;
	printf("Restoring %dx%d@60\n", vinfo.xres, vinfo.yres);
	if (ioctl(fbfd, FBIOPUT_VSCREENINFO, &vinfo))
	{
		perror("Error restoring resolution");
		goto error_resolution_test;
	}

	close(fbfd);

	return 0;

error_resolution_test:
	close(fbfd);
	return errno;
}

static int edid_test(int tout)
{
	int fbfd;
	int i;
	unsigned char edid[256];

	fbfd = open("/dev/fb0", O_RDWR);
	if (fbfd < 0)
	{
		perror("Error opening /dev/fb0 for EDID test!\n");
		return errno;
	}

	if (ioctl(fbfd, XYLONFB_WAIT_EDID, &tout))
	{
		perror("Wait EDID timeout");
		goto error_edid_test;
	}
	if (ioctl(fbfd, XYLONFB_GET_EDID, &edid))
	{
		perror("Error getting EDID");
		goto error_edid_test;
	}

	close(fbfd);

	puts("EDID:");
	for (i = 0; i < 256; i++) {
		if ((i != 0) && (!(i % 16)))
			puts("");
		printf("0x%02X ", edid[i]);
	}

	return 0;

error_edid_test:
	close(fbfd);
	return errno;
}

//static void get_fb_phys_id_from_name(struct fb_data *fbtest)
//{
//	char *p;
//
//	p = fbtest->finfo.id;
//
//	while((*p) && (((*p) < '0') || ((*p) > '9')))
//		p++;
//
//	fbtest->fb_phys_id = 255;
//	if (*p)
//		fbtest->fb_phys_id = (*p) - '0';
//}


int main(int argc, char *argv[])
{
	struct fb_data *fbtest;
	int i, tmp;
	unsigned char con_off;

	if (argc < 2)
	{
		app_usage_1();
		app_usage_2();
		app_usage_general();
		return 1;
	}

	fbtest = (struct fb_data *)calloc(1, sizeof(struct fb_data));
	if (!fbtest)
	{
		puts("Failed allocation of internal parameter structure!");
		return 2;
	}

	fbtest->fbfd = -1;

	/* check for resolution test */
	for (i = 0; i < argc; i++)
		if (!strcmp("-res", argv[i]))
			return resolution_test();
	/* check for EDID test */
	for (i = 0; i < argc; i++)
		if (!strcmp("-edid", argv[i]))
		{
			if (i+1 == argc)
			{
				puts("Timeout value not specified!");
				return 3;
			}
			sscanf(argv[i+1], "%d", &tmp);
			return edid_test(tmp);
		}
	/* check other arguments */
	con_off = tmp = 0;
	for (i = 0; i < argc; i++)
	{
		if (!strcmp("-list", argv[i]) && !tmp)
		{
			fbtest->fbfd =
				list_fb_devices(&fbtest->finfo,
						&fbtest->vinfo, &fbtest->fbid);
			tmp = 1;
		}
		if (!strcmp("-bpp", argv[i]) && !tmp)
		{
			sscanf(argv[i+1], "%d", &tmp);
			fbtest->bpp = (unsigned char)tmp;
			if ((fbtest->bpp != 8) &&
				(fbtest->bpp != 16) &&
				(fbtest->bpp != 32))
			{
				puts("Invalid bpp value!");
				app_usage_2();
				return 4;
			}
			fbtest->fbfd = find_fb_device(&fbtest->finfo,
						      &fbtest->vinfo,
						      (int)fbtest->bpp,
						      (int)fbtest->expanded,
						      &fbtest->fbid);
			tmp = 1;
		}
		if (!strcmp("-e", argv[i]))
		{
			fbtest->expanded = 1;
		}
		if (!strcmp("-r", argv[i]))
		{
			sscanf(argv[i+1], "%hd%*c%hd",
			       &fbtest->xres, &fbtest->yres);
		}
		if (!strcmp("-conoff", argv[i]))
		{
			con_off = 1;
		}
	}

	if (i < argc)
	{
		puts("Given arguments:");
		for (i = 0; i < argc; i++)
			printf("-> %s\n", argv[i]);
		app_usage_1();
		app_usage_2();
		app_usage_general();
		return 5;
	}

	if (fbtest->fbfd < 0)
	{
		return 6;
	}

	/* check for new resolution */
	if (fbtest->xres && fbtest->yres)
	{
		tmp = fbtest->vinfo.xres;
		fbtest->vinfo.xres = fbtest->xres;
		fbtest->xres = tmp;
		tmp = fbtest->vinfo.yres;
		fbtest->vinfo.yres = fbtest->yres;
		fbtest->yres = tmp;
		printf("Setting %dx%d@60\n",
		       fbtest->vinfo.xres, fbtest->vinfo.yres);
		/* set new resolution */
		if (ioctl(fbtest->fbfd, FBIOPUT_VSCREENINFO, &fbtest->vinfo))
		{
			perror("Error setting resolution");
			return errno;
		}
		if (ioctl(fbtest->fbfd, FBIOGET_VSCREENINFO, &fbtest->vinfo))
		{
			perror("Error reading variable information");
			return errno;
		}
	}

	/* Get IP core version */
	if (ioctl(fbtest->fbfd, XYLONFB_IP_CORE_VERSION, &fbtest->ip_ver))
		perror("IOCTL Error");
	printf("logiCVC IP core version: %d.%02d.%c\n",
		(fbtest->ip_ver >> 16),
		((fbtest->ip_ver >> 8) & 0xFF),
		((fbtest->ip_ver & 0xFF) + 'a'));

	/* Get physical used framebuffer ID */
//	get_fb_phys_id_from_name(fbtest);
	if (ioctl(fbtest->fbfd, XYLONFB_LAYER_IDX, &fbtest->ioctl_arg))
		perror("IOCTL Error");
	fbtest->fb_phys_id = (unsigned char)fbtest->ioctl_arg;

	/*
		NOTE:
		logiCVC layer control register intentionally cannot be accessed
		through IOCTL. If there is need for that, this is a way how to
		do it manually.
	*/
	/* Turn off console layer (allways /dev/fb0) */
	if (con_off && fbtest->fbid != 0)
	{
		puts("Turning off console layer...");
		tmp = open("/dev/fb0", O_RDWR);
		if (tmp > 0)
		{
			if (ioctl(tmp, XYLONFB_LAYER_IDX, &fbtest->ioctl_arg))
			{
				perror("IOCTL Error");
			}
			else
			{
				i = fbtest->ioctl_arg;
				printf("/dev/fb0 = logiCVC layer %d\n", i);
				fbtest->hw_access.set = FALSE;
				fbtest->hw_access.offset =
					LOGICVC_BASE_LAYER_CTRL_REG_ADDR +
					(i * 0x80);
				if (ioctl(tmp, XYLONFB_HW_ACCESS,
				    &fbtest->hw_access))
				{
					perror("IOCTL Error");
				}
				else
				{
					fbtest->hw_access.set = TRUE;
					fbtest->hw_access.value &= ~0x01;
					if (ioctl(tmp, XYLONFB_HW_ACCESS,
					    &fbtest->hw_access))
					{
						perror("IOCTL Error");
					}
				}
			}
			close(tmp);
		}
		else
		{
			perror("Error opening /dev/fb0\n");
		}
	}

	/* Set screen in bytes */
	fbtest->screensize = fbtest->finfo.line_length *
			     fbtest->vinfo.yres_virtual;

	/* Map the framebuffer to memory */
	fbtest->fbp = (unsigned char *)mmap(0, fbtest->screensize,
					    PROT_READ | PROT_WRITE, MAP_SHARED,
					    fbtest->fbfd, 0);
	if ((int)fbtest->fbp == -1)
	{
		perror("Error mapping framebuffer device");
		close(fbtest->fbfd);
		return errno;
	}

	puts("Framebuffer device successfully mapped.");

	printf("Details:\n");
	printf("FrameBuffer physical ID: %d\n", fbtest->fb_phys_id);
	printf(" X resolution: %d\n Y resolution: %d\n" \
	       " Virtual X resolution: %d\n Virtual Y resolution: %d\n" \
	       " Bits Per Pixel: %d\n" \
	       " X offset: %d\n Y offset: %d\n",
		fbtest->vinfo.xres, fbtest->vinfo.yres,
		fbtest->vinfo.xres_virtual, fbtest->vinfo.yres_virtual,
		fbtest->vinfo.bits_per_pixel,
		fbtest->vinfo.xoffset, fbtest->vinfo.yoffset);
	printf("Line length in bytes: %d\n", fbtest->finfo.line_length);
	printf("FrameBuffer size %d bytes\n", fbtest->screensize);
	printf("FrameBuffer address: 0x%X\n", (unsigned int)fbtest->fbp);

	fbtest->cmap_data =
		(struct cmap_data *)calloc(1, sizeof(struct cmap_data));

	if (change_cmap(&fbtest->finfo, &fbtest->vinfo,
		fbtest->cmap_data, fbtest->fbfd))
	{
		perror("Error changing CLUT color map");
		goto cmap_error;
	}

	draw_rectangles(&fbtest->finfo, &fbtest->vinfo, fbtest->cmap_data,
		fbtest->fbfd, fbtest->fbp, 0x00FF0000);

	if ((fbtest->finfo.visual == FB_VISUAL_TRUECOLOR) ||
		((fbtest->finfo.visual == FB_VISUAL_FOURCC) &&
		(fbtest->vinfo.bits_per_pixel != 8)))
	{
		puts("Drawing logos...");
		if (fbtest->vinfo.xres_virtual > fbtest->vinfo.xres &&
			fbtest->vinfo.yres_virtual > fbtest->vinfo.yres)
		{
			if (draw_logos(&fbtest->finfo, &fbtest->vinfo,
				       fbtest->fbp) < 0)
			{
				goto app_exit;
			}
		}
	}

	puts("Panning");
	if ((fbtest->finfo.visual == FB_VISUAL_FOURCC) &&
	    (fbtest->vinfo.bits_per_pixel == 16))
	{
		tmp = 2;
	}
	else
	{
		tmp = 1;
	}
	fbtest->vinfo.xoffset = 0;
	for (i = 0; i < (fbtest->vinfo.yres*8/10); i += tmp)
	{
		if (!(i % 5))
			fbtest->vinfo.xoffset += tmp;
		fbtest->vinfo.yoffset = i;

		if (ioctl(fbtest->fbfd, FBIOPAN_DISPLAY, &fbtest->vinfo))
		{
			perror("Error panning the display");
			break;
		}

		usleep(1000);
	}
	for (; i >= 0; i -= tmp)
	{
		if (!(i % 5))
			fbtest->vinfo.xoffset -= tmp;
		fbtest->vinfo.yoffset = i;

		if (ioctl(fbtest->fbfd, FBIOPAN_DISPLAY, &fbtest->vinfo))
		{
			perror("Error panning the display");
			break;
		}

		usleep(1000);
	}
	fbtest->vinfo.xoffset = 0;
	fbtest->vinfo.yoffset = 0;
	if (ioctl(fbtest->fbfd, FBIOPAN_DISPLAY, &fbtest->vinfo))
	{
		perror("Error panning the display");
	}

	puts("Buffering");
	usleep(500000);
	if ((fbtest->ip_ver >> 16) < 4)
	{
		fbtest->layer_buffer.set = TRUE;
		fbtest->layer_buffer.id = 1;
		if (ioctl(fbtest->fbfd, XYLONFB_LAYER_BUFFER,
			  &fbtest->layer_buffer))
		{
			perror("Error buffering");
		}
		fbtest->layer_buffer.set = FALSE;
		if (ioctl(fbtest->fbfd, XYLONFB_LAYER_BUFFER,
			  &fbtest->layer_buffer))
		{
			perror("Error get buffer");
		}
		printf("Visible buffer: %d\n", fbtest->layer_buffer.id);
		if (ioctl(fbtest->fbfd, XYLONFB_LAYER_BUFFER_OFFSET,
			  &fbtest->ioctl_arg))
			perror("IOCTL Error");
		else
			printf("Buffer offset %d\n",
				(unsigned int)fbtest->ioctl_arg);

		sleep(1);

		fbtest->layer_buffer.set = TRUE;
		fbtest->layer_buffer.id = 2;
		if (ioctl(fbtest->fbfd, XYLONFB_LAYER_BUFFER,
			  &fbtest->layer_buffer))
		{
			perror("Error buffering");
		}
		fbtest->layer_buffer.set = FALSE;
		if (ioctl(fbtest->fbfd, XYLONFB_LAYER_BUFFER,
			  &fbtest->layer_buffer))
		{
			perror("Error get buffer");
		}
		printf("Visible buffer: %d\n", fbtest->layer_buffer.id);
		if (ioctl(fbtest->fbfd, XYLONFB_LAYER_BUFFER_OFFSET,
			  &fbtest->ioctl_arg))
			perror("IOCTL Error");
		else
			printf("Buffer offset %d\n",
				(unsigned int)fbtest->ioctl_arg);

		sleep(1);

		fbtest->layer_buffer.set = TRUE;
		fbtest->layer_buffer.id = 0;
		if (ioctl(fbtest->fbfd, XYLONFB_LAYER_BUFFER,
			  &fbtest->layer_buffer))
		{
			perror("Error buffering");
		}
		fbtest->layer_buffer.set = FALSE;
		if (ioctl(fbtest->fbfd, XYLONFB_LAYER_BUFFER,
			  &fbtest->layer_buffer))
		{
			perror("Error get buffer");
		}
		printf("Visible buffer: %d\n", fbtest->layer_buffer.id);
		if (ioctl(fbtest->fbfd, XYLONFB_LAYER_BUFFER_OFFSET,
			  &fbtest->ioctl_arg))
			perror("IOCTL Error");
		else
			printf("Buffer offset %d\n",
				(unsigned int)fbtest->ioctl_arg);
	}
	else
	{
		fbtest->vinfo.xoffset = 0;
		fbtest->vinfo.yoffset = fbtest->vinfo.yres;
		if (ioctl(fbtest->fbfd, FBIOPAN_DISPLAY, &fbtest->vinfo))
		{
			perror("Error panning the display");
		}
		printf("Visible buffer: %d\n",
			(fbtest->vinfo.yoffset / fbtest->vinfo.yres) - 1);
		printf("Buffer offset %d\n",
			(fbtest->vinfo.yoffset / fbtest->vinfo.yres) - 1);

		sleep(1);

		fbtest->vinfo.yoffset += fbtest->vinfo.yres;
		if (ioctl(fbtest->fbfd, FBIOPAN_DISPLAY, &fbtest->vinfo))
		{
			perror("Error panning the display");
		}
		printf("Visible buffer: %d\n",
			(fbtest->vinfo.yoffset / fbtest->vinfo.yres) - 1);
		printf("Buffer offset %d\n",
			(fbtest->vinfo.yoffset / fbtest->vinfo.yres) - 1);

		sleep(1);

		fbtest->vinfo.yoffset = 0;
		if (ioctl(fbtest->fbfd, FBIOPAN_DISPLAY, &fbtest->vinfo))
		{
			perror("Error panning the display");
		}
		printf("Visible buffer: %d\n", fbtest->vinfo.yoffset);
		printf("Buffer offset %d\n", fbtest->vinfo.yoffset);
	}

	puts("V-Sync");
	fbtest->ioctl_flag = TRUE;
	if (ioctl(fbtest->fbfd, XYLONFB_VSYNC_CTRL, &fbtest->ioctl_flag))
		perror("Error enabling V-sync");
	i = 0;
	fbtest->arg = 0;
	while(i < 2)
	{
		puts("Wait for V-sync...");
		if (ioctl(fbtest->fbfd, FBIO_WAITFORVSYNC, &fbtest->arg))
			perror("Error waiting for V-sync");
		else
			puts("V-sync detected!");
		sleep(1);
		i++;
	}
	fbtest->ioctl_flag = FALSE;
	if (ioctl(fbtest->fbfd, XYLONFB_VSYNC_CTRL, &fbtest->ioctl_flag))
		perror("Error disabling V-sync");

	/* testing logiCVC specific IOCTL's */
	puts("Set layer background color");
	fbtest->layer_color.set = TRUE;
	fbtest->layer_color.r = 0x7F;
	fbtest->layer_color.g = 0x7F;
	fbtest->layer_color.b = 0x7F;
	if (ioctl(fbtest->fbfd, XYLONFB_BACKGROUND_COLOR, &fbtest->layer_color))
		perror("IOCTL Error");

	puts("Get layer index");
	if (ioctl(fbtest->fbfd, XYLONFB_LAYER_IDX, &fbtest->ioctl_arg))
		perror("IOCTL Error");
	else
		printf("Layer index %lu\n", fbtest->ioctl_arg);

	puts("Get layer alpha");
	fbtest->layer_transp.set = FALSE;
	if (ioctl(fbtest->fbfd, XYLONFB_LAYER_ALPHA, &fbtest->layer_transp))
		perror("IOCTL Error");
	else
		printf("Layer alpha 0x%X\n", fbtest->layer_transp.alpha);

	puts("Set layer alpha");
	fbtest->layer_transp.set = TRUE;
	for (i = 0xFF; i >= 0; i--)
	{
		fbtest->layer_transp.alpha = i;
		if (ioctl(fbtest->fbfd, XYLONFB_LAYER_ALPHA,
		    &fbtest->layer_transp))
		{
			perror("IOCTL Error");
			break;
		}
		usleep(2000);
	}
	for (i = 0; i < 0xFF; i++)
	{
		fbtest->layer_transp.alpha = i;
		if (ioctl(fbtest->fbfd, XYLONFB_LAYER_ALPHA,
		    &fbtest->layer_transp))
		{
			perror("IOCTL Error");
			break;
		}
		usleep(2000);
	}
	sleep(1);

	puts("Enable layer transparent color");
	fbtest->ioctl_arg = 0;
	if (ioctl(fbtest->fbfd, XYLONFB_LAYER_COLOR_TRANSP_CTRL,
		  &fbtest->ioctl_arg))
		perror("IOCTL Error");

	puts("Get layer transparent color");
	memset(&fbtest->layer_color, 0, sizeof(struct xylonfb_layer_color));
	if (ioctl(fbtest->fbfd, XYLONFB_LAYER_COLOR_TRANSP,
		  &fbtest->layer_color))
		perror("IOCTL Error");
	printf("Layer transparent color R 0x%X G 0x%X B 0x%X\n",
		fbtest->layer_color.r, fbtest->layer_color.g, fbtest->layer_color.b);
	printf("Layer transparent color RGB 0x%X\n", fbtest->layer_color.raw_rgb);

	fbtest->ioctl_tmp = fbtest->layer_color.raw_rgb;

	puts("Set layer transparent color");
	fbtest->layer_color.set = TRUE;
	fbtest->layer_color.r = 0xFF;
	fbtest->layer_color.g = 0;
	fbtest->layer_color.b = 0;
	if (ioctl(fbtest->fbfd, XYLONFB_LAYER_COLOR_TRANSP,
		  &fbtest->layer_color))
		perror("IOCTL Error");
	sleep(1);
	fbtest->layer_color.r = 0;
	fbtest->layer_color.g = 0xFF;
	fbtest->layer_color.b = 0;
	if (ioctl(fbtest->fbfd, XYLONFB_LAYER_COLOR_TRANSP,
		  &fbtest->layer_color))
		perror("IOCTL Error");
	sleep(1);
	fbtest->layer_color.r = 0;
	fbtest->layer_color.g = 0;
	fbtest->layer_color.b = 0xFF;
	if (ioctl(fbtest->fbfd, XYLONFB_LAYER_COLOR_TRANSP,
		  &fbtest->layer_color))
		perror("IOCTL Error");
	sleep(1);

	fbtest->layer_color.raw_rgb = fbtest->ioctl_tmp;
	fbtest->layer_color.use_raw = 1;
	if (ioctl(fbtest->fbfd, XYLONFB_LAYER_COLOR_TRANSP,
		  &fbtest->layer_color))
		perror("IOCTL Error");
	sleep(1);

	fbtest->layer_geometry.set = FALSE;
	puts("Get layer size and position");
	if (ioctl(fbtest->fbfd, XYLONFB_LAYER_GEOMETRY,
		  &fbtest->layer_geometry))
		perror("IOCTL Error");
	printf("Layer position X:%d Y:%d W:%d H:%d\n",
		fbtest->layer_geometry.x, fbtest->layer_geometry.y,
		fbtest->layer_geometry.width, fbtest->layer_geometry.height);

	fbtest->layer_geometry.set = TRUE;
	fbtest->layer_geometry.x = 0;
	fbtest->layer_geometry.y = 0;
	fbtest->layer_geometry.width = fbtest->vinfo.xres;
	fbtest->layer_geometry.height = fbtest->vinfo.yres;
	if (ioctl(fbtest->fbfd, XYLONFB_LAYER_GEOMETRY,
		  &fbtest->layer_geometry))
		perror("IOCTL Error");

	puts("Set layer offset");
	if ((fbtest->finfo.visual == FB_VISUAL_FOURCC) &&
	    (fbtest->vinfo.bits_per_pixel == 16))
	{
		tmp = 2;
	}
	else
	{
		tmp = 1;
	}
	for (i = 0; i <= fbtest->vinfo.xres/2; i += tmp)
	{
		fbtest->layer_geometry.x_offset += tmp;
		if (!(i % 5))
			fbtest->layer_geometry.y_offset += tmp;
		if (ioctl(fbtest->fbfd, XYLONFB_LAYER_GEOMETRY,
			  &fbtest->layer_geometry))
			perror("IOCTL Error");
		usleep(2000);
	}
	while (1)
	{
		if ((fbtest->layer_geometry.x_offset == 0) &&
		    (fbtest->layer_geometry.y_offset == 0))
			break;
		if (fbtest->layer_geometry.x_offset > 0)
			fbtest->layer_geometry.x_offset -= tmp;
		if (fbtest->layer_geometry.y_offset > 0)
			fbtest->layer_geometry.y_offset -= tmp;
		if (ioctl(fbtest->fbfd, XYLONFB_LAYER_GEOMETRY,
			  &fbtest->layer_geometry))
			perror("IOCTL Error");
		usleep(2000);
	}

	puts("Set layer position");
	for (i = 0; i <= fbtest->vinfo.xres/2; i += tmp)
	{
		fbtest->layer_geometry.x += tmp;
		if (!(i % 5))
			fbtest->layer_geometry.y += tmp;
		if (ioctl(fbtest->fbfd, XYLONFB_LAYER_GEOMETRY,
			  &fbtest->layer_geometry))
			perror("IOCTL Error");
		usleep(2000);
	}
	while (1)
	{
		if ((fbtest->layer_geometry.x == 0) &&
		    (fbtest->layer_geometry.y == 0))
			break;
		if (fbtest->layer_geometry.x > 0)
			fbtest->layer_geometry.x -= tmp;
		if (fbtest->layer_geometry.y > 0)
			fbtest->layer_geometry.y -= tmp;
		if (ioctl(fbtest->fbfd, XYLONFB_LAYER_GEOMETRY,
			  &fbtest->layer_geometry))
			perror("IOCTL Error");
		usleep(2000);
	}

	puts("Set layer size");
	for (i = 0; i <= fbtest->vinfo.xres-100; i += tmp)
	{
		fbtest->layer_geometry.width -= tmp;
		if (!(i % 5))
			fbtest->layer_geometry.height -= tmp;
		if (ioctl(fbtest->fbfd, XYLONFB_LAYER_GEOMETRY,
			  &fbtest->layer_geometry))
			perror("IOCTL Error");
		usleep(2000);
	}
	while (1)
	{
		if ((fbtest->layer_geometry.width == fbtest->vinfo.xres) &&
		    (fbtest->layer_geometry.height == fbtest->vinfo.yres))
			break;
		if (fbtest->layer_geometry.width < fbtest->vinfo.xres)
			fbtest->layer_geometry.width += tmp;
		if (fbtest->layer_geometry.height < fbtest->vinfo.yres)
			fbtest->layer_geometry.height += tmp;
		if (ioctl(fbtest->fbfd, XYLONFB_LAYER_GEOMETRY,
			  &fbtest->layer_geometry))
			perror("IOCTL Error");
		usleep(2000);
	}

	printf("Get layer buffer offset %d\n", (unsigned int)fbtest->ioctl_arg);
	if (ioctl(fbtest->fbfd, XYLONFB_LAYER_BUFFER_OFFSET,
		  &fbtest->ioctl_arg))
		perror("IOCTL Error");
	else
		printf("Layer buffer offset %d\n",
			(unsigned int)fbtest->ioctl_arg);
	sleep(1);

#ifdef HW_BUFFER_SWITCHING
	puts("Enable layer external HW controlled buffer switch");
	fbtest->ioctl_arg = 1;
	if (ioctl(fbtest->fbfd, XYLONFB_LAYER_EXT_BUFF_SWITCH,
		  &fbtest->ioctl_arg))
		perror("IOCTL Error");
	sleep(5);
	puts("Disable layer external HW controlled buffer switch");
	fbtest->ioctl_arg = 0;
	if (ioctl(fbtest->fbfd, XYLONFB_LAYER_EXT_BUFF_SWITCH,
		  &fbtest->ioctl_arg))
		perror("IOCTL Error");
#endif

	fbtest->hw_access.set = FALSE;
	fbtest->hw_access.offset = LOGICVC_BASE_LAYER_CTRL_REG_ADDR +
				   (fbtest->fb_phys_id * 0x80);
	if (ioctl(fbtest->fbfd, XYLONFB_HW_ACCESS, &fbtest->hw_access))
	{
		perror("IOCTL Error");
	}
	else
	{
		puts("Turn off graphic layer");
		fbtest->hw_access.set = TRUE;
		fbtest->hw_access.value &= ~0x01;
		if (ioctl(fbtest->fbfd, XYLONFB_HW_ACCESS, &fbtest->hw_access))
			  perror("IOCTL Error");
		sleep(1);
	}

	/* Background color test */
	puts("Get layer background color");
	memset(&fbtest->layer_color, 0, sizeof(struct xylonfb_layer_color));
	if (ioctl(fbtest->fbfd, XYLONFB_BACKGROUND_COLOR,
		  &fbtest->layer_color))
	{
		perror("IOCTL Error");
	}
	else
	{
		printf("Layer background color R 0x%X G 0x%X B 0x%X\n",
			fbtest->layer_color.r,
			fbtest->layer_color.g,
			fbtest->layer_color.b);
		printf("Layer background color RGB 0x%X\n",
			fbtest->layer_color.raw_rgb);

		puts("Set layer background color");
		fbtest->layer_color.set = TRUE;
		fbtest->layer_color.use_raw = 0;
		for (i = fbtest->layer_color.r; i < 255; i++)
		{
			fbtest->layer_color.r = i;
			fbtest->layer_color.g = i;
			fbtest->layer_color.b = i;
			if (ioctl(fbtest->fbfd, XYLONFB_BACKGROUND_COLOR,
				  &fbtest->layer_color))
				perror("IOCTL Error");
			usleep(10000);
		}
		for (; i >= 0; i--)
		{
			fbtest->layer_color.r = i;
			fbtest->layer_color.g = i;
			fbtest->layer_color.b = i;
			if (ioctl(fbtest->fbfd, XYLONFB_BACKGROUND_COLOR,
				  &fbtest->layer_color))
				perror("IOCTL Error");
			usleep(10000);
		}
	}

	/* Clear drawn rectangles */
	draw_rectangles(&fbtest->finfo, &fbtest->vinfo, fbtest->cmap_data,
		fbtest->fbfd, fbtest->fbp, 0);

	/* Turn on console layer */
	if (fbtest->fbid == 0)
	{
		puts("Turn on graphic layer");
		fbtest->hw_access.set = TRUE;
		fbtest->hw_access.value |= 0x01;
		if (ioctl(fbtest->fbfd, XYLONFB_HW_ACCESS, &fbtest->hw_access))
			perror("IOCTL Error");
	}
	else if (con_off)
	{
		puts("Turning on console layer... ");
		tmp = open("/dev/fb0", O_RDWR);
		if (tmp > 0)
		{
			if (ioctl(tmp, XYLONFB_LAYER_IDX, &fbtest->ioctl_arg))
			{
				perror("IOCTL Error");
			}
			else
			{
				i = fbtest->ioctl_arg;
				fbtest->hw_access.set = FALSE;
				fbtest->hw_access.offset =
					LOGICVC_BASE_LAYER_CTRL_REG_ADDR +
					(i * 0x80);
				if (ioctl(tmp, XYLONFB_HW_ACCESS,
				    &fbtest->hw_access))
				{
					perror("IOCTL Error");
				}
				else
				{
					fbtest->hw_access.set = TRUE;
					fbtest->hw_access.value |= 0x01;
					if (ioctl(tmp, XYLONFB_HW_ACCESS,
					    &fbtest->hw_access))
						perror("IOCTL Error");
				}
			}
			close(tmp);
		}
		else
		{
			perror("Error opening /dev/fb0\n");
		}
	}

app_exit:
	restore_cmap(fbtest->cmap_data, fbtest->fbfd);
cmap_error:
	free(fbtest->cmap_data);
	munmap(fbtest->fbp, fbtest->screensize);
	/* restore original resolution */
	if (fbtest->xres && fbtest->yres)
	{
		fbtest->vinfo.xres = fbtest->xres;
		fbtest->vinfo.yres = fbtest->yres;
		printf("Restoring %dx%d@60\n",
			fbtest->vinfo.xres, fbtest->vinfo.yres);
		/* set new resolution */
		if (ioctl(fbtest->fbfd, FBIOPUT_VSCREENINFO, &fbtest->vinfo))
		{
			perror("Error setting resolution");
			return errno;
		}
	}
	close(fbtest->fbfd);
	free(fbtest);

	return 0;
}
