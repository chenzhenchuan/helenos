/*
 * Copyright (c) 2022 Jiri Svoboda
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

/** @addtogroup libui
 * @{
 */
/**
 * @file Menu
 */

#include <adt/list.h>
#include <errno.h>
#include <gfx/color.h>
#include <gfx/context.h>
#include <gfx/render.h>
#include <gfx/text.h>
#include <io/pos_event.h>
#include <stdlib.h>
#include <str.h>
#include <ui/control.h>
#include <ui/paint.h>
#include <ui/popup.h>
#include <ui/menu.h>
#include <ui/menubar.h>
#include <ui/menuentry.h>
#include <ui/resource.h>
#include <ui/window.h>
#include "../private/menubar.h"
#include "../private/menu.h"
#include "../private/resource.h"

enum {
	menu_frame_w = 4,
	menu_frame_h = 4,
	menu_frame_w_text = 2,
	menu_frame_h_text = 1,
	menu_frame_h_margin_text = 1
};

static void ui_menu_popup_close(ui_popup_t *, void *);
static void ui_menu_popup_kbd(ui_popup_t *, void *, kbd_event_t *);
static void ui_menu_popup_pos(ui_popup_t *, void *, pos_event_t *);
static void ui_menu_key_press_unmod(ui_menu_t *, kbd_event_t *);

static ui_popup_cb_t ui_menu_popup_cb = {
	.close = ui_menu_popup_close,
	.kbd = ui_menu_popup_kbd,
	.pos = ui_menu_popup_pos
};

/** Create new menu.
 *
 * @param mbar Menu bar
 * @param caption Caption
 * @param rmenu Place to store pointer to new menu
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ui_menu_create(ui_menu_bar_t *mbar, const char *caption,
    ui_menu_t **rmenu)
{
	ui_menu_t *menu;

	menu = calloc(1, sizeof(ui_menu_t));
	if (menu == NULL)
		return ENOMEM;

	menu->caption = str_dup(caption);
	if (menu->caption == NULL) {
		free(menu);
		return ENOMEM;
	}

	menu->mbar = mbar;
	list_append(&menu->lmenus, &mbar->menus);
	list_initialize(&menu->entries);

	*rmenu = menu;
	return EOK;
}

/** Destroy menu.
 *
 * @param menu Menu or @c NULL
 */
void ui_menu_destroy(ui_menu_t *menu)
{
	ui_menu_entry_t *mentry;

	if (menu == NULL)
		return;

	/* Destroy entries */
	mentry = ui_menu_entry_first(menu);
	while (mentry != NULL) {
		ui_menu_entry_destroy(mentry);
		mentry = ui_menu_entry_first(menu);
	}

	list_remove(&menu->lmenus);
	free(menu->caption);
	free(menu);
}

/** Get first menu in menu bar.
 *
 * @param mbar Menu bar
 * @return First menu or @c NULL if there is none
 */
ui_menu_t *ui_menu_first(ui_menu_bar_t *mbar)
{
	link_t *link;

	link = list_first(&mbar->menus);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, ui_menu_t, lmenus);
}

/** Get next menu in menu bar.
 *
 * @param cur Current menu
 * @return Next menu or @c NULL if @a cur is the last one
 */
ui_menu_t *ui_menu_next(ui_menu_t *cur)
{
	link_t *link;

	link = list_next(&cur->lmenus, &cur->mbar->menus);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, ui_menu_t, lmenus);
}

/** Get last menu in menu bar.
 *
 * @param mbar Menu bar
 * @return Last menu or @c NULL if there is none
 */
ui_menu_t *ui_menu_last(ui_menu_bar_t *mbar)
{
	link_t *link;

	link = list_last(&mbar->menus);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, ui_menu_t, lmenus);
}

/** Get previous menu in menu bar.
 *
 * @param cur Current menu
 * @return Previous menu or @c NULL if @a cur is the fist one
 */
ui_menu_t *ui_menu_prev(ui_menu_t *cur)
{
	link_t *link;

	link = list_prev(&cur->lmenus, &cur->mbar->menus);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, ui_menu_t, lmenus);
}

/** Get menu caption.
 *
 * @param menu Menu
 * @return Caption (owned by @a menu)
 */
const char *ui_menu_caption(ui_menu_t *menu)
{
	return menu->caption;
}

