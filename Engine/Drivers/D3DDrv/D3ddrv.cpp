/****************************************************************************************/
/*  D3DDrv.cpp                                                                          */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: D3D driver                                                             */
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
#include <Windows.h>
#include <stdio.h>

#include "D3DDrv.h"
#include "DCommon.h"

#include "Scene.h"
#include "Render.h"
#include "D3DCache.h"
#include "D3D_Main.h"
#include "PCache.h"
#include "THandle.h"


DRV_Window			ClientWindow;
BOOL				ExitHandlerActive = FALSE;

int32				LastError;
char				LastErrorStr[200];

geBoolean	DRIVERCC DrvShutdown(void);
geBoolean	DRIVERCC ScreenShot(const char *Name);

BOOL DRIVERCC DrvInit(DRV_DriverHook *Hook)
{
	RECT	WRect;

	// Start up
	if (!D3DMain_InitD3D(Hook->hWnd, Hook->DriverName+5, Hook->Width, Hook->Height))
	{
		//SetLastDrvError(DRV_ERROR_INIT_ERROR, "D3D_DrvInit: Could not init driver.\n");
		return FALSE;
	}
	
	// If they are asking for a window mode, use there hWnd for the size
	if (Hook->Width ==-1 && Hook->Height == -1)
	{
		GetClientRect(Hook->hWnd, &WRect);
		
		Hook->Width = (WRect.right - WRect.left);
		Hook->Height = (WRect.bottom - WRect.top);
	}

	ClientWindow.Width = Hook->Width;
	ClientWindow.Height = Hook->Height;
	ClientWindow.hWnd = Hook->hWnd;

	return TRUE;
}

//============================================================================================
//============================================================================================
BOOL DRIVERCC DrvShutdown(void)
{
	D3DMain_ShutdownD3D();
	return TRUE;
}

//============================================================================================
//	DrvResetAll
//============================================================================================
geBoolean DRIVERCC DrvResetAll(void)
{
	return D3DMain_Reset();
}

geRDriver_PixelFormat	PixelFormat[10]; 

#define NUM_PIXEL_FORMATS (sizeof(PixelFormats)/sizeof(geRDriver_PixelFormat))

geBoolean DRIVERCC EnumPixelFormats(DRV_ENUM_PFORMAT_CB *Cb, void *Context)
{
	int32			i;
	gePixelFormat	Format3d, Format2d;
	uint32			CurrentBpp;

	CurrentBpp = AppInfo.ddsd.ddpfPixelFormat.dwRGBBitCount;

	// Setup the 2d surface format
	if (CurrentBpp == 32 && AppInfo.ddSurfFormat.ddpfPixelFormat.dwRGBAlphaBitMask == 0xff000000)
		Format2d = GE_PIXELFORMAT_32BIT_ARGB;
	else if (CurrentBpp == 32 && AppInfo.ddSurfFormat.ddpfPixelFormat.dwBBitMask == 0xff)
		Format2d = GE_PIXELFORMAT_32BIT_XRGB;
	else if (CurrentBpp == 24 && AppInfo.ddSurfFormat.ddpfPixelFormat.dwBBitMask == 0xff)
		Format2d = GE_PIXELFORMAT_24BIT_RGB;
	else if (AppInfo.ddSurfFormat.ddpfPixelFormat.dwGBitMask == (31<<5))
		Format2d = GE_PIXELFORMAT_16BIT_555_RGB;
	else
		Format2d = GE_PIXELFORMAT_16BIT_565_RGB;

	// Setup the 3d (Texture) format
	if (AppInfo.ddTexFormat.ddpfPixelFormat.dwGBitMask == (31<<5))
		Format3d = GE_PIXELFORMAT_16BIT_555_RGB;
	else
		Format3d = GE_PIXELFORMAT_16BIT_565_RGB;


	// Create the surface formats now
	PixelFormat[0].PixelFormat = Format3d;							// 3d 565/555 surface
	PixelFormat[0].Flags = RDRIVER_PF_3D | RDRIVER_PF_COMBINE_LIGHTMAP;
		
	PixelFormat[1].PixelFormat = GE_PIXELFORMAT_16BIT_4444_ARGB;	// 3d 4444 surface
	PixelFormat[1].Flags = RDRIVER_PF_3D | RDRIVER_PF_COMBINE_LIGHTMAP;

	PixelFormat[2].PixelFormat = Format2d;							// 2d 565/555 surface
	PixelFormat[2].Flags = RDRIVER_PF_2D | RDRIVER_PF_CAN_DO_COLORKEY;

	PixelFormat[3].PixelFormat = Format3d;							// Lightmap 565/555 surface
	PixelFormat[3].Flags = RDRIVER_PF_LIGHTMAP;

	PixelFormat[4].PixelFormat = GE_PIXELFORMAT_16BIT_1555_ARGB;	
	PixelFormat[4].Flags = RDRIVER_PF_3D | RDRIVER_PF_COMBINE_LIGHTMAP;

	// Then hand them off to the caller
	for (i=0; i<5; i++)
	{
		if (!Cb(&PixelFormat[i], Context))
			return GE_TRUE;
	}

	return TRUE;
}

