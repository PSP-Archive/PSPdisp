/*
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  osk.c - handling the onscreen keyboards

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
*/

#include "osk.h"

#include "com.h"
#include "menu.h"
#include "msgdlg.h"


int l_danzeffPositionX = 0; // Coordinates of the OSK
int l_danzeffPositionY = 0;


/*
  fillWidecharBuffer
  ---------------------------------------------------
  Convert byte to multi-byte string.
  ---------------------------------------------------
*/
void fillWidecharBuffer(char* inputText, unsigned short* outputBuffer)
{
  int i;
  int length;

  length = strlen(inputText);

  for (i = 0; i < length; i++)
  {
    *outputBuffer = *inputText;
    outputBuffer++;
    inputText++;
  }

  *outputBuffer = 0;
}



/*
  fillBufferFromWidechar
  ---------------------------------------------------
  Convert multi-byte to single-byte string.
  ---------------------------------------------------
*/
void fillBufferFromWidechar(unsigned short* inputBuffer, char* outputText)
{
  int i;

  for (i = 0; inputBuffer[i]; i++)
  {
    outputText[i] = inputBuffer[i];
  }

  outputText[i] = 0;
}




/*
  drawKeyboard
  ---------------------------------------------------
  Draw and handle the on screen keyboard.
  ---------------------------------------------------
  Returns 0 when text was changed, -1 when not.
*/
int oskDrawKeyboard(char* inputText, char* inputDescription, char* outputText, unsigned int maxChars, int flags)
{
  g_oskActive = true;

  int done = 0;

  // Set normal CPU speed
  powerSetClockSpeed(0);

  int inputType;
  if (flags == OSK_INPUT_NUMBERS)
    inputType = PSP_UTILITY_OSK_INPUTTYPE_LATIN_DIGIT;
  else 
    inputType = PSP_UTILITY_OSK_INPUTTYPE_LATIN_DIGIT | PSP_UTILITY_OSK_INPUTTYPE_LATIN_LOWERCASE;

  #define NUM_INPUT_FIELDS (1)
  #define TEXT_LENGTH (128)

  unsigned short intext[NUM_INPUT_FIELDS][TEXT_LENGTH];
  unsigned short outtext[NUM_INPUT_FIELDS][TEXT_LENGTH];
  unsigned short desc[NUM_INPUT_FIELDS][TEXT_LENGTH];

  memset(&intext, 0, NUM_INPUT_FIELDS * TEXT_LENGTH * sizeof(unsigned short));
  memset(&outtext, 0, NUM_INPUT_FIELDS * TEXT_LENGTH * sizeof(unsigned short));
  memset(&desc, 0, NUM_INPUT_FIELDS * TEXT_LENGTH * sizeof(unsigned short));

  int i;

  for(i = 0;i < NUM_INPUT_FIELDS;i++)
  {
    fillWidecharBuffer(inputDescription, desc[i]);
    fillWidecharBuffer(inputText, intext[i]);
  }

  SceUtilityOskData data[NUM_INPUT_FIELDS];

  for(i = 0; i < NUM_INPUT_FIELDS;i++)
  {
    memset(&data[i], 0, sizeof(SceUtilityOskData));
    data[i].language = PSP_UTILITY_OSK_LANGUAGE_DEFAULT; // Use system default for text input
    data[i].lines = 1;
    data[i].unk_24 = 1;
    data[i].inputtype = inputType;
    data[i].desc = desc[i];
    data[i].intext = intext[i];
    data[i].outtextlength = TEXT_LENGTH;
    data[i].outtextlimit = maxChars;
    data[i].outtext = outtext[i];
  }

  SceUtilityOskParams params;
  memset(&params, 0, sizeof(params));
  params.base.size = sizeof(params);

  if (configXOButtons == 0)
    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_UNKNOWN, &params.base.buttonSwap);
  else if (configXOButtons == 1)
    params.base.buttonSwap = PSP_UTILITY_ACCEPT_CROSS;
  else
    params.base.buttonSwap = PSP_UTILITY_ACCEPT_CIRCLE;
  
  sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE, &params.base.language);
  params.base.graphicsThread = 17;
  params.base.accessThread = 19;
  params.base.fontThread = 18;
  params.base.soundThread = 16;
  params.datacount = NUM_INPUT_FIELDS;
  params.data = data;

  sceUtilityOskInitStart(&params);

  while(!done)
  {
    graphicDrawBackground(false, true);

    switch(sceUtilityOskGetStatus())
    {
      case PSP_UTILITY_DIALOG_INIT:
        break;

      case PSP_UTILITY_DIALOG_VISIBLE:
        sceUtilityOskUpdate(1);
        break;

      case PSP_UTILITY_DIALOG_QUIT:
        sceUtilityOskShutdownStart();
        break;

      case PSP_UTILITY_DIALOG_FINISHED:
        break;

      case PSP_UTILITY_DIALOG_NONE:
        done = 1;

      default:
       break;
    }

    sceDisplayWaitVblankStart();
    graphicSwitchBuffers();
  }

  // Restore CPU speed
  powerSetClockSpeedByCurrentMode();

  int result = -1;

  for (i = 0; i < NUM_INPUT_FIELDS; i++)
  {
    switch(data[i].result)
    {
      case PSP_UTILITY_OSK_RESULT_UNCHANGED:
      case PSP_UTILITY_OSK_RESULT_CANCELLED:
        result = -1;
        break;

      case PSP_UTILITY_OSK_RESULT_CHANGED:
        fillBufferFromWidechar(data[i].outtext, outputText);
        result = 0;
        break;

      default:
        break;
    }
  }

  g_oskActive = false;

  return result;
}





