/****************************************************************************************/
/*  Engine.c                                                                            */
/*                                                                                      */
/*  Author: Charles Bloom/John Pollard                                                  */
/*  Description: Maintains the driver interface, as well as the bitmaps attached		*/
/*					to the driver.														*/
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

//#define DONT_DO_SPLASH // CB hack

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h> //timeGetTime
#include <stdlib.h> // _MAX_PATH
#include <direct.h>	// getcwd

#include "engine.h"

#include "Errorlog.h"
#include "DCommon.h"
#include "BitmapList.h"
#include "Bitmap.h"
#include "Bitmap._h"
#include "World.h"
#include "log.h"

//#define DO_ADDREMOVE_MESSAGES

#ifndef _DEBUG
#undef DO_ADDREMOVE_MESSAGES
#endif

extern	geBoolean	DoSplashScreen(geEngine *Engine, geDriver_Mode *Mode);

//============================
//	Internal Protos
//============================

static geBoolean Engine_InitDriver(	geEngine *Engine, 
									geDriver *Driver,
									geDriver_Mode *DriverMode);

static void Engine_DrawFontBuffer(geEngine *Engine);
static void Engine_Tick(geEngine *Engine);

static void SubLarge(LARGE_INTEGER *start, LARGE_INTEGER *end, LARGE_INTEGER *delta);

#define ABS(xx)	( (xx) < 0 ? (-(xx)) : (xx) )

//=====================================================================================
//	geEngine_SetGamma
//=====================================================================================
GENESISAPI geBoolean geEngine_SetGamma(geEngine *Engine, float Gamma)
{
	assert(Engine);

	if ( Gamma < 0.01f )
		Gamma  = 0.01f;

	if ( ABS( Engine->CurrentGamma - Gamma) < 0.01f )
		return GE_TRUE;

	Engine->CurrentGamma = Gamma;

	geEngine_UpdateGamma(Engine);

	return GE_TRUE;
}

//=====================================================================================
//	geEngine_GetGamma
//=====================================================================================
GENESISAPI geBoolean geEngine_GetGamma(geEngine *Engine, float *Gamma)
{
	assert(Engine);
	assert(Gamma);

	*Gamma = Engine->CurrentGamma;

	return GE_TRUE;//Engine->DriverInfo.RDriver->GetGamma(Gamma);
}

//=====================================================================================
//	geEngine_UpdateFogEnable
//=====================================================================================
static geBoolean geEngine_UpdateFogEnable(geEngine *Engine)
{
	if (Engine->DriverInfo.RDriver)
	{
		return Engine->DriverInfo.RDriver->SetFogEnable(Engine->FogEnable, 
														Engine->FogR, 
														Engine->FogG, 
														Engine->FogB, 
														Engine->FogStart, 
														Engine->FogEnd);
	}

	return GE_TRUE;
}

//=====================================================================================
//	geEngine_SetFogEnable
//=====================================================================================
GENESISAPI geBoolean geEngine_SetFogEnable(geEngine *Engine, geBoolean Enable, float r, float g, float b, float Start, float End)
{
	Engine->FogEnable = Enable;
	
	Engine->FogR = r;
	Engine->FogG = g;
	Engine->FogB = b;

	Engine->FogStart = Start;
	Engine->FogEnd = End;

	return geEngine_UpdateFogEnable(Engine);
}

void geEngine_UpdateGamma(geEngine *Engine)
{
DRV_Driver * RDriver;
geFloat LastBitmapGamma;

	assert(Engine);

	RDriver = Engine->DriverInfo.RDriver;

	LastBitmapGamma = Engine->BitmapGamma;

	if ( RDriver && (RDriver->EngineSettings->CanSupportFlags & DRV_SUPPORT_GAMMA) )
	{
		if ( RDriver->SetGamma(Engine->CurrentGamma) )
			Engine->BitmapGamma = 1.0f;
		else
			Engine->BitmapGamma = Engine->CurrentGamma;
	}
	else
	{
		Engine->BitmapGamma = Engine->CurrentGamma;
	}

	if ( ABS(Engine->BitmapGamma - LastBitmapGamma) < 0.1f )
	{
		Engine->BitmapGamma = LastBitmapGamma;
	}
	else
	{
	int i;

		// Attach all the bitmaps for the engine
		if (!BitmapList_SetGamma(Engine->AttachedBitmaps, Engine->BitmapGamma))
		{
			geErrorLog_AddString(-1, "geEngine_UpdateGamma:  BitmapList_SetGamma for Engine failed", NULL);
		}

		for (i=0; i< Engine->NumWorlds; i++)
		{
			// <> sleazy to peak into World like this
			if (!BitmapList_SetGamma(Engine->Worlds[i]->AttachedBitmaps, Engine->BitmapGamma ))
			{
				geErrorLog_AddString(-1, "geEngine_UpdateGamma:  BitmapList_SetGamma for World failed", NULL);
			}
		}
	}

}

//================================================================================
//	geEngine_BitmapListInit
//	Initializes the engine bitmaplist
//================================================================================
geBoolean geEngine_BitmapListInit(geEngine *Engine)
{
	assert(Engine);
	assert(Engine->AttachedBitmaps == NULL);

	if ( Engine->AttachedBitmaps == NULL )
	{
		Engine->AttachedBitmaps = BitmapList_Create();
		if ( ! Engine->AttachedBitmaps )
		{
			geErrorLog_AddString(-1, "geEngine_BitmapListInit:  BitmapList_Create failed...", NULL);
			return GE_FALSE;
		}
	}
	return GE_TRUE;
}

//================================================================================
//	geEngine_BitmapListShutdown
//================================================================================
geBoolean geEngine_BitmapListShutdown(geEngine *Engine)
{
	assert(Engine);

	if ( Engine->AttachedBitmaps )
	{
		assert(	Engine->DriverInfo.Active || BitmapList_CountMembersAttached(Engine->AttachedBitmaps) == 0 );

		//BitmapList_DetachAll(Engine->AttachedBitmaps);
		// Destroy detaches for you!
		BitmapList_Destroy(Engine->AttachedBitmaps);
		Engine->AttachedBitmaps = NULL;
	}

	return GE_TRUE;
}

//================================================================================
//	geEngine_AddBitmap
//================================================================================
GENESISAPI geBoolean geEngine_AddBitmap(geEngine *Engine, geBitmap *Bitmap)
{
	assert(Engine);
	assert(Bitmap);
	assert(Engine->AttachedBitmaps);

	assert(Engine->FrameState == FrameState_None);

	if (!Engine->AttachedBitmaps)
	{
		geErrorLog_AddString(-1, "geEngine_AddBitmap:  AttachedBitmaps is NULL.", NULL);
		return GE_FALSE;
	}

	geBitmap_SetDriverFlags(Bitmap,RDRIVER_PF_2D);

	// Add bitmap to the lit of bitmaps attached to the engine
	if ( BitmapList_Add(Engine->AttachedBitmaps, (geBitmap *)Bitmap) )
	{
		Engine->Changed = GE_TRUE;
		
		#ifdef DO_ADDREMOVE_MESSAGES	
		{
		char str[100];
			sprintf(str,"Engine_AddBitmap : %08X : new\n",Bitmap);
			OutputDebugString(str);
		}
		#endif
	}
	else
	{
		#ifdef DO_ADDREMOVE_MESSAGES	
		{
		char str[100];
			sprintf(str,"Engine_AddBitmap : %08X : old\n",Bitmap);
			OutputDebugString(str);
		}
		#endif
	}

	return GE_TRUE;
}

