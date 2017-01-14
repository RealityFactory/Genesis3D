/****************************************************************************************/
/*  D3D_Main.h                                                                          */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: DD/D3D wrapper                                                         */
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
#ifndef D3D_MAIN_H
#define D3D_MAIN_H

#include <Windows.h>
#include <DDraw.h>
#include <D3D.h>

#define INITGUID

#include "DCommon.h"

#define MAX_APP_MODES					50
#define DDMAIN_MAX_D3D_DRIVERS			10
#define DDMAIN_MAX_TEXTURE_FORMATS		128
#define DDMAIN_MAX_SURFACE_FORMATS		128

#define D3DMAIN_LOG_FILENAME			"D3DDrv.Log"

#define MAX_DRIVER_NAME					1024

//================================================================================
//	Structure defs
//================================================================================

typedef struct
{
    char				Name[MAX_DRIVER_NAME];	// Short name of the driver 
    char				About[MAX_DRIVER_NAME];	// Short string about the driver 
    D3DDEVICEDESC		Desc;					// D3DDEVICEDESC for complete information 
    GUID				Guid;					// it's GUID 
    BOOL				IsHardware;				// does this driver represent a hardware device? 
    BOOL				DoesTextures;			// does this driver do texture mapping? 
    BOOL				DoesZBuffer;			// can this driver use a z-buffer? 
    BOOL				CanDoWindow;			// can it render to Window's display depth? 
	BOOL				DoesTransparency;
	BOOL				DoesAlpha;
	BOOL				DoesClamping;
	BOOL				DoesSrcBlending;
	BOOL				DoesDestBlending;

    WORD				MaxTextureBlendStages;
    WORD				MaxSimultaneousTextures;	
	
	BOOL				CanUse;					// We can use this driver
} DDMain_D3DDriver;

typedef struct 
{
    int32				Width;							// width
    int32				Height;							// height
    int32				Bpp;							// bits per pixel 
    BOOL				ThisDriverCanDo;				// == TRUE if d3d driver can render into
} App_Mode;

typedef struct
{
    DDSURFACEDESC2		ddsd;							// DDSURFACEDESC for complete information 
    BOOL				HasOneBitAlpha;	
    BOOL				HasFourBitAlpha;
} DDMain_SurfFormat;

typedef struct
{
	uint32				R[256];
	uint32				G[256];
	uint32				B[256];
	uint32				A[256];
} RGB_LUT;

// App_Info, used for everything global. 
typedef struct
{
	// Window info
	HWND				hWnd;								// Handle to parent Window

	DDSURFACEDESC2		ddsd;

	// Mode that we were in before initializing
	int32				OldWidth;							// Old screen width
	int32				OldHeight;
	int32				OldBpp;

	int32				CurrentWidth;
	int32				CurrentHeight;
	int32				CurrentBpp;

	int32				OldWindowWidth;						// Old client width
	int32				OldWindowHeight;
	int32				WindowXOffset;
	int32				WindowYOffset;

	RECT				OldWindowRect;
	ULONG				OldGWL_STYLE;

	geBoolean			ModeSet;

	char				DDName[MAX_DRIVER_NAME];			// Have no idea how big to make this.  Anyone?

	LPDIRECTDRAW4		lpDD;								// The current initialized DD object
    LPDIRECT3D3		    lpD3D;								// The current initialized D3D object

    LPDIRECTDRAWSURFACE4	lpFrontBuffer;						// front buffer surface 
    LPDIRECTDRAWSURFACE4	lpBackBuffer;						// back buffer surface 
    LPDIRECTDRAWSURFACE4	lpZBuffer;							// z-buffer surface 
	LPDIRECTDRAWCLIPPER lpClipper;							// Clipper in windowed case
    BOOL				BackBufferInVideo;					// back buf in video mem? 
    BOOL				ZBufferInVideo;						// is Z-buf in video mem? 
    LPDIRECT3DDEVICE3	lpD3DDevice;						// D3D device 
    LPDIRECT3DVIEWPORT3	lpD3DViewport;						// D3D viewport

	LPDIRECT3DMATERIAL3	BackgroundMaterial;

	// 2d surface format (for blt'ing to the display)
	DDSURFACEDESC2		ddSurfFormat;						// 555 or 565 surface desc

	// Texture formats (for the D3D device)
	DDSURFACEDESC2		ddTexFormat;						// 555 or 565 surface desc
	DDSURFACEDESC2		ddFourBitAlphaSurfFormat;			// 4444 surface desc
	DDSURFACEDESC2		ddOneBitAlphaSurfFormat;			// 1555 surface desc

	RGB_LUT				Lut1;
	RGB_LUT				Lut2;
	RGB_LUT				Lut3;

	BOOL				IsPrimary;							// 
	BOOL				FullScreen;
	
	int32				NumModes;
	App_Mode			Modes[MAX_APP_MODES];

	int32				NumDrivers;
	DDMain_D3DDriver	Drivers[DDMAIN_MAX_D3D_DRIVERS];
	int32				CurrentDriver;
	DDDEVICEIDENTIFIER	DeviceIdentifier;

	// Surface formats
	int32				NumSurfFormats;						// Num 2D texture formats avail (from DD4 object)
    DDMain_SurfFormat	SurfFormats[DDMAIN_MAX_SURFACE_FORMATS];

    // Texture formats
	int32				NumTextureFormats;					// Num 3D texture formats avail (from device)
    DDMain_SurfFormat	TextureFormats[DDMAIN_MAX_TEXTURE_FORMATS];

	BOOL				LogToFile;
	BOOL				FoundGoodDevice;
	BOOL				CanDoWindow;
	
	BOOL				RenderingIsOK;

	DWORD				VidMemFree;

	float				Gamma;
	BOOL				GammaChanged;

	geBoolean			CanDoMultiTexture;

	geBoolean			FogEnable;
	float				FogStart;
	float				FogEnd;
	float				FogR;
	float				FogG;
	float				FogB;

	// DD / D3D Flags
	uint32				Flags;
} App_Info;

// DD enum strcuture.  Used when enuming dd
typedef struct
{
	LPDIRECTDRAW4	lpDD;
	char			DriverName[MAX_DRIVER_NAME];
	BOOL			FoundDD;
} DD_Enum;

//================================================================================
//	Globals
//================================================================================
extern App_Info	AppInfo;				// Our global structure that knows all... (once initialized)

//================================================================================
//	Global functions
//================================================================================
BOOL				D3DMain_InitD3D(HWND hWnd, const char *DriverName, int32 Width, int32 Height);
BOOL				D3DMain_ShutdownD3D(void);
geBoolean			D3DMain_Reset(void);
void				D3DMain_Log(LPSTR Str, ... );
BOOL				D3DMain_RestoreAllSurfaces(void);

BOOL				Main_EnumTextureFormats(void);
BOOL				D3DMain_EnumDisplayModes(void);
BOOL				Main_ClearBackBuffer(BOOL Clear, BOOL ClearZ);
BOOL				Main_ShowBackBuffer(void);

BOOL				D3DMain_GetSurfaceFormats(void);

BOOL				Main_CheckDD(void);
BOOL				D3DMain_GetTextureMemory(void);
void				Main_BuildRGBGammaTables(float Gamma);

BOOL				D3DMain_GetClientWindowOffset(HWND hWnd);
geBoolean DRIVERCC	D3DMain_UpdateWindow(void);
geBoolean DRIVERCC	D3DMain_SetActive(geBoolean wParam);
geBoolean DRIVERCC	D3DMain_SetFogEnable(geBoolean Enable, float r, float g, float b, float Start, float End);

#endif