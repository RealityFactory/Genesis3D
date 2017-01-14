/****************************************************************************************/
/*  CSNetMgr.c                                                                          */
/*                                                                                      */
/*  Author: John Pollard/Brian Adelberg                                                 */
/*  Description: Client/Server network code                                             */
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
#include <assert.h>

//#define INITGUID
#include <Windows.H>
#include <objbase.h>

#include "CSNetMgr.h"
#include "NetPlay.h"

#include "BaseType.h"
#include "Ram.h"
#include "ErrorLog.h"

#include <InitGuid.h>

#pragma message(" some assertions in here would be nice:")

#define PACKET_HEADER_SIZE				1

#define NET_TIMEOUT						15000		// Givem 15 secs


// {33925241-05F8-11d0-8063-00A0C90AE891}
DEFINE_GUID(GENESIS_GUID, 
//0x33925241, 0x5f8, 0x11d0, 0x80, 0x63, 0x0, 0xa0, 0xc9, 0xa, 0xe8, 0x91);
0x33925241, 0x0, 0x11d0, 0x80, 0x63, 0x0, 0xa0, 0xc9, 0xa, 0xe8, 0x91);


static  geBoolean	NetSession = GE_FALSE;
static	geBoolean	WeAreTheServer = GE_FALSE;
static  DPID		OurPlayerId;
static	DPID		ServerId;					// The servers Id we are connected too

#define BUFFER_SIZE			20000
//#define BUFFER_SIZE			65535

static uint8				Packet[BUFFER_SIZE];

static geCSNetMgr_NetClient	gClient;

static BOOL geCSNetMgr_ProcessSystemMessage(geCSNetMgr *M,geCSNetMgr_NetID IdTo, LPDPMSG_GENERIC lpMsg, geCSNetMgr_NetMsgType *Type, geCSNetMgr_NetClient *Client);

typedef struct geCSNetMgr
{
	// instance data goes here
	geCSNetMgr *Valid;
} geCSNetMgr;


geBoolean geCSNetMgr_IsValid(geCSNetMgr *M)
{
	if ( M == NULL )
		return GE_FALSE;

	if ( M->Valid != M )
		return GE_FALSE;

	return GE_TRUE;
}

GENESISAPI geCSNetMgr * GENESISCC geCSNetMgr_Create(void)
{
	geCSNetMgr *M;

	M = GE_RAM_ALLOCATE_STRUCT(geCSNetMgr);
	
	if ( M == NULL)
	{
		geErrorLog_Add(-1, NULL); //FIXME
		return NULL;
	}

	M->Valid = M;

	return M;
}

	
GENESISAPI void GENESISCC geCSNetMgr_Destroy(geCSNetMgr **ppM)
{
	assert( ppM != NULL );
	assert( geCSNetMgr_IsValid(*ppM)!=GE_FALSE );
	
	(*ppM) -> Valid = 0;
	geRam_Free(*ppM);
	*ppM = NULL;	
};

//================================================================================
//	geCSNetMgr_ReceiveFromServer
//================================================================================
GENESISAPI geBoolean GENESISCC geCSNetMgr_ReceiveFromServer(geCSNetMgr *M, geCSNetMgr_NetMsgType *Type, int32 *Size, uint8 **Data)
{
	DPID				IdTo;
	DWORD				BSize = BUFFER_SIZE;
	HRESULT				Result;

	*Size = 0;
	*Data = NULL;
	*Type = NET_MSG_NONE;
	
	assert( geCSNetMgr_IsValid(M)!=GE_FALSE );

	if (!NetSession)
	{
		geErrorLog_AddString(-1, "geCSNetMgr_ReceiveFromServer:  No net session.\n", NULL);
		return GE_FALSE;
	}

#if 1
	// Empty out all the system msg's first
	if (!geCSNetMgr_ReceiveSystemMessage(M, OurPlayerId, Type, &gClient))
	{
		geErrorLog_AddString(-1, "geCSNetMgr_ReceiveFromServer:  geCSNetMgr_ReceiveSystemMessage failed.\n", NULL);
		return GE_FALSE;
	}

	if (*Type != NET_MSG_NONE)
	{
		*Size = sizeof( geCSNetMgr_NetClient );
		*Data = (uint8*)&gClient;
		return( GE_TRUE );
	}
#endif

	// Find the server player
	IdTo = 0;

	Result = NetPlayReceive(&ServerId, &IdTo, DPRECEIVE_FROMPLAYER, (uint8*)&Packet, &BSize);

	if (Result != DP_OK)
	{
		if (Result == DPERR_NOMESSAGES)
			return GE_TRUE;		// Not an error, there is simply no msg's. (*Size will == 0, so they will know)

		geErrorLog_AddString(-1, "geCSNetMgr_ReceiveFromServer:  NetPlayReceive failed.\n", NULL);
		return GE_FALSE;
	}

	if (BSize > 0)
	{
		*Type = Packet[0];
		*Size = BSize - PACKET_HEADER_SIZE;
		*Data = (uint8*)&Packet[1];
	}

    return GE_TRUE;
}

