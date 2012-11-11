/*
 * Copyright (c) 2012 Petr Koupy
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

/** @addtogroup draw
 * @{
 */
/**
 * @file
 */

#include <assert.h>

#include "source.h"

void source_init(source_t *source)
{
	transform_identity(&source->transform);
	source->filter = filter_nearest;
	
	source->color = PIXEL(0, 0, 0, 0);
	source->texture = NULL;
	source->texture_tile = false;

	source->alpha = PIXEL(255, 0, 0, 0);
	source->mask = NULL;
	source->mask_tile = false;
}

void source_set_transform(source_t *source, transform_t transform)
{
	source->transform = transform;
	transform_invert(&source->transform);
}

void source_reset_transform(source_t *source)
{
	transform_identity(&source->transform);
}

void source_set_filter(source_t *source, filter_t filter)
{
	source->filter = filter;
}

void source_set_color(source_t *source, pixel_t color)
{
	source->color = color;
}

void source_set_texture(source_t *source, surface_t *texture, bool tile)
{
	source->texture = texture;
	source->texture_tile = tile;
}

void source_set_alpha(source_t *source, pixel_t alpha)
{
	source->alpha = alpha;
}

void source_set_mask(source_t *source, surface_t *mask, bool tile)
{
	source->mask = mask;
	source->mask_tile = tile;
}

pixel_t source_determine_pixel(source_t *source, double x, double y)
{
	if (source->mask || source->texture) {
		transform_apply_affine(&source->transform, &x, &y);
	}

	pixel_t mask_pix;
	if (source->mask) {
		mask_pix = source->filter(
		    surface_pixmap_access(source->mask),
		    x, y, source->mask_tile);
	} else {
		mask_pix = source->alpha;
	}

	if (!ALPHA(mask_pix)) {
		return 0;
	}

	pixel_t texture_pix;
	if (source->texture) {
		texture_pix = source->filter(
		    surface_pixmap_access(source->texture),
		    x, y, source->texture_tile);
	} else {
		texture_pix = source->color;
	}

	if (ALPHA(mask_pix) < 255) {
		double ratio = ((double) ALPHA(mask_pix)) / 255.0;
		double res_a = ratio * ((double) ALPHA(texture_pix));
		return PIXEL((unsigned) res_a,
		    RED(texture_pix), GREEN(texture_pix), BLUE(texture_pix));
	} else {
		return texture_pix;
	}
}

/** @}
 */
