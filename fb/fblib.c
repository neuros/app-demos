#include "fblib.h"

static char *fb_names[] = {
	[FB_OSD0] = "dm_osd0_fb",
	[FB_OSD1] = "dm_osd1_fb",
	[FB_VID0] = "dm_vid0_fb",
	[FB_VID1] = "dm_vid1_fb",
};

static fb_im_t images[] = {
	{
		.w = 320,
		.h = 240,
		.fname = DATA_DIR"320x240_duck.rgb565",
		.format = FB_FORMAT_RGB565,
	},
	{
		.w = 720,
		.h = 480,
		.fname = DATA_DIR"720x480_bars.uyvy",
		.format = FB_FORMAT_YUV422,
	},
};



/*============================================================================*
 *                           Framebuffer Functions                            *
 *============================================================================*/
int fb_info_get(fb_t *fb)
{
	/* query var information */
	if (ioctl(fb->fd, FBIOGET_VSCREENINFO, &fb->var) < 0)
	{
		printf("couldn't get var screen info\n");
		return -1;
	}
	/* set the format field */
	switch (fb->var.bits_per_pixel)
	{
		case 16:
		if ((fb->plane != FB_VID0) && (fb->plane != FB_VID1))
			fb->format = FB_FORMAT_RGB565;
		else
			fb->format = FB_FORMAT_YUV422;
		break;

		case 24:
		fb->format = FB_FORMAT_RGB888;
		break;

		case 4:
		fb->format = FB_FORMAT_bA3;
		break;
	}
	/* query fixed information */
	if (ioctl(fb->fd, FBIOGET_FSCREENINFO, &fb->fix) < 0)
	{
		printf("couldn't get fix screen info\n");
		return -1;
	}
	return 0;
}

int fb_modes_get(fb_t *fb)
{
	/* open the sysfs file for vid0 */
	/* read the modes */
	return 1;
}


int fb_mmap(fb_t *fb)
{
	if (fb->mmap)
		munmap(fb->mmap, fb->size);

	fb->size = (fb->var.xres_virtual * fb->var.yres_virtual * fb->var.bits_per_pixel) / 8;
	fb->mmap = mmap(0, fb->size, PROT_READ | PROT_WRITE, MAP_SHARED, fb->fd, 0);
	if ((int)fb->mmap  == -1)
	{
		printf("couldn't map the framebuffer %d\n", errno);
		return -1;
	}
	return 0;
}

void fb_dump(fb_t *fb)
{
	printf("fb device (%s)\n", fb->file);
	printf("  plane %d\n", fb->plane);
	printf("  mmap at %p to %p (length 0x%08x)\n", fb->mmap, (char *)fb->mmap + fb->size, fb->size);
	printf("  size %dx%d (%dx%d)\n", fb->var.xres, fb->var.yres, fb->var.xres_virtual, fb->var.yres_virtual);
	printf("  line_length %d\n", fb->fix.line_length);
	printf("  bpp = %d\n", fb->var.bits_per_pixel);
	printf("  pos x = %d y = %d\n", fb->var.reserved[0], fb->var.reserved[1]);
}

void fb_delete(fb_t *fb)
{
	if (fb->mmap)
		munmap(fb->mmap, fb->size);
	close(fb->fd);
	free(fb->file);
	free(fb);
}

fb_t * fb_new(const char *name)
{
	fb_t *fb;
	char tmp[256];
	int plane;
	int minor;

	fb = calloc(1, sizeof(fb_t));

	/* get the plane name */
	for (plane = 0; plane < FBS; plane++)
	{
		if (!strcmp(name, fb_names[plane]))
			break;
	}
	if (plane == FBS)
	{
		printf("Error: No matching fb name found\n");
		return NULL;
	}
	/* open all fbs and get the name */
	for (minor = 0; minor < FBS; minor++)
	{
		sprintf(tmp, "/dev/fb%d", minor);
		fb->fd = open(tmp, O_RDWR);
		fb->plane = plane;
		if (fb_info_get(fb) < 0)
		{
			close(fb->fd);
			continue;
		}
		if (!strcmp(fb->fix.id, name))
			break;
	}
	/* no match */
	if (minor == FBS)
	{
		printf("Error: No matching fb number found\n");
		goto error;
	}
	if (fb_mmap(fb) < 0)
	{
		printf("Error: Can not map the memory\n");
		goto error;
	}
	/* dump current values */
	fb->file = strdup(tmp);
	fb_dump(fb);
	return fb;

error:
	close(fb->fd);
	fb_delete(fb);
	return NULL;
}

