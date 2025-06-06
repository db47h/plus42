///////////////////////////////////////////////////////////////////////////////
// Plus42 -- an enhanced HP-42S calculator simulator
// Copyright (C) 2004-2025  Thomas Okken
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 2,
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see http://www.gnu.org/licenses/.
///////////////////////////////////////////////////////////////////////////////

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <errno.h>
#include <locale.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>

#include "shell.h"
#include "shell_main.h"
#include "shell_skin.h"
#include "shell_spool.h"
#include "core_main.h"
#include "core_display.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
#include "icon-128x128.xpm"
#include "icon-48x48.xpm"
#pragma GCC diagnostic pop

#ifndef _POSIX_HOST_NAME_MAX
#define _POSIX_HOST_NAME_MAX 255
#endif
#ifndef _POSIX_LOGIN_NAME_MAX
#define _POSIX_LOGIN_NAME_MAX 255
#endif

#ifdef AUDIO_ALSA
#include "audio_alsa.h"
#endif


/* These are global because the skin code uses them a lot */

GtkWidget *calc_widget = NULL;
GtkWidget *mainwindow;
bool allow_paint = false;
int menu_bar_height = -1;
int disp_rows;
int disp_cols;

state_type state;
char free42dirname[FILENAMELEN];


/* PRINT_LINES is limited to an even lower value than in the Motif version.
 * It appears that GTK does not allow the SUM of a widget's height and its
 * container's height to exceed 32k, which means that as the print widget's
 * height approaches that magical number, the top-level window is forced to
 * become shorter and shorter. So, I set the limit at 30000 instead of
 * 32768; if we're reading a print-out file from Plus42/Motif which has more
 * than 30000 pixels, we chop off the excess from the top.
 */
#define PRINT_LINES 30000
#define PRINT_BYTESPERLINE 36
#define PRINT_SIZE 1080000
// Room for PRINT_LINES / 18 lines, plus two, plus one byte
#define PRINT_TEXT_SIZE 41726


static unsigned char *print_bitmap;
static int printout_top;
static int printout_bottom;
static unsigned char *print_text;
static int print_text_top;
static int print_text_bottom;
static int print_text_pixel_height;
static bool quit_flag = false;
static bool enqueued;


/* Private globals */

static FILE *print_txt = NULL;
static FILE *print_gif = NULL;
static char print_gif_name[FILENAMELEN];
static int gif_seq = -1;
static int gif_lines;

static int pype[2];

static GtkApplication *app = NULL;
static GtkWidget *printwindow;
static GtkWidget *print_widget;
//static GdkGC *print_gc = NULL;
static GtkAdjustment *print_adj;
static GdkPixbuf *icon_128;
static GdkPixbuf *icon_48;
static guint resize_timeout_id = 0;

int ckey = 0;
int skey = -1;
static unsigned char *macro;
static int macro_type;
static bool mouse_key;
static guint16 active_keycode = 0;
static bool just_pressed_shift = false;
static guint timeout_id = 0;
static guint timeout3_id = 0;

static int keymap_length = 0;
static keymap_entry *keymap = NULL;

static guint reminder_id = 0;
static FILE *statefile = NULL;
static char statefilename[FILENAMELEN];
static char printfilename[FILENAMELEN];
static char keymapfilename[FILENAMELEN];

int ann_updown = 0;
int ann_shift = 0;
int ann_print = 0;
int ann_run = 0;
int ann_battery = 0;
int ann_g = 0;
int ann_rad = 0;
static guint ann_print_timeout_id = 0;

void get_keymap(keymap_entry **map, int *length) {
    *map = keymap;
    *length = keymap_length;
}

static bool keyboardShortcutsShowing = false;
static GtkCheckMenuItem *keyboardShortcutsMenuItem;


/* Private functions */

static void read_key_map(const char *keymapfilename);
static void init_shell_state(int4 version);
static int read_shell_state();
static int write_shell_state();
static void int_term_handler(int sig);
static gboolean gt_signal_handler(GIOChannel *source, GIOCondition condition,
                                                            gpointer data);
static void quit();
static char *strclone(const char *s);
static bool file_exists(const char *name);
static void show_message(const char *title, const char *message, GtkWidget *parent = mainwindow);
static void no_mwm_resize_borders(GtkWidget *window);
static void no_mwm_zoom_box(GtkWidget *window);
static void scroll_printout_to_bottom();
static void quitCB();
static void statesCB();
static void showPrintOutCB();
static void exportProgramCB();
static GtkWidget *make_file_select_dialog(
        const char *title, const char *pattern, bool save, GtkWidget *owner);
static void importProgramCB();
static void paperAdvanceCB();
static void copyPrintAsTextCB();
static void copyPrintAsImageCB();
static void clearPrintOutCB();
static void preferencesCB();
static void appendSuffix(char *path, const char *suffix);
static void copyCB();
static void pasteCB();
static void documentationCB();
static void websiteCB();
static void otherWebsiteCB();
static void keyboardShortcutsCB();
static void editKeymapCB();
static void aboutCB();
static gboolean delete_cb(GtkWidget *w, GdkEventAny *ev);
static gboolean delete_print_cb(GtkWidget *w, GdkEventAny *ev);
static gboolean draw_cb(GtkWidget *w, cairo_t *cr, gpointer cd);
static gboolean print_draw_cb(GtkWidget *w, cairo_t *cr, gpointer cd);
static gboolean print_key_cb(GtkWidget *w, GdkEventKey *event, gpointer cd);
static gboolean button_cb(GtkWidget *w, GdkEventButton *event, gpointer cd);
static gboolean key_cb(GtkWidget *w, GdkEventKey *event, gpointer cd);
static void enable_reminder();
static void disable_reminder();
static gboolean repeater(gpointer cd);
static gboolean timeout1(gpointer cd);
static gboolean timeout2(gpointer cd);
static gboolean timeout3(gpointer cd);
static gboolean battery_checker(gpointer cd);
static void repaint_printout(cairo_t *cr, bool dark);
static gboolean reminder(gpointer cd);
static void txt_writer(const char *text, int length);
static void txt_newliner();
static void gif_seeker(int4 pos);
static void gif_writer(const char *text, int length);


#ifdef BCD_MATH
#define TITLE "Plus42 Decimal"
#else
#define TITLE "Plus42 Binary"
#endif

static const char *mainWindowXml =
"<?xml version='1.0' encoding='UTF-8'?>"
"<interface>"
  "<!-- interface-requires gtk+ 3.0 -->"
  "<object class='GtkApplicationWindow' id='window'>"
    "<child>"
      "<object class='GtkVBox' id='box'>"
        "<child>"
          "<object class='GtkMenuBar' id='menubar'>"
            "<child>"
              "%s" // Additional menu level goes here when using -compactmenu
              "<object class='GtkMenuItem' id='file_item'>"
                "<property name='label'>File</property>"
                "<child type='submenu'>"
                  "<object class='GtkMenu' id='file_menu'>"
                    "<child>"
                      "<object class='GtkMenuItem' id='states_item'>"
                        "<property name='label'>States...</property>"
                      "</object>"
                    "</child>"
                    "<child>"
                      "<object class='GtkSeparatorMenuItem' id='sep_1'>"
                      "</object>"
                    "</child>"
                    "<child>"
                      "<object class='GtkMenuItem' id='show_printout_item'>"
                        "<property name='label'>Show Print-Out</property>"
                      "</object>"
                    "</child>"
                    "<child>"
                      "<object class='GtkMenuItem' id='paper_advance_item'>"
                        "<property name='label'>Paper Advance</property>"
                        "<accelerator key='A' signal='activate' modifiers='GDK_CONTROL_MASK'/>"
                      "</object>"
                    "</child>"
                    "<child>"
                      "<object class='GtkSeparatorMenuItem' id='sep_2'>"
                      "</object>"
                    "</child>"
                    "<child>"
                      "<object class='GtkMenuItem' id='import_programs_item'>"
                        "<property name='label'>Import Programs...</property>"
                      "</object>"
                    "</child>"
                    "<child>"
                      "<object class='GtkMenuItem' id='export_programs_item'>"
                        "<property name='label'>Export Programs...</property>"
                      "</object>"
                    "</child>"
                    "<child>"
                      "<object class='GtkSeparatorMenuItem' id='sep_3'>"
                      "</object>"
                    "</child>"
                    "<child>"
                      "<object class='GtkMenuItem' id='preferences_item'>"
                        "<property name='label'>Preferences...</property>"
                      "</object>"
                    "</child>"
                    "<child>"
                      "<object class='GtkSeparatorMenuItem' id='sep_4'>"
                      "</object>"
                    "</child>"
                    "<child>"
                      "<object class='GtkMenuItem' id='quit_item'>"
                        "<property name='label'>Quit</property>"
                        "<accelerator key='Q' signal='activate' modifiers='GDK_CONTROL_MASK'/>"
                      "</object>"
                    "</child>"
                  "</object>"
                "</child>"
              "</object>"
            "</child>"
            "<child>"
              "<object class='GtkMenuItem' id='edit_item'>"
                "<property name='label'>Edit</property>"
                "<child type='submenu'>"
                  "<object class='GtkMenu' id='edit_menu'>"
                    "<child>"
                      "<object class='GtkMenuItem' id='copy_item'>"
                        "<property name='label'>Copy</property>"
                        "<accelerator key='C' signal='activate' modifiers='GDK_CONTROL_MASK'/>"
                      "</object>"
                    "</child>"
                    "<child>"
                      "<object class='GtkMenuItem' id='paste_item'>"
                        "<property name='label'>Paste</property>"
                        "<accelerator key='V' signal='activate' modifiers='GDK_CONTROL_MASK'/>"
                      "</object>"
                    "</child>"
                    "<child>"
                      "<object class='GtkSeparatorMenuItem' id='sep_5'>"
                      "</object>"
                    "</child>"
                    "<child>"
                      "<object class='GtkMenuItem' id='copy_printout_as_text_item'>"
                        "<property name='label'>Copy Print-Out as Text</property>"
                        "<accelerator key='T' signal='activate' modifiers='GDK_CONTROL_MASK'/>"
                      "</object>"
                    "</child>"
                    "<child>"
                      "<object class='GtkMenuItem' id='copy_printout_as_image_item'>"
                        "<property name='label'>Copy Print-Out as Image</property>"
                        "<accelerator key='I' signal='activate' modifiers='GDK_CONTROL_MASK'/>"
                      "</object>"
                    "</child>"
                    "<child>"
                      "<object class='GtkMenuItem' id='clear_printout_item'>"
                        "<property name='label'>Clear Print-Out</property>"
                        "<accelerator key='D' signal='activate' modifiers='GDK_CONTROL_MASK'/>"
                      "</object>"
                    "</child>"
                  "</object>"
                "</child>"
              "</object>"
            "</child>"
            "<child>"
              "<object class='GtkMenuItem' id='skin_item'>"
                "<property name='label'>Skin</property>"
                "<child type='submenu'>"
                  "<object class='GtkMenu' id='skin_menu'>"
                    "<!-- Skin items inserted programmatically here -->"
                  "</object>"
                "</child>"
              "</object>"
            "</child>"
            "<child>"
              "<object class='GtkMenuItem' id='help_item'>"
                "<property name='label'>Help</property>"
                "<child type='submenu'>"
                  "<object class='GtkMenu' id='help_menu'>"
                    "<child>"
                      "<object class='GtkMenuItem' id='documentation_item'>"
                        "<property name='label'>Documentation</property>"
                      "</object>"
                    "</child>"
                    "<child>"
                      "<object class='GtkMenuItem' id='website_item'>"
                        "<property name='label'>Web Site</property>"
                      "</object>"
                    "</child>"
                    "<child>"
                      "<object class='GtkMenuItem' id='other_website_item'>"
                        "<property name='label'>Free42 Web Site</property>"
                      "</object>"
                    "</child>"
                    "<child>"
                      "<object class='GtkSeparatorMenuItem' id='sep_6'>"
                      "</object>"
                    "</child>"
                    "<child>"
                      "<object class='GtkCheckMenuItem' id='keyboard_shortcuts_item'>"
                        "<property name='label'>Keyboard Shortcuts</property>"
                        "<accelerator key='K' signal='activate' modifiers='GDK_CONTROL_MASK'/>"
                      "</object>"
                    "</child>"
                    "<child>"
                      "<object class='GtkMenuItem' id='edit_keymap_item'>"
                        "<property name='label'>Edit Keyboard Map</property>"
                      "</object>"
                    "</child>"
                    "<child>"
                      "<object class='GtkSeparatorMenuItem' id='sep_7'>"
                      "</object>"
                    "</child>"
                    "<child>"
                      "<object class='GtkMenuItem' id='about_item'>"
                        "<property name='label'>About Plus42...</property>"
                      "</object>"
                    "</child>"
                  "</object>"
                "</child>"
              "</object>"
              "%s"
            "</child>"
          "</object>"
          "<packing>"
            "<property name='expand'>FALSE</property>"
            "<property name='fill'>TRUE</property>"
          "</packing>"
        "</child>"
      "</object>"
    "</child>"
  "</object>"
"</interface>";

static const char *compactMenuIntroXml =
"<object class='GtkMenuItem' id='top_item'>"
  "<property name='label'>Menu</property>"
  "<child type='submenu'>"
    "<object class='GtkMenu' id='top_menu'>"
      "<child>";

static const char *compactMenuOutroXml =
      "</child>"
    "</object>"
  "</child>"
"</object>";

static int use_compactmenu = 0;
static char *skin_arg = NULL;

static char cached_number_format[9];

static void activate(GtkApplication *theApp, gpointer userData);
static void menubar_resized(GtkWidget *w, GtkAllocation *allocation, gpointer data);
static void calc_resized(GtkWidget *w, GtkAllocation *allocation, gpointer data);

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-skin") == 0)
            skin_arg = ++i < argc ? argv[i] : NULL;
        else if (strcmp(argv[i], "-compactmenu") == 0)
            use_compactmenu = 1;
        else {
            fprintf(stderr, "Unrecognized option: %s\n", argv[i]);
            exit(1);
        }
    }

    GtkApplication *app;
    int status;
    app = gtk_application_new("com.thomasokken.plus42", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), 0, NULL);
    g_object_unref(app);
    return status;
}

static void copy_one_utf8_char(char *dst, const char *src) {
    int n, c = *src & 255;
    if (c == 0)
        return;
    else if ((c & 0x80) == 0)
        n = 1;
    else if ((c & 0xc0) == 0x80)
        return;
    else if ((c & 0xe0) == 0xc0)
        n = 2;
    else if ((c & 0xf0) == 0xe0)
        n = 3;
    else
        return;
    strncat(dst, src, n);
}

