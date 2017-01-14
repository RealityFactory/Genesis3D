/****************************************************************************************/
/*  GlideDrv.c                                                                          */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Distributes work to other modules.  This is the main GlideDrv file.    */
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

#include "DCommon.h"
#include "GlideDrv.h"
#include "GMain.h"
#include "Glide.h"
#include "GTHandle.h"
#include "Render.h"

int32				LastError;
char				LastErrorStr[255];

static float		CurrentGamma;

DRV_EngineSettings	EngineSettings;
#define ENABLE_WIREFRAME	

//================================================================================================
//================================================================================================
geBoolean DRIVERCC DrvInit(DRV_DriverHook *Hook)
{
	#ifdef ENABLE_WIREFRAME	
	{
		// reset key states (compensate for windows bug)
		uint32 KeyState1, KeyState2;
		#pragma message("Glide : WireFrame enabled!")
		KeyState1 = GetAsyncKeyState(VK_CONTROL);
		KeyState2 = GetAsyncKeyState(VK_F8);
	}
	#endif

	return GMain_Startup(Hook);
}

//================================================================================================
//================================================================================================
geBoolean DRIVERCC DrvShutdown(void)
{
	GMain_Shutdown();

	return TRUE;
}

//================================================================================================
//================================================================================================
geBoolean DRIVERCC DrvResetAll(void)
{
	return GMain_ResetAll();
}

//================================================================================================
//================================================================================================
geBoolean DRIVERCC SetGamma(float Gamma)
{
	CurrentGamma = Gamma;

	//grGammaCorrectionValue(CurrentGamma);
	return TRUE;
}

//================================================================================================
//================================================================================================
geBoolean DRIVERCC GetGamma(float *Gamma)
{
	//*Gamma = CurrentGamma;
	*Gamma = 1.0f;

	return TRUE;
}

//================================================================================================
//================================================================================================
geBoolean DRIVERCC EnumModes(int32 Driver, char *DriverName, DRV_ENUM_MODES_CB *Cb, void *Context)
{
	//if (!GMain_GetBoardInfo(&g_BoardInfo))
	//	return GE_TRUE;

	if (g_BoardInfo.MainRam == 0)
		return GE_TRUE;				// No modes

	if (g_BoardInfo.MainRam >= 1)
	{
		if (!Cb(0, "512x384", 512, 384, Context))
			return GE_TRUE;
	}

	if (g_BoardInfo.MainRam >= 2)
	{
		if (!Cb(1, "640x480", 640, 480, Context))
			return GE_TRUE;
	}

	if (g_BoardInfo.MainRam >= 4)
	{
		if (!Cb(2, "800x600", 800, 600, Context))
			return GE_TRUE;
	}

	if (g_BoardInfo.MainRam > 4)
	{
		if (!Cb(3, "1024x768", 1024, 768, Context))
			return GE_TRUE;
	}

	return GE_TRUE;
}

//================================================================================================
//================================================================================================
geBoolean DRIVERCC EnumSubDrivers(DRV_ENUM_DRV_CB *Cb, void *Context)
{
	// Initialize the Glide library 
	grGlideInit();

	// Get the info about this board
	if (!GMain_GetBoardInfo(&g_BoardInfo))
		return GE_TRUE;

	if (g_BoardInfo.MainRam == 0)
		return GE_TRUE;				// No modes, so don't return any drivers

	if (!Cb(0, "Glide Driver v"DRV_VMAJS"."DRV_VMINS".", Context))
		return GE_TRUE;

	// Shutdown Glide library
	grGlideShutdown();

	return GE_TRUE;
}

