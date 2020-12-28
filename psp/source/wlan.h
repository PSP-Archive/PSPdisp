/*
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  wlan.h

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
*/

#ifndef WLAN_H
#define WLAN_H

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pspsdk.h>
#include <psptypes.h>
#include <psputility.h>
#include <pspnet.h>
#include <pspnet_inet.h>
#include <pspnet_resolver.h>
#include <pspnet_apctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "common.h"
#include "debug.h"
#include "config.h"
#include "graphic.h"
#include "library/intrafont.h"

// sceNetInetSocketAbort is not defined in the PSPSDK headers
#ifndef sceNetInetSocketAbort
extern int sceNetInetSocketAbort(int s);
#endif


#define NET_PORT 17584


#define NET_MAX_IP_LENGTH 32


#define NET_CONNECT_ERROR_TIMEOUT -1
#define NET_CONNECT_ERROR_FATAL -2
#define NET_CONNECT_ERROR_NO_LAN -3


#define NET_SOCKET_TIMEOUT (3 * 1000 * 1000)


bool g_netDialogActive; // Is the connection dialog active?

bool g_netConnected; // Connection to the server established?
int g_netSocket; // Handle of the active socket connection
bool g_netIpChanged; // Ip address got changed through the menu



bool netStartWlanConnection();
bool netDialog();
bool netInit();
bool netTerm();
bool netLoadModules();
bool netUnloadModules();
int netConnect(unsigned long remoteIp, unsigned short remotePort, unsigned int timeout);
void netDisconnect();
bool netResolveName(char *name, in_addr_t *addr);
bool netSetNewServerIp(char* newIp, bool doResolve);
SceUID netSocketTimeoutAlarm(void* common);
void wlanWatchdogThread(SceSize args, void *argp);
void netdrawConnectingMessage(char *ipText, char *resolvedIp);
int netConnectToServerLoop(char* newIp);
void netSendWakeOnLanPacket();


#endif