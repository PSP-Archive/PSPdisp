/*
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  debug.h

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
*/

#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include <stdarg.h>
#include <pspdebug.h>
#include "common.h"
#include "utils.h"
#include "sio/sio.h"


#define DEBUG 0
#define DEBUG_SIO 0
#define DEBUG_FILE 0

#define DEBUG_AUDIO_SHOW_BUFFER 0

#define DEBUG_FILE_NAME "ms0:/pspdisp.txt"

FILE* __debug_file_handle;

#define DEBUG_INIT do { if (DEBUG) debugInit(); } while (0);
#define DEBUG_PRINTF(...) do { if (DEBUG) { printf(__VA_ARGS__); if (DEBUG_FILE) { fprintf(__debug_file_handle, __VA_ARGS__); fflush(__debug_file_handle); } } } while (0);
#define DEBUG_TERM do { if (DEBUG_FILE) { fclose(__debug_file_handle); } } while (0);

void debugInit();

#endif