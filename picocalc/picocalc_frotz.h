//
// picocalc_frotz.h
//
// Frotz os functions for a standard C library and PicoCalc.
//

#pragma once

#include "frotz.h"

#ifndef NO_BASENAME
#include <libgen.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include <sys/param.h>

#define SCREEN_WIDTH 40
#define SCREEN_HEIGHT 32
#define RGB(r,g,b)      ((uint16_t)(((r) >> 3) << 11 | ((g) >> 2) << 5 | ((b) >> 3)))
#define WHITE_PHOSPHOR  RGB(216, 240, 255)  // white phosphor
#define GREEN_PHOSPHOR  RGB(51, 255, 102)   // green phosphor
#define AMBER_PHOSPHOR  RGB(255, 255, 51)   // amber phosphor
#define BRIGHT          RGB(255, 255, 255)  // white

/* from ../common/setup.h */
extern f_setup_t f_setup;

/* From input.c.  */
bool is_terminator (zchar);

/* pc-input.c */
bool pc_handle_setting(const char *setting, bool show_cursor, bool startup);
void pc_init_input(void);

/* pc-output.c */
void pc_init_output(void);
bool pc_output_handle_setting(const char *setting, bool show_cursor, bool startup);
void pc_show_screen(bool show_cursor);
void pc_show_prompt(bool show_cursor, char line_type);
void pc_dump_screen(void);
void pc_display_user_input(char *);
void pc_discard_old_input(int num_chars);
void pc_elide_more_prompt(void);
void pc_set_picture_cell(int row, int col, zchar c);

/* pc-pic.c */
bool pc_init_pictures(void);
