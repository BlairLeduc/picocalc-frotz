#pragma once

#define VERSION "2.55"
#define RELEASE_NOTES "Unofficial PicoCalc Frotz Port"

/* Basic configuration for embedded/PicoCalc build */
#define MAX_UNDO_SLOTS 25
#define MAX_FILE_NAME 80
#define TEXT_BUFFER_SIZE 275
#define INPUT_BUFFER_SIZE 80
#define STACK_SIZE 1024

/* Disable features not needed for embedded systems */
#define NO_BLORB
#define NO_SOUND
#define NO_GRAPHICS
#define NO_TRUECOLOUR

/* Platform specific defines */
#define UNIX
