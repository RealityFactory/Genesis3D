/****************************************************************************************/
/*  PHYSICSSYSTEM.H                                                                     */
/*                                                                                      */
/*  Author: Jason Wood                                                                  */
/*  Description: Rigid body, constraint based physics system interface                  */
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
#if !defined (PHYSICSSYSTEM_H)
#define PHYSICSSYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct gePhysicsSystem gePhysicsSystem;

////////////////////////////////////////////////////////////////////////////////////////////////////
// ctor / dtor

GENESISAPI gePhysicsSystem *GENESISCC gePhysicsSystem_Create(void);
GENESISAPI geBoolean GENESISCC gePhysicsSystem_Destroy(gePhysicsSystem** ppSys);

GENESISAPI geBoolean GENESISCC gePhysicsSystem_Iterate(gePhysicsSystem* psPtr, float Time);

GENESISAPI geBoolean GENESISCC gePhysicsSystem_AddJoint(gePhysicsSystem *psPtr, gePhysicsJoint *Joint);
GENESISAPI geBoolean GENESISCC gePhysicsSystem_AddObject(gePhysicsSystem *psPtr, gePhysicsObject *Object);

GENESISAPI int GENESISCC gePhysicsSystem_GetSourceConfigIndex(const gePhysicsSystem* pSys);
GENESISAPI gePhysicsObject** GENESISCC gePhysicsSystem_GetPhysobs(const gePhysicsSystem* pSys);
GENESISAPI gePhysicsJoint** GENESISCC gePhysicsSystem_GetPhysjnts(const gePhysicsSystem* pSys);
GENESISAPI int GENESISCC gePhysicsSystem_GetNumPhysobs(const gePhysicsSystem* pSys);
GENESISAPI int GENESISCC gePhysicsSystem_GetNumPhysjnts(const gePhysicsSystem* pSys);
GENESISAPI int GENESISCC gePhysicsSystem_GetSumOfConstraintDimensions(const gePhysicsSystem* pSys);

#ifdef __cplusplus
}
#endif

#endif

