//
// output.c - PicoCalc interface, output functions
//

#include "audio.h"
#include "lcd.h"
#include "font.h"

#undef bool
#include "picocalc_frotz.h"

static char latin1_to_ascii[] =
    "    !   c   \x1E   >o< Y   |   S   ''  C   a   <<  not -   R   _   "
    "\x07   \x08   ^2  ^3  '   my  P   .   ,   ^1  \x07   >>  1/4 1/2 3/4 ?   "
    "A   A   A   A   Ae  A   AE  C   E   E   E   E   I   I   I   I   "
    "Th  N   O   O   O   O   Oe  *   O   U   U   U   Ue  Y   Th  ss  "
    "a   a   a   a   ae  a   ae  c   e   e   e   e   i   i   i   i   "
    "th  n   o   o   o   o   oe  :   o   u   u   u   ue  y   th  y   ";

#define CELL_CH(C) (((C) & 0xFF) << 16)
#define CELL_STYLE(S) (((S) & 0xFF))

static uint32_t screen[MAX_SCREEN_WIDTH * SCREEN_HEIGHT];

// row and col coordinates for the next character to display
// This position is always in the valid range of the screen
int cursor_row = 0, cursor_col = 0;
static uint8_t text_style = 0;
static uint8_t foreground = 1, background = 0;

static void addch(zchar c)
{
    if (c == ZC_RETURN || c == '\n' || c == '\r')
    {
        cursor_col = 0; // Reset column on return
        cursor_row++;
        if (cursor_row >= SCREEN_HEIGHT)
        {
            cursor_row--; // Stay at the last row
        }
        lcd_move_cursor(cursor_col, cursor_row);
        return;
    }

    if (c > 0x7F)
    {
        c = 0x02; // Replace non-ASCII characters with error character
    }

    // Update the LCD
    lcd_putc(cursor_col, cursor_row, c);

    // Update the screen buffer
    screen[cursor_row * columns + cursor_col] = CELL_CH(c) | CELL_STYLE(text_style);
    cursor_col++;
    if (cursor_col >= columns)
    {
        cursor_col = 0;
        cursor_row++;
        if (cursor_row >= SCREEN_HEIGHT)
        {
            cursor_col = columns - 1;  // Stay at the last column
            cursor_row = SCREEN_HEIGHT - 1; // Stay at the last row
        }
    }
    lcd_move_cursor(cursor_col, cursor_row);
}


void update_lcd_display(int top, int left, int bottom, int right)
{
    // Update the LCD display
    for (int r = top; r <= bottom; r++)
    {
        for (int c = left; c <= right; c++)
        {
            uint8_t style = screen[r * columns + c] & 0xFF;
            lcd_set_reverse(style & REVERSE_STYLE);
            lcd_set_bold(style & BOLDFACE_STYLE);
            lcd_set_underscore(style & EMPHASIS_STYLE);
            lcd_putc(c, r, screen[r * columns + c] >> 16);
        }
    }
}

void os_init_sound(void)
{
    audio_init();
}

void os_beep(int number)
{
    switch (number)
    {
    case 1: // High beep
        audio_play_sound_blocking(PITCH_A5, PITCH_A5, NOTE_QUARTER);
        break;
    case 2: // Low beep
        audio_play_sound_blocking(PITCH_A3, PITCH_A3, NOTE_QUARTER);
        break;
    default: // Default beep
        audio_play_sound_blocking(PITCH_A4, PITCH_A4, NOTE_QUARTER);
        break;
    }
}

void os_prepare_sample(int UNUSED(a)) {}
void os_finish_with_sample(int UNUSED(a)) {}
void os_start_sample(int UNUSED(a), int UNUSED(b), int UNUSED(c), zword UNUSED(d)) {}
void os_stop_sample(int UNUSED(a)) {}

void os_display_char(zchar c)
{
    // Display a character of the current font using the current colours and
    // text style. The cursor moves to the next position. Printable codes are
    // all ASCII values from 32 to 126, ISO Latin-1 characters from 160 to
    // 255, ZC_GAP (gap between two sentences) and ZC_INDENT (paragraph
    // indentation). The screen should not be scrolled after printing to the
    // bottom right corner.
    if (c >= ZC_LATIN1_MIN)
    {
        char *ptr = latin1_to_ascii + 4 * (c - ZC_LATIN1_MIN);
        do
        {
            addch(*ptr++);
        } while (*ptr != ' ');
        return;
    }

    if (c >= ZC_ASCII_MIN && c <= ZC_ASCII_MAX)
    {
        addch(c);
        return;
    }

    if (c == ZC_RETURN || c == '\n' || c == '\r')
    {
        addch(ZC_RETURN);
        return;
    }

    if (c == ZC_INDENT)
    {
        addch(' ');
        addch(' ');
        addch(' ');
        return;
    }

    if (c == ZC_GAP)
    {
        addch(' ');
        addch(' ');
        return;
    }

    if (c == ZC_RETURN)
    {
        addch(c);
        return;
    }
}

