#ifndef _FB_LIB_H
#define _FB_LIB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <linux/fb.h>
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

typedef struct fb_im_t
{
	fb_format_t format;
	char *fname;
	char *buf;
	int w;
	int h;
} fb_im_t;

#define FB_IMAGES_NUM 2

extern int fb_info_get(fb_t *fb);
extern int fb_mmap(fb_t *fb);
extern void fb_dump(fb_t *fb);
extern void fb_delete(fb_t *fb);
extern fb_t * fb_new(const char *name);
extern void fb_transp_set(fb_t *fb, int on, int level);
extern void fb_transp_solor_set(fb_t *fb, int color);
extern void fb_y_set(fb_t *fb, int y);
extern void fb_x_set(fb_t *fb, int x);
extern void fb_bpp_set(fb_t *fb, int bpp, int attr);
extern void fb_size_set(fb_t *fb, int w, int h, int vw, int vh);
extern void fb_image_draw(fb_t *fb);
extern void fb_enable(fb_t *fb, unsigned int on);
extern void fb_image_load(void);
extern void fb_cbtest_set(fb_t *fb, int on);

#endif