geRDriver_PixelFormat	PixelFormats[] =
{
	{GE_PIXELFORMAT_8BIT			, RDRIVER_PF_3D | RDRIVER_PF_COMBINE_LIGHTMAP},
	{GE_PIXELFORMAT_16BIT_4444_ARGB	, RDRIVER_PF_3D | RDRIVER_PF_COMBINE_LIGHTMAP},
	{GE_PIXELFORMAT_16BIT_565_RGB	, RDRIVER_PF_3D | RDRIVER_PF_COMBINE_LIGHTMAP},
	{GE_PIXELFORMAT_8BIT			, RDRIVER_PF_3D},
	{GE_PIXELFORMAT_16BIT_565_RGB	, RDRIVER_PF_2D | RDRIVER_PF_CAN_DO_COLORKEY},
	{GE_PIXELFORMAT_16BIT_565_RGB	, RDRIVER_PF_LIGHTMAP},
	{THANDLE_PALETTE_FORMAT			, RDRIVER_PF_PALETTE},
	{GE_PIXELFORMAT_16BIT_1555_ARGB	, RDRIVER_PF_3D | RDRIVER_PF_COMBINE_LIGHTMAP},
};

#define NUM_PIXEL_FORMATS	(sizeof(PixelFormats)/sizeof(geRDriver_PixelFormat))

//================================================================================================
//================================================================================================
geBoolean DRIVERCC EnumPixelFormats(DRV_ENUM_PFORMAT_CB *Cb, void *Context)
{
	int32		i;

	// Then hand them off to the caller
	for (i=0; i<NUM_PIXEL_FORMATS; i++)
	{
		if (!Cb(&PixelFormats[i], Context))
			return GE_TRUE;
	}

	return GE_TRUE;
}

geBoolean	DRIVERCC DrvUpdateWindow(void)
{
	return	GE_TRUE;
}
geBoolean	DRIVERCC DrvSetActive(geBoolean Active)
{
	return	GE_TRUE;
}

//================================================================================================
//================================================================================================
DRV_Driver GLIDEDRV = 
{
	"Glide driver. v"DRV_VMAJS"."DRV_VMINS". Copyright 1999, WildTangent Inc.; All Rights Reserved.",
	DRV_VERSION_MAJOR,
	DRV_VERSION_MINOR,

	DRV_ERROR_NONE,
	NULL,
	
	EnumSubDrivers,
	EnumModes,
	
	EnumPixelFormats,

	DrvInit,
	DrvShutdown,
	DrvResetAll,
	DrvUpdateWindow,
	DrvSetActive,

	GTHandle_Create,
	GTHandle_Destroy,

	GTHandle_Lock,
	GTHandle_UnLock,

	GThandle_SetPal,
	GThandle_GetPal,

	NULL,	//SetAlpha
	NULL,	//GetAlpha

	GTHandle_GetInfo,

	BeginScene,
	EndScene,
	BeginWorld,
	EndWorld,
	BeginMeshes,
	EndMeshes,
	BeginModels,
	EndModels,

	Render_GouraudPoly,
	Render_WorldPoly,
	Render_MiscTexturePoly,

	Render_DrawDecal,

	0,0,0,

	&CacheInfo,

	GMain_ScreenShot,

	SetGamma, 
	GetGamma,

	GMain_SetFogEnable,

	NULL,
	NULL,								// Init to NULL, engine SHOULD set this (SetupLightmap)
	NULL
};

//================================================================================================
//================================================================================================

DllExport geBoolean DriverHook(DRV_Driver **Driver)
{
	EngineSettings.CanSupportFlags = (DRV_SUPPORT_ALPHA | DRV_SUPPORT_COLORKEY);
	EngineSettings.PreferenceFlags = 0;

	GLIDEDRV.EngineSettings	= &EngineSettings;

	*Driver = &GLIDEDRV;

	// Make sure the error string ptr is not null, or invalid!!!
	GLIDEDRV.LastErrorStr = LastErrorStr;

	SetLastDrvError(DRV_ERROR_NONE, "GLIDE_DRV:  No error.");

	return GE_TRUE;
}

//================================================================================================
//================================================================================================
void SetLastDrvError(int32 Error, char *ErrorStr)
{
	LastError = Error;
	
	if (ErrorStr)
	{
		strcpy(LastErrorStr, ErrorStr);
	}
	else
		LastErrorStr[0] = 0;

	GLIDEDRV.LastErrorStr = LastErrorStr;
	GLIDEDRV.LastError = LastError;
}
