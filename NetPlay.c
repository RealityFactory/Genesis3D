/****************************************************************************************/
/*  NetPlay.c                                                                           */
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
/*Genesis3D Version 1.1 released November 15, 1999                            */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#define IDIRECTPLAY2_OR_GREATER

#include <Windows.H>
#include <Assert.h>

#include <dplay.h>
#include <dplobby.h>
#include <Stdio.h>

#include "netplay.h"
#include "ErrorLog.h"

//#define INIT_GUID

//#define UNICODE   (do not use, not done yet.  Need to fix strings...)

//************************************************************************************
// Misc globals all will need...
//************************************************************************************


LPGUID							glpGuid;

SP_DESC							GlobalSP;				
SESSION_DESC					*GlobalSession;			// Global sessions availible
DWORD							gSessionCnt;
BOOL							FoundSP = FALSE;		// If the provider was found
BOOL							FoundSession = FALSE;

//LPDIRECTPLAY3A					g_lpDP = NULL;
LPDIRECTPLAY4A					g_lpDP = NULL;

BOOL							FoundConnection = FALSE;
LPVOID							lpConnectionBuffer = NULL;
	
// ************************************************************************************
//	Misc global functions
// ************************************************************************************
HRESULT DPlayCreateSession(LPTSTR lptszSessionName, DWORD MaxPlayers);
HRESULT DPlayOpenSession(LPGUID lpSessionGuid);
BOOL WINAPI EnumSession(LPCDPSESSIONDESC2 lpDPSessionDesc, LPDWORD lpdwTimeOut, DWORD dwFlags, 
                        LPVOID lpContext);
HRESULT DPlayEnumSessions(DWORD dwTimeout, LPDPENUMSESSIONSCALLBACK2 lpEnumCallback, 
                          LPVOID lpContext, DWORD dwFlags);
HRESULT DPlayCreatePlayer(LPDPID lppidID, LPTSTR lptszPlayerName, HANDLE hEvent, 
                          LPVOID lpData, DWORD dwDataSize);
HRESULT DPlayDestroyPlayer(DPID pid);
HRESULT DPlayRelease(void);

static HRESULT DPlayCreate(void );

// New dp3 Connection callback 
BOOL FAR PASCAL DPEnumConnectionsCallback(
						LPCGUID			lpguidSP,
						LPVOID			lpConnection,
						DWORD			dwSize,
						LPCDPNAME		lpName,
						DWORD			dwFlags,
						LPVOID			lpContext);

static void DoDPError(HRESULT Hr);

FILE *DebugF;

//========================================================================================================
//	InitNetPlay
//	Enumerate the service providers, and everything else...
//========================================================================================================
BOOL InitNetPlay(LPGUID lpGuid)
{
	HRESULT		Hr;

	glpGuid = lpGuid;
	
	FoundSP = FALSE;

	Hr = DPlayCreate();

	if (Hr != DP_OK)
	{
		DoDPError(Hr);
		return FALSE;
	}
	
	IDirectPlayX_EnumConnections( g_lpDP, glpGuid, DPEnumConnectionsCallback, NULL, 0);

	if (!FoundConnection)
	{
		geErrorLog_AddString(-1, "InitNetPlay:  No connections available.\n", NULL);
		return FALSE;
	}

	return TRUE;
}