static void activate(GtkApplication *theApp, gpointer userData) {

    if (app != NULL) {
        gtk_window_present(GTK_WINDOW(mainwindow));
        gdk_window_focus(gtk_widget_get_window(mainwindow), GDK_CURRENT_TIME);
        return;
    }

    app = theApp;

    // Capture number format, which may have been changed by
    // gtk_init(), and then set it to the C locale, because the binary/decimal
    // conversions expect a decimal point, not a comma.
    struct lconv *loc = localeconv();
    cached_number_format[0] = 0;
    copy_one_utf8_char(cached_number_format, loc->decimal_point);
    if (loc->thousands_sep[0] != 0) {
        int g1 = loc->grouping[0];
        if (g1 != 0) {
            int g2 = loc->grouping[1];
            if (g2 == 0)
                g2 = g1;
            copy_one_utf8_char(cached_number_format, loc->thousands_sep);
            char g[2];
            g[0] = '0' + g1;
            g[1] = '0' + g2;
            strncat(cached_number_format, g, 2);
        }
    }
    setlocale(LC_NUMERIC, "C");


    /*************************************************************/
    /***** Try to create the $XDG_DATA_HOME/plus42 directory *****/
    /*************************************************************/

    char *xdg_data_home = getenv("XDG_DATA_HOME");
    char *home = getenv("HOME");

    if (xdg_data_home == NULL || xdg_data_home[0] == 0)
        snprintf(free42dirname, FILENAMELEN, "%s/.local/share/plus42", home);
    else
        snprintf(free42dirname, FILENAMELEN, "%s/plus42", xdg_data_home);

    if (!file_exists(free42dirname)) {
        // The Plus42 directory does not exist yet. Before trying to do
        // anything else, make sure the Plus42 directory path starts with a slash.
        if (free42dirname[0] != '/') {
            fprintf(stderr, "Fatal: XDG_DATA_HOME or HOME are invalid; must start with '/'\n");
            exit(1);
        }
        // The Plus42 directory does not exist yet. Trying to create it,
        // and all its ancestors. We're not checking for errors here, since
        // either XDG_DATA_HOME or else HOME really should be set to
        // something sane, and besides, trying to create the first few
        // components of this path is *expected* to return errors, because
        // they will already exist.
        char *slash = free42dirname;
        do {
            *slash = '/';
            char *nextSlash = strchr(slash + 1, '/');
            if (nextSlash != NULL)
                *nextSlash = 0;
            mkdir(free42dirname, 0755);
            slash = nextSlash;
        } while (slash != NULL);
    }

    snprintf(statefilename, FILENAMELEN, "%s/state", free42dirname);
    snprintf(printfilename, FILENAMELEN, "%s/print", free42dirname);
    snprintf(keymapfilename, FILENAMELEN, "%s/keymap.txt", free42dirname);

    if (!file_exists(keymapfilename)) {
        char oldkeymapfilename[FILENAMELEN];
        strcpy(oldkeymapfilename, keymapfilename);
        oldkeymapfilename[strlen(oldkeymapfilename) - 4] = 0;
        if (file_exists(oldkeymapfilename))
            rename(oldkeymapfilename, keymapfilename);
    }


    /****************************/
    /***** Read the key map *****/
    /****************************/

    read_key_map(keymapfilename);


    /***********************************************************/
    /***** Open the state file and read the shell settings *****/
    /***********************************************************/

    int init_mode;
    char core_state_file_name[FILENAMELEN];

    statefile = fopen(statefilename, "r");
    if (statefile != NULL) {
        if (read_shell_state()) {
            if (skin_arg != NULL) {
                strncpy(state.skinName, skin_arg, FILENAMELEN - 1);
                state.skinName[FILENAMELEN - 1] = 0;
            }
            fclose(statefile);
            init_mode = 1;
        } else {
            init_shell_state(-1);
            init_mode = 2;
        }
    } else {
        init_shell_state(-1);
        init_mode = 0;
    }
    snprintf(core_state_file_name, FILENAMELEN, "%s/%s.p42", free42dirname, state.coreName);
    if (init_mode != 1) {
        // The shell state was missing or corrupt, but there
        // may still be a valid core state...
        if (file_exists(core_state_file_name))
            // Core state "Untitled.p42" exists; let's try to read it
            init_mode = 1;
    }

    int rows = 8, cols = 22;
    core_init(&rows, &cols, init_mode, core_state_file_name);

    /*********************************/
    /***** Build the main window *****/
    /*********************************/

    char *xml = (char *) malloc(10240);
    if (use_compactmenu)
        snprintf(xml, 10240, mainWindowXml, compactMenuIntroXml, compactMenuOutroXml);
    else
        snprintf(xml, 10240, mainWindowXml, "", "");
    GtkBuilder *builder = gtk_builder_new();
    gtk_builder_add_from_string(builder, xml, -1, NULL);
    free(xml);
    GObject *obj = gtk_builder_get_object(builder, "window");
    mainwindow = GTK_WIDGET(obj);
    gtk_window_set_application(GTK_WINDOW(mainwindow), app);

    icon_128 = gdk_pixbuf_new_from_xpm_data((const char **) icon_128_xpm);
    icon_48 = gdk_pixbuf_new_from_xpm_data((const char **) icon_48_xpm);

    gtk_window_set_icon(GTK_WINDOW(mainwindow), icon_128);
    gtk_window_set_title(GTK_WINDOW(mainwindow), TITLE);
    gtk_window_set_role(GTK_WINDOW(mainwindow), "Plus42 Calculator");
    //gtk_window_set_resizable(GTK_WINDOW(mainwindow), FALSE);
    no_mwm_zoom_box(mainwindow);
    g_signal_connect(G_OBJECT(mainwindow), "delete_event",
                     G_CALLBACK(delete_cb), NULL);
    if (state.mainWindowKnown)
        gtk_window_move(GTK_WINDOW(mainwindow), state.mainWindowX,
                                            state.mainWindowY);

    GtkWidget *menubar = GTK_WIDGET(gtk_builder_get_object(builder, "menubar"));
    g_signal_connect(G_OBJECT(menubar), "size-allocate", G_CALLBACK(menubar_resized), NULL);

    // The "Skin" menu is dynamic; we don't populate any items in it here.
    // Instead, we attach a callback which scans the .free42 directory for
    // available skins; this callback is invoked when the menu is about to
    // be mapped.
    GtkMenuItem *item = GTK_MENU_ITEM(gtk_builder_get_object(builder, "skin_item"));
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(skin_menu_update), NULL);

    // With GTK 2 and GTK 3.4, the above logic worked fine, but with 3.24,
    // it appears that the pop-up shell is laid out *before* the 'activate'
    // callback is invoked. The result is that you do end up with the correct
    // menu items, but they don't fit in the pop-up and so are cut off.
    // Can't think of a proper way around this, but this at least will fix the
    // Skin menu appearance in the most common use case, i.e. when the set of
    // skins does not change while Plus42 is running.
    skin_menu_update(GTK_WIDGET(item));

    item = GTK_MENU_ITEM(gtk_builder_get_object(builder, "states_item"));
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(statesCB), NULL);
    item = GTK_MENU_ITEM(gtk_builder_get_object(builder, "show_printout_item"));
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(showPrintOutCB), NULL);
    item = GTK_MENU_ITEM(gtk_builder_get_object(builder, "paper_advance_item"));
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(paperAdvanceCB), NULL);
    item = GTK_MENU_ITEM(gtk_builder_get_object(builder, "import_programs_item"));
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(importProgramCB), NULL);
    item = GTK_MENU_ITEM(gtk_builder_get_object(builder, "export_programs_item"));
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(exportProgramCB), NULL);
    item = GTK_MENU_ITEM(gtk_builder_get_object(builder, "preferences_item"));
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(preferencesCB), NULL);
    item = GTK_MENU_ITEM(gtk_builder_get_object(builder, "quit_item"));
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(quitCB), NULL);
    item = GTK_MENU_ITEM(gtk_builder_get_object(builder, "copy_item"));
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(copyCB), NULL);
    item = GTK_MENU_ITEM(gtk_builder_get_object(builder, "paste_item"));
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pasteCB), NULL);
    item = GTK_MENU_ITEM(gtk_builder_get_object(builder, "copy_printout_as_text_item"));
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(copyPrintAsTextCB), NULL);
    item = GTK_MENU_ITEM(gtk_builder_get_object(builder, "copy_printout_as_image_item"));
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(copyPrintAsImageCB), NULL);
    item = GTK_MENU_ITEM(gtk_builder_get_object(builder, "clear_printout_item"));
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(clearPrintOutCB), NULL);
    item = GTK_MENU_ITEM(gtk_builder_get_object(builder, "documentation_item"));
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(documentationCB), NULL);
    item = GTK_MENU_ITEM(gtk_builder_get_object(builder, "website_item"));
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(websiteCB), NULL);
    item = GTK_MENU_ITEM(gtk_builder_get_object(builder, "other_website_item"));
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(otherWebsiteCB), NULL);
    keyboardShortcutsMenuItem = GTK_CHECK_MENU_ITEM(gtk_builder_get_object(builder, "keyboard_shortcuts_item"));
    g_signal_connect(G_OBJECT(keyboardShortcutsMenuItem), "activate", G_CALLBACK(keyboardShortcutsCB), NULL);
    item = GTK_MENU_ITEM(gtk_builder_get_object(builder, "edit_keymap_item"));
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(editKeymapCB), NULL);
    item = GTK_MENU_ITEM(gtk_builder_get_object(builder, "about_item"));
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(aboutCB), NULL);


    /****************************************/
    /* Drawing area for the calculator skin */
    /****************************************/

    GtkWidget *box = GTK_WIDGET(gtk_builder_get_object(builder, "box"));

    int win_width, win_height, flags;
    skin_load(&win_width, &win_height, &rows, &cols, &flags);
    skin_set_window_size(win_width, win_height);
    if (state.mainWindowWidth != 0)
        skin_set_window_size(state.mainWindowWidth, state.mainWindowHeight);
    disp_rows = rows;
    disp_cols = cols;
    GtkWidget *w = gtk_drawing_area_new();
    gtk_box_pack_start(GTK_BOX(box), w, TRUE, TRUE, 0);
    g_signal_connect(G_OBJECT(w), "draw", G_CALLBACK(draw_cb), NULL);
    gtk_widget_add_events(w, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
    g_signal_connect(G_OBJECT(w), "button-press-event", G_CALLBACK(button_cb), NULL);
    g_signal_connect(G_OBJECT(w), "button-release-event", G_CALLBACK(button_cb), NULL);
    gtk_widget_set_can_focus(w, TRUE);
    g_signal_connect(G_OBJECT(w), "key-press-event", G_CALLBACK(key_cb), NULL);
    g_signal_connect(G_OBJECT(w), "key-release-event", G_CALLBACK(key_cb), NULL);
    g_signal_connect(G_OBJECT(w), "size-allocate", G_CALLBACK(calc_resized), NULL);
    gtk_widget_set_size_request(w, 160, 160);
    calc_widget = w;

    /*
    GdkGeometry geom;
    geom.min_width = 160;
    geom.min_height = 160;
    gtk_window_set_geometry_hints(GTK_WINDOW(mainwindow), w, &geom, GdkWindowHints(GDK_HINT_MIN_SIZE));
    */


    /**************************************/
    /***** Build the print-out window *****/
    /**************************************/

    // In the Motif version, I create an XImage and read the bitmap data into
    // it; in the GTK version, that approach is not practical, since pixbuf
    // only comes in 24-bit and 32-bit flavors -- which would mean wasting
    // 25 megabytes for a 286x32768 pixbuf. So, instead, I use a 1 bpp buffer,
    // and simply create pixbufs on the fly whenever I have to repaint.
    print_bitmap = (unsigned char *) malloc(PRINT_SIZE);
    print_text = (unsigned char *) malloc(PRINT_TEXT_SIZE);
    // TODO - handle memory allocation failure

    FILE *printfile = fopen(printfilename, "r");
    if (printfile != NULL) {
        int n = fread(&printout_bottom, 1, sizeof(int), printfile);
        if (n == sizeof(int)) {
            if (printout_bottom > PRINT_LINES) {
                int excess = (printout_bottom - PRINT_LINES) * PRINT_BYTESPERLINE;
                fseek(printfile, excess, SEEK_CUR);
                printout_bottom = PRINT_LINES;
            }
            int bytes = printout_bottom * PRINT_BYTESPERLINE;
            n = fread(print_bitmap, 1, bytes, printfile);
            if (n == bytes) {
                n = fread(&print_text_bottom, 1, sizeof(int), printfile);
                int n2 = fread(&print_text_pixel_height, 1, sizeof(int), printfile);
                if (n == sizeof(int) && n2 == sizeof(int)) {
                    n = fread(print_text, 1, print_text_bottom, printfile);
                    if (n != print_text_bottom) {
                        print_text_bottom = 0;
                        print_text_pixel_height = 0;
                    }
                } else {
                    print_text_bottom = 0;
                    print_text_pixel_height = 0;
                }
            } else {
                printout_bottom = 0;
                print_text_bottom = 0;
                print_text_pixel_height = 0;
            }
        } else {
            printout_bottom = 0;
            print_text_bottom = 0;
            print_text_pixel_height = 0;
        }
        fclose(printfile);
    } else {
        printout_bottom = 0;
        print_text_bottom = 0;
        print_text_pixel_height = 0;
    }
    printout_top = 0;
    print_text_top = 0;

    printwindow = gtk_application_window_new(GTK_APPLICATION(app));
    gtk_window_set_icon(GTK_WINDOW(printwindow), icon_128);
    gtk_window_set_title(GTK_WINDOW(printwindow), "Plus42 Print-Out");
    gtk_window_set_role(GTK_WINDOW(printwindow), "Plus42 Print-Out");
    g_signal_connect(G_OBJECT(printwindow), "delete_event",
                     G_CALLBACK(delete_print_cb), NULL);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
    gtk_container_add(GTK_CONTAINER(printwindow), scroll);
    GtkWidget *view = gtk_viewport_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scroll), view);
    print_widget = gtk_drawing_area_new();
    gtk_widget_set_size_request(print_widget, 358, printout_bottom);
    gtk_container_add(GTK_CONTAINER(view), print_widget);
    print_adj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scroll));
    g_signal_connect(G_OBJECT(print_widget), "draw", G_CALLBACK(print_draw_cb), NULL);
    gtk_widget_set_can_focus(print_widget, TRUE);
    g_signal_connect(G_OBJECT(print_widget), "key-press-event", G_CALLBACK(print_key_cb), NULL);

    gtk_widget_show(print_widget);
    gtk_widget_show(view);
    gtk_widget_show(scroll);

    GdkGeometry geom;
    geom.min_width = 358;
    geom.max_width = 358;
    geom.min_height = 18;
    geom.max_height = 32767;
    geom.width_inc = 1;
    geom.height_inc = 18;
    gtk_window_set_geometry_hints(GTK_WINDOW(printwindow), NULL, &geom, GdkWindowHints(GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE | GDK_HINT_RESIZE_INC));

    if (state.printWindowKnown)
        gtk_window_move(GTK_WINDOW(printwindow), state.printWindowX,
                                                 state.printWindowY);
    gint width, height;
    gtk_window_get_size(GTK_WINDOW(printwindow), &width, &height);
    gtk_window_resize(GTK_WINDOW(printwindow), width,
            state.printWindowKnown ? state.printWindowHeight : 600);

    gtk_widget_realize(printwindow);
    gtk_widget_realize(print_widget);
    scroll_printout_to_bottom();


    /*************************************************/
    /***** Show main window & start the emulator *****/
    /*************************************************/

    if (state.printWindowKnown && state.printWindowMapped)
        gtk_widget_show(printwindow);
    gtk_widget_show_all(mainwindow);
    gtk_widget_show(mainwindow);

    core_repaint_display(disp_rows, disp_cols, flags);
    if (core_powercycle())
        enable_reminder();

    /* Check if /proc/apm exists and is readable, and if so,
     * start the battery checker "thread" that keeps the battery
     * annunciator on the calculator display up to date.
     */
    FILE *apm = fopen("/proc/apm", "r");
    if (apm != NULL) {
        fclose(apm);
        shell_low_battery();
        g_timeout_add(60000, battery_checker, NULL);
    } else {
        /* Check if /sys/class/power_supply exists */
        DIR *d = opendir("/sys/class/power_supply");
        if (d != NULL) {
            closedir(d);
            shell_low_battery();
            g_timeout_add(60000, battery_checker, NULL);
        }
    }

    if (pipe(pype) != 0)
        fprintf(stderr, "Could not create pipe for signal handler; not catching signals.\n");
    else {
        GIOChannel *channel = g_io_channel_unix_new(pype[0]);
        GError *err = NULL;
        g_io_channel_set_encoding(channel, NULL, &err);
        g_io_channel_set_flags(channel,
            (GIOFlags) (g_io_channel_get_flags(channel) | G_IO_FLAG_NONBLOCK), &err);
        g_io_add_watch(channel, G_IO_IN, gt_signal_handler, NULL);

        struct sigaction act;
        act.sa_handler = int_term_handler;
        sigemptyset(&act.sa_mask);
        sigaddset(&act.sa_mask, SIGINT);
        sigaddset(&act.sa_mask, SIGTERM);
        act.sa_flags = 0;
        sigaction(SIGINT, &act, NULL);
        sigaction(SIGTERM, &act, NULL);
    }
}

static void menubar_resized(GtkWidget *w, GtkAllocation *allocation, gpointer data) {
    if (menu_bar_height == allocation->height)
        return;
    menu_bar_height = allocation->height;
    int win_width, win_height;
    skin_get_window_size(&win_width, &win_height);
    int mh = menu_bar_height >= 2 ? menu_bar_height : 0;
    gtk_window_resize(GTK_WINDOW(mainwindow), win_width, win_height + mh);
}

static gboolean resizer(gpointer cd) {
    int win_width, win_height;
    skin_get_window_size(&win_width, &win_height);
    int mh = menu_bar_height >= 2 ? menu_bar_height : 0;
    gtk_window_resize(GTK_WINDOW(mainwindow), win_width, win_height + mh);
    resize_timeout_id = 0;
    return FALSE;
}

static void calc_resized(GtkWidget *w, GtkAllocation *allocation, gpointer data) {
    int skin_width, skin_height;
    skin_get_size(&skin_width, &skin_height);
    int win_width = allocation->width;
    int win_height = allocation->height;
    double hScale = ((double) win_width) / skin_width;
    double vScale = ((double) win_height) / skin_height;
    if (hScale > vScale)
        win_width = (int) (skin_width * vScale + 0.5);
    else if (vScale > hScale)
        win_height = (int) (skin_height * hScale + 0.5);
    skin_set_window_size(win_width, win_height);
    if (resize_timeout_id != 0)
        g_source_remove(resize_timeout_id);
    resize_timeout_id = g_timeout_add(250, resizer, NULL);
}