//================================================================================
//	geEngine_RemoveBitmap
//================================================================================
GENESISAPI geBoolean geEngine_RemoveBitmap(geEngine *Engine, geBitmap *Bitmap)
{
	assert(Engine);
	assert(Bitmap);
	assert(Engine->AttachedBitmaps);

//	assert(Engine->FrameState == FrameState_None);

	if ( ! Engine->AttachedBitmaps )
		return GE_FALSE;

	if ( BitmapList_Remove(Engine->AttachedBitmaps,Bitmap) )
	{
		Engine->Changed = GE_TRUE;
		
		if (!geBitmap_DetachDriver(Bitmap, GE_TRUE))
		{
			geErrorLog_AddString(-1, "geEngine_RemoveBitmap:  geBitmap_DetachDriver failed...", NULL);
			return GE_FALSE;
		}

		#ifdef DO_ADDREMOVE_MESSAGES	
		{
		char str[100];
			sprintf(str,"Engine_RemoveBitmap : %08X : removed\n",Bitmap);
			OutputDebugString(str);
		}
		#endif
	}
	else
	{
		#ifdef DO_ADDREMOVE_MESSAGES	
		{
		char str[100];
			sprintf(str,"Engine_RemoveBitmap : %08X : left\n",Bitmap);
			OutputDebugString(str);
		}
		#endif
	}
	
	return GE_TRUE;
}

//=====================================================================================
//	geEngine_SetDriverAndMode
//=====================================================================================
GENESISAPI geBoolean geEngine_SetDriverAndMode(	geEngine *Engine, 
												geDriver *Driver, 
												geDriver_Mode *DriverMode)
{
	assert(Engine);
	assert(Driver);
	assert(DriverMode);

	// init calls _Reset and eventually it gets down and Detaches all

	//	Set up the Render Driver
	if (!Engine_InitDriver(Engine, Driver, DriverMode))
		return GE_FALSE;

	// Force a Driver update
	geEngine_SetAllWorldChangedFlag(Engine, GE_TRUE);
	Engine->Changed = GE_TRUE;

	geEngine_UpdateGamma(Engine);
	geEngine_UpdateFogEnable(Engine);

#ifdef DONT_DO_SPLASH
	#pragma message("Engine :splash screen disabled")
	Engine = Engine;
#else
	// Do the splash screen
	if	(DoSplashScreen(Engine, DriverMode) == GE_FALSE)
		return GE_FALSE;
#endif

	return GE_TRUE;
}

//=====================================================================================
//	geEngine_GetDriverSystem
//=====================================================================================
GENESISAPI geDriver_System *geEngine_GetDriverSystem(geEngine *Engine)
{
	assert(Engine);

	return (geDriver_System*)&Engine->DriverInfo;
}

//=====================================================================================
//	geEngine_Activate
//		this hits the drivers activation code to manage
//		surfaces and exclusive modes for devices (WM_ACTIVATEAPP)
//=====================================================================================
GENESISAPI geBoolean geEngine_Activate(geEngine *Engine, geBoolean bActive)
{
	DRV_Driver	*RDriver;
	
	assert(Engine);

	RDriver	=Engine->DriverInfo.RDriver;

	if(Engine->DriverInfo.Active && RDriver)
	{
		// <> this can sometimes be a pain for debugging
		#if 1
		if ( RDriver->SetActive )
			return RDriver->SetActive(bActive);
		#endif
	}

	return	GE_TRUE;
}

//====================================================================================
//	geEngine_UpdateWindow
//		this call updates the drivers with a new rect to blit to
//		(usually the result of a window move or resize)
//====================================================================================
GENESISAPI geBoolean geEngine_UpdateWindow(geEngine *Engine)
{
	DRV_Driver	*RDriver;
		
	RDriver	= Engine->DriverInfo.RDriver;

	if(Engine->DriverInfo.Active && RDriver)
	{
		if (RDriver->UpdateWindow )
			return RDriver->UpdateWindow();
	}

	return	GE_TRUE;
}

//=====================================================================================
//	geEngine_ShutdownDriver
//=====================================================================================
GENESISAPI geBoolean geEngine_ShutdownDriver(geEngine *Engine)
{
Sys_DriverInfo *DrvInfo;

	assert(Engine);

	DrvInfo = &(Engine->DriverInfo);

	assert(DrvInfo);

	if (!DrvInfo->Active)
		return GE_TRUE;			// Just return true, and don't do nothing

	#if 0 // <>	
	#ifdef _DEBUG
	OutputDebugString("geEngine_ShutdownDriver\n");
	#endif
	#endif

	// First, reset the driver
	if (!geEngine_ResetDriver(Engine))
	{
		geErrorLog_AddString(-1, "geEngine_ShutdownDriver:  geEngine_ResetDriver failed.", NULL);
		return GE_FALSE;
	}

	// Shutdown the driver
	DrvInfo->RDriver->Shutdown();

	if (!FreeLibrary(DrvInfo->DriverHandle) )
		return GE_FALSE;

	DrvInfo->Active = GE_FALSE;
	DrvInfo->RDriver = NULL;

	return GE_TRUE;
}

//================================================================================
//	geEngine_RenderPoly
//		World MUST ne passed in if using a texture, as it is a container object for ALL 3d textures
//		* stop passing World ?
//		* cut the Flags parameter and pass in zero ?
//================================================================================
GENESISAPI void GENESISCC geEngine_RenderPoly(const geEngine *Engine,
	const GE_TLVertex *Points, int NumPoints, const geBitmap *Texture, uint32 Flags)
{
	geBoolean	Ret;

	assert(Engine && Points );

	if ( Texture )
	{
	geRDriver_THandle * TH;
	
//		assert(World);
//		assert(geEngine_HasWorld(Engine, World) == GE_TRUE);
//		assert(World->AttachedBitmaps);
//		assert(BitmapList_Has(World->AttachedBitmaps, (geBitmap*)Texture) == GE_TRUE);

		TH = geBitmap_GetTHandle(Texture);
		assert(TH);

		Ret = Engine->DriverInfo.RDriver->RenderMiscTexturePoly((DRV_TLVertex *)Points,
			NumPoints,TH,Flags);
	}
	else
	{
		Ret = Engine->DriverInfo.RDriver->RenderGouraudPoly((DRV_TLVertex *)Points,
			NumPoints,Flags);
	}

	assert(Ret == GE_TRUE);
}

GENESISAPI void GENESISCC geEngine_RenderPolyArray(const geEngine *Engine, const GE_TLVertex ** pPoints, int * pNumPoints, int NumPolys, 
								const geBitmap *Texture, uint32 Flags)
{
geBoolean	Ret;
int pn;
DRV_Driver * Driver;

	assert(Engine && pPoints && pNumPoints );

	Driver = Engine->DriverInfo.RDriver;
	assert(Driver);

	if ( Texture )
	{
	geRDriver_THandle * TH;
	
		TH = geBitmap_GetTHandle(Texture);
		assert(TH);

		for(pn=0;pn<NumPolys;pn++)
		{
			assert(pPoints[pn]);
			Ret = Driver->RenderMiscTexturePoly((DRV_TLVertex *)pPoints[pn],
				pNumPoints[pn],TH,Flags);
			assert(Ret);
		}
	}
	else
	{
		for(pn=0;pn<NumPolys;pn++)
		{
			assert(pPoints[pn]);
			Ret = Driver->RenderGouraudPoly((DRV_TLVertex *)pPoints[pn],
				pNumPoints[pn],Flags);
			assert(Ret);
		}
	}

}