//====================================================================================================
//	 NetPlayEnumSession
//====================================================================================================
BOOL NetPlayEnumSession(LPSTR IPAdress, SESSION_DESC **SessionList, DWORD *SessionNum)
{	
	HRESULT		hr;

#if 1
	char					tempBuf[1024];
	DWORD					tempLng = 1024;
	LPDIRECTPLAYLOBBY2A		lpDPL = NULL;

	// Free the old connection buffer
	if(lpConnectionBuffer ) 
	{
		free( lpConnectionBuffer );
		lpConnectionBuffer = NULL;
	}

	hr = CoCreateInstance(	&CLSID_DirectPlayLobby, NULL, CLSCTX_INPROC_SERVER,
							&IID_IDirectPlayLobby3A, (LPVOID *) &lpDPL );

	if (hr != DP_OK)
	{
		DoDPError(hr);
		return( FALSE );
	}

 	hr = IDirectPlayLobby_CreateAddress(lpDPL, &DPSPGUID_TCPIP, &DPAID_INet, (LPVOID)IPAdress, strlen(IPAdress), tempBuf, &tempLng);

	if (hr != DP_OK)
	{
		DoDPError(hr);
		return( FALSE );
	}

	if (lpDPL)
	{
		hr = IDirectPlayLobby_Release(lpDPL);
		
		if (hr != DP_OK)
		{
			DoDPError(hr);
			return( FALSE );
		}
		lpDPL = NULL;
	}

	hr = IDirectPlayX_InitializeConnection( g_lpDP, tempBuf, 0);
#else
	hr = IDirectPlayX_InitializeConnection( g_lpDP, lpConnectionBuffer, 0);
#endif

	if (hr != DP_OK)
	{
		DoDPError(hr);
		return( FALSE );
	}
	
	GlobalSession = NULL;
	gSessionCnt = 0;

	hr = DPlayEnumSessions(0, EnumSession, NULL, 0);
	
	*SessionList = GlobalSession;
	*SessionNum = gSessionCnt;
	
	return( TRUE );

}

//==================================================================================================
//	NetPlayCreateSession
//==================================================================================================
BOOL NetPlayCreateSession(LPSTR SessionName, DWORD MaxPlayers)
{
	HRESULT	Hr;

	assert(g_lpDP);
	assert(lpConnectionBuffer);

	Hr = IDirectPlayX_InitializeConnection(g_lpDP, lpConnectionBuffer, 0);

	if (Hr != DP_OK)
	{
		DoDPError(Hr);
		return FALSE;
	}

	Hr = DPlayCreateSession(SessionName, MaxPlayers);

	if (Hr != DP_OK)
	{
		DoDPError(Hr);
		return FALSE;
	}

	return TRUE;
}

//==================================================================================================
//	NetPlayJoinSession
//==================================================================================================
BOOL NetPlayJoinSession(SESSION_DESC *Session)
{
    HRESULT Hr;
	
	Hr = DPlayOpenSession(&Session->Guid);

	if (Hr != DP_OK)
	{
		DoDPError(Hr);
		return FALSE;
	}

	return TRUE;
}

//==================================================================================================
//	NetPlayCreatePlayer
//	Creates a player for session
//==================================================================================================
BOOL NetPlayCreatePlayer(LPDPID lppidID, LPTSTR lptszPlayerName, HANDLE hEvent, LPVOID lpData, DWORD dwDataSize, geBoolean ServerPlayer)
{
    HRESULT		hr = DPERR_GENERIC;
    DPNAME		name;
	DWORD		Flags;

	assert(g_lpDP);
    
    ZeroMemory(&name,sizeof(name));
    name.dwSize = sizeof(DPNAME);

#ifdef UNICODE
    name.lpszShortName = lptszPlayerName;
#else
    name.lpszShortNameA = lptszPlayerName;
#endif

	Flags = 0;

	//if (ServerPlayer)
	//	Flags |= DPPLAYER_SERVERPLAYER;

	hr = IDirectPlayX_CreatePlayer(g_lpDP, lppidID, &name, hEvent, lpData, dwDataSize, Flags);

	if (hr != DP_OK)
	{
		DoDPError(hr);
		return FALSE;
	}

	return TRUE;
}

//=========================================================================================================
//	NetPlayDestroyPlayer
//=========================================================================================================
BOOL NetPlayDestroyPlayer(DPID pid)
{
	HRESULT Hr = DPlayDestroyPlayer(pid);
	
	if (Hr != DP_OK)
	{
		DoDPError(Hr);
		return FALSE;
	}

	return TRUE;
}

