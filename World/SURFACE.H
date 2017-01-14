/****************************************************************************************/
/*  Surface.h                                                                           */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Creates the surfaces for a BSP from the GFX data                       */
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
#ifndef GE_SURFACE_H
#define GE_SURFACE_H

#include <Assert.h>
#include <Windows.h>

#include "BaseType.h"
#include "PtrTypes.h"
#include "Vec3d.h"
#include "DCommon.h"

#ifdef __cplusplus
extern "C" {
#endif

//================================================================================
//	Structure defines
//================================================================================

// Surface info carries extra info about a face thats not in GFX_Face (File format face)
typedef struct Surf_SurfInfo
{
	DRV_LInfo	LInfo;							// For RDriver use only...

	geVec3d		T2WVecs[2];
	geVec3d		TexOrg;

	geVec3d		VMins;
	geVec3d		VMaxs;
	
	int32		VisFrame;
	
	int32		TexInfo;
	
	int32		XStep;							// Lightmap step values (1:21:10 fixed)
	int32		YStep;
	int32		XScale;
	int32		YScale;

	float		ShiftU;
	float		ShiftV;
	
	int32		NumLTypes;						// Number of lightmap types this face has...
	int32		DLightFrame;					// == Globals->CurFrame if dlighted
	uint32		DLights;						// Bit set for each DLight
	uint32		Flags;							// Surface Flags (NOTE - This is not the flags from the utilities)

} Surf_SurfInfo;

typedef struct Surf_TexVert
{
	float		u, v;
	float		r, g, b, a;
} Surf_TexVert;

// Lit vertex
typedef struct
{
	float X, Y, Z;									// 3d vertex
	float u, v;										// Uv's
	float r, g, b, a;								// color
} Surf_LVertex;

// Transformed Lit vertex
typedef struct
{
	float x, y, z;									// screen points
	float u, v;										// Uv's
	float r, g, b, a;								// color
} Surf_TLVertex;

//	Surface Flags
#define		SURFINFO_TRANS				(1<<0)		// Surface is transparent
#define		SURFINFO_LTYPED				(1<<1)		// This surface has more than one ltype
#define		SURFINFO_LIGHTMAP			(1<<2)		// This surface has a lightmap
#define		SURFINFO_WAVY				(1<<3)		// This surface is a wavy surface

//================================================================================
//	Function defines
//================================================================================
geBoolean Surf_EngineInit(geEngine *Engine);
void Surf_EngineShutdown(geEngine *Engine);
geBoolean Surf_SetEngine(geEngine *Engine);
geBoolean Surf_SetWorld(geWorld *World);
geBoolean Surf_SetGBSP(World_BSP *BSP);
geBoolean Surf_WorldInit(geWorld *World);
void Surf_WorldShutdown(geWorld *World);

BOOL Surf_InSurfBoundingBox(Surf_SurfInfo *Surf, geVec3d *Pos, float Box);

#ifdef __cplusplus
}
#endif

#endif