//================================================================================
//	geEngine_DrawBitmap
//================================================================================
GENESISAPI geBoolean GENESISCC geEngine_DrawBitmap(const geEngine *Engine,
	const geBitmap *Bitmap,
	const geRect * Source, uint32 x, uint32 y)
{
geRDriver_THandle * TH;
geBoolean Ret;
	
	//#pragma message("make geRect the same as RECT, or don't use RECT!?")
	// The drivers once did not include genesis .h's
	// (D3D uses RECT so thats why the drivers adopted RECT's...)
	#pragma message("Engine : Make the drivers use geRect, JP")

	assert(Engine);
	assert(Bitmap);
	
	assert(Engine->AttachedBitmaps);
	assert(BitmapList_Has(Engine->AttachedBitmaps, (geBitmap *)Bitmap) == GE_TRUE);

	TH = geBitmap_GetTHandle(Bitmap);
	assert(TH);

	//Ret = Engine->DriverInfo.RDriver->Drawdecal(TH,(RECT *)Source,x,y);

	if (Source)		// Source CAN be NULL!!!
	{
		RECT rect;

		rect.left = Source->Left;
		rect.top = Source->Top;
		rect.right = Source->Right;
		rect.bottom = Source->Bottom;

		Ret = Engine->DriverInfo.RDriver->DrawDecal(TH, &rect, x,y);
	}
	else
		Ret = Engine->DriverInfo.RDriver->DrawDecal(TH, NULL, x,y);

	if ( ! Ret )
	{
		geErrorLog_AddString(-1,"geEngine_DrawBitmap : DrawDecal failed", NULL);	
	}

return Ret;
}

//====================================================================================
//	geEngine_RebuildFastWorldList
//====================================================================================
geBoolean geEngine_RebuildFastWorldList(geEngine *Engine)
{
	int32				i;
	geEngine_WorldList	*pWorldList;

	Engine->NumWorlds = 0;

	pWorldList = Engine->WorldList;

	for (i=0; i< ENGINE_MAX_WORLDS; i++, pWorldList++)
	{
		if (pWorldList->RefCount > 0)
		{
			assert(pWorldList->World);

			Engine->Worlds[Engine->NumWorlds++] = pWorldList->World;

			assert(Engine->NumWorlds <= ENGINE_MAX_WORLDS);
		}
	}

	return GE_TRUE;
}

//====================================================================================
// geEngine_AddWorld
//====================================================================================
GENESISAPI geBoolean geEngine_AddWorld(geEngine *Engine, geWorld *World)
{
	int32				i;
	geEngine_WorldList	*pWorldList;

	assert(Engine);
	assert(World);

	// Try to find it in the list first
	pWorldList = Engine->WorldList;
	for (i=0; i< ENGINE_MAX_WORLDS; i++, pWorldList++)
	{
		if (pWorldList->World == World)	// World is allready in list
		{
			assert(pWorldList->RefCount > 0);		// There should allready be a ref count!!!
			pWorldList->RefCount++;
				
			#ifdef DO_ADDREMOVE_MESSAGES	
			{
			char str[100];
				sprintf(str,"Engine_AddWorld : %08X : old\n",World);
				OutputDebugString(str);
			}
			#endif

			return GE_TRUE;
		}

	}

	#ifdef DO_ADDREMOVE_MESSAGES	
	{
	char str[100];
		sprintf(str,"Engine_AddWorld : %08X : new\n",World);
		OutputDebugString(str);
	}
	#endif

	// Not found, add a new one
	pWorldList = Engine->WorldList;
	for (i=0; i< ENGINE_MAX_WORLDS; i++, pWorldList++)
	{
		if (!pWorldList->RefCount)
			break;
	}

	if (i == ENGINE_MAX_WORLDS)
	{
		geErrorLog_AddString(-1, "geEngine_AddWorld:  Out of slots.", NULL);
		return GE_FALSE;
	}	

	// Save the info for the first time
	pWorldList->World = World;
	pWorldList->RefCount = 1;

	// Re-build the fast list of worlds
	geEngine_RebuildFastWorldList(Engine);
	Engine->Changed = GE_TRUE;
	World->Changed = GE_TRUE;

	if (!geWorld_CreateRef(World))
	{
		geErrorLog_AddString(-1, "geEngine_AddWorld:  geWorld_CreateRef failed...", NULL);
		return GE_FALSE;
	}

	return GE_TRUE;
}

//====================================================================================
// geEngine_RemoveWorld
//====================================================================================
GENESISAPI geBoolean geEngine_RemoveWorld(geEngine *Engine, geWorld *World)
{
	int32				i;
	geEngine_WorldList	*pWorldList;

	assert(Engine);
	assert(World);

	// Try to find it in the list 
	pWorldList = Engine->WorldList;
	for (i=0; i< ENGINE_MAX_WORLDS; i++, pWorldList++)
	{
		if (pWorldList->World == World)	
			break;
	}

	if (i == ENGINE_MAX_WORLDS)
	{
		geErrorLog_AddString(-1, "geEngine_RemoveWorld:  World not found.", NULL);
		return GE_FALSE;
	}	

	assert(pWorldList->RefCount > 0);

	// Decrease the reference count on the worldlist
	pWorldList->RefCount--;

	// When the ref count gos to zero, remove the world for good
	if (pWorldList->RefCount == 0)		
	{
	
		#ifdef DO_ADDREMOVE_MESSAGES	
		{
		char str[100];
			sprintf(str,"Engine_RemoveWorld : %08X : removed\n",World);
			OutputDebugString(str);
		}
		#endif

		// Detach all the bitmaps in the world before finally removing it
		if (Engine->DriverInfo.Active)
		{
			if (!geWorld_DetachAll(World))
			{
				geErrorLog_AddString(-1, "geEngine_RemoveWorld:  geWorld_DetachAll failed.", NULL);
				return GE_FALSE;
			}
		}

		// Clear this pWorldList slot
		memset(pWorldList, 0, sizeof(*pWorldList));
		
		// Re-build the fast list of worlds
		geEngine_RebuildFastWorldList(Engine);

		// Force an update
		Engine->Changed = GE_TRUE;
		
		// Free the world (decrease it's reference count)
		geWorld_Free(World);
	}
	else
	{
		#ifdef DO_ADDREMOVE_MESSAGES	
		{
		char str[100];
			sprintf(str,"Engine_RemoveWorld : %08X : left\n",World);
			OutputDebugString(str);
		}
		#endif
	}

	return GE_TRUE;
}


//====================================================================================
// geEngine_RemoveAllWorlds
//====================================================================================
geBoolean geEngine_RemoveAllWorlds(geEngine *Engine)
{
	int32				i;
	geEngine_WorldList	*pWorldList;
	geWorld				*World;

	assert(Engine);

	// Try to find it in the list 
	pWorldList = Engine->WorldList;
	for (i=0; i< ENGINE_MAX_WORLDS; i++, pWorldList++)
	{
		World = pWorldList->World;
		if ( World )
		{
			assert(pWorldList->RefCount > 0);

			if (Engine->DriverInfo.Active)
			{
				if (!geWorld_DetachAll(World))
				{
					geErrorLog_AddString(-1, "geEngine_RemoveWorld:  geWorld_DetachAll failed.", NULL);
					return GE_FALSE;
				}
			}

			assert( World->RefCount >= 1 );

			// Free the world (decrease it's reference count)
			geWorld_Free(World);
		}
		memset(pWorldList, 0, sizeof(*pWorldList));
	}
		
	// Force an update
	geEngine_RebuildFastWorldList(Engine);
	Engine->Changed = GE_TRUE;

	return GE_TRUE;
}

//=====================================================================================
//	geEngine_HasWorld
//=====================================================================================
geBoolean geEngine_HasWorld(const geEngine *Engine, const geWorld *World)
{
	int32						i;
	const geEngine_WorldList	*pWorldList;

	assert(Engine);
	assert(World);

	// Try to find it in the list 
	pWorldList = Engine->WorldList;
	for (i=0; i< ENGINE_MAX_WORLDS; i++, pWorldList++)
	{
		if (pWorldList->World == World)	
		{
			assert(pWorldList->RefCount > 0);
			return GE_TRUE;
		}
	}
	
	return GE_FALSE;
}