/** Get menu geometry.
 *
 * @param menu Menu
 * @param spos Starting position
 * @param geom Structure to fill in with computed geometry
 */
void ui_menu_get_geom(ui_menu_t *menu, gfx_coord2_t *spos,
    ui_menu_geom_t *geom)
{
	ui_resource_t *res;
	gfx_coord2_t edim;
	gfx_coord_t frame_w;
	gfx_coord_t frame_h;

	res = ui_window_get_res(menu->mbar->window);

	if (res->textmode) {
		frame_w = menu_frame_w_text;
		frame_h = menu_frame_h_text;
	} else {
		frame_w = menu_frame_w;
		frame_h = menu_frame_h;
	}

	edim.x = ui_menu_entry_calc_width(menu, menu->max_caption_w,
	    menu->max_shortcut_w);
	edim.y = menu->total_h;

	geom->outer_rect.p0 = *spos;
	geom->outer_rect.p1.x = spos->x + edim.x + 2 * frame_w;
	geom->outer_rect.p1.y = spos->y + edim.y + 2 * frame_h;

	geom->entries_rect.p0.x = spos->x + frame_w;
	geom->entries_rect.p0.y = spos->y + frame_h;
	geom->entries_rect.p1.x = geom->entries_rect.p0.x + edim.x;
	geom->entries_rect.p1.y = geom->entries_rect.p0.x + edim.y;
}

/** Get menu rectangle.
 *
 * @param menu Menu
 * @param spos Starting position (top-left corner)
 * @param rect Place to store menu rectangle
 */
void ui_menu_get_rect(ui_menu_t *menu, gfx_coord2_t *spos, gfx_rect_t *rect)
{
	ui_menu_geom_t geom;

	ui_menu_get_geom(menu, spos, &geom);
	*rect = geom.outer_rect;
}

/** Get UI resource from menu.
 *
 * @param menu Menu
 * @return UI resource
 */
ui_resource_t *ui_menu_get_res(ui_menu_t *menu)
{
	return ui_popup_get_res(menu->popup);
}

/** Open menu.
 *
 * @param menu Menu
 * @param prect Parent rectangle around which the menu should be placed
 */
errno_t ui_menu_open(ui_menu_t *menu, gfx_rect_t *prect)
{
	ui_popup_t *popup = NULL;
	ui_popup_params_t params;
	ui_menu_geom_t geom;
	gfx_coord2_t mpos;
	errno_t rc;

	/* Select first entry */
	menu->selected = ui_menu_entry_first(menu);

	/* Determine menu dimensions */

	mpos.x = 0;
	mpos.y = 0;
	ui_menu_get_geom(menu, &mpos, &geom);

	ui_popup_params_init(&params);
	params.rect = geom.outer_rect;
	params.place = *prect;

	rc = ui_popup_create(menu->mbar->ui, menu->mbar->window, &params,
	    &popup);
	if (rc != EOK)
		return rc;

	menu->popup = popup;
	ui_popup_set_cb(popup, &ui_menu_popup_cb, menu);

	return ui_menu_paint(menu, &mpos);
}

/** Close menu.
 *
 * @param menu Menu
 */
void ui_menu_close(ui_menu_t *menu)
{
	ui_popup_destroy(menu->popup);
	menu->popup = NULL;
}

/** Determine if menu is open.
 *
 * @param menu Menu
 * @return @c true iff menu is open
 */
bool ui_menu_is_open(ui_menu_t *menu)
{
	return menu->popup != NULL;
}

/** Paint menu.
 *
 * @param menu Menu
 * @param spos Starting position (top-left corner)
 * @return EOK on success or an error code
 */
errno_t ui_menu_paint_bg_gfx(ui_menu_t *menu, gfx_coord2_t *spos)
{
	ui_resource_t *res;
	ui_menu_geom_t geom;
	gfx_rect_t bg_rect;
	errno_t rc;

	res = ui_menu_get_res(menu);
	ui_menu_get_geom(menu, spos, &geom);

	/* Paint menu frame */

	rc = gfx_set_color(res->gc, res->wnd_face_color);
	if (rc != EOK)
		goto error;

	rc = ui_paint_outset_frame(res, &geom.outer_rect, &bg_rect);
	if (rc != EOK)
		goto error;

	/* Paint menu background */

	rc = gfx_set_color(res->gc, res->wnd_face_color);
	if (rc != EOK)
		goto error;

	rc = gfx_fill_rect(res->gc, &bg_rect);
	if (rc != EOK)
		goto error;

	return EOK;
error:
	return rc;
}

