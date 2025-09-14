//
// init.c - PicoCalc interface, initialization
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "pico/stdlib.h"
#include "hardware/watchdog.h"
#include "pico/rand.h"

#include "ini.h"

#include "audio.h"
#include "fat32.h"
#include "keyboard.h"
#include "lcd.h"
#include "southbridge.h"

#include "frotz-banner.h"

#undef bool
#include "picocalc_frotz.h"
#include "version.h"

uint8_t columns = 40;
uint16_t phosphor = DEFAULT_PHOSPHOR;						  // Default phosphor type
static char selected_story[FAT32_MAX_PATH_LEN] = {0};		  // Selected story name
static char save_path[FAT32_MAX_PATH_LEN] = "/Stories/saves"; // Default save path

static char selected_template[28];
static char normal_template[28];

// Function to handle system exit
void _exit(int status)
{
	watchdog_reboot(0, 0, 0); // Reboot the device
	while (1)
		; // Should never reach here
}

// Implementation of our own basename function
char *basename(char *path)
{
	const char *base = strrchr(path, '/');
	return base ? (char *)(base + 1) : (char *)path;
}

// Function to create a filename with a specific extension
static char *create_filename(const char *base, const char *extension)
{
	size_t size = (strlen(base) + strlen(extension) + 1) * sizeof(char);
	char *filename = malloc(size);
	memcpy(filename, base, (strlen(base) + 1) * sizeof(char));
	strncat(filename, extension, size);
	return filename;
}

// Function to compare two strings for sorting story names (qsort callback)
static int story_cmp(const void *a, const void *b)
{
	story_t *story_a = (story_t *)a;
	story_t *story_b = (story_t *)b;
	const char *sa = (const char *)story_a->story_filename;
	const char *sb = (const char *)story_b->story_filename;

	return strcmp(sa, sb);
}

static void basic_quit(const char *message)
{
	os_erase_area(1, 1, SCREEN_HEIGHT, columns, 0);
	os_set_cursor(14, 1);
	os_display_string(message);
	os_set_cursor(20, 1);
	os_display_string("      Press any key to retry, or");
	os_set_cursor(22, 1);
	os_display_string("      turn off your PicoCalc now.");
	os_read_key(0, false); // Wait for user input before quitting
	exit(-1);
}

static void hide_ext(char *file_name)
{
	char *ext = strrchr(file_name, '.');
	if (ext != NULL)
	{
		*ext = '\0'; // Remove the extension
	}
}

static void show_ext(char *file_name)
{
	file_name[strlen(file_name)] = '.';
}

static void update_settings_display(int top, int selected, story_t *story, uint32_t defaults)
{
	char buffer[FAT32_MAX_PATH_LEN] = {0};

	// Update settings display
	lcd_set_font(&font_5x10);

	bool is_small_font = defaults & SETTINGS_COLUMNS_64;
	if (story->settings & SETTINGS_SET)
	{
		is_small_font = (story->settings & SETTINGS_COLUMNS_64);
	}

	if (is_small_font)
	{
		lcd_putstr(56, top + 7, "Small   ");
	}
	else
	{
		lcd_set_font(&font_8x10);
		lcd_putstr(35, top + 7, "Large");
		lcd_set_font(&font_5x10);
	}

	uint32_t phosphor_setting = defaults & SETTINGS_PHOSPHOR_MASK;
	if (story->settings & SETTINGS_SET)
	{
		phosphor_setting = story->settings & SETTINGS_PHOSPHOR_MASK;
	}
	if (phosphor_setting == SETTINGS_PHOSPHOR_GREEN)
	{
		lcd_set_foreground(GREEN_PHOSPHOR);
		lcd_putstr(56, top + 9, "Green");
	}
	else if (phosphor_setting == SETTINGS_PHOSPHOR_AMBER)
	{
		lcd_set_foreground(AMBER_PHOSPHOR);
		lcd_putstr(56, top + 9, "Amber");
	}
	else
	{
		lcd_set_foreground(WHITE_PHOSPHOR);
		lcd_putstr(56, top + 9, "White");
	}
	lcd_set_foreground(FOREGROUND_COLOUR);
}

