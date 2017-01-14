/****************************************************************************************/
/*  User.h                                                                              */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: User poly's                                                            */
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
#ifndef GE_USER_H
#define GE_USER_H

#include <Assert.h>
#include <Windows.h>

#include "BaseType.h"
#include "Vec3d.h"
#include "XForm3d.h"
#include "Camera.h"
#include "Genesis.h"
#include "World.h"
#include "Surface.h"

#include "DCommon.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_USER_VERTS				4			
#define USER_MAX_SORTED_POLYS		1024

//================================================================================
//	Structure defines
//================================================================================

typedef struct	gePoly
{
#ifdef _DEBUG
	struct gePoly	*Self1;
#endif

	geWorld			*World;
	GE_LVertex		Verts[MAX_USER_VERTS];	// Copy of user verts					

	geWorld_Leaf	*LeafData;
	float			ZOrder;

	int32			NumVerts;

	geBitmap		*Bitmap;

	gePoly_Type		Type;					// Type of poly
	int32			RenderFlags;			// Fx of poly

	float			Scale;					// Currently only used for TexturedPoint

	struct gePoly	*Prev;
	struct gePoly	*Next;

	struct gePoly	*AddOnceNext;

#ifdef _DEBUG
	struct gePoly	*Self2;
#endif
} gePoly;

typedef struct User_Info
{
	gePoly		*AddPolyOnceList;
} User_Info;

//================================================================================
//	Function ProtoTypes
//================================================================================
geBoolean	User_EngineInit(geEngine *Engine);
void		User_EngineShutdown(geEngine *Engine);
geBoolean	User_WorldInit(geWorld *World);
void		User_WorldShutdown(geWorld *World);

geBoolean User_RenderPolyList(gePoly *PolyList);

GENESISAPI	gePoly *geWorld_AddPolyOnce(	geWorld *World, 
										GE_LVertex *Verts, 
										int32 NumVerts, 
										geBitmap *Bitmap,
										gePoly_Type Type, 
										uint32 RenderFlags,
										float Scale);
GENESISAPI	gePoly *geWorld_AddPoly(	geWorld *World, 
									GE_LVertex *Verts, 
									int32 NumVerts, 
									geBitmap *Bitmap,
									gePoly_Type Type,
									uint32 RenderFlags,
									float Scale);

GENESISAPI	void geWorld_RemovePoly(geWorld *World, gePoly *Poly);
GENESISAPI	geBoolean gePoly_GetLVertex(gePoly *Poly, int32 Index, GE_LVertex *LVert);
GENESISAPI	geBoolean gePoly_SetLVertex(gePoly *Poly, int32 Index, const GE_LVertex *LVert);

geBoolean	User_SetCameraInfo(geEngine *Engine, geWorld *World, geCamera *Camera, Frustum_Info *Fi);
geBoolean	User_DestroyOncePolys(geWorld *World);
void		User_DestroyPolyList(geWorld *World, gePoly *List);

void		User_EngineFillRect(geEngine *Engine, const GE_Rect *Rect, const GE_RGBA *Color);

#ifdef __cplusplus
}
#endif

#endif
