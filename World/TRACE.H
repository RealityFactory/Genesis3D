/****************************************************************************************/
/*  Trace.h                                                                             */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: BSP collision detection code                                           */
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
#ifndef GE_TRACE_H
#define GE_TRACE_H

#include <Assert.h>
#include <Windows.h>

#include "Genesis.h"
#include "BaseType.h"
#include "Vec3d.h"
#include "World.h"
//#include "System.h"

#ifdef __cplusplus
extern "C" {
#endif

//=====================================================================================
//	Defines / Structure defines
//=====================================================================================
#define	PSIDE_FRONT			1
#define	PSIDE_BACK			2
#define	PSIDE_BOTH			(PSIDE_FRONT|PSIDE_BACK)
#define	PSIDE_FACING		4

int32 Trace_BoxOnPlaneSide(const geVec3d *Mins, const geVec3d *Maxs, GFX_Plane *Plane);
geBoolean Trace_BBoxInVisibleLeaf(geWorld *World, geVec3d *Mins, geVec3d *Maxs);

//=====================================================================================
//	Function ProtoTypes
//=====================================================================================


geBoolean Trace_GEWorldCollision(	geWorld *World, 
									const		geVec3d *Mins, 
									const		geVec3d *Maxs, 
									const		geVec3d *Front, 
									const		geVec3d *Back, 
									uint32		Contents,			// Contents to collide with
									uint32		CollideFlags,		// GE_COLLIDE_ALL, etc...
									uint32		UserFlags,			// Flags to mask against actors
									GE_CollisionCB *CollisionCB,
									void		*Context,
									GE_Collision *Col);

geBoolean Trace_WorldCollisionBNode(geWorld *World, 
									geVec3d *Front, 
									geVec3d *Back, 
									int32 *ModelNum,
									geVec3d *Impact,
									int32 *Node,
									int32 *Plane,
									int32 *Side);

geBoolean Trace_WorldCollisionExact(geWorld *World, 
									const geVec3d *Front, 
									const geVec3d *Back, 
									uint32 Flags, 
									geVec3d *Impact,
									GFX_Plane *Plane,
									geWorld_Model **Model,
									Mesh_RenderQ **Mesh,
									geActor **Actor,
									uint32 UserFlags,
									GE_CollisionCB *CollisionCB,
									void *Context);

// Internal only/ does not chek meshes/ returns index numbers into bsp structures for models
geBoolean Trace_WorldCollisionExact2(geWorld *World, 
									const geVec3d *Front, 
									const geVec3d *Back, 
									geVec3d *Impact,
									int32 *Node,
									int32 *Plane,
									int32 *Side);

geBoolean Trace_MiscCollision(GFX_BNode *BNodes, GFX_Plane *Planes, const geVec3d *Mins, const geVec3d *Maxs, const geVec3d *Front, const geVec3d *Back, geXForm3d *XForm, geVec3d *I, GFX_Plane *P);
geBoolean Trace_MiscCollision2(GFX_BNode *BNodes, GFX_Plane *Planes, const geVec3d *Front, const geVec3d *Back, geVec3d *I, int32 *P);

geBoolean Trace_WorldCollisionBBox(	geWorld	*World,
									const geVec3d *Mins, const geVec3d *Maxs, 
									const geVec3d *Front, const geVec3d *Back,
									uint32 Flags,
									geVec3d *I, GFX_Plane *P,
									geWorld_Model **Model,
									Mesh_RenderQ **Mesh,
									geActor **Actor,
									uint32 UserFlags,
									GE_CollisionCB *CollisionCB,
									void *Context);

geBoolean Trace_TestModelMove(	geWorld		*World, 
								geWorld_Model		*Model, 
								const geXForm3d	*DXForm, 
								const geVec3d		*Mins, const geVec3d *Maxs,
								const geVec3d		*In, geVec3d *Out);

geBoolean Trace_ModelCollision(geWorld		*World, 
								geWorld_Model		*Model, 
								const geXForm3d	*DXForm,
								GE_Collision    *Collision,
								//Mesh_RenderQ	**ImpactedMesh,
								geVec3d			*ImpactPoint);

geBoolean Trace_InverseTreeFromBox(geVec3d *Mins, geVec3d *Maxs, GFX_BNode *BNodes, GFX_Plane *Planes);

geBoolean Trace_GetContents(geWorld *World, const geVec3d *Pos, const geVec3d *Mins, const geVec3d *Maxs, uint32 Flags, uint32 UserFlags, GE_CollisionCB *CollisionCB, void *Context, GE_Contents *Contents);

void Trace_GetMoveBox(const geVec3d *Mins, const geVec3d *Maxs, const geVec3d *Front, const geVec3d *Back, geVec3d *OMins, geVec3d *OMaxs);

void		Trace_SetupIntersect(geWorld *World);
geBoolean	Trace_IntersectWorldBSP(geVec3d *Front, geVec3d *Back, int32 Node);

#ifdef __cplusplus
}
#endif

#endif
