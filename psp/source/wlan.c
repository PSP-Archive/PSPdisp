/*
  ***************************************************
  PSPdisp (c) 2008 - 2015 Jochen Schleu

  wlan.c - wireless lan connection handling

  This software is licensed under the BSD license.
  See license.txt for details.
  ***************************************************
*/

#include "wlan.h"

#include "com.h"
#include "msgdlg.h"


int l_resolverId = -1; // ID of the DNS name resolver object
in_addr_t l_ip; // Current server IP address
char* l_ipRaw = NULL; // Server IP address string as entered
char l_ipResolved[NET_MAX_IP_LENGTH + 1]; // Resolved IP address string
bool l_ipIsResolved = false; // The IP got resolved and is stored in l_ip
bool l_ipRawContainsName = false; // l_ipRaw contains a name, not an IP
int l_eventHandlerId = -1; // ID of the apctl event handler

SceUID l_timeoutSema = -1; // ID of the timeout signaling semaphore
SceUID l_watchdogThreadId = -1; // ID of the timeout watchdog thread
bool l_watchdogThreadRunning = false; // true while the watchdog thread should run
int l_tempSocket = -1; // Socket used for the initial connection phase


/*
  net_callback
  ---------------------------------------------------
  Callback function for net events.
  ---------------------------------------------------
*/
void net_callback(int oldState, int newState, int event, int error, void *pArg) 
{
  DEBUG_PRINTF("net_callback: old=%d, new=%d\n", oldState, newState)

  if ((oldState == PSP_NET_APCTL_STATE_GOT_IP) && (newState == PSP_NET_APCTL_STATE_DISCONNECTED))
  {
    // Lost connection to wireless network
    g_netConnected = false;
    
    // Stop any pending name resolving
    if (l_resolverId > -1)
      sceNetResolverStop(l_resolverId);

    // Close socket connection
    sceNetInetSocketAbort(g_netSocket);
    sceNetInetShutdown(g_netSocket, SHUT_RDWR);
    sceNetInetClose(g_netSocket);
  }
}




/*
  netSocketTimeoutAlarm
  ---------------------------------------------------
  Callback function the timeout alarm. Makes sure that
  snd/rcv operations terminate after a set amount of
  time.
  ---------------------------------------------------
*/
SceUID netSocketTimeoutAlarm(void* common)
{
  sceKernelSignalSema(l_timeoutSema, 1);
  return 0;
}




/*
  wlanWatchdogThread
  ---------------------------------------------------
  Thread stopping net functions.
  ---------------------------------------------------
*/
void wlanWatchdogThread(SceSize args, void *argp)
{
  while (l_watchdogThreadRunning)
  {
    DEBUG_PRINTF("wlanWatchodgThread ready\n")

    sceKernelWaitSema(l_timeoutSema, 1, NULL);

    DEBUG_PRINTF("***netSocketTimeoutAlarm triggered: socket=%d, temp_socket=%d, resolver=%d\n", g_netSocket, l_tempSocket, l_resolverId)

    if (g_netSocket > -1)
      sceNetInetSocketAbort(g_netSocket);

    if (l_tempSocket > -1)
    {
      sceNetInetSocketAbort(l_tempSocket);
      sceNetInetShutdown(l_tempSocket, SHUT_RDWR);
      sceNetInetClose(l_tempSocket);
    }

    if (l_resolverId > -1)
      sceNetResolverStop(l_resolverId);

    g_netConnected = false;
  }
}





