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

/** @addtogroup nav
 * @{
 */
/**
 * @file Navigator panel
 */

#ifndef PANEL_H
#define PANEL_H

#include <errno.h>
#include <gfx/coord.h>
#include <io/kbd_event.h>
#include <io/pos_event.h>
#include <ui/control.h>
#include <ui/window.h>
#include <stdbool.h>
#include "types/panel.h"

extern errno_t panel_create(ui_window_t *, bool, panel_t **);
extern void panel_destroy(panel_t *);
extern void panel_set_cb(panel_t *, panel_cb_t *, void *);
extern errno_t panel_entry_paint(panel_entry_t *, size_t);
extern errno_t panel_paint(panel_t *);
extern ui_evclaim_t panel_kbd_event(panel_t *, kbd_event_t *);
extern ui_evclaim_t panel_pos_event(panel_t *, pos_event_t *);
extern ui_control_t *panel_ctl(panel_t *);
extern void panel_set_rect(panel_t *, gfx_rect_t *);
extern unsigned panel_page_size(panel_t *);
extern bool panel_is_active(panel_t *);
extern errno_t panel_activate(panel_t *);
extern void panel_deactivate(panel_t *);
extern void panel_entry_attr_init(panel_entry_attr_t *);
extern errno_t panel_entry_append(panel_t *, panel_entry_attr_t *);
extern void panel_entry_delete(panel_entry_t *);
extern void panel_clear_entries(panel_t *);
extern errno_t panel_read_dir(panel_t *, const char *);
extern errno_t panel_sort(panel_t *);
extern int panel_entry_ptr_cmp(const void *, const void *);
extern panel_entry_t *panel_first(panel_t *);
extern panel_entry_t *panel_last(panel_t *);
extern panel_entry_t *panel_next(panel_entry_t *);
extern panel_entry_t *panel_prev(panel_entry_t *);
extern panel_entry_t *panel_page_nth_entry(panel_t *, size_t, size_t *);
extern void panel_cursor_move(panel_t *, panel_entry_t *, size_t);
extern void panel_cursor_up(panel_t *);
extern void panel_cursor_down(panel_t *);
extern void panel_cursor_top(panel_t *);
extern void panel_cursor_bottom(panel_t *);
extern void panel_page_up(panel_t *);
extern void panel_page_down(panel_t *);
extern errno_t panel_open(panel_t *, panel_entry_t *);
extern errno_t panel_open_dir(panel_t *, panel_entry_t *);
extern errno_t panel_open_file(panel_t *, panel_entry_t *);
extern void panel_activate_req(panel_t *);

#endif

/** @}
 */
