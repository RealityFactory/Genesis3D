/****************************************************************************************/
/*  Light.h                                                                             */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Handles lightmaps, dynamic light, fog, etc                             */
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
#ifndef GE_LIGHT_H
#define GE_LIGHT_H

#include <assert.h>

#include "Genesis.h"
#include "BaseType.h"
#include "System.h"
#include "DCommon.h"

#ifdef __cplusplus
extern "C" {
#endif

//=====================================================================================
//	Defines / Structure defines
//=====================================================================================
#define MAX_DYNAMIC_LIGHTS		64	// Maximum number of moving lights in map
#define MAX_LTYPES				24	// Max number of ltypes
//#define	MAX_LMAP_SIZE			128
//#define	MAX_LMAP_SIZE			18
#define	MAX_LMAP_SIZE			36

typedef struct
{
	geBoolean	Active;					// Is this light in use?
	GE_RGBA		Color;					// Color of light (0...255.0f)
	geVec3d		Pos;					// Position of this light
	geFloat		Radius;					// Intensity of this light (Radius)

	// Fixed point color
	uint32		FColorR;
	uint32		FColorG;
	uint32		FColorB;

	geBoolean	CastShadow;
// changed QuestOfDreams DSpotLight
	geBoolean	Spot;   	// is it a spotlight?
	geFloat		Angle;		// arc
	geVec3d		Normal; 	// angles
	int 		Style;  	// fall off style 0,1,2
// end change QuestOfDreams
} Light_DLight;

typedef struct Light_LightInfo
{
	// Intensity tables, for animated styles
	BOOL			LTypeDynamic[MAX_LTYPES];
	int32			LTypeIntensities[MAX_LTYPES];
	uint8			LTypeIntensities2[MAX_LTYPES];

	char			LTypeTable[MAX_LTYPES][70];
	int32			IPos[MAX_LTYPES];                 // Ref position in ltype table

	Light_DLight	DynamicLights[MAX_DYNAMIC_LIGHTS];
	int32			NumDynamicLights;
} Light_LightInfo;

typedef struct tag_light
{
	int light;
	GE_RGBA color;
	int style;
	geVec3d origin;
} light;

// changed QuestOfDreams 05/02/2004
typedef struct	tag_spotlight
{
	int		DummyRadius;
	int		light;
	GE_RGBA	color;
	int		style;
	geVec3d	origin;
	geVec3d	angles;
	int		arc;
} spotlight;
// end change

//=====================================================================================
//	Function ProtoTypes
//=====================================================================================
geBoolean	Light_EngineInit(geEngine *Engine);
void		Light_EngineShutdown(geEngine *Engine);
geBoolean	Light_WorldInit(geWorld *World);
void		Light_WorldShutdown(geWorld *World);
geBoolean	Light_SetEngine(geEngine *Engine);
geBoolean	Light_SetWorld(geWorld *World);
geBoolean	Light_SetGBSP(World_BSP *BSP);

Light_DLight *Light_WorldAddLight(geWorld *World);
void		Light_WorldRemoveLight(geWorld *World, Light_DLight *DLight);
geBoolean	Light_SetupLights(geWorld *World);
geBoolean	Light_SetAttributes(	Light_DLight *Light,
								const geVec3d *Pos,
								const GE_RGBA *RGBA,
								geFloat Radius,
								geBoolean CastShadow);
geBoolean	Light_WorldSetLTypeTable(geWorld *World, int32 LType, const char *Table);

char		Light_WorldGetLTypeCurrent(geWorld *World, int32 LType);
void		Light_SetupLightmap(DRV_LInfo *LInfo, BOOL *Dynamic);
geBoolean	Light_GetLightmapRGB(Surf_SurfInfo *Surf, geVec3d *Pos, GE_RGBA *RGBA);
geBoolean	Light_GetLightmapRGBBlended(Surf_SurfInfo *Surf, geVec3d *Pos, GE_RGBA *RGBA);
void		Light_FogVerts(const geFog *Fog, const geVec3d *POV, const geVec3d *Verts, Surf_TexVert *TexVerts, int32 NumVerts);

// added QuestOfDreams DSpotLight
geBoolean Light_SetSpotAttributes(	Light_DLight *Light,
								const geVec3d *Pos,
								const GE_RGBA *RGBA,
								geFloat Radius,
								geFloat Arc,
								const geVec3d *Angles,
								int Style,
								geBoolean CastShadow);
// end change QuestOfDreams

#ifdef __cplusplus
}
#endif

#endif