void settings_set_value(uint32_t *settings, const char *name, const char *value)
{
	if (strcmp(name, "columns") == 0)
	{
		*settings |= SETTINGS_SET;
		if (strcmp(value, "64") == 0 || strcmp(value, "\"64\"") == 0)
		{
			*settings |= SETTINGS_COLUMNS_64;
		}
		else
		{
			*settings &= ~SETTINGS_COLUMNS_64;
		}
	}
	else if (strcmp(name, "phosphor") == 0)
	{
		*settings |= SETTINGS_SET;
		*settings &= ~SETTINGS_PHOSPHOR_MASK;

		if (strcmp(value, "green") == 0 || strcmp(value, "\"green\"") == 0)
		{
			*settings |= SETTINGS_PHOSPHOR_GREEN;
		}
		else if (strcmp(value, "amber") == 0 || strcmp(value, "\"amber\"") == 0)
		{
			*settings |= SETTINGS_PHOSPHOR_AMBER;
		}
	}
}

void config_write(config_t *config, FILE *file)
{
	fprintf(file, "# The settings.ini file for Frotz configuration\n");

	// Write default settings
	fprintf(file, "[default]\n");
	if (config->defaults & SETTINGS_SET)
	{
		fprintf(file, "# Number of columns on the screen\n");
		fprintf(file, "# columns=40|64\n");
		if (config->defaults & SETTINGS_COLUMNS_64)
		{
			fprintf(file, "columns=64\n\n");
		}
		else
		{
			fprintf(file, "columns=40\n\n");
		}

		fprintf(file, "# The phosphor setting determines the color scheme of the display.\n");
		fprintf(file, "# phosphor=white|green|amber\n");
		if ((config->defaults & SETTINGS_PHOSPHOR_MASK) == SETTINGS_PHOSPHOR_GREEN)
		{
			fprintf(file, "phosphor=green\n\n");
		}
		else if ((config->defaults & SETTINGS_PHOSPHOR_MASK) == SETTINGS_PHOSPHOR_AMBER)
		{
			fprintf(file, "phosphor=amber\n\n");
		}
		else
		{
			fprintf(file, "phosphor=white\n\n");
		}
	}
	if (config->default_save_path[0] != '\0')
	{
		fprintf(file, "# Save path for saved games\n");
		fprintf(file, "# savepath=/Stories/Saves\n");
		fprintf(file, "savepath=%s\n\n", config->default_save_path);
	}

	// Write individual story settings
	for (size_t i = 0; i < config->story_count; i++)
	{
		if (config->stories[i].settings & SETTINGS_SET)
		{
			fprintf(file, "[%s]\n", config->stories[i].story_filename);
			if (config->stories[i].settings & SETTINGS_COLUMNS_64)
			{
				fprintf(file, "columns=64\n");
			}
			else
			{
				fprintf(file, "columns=40\n");
			}

			if ((config->stories[i].settings & SETTINGS_PHOSPHOR_MASK) == SETTINGS_PHOSPHOR_GREEN)
			{
				fprintf(file, "phosphor=green\n");
			}
			else if ((config->stories[i].settings & SETTINGS_PHOSPHOR_MASK) == SETTINGS_PHOSPHOR_AMBER)
			{
				fprintf(file, "phosphor=amber\n");
			}
			else
			{
				fprintf(file, "phosphor=white\n");
			}
			fprintf(file, "\n");
		}
	}
}

