/* Copyright (c) 2009 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#include "_memcpy.h"
#include "base/types.h"
#include "base/at91sam7s256.h"
#include "base/drivers/_efc.h"
#include "base/memmap.h"
#include "base/assert.h"
#include "base/drivers/_efc.h"

#define BELONGS(start, end, pos, len) \
    (((S32)pos) >= ((S32)start) && ((S32)pos) < ((S32)end)) && \
    (((S32)pos) + ((S32)len) <= ((S32)end))

typedef enum {
  CHUNK_INVALID = -1,
  CHUNK_RAM = 0,
  CHUNK_FLASH = 1,
  CHUNK_RELOC_RAM = 2,
  CHUNK_RELOC_FLASH = 3
} belong_t;

#define PAGE_SIZE  256                  // Page size in bytes
#define PAGE_WSIZE 64                   // Page size in 32 bit words
#define PAGES_NUMB 1024                 // Number of pages
#define PAGE_SHIFT 8                    // 2^8 = PAGE_SIZE

#define REAL_LENGTH_RAM     64 * 1024   // 64 kb ram
#define REAL_LENGTH_FLASH  256 * 1024   // 256 kb flash

#define AT91C_RAM_START     ((U32)AT91C_ISRAM)
#define AT91C_FLASH_START   ((U32)AT91C_IFLASH)

#define AT91C_RELOC_START   0x00000000

static void * memcpy_ram(U8 *dst, const U8 *src, U32 len) {
  while (len--) {
    *dst++ = *src++;
  }
  return dst;
}

static bool write_page(U32 page, U8 *buffer) {
  return nx__efc_write_page((U32 *)buffer, page);
}

static int positioning(U32 pos, size_t len) {
  if (BELONGS(AT91C_RAM_START, AT91C_RAM_START + REAL_LENGTH_RAM,
              pos, len))
    return CHUNK_RAM;
  if (BELONGS(AT91C_FLASH_START, AT91C_FLASH_START + REAL_LENGTH_FLASH,
              pos, len))
    return CHUNK_FLASH;

  if (NX_BOTTOM_MAP_RAM &&
      BELONGS(AT91C_RELOC_START, AT91C_RELOC_START + REAL_LENGTH_RAM,
              pos, len))
    return CHUNK_RELOC_RAM;
  if (NX_BOTTOM_MAP_FLASH &&
      BELONGS(AT91C_RELOC_START, AT91C_RELOC_START +
              REAL_LENGTH_FLASH, pos, len))
    return CHUNK_RELOC_FLASH;
  return CHUNK_INVALID;
}

static void replace_flash(U32 pagenumb, U32 offset, const U8 *src, size_t n) {
  U32 i;
  U8 buffer[PAGE_SIZE];
  U8 *page = (U8 *)AT91C_FLASH_START + (pagenumb * PAGE_SIZE);

  for (i = 0; i < offset; i++)
    buffer[i] = *page++;
  for (offset += n; i < offset; i++) {
    buffer[i] = *src++;
  }
  page += n;
  while (i < PAGE_SIZE)
    buffer[i++] = *page++;

  NX_ASSERT_MSG(write_page(pagenumb, buffer), "Flash page write");
}

static void *memcpy_flash(U8 *dest, const U8 *src, size_t n) {
  U32 page = (U32)(dest - AT91C_FLASH_START) / (U32)PAGE_SIZE;
  U32 edge = (U32)(dest + n) & (U32)(PAGE_SIZE - 1);

  if (edge > 0 && edge < n) {
    // splitted in two pages
    replace_flash(page, PAGE_SIZE - n + edge, src, n - edge);
    replace_flash(page + 1, 0, src + n - edge, edge);
  } else {
    edge += (PAGE_SIZE - n);
    edge &= (PAGE_SIZE - 1);
    // on a single page
    replace_flash(page, edge, src, n);
  }

  return dest;
}

void *_memcpy(void *dest, const void *src, size_t n) {
  belong_t belong = positioning((U32)dest, n);
  switch (belong) {
  case CHUNK_RAM:
  case CHUNK_RELOC_RAM:
    return memcpy_ram((U8 *)dest, (U8 *)src, n);
    break;
  case CHUNK_RELOC_FLASH:
  case CHUNK_FLASH:
    return memcpy_flash((U8 *)dest, (U8 *)src, n);
    break;
  default:
    NX_FAIL("Access to undefined memory area");
    return NULL;
  }
}

