/** @file util.h
 *  @brief Generally useful functions from libc.
 */

/* Copyright (c) 2007,2008 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_UTIL_H__
#define __NXOS_BASE_UTIL_H__

#include "base/types.h"

/** @addtogroup typesAndUtils */
/*@{*/

/** Return the numeric minimum of two parameters.
 *
 * @warning This macro will cause the arguments to be evaluated several
 *          times! Use only with pure arguments.
 */
#define MIN(x, y) ((x) < (y) ? (x): (y))

/** Return the numeric maximum of two parameters.
 *
 * @warning This macro will cause the arguments to be evaluated several
 *          times! Use only with pure arguments.
 */
#define MAX(x, y) ((x) > (y) ? (x): (y))

/** Copy @a len bytes from @a src to @a dest.
 *
 * @param dest Destination of the copy.
 * @param src Source of the copy.
 * @param len Number of bytes to copy.
 *
 * @warning The source and destination memory regions must not overlap.
 */
void memcpy(void *dest, const void *src, U32 len);

/** Initialize @a len bytes of @a dest with the constant @a val.
 *
 * @param dest Start of the region to initialize.
 * @param val Constant initializer to use.
 * @param len Length of the region.
 */
void memset(void *dest, const U8 val, U32 len);

/** Return the length of the given null-terminated string.
 *
 * @param str The string to evaluate.
 * @return The length in bytes of the string.
 */
U32 strlen(const char *str);

/** Compare two string prefixes for equality.
 *
 * @param a First string to compare.
 * @param b Second string to compare.
 * @param n Number of bytes to compare.
 * @return TRUE if the first @a n bytes of @a a are equal to @a b,
 * FALSE otherwise.
 *
 * @note This function will halt on the first NULL byte it finds in
 * either string.
 */
bool streqn(const char *a, const char *b, U32 n);

/** compare two strings for equality.
 *
 * This is equivalent to @a strneq, with the maximum length being the
 * length of the shortest input string.
 *
 * @see strneq
 */
bool streq(const char *a, const char *b);

/** Locate leftmost instance of character @a c in string @a s.
 *
 * @param s The string to search.
 * @param c The character to find.
 * @return A pointer to the first occurence of @a c in @a s, or NULL if
 * there is none.
 */
char *strchr(const char *s, const char c);

/** Locate rightmost instance of character @a c in string @a s.
 *
 * @param s The string to search.
 * @param c The character to find.
 * @return A pointer to the last occurence of @a c in @a s, or NULL if
 * there is none.
 */
char *strrchr(const char *s, const char c);

/** Convert a string to the unsigned integer it represents, if possible.
 *
 * @param s The string to convert.
 * @param result A pointer to the integer that will contain the parsed
 * result, if the conversion was successful.
 * @return TRUE with *result set correctly if the conversion was
 * successful, FALSE if the conversion failed.
 *
 * @note If the conversion fails, the value of @a *result will still
 * be clobbered, but won't contain the true value.
 */
bool atou32(const char *s, U32* result);

/** Convert a string to the signed integer it represents, if possible.
 *
 * @param s The string to convert.
 * @param result A pointer to the integer that will contain the parsed
 * result, if the conversion was successful.
 * @return TRUE with *result set correctly if the conversion was
 * successful, FALSE if the conversion failed.
 *
 * @note If the conversion fails, the value of @a *result will still
 * be clobbered, but won't contain the true value.
 */
bool atos32(const char *s, S32* result);

/** Word cursor pointer
 *
 * It must be used as transparent type as first parameter for
 * nx_extract_auto and nx_extract_type functions.
 */
typedef U8 * cursor_t;

#define nx_extract_init(buffer) (cursor_t)buffer

/** Extracts a value even if not aligned. 
 *
 * The size parameter refers to the byte length of the data to be extracted.
 *
 * @note The procedure doesn't cover error checking. Don not use size values
 * different from 1, 2 or 4.
 *
 * @param addr The address of the word
 * @param size Size in bytes of the word (1, 2 or 4)
 */
U32 nx_extract(U8 *addr, U8 size);

/** Quick extraction macro
 *
 * Uses the extract primitive and increments the pointer.
 *
 * @note The procedure doesn't cover error checking. Don not use size values
 * different from 1, 2 or 4.
 *
 * @param addr The address of the word
 * @param size Size in bytes of the word (1, 2 or 4)
 */
#define nx_extract_auto(addr, size) nx_extract(((addr) += size) - size, size)

/** Quick, type based, extraction macro
 *
 * Uses the extract primitive and increments the pointer.
 *
 * @note The procedure doesn't cover error checking. Don not use size values
 * different from 1, 2 or 4.
 *
 * @param addr The address of the word
 * @param type The type to extract (U8, U16, U32)
 */
#define nx_extract_type(addr, type) nx_extract_auto(addr, sizeof(type))

/*@}*/

#endif /* __NXOS_BASE_UTIL_H__ */
