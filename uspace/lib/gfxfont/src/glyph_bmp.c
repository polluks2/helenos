/*
 * Copyright (c) 2020 Jiri Svoboda
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

/** @addtogroup libgfxfont
 * @{
 */
/**
 * @file Glyph bitmap
 */

#include <errno.h>
#include <gfx/coord.h>
#include <gfx/glyph_bmp.h>
#include <stdlib.h>
#include "../private/glyph_bmp.h"

static errno_t gfx_glyph_bmp_extend(gfx_glyph_bmp_t *, gfx_coord2_t *);

/** Open glyph bitmap for editing.
 *
 * @param glyph Glyph
 * @param rbmp Place to store glyph bitmap
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t gfx_glyph_bmp_open(gfx_glyph_t *glyph, gfx_glyph_bmp_t **rbmp)
{
	gfx_glyph_bmp_t *bmp;

	bmp = calloc(1, sizeof(gfx_glyph_bmp_t));
	if (bmp == NULL)
		return ENOMEM;

	bmp->rect.p0.x = 0;
	bmp->rect.p0.y = 0;
	bmp->rect.p1.x = 0;
	bmp->rect.p1.y = 0;
	bmp->pixels = NULL;

	bmp->glyph = glyph;
	*rbmp = bmp;
	return EOK;
}

/** Save glyph bitmap.
 *
 * @param bmp Glyph bitmap
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t gfx_glyph_bmp_save(gfx_glyph_bmp_t *bmp)
{
	return EOK;
}

/** Close glyph bitmap.
 *
 * @param bmp Glyph bitmap
 */
void gfx_glyph_bmp_close(gfx_glyph_bmp_t *bmp)
{
	free(bmp->pixels);
	free(bmp);
}

/** Get pixel from glyph bitmap.
 *
 * @param bmp Glyph bitmap
 * @param x X-coordinate
 * @param y Y-coordinate
 * @return Pixel value
 */
int gfx_glyph_bmp_getpix(gfx_glyph_bmp_t *bmp, gfx_coord_t x, gfx_coord_t y)
{
	gfx_coord2_t pos;
	size_t pitch;

	pos.x = x;
	pos.y = y;
	if (!gfx_pix_inside_rect(&pos, &bmp->rect))
		return 0;

	pitch = bmp->rect.p1.x - bmp->rect.p0.x;
	return bmp->pixels[(y - bmp->rect.p0.y) * pitch +
	    (x - bmp->rect.p0.x)];
}

/** Set pixel in glyph bitmap.
 *
 * @param bmp Glyph bitmap
 * @param x X-coordinate
 * @param y Y-coordinate
 *
 * @reutrn EOK on success, ENOMEM if out of memory
 */
errno_t gfx_glyph_bmp_setpix(gfx_glyph_bmp_t *bmp, gfx_coord_t x,
    gfx_coord_t y, int value)
{
	gfx_coord2_t pos;
	size_t pitch;
	errno_t rc;

	pos.x = x;
	pos.y = y;
	if (!gfx_pix_inside_rect(&pos, &bmp->rect)) {
		rc = gfx_glyph_bmp_extend(bmp, &pos);
		if (rc != EOK)
			return rc;
	}

	pitch = bmp->rect.p1.x - bmp->rect.p0.x;
	bmp->pixels[(y - bmp->rect.p0.y) * pitch +
	    (x - bmp->rect.p0.x)] = value;
	return EOK;
}

/** Extend glyph bitmap to cover a patricular pixel.
 *
 * @param bmp Glyph bitmap
 * @param pos Pixel position
 *
 * @return EOK on sucesss, ENOMEM if out of memory
 */
static errno_t gfx_glyph_bmp_extend(gfx_glyph_bmp_t *bmp, gfx_coord2_t *pos)
{
	gfx_rect_t prect;
	gfx_rect_t nrect;
	int *npixels;
	size_t npitch;
	size_t opitch;
	gfx_coord_t x, y;

	/* Compute new rectangle enveloping current rectangle and new pixel */
	prect.p0 = *pos;
	prect.p1.x = prect.p0.x + 1;
	prect.p1.y = prect.p0.y + 1;

	gfx_rect_envelope(&bmp->rect, &prect, &nrect);

	/* Allocate new pixel array */
	npixels = calloc(sizeof(int), (nrect.p1.x - nrect.p0.x) *
	    (nrect.p1.y - nrect.p0.y));
	if (npixels == NULL)
		return ENOMEM;

	/* Transfer pixel data */
	opitch = bmp->rect.p1.x - bmp->rect.p0.x;
	npitch = nrect.p1.x - nrect.p0.x;

	for (y = bmp->rect.p0.y; y < bmp->rect.p1.y; y++) {
		for (x = bmp->rect.p0.x; x < bmp->rect.p1.x; x++) {
			npixels[(y - nrect.p0.y) * npitch + x - nrect.p0.x] =
			    bmp->pixels[(y - bmp->rect.p0.y) * opitch +
			    x - bmp->rect.p0.x];
		}
	}

	/* Switch new and old data */
	free(bmp->pixels);
	bmp->pixels = npixels;
	bmp->rect = nrect;

	return EOK;
}

/** @}
 */