//=========================================================================================================
//	NetPlayReceive
//=========================================================================================================
HRESULT NetPlayReceive(LPDPID lpidFrom, LPDPID lpidTo, DWORD dwFlags, LPVOID lpData, LPDWORD lpdwDataSize)
{
	HRESULT		Hr;
    assert(g_lpDP);

	Hr = IDirectPlayX_Receive(g_lpDP, lpidFrom, lpidTo, dwFlags, lpData, lpdwDataSize);

	if (Hr != DP_OK)
	{
		if (Hr != DPERR_NOMESSAGES)
			DoDPError(Hr);
	}

	return Hr;
}

//=========================================================================================================
//	NetPlaySend
//=========================================================================================================
HRESULT NetPlaySend(DPID idFrom, DPID idTo, DWORD dwFlags, LPVOID lpData, DWORD dwDataSize)
{
	HRESULT		Hr;

	assert(g_lpDP);

#if 0
	dwFlags |= DPSEND_ASYNC;
    Hr = IDirectPlayX_SendEx(g_lpDP, idFrom, idTo, dwFlags, lpData, dwDataSize, 0, 0, NULL, NULL);
#else
	Hr = IDirectPlayX_Send(g_lpDP, idFrom, idTo, dwFlags, lpData, dwDataSize);
#endif
	
	if (Hr != DP_OK)
	{
		if (Hr == DPERR_PENDING)
			return DP_OK;

		DoDPError(Hr);
	}

	return Hr;
}

//=========================================================================================================
//	DeInitNetPlay
//=========================================================================================================
BOOL DeInitNetPlay(void)
{
	HRESULT Hr;

	if (lpConnectionBuffer)
	{
		free(lpConnectionBuffer);
		lpConnectionBuffer = NULL;
	}

	FoundConnection = FALSE;
	FoundSP = FALSE;
	
	Hr = DPlayRelease();
	
	if (Hr != DP_OK)
	{
		DoDPError(Hr);
		return FALSE;
	}

	return TRUE;
}

//====================================================================================================
//	NetPlayGetNumMessages
//====================================================================================================
geBoolean NetPlayGetNumMessages(int32 *NumMsgSend, int32 *NumBytesSend, int32 *NumMsgRec, int32 *NumBytesRec)
{
	HRESULT		Hr;
	DPID		IdFrom, IdTo;

	IdFrom = IdTo = 0;

	Hr = IDirectPlayX_GetMessageQueue(g_lpDP, IdFrom, IdTo, DPMESSAGEQUEUE_SEND, NumMsgSend, NumBytesSend);

	if (Hr != DP_OK)
	{
		geErrorLog_AddString(-1, "NetPlayGetNumMessages:  IDirectPlayX_GetMessageQueue failed.\n", NULL);
		DoDPError(Hr);
		return GE_FALSE;
	}

	Hr = IDirectPlayX_GetMessageQueue(g_lpDP, IdFrom, IdTo, DPMESSAGEQUEUE_RECEIVE, NumMsgRec, NumBytesRec);

	if (Hr != DP_OK)
	{
		geErrorLog_AddString(-1, "NetPlayGetNumMessages:  IDirectPlayX_GetMessageQueue failed.\n", NULL);
		DoDPError(Hr);
		return GE_FALSE;
	}

	return GE_TRUE;
}

// ************************************************************************************
// WRAPPER Code sarts here...
// ************************************************************************************
//========================================================================================
//	DPlayCreate
//========================================================================================
HRESULT DPlayCreate( void )
{
    HRESULT			hr=E_FAIL;
    LPDIRECTPLAY	lpDP=NULL;

	CoInitialize( NULL );

	hr = CoCreateInstance(	&CLSID_DirectPlay, NULL, CLSCTX_INPROC_SERVER,
							&IID_IDirectPlay4A, (LPVOID *) &g_lpDP );
	//hr = CoCreateInstance(	&CLSID_DirectPlay, NULL, CLSCTX_INPROC_SERVER,
	//						&IID_IDirectPlay3A, (LPVOID *) &g_lpDP );

    return hr;
}

