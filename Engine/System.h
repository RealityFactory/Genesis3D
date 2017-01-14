/****************************************************************************************/
/*  System.h                                                                            */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Friend of engine.c.  Takes care of some of the driver work.            */
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
#ifndef GE_SYSTEM_H
#define GE_SYSTEM_H

//#define OLD_FONT

#include "ErrorLog.h"
#include "Genesis.h"
#include <windows.h>
#include "dcommon.h"
#include "Camera.h"
#include "PtrTypes.h"

#define		VectorToSUB(a, b) ( *(((float*)&a) + b) )

#define		MAX_SUB_DRIVERS				12
#define		MAX_DRIVER_MODES			32
#define		DRV_STR_SIZE				512
#define		DRV_MODE_STR_SIZE			512

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	FrameState_None = 0,
	FrameState_Begin,
} geEngine_FrameState;

//=====================================================================================
//	Structure defines
//=====================================================================================

#define MAX_CLIENT_STRING_LEN	80
#define MAX_CLIENT_STRINGS		20

typedef struct 
{
	int32			x,y;
	char			String[MAX_CLIENT_STRING_LEN];
}	Sys_String;

typedef struct
{
	geBitmap		*FontBitmap;

	uint32			FontLUT1[256];
	Sys_String		ClientStrings[MAX_CLIENT_STRINGS];
	int32			NumStrings;
} Sys_FontInfo;

typedef struct
{
	int32		Freq;
} Sys_CPUInfo;

typedef struct geDriver_Mode
{
	int32			Id;							// Driver assigned mode id
	char			Name[DRV_MODE_STR_SIZE];		// Driver assigned mode name
	int32			Width;						// Mode width
	int32			Height;						// Mode height
} geDriver_Mode;

typedef struct geDriver
{
	int32			Id;							// Driver assigned Id
	char			Name[DRV_STR_SIZE];			// Driver assigned name
	char			FileName[256];				// FileName of driver

	geDriver_Mode	Modes[MAX_DRIVER_MODES];	// Modes for this driver
	int32			NumModes;					// Num modes for this driver

} geDriver;

typedef struct
{
	// Info the enuming fills in
	geDriver		SubDrivers[MAX_SUB_DRIVERS];
	int32			NumSubDrivers;
	char			*CurFileName;

	//	Data for current driver
	geBoolean		Active;				// GE_TRUE if a driver and mode has been initialized
	
	HINSTANCE		DriverHandle;		// CurrentDriver Handle (for DLL)
	
	geDriver		*CurDriver;			// Current driver
	geDriver_Mode	*CurMode;			// Current mode
	DRV_Driver		*RDriver;			// Current driver function hook
} Sys_DriverInfo;

typedef struct
{
	int32			TraversedPolys;		// Total Polys traversed
	int32			SentPolys;			// Total Polys sent to driver
	int32			RenderedPolys;		// Total Rendered polys reported by driver

	int32			NumModels;
	int32			NumMirrors;
#ifdef	MESHES
	int32			NumMeshes;
#endif
	int32			NumActors;
	int32			NumDLights;
	int32			NumFog;
	int32			LMap1;				// Lmaps gone through first pass (reg light)
	int32			LMap2;				// LMaps gone through 2nd pass (fog)
} Sys_DebugInfo;

//{} Hack:

#define ENGINE_PF_WORLD			(0)
#define ENGINE_PF_LIGHTMAP		(1)
#define ENGINE_PF_USER			(2)
#define ENGINE_PF_USER_ALPHA	(3)
#define ENGINE_PF_DECAL			(4)
#define ENGINE_PF_PALETTE		(5)
#define ENGINE_PF_ALPHA_CHANNEL	(6)
#define ENGINE_PF_COUNT			(7)

typedef struct BitmapList BitmapList;

#define ENGINE_MAX_WORLDS		8

typedef struct
{
	geWorld		*World;
	int32		RefCount;

} geEngine_WorldList;

// System globals initialized by module it belongs to...
typedef struct geEngine
{
	//	System info
	Sys_DriverInfo		DriverInfo;			// Info about current driver (this should be enumed)
	Sys_CPUInfo			CPUInfo;			// Info about the Cpu
	Sys_DebugInfo		DebugInfo;

	LARGE_INTEGER		CurrentTic;

	Sys_FontInfo		FontInfo;

	User_Info			*UserInfo;			// Client loaded resources, etc...

	HWND				hWnd;
	char				AppName[200];

	//	Global LUT's
	int16				WaveTable[20];		// Global Wave table (for wavy effects, pumping, etc)
	int16				WaveDir[20];

	// Global info that modules need to render world
	//geWorld				*World;				// The global World
	geEngine_WorldList	WorldList[ENGINE_MAX_WORLDS];		// Current list of worlds renderable by the engine

	int32				NumWorlds;						// Number of active worlds in world list
	geWorld				*Worlds[ENGINE_MAX_WORLDS];		// Linear array of worlds contained in WorldList
	
	// Light module info
	uint8				StyleLUT1[64][256];	// Style intensity table (StyleLUT1[Intensity][Number]);

	geBoolean			Changed;			// == GE_TRUE if needs to be updated with Driver

	geBoolean			DisplayFrameRateCounter; // Whether or not to display the FPS string

	char				*DriverDirectory;	// Path to load driver DLLs from

	BitmapList			*AttachedBitmaps;

	geBoolean			HasPixelFormat[ENGINE_PF_COUNT];
	geRDriver_PixelFormat PixelFormats[ENGINE_PF_COUNT];

	float				CurrentGamma;
	float				BitmapGamma;

	geBoolean			FogEnable;
	float				FogR;
	float				FogG;
	float				FogB;
	float				FogStart;
	float				FogEnd;

	geEngine_FrameState	FrameState;

} geEngine;

//=====================================================================================
//	Function prototypes
//=====================================================================================

//Engine
geEngine *Sys_EngineCreate(HWND hWnd, const char *AppName, const char *DriverDirectory, uint32 Version);

geBoolean	Sys_ShutdownDriver(geEngine *Engine);

void		Sys_EngineFree(geEngine *Engine);

#ifdef	MESHES
Mesh_RenderQ *Sys_WorldAddMesh(geWorld *World, Mesh_MeshDef *MeshDef, int32 Flags);
Mesh_MeshDef *Sys_WorldCreateMesh(geWorld *World, const char *BitmapPath, const char *FileName);
void Sys_WorldFreeMesh(geWorld *World, Mesh_MeshDef *MeshDef);
#endif

// Misc system
geBoolean Sys_GetCPUFreq(Sys_CPUInfo *Info);
geBoolean Sys_EnginePrint(geEngine *Engine, int32 x, int32 y, char *String);
geBoolean	Sys_EngineResetDecorators(geEngine *Engine);

#ifdef __cplusplus
}
#endif
#endif