//================================================================================
//	geCSNetMgr_ReceiveFromClient
//================================================================================
GENESISAPI geBoolean GENESISCC geCSNetMgr_ReceiveFromClient(geCSNetMgr *M, geCSNetMgr_NetMsgType *Type, geCSNetMgr_NetID *IdClient, int32 *Size, uint8 **Data)
{
	DPID				IdTo;
	DWORD				BSize = BUFFER_SIZE;
	HRESULT				Result;

	*Size = 0;
	*Data = NULL;
	*Type = NET_MSG_NONE;

	assert( geCSNetMgr_IsValid(M)!=GE_FALSE );
	
	if (!NetSession)
		return GE_FALSE;
        
	// Empty out all the system msg's first
	if (!geCSNetMgr_ReceiveSystemMessage(M, ServerId, Type, &gClient))
		return GE_FALSE;

	if (*Type != NET_MSG_NONE)
	{
		*Size = sizeof( geCSNetMgr_NetClient );
		*Data = (uint8*)&gClient;
		return( GE_TRUE );
	}

	// Find the client player
	IdTo = ServerId;

    Result = NetPlayReceive((DPID*)IdClient, &IdTo, DPRECEIVE_TOPLAYER, (uint8*)&Packet, &BSize);

	if (Result != DP_OK)
	{
		if (Result == DPERR_NOMESSAGES)
			return GE_TRUE;		// Not an error, there is simply no msg's. (*Size will == 0, so they will know)

		return GE_FALSE;
	}

	if (BSize > 0)
	{
		*Type = Packet[0];
		*Size = BSize - PACKET_HEADER_SIZE;
		*Data = (uint8*)&Packet[1];
	}

    return GE_TRUE;	
}

//================================================================================
//	geCSNetMgr_ReceiveSystemMessage
//================================================================================
GENESISAPI geBoolean GENESISCC geCSNetMgr_ReceiveSystemMessage(geCSNetMgr *M, geCSNetMgr_NetID IdFor, geCSNetMgr_NetMsgType *Type, geCSNetMgr_NetClient *Client)
{
	DPID				SystemId, IdTo;
	DWORD				BSize = BUFFER_SIZE;
	HRESULT				Result;

	assert( geCSNetMgr_IsValid(M)!=GE_FALSE );

	*Type = NET_MSG_NONE;

	if (!NetSession)
		return GE_FALSE;

	// Find system messages
	SystemId = DPID_SYSMSG;
	IdTo = IdFor;

	Result = NetPlayReceive(&SystemId,&IdTo, DPRECEIVE_FROMPLAYER | DPRECEIVE_TOPLAYER, &Packet, &BSize);

	if (Result != DP_OK)
	{
		if (Result == DPERR_NOMESSAGES)
			return GE_TRUE;		// Not an error, there is simply no msg's. (*Size will == 0, so they will know)

		return GE_FALSE;
	}

	if (BSize > 0)
	{
		if (!geCSNetMgr_ProcessSystemMessage(M, IdFor, (LPDPMSG_GENERIC)&Packet, Type, Client))
			return GE_FALSE;
	}

    return GE_TRUE;
}