//========================================================================================
//	DPlayCreateSession
//========================================================================================
HRESULT DPlayCreateSession(LPTSTR lptszSessionName, DWORD MaxPlayers)
{
    HRESULT hr = E_FAIL;
    DPSESSIONDESC2 dpDesc;

	assert(g_lpDP);

    ZeroMemory(&dpDesc, sizeof(dpDesc));
    dpDesc.dwSize = sizeof(dpDesc);
    
	dpDesc.dwFlags = DPSESSION_KEEPALIVE;

#if 0		// Just keeping these here for reference...
	dpDesc.dwFlags |= DPSESSION_CLIENTSERVER;
    dpDesc.dwFlags |= DPSESSION_MIGRATEHOST;
	dpDesc.dwFlags |= DPSESSION_OPTIMIZELATENCY;
	dpDesc.dwFlags |= DPSESSION_DIRECTPLAYPROTOCOL;
#endif

	dpDesc.dwMaxPlayers = MaxPlayers;

#ifdef UNICODE
    dpDesc.lpszSessionName = lptszSessionName;
#else
    dpDesc.lpszSessionNameA = lptszSessionName;
#endif

    // set the application guid
    if (glpGuid)
        dpDesc.guidApplication = *glpGuid;

	hr = IDirectPlayX_Open(g_lpDP, &dpDesc, DPOPEN_CREATE);

	if (hr != DP_OK)
	{
		DoDPError(hr);
	}

    return hr;
}

//========================================================================================================
//	DPlayOpenSession
//========================================================================================================
HRESULT DPlayOpenSession(LPGUID lpSessionGuid)
{
    HRESULT hr = E_FAIL;
    DPSESSIONDESC2 dpDesc;

	assert(g_lpDP);

    ZeroMemory(&dpDesc, sizeof(dpDesc));
    dpDesc.dwSize = sizeof(dpDesc);

	//dpDesc.dwFlags = DPSESSION_DIRECTPLAYPROTOCOL;

    // set the session guid
    if (lpSessionGuid)
        dpDesc.guidInstance = *lpSessionGuid;

    // Set the application guid
    if (glpGuid)
        dpDesc.guidApplication = *glpGuid;

    // open it
	hr = IDirectPlayX_Open(g_lpDP, &dpDesc, DPOPEN_JOIN);

    return hr;
}

//========================================================================================================
//	DPlayEnumSessions
//========================================================================================================
HRESULT DPlayEnumSessions(DWORD dwTimeout, LPDPENUMSESSIONSCALLBACK2 lpEnumCallback, 
                          LPVOID lpContext, DWORD dwFlags)
{
    HRESULT hr = E_FAIL;
    DPSESSIONDESC2 dpDesc;

    ZeroMemory(&dpDesc, sizeof(dpDesc));
    dpDesc.dwSize = sizeof(dpDesc);

    if (glpGuid)
        dpDesc.guidApplication = *glpGuid;

    if (g_lpDP)
        hr = IDirectPlayX_EnumSessions(g_lpDP, &dpDesc, dwTimeout, lpEnumCallback,
                                        lpContext, dwFlags);
    return hr;
}

//========================================================================================================
//	Callback for enum session
//========================================================================================================
BOOL WINAPI EnumSession(LPCDPSESSIONDESC2 lpDPSessionDesc, LPDWORD lpdwTimeOut, DWORD dwFlags, 
                        LPVOID lpContext)
{
    HWND hWnd = (HWND) lpContext;
	LPSTR Str = NULL;

    if(dwFlags & DPESC_TIMEDOUT) 
		return FALSE;       // don't try again

	gSessionCnt++;

	if( GlobalSession )
		GlobalSession = realloc( GlobalSession, gSessionCnt * sizeof(SESSION_DESC));
	else
		GlobalSession = malloc( sizeof( SESSION_DESC ) );

	GlobalSession[gSessionCnt-1].Guid = lpDPSessionDesc->guidInstance;

#ifdef UNICODE
	strcpy(GlobalSession[gSessionCnt-1].SessionName, lpDPSessionDesc->lpszSessionName);
#else
	strcpy(GlobalSession[gSessionCnt-1].SessionName, lpDPSessionDesc->lpszSessionNameA);
#endif

    return(TRUE);
}

