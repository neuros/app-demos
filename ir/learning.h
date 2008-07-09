#ifndef LEARNING_H
#define LEARNING_H
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

#define IR_BLASTER_DEV "/dev/neuros_ir_blaster"
#define WAIT_OSD_IR_WAVE_CLEAN (500*1000)

void learning_init(int status);
void learning_act(unsigned char key);
struct blaster_data_pack *get_num_key(int num);
struct blaster_data_pack *get_enter_key();

#endif
