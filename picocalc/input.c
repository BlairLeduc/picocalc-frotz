//
// input.c - PicoCalc interface, input functions
//

#include <string.h>

#include "lcd.h"
#include "keyboard.h"

#undef bool
#include "picocalc_frotz.h"

extern f_setup_t f_setup;
extern int cursor_row, cursor_col;
extern uint8_t text_style;

volatile bool user_interrupt = FALSE;

char *dirname(char *path)
{
	if (!path || !*path)
	{
		return strdup(".");
	}
	char *slash = strrchr(path, '/');
	if (!slash)
	{
		return strdup(".");
	}

	// Handle root "/"
	if (slash == path)
	{
		return strdup("/");
	}

	size_t len = slash - path;
	char *dir = malloc(len + 1);
	if (!dir)
	{
		return NULL;
	}

	strncpy(dir, path, len);
	dir[len] = '\0';
	return dir;
}

zchar os_read_key(int timeout, bool show_cursor)
{
	// timeout is in tenths of seconds, 0 means no timeout
	if (show_cursor)
	{
		lcd_draw_cursor();
		lcd_enable_cursor(TRUE);
	}

	absolute_time_t start_time = get_absolute_time();
	while (!keyboard_key_available())
	{
		if (timeout > 0)
		{
			absolute_time_t now = get_absolute_time();
			if (absolute_time_diff_us(start_time, now) >= (timeout * 100000))
			{
				break; // timeout!
			}
		}
		tight_loop_contents(); // yield to other tasks
		sleep_ms(100);		   // sleep for a short time to avoid busy-waiting
	}

	if (show_cursor)
	{
		lcd_erase_cursor();
		lcd_enable_cursor(FALSE);
	}

	if (!keyboard_key_available())
	{
		return ZC_TIME_OUT; // No key pressed
	}

	char key = keyboard_get_key();
	switch (key)
	{
	case 0x0D:
		return ZC_RETURN;
	case 0x08:
		return ZC_BACKSPACE;
	case KEY_ESC:
		return ZC_ESCAPE;
	case KEY_UP:
		return ZC_ARROW_UP;
	case KEY_DOWN:
		return ZC_ARROW_DOWN;
	case KEY_LEFT:
		return ZC_ARROW_LEFT;
	case KEY_RIGHT:
		return ZC_ARROW_RIGHT;
	case KEY_F1:
		return ZC_FKEY_F1;
	case KEY_F2:
		return ZC_FKEY_F2;
	case KEY_F3:
		return ZC_FKEY_F3;
	case KEY_F4:
		return ZC_FKEY_F4;
	case KEY_F5:
		return ZC_FKEY_F5;
	case KEY_F6:
		return ZC_FKEY_F6;
	case KEY_F7:
		return ZC_FKEY_F7;
	case KEY_F8:
		return ZC_FKEY_F8;
	case KEY_F9:
		return ZC_FKEY_F9;
	case KEY_F10:
		return ZC_FKEY_F10;
	default:
		return (zchar)key; // Return the key as a zchar
	}
}

zchar os_read_line(int max, zchar *buf, int timeout, int width, int continued)
{
	zchar key;
	int row = cursor_row + 1;
	int col = cursor_col + 1;

	lcd_draw_cursor();
	lcd_enable_cursor(TRUE);

	buf[max - 1] = 0; // Ensure the buffer is null-terminated

	uint8_t index = 0;
	index = strlen(buf);
	if (index >= max)
	{
		index = max - 1; // Ensure we don't overflow the buffer
	}
	if (index > width)
	{
		index = width - 1; // Limit to the width of the input line
	}
	
	col -= index;
	lcd_move_cursor(cursor_col, cursor_row);

	absolute_time_t start_time = get_absolute_time();
	while (1)
	{
		absolute_time_t now = get_absolute_time();
		int remaining_timeout = timeout > 0 ? timeout - absolute_time_diff_us(start_time, now) / 100000 : 0;
		if (remaining_timeout < 0)
		{
			key = ZC_TIME_OUT;
			break;
		}
		key = os_read_key(remaining_timeout, FALSE);
		if (key == ZC_TIME_OUT)
		{
			break;
		}
		switch (key)
		{
		case ZC_BACKSPACE:
			if (index > 0)
			{
				buf[--index] = 0;
				lcd_erase_cursor();
				os_set_cursor(row, col + index);
				os_display_char(' '); // Erase the character
				os_set_cursor(row, col + index);
				lcd_draw_cursor(); // Redraw the cursor
			}
			break;
		default:
			if (is_terminator(key))
			{
				lcd_erase_cursor();
				lcd_enable_cursor(FALSE);

				buf[index] = 0; // Null-terminate the string
				return key;
			}
			if (index < max - 1 && index < width - 1)
			{
				buf[index++] = key;
				lcd_erase_cursor();
				os_display_char(key);
				lcd_draw_cursor();
			}
			else
			{
				os_beep(1); // Beep if buffer is full or width exceeded
			}
			break;
		}
	}

	buf[index] = 0; // Null-terminate the string
	return key;
}

