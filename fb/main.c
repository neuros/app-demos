#include "fblib.h"

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
	printf("cbmode <ON>   : Enables Color Bar test mode (int)\n");
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
static void test_cbmode(fb_t *fb, int on)
{
	printf("Runnning color bar test mode %d\n", on);
	fb_cbtest_set(fb, on);
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
		fb_tmp += fb->fix.line_length;
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
	/* enable color bar mode */
	else if (!strcmp(argv[2], "cbmode"))
	{
		if (argc < 4)
		{
			help();
			return 5;
		}
		test_cbmode(fb, strtoul(argv[3], NULL, 10));
	}
	else
	{
		help();
		return 6;
	}
	fb_delete(fb);
	return 1;
}