keymap_entry *parse_keymap_entry(char *line, int lineno) {
    char *p;
    static keymap_entry entry;

    p =  strchr(line, '#');
    if (p != NULL)
        *p = 0;
    p = strchr(line, '\n');
    if (p != NULL)
        *p = 0;
    p = strchr(line, '\r');
    if (p != NULL)
        *p = 0;

    p = strchr(line, ':');
    if (p != NULL) {
        char *val = p + 1;
        char *tok;
        bool ctrl = false;
        bool alt = false;
        bool shift = false;
        bool cshift = false;
        guint keyval = GDK_KEY_VoidSymbol;
        bool done = false;
        unsigned char macrobuf[KEYMAP_MAX_MACRO_LENGTH + 1];
        int macrolen = 0;

        /* Parse keysym */
        *p = 0;
        tok = strtok(line, " \t");
        while (tok != NULL) {
            if (done) {
                fprintf(stderr, "Keymap, line %d: Excess tokens in key spec.\n", lineno);
                return NULL;
            }
            if (strcasecmp(tok, "ctrl") == 0)
                ctrl = true;
            else if (strcasecmp(tok, "alt") == 0)
                alt = true;
            else if (strcasecmp(tok, "shift") == 0)
                shift = true;
            else if (strcasecmp(tok, "cshift") == 0)
                cshift = true;
            else {
                keyval = gdk_keyval_from_name(tok);
                if (keyval == GDK_KEY_VoidSymbol) {
                    fprintf(stderr, "Keymap, line %d: Unrecognized KeyName.\n", lineno);
                    return NULL;
                }
                done = true;
            }
            tok = strtok(NULL, " \t");
        }
        if (!done) {
            fprintf(stderr, "Keymap, line %d: Unrecognized KeyName.\n", lineno);
            return NULL;
        }

        /* Parse macro */
        tok = strtok(val, " \t");
        while (tok != NULL) {
            char *endptr;
            long k = strtol(tok, &endptr, 10);
            if (*endptr != 0 || k < 1 || k > 255) {
                fprintf(stderr, "Keymap, line %d: Bad value (%s) in macro.\n", lineno, tok);
                return NULL;
            } else if (macrolen == KEYMAP_MAX_MACRO_LENGTH) {
                fprintf(stderr, "Keymap, line %d: Macro too long (max=%d).\n", lineno, KEYMAP_MAX_MACRO_LENGTH);
                return NULL;
            } else
                macrobuf[macrolen++] = k;
            tok = strtok(NULL, " \t");
        }
        macrobuf[macrolen] = 0;

        entry.ctrl = ctrl;
        entry.alt = alt;
        entry.shift = shift;
        entry.cshift = cshift;
        entry.keyval = keyval;
        strcpy((char *) entry.macro, (const char *) macrobuf);
        return &entry;
    } else
        return NULL;
}

static void read_key_map(const char *keymapfilename) {
    FILE *keymapfile = fopen(keymapfilename, "r");
    int kmcap = 0;
    char line[1024];
    int lineno = 0;

    if (keymapfile == NULL) {
        /* Try to create default keymap file */
        extern const long keymap_filesize;
        extern const char keymap_filedata[];
        long n;

        keymapfile = fopen(keymapfilename, "wb");
        if (keymapfile == NULL)
            return;
        n = fwrite(keymap_filedata, 1, keymap_filesize, keymapfile);
        if (n != keymap_filesize) {
            int err = errno;
            fprintf(stderr, "Error writing \"%s\": %s (%d)\n",
                            keymapfilename, strerror(err), err);
        }
        fclose(keymapfile);

        keymapfile = fopen(keymapfilename, "r");
        if (keymapfile == NULL)
            return;
    }

    while (fgets(line, 1024, keymapfile) != NULL) {
        keymap_entry *entry = parse_keymap_entry(line, ++lineno);
        if (entry == NULL)
            continue;
        /* Create new keymap entry */
        if (keymap_length == kmcap) {
            kmcap += 50;
            keymap = (keymap_entry *) realloc(keymap, kmcap * sizeof(keymap_entry));
            // TODO - handle memory allocation failure
        }
        memcpy(keymap + (keymap_length++), entry, sizeof(keymap_entry));
    }

    fclose(keymapfile);
}

static void init_shell_state(int4 version) {
    switch (version) {
        case -1:
            state.extras = 0;
            /* fall through */
        case 0:
            state.printerToTxtFile = 0;
            state.printerToGifFile = 0;
            state.printerTxtFileName[0] = 0;
            state.printerGifFileName[0] = 0;
            state.printerGifMaxLength = 256;
            /* fall through */
        case 1:
            state.mainWindowKnown = 0;
            state.printWindowKnown = 0;
            /* fall through */
        case 2:
            state.skinName[0] = 0;
            /* fall through */
        case 3:
            state.singleInstance = 1;
            /* fall through */
        case 4:
            strcpy(state.coreName, "Untitled");
            /* fall through */
        case 5:
            core_settings.matrix_singularmatrix = false;
            core_settings.matrix_outofrange = false;
            core_settings.auto_repeat = true;
            /* fall through */
        case 6:
            state.old_repaint = true;
            /* fall through */
        case 7:
            /* fall through */
        case 8:
            core_settings.localized_copy_paste = true;
            /* fall through */
        case 9:
            state.mainWindowWidth = 0;
            state.mainWindowHeight = 0;
            /* fall through */
        case 10:
            /* current version (SHELL_VERSION = 10),
             * so nothing to do here since everything
             * was initialized from the state file.
             */
            ;
    }
}

static int read_shell_state() {
    int4 magic;
    int4 state_size;
    int4 state_version;

    if (fread(&magic, 1, sizeof(int4), statefile) != sizeof(int4))
        return 0;
    if (magic != PLUS42_MAGIC)
        return 0;

    int4 dummy;
    if (fread(&dummy, 1, sizeof(int4), statefile) != sizeof(int4))
        return 0;

    if (fread(&state_size, 1, sizeof(int4), statefile) != sizeof(int4))
        return 0;
    if (fread(&state_version, 1, sizeof(int4), statefile) != sizeof(int4))
        return 0;
    if (state_version < 0 || state_version > SHELL_VERSION)
        /* Unknown shell state version */
        return 0;
    if (fread(&state, 1, state_size, statefile) != (size_t) state_size)
        return 0;
    if (state_version >= 6) {
        core_settings.matrix_singularmatrix = state.matrix_singularmatrix;
        core_settings.matrix_outofrange = state.matrix_outofrange;
        core_settings.auto_repeat = state.auto_repeat;
    }
    if (state_version >= 9)
        core_settings.localized_copy_paste = state.localized_copy_paste;

    init_shell_state(state_version);
    return 1;
}

static int write_shell_state() {
    int4 magic = PLUS42_MAGIC;
    int4 version = 27;
    int4 state_size = sizeof(state_type);
    int4 state_version = SHELL_VERSION;

    if (fwrite(&magic, 1, sizeof(int4), statefile) != sizeof(int4))
        return 0;
    if (fwrite(&version, 1, sizeof(int4), statefile) != sizeof(int4))
        return 0;
    if (fwrite(&state_size, 1, sizeof(int4), statefile) != sizeof(int4))
        return 0;
    if (fwrite(&state_version, 1, sizeof(int4), statefile) != sizeof(int4))
        return 0;
    state.matrix_singularmatrix = core_settings.matrix_singularmatrix;
    state.matrix_outofrange = core_settings.matrix_outofrange;
    state.auto_repeat = core_settings.auto_repeat;
    state.localized_copy_paste = core_settings.localized_copy_paste;
    if (fwrite(&state, 1, sizeof(state_type), statefile) != sizeof(int4))
        return 0;

    return 1;
}

static void int_term_handler(int sig) {
    write(pype[1], "1\n", 2);
}

static gboolean gt_signal_handler(GIOChannel *source, GIOCondition condition,
                                                            gpointer data) {
    quit();
    return TRUE;
}

static void quit() {
    FILE *printfile;
    int n, length;

    printfile = fopen(printfilename, "w");
    if (printfile != NULL) {
        // Write bitmap
        length = printout_bottom - printout_top;
        if (length < 0)
            length += PRINT_LINES;
        n = fwrite(&length, 1, sizeof(int), printfile);
        if (n != sizeof(int))
            goto failed;
        if (printout_bottom >= printout_top) {
            n = fwrite(print_bitmap + PRINT_BYTESPERLINE * printout_top,
                       1, PRINT_BYTESPERLINE * length, printfile);
            if (n != PRINT_BYTESPERLINE * length)
                goto failed;
        } else {
            n = fwrite(print_bitmap + PRINT_BYTESPERLINE * printout_top,
                       1, PRINT_SIZE - PRINT_BYTESPERLINE * printout_top,
                       printfile);
            if (n != PRINT_SIZE - PRINT_BYTESPERLINE * printout_top)
                goto failed;
            n = fwrite(print_bitmap, 1,
                       PRINT_BYTESPERLINE * printout_bottom, printfile);
            if (n != PRINT_BYTESPERLINE * printout_bottom)
                goto failed;
        }
        // Write text
        length = print_text_bottom - print_text_top;
        if (length < 0)
            length += PRINT_TEXT_SIZE;
        n = fwrite(&length, 1, sizeof(int), printfile);
        if (n != sizeof(int))
            goto failed;
        n = fwrite(&print_text_pixel_height, 1, sizeof(int), printfile);
        if (n != sizeof(int))
            goto failed;
        if (print_text_bottom >= print_text_top) {
            n = fwrite(print_text + print_text_top, 1, length, printfile);
            if (n != length)
                goto failed;
        } else {
            n = fwrite(print_text + print_text_top, 1, PRINT_TEXT_SIZE - print_text_top, printfile);
            if (n != PRINT_TEXT_SIZE - print_text_top)
                goto failed;
            n = fwrite(print_text, 1, print_text_bottom, printfile);
            if (n != print_text_bottom)
                goto failed;
        }

        fclose(printfile);
        goto done;

        failed:
        fclose(printfile);
        remove(printfilename);

        done:
        ;
    }

    if (print_txt != NULL)
        fclose(print_txt);

    if (print_gif != NULL) {
        shell_finish_gif(gif_seeker, gif_writer);
        fclose(print_gif);
    }

    gint x, y;
    gtk_window_get_position(GTK_WINDOW(mainwindow), &x, &y);
    state.mainWindowKnown = 1;
    state.mainWindowX = x;
    state.mainWindowY = y;
    int w, h;
    skin_get_window_size(&w, &h);
    state.mainWindowWidth = w;
    state.mainWindowHeight = h;

    if (state.printWindowMapped) {
        gtk_window_get_position(GTK_WINDOW(printwindow), &x, &y);
        state.printWindowX = x;
        state.printWindowY = y;
    }
    if (state.printWindowKnown) {
        gtk_window_get_size(GTK_WINDOW(printwindow), &x, &y);
        state.printWindowHeight = y;
    }

    statefile = fopen(statefilename, "w");
    if (statefile != NULL) {
        write_shell_state();
        fclose(statefile);
    }
    char corefilename[FILENAMELEN];
    snprintf(corefilename, FILENAMELEN, "%s/%s.p42", free42dirname, state.coreName);
    core_save_state(corefilename);
    core_cleanup();

    shell_spool_exit();

    exit(0);
}

static char *strclone(const char *s) {
    char *s2 = (char *) malloc(strlen(s) + 1);
    if (s2 != NULL)
        strcpy(s2, s);
    return s2;
}

static bool file_exists(const char *name) {
    struct stat st;
    return stat(name, &st) == 0;
}

static void show_message(const char *title, const char *message, GtkWidget *parent) {
    GtkWidget *msg = gtk_message_dialog_new(GTK_WINDOW(parent),
                                            GTK_DIALOG_MODAL,
                                            GTK_MESSAGE_ERROR,
                                            GTK_BUTTONS_OK,
                                            "%s",
                                            message);
    gtk_window_set_title(GTK_WINDOW(msg), title);
    gtk_window_set_role(GTK_WINDOW(msg), "Plus42 Dialog");
    gtk_dialog_run(GTK_DIALOG(msg));
    gtk_widget_destroy(msg);
}

static void no_mwm_resize_helper(GtkWidget *w, gpointer cd) {
    GdkWindow *win = gtk_widget_get_window(w);
    gdk_window_set_decorations(win, GdkWMDecoration(GDK_DECOR_ALL
                                    | GDK_DECOR_RESIZEH | GDK_DECOR_MAXIMIZE));
    gdk_window_set_functions(win, GdkWMFunction(GDK_FUNC_ALL
                                    | GDK_FUNC_RESIZE | GDK_FUNC_MAXIMIZE));
}

static void no_mwm_resize_borders(GtkWidget *window) {
    // gtk_window_set_resizable(w, FALSE) only sets the WM size hints, so that
    // the minimum and maximum sizes coincide with the actual size. While this
    // certainly has the desired effect of making the window non-resizable, it
    // does not deal with the way mwm visually distinguishes resizable and
    // non-resizable windows; the result is that, when running under mwm, you
    // have a window with resize borders and a maximize button, none of which
    // actually let you resize the window.
    // So, we use an additional GDK call to set the appropriate mwm properties.
    g_signal_connect(G_OBJECT(window), "realize",
                     G_CALLBACK(no_mwm_resize_helper), NULL);
}

static void no_mwm_zoom_helper(GtkWidget *w, gpointer cd) {
    GdkWindow *win = gtk_widget_get_window(w);
    gdk_window_set_decorations(win, GdkWMDecoration(GDK_DECOR_ALL
                                    | GDK_DECOR_MAXIMIZE));
    gdk_window_set_functions(win, GdkWMFunction(GDK_FUNC_ALL
                                    | GDK_FUNC_MAXIMIZE));
}

static void no_mwm_zoom_box(GtkWidget *window) {
    g_signal_connect(G_OBJECT(window), "realize",
                     G_CALLBACK(no_mwm_zoom_helper), NULL);
}

static void scroll_printout_to_bottom() {
    gdouble upper = gtk_adjustment_get_upper(print_adj);
    gdouble page_size = gtk_adjustment_get_page_size(print_adj);
    gtk_adjustment_set_value(print_adj, upper - page_size);
}

static void quitCB() {
    quit();
}

////////////////////////////////////
///// States stuff starts here /////
////////////////////////////////////

static int currentStateIndex;
static int selectedStateIndex;
static char **state_names;
static GtkWidget *statesMenuItems[6];

static void states_changed_cb(GtkWidget *w, gpointer p) {
    selectedStateIndex = -1;
    GList *rows = gtk_tree_selection_get_selected_rows(GTK_TREE_SELECTION(w), NULL);
    if (rows != NULL) {
        GtkTreePath *path = (GtkTreePath *) rows->data;
        sscanf(gtk_tree_path_to_string(path), "%d", &selectedStateIndex);
    }
    g_list_free(rows);
    GtkWidget *btn = (GtkWidget *) p;
    if (selectedStateIndex == -1) {
        gtk_widget_set_sensitive(btn, false);
        gtk_button_set_label(GTK_BUTTON(btn), "Switch To");
        gtk_widget_set_sensitive(statesMenuItems[1], false);
        gtk_widget_set_sensitive(statesMenuItems[2], false);
        gtk_widget_set_sensitive(statesMenuItems[3], false);
        gtk_widget_set_sensitive(statesMenuItems[5], false);
    } else if (selectedStateIndex == currentStateIndex) {
        gtk_widget_set_sensitive(btn, true);
        gtk_button_set_label(GTK_BUTTON(btn), "Revert");
        gtk_widget_set_sensitive(statesMenuItems[1], true);
        gtk_widget_set_sensitive(statesMenuItems[2], true);
        gtk_widget_set_sensitive(statesMenuItems[3], false);
        gtk_widget_set_sensitive(statesMenuItems[5], true);
    } else {
        gtk_widget_set_sensitive(btn, true);
        gtk_button_set_label(GTK_BUTTON(btn), "Switch To");
        gtk_widget_set_sensitive(statesMenuItems[1], true);
        gtk_widget_set_sensitive(statesMenuItems[2], true);
        gtk_widget_set_sensitive(statesMenuItems[3], true);
        gtk_widget_set_sensitive(statesMenuItems[5], true);
    }
}

static GtkWidget *dlg;