void fb_transp_set(fb_t *fb, int on)
{
	if (ioctl(fb->fd, FBIO_TRANSP, &on) < 0)
	{
		printf("couldn't set transparency to %d\n", on);
		return;
	}
	else
	{
		fb_info_get(fb);
	}
}

void fb_transp_solor_set(fb_t *fb, int color)
{
	if (ioctl(fb->fd, FBIO_TRANSP_COLOR, &color) < 0)
	{
		printf("couldn't set transparency color to %d\n", color);
		return;
	}
	else
	{
		fb_info_get(fb);
	}
}

void fb_y_set(fb_t *fb, int y)
{
	if (ioctl(fb->fd, FBIO_SETPOSY, &y) < 0)
	{
		printf("couldn't set x position\n");
		return;
	}
	else
	{
		fb_info_get(fb);
		fb->y = y;
	}
}

void fb_x_set(fb_t *fb, int x)
{
	if (ioctl(fb->fd, FBIO_SETPOSX, &x) < 0)
	{
		printf("couldn't set x position\n");
		return;
	}
	else
	{
		fb_info_get(fb);
		fb->x = x;
	}
}

void fb_bpp_set(fb_t *fb, int bpp, int attr)
{
	struct fb_var_screeninfo var;

	if (attr && fb->plane != FB_OSD1)
		return;

	var = fb->var;
	if (attr && bpp == 4)
	{
		struct fb_bitfield transp;
		transp.length = 3;
		transp.offset = 0;

		var.transp = transp;
	}
	var.bits_per_pixel = bpp;
	if (ioctl(fb->fd, FBIOPUT_VSCREENINFO, &var) < 0)
	{
		printf("couldn't set var screen info\n");
		return;
	}
	else
		fb_info_get(fb);
	fb_mmap(fb);
}

void fb_size_set(fb_t *fb, int w, int h, int vw, int vh)
{
	struct fb_var_screeninfo var;
	int col;

	if (w == fb->var.xres && h == fb->var.yres)
		return;
	var = fb->var;
	var.xres_virtual = vw;
	var.yres_virtual = vh;
	var.xres = w;
	var.yres = h;

	/* set var information */
	if (ioctl(fb->fd, FBIOPUT_VSCREENINFO, &var) < 0)
	{
		printf("couldn't set var screen info\n");
		return;
	}
	else
		fb_info_get(fb);
	/* clear the screen */
	if (fb->format == FB_FORMAT_YUV422)
		col = 0x88;
	else if ((fb->format == FB_FORMAT_RGB565) ||
		(fb->format == FB_FORMAT_RGB888))
		col = 0x00;
	else if (fb->format == FB_FORMAT_bA3)
		col = 0x55;
	else
		printf("unsupported format?\n");

	fb_mmap(fb);
	memset(fb->mmap, col, fb->size);
}

void fb_enable(fb_t *fb, unsigned int on)
{
	ioctl(fb->fd, FBIO_ENABLE, &on);
}

void fb_image_draw(fb_t *fb)
{
	fb_im_t *im = NULL;
	int i;
	char *fb_tmp = fb->mmap;
	char *im_tmp;
	int bytes;

	/* get the image with the same format */
	for (i = 0; i < FB_IMAGES_NUM; i++)
	{
		if (images[i].format == fb->format)
		{
			im = &images[i];
			break;
		}
	}
	if (!im)
	{
		printf("No suitable image for format %d\n", fb->format);
		return;
	}
	im_tmp = im->buf;
	bytes = im->w * fb->var.bits_per_pixel / 8;
	/* draw */
	for (i = 0; i < im->h; i++)
	{
		memcpy(fb_tmp, im_tmp, bytes);
		//printf("bytes = %x\n", *(short int*)im_tmp);
		im_tmp += bytes;
		fb_tmp += fb->fix.line_length;
	}

}

void fb_image_load(void)
{
	int i;

	printf("Loading images ...\n");
	for (i = 0; i < FB_IMAGES_NUM; i++)
	{
		FILE *f;
		struct stat st;
		fb_im_t *im = &images[i];

		printf("%s\n", im->fname);
		f = fopen(im->fname, "r");
		stat(im->fname, &st);
		im->buf = malloc(sizeof(char) * st.st_size);
		fread(im->buf, st.st_size, 1, f);
		fclose(f);
	}
}