/*
  oskCheckDanzeffState
  ---------------------------------------------------
  Check wether the danzeff keyboard is active.
  ---------------------------------------------------
*/
void oskCheckDanzeffState(SceCtrlData* ctrlData)
{
  static unsigned int s_lastButtons = 0;

  if (g_menuActive)
    return;

  if (g_oskDanzeffEnabled)
  {
    // Get state of the osk
    if (ctrlData->Buttons != s_lastButtons)
    {
      // Move keyboard around with the START button
      if (g_oskDanzeffActive && !(ctrlData->Buttons & PSP_CTRL_START) && (s_lastButtons & PSP_CTRL_START))
      {
        if ((l_danzeffPositionX == 0) && (l_danzeffPositionY == 0))
        {
          l_danzeffPositionX = 480 - 150;
        }
        else if ((l_danzeffPositionX == 480 - 150) && (l_danzeffPositionY == 0))
        {
          l_danzeffPositionY = 272 - 150;
        }
        else if ((l_danzeffPositionX == 480 - 150) && (l_danzeffPositionY == 272 - 150))
        {
          l_danzeffPositionX = 0;
        }
        else if ((l_danzeffPositionX == 0) && (l_danzeffPositionY == 272 - 150))
        {
          l_danzeffPositionY = 0;
        }
        danzeff_moveTo(l_danzeffPositionX, l_danzeffPositionY);
      }
      else if (!(ctrlData->Buttons & PSP_CTRL_SELECT) && (s_lastButtons & PSP_CTRL_SELECT))
      {
        // SELECT released
        g_oskDanzeffActive = (!(g_oskDanzeffActive) && g_oskDanzeffOpenOnSelect);
      }

      s_lastButtons = ctrlData->Buttons;
    }

    unsigned char danzeffInput;
    if (g_oskDanzeffActive)
    {
      danzeffInput = danzeff_readInput(*ctrlData);
      if (danzeffInput > 0)
      {
        ctrlData->Buttons = (((unsigned int)danzeffInput) << 24);
      }
      else
      {
        ctrlData->Buttons = ((unsigned int)255 << 24);
      }
    }
  }
}



/*
  oskInitDanzeff
  ---------------------------------------------------
  Load and initialize the Danzeff OSK resources.
  ---------------------------------------------------
*/
void oskInitDanzeff()
{
  // Init Danzeff keyboard
  danzeff_load();

  if (danzeff_isinitialized() == 0)
  {
    // Danzeff init failed
    msgShowError("Danzef OSK cannot be initialized.", false, false);
    sceKernelExitGame();
  }

  danzeff_moveTo(0, 0);

  g_oskDanzeffActive = false;
  g_oskDanzeffEnabled = true;
  g_oskDanzeffOpenOnSelect = false;
}
