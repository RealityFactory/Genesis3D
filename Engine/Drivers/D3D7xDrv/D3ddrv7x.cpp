/****************************************************************************************/
/*  D3DDrv7x.cpp                                                                        */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: D3D driver                                                             */
/*                                                                                      */
/*  Edit History:                                                                       */
/*  01/04/2004 Wendell Buckner                                                          */ 
/*   CONFIG DRIVER - Make the driver configurable by "ini" file settings                */
/*  05/19/2003 Wendell Buckner                                                          */
/*   BUMPMAPPING                                                                        */
/*  04/01/2003 Wendell Buckner                                                          */
/*   BUMPMAPPING                                                                        */
/*  03/31/2003 Wendell Buckner                                                          */
/*   BUMPMAPPING                                                                        */
/*  01/28/2003 Wendell Buckner                                                          */                  
/*    Cache decals so that they can be drawn after all the 3d stuff...                  */                  
/*  12/28/2002 Wendell Buckner                                                          */
/*    Allow/make 32-bit (ARGB) mode the default mode...                                 */
/*    Give out the true 24-bit surface as well...                                       */
/*    Make sure the standard 16-bit is available...                                     */
/*  07/16/2000 Wendell Buckner                                                          */
/*   Convert to Directx7...                                                             */
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
/*  Genesis3D Version 1.1 released November 15, 1999                                    */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved                            */
/*                                                                                      */
/****************************************************************************************/
#include <Windows.h>
#include <stdio.h>

/* 07/16/2000 Wendell Buckner
    Convert to Directx7...    
#include "D3DDrv.h"           */
#include "D3DDrv7x.h"           

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

/* 01/04/2004 Wendell Buckner
    CONFIG DRIVER - Make the driver configurable by "ini" file settings */
extern int ExtraTextures;

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

/* 04/01/2003 Wendell Buckner
    BUMPMAPPING
geRDriver_PixelFormat	PixelFormat[10]; */
#define MAX_PIXEL_FORMATS 25             
geRDriver_PixelFormat	PixelFormat[MAX_PIXEL_FORMATS];


#define NUM_PIXEL_FORMATS (sizeof(PixelFormats)/sizeof(geRDriver_PixelFormat))