char *os_read_file_name(const char *default_name, int flag)
{
	FILE *fp;
	int i;
	char *tempname;
	zchar answer[4];
	char path_separator[2];
	char file_name[MAX_FILE_NAME + 1];
	char *ext;

	path_separator[0] = PATH_SEPARATOR;
	path_separator[1] = 0;

	// If we're restoring a game before the interpreter starts,
	// our filename is already provided.  Just go ahead silently.
	if (f_setup.restore_mode || flag == FILE_NO_PROMPT)
	{
		file_name[0] = 0;
	}
	else
	{
		print_string("Enter a file name.\nDefault is \"");

		// After successfully reading or writing a file, the default
		// name gets saved and used again the next time something is
		// to be read or written.  In restricted mode, we don't want
		// to show any path prepended to the actual file name.  Here,
		// we strip out that path and display only the filename.
		if (f_setup.restricted_path != NULL)
		{
			tempname = basename((char *)default_name);
			print_string(tempname);
		}
		else
		{
			print_string(default_name);
		}
		print_string("\":\n");

		read_string(MAX_FILE_NAME - EXT_LENGTH, (zchar *)file_name);
	}

	// Return failure if path provided when in restricted mode.
	// I think this is better than accepting a path/filename
	// and stripping off the path.
	if (f_setup.restricted_path)
	{
		tempname = dirname(file_name);
		if (strlen(tempname) > 1)
			return NULL;
	}

	// Use the default name if nothing was typed
	if (file_name[0] == 0)
	{
		// If FILE_NO_PROMPT, restrict to currect directory.
		// If FILE_NO_PROMPT and using restricted path, then
		//   nothing more needs to be done to restrict the
		//   file access there.
		if (flag == FILE_NO_PROMPT && f_setup.restricted_path == NULL)
		{
			tempname = basename((char *)default_name);
			strncpy(file_name, tempname, MAX_FILE_NAME);
		}
		else
			strncpy(file_name, default_name, MAX_FILE_NAME);
	}

	// If we're restricted to one directory, strip any leading path left
	// over from a previous call to os_read_file_name(), then prepend
	// the prescribed path to the filename.  Hostile leading paths won't
	// get this far.  Instead we return failure a few lines above here if
	// someone tries it.
	if (f_setup.restricted_path != NULL)
	{
		for (i = strlen(file_name); i > 0; i--)
		{
			if (file_name[i] == PATH_SEPARATOR)
			{
				i++;
				break;
			}
		}
		tempname = strdup(file_name + i);
		strncpy(file_name, f_setup.restricted_path, MAX_FILE_NAME);

		// Make sure the final character is the path separator.
		if (file_name[strlen(file_name) - 1] != PATH_SEPARATOR)
		{
			strncat(file_name, path_separator, MAX_FILE_NAME - strlen(file_name) - 2);
		}
		strncat(file_name, tempname, strlen(file_name) - strlen(tempname) - 1);
	}

	ext = strrchr(file_name, '.');
	if (flag == FILE_NO_PROMPT)
	{
		if (strncmp(ext, EXT_AUX, 4))
		{
			os_warn("Blocked unprompted access of %s. Should only be %s files.", file_name, EXT_AUX);
			return NULL;
		}
	}

	// Add appropriate filename extensions if not already there.
	if (flag == FILE_SAVE || flag == FILE_RESTORE)
	{
		if (ext == NULL || strncmp(ext, EXT_SAVE, EXT_LENGTH) != 0)
		{
			strncat(file_name, EXT_SAVE, EXT_LENGTH);
		}
	}
	else if (flag == FILE_SCRIPT)
	{
		if (ext == NULL || strncmp(ext, EXT_SCRIPT, EXT_LENGTH) != 0)
		{
			strncat(file_name, EXT_SCRIPT, EXT_LENGTH);
		}
	}
	else if (flag == FILE_SAVE_AUX || flag == FILE_LOAD_AUX)
	{
		if (ext == NULL || strncmp(ext, EXT_AUX, EXT_LENGTH) != 0)
		{
			strncat(file_name, EXT_AUX, EXT_LENGTH);
		}
	}
	else if (flag == FILE_RECORD || flag == FILE_PLAYBACK)
	{
		if (ext == NULL || strncmp(ext, EXT_COMMAND, EXT_LENGTH) != 0)
		{
			strncat(file_name, EXT_COMMAND, EXT_LENGTH);
		}
	}

	// Warn if overwriting a file.
	if ((flag == FILE_SAVE || flag == FILE_SAVE_AUX ||
		 flag == FILE_RECORD || flag == FILE_SCRIPT) &&
		((fp = fopen(file_name, "rb")) != NULL))
	{
		fclose(fp);
		print_string("Overwrite existing file? ");
		read_string(4, answer);
		if (tolower(answer[0] != 'y'))
			return NULL;
	}

	return strdup(file_name);
}

void os_more_prompt(void)
{
	uint8_t saved_style = text_style;
	int saved_row = cursor_row + 1;
	int saved_col = cursor_col + 1;

	os_set_text_style(0);
	os_display_string("[MORE]");
	os_read_key(0, TRUE);

	os_set_cursor(saved_row, saved_col);
	os_display_string("      ");
	os_set_cursor(saved_row, saved_col);
	os_set_text_style(saved_style);
}

zword os_read_mouse(void)
{
	// No mouse on the PicoCalc, nothing to do.
	return 0;
}

void os_tick(void)
{
	// Nothing to do
}