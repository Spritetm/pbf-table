//Bindings for highscore load/save facilities
/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */

#include <stdint.h>

/*
Hiscore is 4 entries of 16 byte each.
12 bytes: unpacked BCD representation of the score (123 decimal = 0x01 0x02 0x03)
3 bytes: capital initials
1 byte: 0
*/

//Get the hiscore from non-volatile memory. File is 
//'tablex.hi' with x [1,2,3,4].
void hiscore_get(const char *file, uint8_t *data);

//Set non-volatile memory to the given hiscore.
void hiscore_put(const char *file, const uint8_t *data);