//=====================================================================================
//	geEngine_DetachAllWorlds
//=====================================================================================
geBoolean geEngine_DetachAllWorlds(geEngine *Engine)
{
	int32		i;
	geBoolean	Ret;

	Ret = GE_TRUE;

	for (i=0; i< Engine->NumWorlds; i++)
	{
		if (!geWorld_DetachAll(Engine->Worlds[i]))
		{
			geErrorLog_AddString(-1, "geEngine_DetachAllWorlds:  geWorld_DetachAll failed.", NULL);
			Ret =  GE_FALSE;
		}
		
		if (!geEngine_DestroyWorldLightmapTHandles(Engine, Engine->Worlds[i]))
		{
			geErrorLog_AddString(-1, "geEngine_DetachAllWorlds:  geEngine_DestroyWorldLightmapTHandles failed.", NULL);
			Ret = GE_FALSE;
		}
	}

	return Ret;
}

//=====================================================================================
//	geEngine_CreateWorldLightmapTHandles
//=====================================================================================
geBoolean geEngine_CreateWorldLightmapTHandles(geEngine *Engine, geWorld *World)
{
	DRV_Driver			*RDriver;
	int32				i;
	World_BSP			*BSP;
	GBSP_BSPData		*BSPData;
	#ifdef _DEBUG
	int	CreatedCount = 0;
	#endif	

	assert(Engine);
	assert(World);

	BSP = World->CurrentBSP;
	BSPData = &BSP->BSPData;
	RDriver = Engine->DriverInfo.RDriver;

	//
	//	Create all the lightmap thandles
	//
	World_SetEngine(Engine);
	World_SetWorld(World);
	World_SetGBSP(World->CurrentBSP);

	for (i=0; i<BSPData->NumGFXFaces; i++)
	{
		BOOL		D;
		DRV_LInfo	*pLInfo;

		pLInfo = &BSP->SurfInfo[i].LInfo;

		if ((pLInfo->Face == -1))
			continue;

		if (!(BSP->SurfInfo[i].Flags & SURFINFO_LIGHTMAP))
			continue;

		assert(!pLInfo->THandle);		// This should be true!!!
		
		if (pLInfo->THandle)
			continue;
		
		Light_SetupLightmap(pLInfo, &D);

		// {} _LIGHTMAP_
		pLInfo->THandle = Engine_CreateTHandle(Engine,pLInfo->Width, pLInfo->Height, 1, ENGINE_PF_LIGHTMAP);

		if (!pLInfo->THandle)
		{
			geErrorLog_AddString(-1, RDriver->LastErrorStr, NULL);
			geErrorLog_AddString(-1, "geEngine_CreateWorldTHandles: Engine_CreateTHandle failed...\n", NULL);
			return GE_FALSE;
		}	
		
		#ifdef _DEBUG
		CreatedCount ++;
		#endif	
	}

#ifdef _DEBUG
	Log_Printf("geEngine_CreateWorldLightmapTHandles:Created %d of %d\n",CreatedCount,BSPData->NumGFXFaces);
#endif

	return GE_TRUE; 
}

//=====================================================================================
//	geEngine_DestroyWorldLightmapTHandles
//=====================================================================================
geBoolean geEngine_DestroyWorldLightmapTHandles(geEngine *Engine, geWorld *World)
{
	DRV_Driver			*RDriver;
	int32				i;
	World_BSP			*BSP;
	GBSP_BSPData		*BSPData;
	geBoolean			Ret;
	int32				Handle1=1768710981;
	int32				Handle2=0x21657370;
	#ifdef _DEBUG
	int	DestroyedCount = 0;
	#endif	

	assert(Engine);
	assert(World);

	Ret = GE_TRUE;

	BSP = World->CurrentBSP;
	BSPData = &BSP->BSPData;
	RDriver = Engine->DriverInfo.RDriver;

	//
	//	Create all the lightmap thandles
	//
	World_SetEngine(Engine);
	World_SetWorld(World);
	World_SetGBSP(World->CurrentBSP);

	for (i=0; i<BSPData->NumGFXFaces; i++)
	{
		DRV_LInfo	*pLInfo;

		pLInfo = &BSP->SurfInfo[i].LInfo;

		if ((pLInfo->Face == -1))
			continue;

		if (!(BSP->SurfInfo[i].Flags & SURFINFO_LIGHTMAP))
			continue;

		if (!pLInfo->THandle)
			continue;
		
		if (!RDriver->THandle_Destroy(pLInfo->THandle))
			Ret = GE_FALSE;

		pLInfo->THandle = NULL;
		
		#ifdef _DEBUG
		DestroyedCount ++;
		#endif	
	}

#ifdef _DEBUG
	Log_Printf("geEngine_DestroyWorldLightmapTHandles: Freed %d of %d\n",DestroyedCount,BSPData->NumGFXFaces);
#endif

	return Ret;
}

//=====================================================================================
//	geEngine_AttachAllWorlds
//=====================================================================================
geBoolean geEngine_AttachAllWorlds(geEngine *Engine)
{
	int32		i;
	DRV_Driver			*RDriver;

	assert(Engine);

	RDriver = Engine->DriverInfo.RDriver;

	for (i=0; i< Engine->NumWorlds; i++)
	{
		if (!geWorld_AttachAll(Engine->Worlds[i], RDriver, Engine->BitmapGamma ))
		{
			geErrorLog_AddString(-1, "geEngine_AttachAllWorlds:  geWorld_AttachAll failed.", NULL);
			return GE_FALSE;
		}

		if (!geEngine_CreateWorldLightmapTHandles(Engine, Engine->Worlds[i]))
		{
			geErrorLog_AddString(-1, "geEngine_AttachAllWorlds:  geEngine_CreateWorldLightmapTHandles failed.", NULL);
			return GE_FALSE;
		}

	}

	return GE_TRUE;
}

//=====================================================================================
//	geEngine_AttachAll
//=====================================================================================
geBoolean geEngine_AttachAll(geEngine *Engine)
{
	DRV_Driver			*RDriver;

	assert( Engine );

	RDriver = Engine->DriverInfo.RDriver;
	assert( RDriver );

    // If current driver is not active, then split
	if (!Engine->DriverInfo.Active)
        return GE_TRUE;

	// Attach all the bitmaps for the engine
	if (!BitmapList_AttachAll(Engine->AttachedBitmaps, RDriver, Engine->BitmapGamma))
	{
		geErrorLog_AddString(-1, "geEngine_AttachAll:  BitmapList_AttachAll for Engine failed...", NULL);
		return GE_FALSE;
	}

	// Attach all the bitmaps for the world
	if (!geEngine_AttachAllWorlds(Engine))
	{
		geErrorLog_AddString(-1, "geEngine_AttachAll:  geEngine_AttachAllWorlds failed.", NULL);
		return GE_FALSE;
	}

	return GE_TRUE;
}

//=====================================================================================
//	geEngine_DetachAll
//=====================================================================================
geBoolean geEngine_DetachAll(geEngine *Engine)
{
	assert(Engine);

	// Shutdown all the geBitmaps
	if (!BitmapList_DetachAll(Engine->AttachedBitmaps))
	{
		geErrorLog_AddString(-1, "geEngine_DetachAll:  BitmapList_DetachAll failed for engine.", NULL);
		return GE_FALSE;
	}

	// Detach all the bitmaps that belong to all the currently connected worlds
	if (!geEngine_DetachAllWorlds(Engine))
	{
		geErrorLog_AddString(-1, "geEngine_DetachAll:  geEngine_DetachAllWorlds failed for engine.", NULL);
		return GE_FALSE;
	}

	return GE_TRUE;
}

