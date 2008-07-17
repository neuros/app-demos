#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/fb.h>
#include <unistd.h>

#include <video/davincifb.h>

typedef enum
{
	FB_FORMAT_RGB565,
	FB_FORMAT_RGB888,
	FB_FORMAT_YUV422,
	FB_FORMAT_bA3,
	FB_FORMATS,
} fb_format_t;


typedef struct fb_t {
	int fd;
	int minor;
	int plane;

	struct fb_fix_screeninfo fix;
	struct fb_var_screeninfo var;

	int size;
	void *mmap;

	char *file;
	/* position */
	int x, y;
	/* format */
	fb_format_t format;
} fb_t;

/* the order how the different planes are registered */
typedef enum
{
	FB_OSD0 = DAVINCIFB_WIN_OSD0,
	FB_VID0 = DAVINCIFB_WIN_VID0,
	FB_OSD1 = DAVINCIFB_WIN_OSD1,
	FB_VID1 = DAVINCIFB_WIN_VID1,
	FBS = DAVINCIFB_WINDOWS,
} fb_plane_t;

typedef enum
{
	FB_COMPONENT = DAVINCIFB_OUT_COMPONENT,
	FB_COMPOSITE = DAVINCIFB_OUT_COMPOSITE,
	FB_SVIDEO = DAVINCIFB_OUT_SVIDEO,
} fb_vout_t;

static char *fb_names[] = {
	[FB_OSD0] = "dm_osd0_fb",
	[FB_OSD1] = "dm_osd1_fb",
	[FB_VID0] = "dm_vid0_fb",
	[FB_VID1] = "dm_vid1_fb",
};

typedef struct fb_im_t
{
	fb_format_t format;
	char *fname;
	char *buf;
	int w;
	int h;
} fb_im_t;

#define FB_IMAGES_NUM 2

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
/*============================================================================*
 *                               Test Suite                                   *
 *============================================================================*/
static void help(void)
{
	printf("fbtest <FB> <TEST> ...\n");
	printf("Where TEST can be one of the following:\n");
	printf("output <MODE> : Displays a picture on device FB with mode MODE (string)\n");
	printf("                MODE is formed as [xres yres [vxres vyres]]\n");
	printf("                If no mode is set, use current mode\n");
	printf("enable <ON>   : Enables (1) or disables (0) the FB (int)\n");
	printf("posx <X>      : Sets the X position of the FB (int)\n");
	printf("posy <Y>      : Sets the X position of the FB (int)\n");
	printf("transp <ON>   : Enables the transparency on the FB (int)\n");
	printf("trcol <color> : Sets the transparent color value (short int)\n");
	printf("vout <OUTPUT> : Sets the venc output\n");
	printf("FB can be osd0, osd1, vid0, vid1\n");
	printf("OUTPUT can be composite %d, component %d, svideo %d\n",
		FB_COMPOSITE, FB_COMPONENT, FB_SVIDEO);
	printf("\n");
}


static void test_output(fb_t *fb, int w, int h, int vw, int vh, int set)
{
	printf("Running output test\n");
	if (set)
	{
		printf("setting to w = %d h = %d vw = %d vh = %d\n", w, h, vw, vh);
		fb_size_set(fb, w, h, vw, vh);
	}
	fb_image_draw(fb);
}

static void test_enable(fb_t *fb, int on)
{
	printf("Running enable test\n");
	fb_enable(fb, on);
}

static void test_posx(fb_t *fb, int x)
{
	printf("Running posx test\n");
	fb_x_set(fb, x);
}

static void test_posy(fb_t *fb, int y)
{
	printf("Running posy test\n");
	fb_y_set(fb, y);
}

static void test_vout(fb_t *fb, fb_vout_t out)
{
	FILE *f;

	printf("running vout test %d\n", out);

	/* open the video output sysfs file */
	f = fopen("/sys/class/video_output/venc/state", "a");
	if (!f)
	{
		printf("Error opening the file\n");
		return;
	}
	fprintf(f, "%d", out);
	fclose(f);
}

static void test_transp(fb_t *fb, int on)
{
	printf("Running transparency test %d\n", on);
	fb_transp_set(fb, on);
}

static void test_trcolor(fb_t *fb, int color)
{
	int i, j;
	unsigned short int *fb_tmp;

	printf("Running transparency color test %x\n", color);
	fb_transp_solor_set(fb, color);
	if ((fb->plane != FB_OSD0) && (fb->plane != FB_OSD1))
		return;
	/* draw a rectangle of that size with that color */
	if (color > 0xff)
		color = 0xff;
	fb_tmp = fb->mmap;
	for (i = 0; i < 200; i++)
	{
		for (j = 0; j < 200; j++)
		{
			//printf("old value = %x\n", *fb_tmp2);
			*(fb_tmp + j) = color;
		}
		(unsigned char *)fb_tmp += fb->fix.line_length;
	}
}

int main(int argc, char **argv)
{
	fb_t *fb;
	char fbname[256];

	/* parse the options */
	if (argc < 3)
	{
		help();
		return 1;
	}
	sprintf(fbname, "dm_%s_fb", argv[1]);
	printf("ok1\n");
	fb = fb_new(fbname);
	printf("ok2\n");
	if (!fb)
		return 2;
	fb_image_load();
	/* output test */
	if (!strcmp(argv[2], "output"))
	{
		int w, h, vh, vw, set = 0;

		if (argc < 4)
			goto run_test;
		/* parse the mode */
		if (argc == 4)
		{
			help();
			return 5;
		}
		set = 1;
		w = strtoul(argv[3], NULL, 10);
		h = strtoul(argv[4], NULL, 10);
		if (argc > 5)
			vw = strtoul(argv[5], NULL, 10);
		else
			vw = w;
		if (argc > 6)
			vh = strtoul(argv[6], NULL, 10);
		else
			vh = h;
run_test:
		test_output(fb, w, h, vw, vh, set);
	}
	/* enable */
	else if (!strcmp(argv[2], "enable"))
	{
		if (argc < 4)
		{
			help();
			return 5;
		}
		test_enable(fb, strtoul(argv[3], NULL, 10));
	}
	/* posx */
	else if (!strcmp(argv[2], "posx"))
	{
		if (argc < 4)
		{
			help();
			return 5;
		}
		test_posx(fb, strtoul(argv[3], NULL, 10));
	}
	/* posy */
	else if (!strcmp(argv[2], "posy"))
	{
		if (argc < 4)
		{
			help();
			return 5;
		}
		test_posy(fb, strtoul(argv[3], NULL, 10));
	}
	/* enable transparency */
	else if (!strcmp(argv[2], "transp"))
	{
		if (argc < 4)
		{
			help();
			return 5;
		}
		test_transp(fb, strtoul(argv[3], NULL, 10));
	}
	/* enable transparency */
	else if (!strcmp(argv[2], "trcolor"))
	{
		if (argc < 4)
		{
			help();
			return 5;
		}
		test_trcolor(fb, strtoul(argv[3], NULL, 10));
	}
	/* enable transparency */
	else if (!strcmp(argv[2], "vout"))
	{
		if (argc < 4)
		{
			help();
			return 5;
		}
		test_vout(fb, strtoul(argv[3], NULL, 10));
	}
	else
	{
		help();
		return 6;
	}
	fb_delete(fb);
	return 1;
}


