/****************************************************************************************/
/*  DCommon.h                                                                           */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Header file for all driver modules.                                    */
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
#ifndef DCOMMON_H
#define DCOMMON_H

//#include <Windows.h>	// {} CB commented out windows
// If you include Windows it MUST be before dcommon!

// FIXME:  What should we do with these?
#include "XForm3d.h"
#include "Vec3d.h"
#include "PixelFormat.h"
#include "geTypes.h"		// This is a no no

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push)
#pragma pack(8)

#ifndef WINVER
#ifdef STRICT
typedef struct HWND__ * HWND;
typedef struct HBITMAP__ * HBITMAP;
#else // STRICT
typedef void * HWND;
typedef void * HBITMAP;
#endif // STRICT

#ifndef VOID
#define VOID void
typedef char CHAR;
typedef short SHORT;
typedef long LONG;
#endif

#ifndef BASETYPES
#define BASETYPES
typedef unsigned long ULONG;
typedef ULONG *PULONG;
typedef unsigned short USHORT;
typedef USHORT *PUSHORT;
typedef unsigned char UCHAR;
typedef UCHAR *PUCHAR;
typedef char *PSZ;
#endif  /* !BASETYPES */

typedef unsigned long       DWORD;
typedef int                 geBoolean;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef float               FLOAT;

typedef struct tagRECT
{
    LONG    left;
    LONG    top;
    LONG    right;
    LONG    bottom;
} RECT;

#endif // WINVER

#define	DRIVERCC _fastcall

#ifndef __cplusplus
	#define DllImport	__declspec( dllimport )
	#define DllExport	__declspec( dllexport )
#else
	#define DllImport	extern "C" __declspec( dllimport )
	#define DllExport	extern "C" __declspec( dllexport )
#endif

#define DRV_VERSION_MAJOR		100			// Genesis 1.0
#define DRV_VERSION_MINOR		3			// >= 3.0 added fog
#define DRV_VMAJS				"100"
#define DRV_VMINS				"3"

#ifndef US_TYPEDEFS
#define US_TYPEDEFS

	typedef uint8	U8;
	typedef uint16	U16;
	typedef uint32	U32;
	typedef char	C8;
	typedef int8	S8;
	typedef int16	S16;
	typedef int32	S32;
#endif

//===
typedef struct geRDriver_THandle	geRDriver_THandle;

// DriverFormat flags
#define RDRIVER_PF_2D_SHIFT					(0)			// Supports being used as a 2d decal surface
#define RDRIVER_PF_3D_SHIFT					(1)			// Supports being used as a 3d poly surface
#define RDRIVER_PF_LIGHTMAP_SHIFT			(2)			// Surface is a lightmap surface
#define RDRIVER_PF_PALETTE_SHIFT			(3)			// Surface is a palette
#define RDRIVER_PF_ALPHA_SHIFT				(4)			// Surface is an alpha map
#define RDRIVER_PF_OPTIONAL_SHIFT			(16)
#define RDRIVER_PF_HAS_ALPHA_SHIFT			(RDRIVER_PF_OPTIONAL_SHIFT + 0)		// Surface can take an alpha map
#define RDRIVER_PF_CAN_DO_COLORKEY_SHIFT	(RDRIVER_PF_OPTIONAL_SHIFT + 1)		// Surface supports colorkeying
#define RDRIVER_PF_COMBINE_LIGHTMAP_SHIFT	(RDRIVER_PF_OPTIONAL_SHIFT + 2)			// Supports being rendered with a lightmap (3d will be set as well)

#define RDRIVER_PF_2D					(1<<RDRIVER_PF_2D_SHIFT)				
#define RDRIVER_PF_3D					(1<<RDRIVER_PF_3D_SHIFT)				
#define RDRIVER_PF_LIGHTMAP				(1<<RDRIVER_PF_LIGHTMAP_SHIFT)			
#define RDRIVER_PF_COMBINE_LIGHTMAP		(1<<RDRIVER_PF_COMBINE_LIGHTMAP_SHIFT)	
#define RDRIVER_PF_PALETTE				(1<<RDRIVER_PF_PALETTE_SHIFT)			
#define RDRIVER_PF_ALPHA				(1<<RDRIVER_PF_ALPHA_SHIFT)			
#define RDRIVER_PF_CAN_DO_COLORKEY		(1<<RDRIVER_PF_CAN_DO_COLORKEY_SHIFT)
#define RDRIVER_PF_HAS_ALPHA			(1<<RDRIVER_PF_HAS_ALPHA_SHIFT)		
#define RDRIVER_PF_MAJOR_MASK			((1<<RDRIVER_PF_OPTIONAL_SHIFT)-1)

typedef struct
{
	gePixelFormat	PixelFormat;
	uint32			Flags;				
} geRDriver_PixelFormat;

