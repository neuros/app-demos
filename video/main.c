// UGLY HACK to bring in resizer support to remove the CPU memcpy overhead
#define RESZ_HACK

#ifdef RESZ_HACK
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>

#define ALWAYSERR

#include <asm/arch/davinci_resizer.h>
#include "rszcopy.h"

/* Enables or disables debug output */
#ifdef __DEBUG 
#define DBG(fmt, args...) fprintf(stderr, "Rszcopy Debug: " fmt, ## args)
#else
#define DBG(fmt, args...)
#endif

#if (defined __DEBUG) || (defined ALWAYSERR)
#define ERR(fmt, args...) fprintf(stderr, "Rszcopy Error: " fmt, ## args)
#else
#define ERR(fmt, args...)
#endif

#define RESIZER_DEVICE "/dev/davinci_resizer"
#define FIR_RND_SCALE  256
#define NUM_COEFS      32
#define SCREEN_BPP     16

static Rszcopy_Handle hRszcopy = RSZCOPY_FAILURE;

/******************************************************************************
 * inSizeCalc
 ******************************************************************************/
static int inSizeCalc(int inSize)
{
    int rsz;
    int in = inSize;

    while (1) {
        rsz = ((in - 4) * 256 - 16) / (inSize - 1);

        if (rsz < 256) {
            if (in < inSize) {
                break;
            }
            in++;
        }
        else if (rsz > 256) {
            if (in > inSize) {
                break;
            }
            in--;
        }
        else {
            break;
        }
    }

    return in;
}

/******************************************************************************
 * Rszcopy_config
 ******************************************************************************/
int Rszcopy_config(Rszcopy_Handle hRszcopy,
                   int width, int height, int srcPitch, int dstPitch)
{
    rsz_params_t  params = {
        0,                              /* in_hsize (set at run time) */
        0,                              /* in_vsize (set at run time) */
        0,                              /* in_pitch (set at run time) */
        RSZ_INTYPE_YCBCR422_16BIT,      /* inptyp */
        0,                              /* vert_starting_pixel */
        0,                              /* horz_starting_pixel */
        0,                              /* cbilin */
        RSZ_PIX_FMT_UYVY,               /* pix_fmt */
        0,                              /* out_hsize (set at run time) */
        0,                              /* out_vsize (set at run time) */
        0,                              /* out_pitch (set at run time) */
        0,                              /* hstph */
        0,                              /* vstph */
        {                               /* hfilt_coeffs */
            256, 0, 0, 0, 256, 0, 0, 0, 256, 0, 0, 0, 256, 0, 0, 0,
            256, 0, 0, 0, 256, 0, 0, 0, 256, 0, 0, 0, 256, 0, 0, 0
        },
        {                               /* vfilt_coeffs */
            256, 0, 0, 0, 256, 0, 0, 0, 256, 0, 0, 0, 256, 0, 0, 0,
            256, 0, 0, 0, 256, 0, 0, 0, 256, 0, 0, 0, 256, 0, 0, 0
        },
        {                               /* yenh_params */
            RSZ_YENH_DISABLE,               /* type */
            0,                              /* gain */
            0,                              /* slop */
            0                               /* core */
        }
    };

    DBG("Configuring resizer job to copy image of resolution %dx%d\n",
        width, height);

    /* Set up the rszcopyr job */
    params.in_hsize  = inSizeCalc(width);
    params.in_vsize  = inSizeCalc(height);
    params.in_pitch  = srcPitch;
    params.out_hsize = width;
    params.out_vsize = height;
    params.out_pitch = dstPitch;

    hRszcopy->inSize = srcPitch * params.in_vsize;
    hRszcopy->outSize = dstPitch * params.out_vsize;

    if (ioctl(hRszcopy->rszFd, RSZ_S_PARAM, &params) == -1) {
        ERR("Rszcopy setting parameters failed.\n");
        return RSZCOPY_FAILURE;
    }

    return RSZCOPY_SUCCESS;
}