static char *get_state_name(const char *prompt, const char *initialName) {
    static GtkWidget *state_name_dialog = NULL;
    static GtkWidget *promptLabel;
    static GtkWidget *name;

    if (state_name_dialog == NULL) {
        state_name_dialog = gtk_dialog_new_with_buttons(
                            "State Name",
                            GTK_WINDOW(dlg),
                            GTK_DIALOG_MODAL,
                            "_OK", GTK_RESPONSE_ACCEPT,
                            "_Cancel", GTK_RESPONSE_CANCEL,
                            NULL);
        gtk_window_set_resizable(GTK_WINDOW(state_name_dialog), FALSE);
        no_mwm_resize_borders(state_name_dialog);
        GtkWidget *container = gtk_bin_get_child(GTK_BIN(state_name_dialog));
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_container_add(GTK_CONTAINER(container), box);

        promptLabel = gtk_label_new("Enter state name:");
        gtk_box_pack_start(GTK_BOX(box), promptLabel, FALSE, FALSE, 10);

        name = gtk_entry_new();
        gtk_box_pack_start(GTK_BOX(box), name, FALSE, FALSE, 10);

        gtk_widget_show_all(state_name_dialog);
    }

    gtk_window_set_role(GTK_WINDOW(state_name_dialog), "Plus42 Dialog");
    gtk_label_set_text(GTK_LABEL(promptLabel), prompt);
    gtk_entry_set_text(GTK_ENTRY(name), initialName);
    gtk_editable_select_region(GTK_EDITABLE(name), 0, -1);

    char *result = NULL;
    while (true) {
        gtk_window_set_focus(GTK_WINDOW(state_name_dialog), name);
        gtk_widget_grab_focus(GTK_WIDGET(name));
        int response = gtk_dialog_run(GTK_DIALOG(state_name_dialog));
        if (response == GTK_RESPONSE_ACCEPT) {
            const char *tmp = gtk_entry_get_text(GTK_ENTRY(name));
            if (strchr(tmp, '/') != NULL) {
                show_message("Message", "That name is not valid.", state_name_dialog);
                continue;
            }
            char path[FILENAMELEN];
            snprintf(path, FILENAMELEN, "%s/%s.p42", free42dirname, tmp);
            if (file_exists(path)) {
                show_message("Message", "That name is already in use.", state_name_dialog);
                continue;
            }
            result = (char *) malloc(strlen(tmp) + 1);
            strcpy(result, tmp);
        }
        break;
    }

    gtk_widget_hide(state_name_dialog);
    return result;
}

static bool switchTo(const char *selectedStateName) {
    char path[FILENAMELEN];
    if (strcmp(selectedStateName, state.coreName) == 0) {
        GtkWidget *msg = gtk_message_dialog_new(GTK_WINDOW(dlg),
                                                GTK_DIALOG_MODAL,
                                                GTK_MESSAGE_QUESTION,
                                                GTK_BUTTONS_YES_NO,
                                                "Are you sure you want to revert the state \"%s\" to the last version saved?",
                                                selectedStateName);
        gtk_window_set_title(GTK_WINDOW(msg), "Revert State?");
        gtk_window_set_role(GTK_WINDOW(msg), "Plus42 Dialog");
        bool cancelled = gtk_dialog_run(GTK_DIALOG(msg)) != GTK_RESPONSE_YES;
        gtk_widget_destroy(msg);
        if (cancelled)
            return false;
    } else {
        snprintf(path, FILENAMELEN, "%s/%s.p42", free42dirname, state.coreName);
        core_save_state(path);
    }
    core_cleanup();
    strncpy(state.coreName, selectedStateName, FILENAMELEN);
    state.coreName[FILENAMELEN - 1] = 0;
    snprintf(path, FILENAMELEN, "%s/%s.p42", free42dirname, state.coreName);
    int rows = disp_rows;
    int cols = disp_cols;
    core_init(&rows, &cols, 1, path);
    if (rows != disp_rows || cols != disp_cols)
        update_skin(rows, cols);
    else
        core_repaint_display(rows, cols, -1);
    if (core_powercycle())
        enable_reminder();
    return true;
}

static void states_menu_new() {
    char *name = get_state_name("New state name:", "");
    if (name == NULL)
        return;
    char path[FILENAMELEN];
    snprintf(path, FILENAMELEN, "%s/%s.p42", free42dirname, name);
    FILE *f = fopen(path, "w");
    fprintf(f, PLUS42_MAGIC_STR);
    fclose(f);
    free(name);
    gtk_dialog_response(GTK_DIALOG(dlg), 4);
}

static bool copy_state(const char *orig_name, const char *copy_name) {
    FILE *fin = fopen(orig_name, "r");
    FILE *fout = fopen(copy_name, "w");
    if (fin != NULL && fout != NULL) {
        char buf[1024];
        int n;
        while ((n = fread(buf, 1, 1024, fin)) > 0)
            fwrite(buf, 1, n, fout);
        if (ferror(fin) || ferror(fout))
            goto duplication_failed;
        fclose(fin);
        fclose(fout);
        return true;
    } else {
        duplication_failed:
        if (fin != NULL)
            fclose(fin);
        if (fout != NULL)
            fclose(fout);
        remove(copy_name);
        return false;
    }
}

static void states_menu_duplicate() {
    if (selectedStateIndex == -1)
        return;
    char copyName[FILENAMELEN];
    strcpy(copyName, state_names[selectedStateIndex]);
    int n = 0;

    // We're naming duplicates by appending " copy" or " copy NNN" to the name
    // of the original, but if the name of the original already ends with " copy"
    // or " copy NNN", it seems more elegant to continue the sequence rather than
    // add another " copy" suffix.
    int len = strlen(copyName);
    if (len > 5 && strcmp(copyName + len - 5, " copy") == 0) {
        copyName[len - 5] = 0;
        n = 1;
    } else if (len > 7) {
        int pos = len - 7;
        int m = 0;
        int p = 1;
        while (pos > 0) {
            char c = copyName[pos + 6];
            if (c < '0' || c > '9')
                goto not_a_copy;
            m += p * (c - '0');
            p *= 10;
            if (strncmp(copyName + pos, " copy ", 6) == 0) {
                n = m;
                copyName[pos] = 0;
                break;
            } else
                pos--;
        }
        not_a_copy:;
    }

    char finalName[FILENAMELEN];
    int flen;
    while (true) {
        n++;
        if (n == 1)
            flen = snprintf(finalName, FILENAMELEN, "%s/%s copy.p42", free42dirname, copyName);
        else
            flen = snprintf(finalName, FILENAMELEN, "%s/%s copy %d.p42", free42dirname, copyName, n);
        if (flen >= FILENAMELEN) {
            show_message("Message", "The name of that state is too long to copy.", dlg);
            return;
        }
        if (!file_exists(finalName))
            // File does not exist; that means we have a usable name
            break;
    }

    // Once we get here, finalName contains a valid name for creating the duplicate.
    // What we do next depends on whether the selected state is the currently active
    // one. If it is, we'll call core_save_state(), to make sure the duplicate
    // actually matches the most up-to-date state; otherwise, we can simply copy
    // the existing state file.
    if (strcmp(state_names[selectedStateIndex], state.coreName) == 0)
        core_save_state(finalName);
    else {
        char origName[FILENAMELEN];
        snprintf(origName, FILENAMELEN, "%s/%s.p42", free42dirname, state_names[selectedStateIndex]);
        if (!copy_state(origName, finalName)) {
            show_message("Message", "State duplication failed.", dlg);
            return;
        }
    }
    gtk_dialog_response(GTK_DIALOG(dlg), 4);
}

static void states_menu_rename() {
    if (selectedStateIndex == -1)
        return;
    char prompt[FILENAMELEN];
    snprintf(prompt, FILENAMELEN, "Rename \"%s\" to:", state_names[selectedStateIndex]);
    char *newname = get_state_name(prompt, state_names[selectedStateIndex]);
    if (newname == NULL)
        return;
    char oldpath[FILENAMELEN];
    snprintf(oldpath, FILENAMELEN, "%s/%s.p42", free42dirname, state_names[selectedStateIndex]);
    char newpath[FILENAMELEN];
    snprintf(newpath, FILENAMELEN, "%s/%s.p42", free42dirname, newname);
    rename(oldpath, newpath);
    if (strcmp(state_names[selectedStateIndex], state.coreName) == 0)
        strncpy(state.coreName, newname, FILENAMELEN);
    gtk_dialog_response(GTK_DIALOG(dlg), 4);
}

static void states_menu_delete() {
    if (selectedStateIndex == -1)
        return;
    const char *stateName = state_names[selectedStateIndex];
    if (strcmp(stateName, state.coreName) == 0)
        return;
    GtkWidget *msg = gtk_message_dialog_new(GTK_WINDOW(dlg),
                                            GTK_DIALOG_MODAL,
                                            GTK_MESSAGE_QUESTION,
                                            GTK_BUTTONS_YES_NO,
                                            "Are you sure you want to delete the state \"%s\"?",
                                            stateName);
    gtk_window_set_title(GTK_WINDOW(msg), "Delete State?");
    gtk_window_set_role(GTK_WINDOW(msg), "Plus42 Dialog");
    bool cancelled = gtk_dialog_run(GTK_DIALOG(msg)) != GTK_RESPONSE_YES;
    gtk_widget_destroy(msg);
    if (cancelled)
        return;
    char statePath[FILENAMELEN];
    snprintf(statePath, FILENAMELEN, "%s/%s.p42", free42dirname, stateName);
    remove(statePath);
    gtk_dialog_response(GTK_DIALOG(dlg), 4);
}

static void states_menu_import() {
    static GtkWidget *dialog = NULL;

    if (dialog == NULL)
        dialog = make_file_select_dialog("Import State",
                "Plus42 State (*.p42)\0*.[Pp]42\0All Files (*.*)\0*\0",
                false, dlg);

    gtk_window_set_role(GTK_WINDOW(dialog), "Plus42 Dialog");
    bool cancelled = gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_ACCEPT;
    gtk_widget_hide(dialog);
    if (cancelled)
        return;

    char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    if (filename == NULL)
        return;

    char name[FILENAMELEN];
    char *p = strrchr(filename, '/');
    if (p == NULL)
        strncpy(name, filename, FILENAMELEN);
    else
        strncpy(name, p + 1, FILENAMELEN);
    name[FILENAMELEN - 1] = 0;
    int len = strlen(name);
    if (len > 4 && strcmp(name + len - 4, ".p42") == 0)
        name[len - 4] = 0;
    char destPath[FILENAMELEN];
    snprintf(destPath, FILENAMELEN, "%s/%s.p42", free42dirname, name);
    bool success = false;
    if (file_exists(destPath)) {
        char msg[FILENAMELEN];
        snprintf(msg, FILENAMELEN, "A state named \"%s\" already exists.", name);
        show_message("Message", msg, dlg);
    } else if (!copy_state(filename, destPath)) {
        show_message("Message", "State import failed.", dlg);
    } else {
        success = true;
    }
    g_free(filename);
    if (success)
        gtk_dialog_response(GTK_DIALOG(dlg), 4);
}

static void states_menu_export() {
    if (selectedStateIndex == -1)
        return;

    static GtkWidget *save_dialog = NULL;
    if (save_dialog == NULL)
        save_dialog = make_file_select_dialog("Export State",
                "Plus42 State (*.p42)\0*.[Ff]42\0All Files (*.*)\0*\0",
                true, dlg);

    char *filename = NULL;
    gtk_window_set_role(GTK_WINDOW(save_dialog), "Plus42 Dialog");
    char export_file_name[FILENAMELEN];
    strcpy(export_file_name, state_names[selectedStateIndex]);
    export_file_name[FILENAMELEN - 5] = 0;
    strcat(export_file_name, ".p42");
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(save_dialog), export_file_name);
    if (gtk_dialog_run(GTK_DIALOG(save_dialog)) == GTK_RESPONSE_ACCEPT)
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(save_dialog));
    gtk_widget_hide(GTK_WIDGET(save_dialog));
    if (filename == NULL)
        return;

    strcpy(export_file_name, filename);
    g_free(filename);
    if (strncmp(gtk_file_filter_get_name(
                    gtk_file_chooser_get_filter(
                        GTK_FILE_CHOOSER(save_dialog))), "All", 3) != 0)
        appendSuffix(export_file_name, ".p42");

    if (file_exists(export_file_name)) {
        GtkWidget *msg = gtk_message_dialog_new(GTK_WINDOW(mainwindow),
                                                GTK_DIALOG_MODAL,
                                                GTK_MESSAGE_QUESTION,
                                                GTK_BUTTONS_YES_NO,
                                                "Replace existing \"%s\"?",
                                                export_file_name);
        gtk_window_set_title(GTK_WINDOW(msg), "Replace?");
        gtk_window_set_role(GTK_WINDOW(msg), "Plus42 Dialog");
        bool cancelled = gtk_dialog_run(GTK_DIALOG(msg)) != GTK_RESPONSE_YES;
        gtk_widget_destroy(msg);
        if (cancelled)
            return;
    }

    if (selectedStateIndex == currentStateIndex)
        core_save_state(export_file_name);
    else {
        char orig_path[FILENAMELEN];
        snprintf(orig_path, FILENAMELEN, "%s/%s.p42", free42dirname, state_names[selectedStateIndex]);
        if (!copy_state(orig_path, export_file_name))
            show_message("Message", "State export failed.", dlg);
    }
}

static void states_menu_cb(GtkWidget *w, gpointer p) {
    switch ((size_t) p) {
        case 0:
            states_menu_new();
            break;
        case 1:
            states_menu_duplicate();
            break;
        case 2:
            states_menu_rename();
            break;
        case 3:
            states_menu_delete();
            break;
        case 4:
            states_menu_import();
            break;
        case 5:
            states_menu_export();
            break;
    }
}

static int case_insens_comparator(const void *a, const void *b) {
    return strcasecmp(*(const char **) a, *(const char **) b);
}

static void row_activated_cb(GtkWidget *w, gpointer p) {
    gtk_dialog_response(GTK_DIALOG(dlg), 1);
}

static void states_menu_pos_func(GtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer data) {
    GtkWidget *btn = GTK_WIDGET(data);
    gdk_window_get_origin(gtk_widget_get_window(btn), x, y);
    GtkAllocation allocation;
    gtk_widget_get_allocation(btn, &allocation);
    *x += allocation.x;
    *y += allocation.y + allocation.height;
    *push_in = true;
}