/*
  netStartWlanConnection
  ---------------------------------------------------
  Initialize the WLAN connection.
  ---------------------------------------------------
  Return true if a WLAN connection got established.
*/
bool netStartWlanConnection()
{
  // Load and start WLAN modules
  netLoadModules();

  // Loop
  bool wlanLoopRunning = true;
  g_comRunning = true;
  g_netIpChanged = false;

  while (wlanLoopRunning)
  {
    DEBUG_PRINTF("wlan loop: connecting to AP\n")

    // Initialize WLAN functions
    if (netInit())
    {
      // Turn off WLAN LED
      if (!configWlanLedEnabled)
        kernelSetWlanLedState(false);

      // Start connection wizard 
      if (netDialog())
      {
        int apConnectionState;

        do
        {
          // Connected to an access point, start connecting to server
          DEBUG_PRINTF("wlan loop: connecting to server\n")

          g_netSocket = netConnectToServerLoop(NULL);

          // Send password, the server will reset the connection if it doesn't match
          SceUID alarmId = sceKernelSetAlarm(NET_SOCKET_TIMEOUT, (SceKernelAlarmHandler)netSocketTimeoutAlarm, NULL);
          sceNetInetSend(g_netSocket, &configWlanPassword, 32, 0);     
          sceKernelCancelAlarm(alarmId);

          if ((g_netSocket >= 0) && (wlanLoopRunning))
          {
            g_netConnected = true;

            DEBUG_PRINTF("wlan loop: entering com loop\n");
            comLoop();
            DEBUG_PRINTF("wlan loop: exiting com loop\n");

            netDisconnect();
            DEBUG_PRINTF("wlan loop: disconnected from server\n");
          }

          wlanLoopRunning = g_comRunning;
          sceNetApctlGetState(&apConnectionState);
        }
        while (wlanLoopRunning && (apConnectionState == PSP_NET_APCTL_STATE_GOT_IP));

      }
      else
      {
        // User canceled the net dialog, exit WLAN mode
        wlanLoopRunning = false;
        g_comRunning = false;
      }
    }
    else
    {
      // Failed to initilize, show message and return to menu
      msgShowError("Failed to initialize the network modules. WLAN mode is not available.", false, false);
      wlanLoopRunning = false;
      g_comRunning = false;
    }

    netTerm();
  }  

  netUnloadModules();

  return true;
}





/*
  netConnectToServerLoop
  ---------------------------------------------------
  Draw connection message, resolve IP and
  connect to server.
  ---------------------------------------------------
  Returns false on error.
*/
int netConnectToServerLoop(char* newIp)
{
  // Draw initial message depending on whether the name has to be resolved
  newIp = configIpAddresses[configIpAddressIndex - 1];
  netSetNewServerIp(newIp, false);
  netdrawConnectingMessage(l_ipRaw, l_ipRawContainsName ? l_ipResolved : NULL);

  int result = -1;

  while (g_comRunning && (result < 0))
  {
    if (g_netIpChanged)
    {
      l_ipIsResolved = false;
      g_netIpChanged = false;
    }

    if (!l_ipIsResolved)
    {
      newIp = configIpAddresses[configIpAddressIndex - 1];
      netSetNewServerIp(newIp, true);
    }

    // Draw message overlay "connecting..."
    netdrawConnectingMessage(l_ipRaw, l_ipRawContainsName ? l_ipResolved : NULL);

    // Try to wake up the server
    if (configWlanWakeOnLanEnabled)
      netSendWakeOnLanPacket();

    result = netConnect(l_ip, NET_PORT + configWlanPortOffset, 1500);

    // Evaluate connection result
    if (result == NET_CONNECT_ERROR_FATAL)
    {
      // Should not happen, display fatal error and quit
      msgShowError("Internal socket error.\n\nConnectToServer returned -2.\n\nPlease restart the PSPdisp game.", false, false);
      sceKernelExitGame();
    }
    else if ((result == NET_CONNECT_ERROR_NO_LAN) || (result == NET_CONNECT_ERROR_TIMEOUT))
    {
      // Connect failed, possibly missing Wlan connectivity
      int currentState = -1;
      if (sceNetApctlGetState(&currentState) < 0)
      {
        // Not sure if this can fail, but if it does: display fatal error and quit
        msgShowError("Internal socket error.\n\nsceNetApctlGetState failed.\n\nPlease restart the PSPdisp game.", false, false);
        sceKernelExitGame();
      }
      else
      {
        if (currentState != PSP_NET_APCTL_STATE_GOT_IP)
        {
          // Lost AP connection, return error
          l_ipIsResolved = false;

          return -1;
        }
      }
    }
  }

  return result;
}





/*
  netdrawConnectingMessage
  ---------------------------------------------------
  Display the "Connecting..." overlay.
  ---------------------------------------------------
*/
void netdrawConnectingMessage(char *ipText, char *resolvedIp)
{
  if (g_menuActive)
    return;

  graphicRedrawLastFrame();
 
  graphicStartDrawing();

  char buffer[100];
  if (configWlanPortOffset == 0)
    sprintf(buffer, "Connecting to '%s'.", ipText);
  else
    sprintf(buffer, "Connecting to '%s:%d'.", ipText, NET_PORT + configWlanPortOffset);

  intraFontSetStyle(sansSerifLarge, 1.0f, COLOR_WHITE, COLOR_BLACK, INTRAFONT_ALIGN_CENTER);
  intraFontPrint(sansSerifLarge, 240, 125, buffer);

  if (resolvedIp)
  {
    sprintf(buffer, "(%s)", resolvedIp);
    intraFontPrint(sansSerifLarge, 240, 145, buffer);
  }

  intraFontSetStyle(sansSerifSmall, 1.0f, COLOR_WHITE, COLOR_BLACK, INTRAFONT_ALIGN_CENTER);
  intraFontPrint(sansSerifSmall, 240, 180, "The IP address can be changed in the settings menu.");

  graphicFinishDrawing(true);
}





