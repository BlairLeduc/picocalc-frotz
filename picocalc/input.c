//
// input.c - PicoCalc interface, input functions
//

#include <string.h>

#include "lcd.h"
#include "keyboard.h"

#undef bool
#include "picocalc_frotz.h"
#include <fat32.h>

extern uint16_t phosphor;

volatile bool user_interrupt = FALSE;

static char history_buffer[HISTORY_SIZE][HISTORY_LINE_LENGTH] = {0};
static uint8_t history_head = 0;
static uint8_t history_tail = 0;
static uint8_t history_index = 0;

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

void history_add(const char *line)
{
	strncpy(history_buffer[history_head], line, HISTORY_LINE_LENGTH - 1);
	history_buffer[history_head][HISTORY_LINE_LENGTH - 1] = '\0';
	history_head = (history_head + 1) % HISTORY_SIZE;
	if (history_head == history_tail)
	{
		history_tail = (history_tail + 1) % HISTORY_SIZE; // Overwrite oldest
	}
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
	case 0x09: // Tab key
		return 0x09;
	case KEY_DEL:
		return KEY_DEL; // DEL key
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
	case KEY_HOME:
		return KEY_HOME;
	case KEY_END:
		return KEY_END;
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
		if (key < 0x20 || key >= 0x7F)
		{
			return ZC_TIME_OUT; // Invalid key
		}
		return (zchar)key; // Return the key as a zchar
	}
}

