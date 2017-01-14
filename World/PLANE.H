/****************************************************************************************/
/*  Plane.h                                                                             */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Handy functions that deal with GFX_Plane's                             */
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
#ifndef GE_PLANE_H
#define GE_PLANE_H

#include <Assert.h>
#include <Windows.h>

#include "GBSPFile.h"

#ifdef __cplusplus
extern "C" {
#endif

//=====================================================================================
//	Defines / Structure defines
//=====================================================================================

//=====================================================================================
//	Function ProtoTypes
//=====================================================================================
geBoolean GENESISCC Plane_SetEngine(geEngine *Engine);
geBoolean GENESISCC Plane_SetWorld(geWorld *World);
geBoolean GENESISCC Plane_SetGBSP(World_BSP *BSP);

int32 GENESISCC Plane_FindLeaf(const geWorld *World, int32 Node, const geVec3d *POV);

float GENESISCC Plane_PlaneDistanceFast(const GFX_Plane *Plane, const geVec3d *Point);
float GENESISCC Plane_FaceDistanceFast(const GFX_Face *Face, const geVec3d *Point);
float GENESISCC Plane_PlaneDistance(const GFX_Plane *Plane, const geVec3d *Point);
void gePlane_SetFromVerts(GFX_Plane *Plane, const geVec3d *V1, const geVec3d *V2, const geVec3d *V3);


#ifdef __cplusplus
}
#endif

#endif
