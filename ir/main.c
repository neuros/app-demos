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
#include <fcntl.h>

#include "keymap.h"
#include "learning.h"
#include "blaster.h"
#include "main.h"

#define NEUROS_IR "/dev/neuros_ir"
#define S_LEARNING 0
#define S_BLASTER 1
#define S_SELECT_ENTER 2

static int ir_fd = 0;
static int status;
static int need_enter;
static unsigned char key = 0xff;

static void action_event()
{
    read(ir_fd, &key, 1);
    switch(key)
    {
    case KEY_BACK:
        exit(0);
        break;
    case KEY_RELEASE:
        return;
    default:
        break;
    }
    switch(status)
    {
    case S_LEARNING:
        learning_act(key);
        break;
    case S_BLASTER:
        blaster_act(key);
        break;
    case S_SELECT_ENTER:
        if(key == KEY_1)
        {
            need_enter = 1;
            status = S_LEARNING;
            learning_init(1);
        }
        else
        {
            need_enter = 0;
            go_blaster();
        }
        break;
    }
}

static void mainLoop()
{
    fd_set set;
    struct timeval tv;

    while( 1 )
    {
        FD_ZERO(&set);
        FD_SET(ir_fd, &set);
        tv.tv_sec = 0;
        tv.tv_usec = 100000;
        if(select(ir_fd + 1, &set, NULL, NULL, &tv) > 0)
        {
            if(FD_ISSET(ir_fd, &set))
               action_event();
        }
    }
    close(ir_fd);
}

void go_select_enter()
{
    printf("if your device need a enter key to confirm the channel if need press key \"1\" if not press any other key\n");
    status = S_SELECT_ENTER;
}

void go_blaster()
{
    status = S_BLASTER;
    blaster_init();
}

int need_enter_key()
{
    return need_enter;
}

int main(int argc,char *argv[])
{
    if((ir_fd = open(NEUROS_IR, O_RDONLY)) > 0)
    {
        status = S_LEARNING;
        learning_init(0);
        mainLoop();
        exit(0);
    }
    exit(-1);
}


          