/** Paint menu.
 *
 * @param menu Menu
 * @param spos Starting position (top-left corner)
 * @return EOK on success or an error code
 */
errno_t ui_menu_paint_bg_text(ui_menu_t *menu, gfx_coord2_t *spos)
{
	ui_resource_t *res;
	ui_menu_geom_t geom;
	gfx_rect_t rect;
	errno_t rc;

	res = ui_menu_get_res(menu);
	ui_menu_get_geom(menu, spos, &geom);

	/* Paint menu background */

	rc = gfx_set_color(res->gc, res->wnd_face_color);
	if (rc != EOK)
		goto error;

	rc = gfx_fill_rect(res->gc, &geom.outer_rect);
	if (rc != EOK)
		goto error;

	/* Paint menu box */

	rect = geom.outer_rect;
	rect.p0.x += menu_frame_h_margin_text;
	rect.p1.x -= menu_frame_h_margin_text;

	rc = ui_paint_text_box(res, &rect, ui_box_single, res->wnd_face_color);
	if (rc != EOK)
		goto error;

	return EOK;
error:
	return rc;
}

/** Paint menu.
 *
 * @param menu Menu
 * @param spos Starting position (top-left corner)
 * @return EOK on success or an error code
 */
errno_t ui_menu_paint(ui_menu_t *menu, gfx_coord2_t *spos)
{
	ui_resource_t *res;
	gfx_coord2_t pos;
	ui_menu_entry_t *mentry;
	ui_menu_geom_t geom;
	errno_t rc;

	res = ui_menu_get_res(menu);
	ui_menu_get_geom(menu, spos, &geom);

	/* Paint menu frame and background */
	if (res->textmode)
		rc = ui_menu_paint_bg_text(menu, spos);
	else
		rc = ui_menu_paint_bg_gfx(menu, spos);
	if (rc != EOK)
		goto error;

	/* Paint entries */
	pos = geom.entries_rect.p0;

	mentry = ui_menu_entry_first(menu);
	while (mentry != NULL) {
		rc = ui_menu_entry_paint(mentry, &pos);
		if (rc != EOK)
			goto error;

		pos.y += ui_menu_entry_height(mentry);
		mentry = ui_menu_entry_next(mentry);
	}

	rc = gfx_update(res->gc);
	if (rc != EOK)
		goto error;

	return EOK;
error:
	return rc;
}

/** Handle position event in menu.
 *
 * @param menu Menu
 * @param spos Starting position (top-left corner)
 * @param event Position event
 * @return ui_claimed iff the event was claimed
 */
ui_evclaim_t ui_menu_pos_event(ui_menu_t *menu, gfx_coord2_t *spos,
    pos_event_t *event)
{
	ui_menu_geom_t geom;
	ui_menu_entry_t *mentry;
	gfx_coord2_t pos;
	gfx_coord2_t epos;
	ui_evclaim_t claimed;

	ui_menu_get_geom(menu, spos, &geom);
	epos.x = event->hpos;
	epos.y = event->vpos;

	pos = geom.entries_rect.p0;

	mentry = ui_menu_entry_first(menu);
	while (mentry != NULL) {
		claimed = ui_menu_entry_pos_event(mentry, &pos, event);
		if (claimed == ui_claimed)
			return ui_claimed;

		pos.y += ui_menu_entry_height(mentry);
		mentry = ui_menu_entry_next(mentry);
	}

	/* Event inside menu? */
	if (gfx_pix_inside_rect(&epos, &geom.outer_rect)) {
		/* Claim event */
		return ui_claimed;
	} else {
		/* Press outside menu - close it */
		if (event->type == POS_PRESS)
			ui_menu_bar_deactivate(menu->mbar);
	}

	return ui_unclaimed;
}

/** Handle keyboard event in menu.
 *
 * @param menu Menu
 * @param event Keyboard event
 * @return ui_claimed iff the event was claimed
 */
