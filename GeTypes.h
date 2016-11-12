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

//MRB BEGIN
//geSprite
//
// Sprite flags (geSprite_AddSprite)
//
#define GE_SPRITE_RENDER_NORMAL			(1<<0)		// Render in normal views
#define GE_SPRITE_RENDER_MIRRORS		(1<<1)		// Render in mirror views
#define GE_SPRITE_RENDER_ALWAYS			(1<<2)		// Render always, skipping all visibility tests
#define GE_SPRITE_COLLIDE						(1<<3)		// Collide when calling geWorld_Collision
//MRB END

typedef struct
{
	geBoolean		UseEnvironmentMapping;	//toggle for actor-level environ-map
	geBoolean		Supercede;		//toggle for material-level
	geFloat			PercentEnvironment;
	geFloat			PercentMaterial;		//Used when Supercede == GE_FALSE
	geFloat			PercentPuppet;
} geEnvironmentOptions;

//
//	Model flags (geWorld_ModelSetFlags)
//
#define GE_MODEL_RENDER_NORMAL			(1<<0)		// Render in normal views
#define GE_MODEL_RENDER_MIRRORS			(1<<1)		// Render in mirror views
#define GE_MODEL_RENDER_ALWAYS			(1<<2)		// Render always, skipping all visibility tests
#define GE_MODEL_COLLIDE				(1<<3)		// Collide when calling geWorld_Collision

//MRB BEGIN
typedef struct
{
	geFloat				r, g, b;
} geColor;

typedef struct
{
	geFloat				u, v;
} geUV;
//MRB END

typedef struct
{
	geFloat r, g, b, a;
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

//MRB BEGIN
typedef struct
{
	geFloat X;
	geFloat Y;
} geCoordinate;
//MRB END

// Lit vertex
typedef struct
{
	// FIXME:  Convert 3d X,Y,Z to geVec3d
	geFloat X, Y, Z;									// 3d vertex
	geFloat u, v;										// Uv's
	// FIXME:  Convert r,g,b,a to GE_RGBA
	geFloat r, g, b, a;								// color
} GE_LVertex;

// Transformed Lit vertex
typedef struct
{
	geFloat x, y, z;									// screen points
	geFloat u, v;										// Uv's
	geFloat r, g, b, a;								// color
} GE_TLVertex;

typedef GE_Rect geRect;

#ifdef __cplusplus
}
#endif


#endif GETYPES_H
