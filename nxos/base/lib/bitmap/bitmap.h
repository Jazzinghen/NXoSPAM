#ifndef __NXOS_BITMAP_H__
#define __NXOS_BITMAP_H__

#include "base/types.h"

typedef U32 nx_bmp_t;

#define DW_BMP_INIT(name) nx_bmp_t name = 0

/** Returns the index of the most significative bit set to 1
 *
 * @param bmp The 32-bit bitmap to be analyzed
 * @return The 1-based index of the most significative bit set to 1, or 0 if
 *         there's no such bit.
 */
U8 nx_bmp_msb_pos(nx_bmp_t bmp);

/** Sets a value into the bitmap
 *
 * @param bmp The 32-bit bitmap to be modified
 * @param b The bit to set (1-32). If 0 the bitmap will be cleared
 */
void nx_bmp_set(nx_bmp_t *bmp, U8 b);

/** Resets a value of the bitmap
 * 
 * @param bmp The 32-bit bitmap to be modified
 * @param b The bit to reset (1-32)
 */
void nx_bmp_reset(nx_bmp_t *bmp, U8 b);

#endif /* __NXOS_BITMAP_H__ */