void os_display_string(const zchar *s)
{
    zchar c;

    while ((c = *s++) != 0)
    {
        if (c == ZC_NEW_FONT || c == ZC_NEW_STYLE)
        {
            int arg = (unsigned char)*s++;
            if (c == ZC_NEW_FONT)
            {
                os_set_font(arg);
            }
            if (c == ZC_NEW_STYLE)
            {
                os_set_text_style(arg);
            }
        }
        else
        {
            os_display_char(c);
        }
    }
}

void os_erase_area(int top, int left, int bottom, int right, int UNUSED(win))
{
    top--;
    left--;
    bottom--;
    right--; // Convert to 0-based index

    uint8_t glyph_width = lcd_get_glyph_width();
    lcd_solid_rectangle(
        background,
        left * glyph_width,
        top * GLYPH_HEIGHT,
        (right - left + 1) * glyph_width,
        (bottom - top + 1) * GLYPH_HEIGHT);

    for (int r = top; r <= bottom; r++)
    {
        memset(
            &screen[r * columns + left],
            0x00200000,
            (right - left + 1) * sizeof(uint32_t));
    }
}

void os_scroll_area(int top, int left, int bottom, int right, int units)
{
    top--;
    left--;
    bottom--;
    right--; // Convert to 0-based index

    if (units > 0)
    {
        // Scroll up (move content up, clear bottom)
        for (int r = top; r <= bottom - units; r++)
        {
            memcpy(&screen[r * columns + left],
                   &screen[(r + units) * columns + left],
                   (right - left + 1) * sizeof(uint32_t));
        }
        // Clear the bottom area
        for (int r = bottom - units + 1; r <= bottom; r++)
        {
            memset(&screen[r * columns + left], 0x20000000, (right - left + 1) * sizeof(uint32_t));
        }
    }
    else if (units < 0)
    {
        // Scroll down (move content down, clear top)
        units = -units;
        for (int r = bottom; r >= top + units; r--)
        {
            memcpy(&screen[r * columns + left],
                   &screen[(r - units) * columns + left],
                   (right - left + 1) * sizeof(uint32_t));
        }
        // Clear the top area
        for (int r = top; r < top + units; r++)
        {
            memset(&screen[r * columns + left], 0x20000000, (right - left + 1) * sizeof(uint32_t));
        }
    }

    update_lcd_display(top, left, bottom, right);
}

int os_font_data(int font, int *height, int *width)
{
    if (font == TEXT_FONT)
    {
        *height = 1;
        *width = 1;
        return 1;
    }
    return 0;
}

void os_set_colour(int newfg, int newbg)
{
    // Ignore colour changes
}

void os_reset_screen(void)
{
    // Reset the screen before the program stops.
    // On the PicoCalc, nothing to do.
}

void os_set_font(int UNUSED(x))
{
    // We only have one font, nothing to do.
}

int os_check_unicode(int UNUSED(font), zchar UNUSED(c))
{
    // Return with bit 0 set if the Unicode character can be
    // displayed, and bit 1 if it can be input.
    return 0;
}

int os_char_width(zchar z)
{
    if (z >= ZC_LATIN1_MIN)
    {
        char *p = latin1_to_ascii + 4 * (z - ZC_LATIN1_MIN);
        return strchr(p, ' ') - p;
    }
    if (z == ZC_INDENT)
    {
        return 3;
    }
    if (z == ZC_GAP)
    {
        return 2;
    }
    return 1;
}

int os_string_width(const zchar *s)
{
    int width = 0;
    zchar c;

    while ((c = *s++) != 0)
    {
        if (c == ZC_NEW_STYLE || c == ZC_NEW_FONT)
        {
            s++;
            /* No effect */
        }
        else
            width += os_char_width(c);
    }
    return width;
}

void os_set_cursor(int r, int c)
{
    cursor_row = r - 1;
    cursor_col = c - 1;
    if (cursor_row >= SCREEN_HEIGHT)
    {
        cursor_row = SCREEN_HEIGHT - 1; // Stay at the last row
    }
    if (cursor_col >= columns)
    {
        cursor_col = columns - 1; // Stay at the last column
    }
    lcd_move_cursor(cursor_col, cursor_row);
}

bool os_repaint_window(int UNUSED(win), int UNUSED(ypos_old),
                       int UNUSED(ypos_new), int UNUSED(xpos),
                       int UNUSED(ysize), int UNUSED(xsize))
{
    return FALSE; // No windows in PicoCalc
}

int os_get_text_style(void)
{
    // Return the current text style.
    return text_style;
}

void os_set_text_style(int x)
{
    // Update the current style for the next character to be displayed.
    text_style = x;
    lcd_set_reverse(text_style & REVERSE_STYLE);
    lcd_set_bold(text_style & BOLDFACE_STYLE);
    lcd_set_underscore(text_style & EMPHASIS_STYLE);
}

int os_from_true_colour(zword colour)
{
    return 0; // No true colour support in this implementation
}

zword os_to_true_colour(int index)
{
    return 0; // No true colour support in this implementation
}