int config_handler(void *user, const char *section, const char *name, const char *value)
{
	char buffer[CONFIG_MAX_FILENAME_LEN] = {0};
	config_t *config = (config_t *)user;

	if (strcmp(section, "default") == 0)
	{
		settings_set_value(&config->defaults, name, value);
	}
	else
	{
		strncpy(buffer, section, sizeof(buffer) - 1);
		hide_ext(buffer);

		// Look for existing story entry
		for (size_t i = 0; i < config->story_count; i++)
		{
			if (strcmp(config->stories[i].story_filename, buffer) == 0)
			{
				// Found existing story entry
				settings_set_value(&config->stories[i].settings, name, value);
				return 1;
			}
		}
	}
	return 1; // Ignore unknown sections
}

void story_page(config_t *config, int top, int page_start, int selected)
{
	char buffer[40];

	lcd_set_font(&font_8x10);
	for (int i = 0; i < CONFIG_MAX_STORIES_PER_SCREEN; i++)
	{
		strncpy(buffer, i + page_start == selected ? selected_template : normal_template, sizeof(buffer) - 1);

		if (i + page_start < config->story_count)
		{
			memcpy(buffer, config->stories[i + page_start].story_filename, MIN(strlen(config->stories[i + page_start].story_filename), MAX_DISPLAY_FILENAME_LEN));
		}
		lcd_putstr(0, top + i, buffer);
	}
}

