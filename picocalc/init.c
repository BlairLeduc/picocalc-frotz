//
// init.c - PicoCalc interface, initialization
//

#include <stdarg.h>
#include "pico/stdlib.h"
#include "hardware/watchdog.h"
#include "pico/rand.h"

#include "audio.h"
#include "fat32.h"
#include "keyboard.h"
#include "lcd.h"
#include "southbridge.h"

#undef bool
#include "picocalc_frotz.h"

extern f_setup_t f_setup;
extern z_header_t z_header;

char selected_story[64];

void _exit(int status)
{
	watchdog_reboot(0, 0, 0); // Reboot the device
	while (1)
		; // Should never reach here
}

char *basename(char *path)
{
	const char *base = strrchr(path, '/');
	return base ? (char *)(base + 1) : (char *)path;
}

char *create_filename(const char *base, const char *extension)
{
	size_t size = (strlen(base) + strlen(extension) + 1) * sizeof(char);
	char *filename = malloc(size);
	memcpy(filename, base, (strlen(base) + 1) * sizeof(char));
	strncat(filename, extension, size);
	return filename;
}

int story_cmp(const void *a, const void *b)
{
	const char *sa = (const char *)a;
	const char *sb = (const char *)b;
	return strcmp(sa, sb);
}

void basic_quit(const char* message)
{
	os_erase_area(1, 1, SCREEN_HEIGHT, SCREEN_WIDTH, 0);
	os_set_cursor(14, 1);
	os_display_string(message);
	os_set_cursor(20, 1);
	os_display_string("      Press any key to retry, or");
	os_set_cursor(22, 1);
	os_display_string("      turn off your PicoCalc now.");
	os_read_key(0, false); // Wait for user input before quitting
	exit(-1);

}

void hide_ext(char *file_name)
{
	char *ext = strrchr(file_name, '.');
	if (ext != NULL)
	{
		*ext = '\0'; // Remove the extension
	}
}

void show_ext(char *file_name)
{
	file_name[strlen(file_name)] = '.';
}

bool select_story(void)
{
	char buffer[64];
	char stories[24][38];

	// list available stories from the filesystem
	// For simplicity, we assume the stories are in a directory called "stories"
	// In a real application, you would use a filesystem API to list files.

	fat32_dir_t dir;
	fat32_error_t result = fat32_dir_open(&dir, "/Stories");
	if (result != FAT32_OK)
	{
		basic_quit("   Error opening /Stories directory!");
	}

	fat32_entry_t dir_entry;
	int num_stories = 0;
	do
	{
		result = fat32_dir_read(&dir, &dir_entry);
		if (result == FAT32_OK && dir_entry.filename[0] && (dir_entry.attr & FAT32_ATTR_HIDDEN) == 0)
		{
			// Check if the file is a Z-machine story file
			if (dir_entry.size > 0 && dir_entry.filename[0] != '.')
			{
				size_t len = strlen(dir_entry.filename);
				if (len >= 3 && dir_entry.filename[len - 3] == '.' && dir_entry.filename[len - 2] == 'z' &&
					dir_entry.filename[len - 1] >= '1' && dir_entry.filename[len - 1] <= '8')
				{
					strncpy(stories[num_stories], dir_entry.filename, sizeof(stories[num_stories]) - 1);
					hide_ext(stories[num_stories]);
					num_stories++;
					if (num_stories >= 24)
					{
						break; // Limit to 24 stories
					}
				}
			}
		}
	} while (dir_entry.filename[0]);
	fat32_dir_close(&dir);

	if (num_stories == 0)
	{
		basic_quit("   No story files found in /Stories.");
	}
	else if (num_stories >= 24)
	{
		os_set_cursor(32, 1);
		os_display_string("Only showing the first 24 stories.");
		num_stories = 24;
	}

	// Sort the stories alphabetically
	qsort(stories, num_stories, sizeof(stories[0]), story_cmp);

	os_set_cursor(3, 1);
	os_display_string("Available stories:");
	for (int i = 0; i < num_stories; i++)
	{
		os_set_cursor(5 + i, 2);
		snprintf(buffer, sizeof(buffer), "%-38s", stories[i]);
		os_display_string(buffer);
	}

	os_set_cursor(7 + num_stories, 1);
	os_display_string("Which story would you like to play?");

	char ch = '\0';
	int count = num_stories - 1;
	int selected = 0;

	os_set_cursor(5, 2);
	os_set_text_style(REVERSE_STYLE);
	snprintf(buffer, sizeof(buffer), "%-38s", stories[selected]);
	os_display_string(buffer);
	os_set_text_style(NORMAL_STYLE);

	do
	{
		ch = os_read_key(0, false);
		os_set_cursor(5 + selected, 2);
		snprintf(buffer, sizeof(buffer), "%-38s", stories[selected]);
		os_display_string(buffer);

		if (ch == ZC_ARROW_UP)
		{
			if (selected > 0)
				selected--;
		}
		else if (ch == ZC_ARROW_DOWN)
		{
			if (selected < num_stories - 1)
				selected++;
		}
		else if (ch == ZC_RETURN)
		{
			if (selected >= 0 && selected < num_stories)
			{
				strncpy(selected_story, stories[selected], sizeof(selected_story) - 1);
				selected_story[sizeof(selected_story) - 1] = '\0';
				break;
			}
		}

		os_set_cursor(5 + selected, 2);
		os_set_text_style(REVERSE_STYLE);
		snprintf(buffer, sizeof(buffer), "%-38s", stories[selected]);
		os_display_string(buffer);
		os_set_text_style(NORMAL_STYLE);
	} while (ch != ZC_RETURN);

	selected_story[0] = '\0';
	show_ext(stories[selected]);
	strncat(selected_story, "/stories/", sizeof(selected_story) - 1);
	strncat(selected_story, stories[selected], sizeof(selected_story) - strlen(selected_story) - 1);

	os_erase_area(1, 1, SCREEN_HEIGHT, SCREEN_WIDTH, 0);
	os_set_cursor(1, 1);
	return TRUE;
}

