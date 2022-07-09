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

void hiscore_get(const char *file, uint8_t *data);
void hiscore_put(const char *file, const uint8_t *data);
