/*
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  osk.h

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
*/

#ifndef OSK_H
#define OSK_H

#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspdebug.h>
#include <pspgu.h>
#include <string.h>
#include <psputility.h>

#include "config.h"
#include "graphic.h"

#include "library/danzeff.h"


#define OSK_INPUT_ALL 0
#define OSK_INPUT_NUMBERS 1

bool g_oskActive;

bool g_oskDanzeffActive;
bool g_oskDanzeffEnabled;
bool g_oskDanzeffOpenOnSelect;

int oskDrawKeyboard(char* inputText, char* inputDescription, char* outputText, unsigned int maxChars, int flags);

void oskInitDanzeff();
void oskCheckDanzeffState(SceCtrlData* ctrlData);

#endif