story_t *select_story(config_t *config)
{
	char buffer[FAT32_MAX_PATH_LEN];
	char ch = '\0';
	int page_start = 0;
	int count = config->story_count - 1;
	int selected = 0;
	int top = 10;

	// Prepare the selection templates
	for (int i = 0; i < MAX_DISPLAY_FILENAME_LEN; i++)
	{
		selected_template[i] = '\x12';
		normal_template[i] = '\x20';
	}
	selected_template[MAX_DISPLAY_FILENAME_LEN] = '\x16';
	normal_template[MAX_DISPLAY_FILENAME_LEN] = '\x19';

	// Display the banner
	lcd_clear_screen();
	lcd_blit(frotz_banner, 30, 0, 259, 84);
	lcd_set_foreground(FOREGROUND_COLOUR);
	lcd_set_font(&font_5x10);
	snprintf(buffer, sizeof(buffer), "Version %s. Port to PicoCalc v%s.", VERSION, PICOCALC_FROTZ_VERSION);
	lcd_putstr(14, 8, buffer);
	lcd_putstr(0, 31, "Port Copyright Blair Leduc.");
	lcd_set_font(columns == 40 ? &font_8x10 : &font_5x10);

	// List available stories from the filesystem
	story_page(config, top, page_start, selected);

	// Place the settings text
	lcd_set_font(&font_5x10);
	lcd_set_underscore(true);
	lcd_putc(46, top + 7, 'F');
	lcd_putc(46, top + 9, 'P');
	lcd_putstr(46, top + 11, "Enter");
	lcd_set_underscore(false);
	lcd_putstr(47, top + 7, "ont:");
	lcd_putstr(47, top + 9, "hosphor:");
	lcd_putstr(52, top + 11, "to start");

	// Initial display of settings
	update_settings_display(top, selected, &config->stories[selected], config->defaults);

	do
	{
		// Display battery level for each key press
		uint8_t battery_level = sb_read_battery() & 0x7F;
		snprintf(buffer, sizeof(buffer), "Battery: %u%%", battery_level);
		lcd_set_font(&font_5x10);
		lcd_putstr(51, 31, buffer);

		// Wait for user input
		ch = os_read_key(0, false);

		// Redraw the previously selected story in normal style
		lcd_set_font(&font_8x10);
		strncpy(buffer, normal_template, sizeof(buffer) - 1);
		memcpy(buffer, &config->stories[selected].story_filename, MIN(strlen(config->stories[selected].story_filename), MAX_DISPLAY_FILENAME_LEN));
		lcd_putstr(0, top + selected - page_start, buffer);

		if (ch == ZC_ARROW_UP)
		{
			if (selected > 0)
			{
				if (selected - page_start <= 0)
				{
					page_start--;
					story_page(config, top, page_start, selected - 1);
				}
				selected--;
			}
		}
		else if (ch == KEY_PAGE_UP)
		{
			if (page_start > 0)
			{
				page_start -= CONFIG_MAX_STORIES_PER_SCREEN;
				if (page_start < 0)
				{
					page_start = 0;
				}
				selected = page_start;
				story_page(config, top, page_start, selected);
			}
		}
		else if (ch == KEY_PAGE_DOWN)
		{
			if (page_start + CONFIG_MAX_STORIES_PER_SCREEN <= config->story_count - CONFIG_MAX_STORIES_PER_SCREEN)
			{
				page_start += CONFIG_MAX_STORIES_PER_SCREEN;
				selected = page_start;
				story_page(config, top, page_start, selected);
			}
		}
		else if (ch == ZC_ARROW_DOWN)
		{
			if (selected < config->story_count - 1)
			{
				if (selected - page_start >= CONFIG_MAX_STORIES_PER_SCREEN - 1)
				{
					page_start++;
					story_page(config, top, page_start, selected + 1);
				}
				selected++;
			}
		}
		else if (ch == ZC_RETURN)
		{
			if (selected >= 0 && selected < config->story_count)
			{
				strncpy(selected_story, config->stories[selected].story_filename, sizeof(selected_story) - 1);
				selected_story[sizeof(selected_story) - 1] = '\0';
				break;
			}
		}
		else if (ch == 'f' || ch == 'F')
		{
			// Toggle font size
			columns = (config->stories[selected].settings & SETTINGS_COLUMNS_64) ? 40 : 64;
			config->stories[selected].settings &= ~SETTINGS_COLUMNS_MASK;
			config->stories[selected].settings |= SETTINGS_SET | (columns == 64 ? SETTINGS_COLUMNS_64 : 0);
		}
		else if (ch == 'p' || ch == 'P')
		{
			config->stories[selected].settings &= ~SETTINGS_PHOSPHOR_MASK;
			// Cycle through phosphor types
			if (phosphor == GREEN_PHOSPHOR)
			{
				phosphor = AMBER_PHOSPHOR;
				config->stories[selected].settings |= SETTINGS_SET | SETTINGS_PHOSPHOR_AMBER;
			}
			else if (phosphor == AMBER_PHOSPHOR)
			{
				phosphor = WHITE_PHOSPHOR;
				config->stories[selected].settings |= SETTINGS_SET;
			}
			else
			{
				phosphor = GREEN_PHOSPHOR;
				config->stories[selected].settings |= SETTINGS_SET | SETTINGS_PHOSPHOR_GREEN;
			}
		}

		// Update story name to point to settings
		lcd_set_font(&font_8x10);

		strncpy(buffer, selected_template, sizeof(buffer) - 1);
		memcpy(buffer, &config->stories[selected].story_filename, MIN(strlen(config->stories[selected].story_filename), MAX_DISPLAY_FILENAME_LEN));
		lcd_putstr(0, top + selected - page_start, buffer);

		// Redraw the newly selected story in selected style
		update_settings_display(top, selected, &config->stories[selected], config->defaults);

	} while (ch != ZC_RETURN);

	return &config->stories[selected];
}