/******************************************************************************
 * Rszcopy_create
 ******************************************************************************/
Rszcopy_Handle Rszcopy_create(int rszSpeed)
{
    Rszcopy_Handle hRszcopy;

    /* Allocate the rszcopy object */
    hRszcopy = malloc(sizeof(Rszcopy_Object));

    if (hRszcopy == NULL) {
        ERR("Failed to allocate memory space for handle.\n");
        return RSZCOPY_FAILURE;
    }

    /* Open rszcopyr device */
    hRszcopy->rszFd = open(RESIZER_DEVICE, O_RDWR);

    if (hRszcopy->rszFd == -1) {
        ERR("Failed to open %s\n", RESIZER_DEVICE);
        free(hRszcopy);
        return RSZCOPY_FAILURE;
    }

    if (rszSpeed >= 0) {
        if (ioctl(hRszcopy->rszFd, RSZ_S_EXP, &rszSpeed) == -1) {
            ERR("Error calling RSZ_S_EXP on resizer.\n");
            free(hRszcopy);
            return RSZCOPY_FAILURE;
        }
    }

    return hRszcopy;
}

/******************************************************************************
 * Rszcopy_execute
 ******************************************************************************/
int Rszcopy_execute(Rszcopy_Handle hRszcopy, unsigned long srcBuf,
                    unsigned long dstBuf)
{
    rsz_resize_t rsz;
    int          rszError;

    if (dstBuf % 32) {
        ERR("Destination buffer not aligned on 32 bytes\n");
        return RSZCOPY_FAILURE;
    }

    if (srcBuf % 32) {
        ERR("Source buffer not aligned on 32 bytes\n");
        return RSZCOPY_FAILURE;
    }

    rsz.in_buf.index     = -1;
    rsz.in_buf.buf_type  = RSZ_BUF_IN;
    rsz.in_buf.offset    = srcBuf;
    rsz.in_buf.size      = hRszcopy->inSize;

    rsz.out_buf.index    = -1;
    rsz.out_buf.buf_type = RSZ_BUF_OUT;
    rsz.out_buf.offset   = dstBuf;
    rsz.out_buf.size     = hRszcopy->outSize;

    do {
        rszError = ioctl(hRszcopy->rszFd, RSZ_RESIZE, &rsz);
    } while (rszError == -1 && errno == EAGAIN);

    if (rszError == -1) {
        ERR("Failed to execute resize job\n");
        return RSZCOPY_FAILURE;
    }

    return RSZCOPY_SUCCESS;
}

/******************************************************************************
 * Rszcopy_delete
 ******************************************************************************/
void Rszcopy_delete(Rszcopy_Handle hRszcopy)
{
    if (hRszcopy != NULL) {
        close(hRszcopy->rszFd);
        free(hRszcopy);
    }
}
#endif //end of RESZ_HACK

#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <linux/videodev2.h>
#include <media/davinci_vpfe.h>

#include "fblib.h"

#define VIDEO_BUFFERS_COUNT 3

const char *fb_names[] = {
	[FB_OSD0] = "dm_osd0_fb",
	[FB_VID0] = "dm_vid0_fb",
	[FB_VID1] = "dm_vid1_fb",
	[FB_OSD1] = "dm_osd1_fb",
};

int run = 1;
char fb_device[256] = "dm_vid1_fb";
unsigned int input = 0;


typedef struct video_buffer_t
{
	void *start;
	void * phys; // Let's query V4L2's physical address for RESZ DMA
	int index;
	ssize_t length;
} video_buffer_t;

typedef enum
{
	VIDEO_RW        = 1,
	VIDEO_ASYNCIO   = 2,
	VIDEO_STREAMING = 4,
} video_io_t;

typedef struct video_t
{
	int fd;
	char *file;
        struct v4l2_capability cap;
	struct v4l2_requestbuffers req;
	video_buffer_t buffers[VIDEO_BUFFERS_COUNT];
	video_io_t io;
	int w;
	int h;
} video_t;

