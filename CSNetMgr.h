/****************************************************************************************/
/*  CSNetMgr.h                                                                         */
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
/*  Genesis3D Version 1.1 released November 15, 1999                                 */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#ifndef GE_CSNETMGR_H
#define GE_CSNETMGR_H

#include "BaseType.h"

#ifdef __cplusplus
extern "C" {
#endif


//================================================================================
//	Structure defines
//================================================================================

// GENESIS_PUBLIC_APIS

typedef struct		geCSNetMgr	geCSNetMgr;

typedef uint32				geCSNetMgr_NetID;
#define	MAX_CLIENT_NAME		256

// Types for messages received from GE_ReceiveSystemMessage
typedef enum 
{
	NET_MSG_NONE,					// No msg
	NET_MSG_USER,					// User message
	NET_MSG_CREATE_CLIENT,			// A new client has joined in
	NET_MSG_DESTROY_CLIENT,			// An existing client has left
	NET_MSG_HOST,					// We are the server now
	NET_MSG_SESSIONLOST,			// Connection was lost
	NET_MSG_SERVER_ID,				// Internal, for hand shaking process
} geCSNetMgr_NetMsgType;

typedef struct
{
	char				Name[MAX_CLIENT_NAME];
	geCSNetMgr_NetID	Id;
} geCSNetMgr_NetClient;


#ifdef _INC_WINDOWS
	// Windows.h must be included previously for this api to be exposed.

	typedef struct geCSNetMgr_NetSession
	{
		char		SessionName[200];					// Description of Service provider
		GUID		Guid;								// Service Provider GUID
		#pragma message("define a geGUID?.. wouldn't need a windows dependency here...")
	} geCSNetMgr_NetSession;

GENESISAPI geBoolean		GENESISCC geCSNetMgr_FindSession(geCSNetMgr *M, const char *IPAdress, geCSNetMgr_NetSession **SessionList, int32 *SessionNum );
GENESISAPI geBoolean		GENESISCC geCSNetMgr_JoinSession(geCSNetMgr *M, const char *Name, const geCSNetMgr_NetSession* Session);
#endif

GENESISAPI geCSNetMgr *		GENESISCC geCSNetMgr_Create(void);
GENESISAPI void				GENESISCC geCSNetMgr_Destroy(geCSNetMgr **ppM);
GENESISAPI geCSNetMgr_NetID	GENESISCC geCSNetMgr_GetServerID(geCSNetMgr *M);
GENESISAPI geCSNetMgr_NetID	GENESISCC geCSNetMgr_GetOurID(geCSNetMgr *M);
GENESISAPI geCSNetMgr_NetID	GENESISCC geCSNetMgr_GetAllPlayerID(geCSNetMgr *M);
GENESISAPI geBoolean GENESISCC		geCSNetMgr_ReceiveFromServer(geCSNetMgr *M, geCSNetMgr_NetMsgType *Type, int32 *Size, uint8 **Data);
GENESISAPI geBoolean GENESISCC		geCSNetMgr_ReceiveFromClient(geCSNetMgr *M, geCSNetMgr_NetMsgType *Type, geCSNetMgr_NetID *IdClient, int32 *Size, uint8 **Data);
GENESISAPI geBoolean GENESISCC		geCSNetMgr_ReceiveSystemMessage(geCSNetMgr *M, geCSNetMgr_NetID IdFor, geCSNetMgr_NetMsgType *Type, geCSNetMgr_NetClient *Client);
GENESISAPI geBoolean GENESISCC		geCSNetMgr_ReceiveAllMessages(geCSNetMgr *M, geCSNetMgr_NetID *IdFrom, geCSNetMgr_NetID *IdTo, geCSNetMgr_NetMsgType *Type, int32 *Size, uint8 **Data);
GENESISAPI geBoolean GENESISCC		geCSNetMgr_WeAreTheServer(geCSNetMgr *M);
GENESISAPI geBoolean GENESISCC		geCSNetMgr_StartSession(geCSNetMgr *M, const char *SessionName, const char *PlayerName );
GENESISAPI geBoolean GENESISCC		geCSNetMgr_StopSession(geCSNetMgr *M);
GENESISAPI geBoolean GENESISCC		geCSNetMgr_SendToServer(geCSNetMgr *M, geBoolean Guaranteed, uint8 *Data, int32 DataSize);
GENESISAPI geBoolean GENESISCC		geCSNetMgr_SendToClient(geCSNetMgr *M, geCSNetMgr_NetID To, geBoolean Guaranteed, uint8 *Data, int32 DataSize);


// GENESIS_PRIVATE_APIS
#ifdef __cplusplus
}
#endif

#endif