void os_process_arguments(int UNUSED(argc), char *UNUSED(argv[]))
{
	char *p;

	f_setup.undo_slots = 2;
	f_setup.format = FORMAT_ANSI;

	// Save the story file name
	f_setup.story_file = strdup((char *)selected_story);
	f_setup.story_name = strdup(basename((char *)selected_story));

	// Now strip off the extension
	p = strrchr(f_setup.story_name, '.');
	if (p != NULL)
	{
		*p = '\0';
	}

	// Create nice default file names
	f_setup.script_name = create_filename(f_setup.story_name, EXT_SCRIPT);
	f_setup.command_name = create_filename(f_setup.story_name, EXT_COMMAND);

	if (!f_setup.restore_mode)
	{
		f_setup.save_name = create_filename(f_setup.story_name, EXT_SAVE);
	}
	else
	{
		// Set our auto load save as the name save
		f_setup.save_name = create_filename(f_setup.tmp_save_name, EXT_SAVE);
	}

	f_setup.aux_name = create_filename(f_setup.story_name, EXT_AUX);
}

void os_init_screen(void)
{
	if (z_header.version == V3)
	{
		z_header.config |= CONFIG_SPLITSCREEN;
		z_header.flags &= ~OLD_SOUND_FLAG;
	}

	if (z_header.version >= V4)
	{
		z_header.flags &= ~OLD_SOUND_FLAG;
		z_header.config |= CONFIG_TIMEDINPUT;
	}

	if (z_header.version >= V5)
	{
		z_header.flags |= UNDO_FLAG;
		z_header.flags &= ~SOUND_FLAG;
	}

	z_header.screen_rows = SCREEN_HEIGHT;
	z_header.screen_cols = SCREEN_WIDTH;
	z_header.config |= CONFIG_BOLDFACE | CONFIG_EMPHASIS;
	z_header.screen_height = z_header.screen_rows;
	z_header.screen_width = z_header.screen_cols;
	z_header.font_width = 1;
	z_header.font_height = 1;

	// Use the ms-dos interpreter number for v6, because that's the
	// kind of graphics files we understand.  Otherwise, use DEC.
	if (f_setup.interpreter_number == INTERP_DEFAULT)
	{
		z_header.interpreter_number = z_header.version == 6
			? INTERP_MSDOS
			: INTERP_DEC_20;
	}
	else
	{
		z_header.interpreter_number = f_setup.interpreter_number;
	}

	z_header.interpreter_version = 'F';
}

int os_random_seed(void)
{
	return get_rand_32();
}

void os_quit(int status)
{
	char buffer[2];

	if (status == EXIT_SUCCESS)
	{
		print_string("\n\nGame over. Thanks for playing!\n");
	}
	else
	{
		print_string("\n\nAn error occurred. Please try again.\n");
	}

	print_string("\nPress ENTER to select a story, or ");
	print_string("please turn off your PicoCalc now.\n");
	read_string(1, buffer); // Wait for user input before quitting
}

void os_restart_game(int UNUSED(stage))
{
	// Reset game state not implemented in this interface
}

void os_warn(const char *s, ...)
{
	va_list m;
	int style;
	char buffer[80];

	os_beep(BEEP_HIGH);

	style = os_get_text_style();
	os_set_text_style(BOLDFACE_STYLE);
	print_string("Warning: ");
	os_set_text_style(NORMAL_STYLE);

	va_start(m, s);
	vsnprintf(buffer, sizeof(buffer), s, m);
	va_end(m);

	print_string(buffer);
	os_set_text_style(style);
	print_string("\n");
	return;
}

void os_fatal(const char *s, ...)
{
	va_list m;
	char buffer[80];

	print_string("\nFatal error: ");

	va_start(m, s);
	vsnprintf(buffer, sizeof(buffer), s, m);
	va_end(m);

	print_string(buffer);
	print_string("\n");

	if (!f_setup.ignore_errors)
	{
		os_quit(EXIT_FAILURE);
	}
}

FILE *os_load_story(void)
{
	return fopen(f_setup.story_file, "rb");
}

int os_storyfile_seek(FILE *fp, long offset, int whence)
{
	return fseek(fp, offset, whence);
}

int os_storyfile_tell(FILE *fp)
{
	return ftell(fp);
}

void os_init_setup(void)
{
	sb_init();
	lcd_init();
	keyboard_init(NULL);
	audio_init();
	fat32_init();

	lcd_set_foreground(DEFAULT_PHOSPHOR);
	lcd_enable_cursor(false);
	os_set_cursor(1, 1);
	os_display_string("Welcome to the Unofficial Port of Frotz!");

	if (!select_story())
	{
		os_erase_area(1, 1, SCREEN_HEIGHT, SCREEN_WIDTH, 0);
		os_set_cursor(1, 1);
		os_display_string("No story selected!");
		os_set_cursor(3, 1);
		os_quit(EXIT_FAILURE);
	}
}