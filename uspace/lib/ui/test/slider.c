/*
 * Copyright (c) 2021 Jiri Svoboda
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

#include <gfx/context.h>
#include <gfx/coord.h>
#include <mem.h>
#include <pcut/pcut.h>
#include <stdbool.h>
#include <ui/control.h>
#include <ui/slider.h>
#include <ui/resource.h>
#include "../private/slider.h"

PCUT_INIT;

PCUT_TEST_SUITE(slider);

static errno_t testgc_set_clip_rect(void *, gfx_rect_t *);
static errno_t testgc_set_color(void *, gfx_color_t *);
static errno_t testgc_fill_rect(void *, gfx_rect_t *);
static errno_t testgc_update(void *);
static errno_t testgc_bitmap_create(void *, gfx_bitmap_params_t *,
    gfx_bitmap_alloc_t *, void **);
static errno_t testgc_bitmap_destroy(void *);
static errno_t testgc_bitmap_render(void *, gfx_rect_t *, gfx_coord2_t *);
static errno_t testgc_bitmap_get_alloc(void *, gfx_bitmap_alloc_t *);

static gfx_context_ops_t ops = {
	.set_clip_rect = testgc_set_clip_rect,
	.set_color = testgc_set_color,
	.fill_rect = testgc_fill_rect,
	.update = testgc_update,
	.bitmap_create = testgc_bitmap_create,
	.bitmap_destroy = testgc_bitmap_destroy,
	.bitmap_render = testgc_bitmap_render,
	.bitmap_get_alloc = testgc_bitmap_get_alloc
};

static void test_slider_moved(ui_slider_t *, void *, gfx_coord_t);

static ui_slider_cb_t test_slider_cb = {
	.moved = test_slider_moved
};

static ui_slider_cb_t dummy_slider_cb = {
};

typedef struct {
	bool bm_created;
	bool bm_destroyed;
	gfx_bitmap_params_t bm_params;
	void *bm_pixels;
	gfx_rect_t bm_srect;
	gfx_coord2_t bm_offs;
	bool bm_rendered;
	bool bm_got_alloc;
} test_gc_t;

typedef struct {
	test_gc_t *tgc;
	gfx_bitmap_alloc_t alloc;
	bool myalloc;
} testgc_bitmap_t;

typedef struct {
	bool moved;
	gfx_coord_t pos;
} test_cb_resp_t;

/** Create and destroy slider */
PCUT_TEST(create_destroy)
{
	ui_slider_t *slider = NULL;
	errno_t rc;

	rc = ui_slider_create(NULL, "Hello", &slider);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(slider);

	ui_slider_destroy(slider);
}

/** ui_slider_destroy() can take NULL argument (no-op) */
PCUT_TEST(destroy_null)
{
	ui_slider_destroy(NULL);
}

/** ui_slider_ctl() returns control that has a working virtual destructor */
PCUT_TEST(ctl)
{
	ui_slider_t *slider;
	ui_control_t *control;
	errno_t rc;

	rc = ui_slider_create(NULL, "Hello", &slider);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	control = ui_slider_ctl(slider);
	PCUT_ASSERT_NOT_NULL(control);

	ui_control_destroy(control);
}