//================================================================================
//	geCSNetMgr_ProcessSystemMessage
//================================================================================
static geBoolean geCSNetMgr_ProcessSystemMessage(geCSNetMgr *M, geCSNetMgr_NetID IdTo, LPDPMSG_GENERIC lpMsg, geCSNetMgr_NetMsgType *Type, geCSNetMgr_NetClient *Client)
{
	DWORD dwSize;

	assert( geCSNetMgr_IsValid(M)!=GE_FALSE );

	*Type = NET_MSG_NONE;

	switch( lpMsg->dwType)
    {
		case DPSYS_CREATEPLAYERORGROUP:
        {
			if(WeAreTheServer && IdTo == ServerId)
            {
				LPDPMSG_CREATEPLAYERORGROUP lpAddMsg = (LPDPMSG_CREATEPLAYERORGROUP) lpMsg;
				
				// Don't broadcast the server being created...
				if( lpAddMsg->dpId == ServerId )
					return GE_TRUE;

				// Name the player, and return it as a NET_MSG_CREATE_CLIENT message
				*Type = NET_MSG_CREATE_CLIENT;

				strncpy(Client->Name, lpAddMsg->dpnName.lpszShortNameA, MAX_CLIENT_NAME);
				Client->Id = lpAddMsg->dpId;
				
				// The client is waiting for this message, so send it now.
				// It contains our ServerId, which the client needs...
				dwSize = sizeof( ServerId ) + PACKET_HEADER_SIZE;

				assert(dwSize < BUFFER_SIZE);

				Packet[0] = NET_MSG_SERVER_ID;
				memcpy( &Packet[1], &ServerId, sizeof( ServerId ) );

				// Fire it off...
				if (NetPlaySend( ServerId, Client->Id, DPSEND_GUARANTEED, (void*)&Packet, dwSize ) != DP_OK)
					return GE_FALSE;
            }

			break;
        }

		case DPSYS_DESTROYPLAYERORGROUP:
        {
            if (WeAreTheServer && IdTo == ServerId)
			{
				LPDPMSG_DESTROYPLAYERORGROUP lpDestroyMsg = (LPDPMSG_DESTROYPLAYERORGROUP)lpMsg;

				Client->Id = lpDestroyMsg->dpId;
				*Type = NET_MSG_DESTROY_CLIENT;
			}
			
			break;
        }

		case DPSYS_HOST:
        {           
            WeAreTheServer = GE_TRUE;
			*Type = NET_MSG_HOST;
			break;
        }

		case DPSYS_SESSIONLOST:
		{
			*Type = NET_MSG_SESSIONLOST;
			break;
		}

    }

	return GE_TRUE;
}

//================================================================================
//	geCSNetMgr_ReceiveAllMessages
//================================================================================
GENESISAPI geBoolean GENESISCC geCSNetMgr_ReceiveAllMessages(geCSNetMgr *M, geCSNetMgr_NetID *IdFrom, geCSNetMgr_NetID *IdTo, geCSNetMgr_NetMsgType *Type, int32 *Size, uint8 **Data)
{
	DWORD				BSize = BUFFER_SIZE;
	HRESULT				Result;

	assert( geCSNetMgr_IsValid(M)!=GE_FALSE );

	*Size = 0;
	*Data = NULL;
	*Type = NET_MSG_NONE;

	if (!NetSession)
		return GE_FALSE;
        
    *IdFrom = 0;
	*IdTo = 0;
	//*IdTo = ServerId;

	Result = NetPlayReceive((DPID*)IdFrom, (DPID*)IdTo, DPRECEIVE_ALL, &Packet, &BSize);
	//Result = NetPlayReceive((DPID*)IdFrom, (DPID*)IdTo, DPRECEIVE_TOPLAYER, &Packet, &BSize);

	if (*IdTo != ServerId)
		return GE_TRUE;

	if (Result != DP_OK)
	{
		if (Result == DPERR_NOMESSAGES)
			return GE_TRUE;		// Not an error, there is simply no msg's. (*Size will == 0, so they will know)

		return GE_FALSE;
	}

	if (BSize > 0)
	{
		if (*IdFrom == DPID_SYSMSG)
		{
			// IdTo used to be DPID_ALLPLAYERS...
			// Had to change to IdTo so it would work correctly.
			if (!geCSNetMgr_ProcessSystemMessage(M, *IdTo, (LPDPMSG_GENERIC)&Packet, Type, &gClient))
				return GE_FALSE;

			if (*Type != NET_MSG_NONE)
			{
				*Size = sizeof( geCSNetMgr_NetClient );
				*Data = (uint8*)&gClient;
			}
		}
		else
		{	
			*Type = Packet[0];
			*Size = BSize - PACKET_HEADER_SIZE;
			*Data = (uint8*)&Packet[1];
			return GE_TRUE;
		}
	}

    return GE_TRUE;
}


//================================================================================
//	geCSNetMgr_GetServerID
//================================================================================
GENESISAPI geCSNetMgr_NetID GENESISCC geCSNetMgr_GetServerID(geCSNetMgr *M)
{
	assert( geCSNetMgr_IsValid(M)!=GE_FALSE );
	return(ServerId);
}

