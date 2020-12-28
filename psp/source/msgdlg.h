/*
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  msgdlg.h

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
*/

#ifndef MSGDLG_H
#define MSGDLG_H


#include <pspsdk.h>
#include <psputility.h>
#include "common.h"


bool g_msgActive;


int msgShowError(char* message, bool defaultSettings, bool yesNoChoice);


#endif