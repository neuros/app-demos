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

#define VIDEO_BUFFERS_COUNT 6

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

		wait_4_vsync(fb->fd);
		/* FIXME this is very harcoded, we need to check out the
 		 * format image and fb format */
		/* send the captured image to the fb */
		for (i = 0; i < lines; i++)
		{
			memcpy((unsigned char *)fb->mmap + (fb->fix.line_length * i), \
				   (unsigned char *)v->buffers[buffer.index].start + (format.fmt.pix.bytesperline * i), \
				   format.fmt.pix.bytesperline);
		}
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
	video_close(v);
	fb_delete(fb);
	return 0;
}