//================================================================================
//	geCSNetMgr_GetOurID
//================================================================================
GENESISAPI geCSNetMgr_NetID GENESISCC geCSNetMgr_GetOurID(geCSNetMgr *M)
{
	assert( geCSNetMgr_IsValid(M)!=GE_FALSE );
	return(OurPlayerId);
}

//================================================================================
//	geCSNetMgr_GetAllPlayerID
//================================================================================
GENESISAPI geCSNetMgr_NetID GENESISCC geCSNetMgr_GetAllPlayerID(geCSNetMgr *M)
{
	assert( geCSNetMgr_IsValid(M)!=GE_FALSE );
	return(DPID_ALLPLAYERS);
}

//================================================================================
//	geCSNetMgr_WeAreTheServer
//================================================================================
GENESISAPI geBoolean GENESISCC geCSNetMgr_WeAreTheServer(geCSNetMgr *M)
{
	assert( geCSNetMgr_IsValid(M)!=GE_FALSE );
	return (WeAreTheServer);
}

//================================================================================
//	geCSNetMgr_StartSession
//================================================================================
GENESISAPI geBoolean GENESISCC geCSNetMgr_StartSession(geCSNetMgr *M, const char *SessionName, const char *PlayerName)
{
	char		Name2[MAX_CLIENT_NAME];

	assert( geCSNetMgr_IsValid(M)!=GE_FALSE );
	assert(PlayerName);

	NetSession = GE_FALSE;
	WeAreTheServer = GE_FALSE;

	if (!InitNetPlay((LPGUID)&GENESIS_GUID))
		return GE_FALSE;

	if (!NetPlayCreateSession((char*)SessionName, 16))
		return GE_FALSE;
	
	if (!NetPlayCreatePlayer(&ServerId, "Server", NULL, NULL, 0, GE_TRUE))
	{
		geErrorLog_AddString(-1, "geCSNetMgr_StartSession:  NetPlayCreatePlayer failed for server player.\n", NULL);
		return GE_FALSE;
	}

	strncpy(Name2, PlayerName, MAX_CLIENT_NAME);

	if (!NetPlayCreatePlayer(&OurPlayerId, Name2, NULL, NULL, 0, GE_FALSE))
	{
		geErrorLog_AddString(-1, "geCSNetMgr_StartSession:  NetPlayCreatePlayer failed for client player.\n", NULL);
		return GE_FALSE;
	}

	WeAreTheServer = GE_TRUE;
	NetSession = GE_TRUE;

	return GE_TRUE;
} 	

//================================================================================
//	geCSNetMgr_FindSession
//================================================================================
GENESISAPI geBoolean GENESISCC geCSNetMgr_FindSession(geCSNetMgr *M, const char *IPAdress, geCSNetMgr_NetSession **SessionList, int32 *SessionNum)
{
	assert( geCSNetMgr_IsValid(M)!=GE_FALSE );

	NetSession = GE_FALSE;
	WeAreTheServer = GE_FALSE;

	if (!InitNetPlay((LPGUID)&GENESIS_GUID))
		return GE_FALSE;

	if(!NetPlayEnumSession((char*)IPAdress, (SESSION_DESC**)SessionList, SessionNum) )
		return GE_FALSE;

	return( GE_TRUE );
}

