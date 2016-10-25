/***************************************************************************
**                                                                        **
**  Fbfill is a utility application used to clear/fill the frame buffer.  **
**  Copyright (C) 2016 Xylon d.o.o.                                      **
**                                                                        **
**  This program is free software: you can redistribute it and/or modify  **
**  it under the terms of the GNU General Public License as published by  **
**  the Free Software Foundation, either version 3 of the License, or     **
**  (at your option) any later version.                                   **
**                                                                        **
**  This program is distributed in the hope that it will be useful,       **
**  but WITHOUT ANY WARRANTY; without even the implied warranty of        **
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         **
**  GNU General Public License for more details.                          **
**                                                                        **
**  You should have received a copy of the GNU General Public License     **
**  along with this program.  If not, see http://www.gnu.org/licenses/.   **
**                                                                        **
****************************************************************************/
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdint.h>

#define MAX_STRLEN 10
int main (int argc, char *argv[])
{
	struct fb_fix_screeninfo finfo;
	struct fb_var_screeninfo vinfo;
	void *fbp, *p, *fbp_end;
	
	unsigned int fbfd, width, fb_size;
	unsigned long long int value;
	char fb_dev[MAX_STRLEN];

	if(argc != 4)
	{
		puts("Usage: fbfill /dev/fb*  bits-per-pixel  value");
		return -1;
	}
	if(strlen(argv[1]) >= MAX_STRLEN)
	{
		printf("string %s to long\n", argv[1]);
		return -1;
	}
	else
		strcpy(fb_dev, argv[1]);
	width=strtol(argv[2], 0, 0);
	if(width != 8 && width != 16 && width != 32 && width != 64)
	{
		puts("Supported bits-per-pixel: 8, 16, 32, 64");
		return -1;
	}
	value=strtoull(argv[3], 0, 0);

	printf("0x%llX\n", value);
	fbfd = open(fb_dev, O_RDWR);
	if (fbfd < 0)
	{
		printf("Error opening framebuffer device %s\n", fb_dev);
		perror(NULL);
		return -errno;
	}

	/* Get fixed screen information */
	if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo))
	{
		perror("Error reading fixed information");
		return -errno;
	}
	/* Get variable screen information */
	if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo))
	{
		perror("Error reading variable information");
		return -errno;
	}

	/* Calculate framebuffer memory size */
	fb_size = finfo.line_length * vinfo.yres_virtual;

	/* Map the framebuffer to memory */
	fbp = mmap(0, fb_size, PROT_READ | PROT_WRITE,
				    MAP_SHARED, fbfd, 0);

	if ((int)fbp == -1)
	{
		perror("Error mapping framebuffer device");
		close(fbfd);
		return -errno;
	}

	p = fbp;
	fbp_end = fbp + fb_size;
#ifdef _DEBUG	
	puts("Framebuffer device successfully mapped.");
#endif
	switch(width){
		case 8:	memset(p, value, fb_size);
				break;
		case 16:for(;p < fbp_end; p += 2)
				*(uint16_t*)p = value;
			break;
		case 32:for(;p < fbp_end; p += 4)
				*(uint32_t*)p = value;
			break;
		case 64:for(;p < fbp_end; p += 8)
				*(uint64_t*)p = value;
			break;
	}
#ifdef _DEBUG	
	puts("Framebuffer device write successfull!");
#endif
	munmap(fbp, fb_size);
	close(fbfd);
	return 0;
}
