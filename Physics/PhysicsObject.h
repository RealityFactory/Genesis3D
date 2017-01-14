/****************************************************************************************/
/*  PHYSICSOBJECT.H                                                                     */
/*                                                                                      */
/*  Author: Jason Wood                                                                  */
/*  Description: Constrained rigid body interface                                       */
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
#ifndef	PHYSICSOBJECT_H
#define PHYSICSOBJECT_H

#include "matrix33.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PHYSICSOBJECT_GRAVITY				(-3.9f)

typedef struct gePhysicsObject gePhysicsObject;

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ctor/dtor

GENESISAPI gePhysicsObject * GENESISCC gePhysicsObject_Create(
	const geVec3d *StartLocation,
	float mass,
	geBoolean IsAffectedByGravity,
	geBoolean RespondsToForces,
	float linearDamping,
	float angularDamping,
	const geVec3d *	Mins,
	const geVec3d *	Maxs,
	float physicsScale);
GENESISAPI geBoolean GENESISCC gePhysicsObject_Destroy(gePhysicsObject** pPhysob);

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// functions

GENESISAPI geBoolean GENESISCC gePhysicsObject_ApplyGlobalFrameForce(gePhysicsObject* pod, geVec3d* force, geVec3d* radiusVector, geBoolean isAppliedForce,
	int configIndex);
GENESISAPI geBoolean GENESISCC gePhysicsObject_ApplyGlobalFrameImpulse(gePhysicsObject* pPhysob, geVec3d* pImpulse, geVec3d* pRadVec, int configIndex);
GENESISAPI geBoolean GENESISCC gePhysicsObject_ComputeForces(gePhysicsObject* pod, int configIndex);
GENESISAPI geBoolean GENESISCC gePhysicsObject_Integrate(gePhysicsObject* pod, float deltaTime, int SourceConfigIndex);

GENESISAPI float GENESISCC gePhysicsObject_GetMass(const gePhysicsObject* po);
GENESISAPI void GENESISCC gePhysicsObject_SetMass(gePhysicsObject* po, float mass);

GENESISAPI float GENESISCC gePhysicsObject_GetOneOverMass(const gePhysicsObject* po);

GENESISAPI void GENESISCC gePhysicsObject_GetXForm(const gePhysicsObject* po, geXForm3d* xform, int configIndex);
GENESISAPI void GENESISCC gePhysicsObject_SetXForm(gePhysicsObject* po, const geXForm3d* xform, int configIndex);

GENESISAPI void GENESISCC gePhysicsObject_GetXFormInEditorSpace(const gePhysicsObject* po, geXForm3d* xform, int configIndex);

GENESISAPI void GENESISCC gePhysicsObject_GetOriginalLocation(const gePhysicsObject* po, geVec3d* loc);
GENESISAPI void GENESISCC gePhysicsObject_SetOriginalLocation(gePhysicsObject* po, const geVec3d* loc);

GENESISAPI void GENESISCC gePhysicsObject_GetLocation(const gePhysicsObject *po, geVec3d *Location, int configIndex);
GENESISAPI void GENESISCC gePhysicsObject_GetLocationInEditorSpace(const gePhysicsObject* po, geVec3d* loc, int configIndex);

GENESISAPI void GENESISCC gePhysicsObject_GetLinearVelocity(const gePhysicsObject* po, geVec3d* vel, int configIndex);
GENESISAPI void GENESISCC gePhysicsObject_SetLinearVelocity(gePhysicsObject* po, const geVec3d* vel, int configIndex);

GENESISAPI void GENESISCC gePhysicsObject_GetAngularVelocity(const gePhysicsObject* po, geVec3d* vel, int configIndex);
GENESISAPI void GENESISCC gePhysicsObject_SetAngularVelocity(gePhysicsObject* po, const geVec3d* vel, int configIndex);

GENESISAPI void GENESISCC gePhysicsObject_GetForce(const gePhysicsObject* po, geVec3d* force, int configIndex);
GENESISAPI void GENESISCC gePhysicsObject_SetForce(gePhysicsObject* po, const geVec3d* force, int configIndex);

GENESISAPI void GENESISCC gePhysicsObject_GetTorque(const gePhysicsObject* po, geVec3d* torque, int configIndex);
GENESISAPI void GENESISCC gePhysicsObject_SetTorque(gePhysicsObject* po, const geVec3d* torque, int configIndex);