static void statesCB() {
    static GtkWidget *states_dialog = NULL;
    static GtkTreeView *tree;
    static GtkTreeSelection *select;
    static GtkWidget *currentLabel;
    static GtkWidget *menu;
    char buf[FILENAMELEN];

    if (states_dialog == NULL) {
        // Pop-up menu for "More" button
        menu = gtk_menu_new();
        statesMenuItems[0] = gtk_menu_item_new_with_label("New");
        statesMenuItems[1] = gtk_menu_item_new_with_label("Duplicate");
        statesMenuItems[2] = gtk_menu_item_new_with_label("Rename");
        statesMenuItems[3] = gtk_menu_item_new_with_label("Delete");
        statesMenuItems[4] = gtk_menu_item_new_with_label("Import");
        statesMenuItems[5] = gtk_menu_item_new_with_label("Export");
        for (int i = 0; i < 6; i++) {
            g_signal_connect(G_OBJECT(statesMenuItems[i]), "activate", G_CALLBACK(states_menu_cb), (gpointer) (size_t) i);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), statesMenuItems[i]);
            gtk_widget_show(statesMenuItems[i]);
        }
        gtk_widget_show_all(menu);

        // This is where the actual dialog starts
        states_dialog = gtk_dialog_new_with_buttons(
                            "States",
                            GTK_WINDOW(mainwindow),
                            GTK_DIALOG_MODAL,
                            "Switch To", 1,
                            "More", 2,
                            "Done", 3,
                            NULL);
        for (int i = 1; i <= 3; i++) {
            GtkWidget *btn = gtk_dialog_get_widget_for_response(GTK_DIALOG(states_dialog), i);
            gtk_widget_set_can_default(btn, false);
        }
        gtk_window_set_resizable(GTK_WINDOW(states_dialog), FALSE);
        no_mwm_resize_borders(states_dialog);
        GtkWidget *container = gtk_bin_get_child(GTK_BIN(states_dialog));
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_container_add(GTK_CONTAINER(container), box);

        currentLabel = gtk_label_new("Current:");
        gtk_box_pack_start(GTK_BOX(box), currentLabel, FALSE, FALSE, 10);

        GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        tree = (GtkTreeView *) gtk_tree_view_new();
        select = gtk_tree_view_get_selection(tree);
        gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);
        GtkWidget *switchToBtn = gtk_dialog_get_widget_for_response(GTK_DIALOG(states_dialog), 1);
        g_signal_connect(G_OBJECT(select), "changed", G_CALLBACK(states_changed_cb), (gpointer) switchToBtn);
        // When I try to pass the dialog pointer as closure data, it gets mangled?!?
        // Most be a casting issue, but I don't get it... So just passing it in a global.
        dlg = states_dialog;
        g_signal_connect(G_OBJECT(tree), "row-activated", G_CALLBACK(row_activated_cb), NULL);
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
        //gtk_cell_renderer_text_set_fixed_height_from_font((GtkCellRendererText *) renderer, 12);
        GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("Foo", renderer, "text", 0, NULL);
        gtk_tree_view_append_column(tree, column);
        gtk_tree_view_set_headers_visible(tree, FALSE);
        gtk_container_add(GTK_CONTAINER(scroll), GTK_WIDGET(tree));
        gtk_widget_set_size_request(scroll, -1, 200);
        gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(scroll), FALSE, FALSE, 10);

        gtk_widget_show_all(states_dialog);
    }

    load_state_names:

    // Make sure a file exists for the current state. This isn't necessarily
    // the case, specifically, right after starting up with a version <= 25
    // state file.
    snprintf(buf, FILENAMELEN, "%s/%s.p42", free42dirname, state.coreName);
    if (!file_exists(buf)) {
        FILE *f = fopen(buf, "w");
        fwrite(PLUS42_MAGIC_STR, 1, 4, f);
        fclose(f);
    }

    snprintf(buf, FILENAMELEN, "Current: %s", state.coreName);
    gtk_label_set_text(GTK_LABEL(currentLabel), buf);

    state_names = (char **) malloc(16 * sizeof(char *));
    int state_size = 0, state_capacity = 16;
    DIR *dir = opendir(free42dirname);
    if (dir != NULL) {
        struct dirent *dent;
        while ((dent = readdir(dir)) != NULL) {
            int namelen = strlen(dent->d_name);
            if (namelen < 4)
                continue;
            if (strcmp(dent->d_name + namelen - 4, ".p42") != 0)
                continue;
            char *stn = (char *) malloc(namelen - 3);
            memcpy(stn, dent->d_name, namelen - 4);
            stn[namelen - 4] = 0;
            if (state_size == state_capacity) {
                state_capacity += 16;
                state_names = (char **) realloc(state_names, state_capacity * sizeof(char *));
            }
            state_names[state_size++] = stn;
        }
        closedir(dir);
        qsort(state_names, state_size, sizeof(char *), case_insens_comparator);
    }

    currentStateIndex = -1;
    GtkListStore *model = gtk_list_store_new(1, G_TYPE_STRING);
    GtkTreeIter iter;
    for (int i = 0; i < state_size; i++) {
        gtk_list_store_append(model, &iter);
        gtk_list_store_set(model, &iter, 0, state_names[i], -1);
        if (strcmp(state_names[i], state.coreName) == 0)
            currentStateIndex = i;
    }
    gtk_tree_view_set_model(tree, GTK_TREE_MODEL(model));

    // TODO: does this leak list-stores? Or is everything taken case of by the
    // GObject reference-counting stuff?

    gtk_window_set_role(GTK_WINDOW(states_dialog), "Plus42 Dialog");
    while (true) {
        int response = gtk_dialog_run(GTK_DIALOG(states_dialog));
        if (response == 3 || response == GTK_RESPONSE_DELETE_EVENT)
            break;
        else if (response == 1) {
            GtkWidget *btn = gtk_dialog_get_widget_for_response(GTK_DIALOG(states_dialog), 1);
            if (!gtk_widget_get_sensitive(btn))
                // User double-clicked on a row, but the Switch To button is disabled
                continue;
            GList *rows = gtk_tree_selection_get_selected_rows(select, NULL);
            if (rows != NULL) {
                GtkTreePath *path = (GtkTreePath *) rows->data;
                int sel;
                sscanf(gtk_tree_path_to_string(path), "%d", &sel);
                g_list_free(rows);
                if (switchTo(state_names[sel]))
                    break;
            }
        } else if (response == 2) {
            GtkWidget *btn = gtk_dialog_get_widget_for_response(GTK_DIALOG(states_dialog), 2);
            gtk_menu_popup(GTK_MENU(menu), NULL, NULL, states_menu_pos_func, btn, 0, GDK_CURRENT_TIME);
        } else if (response == 4) {
            for (int i = 0; i < state_size; i++)
                free(state_names[i]);
            free(state_names);
            goto load_state_names;
        }
    }

    gtk_widget_hide(states_dialog);

    for (int i = 0; i < state_size; i++)
        free(state_names[i]);
    free(state_names);
}

//////////////////////////////////
///// States stuff ends here /////
//////////////////////////////////

static void showPrintOutCB() {
    gtk_window_present(GTK_WINDOW(printwindow));
    gdk_window_focus(gtk_widget_get_window(printwindow), GDK_CURRENT_TIME);
    state.printWindowKnown = 1;
    state.printWindowMapped = 1;
}

static void exportProgramCB() {
    static GtkWidget *sel_dialog = NULL;
    static GtkTreeView *tree;
    static GtkTreeSelection *select;

    if (sel_dialog == NULL) {
        sel_dialog = gtk_dialog_new_with_buttons(
                            "Export Programs",
                            GTK_WINDOW(mainwindow),
                            GTK_DIALOG_MODAL,
                            "_OK", GTK_RESPONSE_ACCEPT,
                            "_Cancel", GTK_RESPONSE_CANCEL,
                            NULL);
        gtk_window_set_resizable(GTK_WINDOW(sel_dialog), FALSE);
        no_mwm_resize_borders(sel_dialog);
        GtkWidget *container = gtk_bin_get_child(GTK_BIN(sel_dialog));
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_container_add(GTK_CONTAINER(container), box);

        GtkWidget *label = gtk_label_new("Select Programs to Export:");
        gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 10);

        GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        tree = (GtkTreeView *) gtk_tree_view_new();
        select = gtk_tree_view_get_selection(tree);
        gtk_tree_selection_set_mode(select, GTK_SELECTION_MULTIPLE);
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("Foo", renderer, "text", 0, NULL);
        gtk_tree_view_append_column(tree, column);
        gtk_tree_view_set_headers_visible(tree, FALSE);
        gtk_container_add(GTK_CONTAINER(scroll), GTK_WIDGET(tree));
        gtk_widget_set_size_request(scroll, -1, 200);
        gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(scroll), FALSE, FALSE, 10);

        gtk_widget_show_all(GTK_WIDGET(sel_dialog));
    }

    char *buf = core_list_programs();

    GtkListStore *model = gtk_list_store_new(1, G_TYPE_STRING);
    if (buf != NULL) {
        int count = ((buf[0] & 255) << 24) | ((buf[1] & 255) << 16) | ((buf[2] & 255) << 8) | (buf[3] & 255);
        char *p = buf + 4;
        GtkTreeIter iter;
        while (count-- > 0) {
            gtk_list_store_append(model, &iter);
            gtk_list_store_set(model, &iter, 0, p, -1);
            p += strlen(p) + 1;
        }
    }
    gtk_tree_view_set_model(tree, GTK_TREE_MODEL(model));

    // TODO: does this leak list-stores? Or is everything taken case of by the
    // GObject reference-counting stuff?

    gtk_window_set_role(GTK_WINDOW(sel_dialog), "Plus42 Dialog");
    bool cancelled = gtk_dialog_run(GTK_DIALOG(sel_dialog)) != GTK_RESPONSE_ACCEPT;
    gtk_widget_hide(sel_dialog);
    if (cancelled) {
        free(buf);
        return;
    }

    int count = gtk_tree_selection_count_selected_rows(select);
    if (count == 0) {
        free(buf);
        return;
    }

    int *p2 = (int *) malloc(count * sizeof(int));
    // TODO - handle memory allocation failure
    GList *rows = gtk_tree_selection_get_selected_rows(select, NULL);
    GList *item = rows;
    int i = 0;
    while (item != NULL) {
        GtkTreePath *path = (GtkTreePath *) item->data;
        char *pathstring = gtk_tree_path_to_string(path);
        sscanf(pathstring, "%d", p2 + i);
        item = item->next;
        i++;
    }
    g_list_free(rows);

    char suggested_name[50] = "Untitled.raw";
    char *p = buf + 4;
    int sel = p2[0];
    while (sel > 0) {
        p += strlen(p) + 1;
        sel--;
    }
    if (p[0] == '"') {
        char *closing_quote = strchr(p + 1, '"');
        if (closing_quote != NULL) {
            int len = closing_quote - p - 1;
            for (int i = 0; i < len; i++) {
                char c = p[i + 1];
                if (c == '/' || c == 10)
                    c = '_';
                suggested_name[i] = c;
            }
            suggested_name[len] = 0;
            strcat(suggested_name, ".raw");
        }
    }
    free(buf);

    static GtkWidget *save_dialog = NULL;
    if (save_dialog == NULL)
        save_dialog = make_file_select_dialog("Export Programs",
                "Program Files (*.raw)\0*.[Rr][Aa][Ww]\0All Files (*.*)\0*\0",
                true, mainwindow);

    char *filename = NULL;
    gtk_window_set_role(GTK_WINDOW(save_dialog), "Plus42 Dialog");
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(save_dialog), suggested_name);
    if (gtk_dialog_run(GTK_DIALOG(save_dialog)) == GTK_RESPONSE_ACCEPT)
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(save_dialog));
    gtk_widget_hide(GTK_WIDGET(save_dialog));
    if (filename == NULL) {
        free(p2);
        return;
    }

    char export_file_name[FILENAMELEN];
    strcpy(export_file_name, filename);
    g_free(filename);
    if (strncmp(gtk_file_filter_get_name(
                    gtk_file_chooser_get_filter(
                        GTK_FILE_CHOOSER(save_dialog))), "All", 3) != 0)
        appendSuffix(export_file_name, ".raw");

    if (file_exists(export_file_name)) {
        GtkWidget *msg = gtk_message_dialog_new(GTK_WINDOW(mainwindow),
                                                GTK_DIALOG_MODAL,
                                                GTK_MESSAGE_QUESTION,
                                                GTK_BUTTONS_YES_NO,
                                                "Replace existing \"%s\"?",
                                                export_file_name);
        gtk_window_set_title(GTK_WINDOW(msg), "Replace?");
        gtk_window_set_role(GTK_WINDOW(msg), "Plus42 Dialog");
        cancelled = gtk_dialog_run(GTK_DIALOG(msg)) != GTK_RESPONSE_YES;
        gtk_widget_destroy(msg);
        if (cancelled) {
            free(p2);
            return;
        }
    }

    core_export_programs(count, p2, export_file_name);
    free(p2);
}

static GtkWidget *make_file_select_dialog(const char *title,
                                          const char *pattern,
                                          bool save,
                                          GtkWidget *owner) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
                        title,
                        GTK_WINDOW(owner),
                        save ? GTK_FILE_CHOOSER_ACTION_SAVE
                             : GTK_FILE_CHOOSER_ACTION_OPEN,
                        "_Cancel", GTK_RESPONSE_CANCEL,
                        save ? "_Save" : "_Open",
                        GTK_RESPONSE_ACCEPT,
                        NULL);
    const char *p = pattern;
    while (1) {
        const char *descr, *ext;
        int n = strlen(p);
        if (n == 0)
            break;
        descr = p;
        p += n + 1;
        n = strlen(p);
        if (n == 0)
            break;
        ext = p;
        p += n + 1;

        GtkFileFilter *filter = gtk_file_filter_new();
        gtk_file_filter_add_pattern(filter, ext);
        gtk_file_filter_set_name(filter, descr);
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    }
    return dialog;
}

static void importProgramCB() {
    static GtkWidget *dialog = NULL;

    if (dialog == NULL)
        dialog = make_file_select_dialog("Import Programs",
                "Program Files (*.raw)\0*.[Rr][Aa][Ww]\0All Files (*.*)\0*\0",
                false, mainwindow);

    gtk_window_set_role(GTK_WINDOW(dialog), "Plus42 Dialog");
    bool cancelled = gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_ACCEPT;
    gtk_widget_hide(dialog);
    if (cancelled)
        return;

    char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    if (filename == NULL)
        return;

    char filenamebuf[FILENAMELEN];
    strncpy(filenamebuf, filename, FILENAMELEN);
    filenamebuf[FILENAMELEN - 1] = 0;
    g_free(filename);

    if (strncmp(gtk_file_filter_get_name(
                    gtk_file_chooser_get_filter(
                        GTK_FILE_CHOOSER(dialog))), "All", 3) != 0)
        appendSuffix(filenamebuf, ".raw");

    core_import_programs(0, filenamebuf);
    redisplay();
}

static void paperAdvanceCB() {
    static const char *bits = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
    shell_print("", 0, bits, 18, 0, 0, 143, 9);
}

static char *tb;
static int tblen, tbcap;

static void tbwriter(const char *text, int length) {
    if (tblen + length > tbcap) {
        tbcap += length + 8192;
        tb = (char *) realloc(tb, tbcap);
    }
    if (tb != NULL) {
        memcpy(tb + tblen, text, length);
        tblen += length;
    }
}

static void tbnewliner() {
    tbwriter("\n", 1);
}

static void tbnonewliner() {
    // No-op
}

static void copyPrintAsTextCB() {
    tb = NULL;
    tblen = tbcap = 0;

    int len = print_text_bottom - print_text_top;
    if (len < 0)
        len += PRINT_TEXT_SIZE;
    // Calculate effective top, since printout_top can point
    // at a truncated line, and we want to skip those when
    // copying
    int top = printout_bottom - 2 * print_text_pixel_height;
    if (top < 0)
        top += PRINT_LINES;
    int p = print_text_top;
    int pixel_v = 0;
    while (len > 0) {
        int z = print_text[p++];
        if (z >= 254) {
            int height;
            if (z == 254) {
                if (p == PRINT_TEXT_SIZE)
                    p = 0;
                height = print_text[p++] << 8;
                if (p == PRINT_TEXT_SIZE)
                    p = 0;
                height |= print_text[p++];
                len -= 2;
            } else {
                height = 16;
            }
            char buf[36];
            for (int v = 0; v < height; v += 2) {
                int nv = v == height - 1 ? 1 : 2;
                for (int vv = 0; vv < nv; vv++) {
                    int V = top + (pixel_v + v + vv) * 2;
                    if (V >= PRINT_LINES)
                        V -= PRINT_LINES;
                    for (int h = 0; h < 18; h++) {
                        unsigned char a = print_bitmap[V * PRINT_BYTESPERLINE + 2 * h + 1];
                        unsigned char b = print_bitmap[V * PRINT_BYTESPERLINE + 2 * h];
                        buf[vv * 18 + h] = (a & 128) | ((a & 32) << 1) | ((a & 8) << 2) | ((a & 2) << 3) | ((b & 128) >> 4) | ((b & 32) >> 3) | ((b & 8) >> 2) | ((b & 2) >> 1);
                    }
                }
                shell_spool_bitmap_to_txt(buf, 18, 0, 0, 143, nv, tbwriter, tbnewliner);
            }
            pixel_v += height;
        } else {
            if (p + z < PRINT_TEXT_SIZE) {
                shell_spool_txt((const char *) (print_text + p), z, tbwriter, tbnewliner);
                p += z;
            } else {
                int d = PRINT_TEXT_SIZE - p;
                shell_spool_txt((const char *) (print_text + p), d, tbwriter, tbnonewliner);
                shell_spool_txt((const char *) print_text, z - d, tbwriter, tbnewliner);
                p = z - d;
            }
            len -= z;
            pixel_v += 9;
        }
        len--;
    }
    tbwriter("\0", 1);

    GtkClipboard *clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_text(clip, tb, -1);
    clip = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
    gtk_clipboard_set_text(clip, tb, -1);
    free(tb);
}

static void copyPrintAsImageCB() {
    int length = printout_bottom - printout_top;
    if (length < 0)
        length += PRINT_LINES;
    bool empty = length == 0;
    if (empty)
        length += 2;

    GdkPixbuf *buf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE,
                                    8, 358, length);
    int d_bpl = gdk_pixbuf_get_rowstride(buf);
    guchar *d1 = gdk_pixbuf_get_pixels(buf);

    if (empty) {
        memset(d1, 255, 2148);
    } else {
        for (int v = 0; v < length; v++) {
            int v2 = printout_top + v;
            if (v2 >= PRINT_LINES)
                v2 -= PRINT_LINES;
            int v3 = v2 * 36;
            guchar *dst = d1;
            for (int h = 0; h < 358; h++) {
                unsigned char c;
                if (h < 36 || h >= 322)
                    c = 255;
                else if ((print_bitmap[v3 + ((h - 36) >> 3)] & (1 << ((h - 36) & 7))) == 0)
                    c = 255;
                else
                    c = 0;
                *dst++ = c;
                *dst++ = c;
                *dst++ = c;
            }
            d1 += d_bpl;
        }
    }

    GtkClipboard *clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_image(clip, buf);
}

