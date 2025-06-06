/*****************************************************************************
 * Plus42 -- an enhanced HP-42S calculator simulator
 * Copyright (C) 2004-2025  Thomas Okken
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see http://www.gnu.org/licenses/.
 *****************************************************************************/

#ifdef __OBJC__

#import <Cocoa/Cocoa.h>

void skin_menu_update(NSMenu *skinMenu);
void skin_load(long *width, long *height, int *rows, int *cols, int *flags);

#define KEYMAP_MAX_MACRO_LENGTH 31
struct keymap_entry {
    bool ctrl;
    bool alt;
    bool numpad;
    bool shift; 
    bool cshift; 
    unsigned short keychar;
    unsigned char macro[KEYMAP_MAX_MACRO_LENGTH + 1];
};
keymap_entry *parse_keymap_entry(char *line, int lineno);

void skin_repaint(NSRect *rect, bool shortcuts);
void skin_update_annunciator(int which, int state);
void skin_find_key(int x, int y, bool cshift, int *key, int *code);
int skin_find_skey(int ckey, bool cshift);
unsigned char *skin_find_macro(int ckey, int *type);
unsigned char *skin_keymap_lookup(unsigned short keychar, bool printable,
                  bool ctrl, bool alt, bool numpad, bool shift, bool cshift,
                  bool *exact);
void skin_set_pressed_key(int skey);
void skin_display_blitter(const char *bits, int bytesperline, int x, int y,
                                 int width, int height);
void skin_repaint_display();
void skin_get_size(int *width, int *height);

#endif

struct SkinColor {
    unsigned char r, g, b, pad;
};

#define IMGTYPE_MONO 1
#define IMGTYPE_GRAY 2
#define IMGTYPE_COLORMAPPED 3
#define IMGTYPE_TRUECOLOR 4

int skin_getchar();
void skin_rewind();
bool skin_init_image(int type, int ncolors, const SkinColor *colors,
                     int width, int height);
void skin_put_pixels(unsigned const char *data);
void skin_finish_image();

