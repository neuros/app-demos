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
#include "keymap.h"
#include "blaster.h"
#include "learning.h"

#define CHECK_BLASTER_COMPLETE_CYCLE (20*1000)
#define INTERVAL_OF_KEYS (200*1000)
#define CHANNEL_LEN 10
#define BASE KEY_1

static char key_table[10]=
{
    '1','2','3','4','5','6','7','8','9','0'
};
static char channel[CHANNEL_LEN+1];

static void blaster_key(const char *key_name)
{
    int fd = 0;
    int ret = -1;
    int bls_status;
    struct blaster_data_pack *bls_dat;
    fd = open(IR_BLASTER_DEV, O_RDWR);
    if(fd<0)
        return;
    if((*key_name) < '9' && (*key_name) > '0')
    {
        bls_dat = get_num_key((*key_name) - '0');
    }
    else
    {
        bls_dat = get_enter_key();
    }
    ioctl(fd, RRB_BLASTER_KEY, bls_dat);
    do
    {
        usleep(CHECK_BLASTER_COMPLETE_CYCLE);
        ioctl(fd, RRB_GET_BLASTER_STATUS, bls_status);
    }while(bls_status == BLS_START);
    ret = 0;
bail:
    close(fd);
}

static void blaster_channel()
{
    int i;
    printf("\nblaster channel:%s\n", channel);
    usleep(WAIT_OSD_IR_WAVE_CLEAN);
    for(i = 0; i < strlen(channel); i++)
    {
        blaster_key(&channel[i]);
        usleep(INTERVAL_OF_KEYS);
    }
    if(need_enter_key())
    {
        blaster_key("enter");
    }
    blaster_init();
}

static void add_number(unsigned char key)
{
    if(strlen(channel) < CHANNEL_LEN)
    {
        printf("%c", key_table[key - BASE]);
        channel[strlen(channel)] = key_table[key - BASE];
    }
}

void blaster_act(unsigned char key)
{
    switch(key)
    {
    case KEY_1:
    case KEY_2:
    case KEY_3:
    case KEY_4:
    case KEY_5:
    case KEY_6:
    case KEY_7:
    case KEY_8:
    case KEY_9:
    case KEY_0:
        add_number(key);
        break;
    case KEY_ENTER:
        blaster_channel();
        break;
    default:
        break;
    }
}

void blaster_init()
{
    printf("Please press the channel number that you want to test: ");
    memset(channel, 0, CHANNEL_LEN + 1);
}