void video_dump(video_t *v)
{
	printf("video device (%s)\n", v->file);
	printf("  driver = %s\n", v->cap.driver);
	printf("  card   = %s\n", v->cap.card);
	printf("  caps   = ");
	if (v->cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)
		printf("CAPTURE ");
	if (v->cap.capabilities & V4L2_CAP_VIDEO_OUTPUT)
		printf("OUTPUT ");
	printf("\n");
	printf("  rwmode = ");
	if (v->cap.capabilities & V4L2_CAP_READWRITE)
	{
		printf("RW ");
		v->io |= VIDEO_RW;
	}
	if (v->cap.capabilities & V4L2_CAP_ASYNCIO)
	{
		printf("ASYNCIO ");
		v->io |= VIDEO_ASYNCIO;
	}
	if (v->cap.capabilities & V4L2_CAP_STREAMING)
	{
		printf("STREAMING ");
		v->io |= VIDEO_STREAMING;
	}
	printf("\n");
}

int video_reqbuf(video_t *v)
{
	int ret;
	int i;
	struct v4l2_buffer buffer;

	v->req.count = VIDEO_BUFFERS_COUNT;
	v->req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	v->req.memory = V4L2_MEMORY_MMAP;

	/* request the buffers */
	ret = ioctl (v->fd, VIDIOC_REQBUFS, &v->req);
	if (ret < 0)
	{
                printf ("Request buffers error\n");
	}
	if (v->req.count < VIDEO_BUFFERS_COUNT)
	{
		printf("Not enough memory buffers (%d)\n", v->req.count);
	}
	/* mmap the buffers */
	for (i = 0; i < v->req.count; i++)
	{
		buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buffer.memory = V4L2_MEMORY_MMAP;
		buffer.index = i;

		if (ioctl(v->fd, VIDIOC_QUERYBUF, &buffer) < 0)
		{
            		printf("Failed QUERYBUF on %d buffer\n", i);
			return -ENOMEM;
		}
        	v->buffers[i].index = i;
		v->buffers[i].length = buffer.length;
		v->buffers[i].start = mmap (NULL, buffer.length,
                                 PROT_READ | PROT_WRITE, /* recommended */
                                 MAP_SHARED,             /* recommended */
                                 v->fd, buffer.m.offset);

		v->buffers[i].phys = buffer.m.offset;
		//printf("V4L2 buffer physical address: %p\n", buffer.m.offset);

		if (MAP_FAILED == v->buffers[i].start)
			return -ENOMEM;
		/* queue the buffer */
		if (ioctl(v->fd, VIDIOC_QBUF, &buffer) < 0)
		{
			printf("Failed QBUF\n");
			return -EINVAL;
		}
	}
	return ret;
}

video_t * video_open(const char *name)
{
	int fd;
	struct v4l2_input vinput;
	video_t *v;

	fd = open("/dev/video0", O_RDWR, O_NONBLOCK);
	if (!fd)
	{
		printf("Error opening device %s\n", name);
		return NULL;
	}
	v = calloc(1, sizeof(video_t));
	v->file = strdup(name);
	v->fd = fd;
	/* enumarate inputs */
	vinput.index = 0;
	while (ioctl(fd, VIDIOC_ENUMINPUT, &vinput) != -1)
	{
		vinput.index++;
	}
	if (!vinput.index)
	{
		printf("Couldn't enumerate inputs\n");
	}
	printf("Enumerated %d inputs\n", vinput.index);
	/* get device capabilities */
	if (ioctl(fd, VIDIOC_QUERYCAP, &v->cap) < 0)
	{
		goto err_query_cap;
	}
	/* set the capture image format */
	/* mmap the buffers */
	if (video_reqbuf(v) < 0)
		goto err_reqbuf;

	video_dump(v);
	return v;

err_reqbuf:
err_query_cap:
	close(v->fd);
	free(v->file);
	free(v);
	return NULL;
}

