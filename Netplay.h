/****************************************************************************************/
/*  NetPlay.h                                                                           */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: DirectPlay wrapper                                                     */
/*                                                                                      */
/*  The contents of this file are subject to the Genesis3D Public License               */
/*  Version 1.01 (the "License"); you may not use this file except in                   */
/*  compliance with the License. You may obtain a copy of the License at                */
/*  http://www.genesis3d.com                                                            */
/*                                                                                      */
/*  Software distributed under the License is distributed on an "AS IS"                 */
/*  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See                */
/*  the License for the specific language governing rights and limitations              */
/*  under the License.                                                                  */
/*                                                                                      */
/*  The Original Code is Genesis3D, released March 25, 1999.                            */
/*  Genesis3D Version 1.1 released November 15, 1999                                 */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#ifndef GE_NETPLAY_H
#define GE_NETPLAY_H

#include <windows.h>
#include <dplay.h>

#include "BaseType.h"

#ifdef	__cplusplus
extern "C" {
#endif

// ************************************************************************************
//	Defines
// ************************************************************************************
#define NETPLAY_OPEN_CREATE		1
#define NETPLAY_OPEN_JOIN		2


typedef struct
{
	char	Desc[200];								// Description of Service provider
	GUID	Guid;									// Global Service Provider GUID
} SP_DESC;

// must match stuct AFX_SESSION in cengine.h
typedef struct
{
	char	SessionName[200];						// Description of Service provider
	GUID	Guid;									// Global Service Provider GUID
} SESSION_DESC;

extern	SP_DESC					GlobalSP;			// Global info about the sp
extern  SESSION_DESC*			GlobalSession;		// Global sessions availible
extern	LPGUID					glpGuid;
													
void DoDPError(HRESULT Hr);
BOOL InitNetPlay(LPGUID lpGuid);
BOOL NetPlayEnumSession(LPSTR IPAdress, SESSION_DESC** SessionList, DWORD* SessionNum);
BOOL NetPlayJoinSession(SESSION_DESC* SessionList);
BOOL NetPlayCreateSession(LPSTR SessionName, DWORD MaxPlayers);
BOOL NetPlayCreatePlayer(LPDPID lppidID, LPTSTR lptszPlayerName, HANDLE hEvent, LPVOID lpData, DWORD dwDataSize, geBoolean ServerPlayer);
BOOL NetPlayDestroyPlayer(DPID pid);
HRESULT NetPlaySend(DPID idFrom, DPID idTo, DWORD dwFlags, LPVOID lpData, DWORD dwDataSize);
HRESULT NetPlayReceive(LPDPID lpidFrom, LPDPID lpidTo, DWORD dwFlags, LPVOID lpData, LPDWORD lpdwDataSize);
BOOL DeInitNetPlay(void);

// HACK!!!! Function is in Engine.cpp (So NetPlay.C can call it...)
BOOL			AFX_CPrintfC(char *String);

#ifdef	__cplusplus
}
#endif

#endif