zchar os_read_line(int max, zchar *buf, int timeout, int width, int continued)
{
	zchar key;
	int row = cursor_row + 1; // Start position of the input line (row)
	int col = cursor_col + 1; // Start position of the input line (column)
	static uint8_t index = 0;

	uint8_t length = strlen(buf);
	if (length == 0)
	{
		index = 0;					  // Reset index if buffer is empty
		history_index = history_head; // Reset index to the newest entry
	}

	col -= index; // Adjust start of input field

	lcd_draw_cursor();
	lcd_enable_cursor(TRUE);
	os_set_cursor(row, col + index);

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
		case '\t': // completion
			// Handle tab completion
			char result[24] = {0};

			char saved = buf[index];
			buf[index] = 0; // Temporarily null-terminate the string

			int completion_result = completion((const zchar *)buf, (zchar *)result);
			buf[index] = saved; // Restore the character

			int result_length = strlen(result);
			if (result_length + length > max - 1 || result_length + length > width - 1)
			{
				os_beep(1);
				continue; // Skip if completion is too long
			}
			if (completion_result == 0 || completion_result == 1)
			{
				if (index == length)
				{
					strcat(buf, result);
					index += result_length;
					length += result_length;
					os_display_string(result);
				}
				else
				{
					memmove(buf + index + result_length, buf + index, length - index + 1);
					memcpy(buf + index, result, result_length);
					index += result_length;
					length += result_length;
					os_display_string(buf + index - result_length); // Redisplay the rest of the line
					os_set_cursor(row, col + index);
				}
			}
			break;
		case ZC_BACKSPACE:
			if (index > 0)
			{
				index--;
				length--;
				lcd_erase_cursor();
				os_set_cursor(row, col + index);
				if (index == length)
				{
					buf[index] = 0;		  // Null-terminate the string
					os_display_char(' '); // Erase the character
				}
				else
				{
					memcpy(buf + index, buf + index + 1, length - index + 1);
					os_display_string(buf + index); // Redisplay the rest of the line
					os_display_char(' ');			// Clear the rest of the line
				}
				os_set_cursor(row, col + index);
				lcd_draw_cursor(); // Redraw the cursor
			}
			break;
		case KEY_DEL: // DEL key
			if (index < length)
			{
				lcd_erase_cursor();
				os_set_cursor(row, col + index);
				memcpy(buf + index, buf + index + 1, length - index + 1);
				length--;
				os_display_string(buf + index); // Redisplay the rest of the line
				os_display_char(' ');			// Clear the rest of the line
				os_set_cursor(row, col + index);
				lcd_draw_cursor(); // Redraw the cursor
			}
			break;
		case ZC_ESCAPE: // delete to beginning of line
			if (index > 0)
			{
				lcd_erase_cursor();
				os_set_cursor(row, col);
				os_display_string(buf + index); // Redisplay the rest of the line
				for (int i = index; i < width - 1; i++)
				{
					os_display_char(' '); // Clear the rest of the line
				}
				index = 0;
				length = 0;
				buf[0] = 0; // Reset buffer
				os_set_cursor(row, col);
				lcd_draw_cursor(); // Redraw the cursor
			}
			break;
		case KEY_HOME:
			if (index > 0)
			{
				index = 0;
				lcd_erase_cursor();
				os_set_cursor(row, col);
				lcd_draw_cursor(); // Redraw the cursor
			}
			break;
		case KEY_END:
			if (index < length)
			{
				index = length;
				lcd_erase_cursor();
				os_set_cursor(row, col + index);
				lcd_draw_cursor(); // Redraw the cursor
			}
			break;
		case ZC_ARROW_UP:
			if (history_head != history_tail)
			{ // history is not empty
				if (history_index == history_head)
				{
					// First time pressing up, go to most recent entry
					history_index = (history_head + HISTORY_SIZE - 1) % HISTORY_SIZE;
				}
				else if (history_index != history_tail)
				{
					// Move to the previous entry, wrapping if needed
					history_index = (history_index + HISTORY_SIZE - 1) % HISTORY_SIZE;
				}
				strncpy(buf, history_buffer[history_index], max - 1);
				buf[max - 1] = 0; // Ensure null-termination
				length = strlen(buf);
				index = length;
				lcd_erase_cursor();
				os_set_cursor(row, col);
				os_display_string(buf);
				for (int i = index; i < width - 1; i++)
				{
					os_display_char(' '); // Clear the rest of the line
				}
				os_set_cursor(row, col + index);
				lcd_draw_cursor();
			}
			break;
		case ZC_ARROW_DOWN:
			if (history_head != history_tail && history_index != history_head)
			{
				// Move to the next entry, wrapping if needed
				history_index = (history_index + 1) % HISTORY_SIZE;
				if (history_index == history_head)
				{
					// At the newest entry (blank line)
					buf[0] = 0;
				}
				else
				{
					strncpy(buf, history_buffer[history_index], max - 1);
					buf[max - 1] = 0; // Ensure null-termination
				}
				length = index = strlen(buf);
				lcd_erase_cursor();
				os_set_cursor(row, col);
				os_display_string(buf);
				for (int i = index; i < width - 1; i++)
				{
					os_display_char(' '); // Clear the rest of the line
				}
				os_set_cursor(row, col + index);
				lcd_draw_cursor();
			}
			break;
		case ZC_ARROW_LEFT:
			if (index > 0)
			{
				index--;
				lcd_erase_cursor();
				os_set_cursor(row, col + index);
				lcd_draw_cursor();
			}
			break;
		case ZC_ARROW_RIGHT:
			if (index < length)
			{
				index++;
				lcd_erase_cursor();
				os_set_cursor(row, col + index);
				lcd_draw_cursor();
			}
			break;
		case ZC_FKEY_F10:
			// Handle F10 key press
			if (phosphor == WHITE_PHOSPHOR)
			{
				phosphor = GREEN_PHOSPHOR; // Switch to normal colour
			}
			else if (phosphor == GREEN_PHOSPHOR)
			{
				phosphor = AMBER_PHOSPHOR; // Switch to white phosphor
			}
			else if (phosphor == AMBER_PHOSPHOR)
			{
				phosphor = WHITE_PHOSPHOR; // Switch back to white phosphor
			}
			lcd_set_foreground(phosphor);
			update_lcd_display(0, 0, z_header.screen_height - 1, z_header.screen_width - 1);
			break;
		default:
			if (is_terminator(key))
			{
				lcd_erase_cursor();
				lcd_enable_cursor(FALSE);

				history_add((const char *)buf); // Add to history
				return key;
			}
			if (key >= 0x20 && key < 0x7F)
			{
				if (length < max - 1 && length < width - 1)
				{
					lcd_erase_cursor();
					if (index == length)
					{
						buf[index++] = key;
						buf[index] = 0; // Null-terminate the string
						length++;
						os_display_char(key);
					}
					else
					{
						memmove(buf + index + 1, buf + index, length - index + 1);
						buf[index++] = key;
						length++;
						os_display_string(buf + index - 1); // Redisplay the rest of the line
						os_set_cursor(row, col + index);
					}
					lcd_draw_cursor();
				}
				else
				{
					os_beep(1); // Beep if buffer is full or width exceeded
				}
			}
			break;
		}
	}

	return key;
}