void video_close(video_t *v)
{
	int i;

	/* unmap all buffers */
	for (i = 0; i < VIDEO_BUFFERS_COUNT; i++)
		munmap(v->buffers[i].start, v->buffers[i].length);
	close(v->fd);
	free(v->file);
	free(v);
}

int video_start(video_t *v)
{
	int ret;

	ret = ioctl(v->fd, VIDIOC_STREAMON, &v->req.type);
	if (ret < 0)
	{
		perror("Stream start failed");
	}
	return ret;
}

void video_stop(video_t *v)
{
	if (ioctl(v->fd, VIDIOC_STREAMOFF, &v->req.type) < 0)
	{
		perror("Stream stop failed");
	}
}
/* hide all fb windows, to make only the selected fb visible */
void video_hide(fb_t *fb)
{
	unsigned int i;

	for (i = 0; i < FBS; i++)
	{
		if (i == fb->plane)
			continue;
		else
		{
			fb_t *fbtmp;

			fbtmp = fb_new(fb_names[i]);
			fb_enable(fbtmp, 0);
			fb_delete(fbtmp);
		}
	}
}

/* show all fb windows, return all windows to initial state */
void video_show(fb_t *fb)
{
	unsigned int i;

	for (i = 0; i < FBS; i++)
	{
		if (i == fb->plane)
			continue;
		else
		{
			fb_t *fbtmp;

			fbtmp = fb_new(fb_names[i]);
			fb_enable(fbtmp, 1);
			fb_delete(fbtmp);
		}
	}
}

void abort_signal(int signal)
{
	printf("called\n");
	run = 0;
}

void help(void)
{
	run = 0;
}

static int wait_4_vsync(int fd)
{
    int dummy;

    /* Wait for vertical sync */
    if (ioctl(fd, FBIO_WAITFORVSYNC, &dummy) == -1)
	{
        printf("Failed FBIO_WAITFORVSYNC.\n");
        return -1;
    }

    return 0;
}

#if 0
#include "sys/time.h"
static unsigned int t1;
static unsigned int get_time_ms(void)
{
	struct timeval tm;

	gettimeofday(&tm, NULL);
	tm.tv_sec %= 1000;
	return (tm.tv_sec * 1000 + tm.tv_usec / 1000);
}
#endif