#define RDRIVER_THANDLE_HAS_COLORKEY	(1<<0)		// The thandle is using color keying

typedef struct
{
	int32					Width;
	int32					Height;
	int32					Stride;
	uint32					ColorKey;
	uint32					Flags;
	geRDriver_PixelFormat	PixelFormat;

} geRDriver_THandleInfo;

//===

typedef struct
{
	S32	LMapCount[16][4];				// LMap size / MipLevel
} DRV_Debug;

typedef struct
{
	int32		CacheFull;
	int32		CacheRemoved;
	int32		CacheFlushes;
	int32		TexMisses;
	int32		LMapMisses;
} DRV_CacheInfo;

typedef struct
{
	HWND		hWnd;
	
	U8			*Buffer;

	S32			Width;
	S32			Height;

	S32			PixelPitch;
	S32			BytesPerPixel;

	S32			R_shift;
	S32			G_shift;
	S32			B_shift;

	U32			R_mask;
	U32			G_mask;
	U32			B_mask;

	S32			R_width;
	S32			G_width;
	S32			B_width;
} DRV_Window;

typedef struct 
{
    U8 r, g, b;								// RGB components for RGB lightmaps
} DRV_RGB;

//===========================================================================================
// FIXME:  Get palette stuff, and bitmap out of dcommon
#define	DRV_PALETTE_ENTRIES	256
typedef	DRV_RGB	DRV_Palette[DRV_PALETTE_ENTRIES];

// Bitmap hook into the drivers (engine uses these explicitly as is)
typedef struct
{
	char	Name[32];						// Duh, name of bitmap...
	U32		Flags;							// Flags
	S32		Width;							// Width of bitmap
	S32		Height;							// Height of bitmap
	U8		MipLevels;
	U8		*BitPtr[4];						// Pointer to location of bits (up to 4 miplevels)
	DRV_RGB *Palette;

	// Driver sets these in register functions
	//S32		Id;								// Bitmap handle for hardware...
	geRDriver_THandle	*THandle;
} DRV_Bitmap;
//===========================================================================================

#define LMAP_TYPE_LIGHT			0
#define LMAP_TYPE_FOG			1

// Lightmap hook into the drivers (Engine uses these exlicitly as is...)
// Lightmap info
typedef struct
{
	S16					Width, Height;				// lightmap width/height / 16 +1
    S32					MinU, MinV;					// Min U,V values
    DRV_RGB				*RGBLight[2];				// Pointer to RGB light map data

	S32					Face;						// Face that this map belongs too
	geRDriver_THandle	*THandle;
} DRV_LInfo;

typedef struct
{
	float		ShiftU;						// How much to shift right before draw
	float		ShiftV;

	float		DrawScaleU;						// How much to scale right before draw
	float		DrawScaleV;
} DRV_TexInfo;
    
// Render Flags for ALL render functions
#define DRV_RENDER_ALPHA		(1<<0)	// Render function should expect alpha set in vertices
#define DRV_RENDER_FLUSH		(1<<1)	// Render function should gaurentee that this render happens NOW
#define DRV_RENDER_NO_ZMASK		(1<<2)	// No zbuffering should be performed
#define DRV_RENDER_NO_ZWRITE	(1<<3)	// No z writing will be performed
#define DRV_RENDER_CLAMP_UV		(1<<4)	// Clamp UV in both directions

//
//  PolyMode flags	(A method to override how polys are drawn for debugging purposes...)
//
// Put these in the poly flags!!!  They are currently not used right now...
#define DRV_POLYMODE_NORMAL		1			// Draw as is
#define DRV_POLYMODE_GOURAUD	2			// Gouraud only
#define DRV_POLYMODE_LINES		3			// Outlines only


typedef struct
{
	float	x,y,z;						// float 3d z value
	float	u,v;						// float texture coords
	float	r,g,b,a;					// Color of point, and Alpha
} DRV_TLVertex;

typedef struct
{
	char				AppName[512];
	S32					Driver;
	char				DriverName[512];
	S32					Mode;
	char				ModeName[512];
	S32					Width;
	S32					Height;
	HWND				hWnd;
} DRV_DriverHook;

typedef struct
{
	// Texture info
	geVec3d		VecU;
	geVec3d		VecV;
	int32		TexMinsX;
	int32		TexMinsY;
	int32		TexWidth;
	int32		TexHeight;
	float		TexShiftX;
	float		TexShiftY;

	// Camera info
	geXForm3d	CXForm;
	geVec3d		CPov;

	float		XCenter;
	float		YCenter;

	float		XScale;
	float		YScale;
	float		XScaleInv;			// 1 / XScale
	float		YScaleInv;			// 1 / YScale;
	float		ZScale;				// camera z scale


	geVec3d		PlaneNormal;		// Face normal
	float		PlaneDist;
	geVec3d		RPlaneNormal;		// Rotated Face normal
	geVec3d		Pov;
} GInfo;