//=====================================================================================
//	geEngine_ResetDriver
//=====================================================================================
geBoolean geEngine_ResetDriver(geEngine *Engine)
{
	assert(Engine != NULL);
	assert(Engine->DriverInfo.RDriver);

	// To be safe, detach all things from the current driver
	if (!geEngine_DetachAll(Engine))
	{
		geErrorLog_AddString(-1, "geEngine_ResetDriver:  geEngine_DetachAll failed.", NULL);
		return GE_FALSE;
	}

	// Reset the driver
	if (!Engine->DriverInfo.RDriver->Reset())
	{
		geErrorLog_AddString(-1, "geEngine_ResetDriver:  Engine->DriverInfo.RDriver->Reset() failed.", NULL);
		return GE_FALSE;
	}

	geEngine_UpdateFogEnable(Engine);

	return GE_TRUE;
}

//===================================================================================
//	geEngine_InitFonts
//===================================================================================
geBoolean geEngine_InitFonts(geEngine *Engine)
{
	Sys_FontInfo	*Fi;

	assert(Engine);

	Fi = &Engine->FontInfo;

	assert(Fi->FontBitmap == NULL);

	// Load the bitmap
	{
		geVFile *				MemFile;
		geVFile_MemoryContext	Context;

		{
			extern unsigned char font_bmp[];
			extern int font_bmp_length;

			Context.Data = font_bmp;
			Context.DataLength = font_bmp_length;

			MemFile = geVFile_OpenNewSystem(NULL, GE_VFILE_TYPE_MEMORY, NULL, &Context, GE_VFILE_OPEN_READONLY);
		}

		if	(!MemFile)
		{
			geErrorLog_AddString(-1,"InitFonts : geVFile_OpenNewSystem Memory fontbmp failed.", NULL);
			return GE_FALSE;
		}

		if ( ! (Fi->FontBitmap = geBitmap_CreateFromFile(MemFile)) )
		{
			geErrorLog_AddString(-1,"InitFonts : geBitmap_CreateFromFile failed.", NULL);
			goto fail;
		}

		#if 0
		#pragma message("Engine : fonts will have alpha once Decals do : CB");
		// <> CB : give fonts alpha so they look purty
		//			pointless right now cuz we don't get enum'ed a _2D_ type with alpha
		{
		geBitmap * FontAlpha;
			FontAlpha = geBitmap_Create( geBitmap_Width(Fi->FontBitmap), geBitmap_Height(Fi->FontBitmap), 1, GE_PIXELFORMAT_8BIT_GRAY );
			if ( FontAlpha )
			{
				if ( geBitmap_BlitBitmap(Fi->FontBitmap,FontAlpha) )
				{
					if ( ! geBitmap_SetAlpha( Fi->FontBitmap, FontAlpha ) )
					{
						geErrorLog_AddString(-1,"InitFonts : SetAlpha failed : non-fatal", NULL);
					}
				}
				else
				{
					geErrorLog_AddString(-1,"InitFonts : BlitBitmap failed : non-fatal", NULL);
				}
				geBitmap_Destroy(&FontAlpha);
			}
		}
		#endif

		if (!geBitmap_SetColorKey(Fi->FontBitmap, GE_TRUE, 0, GE_FALSE))
		{
			geErrorLog_AddString(-1,"InitFonts : geBitmap_SetColorKey failed.", NULL);
			goto fail;
		}

		if ( ! geEngine_AddBitmap(Engine,Fi->FontBitmap) )
		{
			geErrorLog_AddString(-1,"InitFonts : geEngine_AddBitmap failed.", NULL);
			goto fail;
		}

		goto success;

		fail:

		geVFile_Close(MemFile);
		return GE_FALSE;

		success:
		
		geVFile_Close(MemFile);
	}

	//
	//	Setup font lookups
	//
	{
		int PosX, PosY, Width, i;

		PosX = 0;
		PosY = 0;
		Width = 128*8;

		for (i=0; i< 128; i++)
		{
			Fi->FontLUT1[i] = (PosX<<16) | PosY;
			PosX+=8;

			if (PosX >= Width)
			{
				PosY += 14;
				PosX = 0;
			}
		}
	}

	return GE_TRUE;
}

//===================================================================================
//	geEngine_ShutdownFonts
//===================================================================================
geBoolean geEngine_ShutdownFonts(geEngine *Engine)
{
	Sys_FontInfo	*Fi;

	assert(Engine);

	Fi = &Engine->FontInfo;

	if (Fi->FontBitmap)
	{
		if (!geEngine_RemoveBitmap(Engine, Fi->FontBitmap))
		{
			geErrorLog_AddString(-1, "geEngine_ShutdownFonts:  geEngine_RemoveBitmap failed.", NULL);
			return GE_FALSE;
		}

		geBitmap_Destroy(&Fi->FontBitmap);
	}

	return GE_TRUE;
}

//=====================================================================================
//	geEngine_SetAllWorldChangedFlag
//	Forces all worlds to be uploaded to the card again
//=====================================================================================
void geEngine_SetAllWorldChangedFlag(geEngine *Engine, geBoolean Flag)
{
	int32		i;

	for (i=0; i< Engine->NumWorlds; i++)
		Engine->Worlds[i]->Changed = Flag;
}


//=====================================================================================
//	geEngine_LoadLibrary
//=====================================================================================
HINSTANCE geEngine_LoadLibrary( const char * lpLibFileName, const char *DriverDirectory)
{
	char	Buff[_MAX_PATH];
	char	*StrEnd;
	HINSTANCE	Library;

	//-------------------------
	strcpy(Buff, DriverDirectory);
	StrEnd = Buff + strlen(Buff) - 1;
	if ( *StrEnd != '\\' && *StrEnd != '/' && *StrEnd != ':' )
	{
		strcat(Buff,"\\");
	}
	strcat(Buff, lpLibFileName);
	Library = LoadLibrary(Buff);
	if ( Library )
		return Library;

#pragma message("Engine : LoadLibrary : need geConfig_GetDriverDir")
#ifdef LOADLIBRARY_HARDCODES
	#pragma message("Engine : using LoadLibrary HardCodes : curdir, q:\\genesis, c:\\genesis")

	//-------------------------

	getcwd(Buff,_MAX_PATH);
	StrEnd = Buff + strlen(Buff) - 1;
	if ( *StrEnd != '\\' && *StrEnd != '/' && *StrEnd != ':' )
	{
		strcat(Buff,"\\");
	}
	strcat(Buff, lpLibFileName);
	Library = LoadLibrary(Buff);
	if ( Library )
		return Library;

	//-------------------------

	strcpy(Buff, "q:\\genesis");
	StrEnd = Buff + strlen(Buff) - 1;
	if ( *StrEnd != '\\' && *StrEnd != '/' && *StrEnd != ':' )
	{
		strcat(Buff,"\\");
	}
	strcat(Buff, lpLibFileName);
	Library = LoadLibrary(Buff);
	if ( Library )
		return Library;

	//-------------------------

	strcpy(Buff, "c:\\genesis");
	StrEnd = Buff + strlen(Buff) - 1;
	if ( *StrEnd != '\\' && *StrEnd != '/' && *StrEnd != ':' )
	{
		strcat(Buff,"\\");
	}
	strcat(Buff, lpLibFileName);
	Library = LoadLibrary(Buff);
	if ( Library )
		return Library;
#endif

return NULL;
}
 
 
extern GInfo GlobalInfo;		// AHH!!!  Get rid of this!!!

//=====================================================================================
//	EngineInitDriver
//=====================================================================================