static void clearPrintOutCB() {
    printout_top = 0;
    printout_bottom = 0;
    print_text_top = 0;
    print_text_bottom = 0;
    print_text_pixel_height = 0;
    gtk_widget_set_size_request(print_widget, 358, 1);

    if (print_gif != NULL) {
        shell_finish_gif(gif_seeker, gif_writer);
        fclose(print_gif);
        print_gif = NULL;
    }
}

struct browse_file_info {
    const char *title;
    const char *patterns;
    GtkWidget *textfield;
    browse_file_info(const char *t, const char *p, GtkWidget *tf)
                        : title(t), patterns(p), textfield(tf) {}
};

static void browse_file(GtkButton *button, gpointer cd) {
    browse_file_info *info = (browse_file_info *) cd;
    GtkWidget *dialog = gtk_widget_get_toplevel(GTK_WIDGET(button));
    GtkWidget *save_dialog = make_file_select_dialog(
                                    info->title, info->patterns, true, dialog);
    const char *filename = gtk_entry_get_text(GTK_ENTRY(info->textfield));
    if (filename[0] == '/')
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(save_dialog), filename);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(save_dialog), filename);

    gtk_window_set_role(GTK_WINDOW(save_dialog), "Plus42 Dialog");
    if (gtk_dialog_run(GTK_DIALOG(save_dialog)) == GTK_RESPONSE_ACCEPT) {
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(save_dialog));

        char filenamebuf[FILENAMELEN];
        strncpy(filenamebuf, filename, FILENAMELEN);
        filenamebuf[FILENAMELEN - 1] = 0;
        const gchar *filtername =
            gtk_file_filter_get_name(
                        gtk_file_chooser_get_filter(
                            GTK_FILE_CHOOSER(save_dialog)));
        if (strncmp(filtername, "Text", 4) == 0)
            appendSuffix(filenamebuf, ".txt");
        else if (strncmp(filtername, "GIF", 3) == 0)
            appendSuffix(filenamebuf, ".gif");

        gtk_entry_set_text(GTK_ENTRY(info->textfield), filenamebuf);
    }
    gtk_widget_destroy(GTK_WIDGET(save_dialog));
}

static void preferencesCB() {
    static GtkWidget *dialog = NULL;
    static GtkWidget *singularmatrix;
    static GtkWidget *matrixoutofrange;
    static GtkWidget *autorepeat;
    static GtkWidget *localizedcopypaste;
    static GtkWidget *repaintwholedisplay;
    static GtkWidget *printtotext;
    static GtkWidget *textpath;
    static GtkWidget *printtogif;
    static GtkWidget *gifpath;
    static GtkWidget *gifheight;

    if (dialog == NULL) {
        dialog = gtk_dialog_new_with_buttons(
                            "Preferences",
                            GTK_WINDOW(mainwindow),
                            GTK_DIALOG_MODAL,
                            "_OK", GTK_RESPONSE_ACCEPT,
                            "_Cancel", GTK_RESPONSE_CANCEL,
                            NULL);
        gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
        no_mwm_resize_borders(dialog);
        GtkWidget *container = gtk_bin_get_child(GTK_BIN(dialog));
        GtkWidget *grid = gtk_grid_new();
        gtk_container_add(GTK_CONTAINER(container), grid);

        singularmatrix = gtk_check_button_new_with_label("Inverting or solving a singular matrix yields \"Singular Matrix\" error");
        gtk_grid_attach(GTK_GRID(grid), singularmatrix, 0, 0, 4, 1);
        matrixoutofrange = gtk_check_button_new_with_label("Overflows during matrix operations yield \"Out of Range\" error");
        gtk_grid_attach(GTK_GRID(grid), matrixoutofrange, 0, 1, 4, 1);
        autorepeat = gtk_check_button_new_with_label("Auto-repeat for number entry and ALPHA mode");
        gtk_grid_attach(GTK_GRID(grid), autorepeat, 0, 2, 4, 1);
        localizedcopypaste = gtk_check_button_new_with_label("Localized Copy & Paste");
        gtk_grid_attach(GTK_GRID(grid), localizedcopypaste, 0, 3, 4, 1);
        repaintwholedisplay = gtk_check_button_new_with_label("Always repaint entire display");
        gtk_grid_attach(GTK_GRID(grid), repaintwholedisplay, 0, 4, 4, 1);
        printtotext = gtk_check_button_new_with_label("Print to text file:");
        gtk_grid_attach(GTK_GRID(grid), printtotext, 0, 5, 1, 1);
        textpath = gtk_entry_new();
        gtk_grid_attach(GTK_GRID(grid), textpath, 1, 5, 2, 1);
        GtkWidget *browse1 = gtk_button_new_with_label("Browse...");
        gtk_grid_attach(GTK_GRID(grid), browse1, 3, 5, 1, 1);
        printtogif = gtk_check_button_new_with_label("Print to GIF file:");
        gtk_grid_attach(GTK_GRID(grid), printtogif, 0, 6, 1, 1);
        gifpath = gtk_entry_new();
        gtk_grid_attach(GTK_GRID(grid), gifpath, 1, 6, 2, 1);
        GtkWidget *browse2 = gtk_button_new_with_label("Browse...");
        gtk_grid_attach(GTK_GRID(grid), browse2, 3, 6, 1, 1);
        GtkWidget *label = gtk_label_new("Maximum GIF height (pixels):");
        gtk_grid_attach(GTK_GRID(grid), label, 1, 7, 1, 1);
        gifheight = gtk_entry_new();
        gtk_entry_set_max_length(GTK_ENTRY(gifheight), 5);
        gtk_grid_attach(GTK_GRID(grid), gifheight, 2, 7, 1, 1);

        g_signal_connect(G_OBJECT(browse1), "clicked", G_CALLBACK(browse_file),
                (gpointer) new browse_file_info("Select Text File Name",
                                                "Text (*.txt)\0*.[Tt][Xx][Tt]\0All Files (*.*)\0*\0",
                                                textpath));
        g_signal_connect(G_OBJECT(browse2), "clicked", G_CALLBACK(browse_file),
                (gpointer) new browse_file_info("Select GIF File Name",
                                                "GIF (*.gif)\0*.[Gg][Ii][Ff]\0All Files (*.*)\0*\0",
                                                gifpath));

        gtk_widget_show_all(GTK_WIDGET(dialog));
    }

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(singularmatrix), core_settings.matrix_singularmatrix);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(matrixoutofrange), core_settings.matrix_outofrange);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(autorepeat), core_settings.auto_repeat);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(localizedcopypaste), core_settings.localized_copy_paste);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(printtotext), state.printerToTxtFile);
    gtk_entry_set_text(GTK_ENTRY(textpath), state.printerTxtFileName);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(printtogif), state.printerToGifFile);
    gtk_entry_set_text(GTK_ENTRY(gifpath), state.printerGifFileName);
    char maxlen[6];
    snprintf(maxlen, 6, "%d", state.printerGifMaxLength);
        gtk_entry_set_text(GTK_ENTRY(gifheight), maxlen);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(repaintwholedisplay), !state.old_repaint);

    gtk_window_set_role(GTK_WINDOW(dialog), "Plus42 Dialog");
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        core_settings.matrix_singularmatrix = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(singularmatrix));
        core_settings.matrix_outofrange = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(matrixoutofrange));
        core_settings.auto_repeat = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(autorepeat));
        core_settings.localized_copy_paste = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(localizedcopypaste));

        state.printerToTxtFile = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(printtotext));
        char *old = strclone(state.printerTxtFileName);
        const char *s = gtk_entry_get_text(GTK_ENTRY(textpath));
        strncpy(state.printerTxtFileName, s, FILENAMELEN);
        state.printerTxtFileName[FILENAMELEN - 1] = 0;
        appendSuffix(state.printerTxtFileName, ".txt");
        if (print_txt != NULL && (!state.printerToTxtFile || strcmp(state.printerTxtFileName, old) != 0)) {
            fclose(print_txt);
            print_txt = NULL;
        }
        free(old);

        state.printerToGifFile = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(printtogif));
        old = strclone(state.printerGifFileName);
        s = gtk_entry_get_text(GTK_ENTRY(gifpath));
        strncpy(state.printerGifFileName, s, FILENAMELEN);
        state.printerGifFileName[FILENAMELEN - 1] = 0;
        appendSuffix(state.printerGifFileName, ".gif");
        if (print_gif != NULL && (!state.printerToGifFile || strcmp(state.printerGifFileName, old) != 0)) {
            shell_finish_gif(gif_seeker, gif_writer);
            fclose(print_gif);
            print_gif = NULL;
            gif_seq = -1;
        }
        free(old);

        s = gtk_entry_get_text(GTK_ENTRY(gifheight));
        if (sscanf(s, "%d", &state.printerGifMaxLength) == 1) {
            if (state.printerGifMaxLength < 16)
                state.printerGifMaxLength = 16;
            else if (state.printerGifMaxLength > 32767) state.printerGifMaxLength = 32767;
        } else
            state.printerGifMaxLength = 256;

        state.old_repaint = !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(repaintwholedisplay));
    }

    gtk_widget_hide(GTK_WIDGET(dialog));
}

static void appendSuffix(char *path, const char *suffix) {
    int len = strlen(path);
    int slen = strlen(suffix);
    if (len == 0 || len >= FILENAMELEN - slen)
        return;
    if (len >= slen && strcasecmp(path + len - slen, suffix) != 0)
        strcat(path, suffix);
}

static void copyCB() {
    char *buf = core_copy();
    GtkClipboard *clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_text(clip, buf, -1);
    clip = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
    gtk_clipboard_set_text(clip, buf, -1);
    free(buf);
}

static void paste2(GtkClipboard *clip, const gchar *text, gpointer cd) {
    if (text != NULL) {
        core_paste(text);
        redisplay();
        // GTK will free the text once the callback returns.
    }
}

static void pasteCB() {
    GtkClipboard *clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_request_text(clip, paste2, NULL);
}

static bool focus_ok_button(GtkWindow *window, GtkWidget *container) {
    GList *children = gtk_container_get_children(GTK_CONTAINER(container));
    if (children == NULL)
        return false;
    GList *child = children;
    while (child != NULL) {
        GtkWidget *w = (GtkWidget *) child->data;
        if (GTK_IS_BUTTON(w) && !GTK_IS_LINK_BUTTON(w)) {
            gtk_window_set_focus(window, w);
            g_list_free(children);
            return true;
        } else if (GTK_IS_CONTAINER(w)) {
            if (focus_ok_button(window, w)) {
                g_list_free(children);
                return true;
            }
        }
        child = child->next;
    }
    g_list_free(children);
    return false;
}

static void documentationCB() {
#if GTK_MINOR_VERSION >= 22
    gtk_show_uri_on_window(GTK_WINDOW(mainwindow), "https://thomasokken.com/plus42/#doc", GDK_CURRENT_TIME, NULL);
#else
    gtk_show_uri(gtk_widget_get_screen(mainwindow), "https://thomasokken.com/plus42/#doc", GDK_CURRENT_TIME, NULL);
#endif
}

static void websiteCB() {
#if GTK_MINOR_VERSION >= 22
    gtk_show_uri_on_window(GTK_WINDOW(mainwindow), "https://thomasokken.com/plus42/", GDK_CURRENT_TIME, NULL);
#else
    gtk_show_uri(gtk_widget_get_screen(mainwindow), "https://thomasokken.com/plus42/", GDK_CURRENT_TIME, NULL);
#endif
}

static void otherWebsiteCB() {
#if GTK_MINOR_VERSION >= 22
    gtk_show_uri_on_window(GTK_WINDOW(mainwindow), "https://thomasokken.com/free42/", GDK_CURRENT_TIME, NULL);
#else
    gtk_show_uri(gtk_widget_get_screen(mainwindow), "https://thomasokken.com/free42/", GDK_CURRENT_TIME, NULL);
#endif
}

static void keyboardShortcutsCB() {
    keyboardShortcutsShowing = !keyboardShortcutsShowing;
    gtk_check_menu_item_set_active(keyboardShortcutsMenuItem, keyboardShortcutsShowing);
    GdkWindow *win = gtk_widget_get_window(calc_widget);
    gdk_window_invalidate_rect(win, NULL, FALSE);
}

static void editKeymapCB() {
    char cmd[FILENAMELEN + 12];
    snprintf(cmd, FILENAMELEN + 12, "xdg-open '%s'", keymapfilename);
    system(cmd);
}

static void aboutCB() {
    static GtkWidget *about = NULL;

    if (about == NULL) {
        about = gtk_dialog_new_with_buttons(
                            "About Plus42",
                            GTK_WINDOW(mainwindow),
                            GTK_DIALOG_MODAL,
                            "_OK",
                            GTK_RESPONSE_ACCEPT,
                            NULL);
        gtk_window_set_resizable(GTK_WINDOW(about), FALSE);
        no_mwm_resize_borders(about);
        GtkWidget *container = gtk_bin_get_child(GTK_BIN(about));
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_container_add(GTK_CONTAINER(container), box);
        GtkWidget *image = gtk_image_new_from_pixbuf(icon_48);
        gtk_box_pack_start(GTK_BOX(box), image, FALSE, FALSE, 10);
        GtkWidget *box2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        GtkWidget *version = gtk_label_new("Plus42 " VERSION);
        gtk_misc_set_alignment(GTK_MISC(version), 0, 0);
        gtk_box_pack_start(GTK_BOX(box2), version, FALSE, FALSE, 10);
        GtkWidget *author = gtk_label_new("\302\251 2004-2025 Thomas Okken");
        gtk_misc_set_alignment(GTK_MISC(author), 0, 0);
        gtk_box_pack_start(GTK_BOX(box2), author, FALSE, FALSE, 0);
        GtkWidget *websitelink = gtk_link_button_new("https://thomasokken.com/plus42/");
        GtkWidget *websitebox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_box_pack_start(GTK_BOX(websitebox), websitelink, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box2), websitebox, FALSE, FALSE, 0);
        GtkWidget *forumlink = gtk_link_button_new("https://thomasokken.com/plus42/#doc");
        GtkWidget *forumbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_box_pack_start(GTK_BOX(forumbox), forumlink, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box2), forumbox, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), box2, FALSE, FALSE, 0);
        focus_ok_button(GTK_WINDOW(about), container);
        gtk_widget_show_all(GTK_WIDGET(about));
    } else {
        GtkWidget *container = gtk_bin_get_child(GTK_BIN(about));
        focus_ok_button(GTK_WINDOW(about), container);
    }

    gtk_window_set_role(GTK_WINDOW(about), "Plus42 Dialog");
    gtk_dialog_run(GTK_DIALOG(about));
    gtk_widget_hide(GTK_WIDGET(about));
}

static gboolean delete_cb(GtkWidget *w, GdkEventAny *ev) {
    quit();
    return TRUE;
}

static gboolean delete_print_cb(GtkWidget *w, GdkEventAny *ev) {
    gint x, y;
    gtk_window_get_position(GTK_WINDOW(printwindow), &x, &y);
    state.printWindowX = x;
    state.printWindowY = y;
    state.printWindowMapped = 0;
    state.printWindowKnown = 1;
    gtk_widget_hide(GTK_WIDGET(printwindow));
    return TRUE;
}

static bool is_dark(GtkWidget *w) {
    GtkStyle *style = gtk_widget_get_style(w);
    GdkColor c = style->bg[GTK_STATE_NORMAL];
    return 0.299 * c.red + 0.587 * c.green + 0.114 * c.blue < 32768;
}

