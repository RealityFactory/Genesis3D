/****************************************************************************************/
/*  PHYSICSJOINT.H                                                                      */
/*                                                                                      */
/*  Author: Jason Wood                                                                  */
/*  Description: Rigid body joint interface                                             */
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
#ifndef	PHYSICSJOINT_H
#define PHYSICSJOINT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	JT_WORLD = 0,
	JT_SPHERICAL,
	JT_PTTOPATH,
	JT_PTTOSURFACE
}	gePhysicsJoint_Kind;

typedef struct gePhysicsJoint gePhysicsJoint;

////////////////////////////////////////////////////////////////////////////////////////////////////
// ctor / dtor

//gePhysicsJoint* gePhysicsJoint_Create(ItemJoint* pItemJoint);
GENESISAPI gePhysicsJoint * GENESISCC gePhysicsJoint_Create(gePhysicsJoint_Kind Kind, const geVec3d *Location, 
	float assemblyRate, gePhysicsObject *PS1, gePhysicsObject *PS2, float physicsScale);
GENESISAPI geBoolean GENESISCC gePhysicsJoint_Destroy(gePhysicsJoint** ppPhysjnt);

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// functions

GENESISAPI gePhysicsJoint_Kind GENESISCC gePhysicsJoint_GetType(const gePhysicsJoint* pPhysjnt);
GENESISAPI void GENESISCC gePhysicsJoint_GetLocationA(const gePhysicsJoint* pPhysjnt, geVec3d* pLoc);
GENESISAPI void GENESISCC gePhysicsJoint_GetLocationB(const gePhysicsJoint* pPhysjnt, geVec3d* pLoc);
GENESISAPI void GENESISCC gePhysicsJoint_SetLocationA(gePhysicsJoint* pPhysjnt, const geVec3d* pLoc);
GENESISAPI void GENESISCC gePhysicsJoint_SetLocationB(gePhysicsJoint* pPhysjnt, const geVec3d* pLoc);
GENESISAPI void GENESISCC gePhysicsJoint_GetLocationAInWorldSpace(const gePhysicsJoint* pPhysjnt, geVec3d* pLoc);
GENESISAPI void GENESISCC gePhysicsJoint_GetLocationBInWorldSpace(const gePhysicsJoint* pPhysjnt, geVec3d* pLoc);
GENESISAPI void GENESISCC gePhysicsJoint_SetLocationAInWorldSpace(gePhysicsJoint* pPhysjnt, const geVec3d* pLoc);
GENESISAPI void GENESISCC gePhysicsJoint_SetLocationBInWorldSpace(gePhysicsJoint* pPhysjnt, const geVec3d* pLoc);
GENESISAPI gePhysicsObject* GENESISCC gePhysicsJoint_GetObject1(const gePhysicsJoint* pPhysjnt);
GENESISAPI gePhysicsObject* GENESISCC gePhysicsJoint_GetObject2(const gePhysicsJoint* pPhysjnt);
GENESISAPI float GENESISCC gePhysicsJoint_GetAssemblyRate(const gePhysicsJoint* pPhysjnt);
GENESISAPI void GENESISCC gePhysicsJoint_SetAssemblyRate(gePhysicsJoint* pPhysjnt, float assemblyRate);

#ifdef __cplusplus
}
#endif

#endif
