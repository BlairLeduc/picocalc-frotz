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

#include "fat32.h"

#define MAX_SCREEN_WIDTH (64) // PicoCalc screen width in characters (maximum)
#define SCREEN_HEIGHT (32)

#define RGB(r,g,b)          ((uint16_t)(((r) >> 3) << 11 | ((g) >> 2) << 5 | ((b) >> 3)))
#define WHITE_PHOSPHOR      RGB(216, 240, 255)  // white phosphor
#define GREEN_PHOSPHOR      RGB(51, 255, 51)    // green phosphor
#define AMBER_PHOSPHOR      RGB(255, 183, 0)    // amber phosphor
#define FOREGROUND_COLOUR   RGB(255, 255, 255)  // default foreground colour
#define DEFAULT_PHOSPHOR    WHITE_PHOSPHOR

#define HISTORY_SIZE 20
#define HISTORY_LINE_LENGTH 40

#define SETTINGS_SET 0x01
#define SETTINGS_COLUMNS_MASK   0x02
#define SETTINGS_COLUMNS_64     0x02
#define SETTINGS_PHOSPHOR_MASK  0x0C
#define SETTINGS_PHOSPHOR_GREEN 0x04
#define SETTINGS_PHOSPHOR_AMBER 0x08

#define CONFIG_MAX_FILENAME_LEN 32
#define CONFIG_MAX_STORIES 128
#define CONFIG_MAX_STORIES_PER_SCREEN 20

typedef uint32_t settings_t;

typedef struct
{
    settings_t settings;
    char story_filename[CONFIG_MAX_FILENAME_LEN];
} story_t;

typedef struct
{
    story_t stories[CONFIG_MAX_STORIES];
    size_t story_count;
    settings_t defaults;
    char default_save_path[FAT32_MAX_PATH_LEN];
} config_t;


// No os_get_cursor() function in Frotz; we need access to the cursor position
// for input handling.
extern int cursor_row, cursor_col;
extern uint8_t columns; // Number of columns in the display

// Function prototypes
void update_lcd_display(int top, int left, int bottom, int right);