static geBoolean Engine_InitDriver(	geEngine *Engine, 
								geDriver *Driver,
								geDriver_Mode *DriverMode)
{
	Sys_DriverInfo		*DrvInfo;
	DRV_Hook			*Hook;
	DRV_DriverHook		DLLDriverHook;
	DRV_Driver			*RDriver;

	assert(Engine != NULL);
	assert(Driver != NULL);
	assert(DriverMode != NULL);

	DrvInfo = &Engine->DriverInfo;

	//#pragma message("Engine : DriverMode is changing, do Bitmap re-attaches")
	// _Shutdown calls _Reset which detaches all

	if (! geEngine_ShutdownDriver(Engine))
	{
		geErrorLog_AddString(-1, "Engine_InitDriver:  geEngine_ShutdownDriver failed.", NULL);
		return GE_FALSE;
	}

	if (DrvInfo->Active)
	{
		geErrorLog_Add(GE_ERR_DRIVER_ALLREADY_INITIALIZED, NULL);
		return GE_FALSE;
	}
	
	DrvInfo->CurDriver = Driver;
	DrvInfo->CurMode = DriverMode;

	DrvInfo->DriverHandle = geEngine_LoadLibrary(Driver->FileName, Engine->DriverDirectory);

	if (!DrvInfo->DriverHandle)
	{
		geErrorLog_Add(GE_ERR_DRIVER_NOT_FOUND, NULL);
		return GE_FALSE;
	}

	#ifdef LINK_STATIC_DRIVER
	{
		extern BOOL DriverHook(DRV_Driver **Driver);
		Hook = (DRV_Hook*)DriverHook;
	}
	#else
	Hook = (DRV_Hook*)GetProcAddress(DrvInfo->DriverHandle, "DriverHook");
	#endif
	
	if (!Hook)
	{
		geErrorLog_Add(GE_ERR_INVALID_DRIVER, NULL);
		return GE_FALSE;
	}

	if (!Hook(&DrvInfo->RDriver))
	{
		DrvInfo->RDriver = NULL;
		geErrorLog_Add(GE_ERR_INVALID_DRIVER, NULL);
		return GE_FALSE;
	}

	// Get a handy pointer to the driver
	RDriver = DrvInfo->RDriver;

	if (RDriver->VersionMajor != DRV_VERSION_MAJOR || RDriver->VersionMinor != DRV_VERSION_MINOR)
	{
		geErrorLog_Add(GE_ERR_INVALID_DRIVER, NULL);
		return GE_FALSE;
	}

	// We MUST set this! So driver can setup lightmap data when needed...
	RDriver->SetupLightmap = Light_SetupLightmap;
	RDriver->GlobalInfo = &GlobalInfo;

	strcpy(DLLDriverHook.AppName, Engine->AppName);

	//
	//	Setup what driver they want
	//

	DLLDriverHook.Driver = Driver->Id;
	strcpy(DLLDriverHook.DriverName, Driver->Name);
	DLLDriverHook.Mode = DriverMode->Id;
	DLLDriverHook.Width = DriverMode->Width;
	DLLDriverHook.Height = DriverMode->Height;
	DLLDriverHook.hWnd = Engine->hWnd;
	strcpy(DLLDriverHook.ModeName, DriverMode->Name);
	
	if (!RDriver->Init(&DLLDriverHook))
	{
		geErrorLog_Add(GE_ERR_DRIVER_INIT_FAILED, NULL);
		geErrorLog_AddString(-1, RDriver->LastErrorStr , NULL);
		return GE_FALSE;
	}

	DrvInfo->Active = GE_TRUE;

	Engine_SetupPixelFormats(Engine); // <> temp hack

	Engine->Changed = GE_TRUE;

	return GE_TRUE;
}

//=====================================================================================
//	geEngine_Prep
//=====================================================================================
static	geBoolean geEngine_Prep(geEngine *Engine)
{
	geBoolean		WorldChanged;
	int32			i;

	assert(Engine);

	// See if any of the attached worlds has changed...
	WorldChanged = GE_FALSE;

	for (i=0; i< Engine->NumWorlds; i++)
	{
		if (Engine->Worlds[i]->Changed)
			WorldChanged = GE_TRUE;
	}

	// Check to see if the world has changed
	if (!WorldChanged && !Engine->Changed)		
		return GE_TRUE;		// Nothing to do if any of the worlds (and engine) has not changed

	// Throw everything off the card...
	if (!geEngine_ResetDriver(Engine))
	{
		geErrorLog_AddString(-1,"geEngine_Prep : geEngine_ResetDriver", NULL);
		return GE_FALSE;
	}
	
	// Attach all the current bitmaps to the current driver
	if (!geEngine_AttachAll(Engine))
	{
		geErrorLog_AddString(-1,"geEngine_Prep : geEngine_AttachAll failed", NULL);
		return GE_FALSE;
	}
	
	// Reset all the changed flags
	geEngine_SetAllWorldChangedFlag(Engine, GE_FALSE);
	Engine->Changed = GE_FALSE;
	
	return GE_TRUE;
}

//=====================================================================================
//	geEngine_RenderWorld
//=====================================================================================
GENESISAPI geBoolean geEngine_RenderWorld(geEngine *Engine, geWorld *World, geCamera *Camera, geFloat Time)
{
	Sys_DriverInfo	*DInfo;
	int32			Width, Height;
 
	assert(Engine != NULL);
	assert(World != NULL);
	assert(Camera != NULL);

	assert(geEngine_HasWorld(Engine, World) == GE_TRUE);	// You have to add the world to the engine before rendering!

	assert(Engine->Changed == GE_FALSE);
	assert(World->Changed == GE_FALSE);

	if (!World || !Camera)
	{
		geErrorLog_Add(GE_ERR_INVALID_PARMS, NULL);
		return GE_FALSE;
	}

	DInfo = &Engine->DriverInfo;

	// This must not be NULL, or there is not a true current driver mode set
	assert(DInfo->CurMode);

	Width = DInfo->CurMode->Width;
	Height = DInfo->CurMode->Height;

	if (Width != -1 && Height != -1)		// If not in window mode...
	{
		geFloat		CameraWidth,CameraHeight;

		geCamera_GetWidthHeight(Camera,&CameraWidth,&CameraHeight);

		if (CameraWidth > Width || CameraHeight > Height)
		{
			geErrorLog_Add(GE_ERR_INVALID_CAMERA, NULL);
			return GE_FALSE;
		}
	}
	
	if (!World_WorldRenderQ(Engine, World, Camera))
		return GE_FALSE;

	return GE_TRUE;
}

//===================================================================================
//	geEngine_BeginFrame
//===================================================================================
GENESISAPI geBoolean geEngine_BeginFrame(geEngine *Engine, geCamera *Camera, geBoolean ClearScreen)
{
	RECT	DrvRect, *pDrvRect;
	geRect	gDrvRect;

	assert(Engine != NULL);
	
	assert(Engine->FrameState == FrameState_None);

	Engine->FrameState = FrameState_Begin;
	
	// Make sure the driver is avtive
	if (!Engine->DriverInfo.Active)
	{
		geErrorLog_Add(GE_ERR_DRIVER_NOT_INITIALIZED, NULL);
		return GE_FALSE;
	}
	
	assert(Engine->DriverInfo.RDriver != NULL);

	// Make sure we have everything finalized with this world so the engine can render it
	if (!geEngine_Prep(Engine))
		return FALSE;

	// Do some timing stuff
	QueryPerformanceCounter(&Engine->CurrentTic);

	// Clear some debug info
	memset(&Engine->DebugInfo, 0, sizeof(Engine->DebugInfo));

	if(Camera)
	{
		geCamera_GetClippingRect(Camera, &gDrvRect);
	
		DrvRect.left	=gDrvRect.Left;
		DrvRect.top		=gDrvRect.Top;
		DrvRect.right	=gDrvRect.Right;
		DrvRect.bottom	=gDrvRect.Bottom;

		pDrvRect = &DrvRect;
	}
	else
		pDrvRect = NULL;

	if (!Engine->DriverInfo.RDriver->BeginScene( ClearScreen , TRUE, pDrvRect))
	{
		geErrorLog_Add(GE_ERR_DRIVER_BEGIN_SCENE_FAILED, NULL);
		return GE_FALSE;
	}

	return GE_TRUE;
}