geBoolean DRIVERCC SetGamma(float Gamma)
{
	return GE_TRUE;
}

geBoolean DRIVERCC GetGamma(float *Gamma)
{
	*Gamma = 1.0f;
		
	return GE_TRUE;
}

BOOL DRIVERCC EnumSubDrivers2(DRV_ENUM_DRV_CB *Cb, void *Context);
BOOL DRIVERCC EnumModes2(int32 Driver, char *DriverName, DRV_ENUM_MODES_CB *Cb, void *Context);

DRV_Driver D3DDRV = 
{
    "D3D driver. v"DRV_VMAJS"."DRV_VMINS". Copyright 1999, WildTangent Inc.; All Rights Reserved.",
	DRV_VERSION_MAJOR,
	DRV_VERSION_MINOR,

	DRV_ERROR_NONE,
	NULL,
	
	EnumSubDrivers2,
	EnumModes2,

	EnumPixelFormats,

	DrvInit,
	DrvShutdown,
	DrvResetAll,
	D3DMain_UpdateWindow,
	D3DMain_SetActive,

	THandle_Create,
	THandle_Destroy,

	THandle_Lock,
	THandle_UnLock,

	NULL,			// SetPal
	NULL,			// GetPal

	NULL,			// SetAlpha
	NULL,			// GetAlpha

	THandle_GetInfo,

	BeginScene,
	EndScene,
	BeginWorld,
	EndWorld,
	BeginMeshes,
	EndMeshes,
	BeginModels,
	EndModels,

	RenderGouraudPoly,
	RenderWorldPoly,
	RenderMiscTexturePoly,

	DrawDecal,

	0,0,0,
	
	&CacheInfo,

	ScreenShot,

	SetGamma,
	GetGamma,

	D3DMain_SetFogEnable,

	NULL,
	NULL,								// Init to NULL, engine SHOULD set this (SetupLightmap)
	NULL
};

DRV_EngineSettings EngineSettings;

DllExport BOOL DriverHook(DRV_Driver **Driver)
{
	EngineSettings.CanSupportFlags = (DRV_SUPPORT_ALPHA | DRV_SUPPORT_COLORKEY);
	EngineSettings.PreferenceFlags = 0;//DRV_PREFERENCE_NO_MIRRORS;

	D3DDRV.EngineSettings = &EngineSettings;
    
	*Driver = &D3DDRV;

	// Make sure the error string ptr is not null, or invalid!!!
    D3DDRV.LastErrorStr = LastErrorStr;

	SetLastDrvError(DRV_ERROR_NONE, "D3DDrv:  No error.");

	return TRUE;
}

void SetLastDrvError(int32 Error, char *ErrorStr)
{
	LastError = Error;
	
	if (ErrorStr)
	{
		strcpy(LastErrorStr, ErrorStr);
	}
	else
		LastErrorStr[0] = NULL;

    D3DDRV.LastErrorStr = LastErrorStr;
    D3DDRV.LastError = LastError;
}

BOOL DRIVERCC ScreenShot(const char *Name)
{
	return FALSE;
}

