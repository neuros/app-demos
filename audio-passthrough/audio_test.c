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

#define __DEBUG
#ifdef __DEBUG
#define ERR(x...) printf(x)
#else
#define ERR(x...)
#endif
#define FAILURE -1
#define SUCCESS 0
/* The number of channels of the audio codec */
#define NUM_CHANNELS           2

/* The sample rate of the audio codec */
#define SAMPLE_RATE            8000

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

int soundFd = -1;
int outputFd = -1;

/* Whether to use 'mic in' or 'line in' for sound input */
typedef enum SoundInput {
    MIC_SOUND_INPUT,
    LINEIN_SOUND_INPUT
} SoundInput;

void signal_handler(int signo)
{
	printf("Audio passthrough test finished.\n");
	if(soundFd > 0)
		close(soundFd);
	if(outputFd > 0)
		close(outputFd);

	exit(0);
}

static int initOutputSoundDevice(void)
{
    int     vol         = LEFT_GAIN | (RIGHT_GAIN << 8);
    int     channels    = NUM_CHANNELS;
    int     sampleRate  = SAMPLE_RATE;
    int     format      = AFMT_S16_LE;
    int     soundFd;
    int     mixerFd;

    /* Set the output volume */
    mixerFd = open(MIXER_DEVICE, O_RDONLY);

    if (mixerFd == -1) {
        ERR("Failed to open %s\n", MIXER_DEVICE);
        return FAILURE;
    }

    if (ioctl(mixerFd, SOUND_MIXER_WRITE_VOLUME, &vol) == -1) {
        ERR("Failed to set the volume of line in.\n");
        close(mixerFd);
        return FAILURE;
    }

    close(mixerFd);

    /* Open the sound device for writing */
    soundFd = open(SOUND_DEVICE, O_WRONLY);

    if (soundFd == -1) {
        ERR("Failed to open the sound device (%s)\n", SOUND_DEVICE);
        return FAILURE;
    }

    /* Set the sound format (only AFMT_S16_LE supported) */
    if (ioctl(soundFd, SNDCTL_DSP_SETFMT, &format) == -1) {
        ERR("Could not set format %d\n", format);
        return FAILURE;
    }

    /* Set the number of channels */
    if (ioctl(soundFd, SNDCTL_DSP_CHANNELS, &channels) == -1) {
        ERR("Could not set mixer to %d channels\n", channels);
        return FAILURE;
    }

    /* Set the sample rate */
    if (ioctl(soundFd, SNDCTL_DSP_SPEED, &sampleRate) == -1) {
        ERR("Could not set sample rate (%d)\n", sampleRate);
        return FAILURE;
    }

    return soundFd;
}


static int  initSoundDevice(SoundInput soundInput)
{
    int     vol        = LEFT_GAIN | (RIGHT_GAIN << 8);
    int     sampleRate = SAMPLE_RATE;
    int     channels   = NUM_CHANNELS;
    int     format     = AFMT_S16_LE;
    int     soundFd;
    int     mixerFd;
    int     recMask;
    int     recorder;

    if (soundInput == MIC_SOUND_INPUT) {
        printf("Microphone recorder selected\n");
        recorder = SOUND_MASK_MIC;
    }
    else {
        printf("Line in recorder selected\n");
        recorder = SOUND_MASK_LINE;
    }

    /* Select the right capture device and volume */
    mixerFd = open(MIXER_DEVICE, O_RDONLY);

    if (mixerFd == -1) {
        ERR("Failed to open %s\n", MIXER_DEVICE);
        return FAILURE;
    }

    if (ioctl(mixerFd, SOUND_MIXER_READ_RECMASK, &recMask) == -1) {
        ERR("Failed to ask mixer for available recorders.\n");
        return FAILURE;
    }

    if ((recMask & recorder) == 0) {
        ERR("Recorder not supported\n");
        return FAILURE;
    }

    if (ioctl(mixerFd, SOUND_MIXER_WRITE_RECSRC, &recorder) == -1) {
        ERR("Failed to set recorder.\n");
        return FAILURE;
    }

    if (ioctl(mixerFd, SOUND_MIXER_WRITE_IGAIN, &vol) == -1) {
        ERR("Failed to set the volume of line in.\n");
        return FAILURE;
    }

    close(mixerFd);

    /* Open the sound device for writing */
    soundFd = open(SOUND_DEVICE, O_RDONLY);

    if (soundFd == -1) {
        ERR("Failed to open the sound device (%s)\n", SOUND_DEVICE);
        return FAILURE;
    }

    /* Set the sound format (only AFMT_S16_LE supported) */
    if (ioctl(soundFd, SNDCTL_DSP_SETFMT, &format) == -1) {
        ERR("Could not set format %d\n", format);
        return FAILURE;
    }

    /* Set the number of channels */
    if (ioctl(soundFd, SNDCTL_DSP_CHANNELS, &channels) == -1) {
        ERR("Could not set mixer to %d channels\n", channels);
        return FAILURE;
    }

    /* Set the sample rate */
    if (ioctl(soundFd, SNDCTL_DSP_SPEED, &sampleRate) == -1) {
        ERR("Could not set sample rate (%d)\n", sampleRate);
        return FAILURE;
    }

    return soundFd;
}

int main(int argc, char** argv)
{
	unsigned short inputBuf[INPUTBUFSIZE];
	int numOfBytes;

	signal(SIGINT, signal_handler);
	soundFd = initSoundDevice(LINEIN_SOUND_INPUT);
	if (soundFd == FAILURE) {
		printf("init Sound Device error\n");
		return 1;
	} else {
		printf("init sound device ok\n");
	}

	outputFd = initOutputSoundDevice();
	if (outputFd == FAILURE) {
		printf("init Output Device error\n");
		close(soundFd);
		return 1;
	} else {
		printf("init output device ok\n");
	}
	
	printf("Press Ctrl + C to quit.\n");

	while(1) {
	    numOfBytes = read(soundFd, inputBuf, INPUTBUFSIZE);
	    if (numOfBytes == -1) {
            	ERR("Error reading the data from speech file\n");
		return 1;
	    }
	    write(outputFd, inputBuf, numOfBytes);
        }


	return 0;
}