//================================================================================
//	geCSNetMgr_JoinSession
//================================================================================
GENESISAPI geBoolean GENESISCC geCSNetMgr_JoinSession(geCSNetMgr *M, const char *Name, const geCSNetMgr_NetSession* Session)
{

	uint32	StartTime;
	DWORD	BSize = BUFFER_SIZE;

	WeAreTheServer = GE_FALSE;
	NetSession = GE_FALSE;

	assert( geCSNetMgr_IsValid(M)!=GE_FALSE );

	if (!NetPlayJoinSession( (SESSION_DESC*)Session) )
	{
		geErrorLog_AddString(-1, "geCSNetMgr_JoinSession:  NetPlayJoinSession failed.\n", NULL);
		return GE_FALSE;
	}

	if (!NetPlayCreatePlayer(&OurPlayerId, (char*)Name, NULL, NULL, 0, GE_FALSE))
	{
		geErrorLog_AddString(-1, "geCSNetMgr_JoinSession:  NetPlayCreatePlayer failed.\n", NULL);
		return GE_FALSE;
	}

	//Clients must wait until they get a Server Id.
	//  All other System messages. are ignored, until this happens.
	//  The only system message that should come before this is
	//  Create Client message.  Since Clients don't need this message
	//  this should not be a problem.

	StartTime = timeGetTime();
#if 1
	while( NET_TIMEOUT > (timeGetTime() -  StartTime) )
	{
		DPID	IdFrom, IdTo;
		HRESULT	Result;

		BSize = BUFFER_SIZE;

		IdFrom = IdTo = 0;

		Result = NetPlayReceive(&IdFrom, &IdTo, DPRECEIVE_ALL, &Packet, &BSize);
		
		if (Result == DP_OK)
		{
			if (BSize > 0)
			{
				if( (IdFrom != DPID_SYSMSG) &&(Packet[0] == NET_MSG_SERVER_ID)  )
				{
 					memcpy( &ServerId, &Packet[1], sizeof( ServerId ) );
					NetSession = GE_TRUE;
					return GE_TRUE;
				}
			}
		}
		else if (Result != DPERR_NOMESSAGES)
			return GE_FALSE;
	}
#else
	NetSession = GE_TRUE;
	ServerId = DPID_SERVERPLAYER;
	return( GE_TRUE);
#endif

	return( GE_FALSE);
} 	

//================================================================================
//	geCSNetMgr_StopSession
//================================================================================
GENESISAPI geBoolean GENESISCC geCSNetMgr_StopSession(geCSNetMgr *M)
{
	assert( geCSNetMgr_IsValid(M)!=GE_FALSE );

	if (NetSession)
	{
		NetSession = GE_FALSE;
	
		if (WeAreTheServer)		// If we are the server, then free the server player
		{
			if (!NetPlayDestroyPlayer(ServerId))
				return GE_FALSE;
		}

		if (!NetPlayDestroyPlayer(OurPlayerId))
			return GE_FALSE;

		if (!DeInitNetPlay())
		{
			//AFX_CPrintf("AFX_DestroyNetSession: There was a problem while De-Intializing NetPlay.\n");
			return GE_FALSE;
		}

	}

	WeAreTheServer = GE_FALSE;
	return GE_TRUE;
}

//================================================================================
//	geCSNetMgr_SendToServer
//================================================================================
GENESISAPI geBoolean GENESISCC geCSNetMgr_SendToServer(geCSNetMgr *M,  BOOL Guaranteed, uint8 *Data, int32 DataSize)
{
    DWORD           dwFlags = 0;

	assert( geCSNetMgr_IsValid(M)!=GE_FALSE );
	assert(DataSize+PACKET_HEADER_SIZE < BUFFER_SIZE);

	if (!NetSession)
		return GE_FALSE;

	if( DataSize+PACKET_HEADER_SIZE >= BUFFER_SIZE )
		return GE_FALSE;
	
	if (Guaranteed)
		dwFlags |= DPSEND_GUARANTEED;

	memcpy( &Packet[1], Data, DataSize );
	DataSize += PACKET_HEADER_SIZE;
	Packet[0] = NET_MSG_USER;

	if (NetPlaySend(OurPlayerId, ServerId, dwFlags, (void*)&Packet, DataSize) != DP_OK)
		return GE_FALSE;
	
	return GE_TRUE;
}

//================================================================================
//	geCSNetMgr_SendToClient
//================================================================================
GENESISAPI geBoolean GENESISCC geCSNetMgr_SendToClient(geCSNetMgr *M, geCSNetMgr_NetID To, BOOL Guaranteed, uint8 *Data, int32 DataSize)
{
    DWORD           dwFlags = 0;

	assert( geCSNetMgr_IsValid(M)!=GE_FALSE );
	assert(DataSize+PACKET_HEADER_SIZE < BUFFER_SIZE);
	
	if (!NetSession)
		return GE_FALSE;
	
	if( DataSize+PACKET_HEADER_SIZE >= BUFFER_SIZE )
		return GE_FALSE;

	if (Guaranteed)
		dwFlags |= DPSEND_GUARANTEED;
	
	memcpy( &Packet[1], Data, DataSize );
	DataSize += PACKET_HEADER_SIZE;
	Packet[0] = NET_MSG_USER;

	if (NetPlaySend(ServerId, To, dwFlags, (void*)&Packet, DataSize) != DP_OK)
		return GE_FALSE;
	
	return GE_TRUE;
}