/** Set slider rectangle sets internal field */
PCUT_TEST(set_rect)
{
	ui_slider_t *slider;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_slider_create(NULL, "Hello", &slider);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 3;
	rect.p1.y = 4;

	ui_slider_set_rect(slider, &rect);
	PCUT_ASSERT_INT_EQUALS(rect.p0.x, slider->rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(rect.p0.y, slider->rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(rect.p1.x, slider->rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(rect.p1.y, slider->rect.p1.y);

	ui_slider_destroy(slider);
}

/** Paint slider in graphics mode */
PCUT_TEST(paint_gfx)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_slider_t *slider;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_slider_create(resource, "Hello", &slider);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_slider_paint_gfx(slider);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_slider_destroy(slider);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Paint slider in text mode */
PCUT_TEST(paint_text)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_slider_t *slider;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_slider_create(resource, "Hello", &slider);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_slider_paint_text(slider);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_slider_destroy(slider);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test ui_slider_moved() */
PCUT_TEST(moved)
{
	errno_t rc;
	ui_slider_t *slider;
	test_cb_resp_t resp;

	rc = ui_slider_create(NULL, "Hello", &slider);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Moved with no callbacks set */
	ui_slider_moved(slider, 42);

	/* Moved with callback not implementing clicked */
	ui_slider_set_cb(slider, &dummy_slider_cb, NULL);
	ui_slider_moved(slider, 42);

	/* Moved with real callback set */
	resp.moved = false;
	resp.pos = 0;
	ui_slider_set_cb(slider, &test_slider_cb, &resp);
	ui_slider_moved(slider, 42);
	PCUT_ASSERT_TRUE(resp.moved);
	PCUT_ASSERT_INT_EQUALS(42, resp.pos);

	ui_slider_destroy(slider);
}

/** Press and release slider */
PCUT_TEST(press_release)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	gfx_coord2_t pos;
	gfx_rect_t rect;
	ui_slider_t *slider;
	test_cb_resp_t resp;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_slider_create(resource, "Hello", &slider);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 110;
	rect.p1.y = 120;
	ui_slider_set_rect(slider, &rect);

	resp.moved = false;
	ui_slider_set_cb(slider, &test_slider_cb, &resp);

	PCUT_ASSERT_FALSE(slider->held);

	pos.x = 11;
	pos.y = 22;

	ui_slider_press(slider, &pos);
	PCUT_ASSERT_TRUE(slider->held);
	PCUT_ASSERT_FALSE(resp.moved);

	pos.x = 21;
	pos.y = 32;

	ui_slider_release(slider, &pos);
	PCUT_ASSERT_FALSE(slider->held);
	PCUT_ASSERT_TRUE(resp.moved);
	PCUT_ASSERT_INT_EQUALS(10, slider->pos);

	ui_slider_destroy(slider);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Press, update and release slider */
PCUT_TEST(press_uodate_release)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	gfx_coord2_t pos;
	gfx_rect_t rect;
	ui_slider_t *slider;
	test_cb_resp_t resp;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_slider_create(resource, "Hello", &slider);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 110;
	rect.p1.y = 120;
	ui_slider_set_rect(slider, &rect);

	resp.moved = false;
	ui_slider_set_cb(slider, &test_slider_cb, &resp);

	PCUT_ASSERT_FALSE(slider->held);

	pos.x = 11;
	pos.y = 22;

	ui_slider_press(slider, &pos);
	PCUT_ASSERT_TRUE(slider->held);
	PCUT_ASSERT_FALSE(resp.moved);

	pos.x = 21;
	pos.y = 32;

	ui_slider_update(slider, &pos);
	PCUT_ASSERT_TRUE(slider->held);
	PCUT_ASSERT_TRUE(resp.moved);
	PCUT_ASSERT_INT_EQUALS(10, slider->pos);

	pos.x = 31;
	pos.y = 42;

	ui_slider_release(slider, &pos);
	PCUT_ASSERT_FALSE(slider->held);
	PCUT_ASSERT_TRUE(resp.moved);
	PCUT_ASSERT_INT_EQUALS(20, slider->pos);

	ui_slider_destroy(slider);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** ui_pos_event() correctly translates POS_PRESS/POS_RELEASE */
PCUT_TEST(pos_event_press_release)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_slider_t *slider;
	ui_evclaim_t claim;
	pos_event_t event;
	gfx_rect_t rect;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_slider_create(resource, "Hello", &slider);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_FALSE(slider->held);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 30;
	rect.p1.y = 40;
	ui_slider_set_rect(slider, &rect);

	/* Press outside is not claimed and does nothing */
	event.type = POS_PRESS;
	event.hpos = 1;
	event.vpos = 2;
	claim = ui_slider_pos_event(slider, &event);
	PCUT_ASSERT_FALSE(slider->held);
	PCUT_ASSERT_EQUALS(ui_unclaimed, claim);

	/* Press inside is claimed and depresses slider */
	event.type = POS_PRESS;
	event.hpos = 11;
	event.vpos = 22;
	claim = ui_slider_pos_event(slider, &event);
	PCUT_ASSERT_TRUE(slider->held);
	PCUT_ASSERT_EQUALS(ui_claimed, claim);

	/* Release outside (or anywhere) is claimed and relases slider */
	event.type = POS_RELEASE;
	event.hpos = 41;
	event.vpos = 32;
	claim = ui_slider_pos_event(slider, &event);
	PCUT_ASSERT_FALSE(slider->held);
	PCUT_ASSERT_EQUALS(ui_claimed, claim);

	ui_slider_destroy(slider);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** ui_slider_length() correctly determines slider length */
PCUT_TEST(length)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_slider_t *slider;
	gfx_coord_t length;
	gfx_rect_t rect;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_slider_create(resource, "Hello", &slider);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_FALSE(slider->held);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 110;
	rect.p1.y = 120;
	ui_slider_set_rect(slider, &rect);

	length = ui_slider_length(slider);
	PCUT_ASSERT_INT_EQUALS(110 - 10 - 15, length);

	ui_slider_destroy(slider);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

static errno_t testgc_set_clip_rect(void *arg, gfx_rect_t *rect)
{
	(void) arg;
	(void) rect;
	return EOK;
}

static errno_t testgc_set_color(void *arg, gfx_color_t *color)
{
	(void) arg;
	(void) color;
	return EOK;
}

static errno_t testgc_fill_rect(void *arg, gfx_rect_t *rect)
{
	(void) arg;
	(void) rect;
	return EOK;
}

static errno_t testgc_update(void *arg)
{
	(void) arg;
	return EOK;
}

static errno_t testgc_bitmap_create(void *arg, gfx_bitmap_params_t *params,
    gfx_bitmap_alloc_t *alloc, void **rbm)
{
	test_gc_t *tgc = (test_gc_t *) arg;
	testgc_bitmap_t *tbm;

	tbm = calloc(1, sizeof(testgc_bitmap_t));
	if (tbm == NULL)
		return ENOMEM;

	if (alloc == NULL) {
		tbm->alloc.pitch = (params->rect.p1.x - params->rect.p0.x) *
		    sizeof(uint32_t);
		tbm->alloc.off0 = 0;
		tbm->alloc.pixels = calloc(sizeof(uint32_t),
		    (params->rect.p1.x - params->rect.p0.x) *
		    (params->rect.p1.y - params->rect.p0.y));
		tbm->myalloc = true;
		if (tbm->alloc.pixels == NULL) {
			free(tbm);
			return ENOMEM;
		}
	} else {
		tbm->alloc = *alloc;
	}

	tbm->tgc = tgc;
	tgc->bm_created = true;
	tgc->bm_params = *params;
	tgc->bm_pixels = tbm->alloc.pixels;
	*rbm = (void *)tbm;
	return EOK;
}

static errno_t testgc_bitmap_destroy(void *bm)
{
	testgc_bitmap_t *tbm = (testgc_bitmap_t *)bm;
	if (tbm->myalloc)
		free(tbm->alloc.pixels);
	tbm->tgc->bm_destroyed = true;
	free(tbm);
	return EOK;
}

static errno_t testgc_bitmap_render(void *bm, gfx_rect_t *srect,
    gfx_coord2_t *offs)
{
	testgc_bitmap_t *tbm = (testgc_bitmap_t *)bm;
	tbm->tgc->bm_rendered = true;
	tbm->tgc->bm_srect = *srect;
	tbm->tgc->bm_offs = *offs;
	return EOK;
}

static errno_t testgc_bitmap_get_alloc(void *bm, gfx_bitmap_alloc_t *alloc)
{
	testgc_bitmap_t *tbm = (testgc_bitmap_t *)bm;
	*alloc = tbm->alloc;
	tbm->tgc->bm_got_alloc = true;
	return EOK;
}

static void test_slider_moved(ui_slider_t *slider, void *arg, gfx_coord_t pos)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->moved = true;
	resp->pos = pos;
}

PCUT_EXPORT(slider);