static gboolean draw_cb(GtkWidget *w, cairo_t *cr, gpointer cd) {
    cairo_save(cr);
    int win_width, win_height, skin_width, skin_height;
    skin_get_window_size(&win_width, &win_height);
    skin_get_size(&skin_width, &skin_height);
    cairo_scale(cr, ((double) win_width) / skin_width, ((double) win_height) / skin_height);

    allow_paint = true;
    bool only_disp = need_to_paint_only_display(cr);
    if (!only_disp)
        skin_repaint(cr);
    skin_repaint_display(cr);
    if (!only_disp) {
        if (ann_updown)
            skin_repaint_annunciator(cr, 1);
        if (ann_shift)
            skin_repaint_annunciator(cr, 2);
        if (ann_print)
            skin_repaint_annunciator(cr, 3);
        if (ann_run)
            skin_repaint_annunciator(cr, 4);
        if (ann_battery)
            skin_repaint_annunciator(cr, 5);
        if (ann_g)
            skin_repaint_annunciator(cr, 6);
        if (ann_rad)
            skin_repaint_annunciator(cr, 7);
        if (ckey != 0)
            skin_repaint_key(cr, skey, 1);
    } else {
        if (skey >= -7 && skey <= -2)
            skin_repaint_key(cr, skey, 1);
    }

    if (keyboardShortcutsShowing)
        skin_draw_keyboard_shortcuts(cr);
    if (is_dark(w))
        skin_make_darker(cr);

    cairo_restore(cr);
    return TRUE;
}

static gboolean print_draw_cb(GtkWidget *w, cairo_t *cr, gpointer cd) {
    repaint_printout(cr, is_dark(w));
    return TRUE;
}

static gboolean print_key_cb(GtkWidget *w, GdkEventKey *event, gpointer cd) {

    // This is a bit hacky, but I want the Ctrl-<Key>
    // shortcuts to work even when the Print-Out window
    // is on top.

    if (event->type != GDK_KEY_PRESS)
        return TRUE;

    bool ctrl = (event->state & GDK_CONTROL_MASK) != 0;
    bool alt = (event->state & GDK_MOD1_MASK) != 0;
    bool shift = (event->state & GDK_SHIFT_MASK) != 0;
    if (!ctrl || alt || shift)
        return TRUE;

    switch (event->keyval) {
        case GDK_KEY_a:
            paperAdvanceCB();
            break;
        case GDK_KEY_t:
            copyPrintAsTextCB();
            break;
        case GDK_KEY_i:
            copyPrintAsImageCB();
            break;
        case GDK_KEY_d:
            clearPrintOutCB();
            break;
        case GDK_KEY_q:
            quit();
            break;
        case GDK_KEY_c:
            copyCB();
            break;
        case GDK_KEY_v:
            pasteCB();
            break;
    }
    return TRUE;
}

static void shell_keydown(bool cshift) {
    GdkWindow *win = gtk_widget_get_window(calc_widget);

    int repeat;
    bool keep_running;
    if (skey == -1)
        skey = skin_find_skey(ckey, cshift);
    skin_invalidate_key(win, skey);
    if (timeout3_id != 0 && (macro != NULL || ckey != 28 /* KEY_SHIFT */)) {
        g_source_remove(timeout3_id);
        timeout3_id = 0;
        core_timeout3(false);
    }

    if (macro != NULL) {
        if (macro_type != 0) {
            keep_running = core_keydown_command((const char *) macro, macro_type == 2, &enqueued, &repeat);
        } else {
            if (*macro == 0) {
                squeak();
                return;
            }
            bool one_key_macro = macro[1] == 0 || (macro[2] == 0 && macro[0] == 28);
            if (one_key_macro) {
                while (*macro != 0) {
                    keep_running = core_keydown(*macro++, &enqueued, &repeat);
                    if (*macro != 0 && !enqueued)
                        core_keyup();
                }
            } else {
                bool waitForProgram = !program_running();
                while (*macro != 0) {
                    keep_running = core_keydown(*macro++, &enqueued, &repeat);
                    if (*macro != 0 && !enqueued)
                        keep_running = core_keyup();
                    while (waitForProgram && keep_running)
                        keep_running = core_keydown(0, &enqueued, &repeat);
                }
                repeat = 0;
            }
        }
    } else
        keep_running = core_keydown(ckey, &enqueued, &repeat);

    if (quit_flag)
        quit();
    if (keep_running)
        enable_reminder();
    else {
        disable_reminder();
        if (timeout_id != 0)
            g_source_remove(timeout_id);
        if (repeat != 0)
            timeout_id = g_timeout_add(repeat == 1 ? 1000 : 500, repeater, NULL);
        else if (!enqueued)
            timeout_id = g_timeout_add(250, timeout1, NULL);
    }
}

static void shell_keyup() {
    GdkWindow *win = gtk_widget_get_window(calc_widget);
    skin_invalidate_key(win, skey);

    ckey = 0;
    skey = -1;
    if (timeout_id != 0) {
        g_source_remove(timeout_id);
        timeout_id = 0;
    }
    if (!enqueued) {
        bool keep_running = core_keyup();
        if (quit_flag)
            quit();
        if (keep_running)
            enable_reminder();
        else
            disable_reminder();
    }
}

static gboolean button_cb(GtkWidget *w, GdkEventButton *event, gpointer cd) {
    if (event->type == GDK_BUTTON_PRESS) {
        if (ckey == 0) {
            int win_width, win_height, skin_width, skin_height;
            skin_get_window_size(&win_width, &win_height);
            skin_get_size(&skin_width, &skin_height);
            int x = (int) (event->x * skin_width / win_width);
            int y = (int) (event->y * skin_height / win_height);
            skin_find_key(x, y, ann_shift != 0, &skey, &ckey);
            if (ckey != 0) {
                macro = skin_find_macro(ckey, &macro_type);
                shell_keydown(ann_shift != 0);
                mouse_key = true;
            }
        }
    } else if (event->type == GDK_BUTTON_RELEASE) {
        if (ckey != 0 && mouse_key)
            shell_keyup();
    }
    return TRUE;
}

static gboolean key_cb(GtkWidget *w, GdkEventKey *event, gpointer cd) {
    if (event->type == GDK_KEY_PRESS) {
        if (event->hardware_keycode == active_keycode)
            // Auto-repeat
            return TRUE;
        if (ckey == 0 || !mouse_key) {
            int i;

            bool printable = event->length == 1 && event->string[0] >= 32 && event->string[0] <= 126;
            just_pressed_shift = false;

            if (event->keyval == GDK_KEY_Shift_L || event->keyval == GDK_KEY_Shift_R) {
                just_pressed_shift = true;
                return TRUE;
            }
            bool ctrl = (event->state & GDK_CONTROL_MASK) != 0;
            bool alt = (event->state & GDK_MOD1_MASK) != 0;
            bool shift = (event->state & GDK_SHIFT_MASK) != 0;
            bool cshift = ann_shift != 0;

            if (ckey != 0) {
                shell_keyup();
                active_keycode = 0;
            }

            bool exact;
            unsigned char *key_macro = skin_keymap_lookup(event->keyval,
                                printable, ctrl, alt, shift, cshift, &exact);
            if (key_macro == NULL || !exact) {
                for (i = 0; i < keymap_length; i++) {
                    keymap_entry *entry = keymap + i;
                    if (ctrl == entry->ctrl
                            && alt == entry->alt
                            && (printable || shift == entry->shift)
                            && event->keyval == entry->keyval) {
                        if (shift == entry->shift && cshift == entry->cshift) {
                            key_macro = entry->macro;
                            break;
                        } else {
                            if ((shift || !entry->shift) && (cshift || !entry->cshift) && key_macro == NULL)
                                key_macro = entry->macro;
                        }
                    }
                }
            }

            if (key_macro == NULL || (key_macro[0] != 36 || key_macro[1] != 0)
                    && (key_macro[0] != 28 || key_macro[1] != 36 || key_macro[2] != 0)) {
                // The test above is to make sure that whatever mapping is in
                // effect for R/S will never be overridden by the special cases
                // for the ALPHA and A..F menus.
                if (!ctrl && !alt) {
                    char c = event->string[0];
                    if (printable && core_alpha_menu() != 0) {
                        if (c >= 'a' && c <= 'z')
                            c = c + 'A' - 'a';
                        else if (c >= 'A' && c <= 'Z')
                            c = c + 'a' - 'A';
                        ckey = 1024 + c;
                        skey = -1;
                        macro = NULL;
                        shell_keydown(false);
                        mouse_key = false;
                        active_keycode = event->hardware_keycode;
                        return TRUE;
                    } else if (core_hex_menu() && ((c >= 'a' && c <= 'f')
                                                || (c >= 'A' && c <= 'F'))) {
                        if (c >= 'a' && c <= 'f')
                            ckey = c - 'a' + 1;
                        else
                            ckey = c - 'A' + 1;
                        skey = -1;
                        macro = NULL;
                        shell_keydown(false);
                        mouse_key = false;
                        active_keycode = event->hardware_keycode;
                        return TRUE;
                    } else if (event->keyval == GDK_KEY_Left
                            || event->keyval == GDK_KEY_Right
                            || event->keyval == GDK_KEY_Delete) {
                        int which;
                        if (event->keyval == GDK_KEY_Left)
                            which = shift ? 2 : 1;
                        else if (event->keyval == GDK_KEY_Right)
                            which = shift ? 4 : 3;
                        else if (event->keyval == GDK_KEY_Delete)
                            which = 5;
                        else
                            which = 0;
                        if (which != 0) {
                            which = core_special_menu_key(which);
                            if (which != 0) {
                                ckey = which;
                                skey = -1;
                                macro = NULL;
                                shell_keydown(false);
                                mouse_key = false;
                                active_keycode = event->hardware_keycode;
                                return TRUE;
                            }
                        }
                    }
                }
            }

            if (key_macro != NULL) {
                // A keymap entry is a sequence of zero or more calculator
                // keystrokes (1..37) and/or macros (38..255). We expand
                // macros here before invoking shell_keydown().
                // If the keymap entry is one key, or two keys with the
                // first being 'shift', we highlight the key in question
                // by setting ckey; otherwise, we set ckey to -10, which
                // means no skin key will be highlighted.
                ckey = -10;
                skey = -1;
                bool skin_shift = cshift;
                if (key_macro[0] != 0)
                    if (key_macro[1] == 0)
                        ckey = key_macro[0];
                    else if (key_macro[2] == 0 && key_macro[0] == 28) {
                        ckey = key_macro[1];
                        skin_shift = true;
                    }
                bool needs_expansion = false;
                for (int j = 0; key_macro[j] != 0; j++)
                    if (key_macro[j] > 37) {
                        needs_expansion = true;
                        break;
                    }
                if (needs_expansion) {
                    static unsigned char macrobuf[1024];
                    int p = 0;
                    for (int j = 0; key_macro[j] != 0 && p < 1023; j++) {
                        int c = key_macro[j];
                        if (c <= 37)
                            macrobuf[p++] = c;
                        else {
                            unsigned char *m = skin_find_macro(c, &macro_type);
                            if (m != NULL)
                                while (*m != 0 && p < 1023)
                                    macrobuf[p++] = *m++;
                        }
                    }
                    macrobuf[p] = 0;
                    macro = macrobuf;
                } else {
                    macro = key_macro;
                    macro_type = 0;
                }
                shell_keydown(skin_shift);
                mouse_key = false;
                active_keycode = event->hardware_keycode;
            }
        }
    } else if (event->type == GDK_KEY_RELEASE) {
        if (ckey == 0) {
            if (just_pressed_shift && (event->keyval == GDK_KEY_Shift_L
                                    || event->keyval == GDK_KEY_Shift_R)) {
                ckey = 28;
                skey = -1;
                macro = NULL;
                shell_keydown(false);
                shell_keyup();
            }
        } else {
            if (!mouse_key && event->hardware_keycode == active_keycode) {
                shell_keyup();
                active_keycode = 0;
            }
        }
    }
    return TRUE;
}

static void enable_reminder() {
    if (reminder_id == 0)
        reminder_id = g_idle_add(reminder, NULL);
    if (timeout_id != 0) {
        g_source_remove(timeout_id);
        timeout_id = 0;
    }
}

static void disable_reminder() {
    if (reminder_id != 0) {
        g_source_remove(reminder_id);
        reminder_id = 0;
    }
}

static gboolean repeater(gpointer cd) {
    int repeat = core_repeat();
    if (repeat != 0)
        timeout_id = g_timeout_add(repeat == 1 ? 200 : repeat == 2 ? 100 : 500, repeater, NULL);
    else
        timeout_id = g_timeout_add(250, timeout1, NULL);
    return FALSE;
}

static gboolean timeout1(gpointer cd) {
    if (ckey != 0) {
        core_keytimeout1();
        timeout_id = g_timeout_add(1750, timeout2, NULL);
    } else
        timeout_id = 0;
    return FALSE;
}

static gboolean timeout2(gpointer cd) {
    if (ckey != 0)
        core_keytimeout2();
    timeout_id = 0;
    return FALSE;
}

static gboolean timeout3(gpointer cd) {
    bool keep_running = core_timeout3(true);
    timeout3_id = 0;
    if (keep_running)
        enable_reminder();
    return FALSE;
}

static gboolean battery_checker(gpointer cd) {
    shell_low_battery();
    return TRUE;
}

static void repaint_printout(cairo_t *cr, bool dark) {
    GdkRectangle clip;
    if (!gdk_cairo_get_clip_rectangle(cr, &clip))
        gtk_widget_get_allocation(print_widget, &clip);

    GdkPixbuf *buf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE,
                                    8, clip.width, clip.height);
    int d_bpl = gdk_pixbuf_get_rowstride(buf);
    guchar *d1 = gdk_pixbuf_get_pixels(buf);
    int length = printout_bottom - printout_top;
    if (length < 0)
        length += PRINT_LINES;

    for (int v = clip.y; v < clip.y + clip.height; v++) {
        int v2 = printout_top + v;
        if (v2 >= PRINT_LINES)
            v2 -= PRINT_LINES;
        int v3 = v2 * 36;
        guchar *dst = d1;
        for (int h = clip.x; h < clip.x + clip.width; h++) {
            unsigned char c;
            if (v >= length)
                c = dark ? 64 : 192;
            else if (h < 36 || h >= 322)
                c = dark ? 18 : 255;
            else if ((print_bitmap[v3 + ((h - 36) >> 3)] & (1 << ((h - 36) & 7))) == 0)
                c = dark ? 18 : 255;
            else
                c = dark ? 219 : 0;
            *dst++ = c;
            *dst++ = c;
            *dst++ = c;
        }
        d1 += d_bpl;
    }

    gdk_cairo_set_source_pixbuf(cr, buf, clip.x, clip.y);
    cairo_paint(cr);
    g_object_unref(G_OBJECT(buf));
}

static gboolean reminder(gpointer cd) {
    bool dummy1;
    int dummy2;
    bool keep_running = core_keydown(0, &dummy1, &dummy2);
    if (quit_flag)
        quit();
    if (keep_running)
        return TRUE;
    else {
        reminder_id = 0;
        return FALSE;
    }
}

/* Callbacks used by shell_print() and shell_spool_txt() / shell_spool_gif() */

static void txt_writer(const char *text, int length) {
    int n;
    if (print_txt == NULL)
        return;
    n = fwrite(text, 1, length, print_txt);
    if (n != length) {
        char buf[1000];
        state.printerToTxtFile = 0;
        fclose(print_txt);
        print_txt = NULL;
        snprintf(buf, 1000, "Error while writing to \"%s\".\nPrinting to text file disabled", state.printerTxtFileName);
        show_message("Message", buf);
    }
}

static void txt_newliner() {
    if (print_txt == NULL)
        return;
    fputc('\r', print_txt);
    fputc('\n', print_txt);
    fflush(print_txt);
}

static void gif_seeker(int4 pos) {
    if (print_gif == NULL)
        return;
    if (fseek(print_gif, pos, SEEK_SET) == -1) {
        char buf[1000];
        state.printerToGifFile = 0;
        fclose(print_gif);
        print_gif = NULL;
        snprintf(buf, 1000, "Error while seeking \"%s\".\nPrinting to GIF file disabled", print_gif_name);
        show_message("Message", buf);
    }
}

static void gif_writer(const char *text, int length) {
    int n;
    if (print_gif == NULL)
        return;
    n = fwrite(text, 1, length, print_gif);
    if (n != length) {
        char buf[1000];
        state.printerToGifFile = 0;
        fclose(print_gif);
        print_gif = NULL;
        snprintf(buf, 1000, "Error while writing to \"%s\".\nPrinting to GIF file disabled", print_gif_name);
        show_message("Message", buf);
    }
}

void shell_blitter(const char *bits, int bytesperline, int x, int y,
                                     int width, int height) {
    /* In case we happen to get called at a moment when shell and core
     * are out of sync as to what size the display is...
     */
    if (x >= disp_w || y >= disp_h)
        return;
    if (x + width > disp_w)
        width = disp_w - x;
    if (y + height > disp_h)
        height = disp_h - y;

    if (state.old_repaint) {
        GdkWindow *win = gtk_widget_get_window(calc_widget);

        skin_display_invalidater(win, bits, bytesperline, x, y, width, height);
        if (skey >= -7 && skey <= -2)
            skin_invalidate_key(win, skey);
    } else {
        skin_display_invalidater(NULL, bits, bytesperline, x, y, width, height);
    }
}

