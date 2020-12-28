/*
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  msgdlg.c - built in message dialog

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
*/

#include "msgdlg.h"

#include "graphic.h"
#include "config.h"


/*
  msgShowError
  ---------------------------------------------------
  Display a message with the built in PSP error dialog.
  ---------------------------------------------------
*/
int msgShowError(char* message, bool defaultSettings, bool yesNoChoice)
{
  g_msgActive = true;

  pspUtilityMsgDialogParams dialog;

  memset(&dialog, 0, sizeof(pspUtilityMsgDialogParams));
  dialog.base.size = sizeof(pspUtilityMsgDialogParams);

  sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE, &dialog.base.language);

  // By the time this is called, the config file may not be loaded yet
  if (defaultSettings)
    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_UNKNOWN, &dialog.base.buttonSwap);
  else
    dialog.base.buttonSwap = ((configXOButtons == 0) ? PSP_UTILITY_ACCEPT_CROSS : PSP_UTILITY_ACCEPT_CIRCLE);

  dialog.base.graphicsThread = 0x11;
  dialog.base.accessThread = 0x13;
  dialog.base.fontThread = 0x12;
  dialog.base.soundThread = 0x10;

  dialog.mode = PSP_UTILITY_MSGDIALOG_MODE_TEXT;
  dialog.options = PSP_UTILITY_MSGDIALOG_OPTION_TEXT;
  if (yesNoChoice)
    dialog.options |= PSP_UTILITY_MSGDIALOG_OPTION_YESNO_BUTTONS;
  strcpy(dialog.message, message);

  sceUtilityMsgDialogInitStart(&dialog);

  bool running = true;

  while (running)
  {
    graphicDrawBackground(false, false);

    switch(sceUtilityMsgDialogGetStatus()) 
    {
      case PSP_UTILITY_DIALOG_VISIBLE:
        sceUtilityMsgDialogUpdate(1);
        break;

      case PSP_UTILITY_DIALOG_QUIT:
        sceUtilityMsgDialogShutdownStart();
        break;

      case PSP_UTILITY_DIALOG_NONE:
		running = false;
        break;
    }

    sceDisplayWaitVblankStart();
    sceGuSwapBuffers();
  }

  g_msgActive = false;

  return (dialog.buttonPressed == PSP_UTILITY_MSGDIALOG_RESULT_YES);
}