// What the driver can support as far as texture mapping is concerned
#define DRV_SUPPORT_ALPHA					(1<<0)		// Driver can do alpha blending
#define DRV_SUPPORT_COLORKEY				(1<<1)		// Driver can do pixel masking
#define DRV_SUPPORT_GAMMA					(1<<2)		// Gamma function works with the driver

// A hint to the engine as far as what to turn on and off...
#define DRV_PREFERENCE_NO_MIRRORS			(1<<0)		// Engine should NOT render mirrors
#define DRV_PREFERENCE_SORT_WORLD_FB		(1<<1)		// Sort world Front to Back
#define DRV_PREFERENCE_SORT_WORLD_BF		(1<<2)		// Sort world Back to Front
#define DRV_PREFERENCE_DRAW_WALPHA_IN_BSP	(1<<3)		// Draw world alphas in BSP sort

typedef struct
{
	U32			CanSupportFlags;
	U32			PreferenceFlags;
	U32			Reserved1;
	U32			Reserved2;
} DRV_EngineSettings;

// Enumeration defines
typedef geBoolean DRV_ENUM_MODES_CB( S32 Mode, char *ModeName, S32 Width, S32 Height, void *Context);
typedef geBoolean DRV_ENUM_DRV_CB( S32 Driver, char *DriverName, void *Context);

typedef geBoolean DRIVERCC DRV_ENUM_DRIVER(DRV_ENUM_DRV_CB *Cb, void *Context); 
typedef geBoolean DRIVERCC DRV_ENUM_MODES(S32 Driver, char *DriverName, DRV_ENUM_MODES_CB *Cb, void *Context); 

typedef geBoolean DRV_ENUM_PFORMAT_CB(geRDriver_PixelFormat *Format, void *Context);
typedef geBoolean DRIVERCC DRV_ENUM_PFORMAT(DRV_ENUM_PFORMAT_CB *Cb, void *Context); 

// Create/Destroy/Etc Driver functions
typedef geBoolean DRIVERCC DRV_INIT(DRV_DriverHook *Hook);
typedef geBoolean DRIVERCC DRV_SHUTDOWN(void);
typedef geBoolean DRIVERCC DRV_RESET(void);
typedef geBoolean DRIVERCC DRV_UPDATE_WINDOW(void);
typedef geBoolean DRIVERCC DRV_SET_ACTIVE(geBoolean active);

// Texture surface functions
typedef geRDriver_THandle *DRIVERCC CREATE_TEXTURE(int32 Width, int32 Height, int32 NumMipLevels, const geRDriver_PixelFormat *PixelFormat);
typedef geBoolean DRIVERCC DESTROY_TEXTURE(geRDriver_THandle *THandle);

typedef geBoolean DRIVERCC LOCK_THANDLE(geRDriver_THandle *THandle, int32 MipLevel, void **Data);
typedef geBoolean DRIVERCC UNLOCK_THANDLE(geRDriver_THandle *THandle, int32 MipLevel);

typedef geBoolean DRIVERCC SET_PALETTE(geRDriver_THandle *THandle, geRDriver_THandle *PalHandle);
typedef geRDriver_THandle *DRIVERCC GET_PALETTE(geRDriver_THandle *THandle);

typedef geBoolean DRIVERCC SET_ALPHA(geRDriver_THandle *THandle, geRDriver_THandle *PalHandle);
typedef geRDriver_THandle *DRIVERCC GET_ALPHA(geRDriver_THandle *THandle);

typedef geBoolean DRIVERCC THANDLE_GET_INFO(geRDriver_THandle *THandle, int32 MipLevel, geRDriver_THandleInfo *Info);

// Scene management functions
typedef geBoolean DRIVERCC BEGIN_SCENE(geBoolean Clear, geBoolean ClearZ, RECT *WorldRect);
typedef geBoolean DRIVERCC END_SCENE(void);
typedef geBoolean DRIVERCC BEGIN_WORLD(void);
typedef geBoolean DRIVERCC END_WORLD(void);
typedef geBoolean DRIVERCC BEGIN_MESHES(void);
typedef geBoolean DRIVERCC END_MESHES(void);
typedef geBoolean DRIVERCC BEGIN_MODELS(void);
typedef geBoolean DRIVERCC END_MODELS(void);

// Render functions
typedef geBoolean DRIVERCC RENDER_G_POLY(DRV_TLVertex *Pnts, S32 NumPoints, U32 Flags);
typedef geBoolean DRIVERCC RENDER_W_POLY(DRV_TLVertex *Pnts, S32 NumPoints, geRDriver_THandle *THandle, DRV_TexInfo *TexInfo, DRV_LInfo *LInfo, U32 Flags);
typedef geBoolean DRIVERCC RENDER_MT_POLY(DRV_TLVertex *Pnts, S32 NumPoints, geRDriver_THandle *THandle, U32 Flags);

