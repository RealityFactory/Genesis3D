/****************************************************************************************/
/*  PHYSICSOBJECT.C                                                                     */
/*                                                                                      */
/*  Author: Jason Wood                                                                  */
/*  Description: Constrained rigid body implementation                                  */
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
/*Genesis3D Version 1.1 released November 15, 1999                            */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#include <math.h>
#include <assert.h>
#include <stdio.h>

#include "genesis.h"
#include "ram.h"
#include "matrix33.h"
#include "quatern.h"


#include "PhysicsObject.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// gePhysicsObject structure definition

typedef struct
{
	geXForm3d xform; // includes location and rotation matrix
	geVec3d linearVelocity;
	geVec3d angularVelocity;
	geVec3d force, appliedForce;
	geVec3d torque, appliedTorque;
	geQuaternion orientation;

}	gePhysicsObject_Config;

typedef struct gePhysicsObject
{
	float mass, oneOverMass;
	
	geVec3d	OriginalLocation;
	
	Matrix33 inertiaTensor,
		inertiaTensorInverse;
	geBoolean							isAffectedByGravity;
	geBoolean							respondsToForces;
	float						linearDamping;
	float						angularDamping;

	int activeConfig;
	gePhysicsObject_Config configs[2];

	float physicsScale;

}	gePhysicsObject;

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ctor / dtor

GENESISAPI gePhysicsObject * GENESISCC gePhysicsObject_Create(
	const geVec3d *StartLocation,
	float mass,
	geBoolean IsAffectedByGravity,
	geBoolean RespondsToForces,
	float linearDamping,
	float angularDamping,
	const geVec3d *	Mins,
	const geVec3d *	Maxs,
	float physicsScale)
{
	gePhysicsObject*	pgePhysicsObject;
	geVec3d defaultAxis;
	geVec3d	bbScale;
	int i;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	pgePhysicsObject = NULL;
	pgePhysicsObject = GE_RAM_ALLOCATE_STRUCT(gePhysicsObject);

	if (pgePhysicsObject == NULL)
		return NULL;

	assert(pgePhysicsObject != NULL);

	////////////////////////////////////////////////////////////////////////////////////////////////////
	// fill user data with relevant class data

	pgePhysicsObject->respondsToForces = RespondsToForces;
	pgePhysicsObject->isAffectedByGravity = IsAffectedByGravity;

	pgePhysicsObject->linearDamping = linearDamping;
	pgePhysicsObject->angularDamping = angularDamping;

	pgePhysicsObject->physicsScale = physicsScale;
	geVec3d_Copy(StartLocation, &pgePhysicsObject->OriginalLocation);
	geVec3d_Scale(&pgePhysicsObject->OriginalLocation, physicsScale, &pgePhysicsObject->OriginalLocation);

	assert(mass >= 0.1f);

	pgePhysicsObject->mass = mass;
	pgePhysicsObject->oneOverMass = 1.f / mass;	

	////////////////////////////////////////////////////////////////////////////////////////////////////
	// initialize gePhysicsObject's state

	for (i = 0; i < 2; i ++)
	{		
		geVec3d_Clear(&pgePhysicsObject->configs[i].linearVelocity);
		geVec3d_Clear(&pgePhysicsObject->configs[i].angularVelocity);
		geVec3d_Clear(&pgePhysicsObject->configs[i].force);
		geVec3d_Clear(&pgePhysicsObject->configs[i].torque);
		geVec3d_Clear(&pgePhysicsObject->configs[i].appliedForce);
		geVec3d_Clear(&pgePhysicsObject->configs[i].appliedTorque);

		geVec3d_Set(&defaultAxis, 1.f, 0.f, 0.f);
		geQuaternion_SetFromAxisAngle(&pgePhysicsObject->configs[i].orientation,
			&defaultAxis,
			0.f);

		geXForm3d_SetIdentity(&pgePhysicsObject->configs[i].xform);
		geVec3d_Clear(&pgePhysicsObject->configs[i].xform.Translation);
	}

	Matrix33_SetIdentity(&pgePhysicsObject->inertiaTensor);
	Matrix33_SetIdentity(&pgePhysicsObject->inertiaTensorInverse);

	geVec3d_Subtract(Maxs, Mins, &bbScale);
	geVec3d_Scale(&bbScale, physicsScale, &bbScale);

	////////////////////////////////////////////////////////////////////////////////////////////////////
	// compute gePhysicsObject's inertia tensor and inverse
	// we assume the gePhysicsObject is an axis-aligned box

	pgePhysicsObject->inertiaTensor.x[0][0] = pgePhysicsObject->mass / 12.f * (bbScale.Y * bbScale.Y + bbScale.Z * bbScale.Z);
	pgePhysicsObject->inertiaTensor.x[1][1] = pgePhysicsObject->mass / 12.f * (bbScale.X * bbScale.X + bbScale.Z * bbScale.Z);
	pgePhysicsObject->inertiaTensor.x[2][2] = pgePhysicsObject->mass / 12.f * (bbScale.X * bbScale.X + bbScale.Y * bbScale.Y);

	assert(pgePhysicsObject->inertiaTensor.x[0][0] > (float)1e-5);
	assert(pgePhysicsObject->inertiaTensor.x[1][1] > (float)1e-5);
	assert(pgePhysicsObject->inertiaTensor.x[2][2] > (float)1e-5);

	pgePhysicsObject->inertiaTensorInverse.x[0][0] = 1 / pgePhysicsObject->inertiaTensor.x[0][0];
	pgePhysicsObject->inertiaTensorInverse.x[1][1] = 1 / pgePhysicsObject->inertiaTensor.x[1][1];
	pgePhysicsObject->inertiaTensorInverse.x[2][2] = 1 / pgePhysicsObject->inertiaTensor.x[2][2];

	pgePhysicsObject->activeConfig = 0;

	return pgePhysicsObject;
}