//====================================================================================================
//	DPlayCreatePlayer
//====================================================================================================
HRESULT DPlayCreatePlayer(LPDPID lppidID, LPTSTR lptszPlayerName, HANDLE hEvent, 
                          LPVOID lpData, DWORD dwDataSize)
{
    HRESULT		hr = DPERR_GENERIC;
    DPNAME		name;

	assert(g_lpDP);
    
    ZeroMemory(&name,sizeof(name));
    name.dwSize = sizeof(DPNAME);

#ifdef UNICODE
    name.lpszShortName = lptszPlayerName;
#else
    name.lpszShortNameA = lptszPlayerName;
#endif

	hr = IDirectPlayX_CreatePlayer(g_lpDP, lppidID, &name, hEvent, lpData, dwDataSize, DPPLAYER_SERVERPLAYER);
                                    
    return hr;
}

//====================================================================================================
//	DPlayDestroyPlayer
//====================================================================================================
HRESULT DPlayDestroyPlayer(DPID pid)
{
	HRESULT hr=E_FAIL;

	assert(g_lpDP);
	
	hr = IDirectPlayX_DestroyPlayer(g_lpDP, pid);

	return hr;
}

//====================================================================================================
//	DPlayRelease
//====================================================================================================
HRESULT DPlayRelease(void)
{
    HRESULT hr = DP_OK;

	if (g_lpDP)
	{
		IDirectPlayX_Close(g_lpDP );

		hr = IDirectPlayX_Release(g_lpDP);
		g_lpDP = NULL;
	}
	
	CoUninitialize();

    return hr;
}

//====================================================================================================
//	DPEnumConnectionsCallback
//====================================================================================================
BOOL FAR PASCAL DPEnumConnectionsCallback(
						LPCGUID			lpguidSP,
						LPVOID			lpConnection,
						DWORD			dwSize,
						LPCDPNAME		lpName,
						DWORD			dwFlags,
						LPVOID			lpContext)
{
	LPSTR Str;
	
	if (FoundConnection)
		return TRUE;

	Str = lpName->lpszShortNameA;

	// Loop through and try to see if this is the service provider we want (TCP/IP for now...)
	while (strlen(Str) > 0)
	{
		if (!strnicmp(Str, "TCP", 3))
		//if (!strnicmp(Str, "Serial", 6))
		{
			// Make sure it's deleted
			if (lpConnectionBuffer)
			{
				free(lpConnectionBuffer);
				lpConnectionBuffer = NULL;
			}
	
			// make space for Connection Shortcut
			lpConnectionBuffer = (char*)malloc(dwSize);
			if (lpConnectionBuffer == NULL)
				goto FAILURE;

			memcpy(lpConnectionBuffer, lpConnection, dwSize);
			FoundConnection = TRUE;
			break;
		}
		Str++;
	}

FAILURE:
    return (TRUE);
}