ui_evclaim_t ui_menu_kbd_event(ui_menu_t *menu, kbd_event_t *event)
{
	if (event->type == KEY_PRESS && (event->mods &
	    (KM_CTRL | KM_ALT | KM_SHIFT)) == 0) {
		ui_menu_key_press_unmod(menu, event);
	}

	return ui_claimed;
}

/** Move one entry up.
 *
 * Non-selectable entries are skipped. If we are already at the top,
 * we wrap around.
 *
 * @param menu Menu
 */
void ui_menu_up(ui_menu_t *menu)
{
	gfx_coord2_t mpos;
	ui_menu_entry_t *nentry;

	if (menu->selected == NULL)
		return;

	nentry = ui_menu_entry_prev(menu->selected);
	if (nentry == NULL)
		nentry = ui_menu_entry_last(menu);

	/* Need to find a selectable entry */
	while (!ui_menu_entry_selectable(nentry)) {
		nentry = ui_menu_entry_prev(nentry);
		if (nentry == NULL)
			nentry = ui_menu_entry_last(menu);

		/* Went completely around and found nothing? */
		if (nentry == menu->selected)
			return;
	}

	menu->selected = nentry;

	mpos.x = 0;
	mpos.y = 0;
	(void) ui_menu_paint(menu, &mpos);
}

/** Move one entry down.
 *
 * Non-selectable entries are skipped. If we are already at the bottom,
 * we wrap around.
 *
 * @param menu Menu
 */
void ui_menu_down(ui_menu_t *menu)
{
	gfx_coord2_t mpos;
	ui_menu_entry_t *nentry;

	if (menu->selected == NULL)
		return;

	nentry = ui_menu_entry_next(menu->selected);
	if (nentry == NULL)
		nentry = ui_menu_entry_first(menu);

	/* Need to find a selectable entry */
	while (!ui_menu_entry_selectable(nentry)) {
		nentry = ui_menu_entry_next(nentry);
		if (nentry == NULL)
			nentry = ui_menu_entry_first(menu);

		/* Went completely around and found nothing? */
		if (nentry == menu->selected)
			return;
	}

	menu->selected = nentry;

	mpos.x = 0;
	mpos.y = 0;
	(void) ui_menu_paint(menu, &mpos);
}

/** Handle key press without modifiers in menu popup window.
 *
 * @param menu Menu
 * @param event Keyboard event
 */
static void ui_menu_key_press_unmod(ui_menu_t *menu, kbd_event_t *event)
{
	switch (event->key) {
	case KC_ESCAPE:
		ui_menu_bar_deactivate(menu->mbar);
		break;
	case KC_LEFT:
		ui_menu_bar_left(menu->mbar);
		break;
	case KC_RIGHT:
		ui_menu_bar_right(menu->mbar);
		break;
	case KC_UP:
		ui_menu_up(menu);
		break;
	case KC_DOWN:
		ui_menu_down(menu);
		break;
	case KC_ENTER:
		if (menu->selected != NULL)
			ui_menu_entry_activate(menu->selected);
		break;
	default:
		break;
	}
}

/** Handle close event in menu popup window.
 *
 * @param popup Menu popup window
 * @param arg Argument (ui_menu_t *)
 */
static void ui_menu_popup_close(ui_popup_t *popup, void *arg)
{
	ui_menu_t *menu = (ui_menu_t *)arg;

	/* Deactivate menu bar, close menu */
	ui_menu_bar_deactivate(menu->mbar);
}

/** Handle keyboard event in menu popup window.
 *
 * @param popup Menu popup window
 * @param arg Argument (ui_menu_t *)
 * @param event Keyboard event
 */
static void ui_menu_popup_kbd(ui_popup_t *popup, void *arg, kbd_event_t *event)
{
	ui_menu_t *menu = (ui_menu_t *)arg;

	ui_menu_kbd_event(menu, event);
}

/** Handle position event in menu popup window.
 *
 * @param popup Menu popup window
 * @param arg Argument (ui_menu_t *)
 * @param event Position event
 */
static void ui_menu_popup_pos(ui_popup_t *popup, void *arg, pos_event_t *event)
{
	ui_menu_t *menu = (ui_menu_t *)arg;
	gfx_coord2_t spos;

	spos.x = 0;
	spos.y = 0;
	ui_menu_pos_event(menu, &spos, event);
}

/** @}
 */