GENESISAPI geBoolean GENESISCC gePhysicsObject_Destroy(gePhysicsObject** pPhysob)
{
	assert(pPhysob != NULL);
	assert(*pPhysob != NULL);

	geRam_Free(*pPhysob);
	*pPhysob = NULL;

	return GE_TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// functions

// apply force in global frame with changes taking effect on next iteration of gePhysicsObject's owner
GENESISAPI geBoolean GENESISCC gePhysicsObject_ApplyGlobalFrameForce(gePhysicsObject* pod, geVec3d* force, geVec3d* radiusVector, geBoolean isAppliedForce,
	int configIndex)
{
	gePhysicsObject_Config* pConfig;
	geVec3d torqueToAdd;

	assert(pod != NULL);
	assert(force != NULL);
	assert(radiusVector != NULL);
	assert( configIndex >= 0 );
	assert( configIndex <  2 );

	pConfig = &pod->configs[configIndex];

	if (! isAppliedForce)
	{
		geVec3d_Add(force, &pConfig->force, &pConfig->force);

		geVec3d_CrossProduct(radiusVector, force, &torqueToAdd);
		geVec3d_Add(&torqueToAdd, &pConfig->torque, &pConfig->torque);
	}

	else
	{
		geVec3d_Add(force, &pConfig->appliedForce, &pConfig->appliedForce);

		geVec3d_CrossProduct(radiusVector, force, &torqueToAdd);
		geVec3d_Add(&torqueToAdd, &pConfig->appliedTorque, &pConfig->appliedTorque);
	}

	return GE_TRUE;
}

// apply impulse in global frame with immediate change in velocities
GENESISAPI geBoolean GENESISCC gePhysicsObject_ApplyGlobalFrameImpulse(
									gePhysicsObject* pPhysob, 
									geVec3d* pImpulse, 
									geVec3d* pRadVec, 
									int configIndex)
{
	gePhysicsObject_Config* pConfig;
	Matrix33 R, Rt;
	geVec3d rCrossRW, rCrossRL, dv, dw;
	assert( pPhysob  != NULL );
	assert( pImpulse != NULL );
	assert( pRadVec  != NULL );
	assert( configIndex >= 0 );
	assert( configIndex <  2 );

	pConfig = &pPhysob->configs[configIndex];

	geVec3d_Scale(pImpulse, pPhysob->oneOverMass, &dv);
	geVec3d_Add(&dv, &pConfig->linearVelocity, &pConfig->linearVelocity);

	geVec3d_CrossProduct(pRadVec, pImpulse, &rCrossRW);

	Matrix33_ExtractFromXForm3d(&pConfig->xform, &R);
	Matrix33_GetTranspose(&R, &Rt);

	Matrix33_MultiplyVec3d(&Rt, &rCrossRW, &rCrossRL);
	Matrix33_MultiplyVec3d(&pPhysob->inertiaTensorInverse, &rCrossRL, &dw);
	geVec3d_Add(&dw, &pConfig->angularVelocity, &pConfig->angularVelocity);

	return GE_TRUE;
}

GENESISAPI geBoolean GENESISCC gePhysicsObject_ComputeForces(gePhysicsObject* pod, int configIndex)
{
	gePhysicsObject_Config* pConfig = &pod->configs[configIndex];
	assert( configIndex >= 0 );
	assert( configIndex <  2 );
	assert( pod != NULL );

	// add damping
	geVec3d_Scale(&pConfig->linearVelocity, 1.f - pod->linearDamping, &pConfig->linearVelocity);
	geVec3d_Scale(&pConfig->angularVelocity, 1.f - pod->angularDamping, &pConfig->angularVelocity);

	// clear force and torque accumulators
	geVec3d_Clear(&pConfig->force);
	geVec3d_Clear(&pConfig->torque);
	
	// add gravity
	if (pod->isAffectedByGravity)
		geVec3d_Set(&pConfig->force, 0.f, PHYSICSOBJECT_GRAVITY * pod->mass, 0.f);

	// add forces
	if (! pod->respondsToForces) return GE_TRUE;

	geVec3d_Add(&pConfig->appliedForce, &pConfig->force, &pConfig->force);
	geVec3d_Add(&pConfig->appliedTorque, &pConfig->torque, &pConfig->torque);	
	
	return GE_TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// integrate a gePhysicsObject's equations of motion by time step deltaTime

GENESISAPI geBoolean GENESISCC gePhysicsObject_Integrate(
				gePhysicsObject* pod, 
				float dt, 
				int sourceConfigIndex)
{
	gePhysicsObject_Config* pSourceConfig, *pTargetConfig;
	geVec3d tau;
	geVec3d a, dv;
	geVec3d angularMomentum, angularAcceleration;
	geVec3d omega_x_L, term1, dtTimesOmega;
	Matrix33 R, Rt;
	
	float qmag;
	geQuaternion qdot;
	float G[3][4], Gt[4][3];
	int i, j;
	static int M[]={0x696C6345,0x21657370};
	float dt2 = 0.5f * (dt * dt);

	assert( sourceConfigIndex >= 0 );
	assert( sourceConfigIndex <  2 );
	assert( pod != NULL );

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	pSourceConfig = &pod->configs[sourceConfigIndex];
	pTargetConfig = &pod->configs[1 - sourceConfigIndex];

	geVec3d_Scale(&pSourceConfig->force, pod->oneOverMass, &a);
	geVec3d_Scale(&a, dt, &dv);
	geVec3d_Add(&pSourceConfig->linearVelocity, &dv, &pTargetConfig->linearVelocity);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	Matrix33_MultiplyVec3d(&pod->inertiaTensor, &pSourceConfig->angularVelocity, &angularMomentum);
	geVec3d_CrossProduct(&pSourceConfig->angularVelocity, &angularMomentum, &omega_x_L);

	////////////////////////////////////////////////////////////////////////////////////////////////////
	// compute torque in body frame (= R ^ T * torque)

	Matrix33_ExtractFromXForm3d(&pSourceConfig->xform, &R);
	Matrix33_GetTranspose(&R, &Rt);
	Matrix33_MultiplyVec3d(&Rt, &pSourceConfig->torque, &tau);

	geVec3d_Subtract(&tau, &omega_x_L, &term1);

	Matrix33_MultiplyVec3d(&pod->inertiaTensorInverse, &term1, &angularAcceleration);	

	geXForm3d_Rotate(&pSourceConfig->xform, &pSourceConfig->angularVelocity, &dtTimesOmega);
	geVec3d_Scale(&dtTimesOmega, dt, &dtTimesOmega);

	G[0][0] = -pSourceConfig->orientation.X; 
	G[0][1] = pSourceConfig->orientation.W; 
	G[0][2] = -pSourceConfig->orientation.Z; 
	G[0][3] = pSourceConfig->orientation.Y;

	G[1][0] = -pSourceConfig->orientation.Y; 
	G[1][1] = pSourceConfig->orientation.Z; 
	G[1][2] = pSourceConfig->orientation.W; 
	G[1][3] = -pSourceConfig->orientation.X;

	G[2][0] = -pSourceConfig->orientation.Z; 
	G[2][1] = -pSourceConfig->orientation.Y; 
	G[2][2] = pSourceConfig->orientation.X; 
	G[2][3] = pSourceConfig->orientation.W;

	for (i = 0; i < 3; i++)
		for (j = 0; j < 4; j++)
			Gt[j][i] = 0.5f * G[i][j];

	qdot.W = Gt[0][0] * dtTimesOmega.X + Gt[0][1] * dtTimesOmega.Y + Gt[0][2] * dtTimesOmega.Z;
	qdot.X = Gt[1][0] * dtTimesOmega.X + Gt[1][1] * dtTimesOmega.Y + Gt[1][2] * dtTimesOmega.Z;
	qdot.Y = Gt[2][0] * dtTimesOmega.X + Gt[2][1] * dtTimesOmega.Y + Gt[2][2] * dtTimesOmega.Z;
	qdot.Z = Gt[3][0] * dtTimesOmega.X + Gt[3][1] * dtTimesOmega.Y + Gt[3][2] * dtTimesOmega.Z;

	pTargetConfig->orientation.W = pSourceConfig->orientation.W + qdot.W;
	pTargetConfig->orientation.X = pSourceConfig->orientation.X + qdot.X;
	pTargetConfig->orientation.Y = pSourceConfig->orientation.Y + qdot.Y;
	pTargetConfig->orientation.Z = pSourceConfig->orientation.Z + qdot.Z;
	
	qmag = geQuaternion_Normalize(&pTargetConfig->orientation);
	geQuaternion_ToMatrix(&pTargetConfig->orientation, &pTargetConfig->xform);	
	
	pTargetConfig->angularVelocity.X = pSourceConfig->angularVelocity.X + dt * angularAcceleration.X;	
	pTargetConfig->angularVelocity.Y = pSourceConfig->angularVelocity.Y + dt * angularAcceleration.Y;
	pTargetConfig->angularVelocity.Z = pSourceConfig->angularVelocity.Z + dt * angularAcceleration.Z;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	pTargetConfig->xform.Translation.X =
		pSourceConfig->xform.Translation.X + dt * pSourceConfig->linearVelocity.X + dt2 * a.X;
	pTargetConfig->xform.Translation.Y =
		pSourceConfig->xform.Translation.Y + dt * pSourceConfig->linearVelocity.Y + dt2 * a.Y;
	pTargetConfig->xform.Translation.Z =
		pSourceConfig->xform.Translation.Z + dt * pSourceConfig->linearVelocity.Z + dt2 * a.Z;

	return GE_TRUE;
}

GENESISAPI float GENESISCC gePhysicsObject_GetMass(const gePhysicsObject* po)
{
	assert(po != NULL);

	return po->mass;
}

GENESISAPI void GENESISCC gePhysicsObject_SetMass(gePhysicsObject* po, float mass)
{
	assert(po != NULL);

	po->mass = mass;

	#pragma message("TODO: set i tensor")
}

GENESISAPI float GENESISCC gePhysicsObject_GetOneOverMass(const gePhysicsObject* po)
{
	assert(po != NULL);

	return po->oneOverMass;
}

GENESISAPI void GENESISCC gePhysicsObject_GetXForm(	const gePhysicsObject* po, 
													geXForm3d* xform, 
													int configIndex)
{
	assert(po != NULL);
	assert(xform != NULL);
	assert( configIndex >= 0 );
	assert( configIndex <  2 );

	geXForm3d_Copy(&po->configs[configIndex].xform, xform);
}

GENESISAPI void GENESISCC gePhysicsObject_SetXForm(	gePhysicsObject* po, 
													const geXForm3d* xform, 
													int configIndex)
{
	assert(po != NULL);
	assert(xform != NULL);
	assert( configIndex >= 0 );
	assert( configIndex <  2 );

	geXForm3d_Copy(xform, &po->configs[configIndex].xform);
}

GENESISAPI void GENESISCC gePhysicsObject_GetXFormInEditorSpace(const gePhysicsObject* po, 
																geXForm3d* xform, 
																int configIndex)
{
	assert(po != NULL);
	assert(xform != NULL);
	assert( configIndex >= 0 );
	assert( configIndex <  2 );

	geXForm3d_Copy(&po->configs[configIndex].xform, xform);
	geVec3d_Scale(&xform->Translation, 1 / po->physicsScale, &xform->Translation);
}

GENESISAPI void GENESISCC gePhysicsObject_GetOriginalLocation(const gePhysicsObject* po, geVec3d* loc)
{
	assert(po != NULL);
	assert(loc != NULL);

	geVec3d_Copy(&po->OriginalLocation, loc);
}

GENESISAPI void GENESISCC gePhysicsObject_SetOriginalLocation(gePhysicsObject* po, const geVec3d* loc)
{
	assert(po != NULL);
	assert(loc != NULL);

	geVec3d_Copy(loc, &po->OriginalLocation);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// get gePhysicsObject's location in Physics space (xlation and olocation are in Physics space units)
GENESISAPI void GENESISCC gePhysicsObject_GetLocation(	const gePhysicsObject *po, 
														geVec3d *Location, 
														int configIndex)
{
	assert(po != NULL);
	assert(Location != NULL);
	assert( configIndex >= 0 );
	assert( configIndex <  2 );

	geVec3d_Add(&po->configs[configIndex].xform.Translation, &po->OriginalLocation, Location);
}

// get gePhysicsObject's location in editor(world) space
GENESISAPI void GENESISCC gePhysicsObject_GetLocationInEditorSpace(const gePhysicsObject* po, 
																	geVec3d* loc, 
																	int configIndex)
{
	assert(po != NULL);
	assert(loc != NULL);
	assert( configIndex >= 0 );
	assert( configIndex <  2 );

	geVec3d_Add(&po->OriginalLocation, &po->configs[configIndex].xform.Translation, loc);
	geVec3d_Scale(loc, 1 / po->physicsScale, loc);
}

GENESISAPI void GENESISCC gePhysicsObject_GetLinearVelocity(const gePhysicsObject* po, 
															geVec3d* vel, 
															int configIndex)
{
	assert(po != NULL);
	assert(vel != NULL);
	assert( configIndex >= 0 );
	assert( configIndex <  2 );

	geVec3d_Copy(&po->configs[configIndex].linearVelocity, vel);
}

GENESISAPI void GENESISCC gePhysicsObject_SetLinearVelocity(gePhysicsObject* po, 
															const geVec3d* vel, 
															int configIndex)
{
	assert(po != NULL);
	assert(vel != NULL);
	assert( configIndex >= 0 );
	assert( configIndex <  2 );

	geVec3d_Copy(vel, &po->configs[configIndex].linearVelocity);
}

GENESISAPI void GENESISCC gePhysicsObject_GetAngularVelocity(const gePhysicsObject* po, 
															 geVec3d* vel, 
															 int configIndex)
{
	assert(po != NULL);
	assert(vel != NULL);
	assert( configIndex >= 0 );
	assert( configIndex <  2 );

	geVec3d_Copy(&po->configs[configIndex].angularVelocity, vel);
}

GENESISAPI void GENESISCC gePhysicsObject_SetAngularVelocity(gePhysicsObject* po, 
															 const geVec3d* vel, 
															 int configIndex)
{
	assert(po != NULL);
	assert(vel != NULL);
	assert( configIndex >= 0 );
	assert( configIndex <  2 );

	geVec3d_Copy(vel, &po->configs[configIndex].angularVelocity);
}

GENESISAPI void GENESISCC gePhysicsObject_GetForce( const gePhysicsObject* po, 
													geVec3d* force, 
													int configIndex)
{
	assert(po != NULL);
	assert(force != NULL);
	assert( configIndex >= 0 );
	assert( configIndex <  2 );

	geVec3d_Copy(&po->configs[configIndex].force, force);
}

GENESISAPI void GENESISCC gePhysicsObject_SetForce(	gePhysicsObject* po, 
													const geVec3d* force, 
													int configIndex)
{
	assert(po != NULL);
	assert(force != NULL);
	assert( configIndex >= 0 );
	assert( configIndex <  2 );

	geVec3d_Copy(force, &po->configs[configIndex].force);
}

GENESISAPI void GENESISCC gePhysicsObject_GetTorque(const gePhysicsObject* po, 
													geVec3d* torque, 
													int configIndex)
{
	assert(po != NULL);
	assert(torque != NULL);
	assert( configIndex >= 0 );
	assert( configIndex <  2 );

	geVec3d_Copy(&po->configs[configIndex].torque, torque);
}

GENESISAPI void GENESISCC gePhysicsObject_SetTorque(gePhysicsObject* po, 
													const geVec3d* torque, 
													int configIndex)
{
	assert(po != NULL);
	assert(torque != NULL);
	assert( configIndex >= 0 );
	assert( configIndex <  2 );

	geVec3d_Copy(torque, &po->configs[configIndex].torque);
}

GENESISAPI void GENESISCC gePhysicsObject_GetAppliedForce(	const gePhysicsObject* po, 
															geVec3d* force, 
															int configIndex)
{
	assert(po != NULL);
	assert(force != NULL);
	assert( configIndex >= 0 );
	assert( configIndex <  2 );

	geVec3d_Copy(&po->configs[configIndex].appliedForce, force);
}

GENESISAPI void GENESISCC gePhysicsObject_SetAppliedForce(	gePhysicsObject* po, 
															const geVec3d* force, 
															int configIndex)
{
	assert(po != NULL);
	assert(force != NULL);
	assert( configIndex >= 0 );
	assert( configIndex <  2 );

	geVec3d_Copy(force, &po->configs[configIndex].appliedForce);
}

GENESISAPI void GENESISCC gePhysicsObject_GetAppliedTorque(	const gePhysicsObject* po, 
															geVec3d* torque, 
															int configIndex)
{
	assert(po != NULL);
	assert(torque != NULL);
	assert( configIndex >= 0 );
	assert( configIndex <  2 );

	geVec3d_Copy(&po->configs[configIndex].appliedTorque, torque);
}

GENESISAPI void GENESISCC gePhysicsObject_SetAppliedTorque(	gePhysicsObject* po, 
															const geVec3d* torque, 
															int configIndex)
{
	assert(po != NULL);
	assert(torque != NULL);
	assert( configIndex >= 0 );
	assert( configIndex <  2 );

	geVec3d_Copy(torque, &po->configs[configIndex].appliedTorque);
}

GENESISAPI void GENESISCC gePhysicsObject_ClearForce(gePhysicsObject* po, int configIndex)
{
	assert(po != NULL);
	assert( configIndex >= 0 );
	assert( configIndex <  2 );

	geVec3d_Clear(&po->configs[configIndex].force);
}

GENESISAPI void GENESISCC gePhysicsObject_ClearTorque(gePhysicsObject* po, int configIndex)
{
	assert(po != NULL);
	assert( configIndex >= 0 );
	assert( configIndex <  2 );

	geVec3d_Clear(&po->configs[configIndex].torque);
}

GENESISAPI void GENESISCC gePhysicsObject_ClearAppliedForce(gePhysicsObject* po, int configIndex)
{
	assert(po != NULL);
	assert( configIndex >= 0 );
	assert( configIndex <  2 );

	geVec3d_Clear(&po->configs[configIndex].appliedForce);
}

GENESISAPI void GENESISCC gePhysicsObject_ClearAppliedTorque(gePhysicsObject* po, int configIndex)
{
	assert(po != NULL);
	assert( configIndex >= 0 );
	assert( configIndex <  2 );

	geVec3d_Clear(&po->configs[configIndex].appliedTorque);
}

GENESISAPI void GENESISCC gePhysicsObject_IncForce(	gePhysicsObject* po, 
													const geVec3d* forceInc, 
													int configIndex)
{
	assert(po != NULL);
	assert(forceInc != NULL);
	assert( configIndex >= 0 );
	assert( configIndex <  2 );

	geVec3d_Add(&po->configs[configIndex].force, forceInc, &po->configs[configIndex].force);
}

GENESISAPI void GENESISCC gePhysicsObject_IncTorque(gePhysicsObject* po, 
													const geVec3d* torqueInc, 
													int configIndex)
{
	assert(po != NULL);
	assert(torqueInc != NULL);
	assert( configIndex >= 0 );
	assert( configIndex <  2 );

	geVec3d_Add(&po->configs[configIndex].torque, torqueInc, &po->configs[configIndex].torque);
}

GENESISAPI void GENESISCC gePhysicsObject_IncAppliedForce(	gePhysicsObject* po, 
															const geVec3d* forceInc, 
															int configIndex)
{
	assert(po != NULL);
	assert(forceInc != NULL);
	assert( configIndex >= 0 );
	assert( configIndex <  2 );

	geVec3d_Add(&po->configs[configIndex].appliedForce, forceInc, &po->configs[configIndex].appliedForce);
}

GENESISAPI void GENESISCC gePhysicsObject_IncAppliedTorque(gePhysicsObject* po, 
															const geVec3d* torqueInc, int configIndex)
{
	assert(po != NULL);
	assert(torqueInc != NULL);
	assert( configIndex >= 0 );
	assert( configIndex <  2 );

	geVec3d_Add(&po->configs[configIndex].appliedTorque, torqueInc, &po->configs[configIndex].appliedTorque);
}

GENESISAPI void GENESISCC gePhysicsObject_GetOrientation(	const gePhysicsObject* po, 
															geQuaternion* orient, int configIndex)
{
	assert(po != NULL);
	assert(orient != NULL);
	assert( configIndex >= 0 );
	assert( configIndex <  2 );

	geQuaternion_Copy(&po->configs[configIndex].orientation, orient);
}

GENESISAPI void GENESISCC gePhysicsObject_SetOrientation(gePhysicsObject* po, 
														 const geQuaternion* orient, 
														 int configIndex)
{
	assert(po != NULL);
	assert(orient != NULL);
	assert( configIndex >= 0 );
	assert( configIndex <  2 );

	geQuaternion_Copy(orient, &po->configs[configIndex].orientation);
}

// get inertia tensor and inverse in body (local unrotated) space
GENESISAPI void GENESISCC gePhysicsObject_GetInertiaTensor(const gePhysicsObject* po, Matrix33* iTensor)
{
	assert(po != NULL);
	assert(iTensor != NULL);

	Matrix33_Copy(&po->inertiaTensor, iTensor);
}

GENESISAPI void GENESISCC gePhysicsObject_GetInertiaTensorInverse(const gePhysicsObject* po, Matrix33* iTensorInv)
{
	assert(po != NULL);
	assert(iTensorInv != NULL);

	Matrix33_Copy(&po->inertiaTensorInverse, iTensorInv);
}

GENESISAPI void GENESISCC gePhysicsObject_GetInertiaTensorInPhysicsSpace(
										const gePhysicsObject* pPhysob, 
										Matrix33* pITensor, 
										int configIndex)
{
	Matrix33 R, Rt, RJ;

	assert(pPhysob != NULL);
	assert(pITensor != NULL);
	assert( configIndex >= 0 );
	assert( configIndex <  2 );

	Matrix33_ExtractFromXForm3d(&pPhysob->configs[configIndex].xform, &R);
	Matrix33_GetTranspose(&R, &Rt);

	Matrix33_Multiply(&R, &pPhysob->inertiaTensor, &RJ);
	Matrix33_Multiply(&RJ, &Rt, pITensor);
}

GENESISAPI void GENESISCC gePhysicsObject_GetInertiaTensorInverseInPhysicsSpace(
										const gePhysicsObject* pPhysob, 
										Matrix33* pITensorInv, 
										int configIndex)
{
	Matrix33 R, Rt, RJi;

	assert(pPhysob != NULL);
	assert(pITensorInv != NULL);
	assert( configIndex >= 0 );
	assert( configIndex <  2 );

	Matrix33_ExtractFromXForm3d(&pPhysob->configs[configIndex].xform, &R);
	Matrix33_GetTranspose(&R, &Rt);

	Matrix33_Multiply(&R, &pPhysob->inertiaTensorInverse, &RJi);
	Matrix33_Multiply(&RJi, &Rt, pITensorInv);
}

GENESISAPI geBoolean GENESISCC gePhysicsObject_IsAffectedByGravity(const gePhysicsObject* po)
{
	assert(po != NULL);

	return po->isAffectedByGravity;
}

GENESISAPI void GENESISCC gePhysicsObject_SetIsAffectedByGravity(gePhysicsObject* po, geBoolean flag)
{
	assert(po != NULL);

	po->isAffectedByGravity = flag;
}

GENESISAPI geBoolean GENESISCC gePhysicsObject_RespondsToForces(const gePhysicsObject* po)
{
	assert(po != NULL);

	return po->respondsToForces;
}

GENESISAPI void GENESISCC gePhysicsObject_SetRespondsToForces(gePhysicsObject* po, geBoolean flag)
{
	assert(po != NULL);

	po->respondsToForces = flag;
}

GENESISAPI float GENESISCC gePhysicsObject_GetLinearDamping(const gePhysicsObject* po)
{
	assert(po != NULL);

	return po->linearDamping;
}

GENESISAPI void GENESISCC gePhysicsObject_SetLinearDamping(gePhysicsObject* po, float linearDamping)
{
	assert(po != NULL);
	assert(linearDamping >= 0.f && linearDamping <= 1.f);

	po->linearDamping = linearDamping;
}

GENESISAPI float GENESISCC gePhysicsObject_GetAngularDamping(const gePhysicsObject* po)
{
	assert(po != NULL);

	return po->angularDamping;
}

GENESISAPI void GENESISCC gePhysicsObject_SetAngularDamping(gePhysicsObject* po, float angularDamping)
{
	assert(po != NULL);
	assert(angularDamping >= 0.f && angularDamping <= 1.f);

	po->angularDamping = angularDamping;
}

GENESISAPI void GENESISCC gePhysicsObject_SetActiveConfig(gePhysicsObject* pPhysob, int configIndex)
{
	assert(pPhysob != NULL);
	pPhysob->activeConfig = configIndex;
}

GENESISAPI int GENESISCC gePhysicsObject_GetActiveConfig(gePhysicsObject* pPhysob)
{
	assert(pPhysob != NULL);
	return pPhysob->activeConfig;
}

GENESISAPI void GENESISCC gePhysicsObject_SetPhysicsScale(gePhysicsObject* pPhysob, float scale)
{
	assert(pPhysob != NULL);
	pPhysob->physicsScale = scale;
}

GENESISAPI float GENESISCC gePhysicsObject_GetPhysicsScale(gePhysicsObject* pPhysob)
{
	assert(pPhysob != NULL);
	return pPhysob->physicsScale;
}