/*
  netDialog
  ---------------------------------------------------
  Display the firmwares wlan connection dialog.
  Basically copied from an sdk sample.
  ---------------------------------------------------
  Returns true when connected to an AP.
*/
bool netDialog()
{
  g_netDialogActive = true;

  bool done = false;

  // Restore standard clock speed
  powerSetClockSpeed(0);

  // Retrieve the system language
  int systemLanguage = 0;
  if (sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE, &systemLanguage) == PSP_SYSTEMPARAM_RETVAL_FAIL)
  {
    systemLanguage = PSP_SYSTEMPARAM_LANGUAGE_ENGLISH;
  }

  pspUtilityNetconfData data;
  memset(&data, 0, sizeof(data));
  data.base.size = sizeof(data);
  data.base.language = systemLanguage;

  if (configXOButtons == 0)
    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_UNKNOWN, &data.base.buttonSwap);
  else if (configXOButtons == 1)
    data.base.buttonSwap = PSP_UTILITY_ACCEPT_CROSS;
  else
    data.base.buttonSwap = PSP_UTILITY_ACCEPT_CIRCLE;

  data.base.graphicsThread = 17;
  data.base.accessThread = 19;
  data.base.fontThread = 18;
  data.base.soundThread = 16;
  data.action = 3; // Connect to last used network

  struct pspUtilityNetconfAdhoc adhocparam;
  memset(&adhocparam, 0, sizeof(adhocparam));
  data.adhocparam = &adhocparam;

  done = (sceUtilityNetconfInitStart(&data) < 0);

  while (!done)
  {    
    switch (sceUtilityNetconfGetStatus())
    {
      case PSP_UTILITY_DIALOG_NONE:
        break;

      case PSP_UTILITY_DIALOG_INIT:
        graphicDrawBackground(false, true);
        break;

      case PSP_UTILITY_DIALOG_VISIBLE:
        graphicDrawBackground(false, true);
        sceUtilityNetconfUpdate(1);
        break;

      case PSP_UTILITY_DIALOG_QUIT:
        sceUtilityNetconfShutdownStart();
        break;

      case PSP_UTILITY_DIALOG_FINISHED:
        done = true;
        break;
    }

    sceDisplayWaitVblankStart();
    graphicSwitchBuffers();
  }

  g_menuForceRedraw = true;
  g_netDialogActive = false;

  // Restore WLAN clock speed
  powerSetClockSpeed(configWlanClockSpeed);

  // Check for access point connection
  int state;
  sceNetApctlGetState(&state);

  return (state == PSP_NET_APCTL_STATE_GOT_IP);
}




/*
  netLoadModules
  ---------------------------------------------------
  Initialize the wlan subsystem.
  ---------------------------------------------------
  Returns false on error.
*/
bool netLoadModules()
{
  if (sceUtilityLoadModule(PSP_MODULE_NET_COMMON) < 0)
    return false;

  if (sceUtilityLoadModule(PSP_MODULE_NET_INET) < 0)
    return false;

  return true;
}




/*
  netInit
  ---------------------------------------------------
  Initialize the wlan subsystem. Leave modules loaded.
  ---------------------------------------------------
  Returns false on error.
*/
bool netInit()
{
  // Set global variables
  g_netConnected = false;
  g_netSocket = -1;

  if (sceNetInit(0x20000, 0x30, 0x1000, 0x30, 0x1000) < 0)
    return false;

  if (sceNetInetInit() < 0)
    return false;

  if (sceNetApctlInit(0x8000, 0x30) < 0)
    return false;

  l_eventHandlerId = sceNetApctlAddHandler(net_callback, NULL);
  if (l_eventHandlerId < 0)
    return false;

  l_timeoutSema = sceKernelCreateSema("l_timeoutSema", 0, 0, 1, NULL);
  if (l_timeoutSema < 0)
    return false;

  l_watchdogThreadId = sceKernelCreateThread("wlanWatchdogThread", (SceKernelThreadEntry)wlanWatchdogThread, 0x18, 0x10000, PSP_THREAD_ATTR_USER, 0);    
  if (l_watchdogThreadId < 0)
    return false;

  l_watchdogThreadRunning = true;
  if (sceKernelStartThread(l_watchdogThreadId, 0, NULL) < 0)
    return false;

  return true;
}