typedef geBoolean DRIVERCC DRAW_DECAL(geRDriver_THandle *THandle, RECT *SRect, int32 x, int32 y);

typedef geBoolean DRIVERCC SCREEN_SHOT(const char *Name);

typedef geBoolean DRIVERCC SET_GAMMA(float Gamma);
typedef geBoolean DRIVERCC GET_GAMMA(float *Gamma);
typedef geBoolean DRIVERCC DRV_SET_FOG_ENABLE(geBoolean Enable, float r, float g, float b, float Start, float End);

typedef void SETUP_LIGHTMAP_CB(DRV_LInfo *LInfo, geBoolean *Dynamic);

typedef struct
{
	char				*Name;
	S32					VersionMajor;
	S32					VersionMinor;

	// Error handling hooks set by driver
	S32					LastError;							// Last error driver made
	char				*LastErrorStr;						// NULL terminated error string
	
	// Enum Modes/Drivers
	DRV_ENUM_DRIVER		*EnumSubDrivers;
	DRV_ENUM_MODES		*EnumModes;
	
	DRV_ENUM_PFORMAT	*EnumPixelFormats;

	// Init/DeInit functions
	DRV_INIT					*Init;
	DRV_SHUTDOWN				*Shutdown;
	DRV_RESET					*Reset;
	DRV_UPDATE_WINDOW			*UpdateWindow;
	DRV_SET_ACTIVE				*SetActive;
	
	// Create/Destroy texture functions
	CREATE_TEXTURE		*THandle_Create;
	DESTROY_TEXTURE		*THandle_Destroy;

	// Texture manipulation functions
	LOCK_THANDLE		*THandle_Lock;
	UNLOCK_THANDLE		*THandle_UnLock;

	// Palette access functions
	SET_PALETTE			*THandle_SetPalette;
	GET_PALETTE			*THandle_GetPalette;

	// Palette access functions
	SET_ALPHA			*THandle_SetAlpha;
	GET_ALPHA  			*THandle_GetAlpha;

	THANDLE_GET_INFO	*THandle_GetInfo;

	// Scene management functions
	BEGIN_SCENE			*BeginScene;
	END_SCENE			*EndScene;
	BEGIN_WORLD			*BeginWorld;
	END_WORLD			*EndWorld;
	BEGIN_MESHES		*BeginMeshes;
	END_MESHES			*EndMeshes;
	BEGIN_MODELS		*BeginModels;
	END_MODELS			*EndModels;
	
	// Render functions
	RENDER_G_POLY		*RenderGouraudPoly;
	RENDER_W_POLY		*RenderWorldPoly;
	RENDER_MT_POLY		*RenderMiscTexturePoly;

	//Decal functions
	DRAW_DECAL			*DrawDecal;

	S32					NumWorldPixels;
	S32					NumWorldSpans;
	S32					NumRenderedPolys;
	DRV_CacheInfo		*CacheInfo;

	SCREEN_SHOT			*ScreenShot;

	SET_GAMMA			*SetGamma;
	GET_GAMMA			*GetGamma;

	DRV_SET_FOG_ENABLE	*SetFogEnable;
	
	// Driver preferences
	DRV_EngineSettings	*EngineSettings;

	// The engine supplies these for the drivers misc use
	SETUP_LIGHTMAP_CB	*SetupLightmap;

	// Temp hack global
	GInfo				*GlobalInfo;
} DRV_Driver;

typedef geBoolean DRV_Hook(DRV_Driver **Hook);

//
//	Error defines set by the driver.  These will be in the LastError member of DRV_Driver
//	structure.  LastErrorStr will contain a NULL terminated detail error string set by the driver
//
#define DRV_ERROR_NONE					0	// No error has occured
#define DRV_ERROR_INVALID_PARMS			1	// invalid parameters passed
#define DRV_ERROR_NULL_WINDOW			2	// Null window supplied
#define DRV_ERROR_INIT_ERROR			3	// Error intitializing
#define DRV_ERROR_INVALID_REGISTER_MODE	4	// Invalid register mode
#define DRV_ERROR_NO_MEMORY				5	// Not enough ram
#define DRV_ERROR_MAX_TEXTURES			6	// Max texture capacity has been exceeded...
#define DRV_ERROR_GENERIC				7	// Generic error	 
#define DRV_ERROR_UNDEFINED				8	// An undefined error has occured
#define DRV_ERROR_INVALID_WINDOW_MODE	9	// Requested window/full not supported

typedef enum
{
	RENDER_NONE,
	RENDER_WORLD,
	RENDER_MESHES,
	RENDER_MODELS
} DRV_RENDER_MODE;

#ifdef __cplusplus
}
#endif

#pragma pack(pop)

#endif
