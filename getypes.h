/****************************************************************************************/
/*  GeTypes.h                                                                           */
/*                                                                                      */
/*  Description: Genesis Types (not primitive enough for basetype)                      */
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
#ifndef GE_TYPES_H
#define GE_TYPES_H

#include "BaseType.h"


#ifdef __cplusplus
extern "C" {
#endif

//
//	Collision defines (for geWorld_Collision)
//
#define GE_COLLIDE_MESHES			(1<<0)
#define GE_COLLIDE_MODELS			(1<<1)
#define GE_COLLIDE_ACTORS			(1<<2)
#define GE_COLLIDE_NO_SUB_MODELS	(1<<3)
#define GE_COLLIDE_ALL				(GE_COLLIDE_MESHES | GE_COLLIDE_MODELS | GE_COLLIDE_ACTORS)

//
// Actor flags (geWorld_AddActor)
//
#define GE_ACTOR_RENDER_NORMAL			(1<<0)		// Render in normal views
#define GE_ACTOR_RENDER_MIRRORS			(1<<1)		// Render in mirror views
#define GE_ACTOR_RENDER_ALWAYS			(1<<2)		// Render always, skipping all visibility tests
#define GE_ACTOR_COLLIDE				(1<<3)		// Collide when calling geWorld_Collision

//
//	Model flags (geWorld_ModelSetFlags)
//
#define GE_MODEL_RENDER_NORMAL			(1<<0)		// Render in normal views
#define GE_MODEL_RENDER_MIRRORS			(1<<1)		// Render in mirror views
#define GE_MODEL_RENDER_ALWAYS			(1<<2)		// Render always, skipping all visibility tests
#define GE_MODEL_COLLIDE				(1<<3)		// Collide when calling geWorld_Collision

typedef struct
{
	float r, g, b, a;
} GE_RGBA;

typedef struct
{
	int32	Left;
	int32	Right;
	int32	Top;
	int32	Bottom;
} GE_Rect;

typedef struct
{
	geFloat MinX,MaxX;
	geFloat MinY,MaxY;
} geFloatRect;

// Lit vertex
typedef struct
{
	// FIXME:  Convert 3d X,Y,Z to geVec3d
	float X, Y, Z;									// 3d vertex
	float u, v;										// Uv's
	// FIXME:  Convert r,g,b,a to GE_RGBA
	float r, g, b, a;								// color
} GE_LVertex;

// Transformed Lit vertex
typedef struct
{
	float x, y, z;									// screen points
	float u, v;										// Uv's
	float r, g, b, a;								// color
} GE_TLVertex;

typedef GE_Rect geRect;

#ifdef __cplusplus
}
#endif


#endif GETYPES_H
