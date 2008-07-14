/*               <www.neurostechnology.com>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 2 of the License.
 *  
 *
 *  This program is distributed in the hope that, in addition to its 
 *  original purpose to support Neuros hardware, it will be useful 
 *  otherwise, but WITHOUT ANY WARRANTY; without even the implied 
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 ****************************************************************************
 */

#include <stdlib.h>
#include <stdio.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/neuros_ir_blaster.h>

#define IR_BLASTER_DEV "/dev/neuros_ir_blaster"
#define IR_DEV "/dev/neuros_ir"
#define KEY_TEST 0x3f
#define MAX_RETRY 5

static int ir_fd = 0;
// OSD KEY_TEST data in blaster_data_pack format, which will be blastered out and received
// by the OSD ir receiver
struct blaster_data_pack bls_dat={
    .bitstimes = 0x801A,
    .mbits[0] = 0x3,
    .mbits[1] = 0,
    .mbits[2] = 0,
    .mbits[3] = 0x2,
    .dbits[0] = 0x13,
    .dbits[1] = 0x45,
    .dbits[2] = 0x04,
    .dbits[3] = 0x02,
    .bit0 = 1012,
    .bit1 = 2075,
    .specbits[0] = 4032,
    .specbits[1] = 1012,
    .specbits[2] = 43288,
}; 

static int blaster_test()
{
    int fd = 0;

    if ((fd = open(IR_BLASTER_DEV, O_RDONLY)) <= 0)
    {
        printf("Factory test error: open ir blaster device error\n");
        return -1;    
    }
    if (ioctl(fd, RRB_FACTORY_TEST))
    {
        printf("Factory test error: ioctl RRB_FACTORY_TEST error\n");
        return -1;
    }
    if (ioctl(fd, RRB_BLASTER_KEY, &bls_dat))
    {
        printf("Factory test error: ioctl RRB_BLASTER_KEY error\n");
        return -1;
    }
    return 0;                 
}

static void action_event()
{
    static unsigned char key = 0xff;

    read(ir_fd, &key, 1);
    switch(key)
    {
    case KEY_TEST:
        printf("IR BLASTER test works!\n");
        exit(0);
        break;
    default:
        break;
    }
}

static void main_loop()
{
    fd_set set;
    struct timeval tv;
    int retry;

    for (retry = 0; retry < MAX_RETRY; retry++)
    {
        FD_ZERO(&set);
        FD_SET(ir_fd, &set);
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        if (select(ir_fd + 1, &set, NULL, NULL, &tv) > 0)
        {
            if (FD_ISSET(ir_fd, &set))
               action_event();
        }
    }
    close(ir_fd);
    if (retry == MAX_RETRY)
    {
        printf("Factory test error: do not receive test key\n");
        exit(-1);
    }
}

int main(int argc, char *argv[])
{
    if ((ir_fd = open(IR_DEV, O_RDONLY)) > 0)
    {
        if (blaster_test())
        {
            printf("Factory test error: blaster error\n");
            exit(-1);
        }
        main_loop();
    }
    printf("Factory test error: open ir device error\n");
    exit(-1);
}