extern int32	NumExactCast;
extern int32	NumBBoxCast;
extern int32	NumGetContents;

//===================================================================================
//	geEngine_EndFrame
//===================================================================================
GENESISAPI geBoolean geEngine_EndFrame(geEngine *Engine)
{
	LARGE_INTEGER		NowTic, DeltaTic;
	float				Fps;
	//DRV_Debug			*Debug;

	assert(Engine != NULL);
	
	assert(Engine->FrameState == FrameState_Begin);

	Engine->FrameState = FrameState_None;

	if (!Engine->DriverInfo.Active)
	{
		geErrorLog_Add(GE_ERR_DRIVER_NOT_INITIALIZED, NULL);
		return GE_FALSE;
	}
	
	assert(Engine->DriverInfo.RDriver != NULL);

	//Debug = (DRV_Debug*)Engine->DriverInfo.RDriver->LoadMiscTexture;
	//geEngine_Printf(Engine, 2, 20, "LMap Cycles = %i", Debug->LMapCount[0][0]);

	Engine_DrawFontBuffer(Engine);

	if (!Engine->DriverInfo.RDriver->EndScene())
	{
		geErrorLog_Add(GE_ERR_DRIVER_END_SCENE_FAILED, NULL);
		return GE_FALSE;
	}

	QueryPerformanceCounter(&NowTic);
	//CurrentFrequency = ((float)PR_EntireFrame.ElapsedCycles/200.0f)

	SubLarge(&Engine->CurrentTic, &NowTic, &DeltaTic);

	if (DeltaTic.LowPart > 0)
		Fps =  (float)Engine->CPUInfo.Freq / (float)DeltaTic.LowPart;
	else 
		Fps = 100.0f;

	if (Engine->DisplayFrameRateCounter == GE_TRUE)			// Dieplay debug info
	{
		#define				MAX_FPS_ARRAY		20

		float				AverageFps;
		DRV_CacheInfo		*pCacheInfo;
		static float		FpsArray[MAX_FPS_ARRAY];
		static int32		NumFps = 0, i;

		// Changed Average Fps to go accross last n frames, JP...
		FpsArray[(NumFps++) % MAX_FPS_ARRAY] = Fps;
		
		for (AverageFps = 0.0f, i=0; i<MAX_FPS_ARRAY; i++)
			AverageFps += FpsArray[i];

		AverageFps *= (1.0f/(float)MAX_FPS_ARRAY);

		// Grab some driver debug info
		Engine->DebugInfo.RenderedPolys = Engine->DriverInfo.RDriver->NumRenderedPolys;

		geEngine_Printf(Engine, 2,2+15*0, "Fps    : %2.2f / %2.2f", Fps, AverageFps);
		geEngine_Printf(Engine, 2,2+15*1, "Polys  : %4i/%4i/%4i", Engine->DebugInfo.TraversedPolys, Engine->DebugInfo.SentPolys, Engine->DebugInfo.RenderedPolys);

		geEngine_Printf(Engine, 2,2+15*2, "Mirrors: %3i", Engine->DebugInfo.NumMirrors);

		pCacheInfo = Engine->DriverInfo.RDriver->CacheInfo;

		if (pCacheInfo)
		{
			geEngine_Printf(Engine, 2, 2+15*3, "Cache:  %4i/%4i/%4i/%4i/%4i", 
											pCacheInfo->CacheFull,
											pCacheInfo->CacheRemoved,
											pCacheInfo->CacheFlushes,
											pCacheInfo->TexMisses,
											pCacheInfo->LMapMisses);
																			
		}

		geEngine_Printf(Engine, 2,2+15*4, "Actors : %3i, Models: %3i", Engine->DebugInfo.NumActors, Engine->DebugInfo.NumModels);
		geEngine_Printf(Engine, 2,2+15*5, "DLights: %3i", Engine->DebugInfo.NumDLights);
		geEngine_Printf(Engine, 2,2+15*6, "Fog    : %3i", Engine->DebugInfo.NumFog);
		geEngine_Printf(Engine, 2,2+15*7, "LMap1  : %3i, LMap2  : %3i", Engine->DebugInfo.LMap1, Engine->DebugInfo.LMap2);

		// For now, just display debug info for the first world...
		if (Engine->NumWorlds)
		{
			geWorld_DebugInfo	*Info;
			
			Info = &Engine->Worlds[0]->DebugInfo;
			geEngine_Printf(Engine, 2, 2+15*8, "Nodes: %3i/%3i, Leafs: %3i/%3i, Userp: %3i/%3i", Info->NumNodesTraversed1, Info->NumNodesTraversed2, Info->NumLeafsHit1, Info->NumLeafsHit2, Info->NumLeafsWithUserPolys, Info->NumUserPolys);
			geEngine_Printf(Engine, 2, 2+15*9, "Cast: %3i/%3i, GetC: %3i: %i",  NumExactCast, NumBBoxCast, NumGetContents);

			memset(Info, 0, sizeof(*Info));

			NumExactCast = 0;
			NumBBoxCast = 0;
			NumGetContents = 0;

		}
	}

	// Do an engine frame
	Engine_Tick(Engine);

	return GE_TRUE;
}

//===================================================================================
//	Engine_Tick
//===================================================================================
static void Engine_Tick(geEngine *Engine)
{
	int32		i;

	for (i=0; i< 20; i++)
	{
		if (Engine->WaveDir[i] == 1)
			Engine->WaveTable[i] += 14;
		else
			Engine->WaveTable[i] -= 14;
		if (Engine->WaveTable[i] < 50)
		{
			Engine->WaveTable[i] += 14;
			Engine->WaveDir[i] = 1;
		}
		if (Engine->WaveTable[i] > 255)
		{
			Engine->WaveTable[i] -= 14;
			Engine->WaveDir[i] = 0;
		}
	}
}

//===================================================================================
//	Engine_DrawFontBuffer
//===================================================================================
static void Engine_DrawFontBuffer(geEngine *Engine)
{
	geRect			Rect;
	int32			i, x, y, w, StrLength;
	Sys_FontInfo	*Fi;
	char			*Str;
	int32			FontWidth,FontHeight;
	
	Fi = &Engine->FontInfo;

	if ( Fi->NumStrings == 0) 
		return;

	FontWidth	= 8;
	FontHeight	= 15;
		
	for (i=0; i< Fi->NumStrings; i++)
	{
		x = Fi->ClientStrings[i].x;
		y = Fi->ClientStrings[i].y;
		Str = Fi->ClientStrings[i].String;
		StrLength = strlen(Str);

		for (w=0; w< StrLength; w++)
		{
			/*
			Rect.left = (Fi->FontLUT1[*Str]>>16)+1;
			Rect.right = Rect.left+16;
			Rect.top = (Fi->FontLUT1[*Str]&0xffff)+1;
			Rect.bottom = Rect.top+16;
			*/
			Rect.Left = (Fi->FontLUT1[*Str]>>16);
			Rect.Right = Rect.Left + FontWidth - 1;
			Rect.Top = (Fi->FontLUT1[*Str]&0xffff);
			Rect.Bottom = Rect.Top + FontHeight - 1;

			if ( ! geEngine_DrawBitmap(Engine, Fi->FontBitmap, &Rect, x, y) )
			{
				geEngine_Printf(Engine, 10, 50, "Could not draw font...\n");
				geErrorLog_AddString(-1,"DrawFontBuffer : Could not draw font...\n", NULL);
			}
			//x+= 16;
			x += FontWidth;
			Str++;
		}
	}

	Fi->NumStrings = 0;
}