void shell_beeper(int tone) {
#ifdef AUDIO_ALSA
    const char *display_name = gdk_display_get_name(gdk_display_get_default());
    if (display_name == NULL || display_name[0] == ':') {
        const int tone_freqs[] = { 165, 220, 247, 277, 294, 330, 370, 415, 440, 554, 1865 };
        int frequency = tone_freqs[tone];
        int duration = tone == 10 ? 125 : 250;
        if (!alsa_beeper(frequency, duration))
            gdk_display_beep(gdk_display_get_default());
    } else
        gdk_display_beep(gdk_display_get_default());
#else
    gdk_display_beep(gdk_display_get_default());
#endif
}

static gboolean ann_print_timeout(gpointer cd) {
    GdkWindow *win = gtk_widget_get_window(calc_widget);

    ann_print_timeout_id = 0;
    ann_print = 0;
    skin_invalidate_annunciator(win, 3);

    return FALSE;
}

const char *shell_platform() {
    return VERSION " " VERSION_PLATFORM;
}

void shell_annunciators(int updn, int shf, int prt, int run, int g, int rad) {
    GdkWindow *win = gtk_widget_get_window(calc_widget);

    if (updn != -1 && ann_updown != updn) {
        ann_updown = updn;
        skin_invalidate_annunciator(win, 1);
    }
    if (shf != -1 && ann_shift != shf) {
        ann_shift = shf;
        skin_invalidate_annunciator(win, 2);
    }
    if (prt != -1) {
        if (ann_print_timeout_id != 0) {
            g_source_remove(ann_print_timeout_id);
            ann_print_timeout_id = 0;
        }
        if (ann_print != prt)
            if (prt) {
                ann_print = 1;
                skin_invalidate_annunciator(win, 3);
            } else {
                ann_print_timeout_id = g_timeout_add(1000, ann_print_timeout, NULL);
            }
    }
    if (run != -1 && ann_run != run) {
        ann_run = run;
        skin_invalidate_annunciator(win, 4);
    }
    if (g != -1 && ann_g != g) {
        ann_g = g;
        skin_invalidate_annunciator(win, 6);
    }
    if (rad != -1 && ann_rad != rad) {
        ann_rad = rad;
        skin_invalidate_annunciator(win, 7);
    }
}

bool shell_wants_cpu() {
    static uint4 lastCount = 0;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint4 count = tv.tv_sec * 1000L + tv.tv_usec / 1000;
    if (count - lastCount < 10)
        return false;
    lastCount = count;
    return g_main_context_pending(NULL);
}

void shell_delay(int duration) {
    gdk_display_flush(gdk_display_get_default());
    g_usleep(duration * 1000);
}

void shell_request_timeout3(int delay) {
    if (timeout3_id != 0)
        g_source_remove(timeout3_id);
    timeout3_id = g_timeout_add(delay, timeout3, NULL);
}

void shell_request_display_size(int rows, int cols) {
    update_skin(rows, cols);
}

uint8 shell_get_mem() {
    FILE *meminfo = fopen("/proc/meminfo", "r");
    char line[1024];
    uint8 bytes = 0;
    if (meminfo == NULL)
        return 0;
    while (fgets(line, 1024, meminfo) != NULL) {
        if (strncmp(line, "MemFree:", 8) == 0) {
            uint8 kbytes;
            if (sscanf(line + 8, "%llu", &kbytes) == 1)
                bytes = 1024 * kbytes;
            break;
        }
    }
    fclose(meminfo);
    return bytes;
}

bool shell_low_battery() {

    int lowbat = 0;
    FILE *apm = fopen("/proc/apm", "r");
    if (apm != NULL) {
        /* /proc/apm partial legend:
         *
         * 1.16 1.2 0x03 0x01 0x03 0x09 9% -1 ?
         *               ^^^^ ^^^^
         *                 |    +-- Battery status (0 = full, 1 = low,
         *                 |                        2 = critical, 3 = charging)
         *                 +------- AC status (0 = offline, 1 = online)
         */
        char line[1024];
        int ac_stat, bat_stat;
        if (fgets(line, 1024, apm) == NULL)
            goto done1;
        if (sscanf(line, "%*s %*s %*s %x %x", &ac_stat, &bat_stat) == 2)
            lowbat = ac_stat != 1 && (bat_stat == 1 || bat_stat == 2);
        done1:
        fclose(apm);
    } else {
        /* Battery considered low if
         *
         *   /sys/class/power_supply/BATn/status == "Discharging"
         *   and
         *   /sys/class/power_supply/BATn/capacity <= 10
         *
         * Assuming status will always be "Discharging" when the system is
         * actually running on battery (it could also be "Full", but then it is
         * definitely not low!), and that capacity is a number between 0 and
         * 100. The choice of 10% or less as being "low" is completely
         * arbitrary.
         * Checking BATn where n = 0, 1, or 2. Some docs suggest BAT0 should
         * exist, others suggest 1 should exist; I'm playing safe and trying
         * both, and throwing in BAT2 just for the fun of it.
         */
        char status_filename[50];
        char capacity_filename[50];
        char line[50];
        for (int n = 0; n <= 2; n++) {
            snprintf(status_filename, 50, "/sys/class/power_supply/BAT%d/status", n);
            FILE *status_file = fopen(status_filename, "r");
            if (status_file == NULL)
                continue;
            snprintf(capacity_filename, 50, "/sys/class/power_supply/BAT%d/capacity", n);
            FILE *capacity_file = fopen(capacity_filename, "r");
            if (capacity_file == NULL) {
                fclose(status_file);
                continue;
            }
            bool discharging = fgets(line, 50, status_file) != NULL && strncasecmp(line, "discharging", 11) == 0;
            int capacity;
            if (fscanf(capacity_file, "%d", &capacity) != 1)
                capacity = 100;
            fclose(status_file);
            fclose(capacity_file);
            lowbat = discharging && capacity <= 10;
            break;
        }
    }
    if (lowbat != ann_battery) {
        ann_battery = lowbat;
        if (allow_paint) {
            GdkWindow *win = gtk_widget_get_window(calc_widget);
            skin_invalidate_annunciator(win, 5);
        }
    }
    return lowbat != 0;
}

void shell_powerdown() {
    /* We defer the actual shutdown so the emulator core can
     * return from core_keyup() or core_keydown() and isn't
     * asked to save its state while still in the middle of
     * executing the OFF instruction...
     */
    quit_flag = true;
}

void shell_message(const char *message) {
    show_message("Core", message);
}

int8 shell_random_seed() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000LL + tv.tv_usec / 1000;
}

uint4 shell_milliseconds() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint4) (tv.tv_sec * 1000L + tv.tv_usec / 1000);
}

const char *shell_number_format() {
    return cached_number_format;
}

int shell_date_format() {
    struct tm t;
    t.tm_sec = 0;
    t.tm_min = 0;
    t.tm_hour = 0;
    t.tm_mday = 22; // 22
    t.tm_mon = 10; // 11
    t.tm_year = 1433; // 3333
    t.tm_wday = 0;
    t.tm_yday = 0;
    t.tm_isdst = 0;

    char buf[32];
    strftime(buf, 32, "%x", &t);

    int y = strstr(buf, "3") - buf;
    int m = strstr(buf, "1") - buf;
    int d = strstr(buf, "2") - buf;

    if (d < m && m < y)
        return 1;
    else if (y < m && m < d)
        return 2;
    else
        return 0;
}

bool shell_clk24() {
    struct tm t;
    t.tm_sec = 0;
    t.tm_min = 0;
    t.tm_hour = 0;
    t.tm_mday = 22; // 22
    t.tm_mon = 10; // 11
    t.tm_year = 1433; // 3333
    t.tm_wday = 0;
    t.tm_yday = 0;
    t.tm_isdst = 0;

    char buf[32];
    strftime(buf, 32, "%X", &t);

    return strstr(buf, "A") == NULL;
}

struct print_growth_info {
    int y, height;
    print_growth_info(int yy, int hheight) : y(yy), height(hheight) {}
};

static gboolean print_widget_grew(GtkWidget *w, GdkEventConfigure *event,
                                                                gpointer cd) {
    print_growth_info *info = (print_growth_info *) cd;
    scroll_printout_to_bottom();
    GdkRectangle clip;
    clip.x = 0;
    clip.y = info->y;
    clip.width = 358;
    clip.height = info->height;
    GdkWindow *win = gtk_widget_get_window(print_widget);
    gdk_window_invalidate_rect(win, &clip, FALSE);
    g_signal_handlers_disconnect_by_func(G_OBJECT(w), (gpointer) print_widget_grew, cd);
    delete info;
    return FALSE;
}

void shell_print(const char *text, int length,
                 const char *bits, int bytesperline,
                 int x, int y, int width, int height) {
    int xx, yy;
    int oldlength, newlength;

    for (yy = 0; yy < height; yy++) {
        int4 Y = (printout_bottom + 2 * yy) % PRINT_LINES;
        for (xx = 0; xx < 143; xx++) {
            int bit, px, py;
            if (xx < width) {
                char c = bits[(y + yy) * bytesperline + ((x + xx) >> 3)];
                bit = (c & (1 << ((x + xx) & 7))) != 0;
            } else
                bit = 0;
            for (px = xx * 2; px < (xx + 1) * 2; px++)
                for (py = Y; py < Y + 2; py++)
                    if (bit)
                        print_bitmap[py * PRINT_BYTESPERLINE + (px >> 3)]
                            |= 1 << (px & 7);
                    else
                        print_bitmap[py * PRINT_BYTESPERLINE + (px >> 3)]
                            &= ~(1 << (px & 7));
        }
    }

    oldlength = printout_bottom - printout_top;
    if (oldlength < 0)
        oldlength += PRINT_LINES;
    printout_bottom = (printout_bottom + 2 * height) % PRINT_LINES;
    newlength = oldlength + 2 * height;

    if (newlength >= PRINT_LINES) {
        printout_top = (printout_bottom + 2) % PRINT_LINES;
        newlength = PRINT_LINES - 2;
        if (newlength != oldlength)
            gtk_widget_set_size_request(print_widget, 358, newlength);
        scroll_printout_to_bottom();
        gtk_widget_queue_draw(print_widget);
    } else {
        gtk_widget_set_size_request(print_widget, 358, newlength);
        // The resize request does not take effect immediately;
        // if I call scroll_printout_to_bottom() now, the scrolling will take
        // place *before* the resizing, leaving the scroll bar in the wrong
        // position.
        // I work around this by using a callback to finish the job.
        g_signal_connect(G_OBJECT(print_widget), "configure-event",
                         G_CALLBACK(print_widget_grew),
                         (gpointer) new print_growth_info(oldlength, 2 * height));
    }

    if (state.printerToTxtFile) {
        int err;
        char buf[1000];

        if (print_txt == NULL) {
            print_txt = fopen(state.printerTxtFileName, "a");
            if (print_txt == NULL) {
                err = errno;
                state.printerToTxtFile = 0;
                snprintf(buf, 1000, "Can't open \"%s\" for output:\n%s (%d)\nPrinting to text file disabled.", state.printerTxtFileName, strerror(err), err);
                show_message("Message", buf);
                goto done_print_txt;
            }
        }

        if (text != NULL)
            shell_spool_txt(text, length, txt_writer, txt_newliner);
        else
            shell_spool_bitmap_to_txt(bits, bytesperline, x, y, width, height, txt_writer, txt_newliner);
        done_print_txt:;
    }

    if (state.printerToGifFile) {
        int err;
        char buf[1000];

        if (print_gif != NULL
                && gif_lines + height > state.printerGifMaxLength) {
            shell_finish_gif(gif_seeker, gif_writer);
            fclose(print_gif);
            print_gif = NULL;
        }

        if (print_gif == NULL) {
            while (1) {
                int len, p;

                gif_seq = (gif_seq + 1) % 10000;

                strcpy(print_gif_name, state.printerGifFileName);
                len = strlen(print_gif_name);

                /* Strip ".gif" extension, if present */
                if (len >= 4 &&
                        strcasecmp(print_gif_name + len - 4, ".gif") == 0) {
                    len -= 4;
                    print_gif_name[len] = 0;
                }

                /* Strip ".[0-9]+", if present */
                p = len;
                while (p > 0 && print_gif_name[p] >= '0'
                             && print_gif_name[p] <= '9')
                    p--;
                if (p < len && p >= 0 && print_gif_name[p] == '.')
                    print_gif_name[p] = 0;

                /* Make sure we have enough space for the ".nnnn.gif" */
                p = FILENAMELEN - 10;
                print_gif_name[p] = 0;
                p = strlen(print_gif_name);
                snprintf(print_gif_name + p, 6, ".%04d", gif_seq);
                strcat(print_gif_name, ".gif");

                if (!file_exists(print_gif_name))
                    break;
            }
            print_gif = fopen(print_gif_name, "w+");
            if (print_gif == NULL) {
                err = errno;
                state.printerToGifFile = 0;
                snprintf(buf, 1000, "Can't open \"%s\" for output:\n%s (%d)\nPrinting to GIF file disabled.", print_gif_name, strerror(err), err);
                show_message("Message", buf);
                goto done_print_gif;
            }
            if (!shell_start_gif(gif_writer, 143, state.printerGifMaxLength)) {
                state.printerToGifFile = 0;
                show_message("Message", "Not enough memory for the GIF encoder.\nPrinting to GIF file disabled.");
                goto done_print_gif;
            }
            gif_lines = 0;
        }

        shell_spool_gif(bits, bytesperline, x, y, width, height, gif_writer);
        gif_lines += height;

        if (print_gif != NULL && gif_lines + 9 > state.printerGifMaxLength) {
            shell_finish_gif(gif_seeker, gif_writer);
            fclose(print_gif);
            print_gif = NULL;
        }
        done_print_gif:;
    }

    if (text == NULL) {
        print_text[print_text_bottom] = 254;
        print_text_bottom = (print_text_bottom + 1) % PRINT_TEXT_SIZE;
        print_text[print_text_bottom] = height >> 8;
        print_text_bottom = (print_text_bottom + 1) % PRINT_TEXT_SIZE;
        print_text[print_text_bottom] = height;
        print_text_bottom = (print_text_bottom + 1) % PRINT_TEXT_SIZE;
    } else {
        print_text[print_text_bottom] = length;
        print_text_bottom = (print_text_bottom + 1) % PRINT_TEXT_SIZE;
    }
    if (text != NULL) {
        if (print_text_bottom + length < PRINT_TEXT_SIZE) {
            memcpy(print_text + print_text_bottom, text, length);
            print_text_bottom += length;
        } else {
            int part = PRINT_TEXT_SIZE - print_text_bottom;
            memcpy(print_text + print_text_bottom, text, part);
            memcpy(print_text, text + part, length - part);
            print_text_bottom = length - part;
        }
    }
    print_text_pixel_height += text == NULL ? height : 9;
    while (print_text_pixel_height > PRINT_LINES / 2 - 1) {
        unsigned char len = print_text[print_text_top];
        int tll;
        if (len == 255) {
            /* Old-style fixed-size PRLCD */
            tll = 16;
        } else if (len == 254) {
            /* New any-size PRLCD; height encoded in next 2 bytes */
            if (++print_text_top == PRINT_TEXT_SIZE)
                print_text_top = 0;
            tll = print_text[print_text_top] << 8;
            if (++print_text_top == PRINT_TEXT_SIZE)
                print_text_top = 0;
            tll |= print_text[print_text_top];
        } else {
            /* Text */
            tll = 9;
        }
        print_text_pixel_height -= tll;
        print_text_top += len >= 254 ? 1 : (print_text[print_text_top] + 1);
        if (print_text_top >= PRINT_TEXT_SIZE)
            print_text_top -= PRINT_TEXT_SIZE;
    }
}

static FILE *logfile = NULL;

void shell_log(const char *message) {
    if (logfile == NULL)
        logfile = fopen("plus42.log", "w");
    fprintf(logfile, "%s\n", message);
    fflush(logfile);
}

void shell_get_time_date(uint4 *time, uint4 *date, int *weekday) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm tms;
    localtime_r(&tv.tv_sec, &tms);
    if (time != NULL)
        *time = ((tms.tm_hour * 100 + tms.tm_min) * 100 + tms.tm_sec) * 100 + tv.tv_usec / 10000;
    if (date != NULL)
        *date = ((tms.tm_year + 1900) * 100 + tms.tm_mon + 1) * 100 + tms.tm_mday;
    if (weekday != NULL)
        *weekday = tms.tm_wday;
}