/*
  netUnloadModules
  ---------------------------------------------------
  Shutdown the wlan subsystem.
  ---------------------------------------------------
  Returns false on error.
*/
bool netUnloadModules()
{
  bool result1 = (sceUtilityUnloadModule(PSP_MODULE_NET_INET) == 0);
  bool result2 = (sceUtilityUnloadModule(PSP_MODULE_NET_COMMON) == 0);

  return (result1 && result2);
}




/*
  netTerm
  ---------------------------------------------------
  Shutdown the wlan subsystem. Leave modules loaded.
  ---------------------------------------------------
  Returns false on error.
*/
bool netTerm()
{
  g_netConnected = false;
  g_netSocket = -1;

  l_watchdogThreadRunning = false;

  if (l_watchdogThreadId > -1)
  {
    sceKernelSignalSema(l_timeoutSema, 1);
    sceKernelWaitThreadEnd(l_watchdogThreadId, NULL);
    sceKernelDeleteThread(l_watchdogThreadId);
  }

  if (l_timeoutSema > -1)
    sceKernelDeleteSema(l_timeoutSema);

  if (l_eventHandlerId > -1)
    sceNetApctlDelHandler(l_eventHandlerId);

  sceNetApctlTerm();
  
  sceNetInetTerm();

  sceNetTerm();

  return true;
}





/*
  netConnect
  ---------------------------------------------------
  Try to connect to the given IP and port.
  Code based on this: 
  http://forums.ps2dev.org/viewtopic.php?p=67436
  ---------------------------------------------------
  Returns the socket handle or a negative code on error.
*/
int netConnect(unsigned long remoteIp, unsigned short remotePort, unsigned int timeout) 
{ 
  struct sockaddr_in Addr; 
  int err; 
  int NoBlock; 
  int ElapsedTime = 0; 

  do 
  { 
    l_tempSocket = sceNetInetSocket(PF_INET, SOCK_STREAM, 0); 
    if(l_tempSocket < 0) 
    {
      // Fatal error
      return NET_CONNECT_ERROR_FATAL; 
    } 

    Addr.sin_family = AF_INET; 
    Addr.sin_port = htons(remotePort); 
    Addr.sin_addr.s_addr = (remoteIp); 

    NoBlock = 0; 
    sceNetInetSetsockopt(l_tempSocket, SOL_SOCKET, SO_NONBLOCK, &NoBlock, sizeof(NoBlock)); 

    // Try to connect in blocking mode. The alarm will close the socket after the timeout so
    // that the connection attempt appears non-blocking.
    SceUID alarmId = sceKernelSetAlarm(100 * 1000, (SceKernelAlarmHandler)netSocketTimeoutAlarm, NULL);
    err = sceNetInetConnect(l_tempSocket, (struct sockaddr *)(&Addr), sizeof(Addr)); 
    sceKernelCancelAlarm(alarmId);

    if (err == 0)
    {
      // Success
      DEBUG_PRINTF("Connection established after %d ms...\n", ElapsedTime);

      // Make socket blocking
      NoBlock = 0; 
      sceNetInetSetsockopt(l_tempSocket, SOL_SOCKET, SO_NONBLOCK, &NoBlock, sizeof(NoBlock)); 

      // Set timeout for the new socket
      struct timeval arg;
      arg.tv_usec = 0;
      arg.tv_sec = 1;
      sceNetInetSetsockopt(l_tempSocket, SOL_SOCKET, SO_RCVTIMEO, &arg, sizeof(arg));
      sceNetInetSetsockopt(l_tempSocket, SOL_SOCKET, SO_SNDTIMEO, &arg, sizeof(arg));

      return l_tempSocket; 
    }
    else
    {
      // 100 ms between connection attempts
      ElapsedTime += 100;
    }
  } 
  while (ElapsedTime < timeout); 
  
  // Return after timeout if no connection could be established
  NoBlock = 0; 
  sceNetInetSetsockopt(l_tempSocket, SOL_SOCKET, SO_NONBLOCK, &NoBlock, sizeof(NoBlock)); 
  sceNetInetClose(l_tempSocket); 
  return NET_CONNECT_ERROR_TIMEOUT; 
} 