char *os_read_file_name(const char *default_name, int flag)
{
	FILE *fp;
	int i;
	char *tempname;
	zchar answer[4];
	char path_separator[2];
	char file_name[FAT32_MAX_PATH_LEN + 1];
	char buffer[64];
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
		do
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
			print_string("\" (? for list):\n");

			read_string(FAT32_MAX_PATH_LEN - EXT_LENGTH, (zchar *)file_name);

			// If the user entered '?', show the list of files
			if (file_name[0] == '?')
			{
				char *dirname = f_setup.restricted_path ? f_setup.restricted_path : "/";

				fat32_file_t dir;
				fat32_entry_t dir_entry;

				fat32_error_t result = fat32_open(&dir, dirname);
				if (result != FAT32_OK)
				{
					snprintf(buffer, sizeof(buffer), "Error opening directory %s: %s", dirname, fat32_error_string(result));
					print_string(buffer);
				}
				else
				{
					int count = 0;
					print_string("Saved games:\n");
					do
					{
						result = fat32_dir_read(&dir, &dir_entry);
						if (result != FAT32_OK)
						{
							snprintf(buffer, sizeof(buffer), "Error reading directory: %s", fat32_error_string(result));
							print_string(buffer);
							break;
						}
						if (dir_entry.filename[0])
						{
							if (dir_entry.attr & (FAT32_ATTR_VOLUME_ID | FAT32_ATTR_HIDDEN | FAT32_ATTR_SYSTEM | FAT32_ATTR_DIRECTORY))
							{
								// It's a volume label, hidden file, or system file, skip it
								continue;
							}
							else
							{
								// It's a directory, append '/' to the name
								print_string(dir_entry.filename);
								print_string("\n");
								if (++count % (z_header.screen_rows - 1) == 0)
								{
									os_more_prompt(); // Pause every 24 entries
								}
							}
						}
					} while (dir_entry.filename[0]);

					fat32_close(&dir);
					print_string("\n");
				}
			}
		} while (file_name[0] == '?');
	}

	// Return failure if path provided when in restricted mode.
	// I think this is better than accepting a path/filename
	// and stripping off the path.
	if (f_setup.restricted_path)
	{
		tempname = dirname(file_name);
		if (strlen(tempname) > 1)
		{
			return NULL;
		}
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
			strncpy(file_name, tempname, FAT32_MAX_PATH_LEN);
		}
		else
		{
			strncpy(file_name, default_name, FAT32_MAX_PATH_LEN);
		}
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
		strncpy(file_name, f_setup.restricted_path, FAT32_MAX_PATH_LEN);

		// Make sure the final character is the path separator.
		if (file_name[strlen(file_name) - 1] != PATH_SEPARATOR)
		{
			strncat(file_name, path_separator, FAT32_MAX_PATH_LEN - strlen(file_name) - 2);
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
	uint8_t saved_style = os_get_text_style();
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