geBoolean DRIVERCC EnumPixelFormats(DRV_ENUM_PFORMAT_CB *Cb, void *Context)
{
	int32			i;
	gePixelFormat	Format3d, Format2d;
	uint32			CurrentBpp;
	int32			j;
    DDSURFACEDESC2 NoSurfDesc;

    memset(&NoSurfDesc, 0, sizeof(DDSURFACEDESC2));

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

/* 12/28/2002 Wendell Buckner
    Allow/make 32-bit (ARGB) mode the default mode...  
	if (AppInfo.ddTexFormat.ddpfPixelFormat.dwGBitMask == (31<<5)) */
    if (CurrentBpp == 32 && AppInfo.ddTexFormat.ddpfPixelFormat.dwRGBAlphaBitMask == 0xff000000)		
		Format3d = GE_PIXELFORMAT_32BIT_ARGB;

	else if (CurrentBpp == 32 && AppInfo.ddTexFormat.ddpfPixelFormat.dwBBitMask == 0xff)
		Format2d = GE_PIXELFORMAT_32BIT_XRGB;

/* 12/28/2002 Wendell Buckner
    Give out the true 24-bit surface as well...  */
	else if (CurrentBpp == 24 && AppInfo.ddTexFormat.ddpfPixelFormat.dwBBitMask == 0xff)
		Format3d = GE_PIXELFORMAT_24BIT_RGB;

	// Setup the 3d (Texture) format
	else if (AppInfo.ddTexFormat.ddpfPixelFormat.dwGBitMask == (31<<5))
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

/* 12/28/2002 Wendell Buckner
    Make sure the standard 16-bit is available...  	 */
//  THIS KILLED ME BUT IT'S FIXED NOW, I NEED TO CHECK AND MAKE SURE IT'S 555 OR 565 THE CALLING
//  PROGRAM KNOWS THE DIFFERENCE...	
	// Setup the 3d (Texture) format
	if (AppInfo.ddTexFormat16.ddpfPixelFormat.dwGBitMask == (31<<5))
	 PixelFormat[5].PixelFormat = GE_PIXELFORMAT_16BIT_555_RGB;
	else
 	 PixelFormat[5].PixelFormat = GE_PIXELFORMAT_16BIT_565_RGB;
	
	PixelFormat[5].Flags = RDRIVER_PF_3D | RDRIVER_PF_COMBINE_LIGHTMAP;

    j = 6;

/* 12/28/2002 Wendell Buckner
    Give out the true 24-bit or 32-bit XRGB surface as well...  */
    if ( (AppInfo.ddTexFormat24.ddpfPixelFormat.dwRGBBitCount == 32) && (CurrentBpp == 32) )
	{	 
	 PixelFormat[j].PixelFormat = GE_PIXELFORMAT_32BIT_XRGB;;	// 3d X888 surface
	 PixelFormat[j].Flags = RDRIVER_PF_3D | RDRIVER_PF_COMBINE_LIGHTMAP;	 
	 j++;
    }

/* 12/28/2002 Wendell Buckner
    Allow/make 32-bit (ARGB) mode the default mode...  */
    if ( (AppInfo.ddTexFormat32.ddpfPixelFormat.dwRGBBitCount == 32) && (CurrentBpp == 32) )
	{	 
	 PixelFormat[j].PixelFormat = GE_PIXELFORMAT_32BIT_ARGB;	// 3d 8888 surface
	 PixelFormat[j].Flags = RDRIVER_PF_3D | RDRIVER_PF_COMBINE_LIGHTMAP;
	 j++;
	}

/* 01/04/2004 Wendell Buckner
    CONFIG DRIVER - Make the driver configurable by "ini" file settings */
	if( ExtraTextures )
	{

/* 03/31/2003 Wendell Buckner
    BUMPMAPPING */
    if ( (AppInfo.ddBumpMapNoLuminance.ddpfPixelFormat.dwBumpBitCount == 16)  )
	{	 
	 PixelFormat[j].PixelFormat = GE_PIXELFORMAT_16BIT_88_UV;	// 3d 88 bump surface
	 PixelFormat[j].Flags = RDRIVER_PF_3D | RDRIVER_PF_BUMPMAP;
	 j++;
	}

    if ( (AppInfo.ddBumpMapSixBitLuminance.ddpfPixelFormat.dwBumpBitCount == 16)  )
	{	 
	 PixelFormat[j].PixelFormat = GE_PIXELFORMAT_16BIT_556_UVL;	// 3d 556 bump surface
	 PixelFormat[j].Flags = RDRIVER_PF_3D | RDRIVER_PF_BUMPMAP;
	 j++;
	}

    if ( (AppInfo.ddBumpMapEightBitLuminance.ddpfPixelFormat.dwBumpBitCount == 24)  )
	{	 
	 PixelFormat[j].PixelFormat = GE_PIXELFORMAT_24BIT_888_UVL;	// 3d 888 bump surface
	 PixelFormat[j].Flags = RDRIVER_PF_3D | RDRIVER_PF_BUMPMAP;
	 j++;
	}

/* 01/04/2004 Wendell Buckner
    CONFIG DRIVER - Make the driver configurable by "ini" file settings */
	}

	// Then hand them off to the caller
/* 12/28/2002 Wendell Buckner
    Give out the true 24-bit or 32-bit XRGB surface as well...  
	for (i=0; i<5; i++)   */
	for (i=0; i<j; i++)
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
// changed QD Shadows
	BeginShadowVolumes,
	EndShadowVolumes,
	1,
// end change

	RenderGouraudPoly,
	RenderWorldPoly,
	RenderMiscTexturePoly,
// changed QD Shadows
	RenderStencilPoly,
	DrawShadowPoly,
// end change
/*  01/28/2003 Wendell Buckner                                                          
     Cache decals so that they can be drawn after all the 3d stuff...                   
  	DrawDecal,                                                                          */
    DrawDecalRect,

	0,0,0,
	
	&CacheInfo,

	ScreenShot,

	SetGamma,
	GetGamma,

	D3DMain_SetFogEnable,

	NULL,
	NULL,								// Init to NULL, engine SHOULD set this (SetupLightmap)
	NULL,

/* 05/19/2003 Wendell Buckner 
    BUMPMAPPING */
	THandle_Combine,
    THandle_UnCombine,
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
   DDSURFACEDESC2 ddsd;
   BITMAPFILEHEADER bfh;
   BITMAPINFOHEADER bih;
   HRESULT result;
   HDC surfDC = NULL;
   HDC memDC = NULL;
   HBITMAP bitmap = NULL;
   HGDIOBJ oldbit = NULL;
   FILE *file = NULL;
   void *data= NULL;
   int width, height, bpp;
   int datasize;
   BOOL success = FALSE;

   memset(&ddsd,0,sizeof(ddsd));
   ddsd.dwSize = sizeof(ddsd);
   result = AppInfo.lpBackBuffer->GetSurfaceDesc(&ddsd);

   if (FAILED(result))
      goto cleanup;

   width = ddsd.dwWidth;
   height= ddsd.dwHeight;
   bpp = ddsd.ddpfPixelFormat.dwRGBBitCount / 8;

   if (bpp < 2) 
      bpp = 2;

   if (bpp > 3)
      bpp = 3;

   datasize = width * bpp * height;

   if (width * bpp % 4)
      datasize += height * (4 - width * bpp % 4);

   memset((void*)&bfh, 0, sizeof(bfh));

   bfh.bfType = 'B'+('M'<<8);
   bfh.bfSize = sizeof(bfh) + sizeof(bih) + datasize;
   bfh.bfOffBits = sizeof(bfh) + sizeof(bih);
                            
   memset((void*)&bih, 0, sizeof(bih));

   bih.biSize = sizeof(bih);
   bih.biWidth = ddsd.dwWidth;
   bih.biHeight = ddsd.dwHeight;
   bih.biPlanes = 1;
   bih.biBitCount = (unsigned short)(bpp * 8);
   bih.biCompression = BI_RGB;
                            
   result = AppInfo.lpBackBuffer->GetDC(&surfDC);

   if (FAILED(result))
      goto cleanup;

   bitmap = CreateDIBSection(NULL, (BITMAPINFO *)&bih, DIB_RGB_COLORS, &data, NULL, 0);

   if (!bitmap)
      goto cleanup;

   if (!data)
      goto cleanup;

   memDC = CreateCompatibleDC(surfDC);

   if (!memDC)
      goto cleanup;

   oldbit = SelectObject(memDC, bitmap);

   if (!oldbit || FAILED(oldbit))
      goto cleanup;

   result = BitBlt(memDC, 0, 0, width, height, surfDC, 0, 0, SRCCOPY);

   if (!result)
      goto cleanup;

   AppInfo.lpBackBuffer->ReleaseDC(surfDC); 
   surfDC = NULL;                           
   file = fopen(Name, "wb");

   if (!file)
      goto cleanup;
                          
   fwrite((void*)&bfh, sizeof(bfh), 1, file);
   fwrite((void*)&bih, sizeof(bih), 1, file);
   fwrite((void*)data, 1, datasize, file);

   success = TRUE;
                           
cleanup:
                          
   if (oldbit && !FAILED(oldbit))
      SelectObject(memDC, oldbit);

   if (memDC)
      DeleteDC(memDC);
 
   if (surfDC)
      AppInfo.lpBackBuffer->ReleaseDC(surfDC);
 
   if (bitmap)
      DeleteObject(bitmap);
 
   if (file)
      fclose(file);

   return success;
}
