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
#include "learning.h"

#define LEARN_TIMES 3

static int learncount;
static int keycount;
static int keynum;
static int learnenter;
static struct blaster_data_pack key_num_pack[10];
static struct blaster_data_pack key_enter_pack;
static struct blaster_data_type *bls_dat_array[3];

static void refresh_learning_text()
{
    if(!learnenter)
    {
        switch(learncount)
        {
        case 0:
            printf("please press key \"%d\"\n", keycount);
            break;
        case 1:
            printf("please press key \"%d\" again\n", keycount);
            break;
        case 2:
            printf("please press key \"%d\" the third time\n", keycount);
            break;
        default:
            break;
        }
    }
    else
    {
        switch(learncount)
        {
        case 0:
            printf("please press key \"enter\"\n");
            break;
        case 1:
            printf("please press key \"enter\" again\n");
            break;
        case 2:
            printf("please press key \"enter\" the third time\n");
            break;
        default:
            break;
        }
    }
}

static int get_learned_data(int count)
{
    int fd = 0,ret = 0;
    fd = open(IR_BLASTER_DEV, O_RDWR);
    if(fd < 0)
    {
        return -1;
    }
    memset(bls_dat_array[count], 0, sizeof(struct blaster_data_type));
    ret = ioctl(fd, RRB_READ_LEARNING_DATA, bls_dat_array[count]);
    close(fd);
    return ret;
}

static int start_learn_key(void)
{
    int fd = 0,ret = 0;
    fd = open(IR_BLASTER_DEV, O_RDWR);
    if(fd == 0 || fd == -1)
    {
        return -1;
    }
    ret = ioctl(fd, RRB_CAPTURE_KEY);
    close(fd);
    return ret;
}

#define DIFFERENT_KEY (BLASTER_MAX_SBITS+3)
static int compact_bls_data(struct blaster_data_type *blsdat, struct blaster_data_pack *blsdat_pack)
{
    int i, j, count;
    int buff_used = 1;
    unsigned int buff[DIFFERENT_KEY];
    unsigned char buff_count[DIFFERENT_KEY];
    unsigned char bit[3] = {1,1,1};
    memset(buff, 0, DIFFERENT_KEY * sizeof(short));
    memset(buff_count, 0, DIFFERENT_KEY * sizeof(char));
    count = ((blsdat->bitstimes & BITS_COUNT_MASK) >> BITS_COUNT_START); 
    if(count > BLASTER_MAX_CHANGE)
        return -1;
    buff[0] = blsdat->bits[0];
    buff_count[0] = 1;
    buff_used = 1;
    blsdat_pack->bitstimes = blsdat->bitstimes;
    for(i = 1; i < count; i++)
    {
        for(j = 0; j < buff_used; j++)
        {
            if(buff_count[j] == 0)
                return -1;
            if(the_similar(blsdat->bits[i], buff[j] / buff_count[j]))
            {
                buff[j] += blsdat->bits[i];
                buff_count[j]++;
                break;
            }
        }
        if(j == buff_used)
        {
            if(j > DIFFERENT_KEY - 1)
                return -1;
            buff[j] = blsdat->bits[i];
            buff_count[j] = 1;
            buff_used++;
        }
    }

    for(j = 0; j < buff_used; j++)
    {
        if(buff_count[j] == 0)
            return -1;
        buff[j] = buff[j] / buff_count[j];
    }
    blsdat_pack->bit0 = buff[0];
    bit[0] = buff_count[0];
    for(i = 1; i < buff_used; i++)
    {
        if(buff_count[i] > bit[0])
        {
            if(bit[1] > 1)
            {
                blsdat_pack->bit2 = blsdat_pack->bit1;
                bit[2] = bit[1];
            }
            blsdat_pack->bit1 = blsdat_pack->bit0;
            blsdat_pack->bit0 = buff[i];
            bit[1] = bit[0];
            bit[0] = buff_count[i];
        }
        else if(buff_count[i] > bit[1])
        {
            if(bit[1] > 1)
            {
                blsdat_pack->bit2 = blsdat_pack->bit1;
                bit[2] = bit[1];
            }
            blsdat_pack->bit1 = buff[i];
            bit[1] = buff_count[i];
        }
        else if(buff_count[i] > bit[2])
        {
            blsdat_pack->bit2 = buff[i];
            bit[2] = buff_count[i];
        }
    }
    j = 0;
    for(i = 0; i < count; i++)
    {
        if(the_similar(blsdat_pack->bit0, blsdat->bits[i]))
        {
            keybit_set(i, blsdat_pack->dbits, 0);
        }
        else if(the_similar(blsdat_pack->bit1, blsdat->bits[i]))
        {
            keybit_set(i, blsdat_pack->dbits, 1);
        }
        else if(the_similar(blsdat_pack->bit2, blsdat->bits[i]))
        {
            keybit_set(i, blsdat_pack->mbits, 1);
        }
        else
        {
            keybit_set(i, blsdat_pack->dbits, 1);
            keybit_set(i, blsdat_pack->mbits, 1);
            if(j >= BLASTER_MAX_SBITS)
                return -1;
            blsdat_pack->specbits[j] = blsdat->bits[i];
            j++;
        }
    }
    return 0;
}