int main(int argc, char **argv)
{
	video_t *v;
	fb_t *fb;
	struct v4l2_format format;
	unsigned int lines;
	v4l2_std_id std_id = V4L2_STD_NTSC;
	char *std = NULL;

	if (argc > 1)
	{
		snprintf(fb_device, 256, "dm_%s_fb", argv[1]);
	}
	if (argc > 2)
	{
		input = atoi(argv[2]);
	}
	if (argc > 2)
	{
		std = argv[3];
	}
	printf("Using %s as fb device and input number %d\n", fb_device, input);
	/* open up the device */
	v = video_open("/dev/video0");
	fb =  fb_new(fb_device);
	if (!fb)
		return 2;
	/* register the signal handler */
	if (signal(SIGINT, abort_signal) == SIG_ERR)
	{
		printf("Can't register signal handler\n");
		return 3;
	}

	/* set input */
	if (ioctl(v->fd, VIDIOC_S_INPUT, &input) < 0)
	{
		printf("Set input failed\n");
		return 4;
	}
	if (std != NULL)
	{
		if (!strcmp(std, "ntsc") && input != VPFE_AMUX_COMPONENT)
		{
			std_id = V4L2_STD_NTSC;
		}
		else if (!strcmp(std, "pal") && input != VPFE_AMUX_COMPONENT)
		{
			std_id = V4L2_STD_PAL;
		}
		else if (!strcmp(std, "480p") && input == VPFE_AMUX_COMPONENT)
		{
			std_id = V4L2_STD_HD_480P;
		}
		else if (!strcmp(std, "576p") && input == VPFE_AMUX_COMPONENT)
		{
			std_id = V4L2_STD_HD_576P;
		}
		else if (!strcmp(std, "720p") && input == VPFE_AMUX_COMPONENT)
		{
			std_id = V4L2_STD_HD_720P;
		}
		else if (!strcmp(std, "1080i") && input == VPFE_AMUX_COMPONENT)
		{
			std_id = V4L2_STD_HD_1080I;
		}
	}
	printf("Setting standard %d\n", std_id);
	if (ioctl(v->fd, VIDIOC_S_STD, &std_id) < 0)
	{
		printf("Set std failed\n");
		return 6;
	}
	/* TODO get stds */
	/* TODO set fmt image */
	/* get fmt image */
	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(v->fd, VIDIOC_G_FMT, &format) < 0)
	{
		printf("Get format failed\n");
		return 5;
	}
	lines = format.fmt.pix.height > fb->var.yres ? fb->var.yres : format.fmt.pix.height;
	/* hide all planes but the selected */
	video_hide(fb);
	/* capture */
	if (video_start(v) < 0)
		goto err;
	fb_mmap(fb);
	while (run)
	{
		int ret;
		int i;
		struct v4l2_buffer buffer;

		buffer.type = v->req.type;
		buffer.memory = V4L2_MEMORY_MMAP;

		ret = ioctl(v->fd, VIDIOC_DQBUF, &buffer);
		if (ret < 0)
		{
			if (ret == -EINVAL)
				continue;
			perror("Error dequeueing");
			break;
		}

		//t1 = get_time_ms();
		wait_4_vsync(fb->fd);
		//printf("delta = %u\n", get_time_ms() - t1);
#ifndef RESZ_HACK
		/* FIXME this is very harcoded, we need to check out the
 		 * format image and fb format */
		/* send the captured image to the fb */
		for (i = 0; i < lines; i++)
		{
			memcpy((unsigned char *)fb->mmap + (fb->fix.line_length * i), \
				   (unsigned char *)v->buffers[buffer.index].start + (format.fmt.pix.bytesperline * i), \
				   format.fmt.pix.bytesperline);

		}
#else   //RESZ_HACK
		{
			if (RSZCOPY_FAILURE == hRszcopy)
			{
				/* Create the resize job */
				hRszcopy = Rszcopy_create(RSZCOPY_DEFAULTRSZRATE);
			}

			if (hRszcopy == RSZCOPY_FAILURE)
			{
				ERR("Failed to create resize copy job\n");
				goto err;
			}

			// FIXME: we do not actually resize, DMA only.
			if (Rszcopy_config(hRszcopy,
							   format.fmt.pix.bytesperline/2,
							   lines,
                               format.fmt.pix.bytesperline,
							   format.fmt.pix.bytesperline) == RSZCOPY_FAILURE)
			{
				ERR("Failed to configure resize job\n");
				goto err;
			}

			/* Copy frame to frame buffer using the H/W resizer */
			if (Rszcopy_execute(hRszcopy, v->buffers[buffer.index].phys,
								fb->phys) == RSZCOPY_FAILURE)
			{
				ERR("Frame copy using resizer failed\n");
				goto err;
			}
		}
#endif // end of RESZ_HACK

		wait_4_vsync(fb->fd);

		/* send the captured frame back to the queue */
		ret = ioctl(v->fd, VIDIOC_QBUF, &buffer);
		if (ret < 0)
		{
			if (ret == -EINVAL)
				continue;
			perror("Error queueing");
			break;
		}
	}
	/* show all planes back */
	video_show(fb);
	video_stop(v);
err:
#ifdef RESZ_HACK
	if (RSZCOPY_FAILURE != hRszcopy) Rszcopy_delete(hRszcopy);
#endif
	video_close(v);
	fb_delete(fb);
	return 0;
}