GENESISAPI void GENESISCC gePhysicsObject_GetAppliedForce(const gePhysicsObject* po, geVec3d* force, int configIndex);
GENESISAPI void GENESISCC gePhysicsObject_SetAppliedForce(gePhysicsObject* po, const geVec3d* force, int configIndex);

GENESISAPI void GENESISCC gePhysicsObject_GetAppliedTorque(const gePhysicsObject* po, geVec3d* torque, int configIndex);
GENESISAPI void GENESISCC gePhysicsObject_SetAppliedTorque(gePhysicsObject* po, const geVec3d* torque, int configIndex);

GENESISAPI void GENESISCC gePhysicsObject_ClearForce(gePhysicsObject* po, int configIndex);
GENESISAPI void GENESISCC gePhysicsObject_ClearTorque(gePhysicsObject* po, int configIndex);

GENESISAPI void GENESISCC gePhysicsObject_ClearAppliedForce(gePhysicsObject* po, int configIndex);
GENESISAPI void GENESISCC gePhysicsObject_ClearAppliedTorque(gePhysicsObject* po, int configIndex);

GENESISAPI void GENESISCC gePhysicsObject_IncForce(gePhysicsObject* po, const geVec3d* forceInc, int configIndex);
GENESISAPI void GENESISCC gePhysicsObject_IncTorque(gePhysicsObject* po, const geVec3d* torqueInc, int configIndex);

GENESISAPI void GENESISCC gePhysicsObject_IncAppliedForce(gePhysicsObject* po, const geVec3d* forceInc, int configIndex);
GENESISAPI void GENESISCC gePhysicsObject_IncAppliedTorque(gePhysicsObject* po, const geVec3d* torqueInc, int configIndex);

GENESISAPI void GENESISCC gePhysicsObject_GetOrientation(const gePhysicsObject* po, geQuaternion* orient, int configIndex);
GENESISAPI void GENESISCC gePhysicsObject_SetOrientation(gePhysicsObject* po, const geQuaternion* orient, int configIndex);

GENESISAPI void GENESISCC gePhysicsObject_GetInertiaTensor(const gePhysicsObject* po, Matrix33* iTensor);
GENESISAPI void GENESISCC gePhysicsObject_GetInertiaTensorInverse(const gePhysicsObject* po, Matrix33* iTensorInv);

GENESISAPI void GENESISCC gePhysicsObject_GetInertiaTensorInPhysicsSpace(const gePhysicsObject* pPhysob, Matrix33* pITensor, int configIndex);
GENESISAPI void GENESISCC gePhysicsObject_GetInertiaTensorInverseInPhysicsSpace(const gePhysicsObject* pPhysob, Matrix33* pITensorInv, int configIndex);

GENESISAPI geBoolean GENESISCC gePhysicsObject_IsAffectedByGravity(const gePhysicsObject* po);
GENESISAPI void GENESISCC gePhysicsObject_SetIsAffectedByGravity(gePhysicsObject* po, geBoolean flag);

GENESISAPI geBoolean GENESISCC gePhysicsObject_RespondsToForces(const gePhysicsObject* po);
GENESISAPI void GENESISCC gePhysicsObject_SetRespondsToForces(gePhysicsObject* po, geBoolean flag);

GENESISAPI float GENESISCC gePhysicsObject_GetLinearDamping(const gePhysicsObject* po);
GENESISAPI void GENESISCC gePhysicsObject_SetLinearDamping(gePhysicsObject* po, float linearDamping);

GENESISAPI float GENESISCC gePhysicsObject_GetAngularDamping(const gePhysicsObject* po);
GENESISAPI void GENESISCC gePhysicsObject_SetAngularDamping(gePhysicsObject* po, float angularDamping);

GENESISAPI void GENESISCC gePhysicsObject_SetActiveConfig(gePhysicsObject* pPhysob, int configIndex);
GENESISAPI int GENESISCC gePhysicsObject_GetActiveConfig(gePhysicsObject* pPhysob);

GENESISAPI void GENESISCC gePhysicsObject_SetPhysicsScale(gePhysicsObject* pPhysob, float scale);
GENESISAPI float GENESISCC gePhysicsObject_GetPhysicsScale(gePhysicsObject* pPhysob);

#ifdef __cplusplus
}
#endif

#endif