void os_process_arguments(int UNUSED(argc), char *UNUSED(argv[]))
{
	char *p;

	f_setup.undo_slots = 20;
	f_setup.format = FORMAT_ANSI;

	// Save the story file name
	f_setup.story_file = strdup((char *)selected_story);
	f_setup.story_name = strdup(basename((char *)selected_story));

	f_setup.restricted_path = save_path; // Set the restricted path for file operations

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
	z_header.screen_cols = columns;
	z_header.config |= CONFIG_EMPHASIS;
	if (columns == 40)
	{
		z_header.config |= CONFIG_BOLDFACE;
	}
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
	keyboard_init();
	audio_init();
	fat32_init();

	lcd_enable_cursor(false);
	keyboard_set_background_poll(true);

	config_t config = {0};
	config.defaults = SETTINGS_SET;
	strcpy(config.default_save_path, "/Stories/Saves");

	fat32_file_t dir;
	fat32_error_t result = fat32_open(&dir, "/Stories");
	if (result != FAT32_OK)
	{
		basic_quit("   Error opening /Stories directory!");
	}

	// Scan for story files in the /Stories directory
	// and add them to the settings list
	fat32_entry_t dir_entry;
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
					strncpy(
						config.stories[config.story_count].story_filename,
						dir_entry.filename,
						sizeof(config.stories[config.story_count].story_filename) - 1);
					hide_ext(config.stories[config.story_count].story_filename);
					config.story_count++;
					if (config.story_count >= CONFIG_MAX_STORIES)
					{
						break; // Limit to MAX_NUM_STORIES
					}
				}
			}
		}
	} while (dir_entry.filename[0]);

	fat32_close(&dir);

	if (config.story_count == 0)
	{
		lcd_clear_screen();
		basic_quit("   No story files found in /Stories.");
	}

	// Sort the stories alphabetically
	qsort(config.stories, config.story_count, sizeof(story_t), story_cmp);

	// Load default settings and the story settings from the INI file
	const char *ini_name = "/Stories/settings.ini";
	ini_parse(ini_name, config_handler, &config);

	story_t *story = select_story(&config);
	if (!selected_story)
	{
		os_erase_area(1, 1, SCREEN_HEIGHT, columns, 0);
		os_set_cursor(1, 1);
		os_display_string("No story selected!");
		os_set_cursor(3, 1);
		os_quit(EXIT_FAILURE);
	}

	// Update the INI file with any new settings (if we can)
	FILE *ini_file = fopen(ini_name, "w+");
	if (ini_file)
	{
		config_write(&config, ini_file);
		fclose(ini_file);
	}

	// Construct the full path to the selected story
	selected_story[0] = '\0';
	show_ext(story->story_filename);
	strncat(selected_story, "/Stories/", sizeof(selected_story) - strlen(selected_story) - 1);
	strncat(selected_story, story->story_filename, sizeof(selected_story) - strlen(selected_story) - 1);
	hide_ext(story->story_filename);

	// Apply default settings for the selected story if not set
	if (!(story->settings & SETTINGS_SET))
	{
		story->settings = config.defaults;
	}

	if (story->settings & SETTINGS_COLUMNS_64)
	{
		columns = 64;
		lcd_set_font(&font_5x10);
	}
	else
	{
		columns = 40;
		lcd_set_font(&font_8x10);
	}

	if (story->settings & SETTINGS_PHOSPHOR_GREEN)
	{
		phosphor = GREEN_PHOSPHOR;
	}
	else if (story->settings & SETTINGS_PHOSPHOR_AMBER)
	{
		phosphor = AMBER_PHOSPHOR;
	}
	else
	{
		phosphor = WHITE_PHOSPHOR;
	}
	lcd_set_foreground(phosphor);

	// Create the save path
	strncpy(save_path, config.default_save_path, sizeof(save_path) - 1);
	strncat(save_path, "/", sizeof(save_path) - strlen(save_path) - 1);
	strncat(save_path, story->story_filename, sizeof(save_path) - strlen(save_path) - 1);
	save_path[sizeof(save_path) - 1] = '\0'; // Ensure null-termination

	// Create the save directory if it doesn't exist
	if (fat32_open(&dir, save_path) != FAT32_OK)
	{
		if (fat32_dir_create(&dir, save_path) == FAT32_OK)
		{
			fat32_close(&dir);
		}
	}
	else
	{
		fat32_close(&dir);
	}

	// Clear the screen for the game
	os_erase_area(1, 1, SCREEN_HEIGHT, columns, 0);
	os_set_cursor(1, 1);
}