/*
  netDisconnect
  ---------------------------------------------------
  Disconnect from the server.
  ---------------------------------------------------
*/
void netDisconnect()
{
  sceNetInetShutdown(g_netSocket, SHUT_RDWR);

  sceNetInetClose(g_netSocket);

  g_netSocket = -1;
  g_netConnected = false;
}




/*
  netResolveName
  ---------------------------------------------------
  Resolve an address through DNS.
  ---------------------------------------------------
  Returns false on error.
*/
bool netResolveName(char *name, in_addr_t *addr)
{
  l_resolverId = -1;
  char buf[1024];

  sceNetResolverInit();

  if(sceNetResolverCreate(&l_resolverId, buf, sizeof(buf)) < 0)
  {
    return false;
  }

  int result = sceNetResolverStartNtoA(l_resolverId, name, (struct in_addr*)addr, 1, 3);
  sceNetResolverDelete(l_resolverId);

  l_resolverId = -1;

  return (result > -1);
}




/*
  netSetNewServerIp
  ---------------------------------------------------
  Determine if the entered string is an IP or
  a name. Resolve name if necessary.
  ---------------------------------------------------
*/
bool netSetNewServerIp(char* newIp, bool doResolve)
{
  l_ipRaw = newIp;

  if (sceNetInetInetAton(newIp, (struct in_addr*)&l_ip))
  {
    // l_ipRaw contains an IP
    strncpy(l_ipResolved, l_ipRaw, NET_MAX_IP_LENGTH);
    l_ipIsResolved = true;
    l_ipRawContainsName = false;

    return true;
  }
  else
  {
    l_ipRawContainsName = true;

    if (doResolve)
    {
      SceUID alarmId = sceKernelSetAlarm(NET_SOCKET_TIMEOUT, (SceKernelAlarmHandler)netSocketTimeoutAlarm, NULL);
      bool resolveSuccessfull = netResolveName(l_ipRaw, &l_ip);
      sceKernelCancelAlarm(alarmId);

      if (resolveSuccessfull)
      {
        // Name could be resolved
        inet_ntop(AF_INET, &l_ip, l_ipResolved, NET_MAX_IP_LENGTH);
        l_ipIsResolved = true;
        return true;
      }
    }

    strcpy(l_ipResolved, "resolving name...");
    l_ipIsResolved = false;

    return false;
  }
}



/*
  netSendWakeOnLanPacket
  ---------------------------------------------------
  Sends a magic packet.
  ---------------------------------------------------
*/
void netSendWakeOnLanPacket()
{
  int i;
  int sock; 
  int result;
  struct sockaddr_in Addr; 
  unsigned char magicPacket[102];
  unsigned int mac_long[6] = { 0, 0, 0, 0, 0, 0 };
  unsigned char mac[6] = { 0, 0, 0, 0, 0, 0 };

  sscanf(configMacAddresses[configIpAddressIndex - 1], "%2x:%2x:%2x:%2x:%2x:%2x", &mac_long[0], &mac_long[1], &mac_long[2], &mac_long[3], &mac_long[4], &mac_long[5]);

  for (i = 0; i < 6; i++)
    mac[i] = (unsigned char)mac_long[i];

  if ((mac_long[0] == 0) && (mac_long[1] == 0) && (mac_long[2] == 0) && (mac_long[3] == 0) && (mac_long[4] == 0) && (mac_long[5] == 0))
	  return;

  sock = sceNetInetSocket(PF_INET, SOCK_DGRAM, 0); 
  if(sock < 0) 
	return;

  // First 6 bytes are 0xFF
  memset(&magicPacket[0], 0xFF, 6);

  // Repeat the MAC address 16 times
  for (i = 0; i < 16; i++)
  {
    memcpy(&magicPacket[(i + 1) * 6], mac, 6);
  }

  Addr.sin_family = AF_INET; 
  Addr.sin_port = htons(9); // Port 9 = discard

  if (configWlanWakeOnLanBroadcasting)
  {
    sceNetInetInetAton("255.255.255.255", (struct in_addr*)&Addr.sin_addr.s_addr);
  }
  else
  {
    if (l_ipIsResolved)
	    Addr.sin_addr.s_addr = l_ip;
	  else
	  {
      sceNetInetClose(sock);
      return;
	  }
  }

  result = sceNetInetConnect(sock, (struct sockaddr *)(&Addr), sizeof(Addr)); 

  if (result < 0)
	return;

  int broadcast = 1; 
  sceNetInetSetsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)); 

  // Send the packet
  sceNetInetSend(sock, magicPacket, 102, 0);

  sceNetInetClose(sock); 
}
