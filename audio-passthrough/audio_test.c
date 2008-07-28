#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/soundcard.h>
#include <signal.h>

/* The number of channels of the audio codec */
#define NUM_CHANNELS           2

/* The gain (0-100) of the left and right channels */
#define LEFT_GAIN              100
#define RIGHT_GAIN             100

/* Number of samples to process at once */
#define NUMSAMPLES             80

/* Mono 16 bit */
#define RAWBUFSIZE             NUMSAMPLES * 2

/* Stereo 16 bit */
#define INPUTBUFSIZE           RAWBUFSIZE * 2
#define SOUND_DEVICE    "/dev/dsp"
#define MIXER_DEVICE    "/dev/mixer"


int input_fd  = -1;
int output_fd = -1;
char *recsrc = NULL;
int sampleRate=16000;

void close_sound_device(void)
{
    if (output_fd > 0)
        close(output_fd);
    if (input_fd > 0)
        close(input_fd);

    input_fd  = -1;
    output_fd = -1;
}

void signal_handler(int signo)
{
    printf("Audio passthrough test finished.\n");
    close_sound_device();

    exit(0);
}


int main(int argc, char** argv)
{
    int     vol        = LEFT_GAIN | (RIGHT_GAIN << 8);
    int     channels   = NUM_CHANNELS;
    int     format     = AFMT_S16_LE;

    int mixer_fd  = -1;
	mixer_info info;
	int     recMask;
    int		i, src;
    oss_mixer_enuminfo ei;

    int c;
    extern char *optarg;
    unsigned short inputBuf[INPUTBUFSIZE];
    int numOfBytes;

    while ((c = getopt (argc, argv, "s:i:")) != EOF)
    {
        switch (c)
        {
        case 's':
			sampleRate = atoi(optarg);
            break;
        case 'i':
            recsrc = optarg;
            break;
        }
    }

    signal(SIGINT, signal_handler);

    /* open mixer device */
    mixer_fd = open(MIXER_DEVICE, O_RDONLY);
    if (mixer_fd < 0)
    {
        printf("Failed to open %s\n", MIXER_DEVICE);
        return -1;
    }

	/* set line input */
    if (ioctl(mixer_fd, SOUND_MIXER_INFO, &info) == -1) {
        printf("Failed to get mixer infomation\n");
        close(mixer_fd);
        return -1;
    }

    if (ioctl(mixer_fd, SOUND_MIXER_READ_RECMASK, &recMask) == -1) {
        printf("Failed to ask mixer for available recorders.\n");
        close(mixer_fd);
        return -1;
    }

	src = SOUND_MASK_LINE;
    if ((recMask & src) == 0) {
        printf("Recorder not supported\n");
        close(mixer_fd);
        return -1;
    }

    if (ioctl(mixer_fd, SOUND_MIXER_WRITE_RECSRC, &src) == -1) {
        printf("Failed to set recorder.\n");
        close(mixer_fd);
        return -1;
    }

    /* set volume */
    if (ioctl(mixer_fd, SOUND_MIXER_WRITE_IGAIN, &vol) == -1)
    {
        printf("Failed to set the volume of line in.\n");
        close(mixer_fd);
        return -1;
    }
    if (ioctl(mixer_fd, SOUND_MIXER_WRITE_VOLUME, &vol) == -1)
    {
        printf("Failed to set the volume of line in.\n");
        close(mixer_fd);
        return -1;
    }

    /* select record source */
    if (ioctl (mixer_fd, SNDCTL_DSP_GET_RECSRC_NAMES, &ei) == -1)
    {
        printf("SNDCTL_DSP_GET_RECSRC_NAMES failed\n");
        close(mixer_fd);
        return -1;
    }
	
    printf("Possible recording sources for the selected device:\n\n");
	for (i = 0; i < ei.nvalues; i++)
	{
		printf("\t%s\n", ei.strings + ei.strindex[i]);
	}
	printf("\n");

	if(recsrc == NULL)
	{
		printf("Usage: %s -s<speed> -i<record source>\n", argv[0]);
		close(mixer_fd);
		return 0;
	}

    src = -1;
    for (i = 0; i < ei.nvalues; i++)
    {
        if (strcmp (recsrc, ei.strings + ei.strindex[i]) == 0)
        {
            src = i;
            break;
        }
    }

    if (src < 0 || src > ei.nvalues)
    {
        printf("invalid record source: %s\n",recsrc);
    }
    else
    {
        if (ioctl (mixer_fd, SNDCTL_DSP_SET_RECSRC, &src) == -1)
        {
            printf("SNDCTL_DSP_SET_RECSRC failed\n");
        	close(mixer_fd);
            return -1;
        }
    }
    close(mixer_fd);




    /* Open the sound device for writing */
    input_fd = open(SOUND_DEVICE, O_RDONLY);
    if (input_fd < 0)
    {
        printf("Failed to open the sound device (%s)\n", SOUND_DEVICE);
		close_sound_device();
        return -1;
    }


    /* Set the sound format (only AFMT_S16_LE supported) */
    if (ioctl(input_fd, SNDCTL_DSP_SETFMT, &format) == -1)
    {
        printf("Could not set format %d\n", format);
		close_sound_device();
        return -1;
    }

    /* Set the number of channels */
    if (ioctl(input_fd, SNDCTL_DSP_CHANNELS, &channels) == -1)
    {
        printf("Could not set mixer to %d channels\n", channels);
		close_sound_device();
        return -1;
    }

    /* Set the sample rate */
    if (ioctl(input_fd, SNDCTL_DSP_SPEED, &sampleRate) == -1)
    {
        printf("Could not set sample rate (%d)\n", sampleRate);
		close_sound_device();
        return -1;
    }






    /* Open the sound device for writing */

    output_fd = open(SOUND_DEVICE, O_WRONLY);
    if (output_fd == -1)
    {
        printf("Failed to open the sound device (%s)\n", SOUND_DEVICE);
		close_sound_device();
        return -1;
    }
    /* Set the sound format (only AFMT_S16_LE supported) */
    if (ioctl(output_fd, SNDCTL_DSP_SETFMT, &format) == -1)
    {
        printf("Could not set format %d\n", format);
		close_sound_device();
        return -1;
    }

    /* Set the number of channels */
    if (ioctl(output_fd, SNDCTL_DSP_CHANNELS, &channels) == -1)
    {
        printf("Could not set mixer to %d channels\n", channels);
		close_sound_device();
        return -1;
    }

    /* Set the sample rate */
    if (ioctl(output_fd, SNDCTL_DSP_SPEED, &sampleRate) == -1)
    {
        printf("Could not set sample rate (%d)\n", sampleRate);
		close_sound_device();
        return -1;
    }

    printf("Press Ctrl + C to quit.\n");

    while (1)
    {
        numOfBytes = read(input_fd, inputBuf, INPUTBUFSIZE);
        if (numOfBytes == -1)
        {
            printf("Error reading the data from speech file\n");
			break;
        }
        write(output_fd, inputBuf, numOfBytes);
    }
	close_sound_device();

    return 0;
}