static void SubLarge(LARGE_INTEGER *start, LARGE_INTEGER *end, LARGE_INTEGER *delta)
{
	_asm {
		mov ebx,dword ptr [start]
		mov esi,dword ptr [end]

		mov eax,dword ptr [esi+0]
		sub eax,dword ptr [ebx+0]

		mov edx,dword ptr [esi+4]
		sbb edx,dword ptr [ebx+4]

		mov ebx,dword ptr [delta]
		mov dword ptr [ebx+0],eax
		mov dword ptr [ebx+4],edx
	}
}


//===================================================================================
// geEngine_Puts
//===================================================================================
geBoolean geEngine_Puts(geEngine *Engine, int32 x, int32 y, const char *String)
{
	Sys_FontInfo	*Fi;

	Fi = &Engine->FontInfo;

	if (strlen(String) > MAX_CLIENT_STRING_LEN)
		return GE_FALSE;
					 
	if (Fi->NumStrings >= MAX_CLIENT_STRINGS)
		return GE_FALSE;

	strcpy(Fi->ClientStrings[Fi->NumStrings].String, String);

	Fi->ClientStrings[Fi->NumStrings].x = x;	
	Fi->ClientStrings[Fi->NumStrings].y = y;

	Fi->NumStrings++;

	return TRUE;
}

//========================================================================================
//	geEngine_Printf
//========================================================================================
GENESISAPI geBoolean geEngine_Printf(geEngine *Engine, int32 x, int32 y, const char *String, ...)
{
	va_list			ArgPtr;
    char			TempStr[1024];

	va_start(ArgPtr, String);
    vsprintf(TempStr, String, ArgPtr);
	va_end(ArgPtr);

	return geEngine_Puts(Engine, x, y, TempStr);
}



//========================================================================================
//========================================================================================
static BOOL Hack_EnumCallBack(const geRDriver_PixelFormat *Format, void *Context)
{
geRDriver_PixelFormat ** pPixelArrayPtr;
	pPixelArrayPtr = Context;
#if 0
	*(*pPixelArrayPtr)++ = *Format;
#else
	**pPixelArrayPtr = *Format;
	(*pPixelArrayPtr) += 1;
#endif
return 1;
}

static geRDriver_PixelFormat * Hack_FindPixelFormat(geRDriver_PixelFormat * PixelFormats,int ArrayLen,uint32 Flags,geBoolean NeedsAlpha)
{
int cnt;
geRDriver_PixelFormat * pf;

	for(cnt=ArrayLen,pf = PixelFormats;cnt--;pf++)
	{
		if ( ! NeedsAlpha || ( NeedsAlpha && (gePixelFormat_HasAlpha(pf->PixelFormat) || pf->Flags & RDRIVER_PF_HAS_ALPHA) ) )
		{
			if( pf->Flags == Flags )
			{
				return pf;
			}
		}
	}
	for(cnt=ArrayLen,pf = PixelFormats;cnt--;pf++)
	{
		if ( ! NeedsAlpha || ( NeedsAlpha && (gePixelFormat_HasAlpha(pf->PixelFormat) || pf->Flags & RDRIVER_PF_HAS_ALPHA) ) )
		{
			if( pf->Flags & Flags )
			{
				return pf;
			}
		}
	}

	for(cnt=ArrayLen,pf = PixelFormats;cnt--;pf++)
	{
		if( pf->Flags == Flags )
		{
			return pf;
		}
	}
	for(cnt=ArrayLen,pf = PixelFormats;cnt--;pf++)
	{
		if( pf->Flags & Flags )
		{
			return pf;
		}
	}

	return NULL;
}

geBoolean Engine_SetupPixelFormats(geEngine *Engine)
{
	geRDriver_PixelFormat PixelFormatsArray[100],*PixelArrayPtr;
	int PixelFormatsLen;

	PixelArrayPtr = PixelFormatsArray;

	Engine->DriverInfo.RDriver->EnumPixelFormats(Hack_EnumCallBack , &PixelArrayPtr);

	PixelFormatsLen = ((uint32)PixelArrayPtr - (uint32)PixelFormatsArray)/sizeof(geRDriver_PixelFormat);
	assert(PixelFormatsLen > 0);

	#define SetupPF( type, flag, alpha )	\
		if ( PixelArrayPtr = Hack_FindPixelFormat(PixelFormatsArray,PixelFormatsLen,flag,alpha) )	\
		{													\
			Engine->PixelFormats[type] = *PixelArrayPtr;	\
			Engine->HasPixelFormat[type] = GE_TRUE;			\
		}													\
		else												\
		{													\
			Engine->HasPixelFormat[type] = GE_FALSE;		\
		}
		
	SetupPF( ENGINE_PF_WORLD,		RDRIVER_PF_COMBINE_LIGHTMAP,0);
	SetupPF( ENGINE_PF_LIGHTMAP,	RDRIVER_PF_LIGHTMAP,0);
	SetupPF( ENGINE_PF_USER,		RDRIVER_PF_3D,0);
	SetupPF( ENGINE_PF_USER_ALPHA,	RDRIVER_PF_3D,1);
	SetupPF( ENGINE_PF_DECAL,		RDRIVER_PF_2D,0);
	SetupPF( ENGINE_PF_PALETTE,		RDRIVER_PF_PALETTE,0);
	SetupPF( ENGINE_PF_ALPHA_CHANNEL,RDRIVER_PF_ALPHA,0);

	return GE_TRUE;
}

//=====================================================================================
//	Engine_CreateTHandle
//=====================================================================================

geRDriver_THandle * Engine_CreateTHandle(geEngine *Engine,int Width,int Height,int Mips, int EngineTexType)
{
geRDriver_THandle * THandle;

	if ( ! Engine->HasPixelFormat[EngineTexType] )
	{
		//{} assert( Engine->HasPixelFormat[EngineTexType] );
		return NULL;
	}

	THandle = Engine->DriverInfo.RDriver->THandle_Create(Width,Height,Mips,&(Engine->PixelFormats[EngineTexType]));

	if (! THandle )
		return NULL;

	if ( gePixelFormat_HasPalette(Engine->PixelFormats[EngineTexType].PixelFormat) )
	{
		geRDriver_THandle * PalHandle;
		PalHandle = Engine->DriverInfo.RDriver->THandle_Create(256,1,1,&(Engine->PixelFormats[ENGINE_PF_PALETTE]));
		if ( ! PalHandle )
		{
			Engine->DriverInfo.RDriver->THandle_Destroy(THandle);
			return NULL;
		}
		Engine->DriverInfo.RDriver->THandle_SetPalette(THandle,PalHandle);

		#ifdef _DEBUG
		{
		geRDriver_THandleInfo Info;
		(Engine->DriverInfo.RDriver)->THandle_GetInfo(PalHandle,0,&Info);
		assert(Info.Width == 256);
		assert(Info.Height == 1);
		}
		#endif
	}

	if ( (Engine->PixelFormats[EngineTexType].Flags) & RDRIVER_PF_HAS_ALPHA )
	{
		geRDriver_THandle * AlphaHandle;
		assert( Engine->PixelFormats[ENGINE_PF_ALPHA_CHANNEL].Flags & RDRIVER_PF_ALPHA );
		assert( ! (Engine->PixelFormats[ENGINE_PF_ALPHA_CHANNEL].Flags & RDRIVER_PF_HAS_ALPHA) );
		AlphaHandle = Engine_CreateTHandle(Engine,Width,Height,Mips,ENGINE_PF_ALPHA_CHANNEL);
		if ( ! AlphaHandle )
		{
			Engine->DriverInfo.RDriver->THandle_Destroy(THandle);
			return NULL;
		}
		Engine->DriverInfo.RDriver->THandle_SetAlpha(THandle,AlphaHandle);
	}

return THandle;
}

void Engine_DestroyTHandle(geEngine *Engine,geRDriver_THandle * THandle)
{
	Engine->DriverInfo.RDriver->THandle_Destroy(THandle);
}