//====================================================================================================
//	DoDPError
//====================================================================================================
static void DoDPError(HRESULT Hr)
{
	switch (Hr)
	{
	case CLASS_E_NOAGGREGATION:
		geErrorLog_AddString(-1, "A non-NULL value was passed for the pUnkOuter parameter in DirectPlayCreate, DirectPlayLobbyCreate, or IDirectPlayLobby2::Connect.\n", NULL);
		break;

	case DP_OK:
		geErrorLog_AddString(-1, "The request completed successfully.\n", NULL);
		break;

	case DPERR_ACCESSDENIED:
		geErrorLog_AddString(-1, "The session is full or an incorrect password was supplied.\n", NULL);
		break;

	case DPERR_ACTIVEPLAYERS:
		geErrorLog_AddString(-1, "The requested operation cannot be performed because there are existing active players.\n", NULL); 
		break;

	case DPERR_ALREADYINITIALIZED:
		geErrorLog_AddString(-1, "This object is already initialized. \n", NULL);
		break;

	case DPERR_APPNOTSTARTED:
		geErrorLog_AddString(-1, "The application has not been started yet.\n", NULL); 
		break;

	case DPERR_AUTHENTICATIONFAILED:
		geErrorLog_AddString(-1, "The password or credentials supplied could not be authenticated. \n", NULL);
		break;

	case DPERR_BUFFERTOOLARGE:
		geErrorLog_AddString(-1, "The data buffer is too large to store. \n", NULL);
		break;

	case DPERR_BUSY:
		geErrorLog_AddString(-1, "A message cannot be sent because the transmission medium is busy. \n", NULL);
		break;

	case DPERR_BUFFERTOOSMALL:
		geErrorLog_AddString(-1, "The supplied buffer is not large enough to contain the requested data. \n", NULL);
		break;

	case DPERR_CANTADDPLAYER:
		geErrorLog_AddString(-1, "The player cannot be added to the session. \n", NULL);
		break;

	case DPERR_CANTCREATEGROUP:
		geErrorLog_AddString(-1, "A new group cannot be created. \n", NULL);
		break;

	case DPERR_CANTCREATEPLAYER:
		geErrorLog_AddString(-1, "A new player cannot be created. \n", NULL);
		break;

	case DPERR_CANTCREATEPROCESS:
		geErrorLog_AddString(-1, "Cannot start the application. \n", NULL);
		break;

	case DPERR_CANTCREATESESSION:
		geErrorLog_AddString(-1, "A new session cannot be created. \n", NULL);
		break;

	case DPERR_CANTLOADCAPI:
		geErrorLog_AddString(-1, "No credentials were supplied and the CryptoAPI package (CAPI) to use for cryptography services cannot be loaded. \n", NULL);
		break;

	case DPERR_CANTLOADSECURITYPACKAGE:
		geErrorLog_AddString(-1, "The software security package cannot be loaded. \n", NULL);
		break;

	case DPERR_CANTLOADSSPI:
		geErrorLog_AddString(-1, "No credentials were supplied and the software security package (SSPI) that will prompt for credentials cannot be loaded. \n", NULL);
		break;

	case DPERR_CAPSNOTAVAILABLEYET:
		geErrorLog_AddString(-1, "The capabilities of the DirectPlay object have not been determined yet. This error will occur if the DirectPlay object is implemented on a connectivity solution that requires polling to determine available bandwidth and latency. \n", NULL);
		break;

	case DPERR_CONNECTING:
		geErrorLog_AddString(-1, "The method is in the process of connecting to the network. The application should keep calling the method until it returns DP_OK, indicating successful completion, or it returns a different error. \n", NULL);
		break;

	case DPERR_ENCRYPTIONFAILED:
		geErrorLog_AddString(-1, "The requested information could not be digitally encrypted. Encryption is used for message privacy. This error is only relevant in a secure session. \n", NULL);
		break;

	case DPERR_EXCEPTION:
		geErrorLog_AddString(-1, "An exception occurred when processing the request. \n", NULL);
		break;

	case DPERR_GENERIC:
		geErrorLog_AddString(-1, "An undefined error condition occurred. \n", NULL);
		break;

	case DPERR_INVALIDFLAGS:
		geErrorLog_AddString(-1, "The flags passed to this method are invalid. \n", NULL);
		break;

	case DPERR_INVALIDGROUP:
		geErrorLog_AddString(-1, "The group ID is not recognized as a valid group ID for this game session. \n", NULL);
		break;

	case DPERR_INVALIDINTERFACE:
		geErrorLog_AddString(-1, "The interface parameter is invalid. \n", NULL);
		break;

	case DPERR_INVALIDOBJECT:
		geErrorLog_AddString(-1, "The DirectPlay object pointer is invalid. \n", NULL);
		break;

	case DPERR_INVALIDPARAMS: 
		geErrorLog_AddString(-1, "One or more of the parameters passed to the method are invalid. \n", NULL);
		break;

	case DPERR_INVALIDPASSWORD: 
		geErrorLog_AddString(-1, "An invalid password was supplied when attempting to join a session that requires a password. \n", NULL);
		break;

	case DPERR_INVALIDPLAYER: 
		geErrorLog_AddString(-1, "The player ID is not recognized as a valid player ID for this game session. \n", NULL);
		break;
	
	case DPERR_LOGONDENIED: 
		geErrorLog_AddString(-1, "The session could not be opened because credentials are required and either no credentials were supplied or the credentials were invalid. \n", NULL);
		break;

	case DPERR_NOCAPS:
		geErrorLog_AddString(-1, "The communication link that DirectPlay is attempting to use is not capable of this function. \n", NULL);
		break;

	case DPERR_NOCONNECTION: 
		geErrorLog_AddString(-1, "No communication link was established. \n", NULL);
		break;

	case DPERR_NOINTERFACE: 
		geErrorLog_AddString(-1, "The interface is not supported. \n", NULL);
		break;

	case DPERR_NOMESSAGES:
		geErrorLog_AddString(-1, "There are no messages in the receive queue. \n", NULL);
		break;

	case DPERR_NONAMESERVERFOUND:
		geErrorLog_AddString(-1, "No name server (host) could be found or created. A host must exist to create a player. \n", NULL);
		break;

	case DPERR_NONEWPLAYERS: 
		geErrorLog_AddString(-1, "The session is not accepting any new players. \n", NULL);
		break;

	case DPERR_NOPLAYERS: 
		geErrorLog_AddString(-1, "There are no active players in the session. \n", NULL);
		break;

	case DPERR_NOSESSIONS: 
		geErrorLog_AddString(-1, "There are no existing sessions for this game. \n", NULL);
		break;

	case DPERR_NOTLOBBIED: 
		geErrorLog_AddString(-1, "Returned by the IDirectPlayLobby2::Connect method if the application was not started by using the IDirectPlayLobby2::RunApplication method or if there is no DPLCONNECTION structure currently initialized for this DirectPlayLobby object. \n", NULL);
		break;

	case DPERR_NOTLOGGEDIN: 
		geErrorLog_AddString(-1, "An action cannot be performed because a player or client application is not logged in. Returned by the IDirectPlay3::Send method when the client application tries to send a secure message without being logged in. \n", NULL);
		break;

	case DPERR_OUTOFMEMORY: 
		geErrorLog_AddString(-1, "There is insufficient memory to perform the requested operation. \n", NULL);
		break;

	case DPERR_PLAYERLOST:
		geErrorLog_AddString(-1, "A player has lost the connection to the session. \n", NULL);
		break;

	case DPERR_SENDTOOBIG: 
		geErrorLog_AddString(-1, "The message being sent by the IDirectPlay3::Send method is too large. \n", NULL);
		break;

	case DPERR_SESSIONLOST: 
		geErrorLog_AddString(-1, "The connection to the session has been lost. \n", NULL);
		break;

	case DPERR_SIGNFAILED: 
		geErrorLog_AddString(-1, "The requested information could not be digitally signed. Digital signatures are used to establish the authenticity of messages. \n", NULL);
		break;

	case DPERR_TIMEOUT: 
		geErrorLog_AddString(-1, "The operation could not be completed in the specified time. \n", NULL);
		break;

	case DPERR_UNAVAILABLE: 
		geErrorLog_AddString(-1, "The requested function is not available at this time. \n", NULL);
		break;

	case DPERR_UNINITIALIZED: 
		geErrorLog_AddString(-1, "The requested object has not been initialized. \n", NULL);
		break;

	case DPERR_UNKNOWNAPPLICATION: 
		geErrorLog_AddString(-1, "An unknown application was specified. \n", NULL);
		break;

	case DPERR_UNSUPPORTED:
		geErrorLog_AddString(-1, "The function is not available in this implementation. Returned from IDirectPlay3::GetGroupConnectionSettings and IDirectPlay3::SetGroupConnectionSettings if they are called from a session that is not a lobby session. \n", NULL);
		break;

	case DPERR_USERCANCEL: 
		geErrorLog_AddString(-1, "Can be returned in two ways. 1) The user canceled the connection process during a call to the IDirectPlay3::Open method. 2) The user clicked Cancel in one of the DirectPlay service provider dialog boxes during a call to IDirectPlay3::EnumSessions. \n", NULL);
		break;

	default:
		geErrorLog_AddString(-1, "NetPlayError:  Don't know this one...\n", NULL);
		break;
	}
}
