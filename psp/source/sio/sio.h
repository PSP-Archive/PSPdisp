/*
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  sio.h

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
*/

#ifndef SIO_H
#define SIO_H

int kernelDebugSioInit(int baudrate);
int kernelDebugSioTerm();
int kernelDebugSioPutText(const char *data, int len);

#endif