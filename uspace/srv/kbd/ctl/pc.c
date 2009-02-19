/*
 * Copyright (c) 2009 Jiri Svoboda
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

/** @addtogroup kbd_ctl
 * @ingroup kbd
 * @{
 */
/**
 * @file
 * @brief	PC keyboard controller driver.
 */

#include <kbd.h>
#include <kbd/kbd.h>
#include <kbd/keycode.h>
#include <kbd_ctl.h>

static int scanmap_simple[];

void kbd_ctl_parse_scancode(int scancode)
{
	kbd_ev_type_t type;
	unsigned int key;

	if (scancode < 0 || scancode >= 0x100)
		return;

	if (scancode & 0x80) {
		scancode &= ~0x80;
		type = KE_RELEASE;
	} else {
		type = KE_PRESS;
	}

	key = scanmap_simple[scancode];
	if (key != 0)
		kbd_push_ev(type, key);
}

static int scanmap_simple[128] = {

	[0x29] = KC_BACKTICK,

	[0x02] = KC_1,
	[0x03] = KC_2,
	[0x04] = KC_3,
	[0x05] = KC_4,
	[0x06] = KC_5,
	[0x07] = KC_6,
	[0x08] = KC_7,
	[0x09] = KC_8,
	[0x0a] = KC_9,
	[0x0b] = KC_0,

	[0x0c] = KC_MINUS,
	[0x0d] = KC_EQUALS,
	[0x0e] = KC_BACKSPACE,

	[0x0f] = KC_TAB,

	[0x10] = KC_Q,
	[0x11] = KC_W,
	[0x12] = KC_E,
	[0x13] = KC_R,
	[0x14] = KC_T,
	[0x15] = KC_Y,
	[0x16] = KC_U,
	[0x17] = KC_I,
	[0x18] = KC_O,
	[0x19] = KC_P,

	[0x1a] = KC_LBRACKET,
	[0x1b] = KC_RBRACKET,

	[0x3a] = KC_CAPS_LOCK,

	[0x1e] = KC_A,
	[0x1f] = KC_S,
	[0x20] = KC_D,
	[0x21] = KC_F,
	[0x22] = KC_G,
	[0x23] = KC_H,
	[0x24] = KC_J,
	[0x25] = KC_K,
	[0x26] = KC_L,

	[0x27] = KC_SEMICOLON,
	[0x28] = KC_QUOTE,
	[0x2b] = KC_BACKSLASH,

	[0x2a] = KC_LSHIFT,

	[0x2c] = KC_Z,
	[0x2d] = KC_X,
	[0x2e] = KC_C,
	[0x2f] = KC_V,
	[0x30] = KC_B,
	[0x31] = KC_N,
	[0x32] = KC_M,

	[0x33] = KC_COMMA,
	[0x34] = KC_PERIOD,
	[0x35] = KC_SLASH,

	[0x36] = KC_RSHIFT,

	[0x1d] = KC_LCTRL,
	[0x38] = KC_LALT,
	[0x39] = KC_SPACE,

	[0x01] = KC_ESCAPE,

	[0x3b] = KC_F1,
	[0x3c] = KC_F2,
	[0x3d] = KC_F3,
	[0x3e] = KC_F4,
	[0x3f] = KC_F5,
	[0x40] = KC_F6,
	[0x41] = KC_F7,

	[0x42] = KC_F8,
	[0x43] = KC_F9,
	[0x44] = KC_F10,

	[0x57] = KC_F11,
	[0x58] = KC_F12,

	[0x1c] = KC_ENTER

/*
	[0x1] = KC_PRNSCR,
	[0x1] = KC_SCROLL_LOCK,
	[0x1] = KC_PAUSE,
*/
};

/**
 * @}
 */ 
