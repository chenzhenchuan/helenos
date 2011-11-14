/*
 * Copyright (c) 2011 Frantisek Princ
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup libext4
 * @{
 */ 

/**
 * @file	libext4_bitmap.c
 * @brief	Ext4 bitmap operations.
 */

#include <errno.h>
#include <libblock.h>
#include <sys/types.h>
#include "libext4.h"

void ext4_bitmap_free_bit(uint8_t *bitmap, uint32_t index)
{
	uint32_t byte_index = index / 8;
	uint32_t bit_index = index % 8;

	uint8_t *target = bitmap + byte_index;

	*target &= ~ (1 << bit_index);
}

void ext4_bitmap_set_bit(uint8_t *bitmap, uint32_t index)
{
	uint32_t byte_index = index / 8;
	uint32_t bit_index = index % 8;

	uint8_t *target = bitmap + byte_index;

	*target |= 1 << bit_index;
}

bool ext4_bitmap_is_free_bit(uint8_t *bitmap, uint32_t index)
{
	uint32_t byte_index = index / 8;
	uint32_t bit_index = index % 8;

	uint8_t *target = bitmap + byte_index;

	if (*target & (1 << bit_index)) {
		return false;
	} else {
		return true;
	}

}

int ext4_bitmap_find_free_byte_and_set_bit(uint8_t *bitmap, uint32_t start, uint32_t *index, uint32_t size)
{
	uint8_t *pos = bitmap + (start / 8) + 1;

	while (pos < bitmap + size) {
		if (*pos == 0) {
			*pos |= 1;

			*index = (pos - bitmap) * 8;
			return EOK;
		}

		++pos;
	}

	return ENOSPC;

}

int ext4_bitmap_find_free_bit_and_set(uint8_t *bitmap, uint32_t start, uint32_t *index, uint32_t size)
{
	uint8_t *pos = bitmap + (start / 8);
	int i;
	uint8_t value, new_value;

	while (pos < bitmap + size) {
		if ((*pos & 255) != 255) {
			// free bit found
			break;
		}

		++pos;
	}

	// Check the byte containing start
	if (pos == bitmap + (start / 8)) {
		for (i = start % 8; i < 8; ++i) {
			value = *pos;
			if ((value & (1 << i)) == 0) {
				// free bit found
				new_value = value | (1 << i);
				*pos = new_value;
				*index = (pos - bitmap) * 8 + i;
				return EOK;
			}
		}
	}

	if (pos < bitmap + size) {

		for(i = 0; i < 8; ++i) {
			value = *pos;

			if ((value & (1 << i)) == 0) {
				// free bit found
				new_value = value | (1 << i);
				*pos = new_value;
				*index = (pos - bitmap) * 8 + i;
				return EOK;
			}
		}
	}

	return ENOSPC;
}

/**
 * @}
 */ 