static int compare_two_key(struct blaster_data_type *bls_dat1, struct blaster_data_type *bls_dat2)
{
    int i, count1, count2;
    if((bls_dat1->bitstimes & FIRST_LEVEL_BIT_MASK) != (bls_dat2->bitstimes & FIRST_LEVEL_BIT_MASK))
    {
        return -1;
    }
    count1 = ((bls_dat1->bitstimes & BITS_COUNT_MASK) >> BITS_COUNT_START);
    count2 = ((bls_dat2->bitstimes & BITS_COUNT_MASK) >> BITS_COUNT_START);
    if(count1 == 0 || count2 == 0)
        return -1;
    if(count2 < count1)
    {
        count1 = count2;
        bls_dat1->bitstimes = bls_dat2->bitstimes;
    }
    else
    {
        bls_dat2->bitstimes = bls_dat1->bitstimes;
    }
    for(i=0; i<count1; i++)
    {
        if(!the_similar(bls_dat1->bits[i], bls_dat2->bits[i]))
        {
            return -1;
        }
    }
    for(i=0; i<count1; i++)
    {
        bls_dat1->bits[i] = bls_dat2->bits[i] = ((bls_dat1->bits[i] + bls_dat2->bits[i]) >> 1);
    }
    return 0;
}

static void sort_by_wave_count()
{
    struct blaster_data_type *tmp;
    int count[LEARN_TIMES], i, j, n;
    count[0] = ((bls_dat_array[0]->bitstimes & BITS_COUNT_MASK) >> BITS_COUNT_START);
    count[1] = ((bls_dat_array[1]->bitstimes & BITS_COUNT_MASK) >> BITS_COUNT_START);
    count[2] = ((bls_dat_array[2]->bitstimes & BITS_COUNT_MASK) >> BITS_COUNT_START);
    for(n = 0; n < LEARN_TIMES; n++)
    {
        j = n;
        for(i = n; i < LEARN_TIMES; i++)
        {
            if(count[i] > count[j])
                j = i;
        }
        if(j != n)
        {
            i = count[n];
            count[n] = count[j];
            count[j] = i;
            tmp = bls_dat_array[n];
            bls_dat_array[n] = bls_dat_array[j];
            bls_dat_array[j] = tmp;
        }
    }
}

static int process_data()
{
    struct blaster_data_type *bls_dat_type_ok;
    sort_by_wave_count();
    if((!compare_two_key(bls_dat_array[0], bls_dat_array[1])) || (!compare_two_key(bls_dat_array[0], bls_dat_array[2])))
    {
        bls_dat_type_ok = bls_dat_array[0];
    }
    else if((!compare_two_key(bls_dat_array[1], bls_dat_array[2])))
    {
        bls_dat_type_ok = bls_dat_array[1];
    }
    else
    {
        return -1;
    }
    if(!learnenter)
    {
        if(compact_bls_data(bls_dat_type_ok, &key_num_pack[keycount]))
        {
            return -1;
        }
    }
    else
    {
        if(compact_bls_data(bls_dat_type_ok, &key_enter_pack))
        {
            return -1;
        }
    }
    return 0;
}

static void process_key()
{
    int ret;
    get_learned_data(learncount);
    learncount++;
    if(learncount < LEARN_TIMES)
    {
        refresh_learning_text();
        ret = start_learn_key();
        if(ret)
        {
            printf("learning error1\n");
            exit(-1);
        }
    }
    else
    {
        if(process_data())
        {
            printf("The key you just pressed is not consistent with those you pressed before. Please try again.\n");
            usleep(WAIT_OSD_IR_WAVE_CLEAN);
            goto retry;
        }
        keycount++;
        if(keycount >= keynum)
        {
            if(!learnenter)
            {
                go_select_enter();
            }
            else
            {
                go_blaster();
            }
            return;
        }
retry:
        learncount = 0;
        refresh_learning_text();
        ret = start_learn_key();
        if(ret)
        {
            printf("learning error2\n");
            exit(-1);
        }
    }
}

void learning_act(unsigned char key)
{
    int ret;
    switch(key)
    {
    case LEARNING_RELEASE_KEY:
        process_key();
        break;
    case LEARNING_COMPLETE_KEY:
        break;
    case KEY_LEFT:
        if(keycount > 0)
            keycount--;
        learncount = 0;
        refresh_learning_text();
        usleep(WAIT_OSD_IR_WAVE_CLEAN);
        ret = start_learn_key();
        if(ret)
        {
            printf("learning error3\n");
            exit(-1);
        }
        break;
    default:
        ret = start_learn_key();
        if(ret)
        {
            printf("learning error5\n");
            exit(-1);
        }
        printf("It seems you are pointing the OSD remote, please use the RECEIVER REMOTE CONTROL that is used for your tuner device.\n");
        break;
    }
}

void learning_init(int status)
{
    int ret, i;
    keycount = 0;
    learncount = 0;
    learnenter = status;
    for(i=0; i<3; i++)
    {
        bls_dat_array[i] = malloc(sizeof(struct blaster_data_type));
    }
    if(!learnenter)
    {
        keynum = 10;
    }
    else
        keynum = 1;
    usleep(WAIT_OSD_IR_WAVE_CLEAN);
    ret = start_learn_key();
    if(ret)
    {
        printf("learning error4\n");
        exit(-1);
    }
    printf("Please point your tuner remote to the IR received, then follow the instructions on screen.\n"
           "You can abort this app at any time by pressing BACK on your OSD remote.\n"
           "You can repeat learning the last key your just learned by pressing LEFT on your OSD remote.\n");            
    refresh_learning_text();
}

struct blaster_data_pack *get_num_key(int num)
{
    return &key_num_pack[num];
}

struct blaster_data_pack *get_enter_key()
{
    return &key_enter_pack;
}
