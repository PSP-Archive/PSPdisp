/*
  ***************************************************
  PSPdisp (c) 2008 Jochen Schleu

  debug.c - kernel debugger output

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
*/

#include "debug.h"
#include "pspdisp.h"


VOID DebugPrint(IN PCHAR DebugMessage, ...) 
{
#ifdef KERNEL_DEBUGGER_OUTPUT
    
  va_list ap;
  va_start(ap, DebugMessage);

  EngDebugPrint("PSPdisp: ", DebugMessage, ap);

  va_end(ap);

#endif
}