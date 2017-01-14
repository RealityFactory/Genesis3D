/****************************************************************************************/
/*  PHYSICSSYSTEM.C                                                                     */
/*                                                                                      */
/*  Author: Jason Wood                                                                  */
/*  Description: Rigid body, constraint based physics system implementation             */
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
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <float.h>

#include "vec3d.h"
#include "xform3d.h"
#include "ram.h"
#include "matrix33.h"
#include "quatern.h"

#include "PhysicsObject.h"
#include "PhysicsJoint.h"
#include "PhysicsSystem.h"

typedef struct LinearSystemStruct
{	
	float ** M;
	float * X;
	float * KVector;
}	LinearSystemStruct;

typedef struct gePhysicsSystem
{	
	int										sumOfConstraintDimensions;
	LinearSystemStruct *	linsys;
	int										PhysicsObjectCount;
	int										PhysicsJointCount;
	gePhysicsObject **							Objects;
	gePhysicsJoint **						Joints;
	float physicsScale;

	int sourceConfigIndex, targetConfigIndex;

}	gePhysicsSystem;


static geBoolean gePhysicsSystem_EnforceConstraints(gePhysicsSystem* physsysPtr, float subStepSize);
static geBoolean gePhysicsSystem_SolveForConstraintForces(gePhysicsSystem* physsysPtr);

static	Matrix33 gePhysicsSystemIdentityMatrix;

GENESISAPI gePhysicsSystem* GENESISCC gePhysicsSystem_Create(void)
{
	gePhysicsSystem* pPhyssys;

	pPhyssys = NULL;
	pPhyssys = GE_RAM_ALLOCATE_STRUCT(gePhysicsSystem);
	if (pPhyssys == NULL)
	{
		return NULL;
	}	

	memset(pPhyssys, 0, sizeof(*pPhyssys));

	Matrix33_SetIdentity(&gePhysicsSystemIdentityMatrix);

	pPhyssys->sourceConfigIndex = 0;
	pPhyssys->targetConfigIndex = 1;

	return pPhyssys;
}

GENESISAPI geBoolean	GENESISCC gePhysicsSystem_AddObject(gePhysicsSystem *PS, gePhysicsObject *Object)
{
	gePhysicsObject **	NewList;
	assert( PS != NULL );

	NewList = geRam_Realloc(PS->Objects, sizeof(*NewList) * (PS->PhysicsObjectCount + 1));
	if	(!NewList)
		return GE_FALSE;

	NewList[PS->PhysicsObjectCount] = Object;
	PS->PhysicsObjectCount++;
	PS->Objects = NewList;

	return GE_TRUE;
}

GENESISAPI geBoolean	GENESISCC gePhysicsSystem_AddJoint(gePhysicsSystem *PS, gePhysicsJoint *Joint)
{
	gePhysicsJoint **	NewList;
	int					i;
	gePhysicsJoint_Kind type;
	assert( PS != NULL );
	assert( Joint != NULL );

	NewList = geRam_Realloc(PS->Joints, sizeof(*NewList) * (PS->PhysicsJointCount + 1));
	if	(!NewList)
		return GE_FALSE;

	NewList[PS->PhysicsJointCount] = Joint;
	PS->PhysicsJointCount++;
	PS->Joints = NewList;

	// Free any old data
	if	(PS->linsys)
	{
		assert(PS->linsys->M);
		assert(PS->linsys->X);
		assert(PS->linsys->KVector);

		for (i = 0; i < PS->sumOfConstraintDimensions; i++)
		{
			assert(PS->linsys->M[i]);
			geRam_Free(PS->linsys->M[i]);
		}

		geRam_Free(PS->linsys->M);
		geRam_Free(PS->linsys->X);
		geRam_Free(PS->linsys->KVector);
	}

	PS->linsys = GE_RAM_ALLOCATE_STRUCT(LinearSystemStruct);
	if (PS->linsys == NULL)
	{
		return GE_FALSE;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	// compute size of linear system

	PS->sumOfConstraintDimensions = 0;
	for (	i = 0; i < PS->PhysicsJointCount; i++)
	{
		type = gePhysicsJoint_GetType(NewList[i]);
		switch (type)
		{
			case JT_WORLD:
			case JT_SPHERICAL:
				PS->sumOfConstraintDimensions += 3;
				break;

			default:
				// shouldn't happen !
				assert(!"Illegal joint kind");
				return GE_FALSE;
		}
	}

	if (PS->sumOfConstraintDimensions == 0)
		return GE_FALSE;

	////////////////////////////////////////////////////////////////////////////////////////////////////
	// alloc mem for the linear system and handle exceptions

	PS->linsys->M = (float**)geRam_Allocate(PS->sumOfConstraintDimensions * sizeof(float*));

	if (PS->linsys->M == NULL)
	{
		return GE_FALSE;
	}

	for (i = 0; i < PS->sumOfConstraintDimensions; i++)
	{
		PS->linsys->M[i] = (float*)geRam_Allocate(PS->sumOfConstraintDimensions * sizeof(float));

		if (PS->linsys->M[i] == NULL)
		{
			return GE_FALSE;
		}
	}

	PS->linsys->X = (float*)geRam_Allocate(PS->sumOfConstraintDimensions * sizeof(float));

	if (PS->linsys->X == NULL)
	{
		return GE_FALSE;
	}

	PS->linsys->KVector = (float*)geRam_Allocate(PS->sumOfConstraintDimensions * sizeof(float));

	if (PS->linsys->KVector == NULL)
	{
		return GE_FALSE;
	}	

	return GE_TRUE;
}

GENESISAPI geBoolean GENESISCC gePhysicsSystem_Destroy(gePhysicsSystem** ppPhyssys)
{
	gePhysicsSystem *	pPhyssys;
	int				i;

	assert(ppPhyssys != NULL);
	assert(*ppPhyssys != NULL);

	pPhyssys = *ppPhyssys;
	// Free any old data
	if	(pPhyssys->linsys)
	{
		assert(pPhyssys->linsys->M);
		assert(pPhyssys->linsys->X);
		assert(pPhyssys->linsys->KVector);

		for (i = 0; i < pPhyssys->sumOfConstraintDimensions; i++)
		{
			assert(pPhyssys->linsys->M[i]);
			geRam_Free(pPhyssys->linsys->M[i]);
		}

		geRam_Free(pPhyssys->linsys->M);
		geRam_Free(pPhyssys->linsys->X);
		geRam_Free(pPhyssys->linsys->KVector);
	}

	geRam_Free(*ppPhyssys);
	*ppPhyssys = NULL;

	return GE_TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// physics stuff follows

static float fmin(float a, float b)
{
	return a < b ? a : b;
}

GENESISAPI geBoolean GENESISCC gePhysicsSystem_Iterate(gePhysicsSystem* psPtr, float Time)
{
	int				i;

	int numIntegrationSteps;
	float minAssemblyRate, subStepSize;
	float amountIntegrated = 0.f;

	assert( psPtr != NULL );

	////////////////////////////////////////////////////////////////////////////////////////////////////
	// integrate numIntegrationSteps times during the frame
	// this is done to ensure smoother motion and enforce constraint stability

	minAssemblyRate = FLT_MAX;

	if (psPtr->PhysicsJointCount == 0)
	{
		numIntegrationSteps = 1;
	}

	else
		numIntegrationSteps = 5;

	if (Time > 0.03f) Time = 0.03f;

	subStepSize = Time / numIntegrationSteps;

	for (	amountIntegrated = 0.f;
				amountIntegrated < Time;
				amountIntegrated += subStepSize)
	{
		for	(i = 0; i < psPtr->PhysicsObjectCount; i++)
		{
			if (!gePhysicsObject_ComputeForces(psPtr->Objects[i], psPtr->sourceConfigIndex))
				return GE_FALSE;
		}			

		////////////////////////////////////////////////////////////////////////////////////////////////////
		// enforce constraints

		if (psPtr->sumOfConstraintDimensions > 0)
		{
			if (!gePhysicsSystem_EnforceConstraints(psPtr, subStepSize))
				return GE_FALSE;
		}

		for	(i = 0; i < psPtr->PhysicsObjectCount; i++)
		{
			if (!gePhysicsObject_Integrate(psPtr->Objects[i], subStepSize, psPtr->sourceConfigIndex))
				return GE_FALSE;							
		}

		psPtr->sourceConfigIndex = (psPtr->sourceConfigIndex == 0 ? 1 : 0);
		psPtr->targetConfigIndex = (psPtr->targetConfigIndex == 0 ? 1 : 0);
		
		// let physical object's control fns update themselves		
	}

	for	(i = 0; i < psPtr->PhysicsObjectCount; i++)
	{
		gePhysicsObject* pod;		
		
		pod = psPtr->Objects[i];
		
		gePhysicsObject_ClearAppliedForce(pod, psPtr->sourceConfigIndex);
		gePhysicsObject_ClearAppliedTorque(pod, psPtr->sourceConfigIndex);
		gePhysicsObject_SetActiveConfig(psPtr->Objects[i], psPtr->sourceConfigIndex);
	}

	return GE_TRUE;
}

static geBoolean gePhysicsSystem_EnforceConstraints(gePhysicsSystem* PS, float subStepSize)
{
	float h, hSquared;
	geVec3d beta, D0, D1;
	geVec3d K;
	geVec3d rA, rB;
	geVec3d accA, accB;
	geVec3d velA, velB;

	Matrix33 tmpMat;
	Matrix33 term11, term12, term21, term22;
	Matrix33 rStarA, itA, itiA, rStarB, itB, itiB;
	Matrix33 rotA, rotB, rotAt, rotBt;

	geVec3d tmpVec;
	geVec3d jntLocA, jntLocB;
	geVec3d jntLocAWS, jntLocBWS;
	geVec3d offsetVecA, offsetVecB;
	geVec3d omegaA, LA, omegaB, LB;
	geVec3d zeroForceAccelerationA, zeroForceAccelerationB;
	geVec3d ptVelocityA, ptVelocityB;
	geVec3d alphaA, alphaB;
	geVec3d ptAccelerationA, ptAccelerationB;

	geVec3d linearVelocityA, linearVelocityB;
	geVec3d angularVelocityA, angularVelocityB;

	geXForm3d xformA, xformB;

	Matrix33 iTensorA, iTensorB;
	Matrix33 iTensorInvA, iTensorInvB;

	geVec3d constraintForce;
	Matrix33 Mblock;

	int i, j, k, iOffset;
	int size;

	int si;

	gePhysicsJoint*	jntData;
	gePhysicsObject *podA, *podB;

	gePhysicsJoint_Kind type;

	////////////////////////////////////////////////////////////////////////////////////////////////////
	// BEGIN
	assert( PS != NULL );

	si = PS->sourceConfigIndex;

	assert(PS != NULL);

	size = PS->sumOfConstraintDimensions;

	for (i = 0; i < size; i++)
		for (j = 0; j < size; j++)
			PS->linsys->M[i][j] = 0.f;


	for	(i = 0, iOffset = 0; i < PS->PhysicsJointCount; i++)
	{
		jntData = PS->Joints[i];
		h = gePhysicsJoint_GetAssemblyRate(jntData);
		hSquared = h * h;

		////////////////////////////////////////////////////////////////////////////////////////////////////
		// compute joints' actual locations in the world

		type = gePhysicsJoint_GetType(jntData);

		switch(type)
		{
			case JT_WORLD:
				
				podA = gePhysicsJoint_GetObject1(jntData);
				
				assert(podA != NULL);				

				gePhysicsObject_GetLocation(podA, &offsetVecA, si);
				gePhysicsObject_GetXForm(podA, &xformA, si);
				gePhysicsJoint_GetLocationA(jntData, &jntLocA);

				geXForm3d_Rotate(&xformA, &jntLocA, &rA);
				geVec3d_Add(&offsetVecA, &rA, &tmpVec);
				gePhysicsJoint_SetLocationAInWorldSpace(jntData, &tmpVec);

				Matrix33_MakeCrossProductMatrix33(&rA, &rStarA);

				Matrix33_ExtractFromXForm3d(&xformA, &rotA);
				Matrix33_GetTranspose(&rotA, &rotAt);

				gePhysicsObject_GetInertiaTensor(podA, &iTensorA);
				gePhysicsObject_GetInertiaTensorInverse(podA, &iTensorInvA);

				Matrix33_Multiply(&rotA, &iTensorA, &tmpMat);
				Matrix33_Multiply(&tmpMat, &rotAt, &itA);

				Matrix33_Multiply(&rotA, &iTensorInvA, &tmpMat);
				Matrix33_Multiply(&tmpMat, &rotAt, &itiA);

				gePhysicsObject_GetAngularVelocity(podA, &angularVelocityA, si);

				Matrix33_MultiplyVec3d(&rotA, &angularVelocityA, &omegaA);
				Matrix33_MultiplyVec3d(&itA, &omegaA, &LA);

				////////////////////////////////////////////////////////////////////////////////////////////////////
				// create M submatrix

				Matrix33_MultiplyScalar(gePhysicsObject_GetOneOverMass(podA), &gePhysicsSystemIdentityMatrix, &term11);
				Matrix33_Multiply(&rStarA, &itiA, &tmpMat);
				Matrix33_Multiply(&tmpMat, &rStarA, &term12);
				Matrix33_Subtract(&term11, &term12, &Mblock);

				////////////////////////////////////////////////////////////////////////////////////////////////////
				// compute deviation

				geVec3d_CrossProduct(&omegaA, &LA, &zeroForceAccelerationA);
				geVec3d_CrossProduct(&omegaA, &rA, &ptVelocityA);
				geVec3d_CrossProduct(&omegaA, &ptVelocityA, &alphaA);
				geVec3d_Add(&zeroForceAccelerationA, &alphaA, &ptAccelerationA);

				////////////////////////////////////////////////////////////////////////////////////////////////////
				// tmpMat holds rStarA * itiA

				Matrix33_MultiplyVec3d(&tmpMat, &ptAccelerationA, &beta);

				gePhysicsObject_GetLinearVelocity(podA, &linearVelocityA, si);
				geVec3d_Add(&linearVelocityA, &ptVelocityA, &D1);

				gePhysicsJoint_GetLocationAInWorldSpace(jntData, &jntLocAWS);
				gePhysicsJoint_GetLocationB(jntData, &jntLocB);
				geVec3d_Subtract(&jntLocAWS, &jntLocB, &D0);

				////////////////////////////////////////////////////////////////////////////////////////////////////
				// compute K subvector

				geVec3d_Scale(&D1, 2.f / h, &D1);
				geVec3d_Scale(&D0, 1.f / hSquared, &D0);
				geVec3d_Add(&beta, &D1, &K);
				geVec3d_Add(&D0, &K, &K);

				////////////////////////////////////////////////////////////////////////////////////////////////////
				// fill linear system appropriately

				PS->linsys->KVector[iOffset] = -K.X;
				PS->linsys->KVector[iOffset + 1] = -K.Y;
				PS->linsys->KVector[iOffset + 2] = -K.Z;

				for (j = 0; j < 3; j++)
				{
					for (k = 0; k < 3; k++)
					{
						PS->linsys->M[iOffset + j][iOffset + k] = Mblock.x[j][k];
					}
				}

				iOffset += 3;
				break;
			
			case JT_SPHERICAL:

				////////////////////////////////////////////////////////////////////////////////////////////////////
				 // compute joint locations in world space
				
				////////////////////////////////////////////////////////////////////////////////////////////////////
				// for gePhysicsObject A

				podA = gePhysicsJoint_GetObject1(jntData);

				assert(podA != NULL);

				gePhysicsObject_GetLocation(podA, &offsetVecA, si);
				gePhysicsObject_GetXForm(podA, &xformA, si);
				gePhysicsJoint_GetLocationA(jntData, &jntLocA);

				geXForm3d_Rotate(&xformA, &jntLocA, &rA);				
				geVec3d_Add(&offsetVecA, &rA, &tmpVec);
				gePhysicsJoint_SetLocationAInWorldSpace(jntData, &tmpVec);

				////////////////////////////////////////////////////////////////////////////////////////////////////
				// for gePhysicsObject B
					
				podB = gePhysicsJoint_GetObject2(jntData);

				assert(podB != NULL);

				gePhysicsObject_GetLocation(podB, &offsetVecB, si);
				gePhysicsObject_GetXForm(podB, &xformB, si);
				gePhysicsJoint_GetLocationB(jntData, &jntLocB);

				geXForm3d_Rotate(&xformB, &jntLocB, &rB);				
				geVec3d_Add(&offsetVecB, &rB, &tmpVec);
				gePhysicsJoint_SetLocationBInWorldSpace(jntData, &tmpVec);

				////////////////////////////////////////////////////////////////////////////////////////////////////
				// do physics setup

				////////////////////////////////////////////////////////////////////////////////////////////////////
				// for gePhysicsObject A								

				Matrix33_MakeCrossProductMatrix33(&rA, &rStarA);

				Matrix33_ExtractFromXForm3d(&xformA, &rotA);
				Matrix33_GetTranspose(&rotA, &rotAt);

				gePhysicsObject_GetInertiaTensor(podA, &iTensorA);
				gePhysicsObject_GetInertiaTensorInverse(podA, &iTensorInvA);

				Matrix33_Multiply(&rotA, &iTensorA, &tmpMat);
				Matrix33_Multiply(&tmpMat, &rotAt, &itA);

				Matrix33_Multiply(&rotA, &iTensorInvA, &tmpMat);
				Matrix33_Multiply(&tmpMat, &rotAt, &itiA);

				gePhysicsObject_GetAngularVelocity(podA, &angularVelocityA, si);

				Matrix33_MultiplyVec3d(&rotA, &angularVelocityA, &omegaA);
				Matrix33_MultiplyVec3d(&itA, &omegaA, &LA);

				////////////////////////////////////////////////////////////////////////////////////////////////////
				// for gePhysicsObject B				

				Matrix33_MakeCrossProductMatrix33(&rB, &rStarB);

				Matrix33_ExtractFromXForm3d(&xformB, &rotB);
				Matrix33_GetTranspose(&rotB, &rotBt);

				gePhysicsObject_GetInertiaTensor(podB, &iTensorB);
				gePhysicsObject_GetInertiaTensorInverse(podB, &iTensorInvB);

				Matrix33_Multiply(&rotB, &iTensorB, &tmpMat);
				Matrix33_Multiply(&tmpMat, &rotBt, &itB);

				Matrix33_Multiply(&rotB, &iTensorInvB, &tmpMat);
				Matrix33_Multiply(&tmpMat, &rotBt, &itiB);

				gePhysicsObject_GetAngularVelocity(podB, &angularVelocityB, si);

				Matrix33_MultiplyVec3d(&rotB, &angularVelocityB, &omegaB);
				Matrix33_MultiplyVec3d(&itB, &omegaB, &LB);

				////////////////////////////////////////////////////////////////////////////////////////////////////
				// create M submatrix

				Matrix33_MultiplyScalar(gePhysicsObject_GetOneOverMass(podA) + 
					gePhysicsObject_GetOneOverMass(podB), &gePhysicsSystemIdentityMatrix, &term11);

				Matrix33_Multiply(&rStarA, &itiA, &tmpMat);
				Matrix33_Multiply(&tmpMat, &rStarA, &term12);

				Matrix33_Multiply(&rStarB, &itiB, &tmpMat);
				Matrix33_Multiply(&tmpMat, &rStarB, &term22);

				Matrix33_Add(&term12, &term22, &term21);
	
				Matrix33_Subtract(&term11, &term21, &Mblock);

				////////////////////////////////////////////////////////////////////////////////////////////////////
				// compute deviation

				geVec3d_CrossProduct(&omegaA, &LA, &zeroForceAccelerationA);
				geVec3d_CrossProduct(&omegaA, &rA, &ptVelocityA);
				geVec3d_CrossProduct(&omegaA, &ptVelocityA, &alphaA);
				geVec3d_Add(&zeroForceAccelerationA, &alphaA, &ptAccelerationA);

				geVec3d_CrossProduct(&omegaB, &LB, &zeroForceAccelerationB);
				geVec3d_CrossProduct(&omegaB, &rB, &ptVelocityB);
				geVec3d_CrossProduct(&omegaB, &ptVelocityB, &alphaB);
				geVec3d_Add(&zeroForceAccelerationB, &alphaB, &ptAccelerationB);

				Matrix33_MultiplyVec3d(&itiA, &ptAccelerationA, &tmpVec);
				Matrix33_MultiplyVec3d(&rStarA, &tmpVec, &accA);

				Matrix33_MultiplyVec3d(&itiB, &ptAccelerationB, &tmpVec);
				Matrix33_MultiplyVec3d(&rStarB, &tmpVec, &accB);							

				geVec3d_Subtract(&accA, &accB, &beta);

				gePhysicsObject_GetLinearVelocity(podA, &linearVelocityA, si);
				geVec3d_Add(&linearVelocityA, &ptVelocityA, &velA);
				gePhysicsObject_GetLinearVelocity(podB, &linearVelocityB, si);
				geVec3d_Add(&linearVelocityB, &ptVelocityB, &velB);

				geVec3d_Subtract(&velA, &velB, &D1);

				gePhysicsJoint_GetLocationAInWorldSpace(jntData, &jntLocAWS);
				gePhysicsJoint_GetLocationBInWorldSpace(jntData, &jntLocBWS);

				geVec3d_Subtract(&jntLocAWS, &jntLocBWS, &D0);

				////////////////////////////////////////////////////////////////////////////////////////////////////
				// compute K subvector

				geVec3d_Scale(&D1, 2.f / h, &D1);
				geVec3d_Scale(&D0, 1.f / hSquared, &D0);
				geVec3d_Add(&beta, &D1, &K);
				geVec3d_Add(&D0, &K, &K);

				////////////////////////////////////////////////////////////////////////////////////////////////////
				// fill linear system appropriately

				PS->linsys->KVector[iOffset] = -K.X;
				PS->linsys->KVector[iOffset + 1] = -K.Y;
				PS->linsys->KVector[iOffset + 2] = -K.Z;

				for (j = 0; j < 3; j++)
				{
					for (k = 0; k < 3; k++)
					{
						PS->linsys->M[iOffset + j][iOffset + k] = Mblock.x[j][k];
					}
				}

				iOffset += 3;
				break;

			default:
				assert(!"Illegal joint type");
				break;
		}	// switch		
	} // for

	if (!gePhysicsSystem_SolveForConstraintForces(PS))
	{
		return GE_FALSE;
	}

	for	(i = 0, iOffset = 0; i < PS->PhysicsJointCount; i++)
	{
		jntData = PS->Joints[i];

		////////////////////////////////////////////////////////////////////////////////////////////////////
		// compute joints' actual locations in the world

		type = gePhysicsJoint_GetType(jntData);

		switch(type)
		{
			case JT_WORLD:

				podA = gePhysicsJoint_GetObject1(jntData);
				assert(podA != NULL);

				gePhysicsObject_GetXForm(podA, &xformA, si);
				gePhysicsJoint_GetLocationA(jntData, &jntLocA);
				geXForm3d_Rotate(&xformA, &jntLocA, &rA);

				constraintForce.X = PS->linsys->X[iOffset];
				constraintForce.Y = PS->linsys->X[iOffset + 1];
				constraintForce.Z = PS->linsys->X[iOffset + 2];

				gePhysicsObject_ApplyGlobalFrameForce(podA, &constraintForce, &rA, GE_FALSE, si);

				iOffset += 3;

				break;

			case JT_SPHERICAL:

				podA = gePhysicsJoint_GetObject1(jntData);
				assert(podA != NULL);

				gePhysicsObject_GetXForm(podA, &xformA, si);
				gePhysicsJoint_GetLocationA(jntData, &jntLocA);
				geXForm3d_Rotate(&xformA, &jntLocA, &rA);

				constraintForce.X = PS->linsys->X[iOffset];
				constraintForce.Y = PS->linsys->X[iOffset + 1];
				constraintForce.Z = PS->linsys->X[iOffset + 2];

				gePhysicsObject_ApplyGlobalFrameForce(podA, &constraintForce, &rA, GE_FALSE, si);

				////////////////////////////////////////////////////////////////////////////////////////////////////
				// apply -ve force to gePhysicsObject B

				podB = gePhysicsJoint_GetObject2(jntData);
				assert(podB != NULL);

				gePhysicsObject_GetXForm(podB, &xformB, si);
				gePhysicsJoint_GetLocationB(jntData, &jntLocB);
				geXForm3d_Rotate(&xformB, &jntLocB, &rB);

				constraintForce.X = -constraintForce.X;
				constraintForce.Y = -constraintForce.Y;
				constraintForce.Z = -constraintForce.Z;

				gePhysicsObject_ApplyGlobalFrameForce(podB, &constraintForce, &rB, GE_FALSE, si);

				iOffset += 3;

				break;

			default:
				break;
		} // switch
	} // for

	return GE_TRUE;
}

static int imin(int a, int b)
{
	return a < b ? a : b;
}

static geBoolean gePhysicsSystem_SolveForConstraintForces(gePhysicsSystem* PS)
{
	int i, j, k, n;
	int ixend1, ixend2;
	float num;
	float** M;
	float* b;
	float* x;

	assert(PS != NULL);
	
	b = PS->linsys->KVector;
	x = PS->linsys->X;
	M = PS->linsys->M;

	assert(b != NULL);
	assert(x != NULL);
	assert(M != NULL);

	n = PS->sumOfConstraintDimensions;

	for (i = 0; i < n; i++)
	{
		assert(M[i] != NULL);

		if ((float)fabs(M[i][i]) < (float)(1e-5))
		{
			return GE_FALSE;
		}

		num = 1 / M[i][i];

		for (j = i; j < n; j++)
			M[i][j] *= num;

		b[i] *= num;
		
				

		ixend1 = imin(n, i + 3);

		for (j = i + 1; j < ixend1; j++)
		{
			num = M[j][i];

			ixend2 = imin(n, i + 2);

			for (k = i; k < ixend2; k++)
			{
				M[i][k] -= num * M[i][k];
			}

			b[j] -= num * b[i];
		}				
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	// backsubstitute

	for (i = n - 1; i >= 0; i--)
	{
		x[i] = b[i];

		ixend1 = imin(n, i + 2);

		for (j = i + 1; j < ixend1; j++)
		{
			x[i] -= M[i][j] * x[j];
		}
	}

	return GE_TRUE;
}

GENESISAPI int GENESISCC gePhysicsSystem_GetSourceConfigIndex(const gePhysicsSystem* pSys)
{
	assert(pSys != NULL);

	return pSys->sourceConfigIndex;
}

GENESISAPI gePhysicsObject** GENESISCC gePhysicsSystem_GetPhysicsObjects(const gePhysicsSystem* pSys)
{
	assert(pSys != NULL);

	return pSys->Objects;
}

GENESISAPI gePhysicsJoint** GENESISCC gePhysicsSystem_GetPhysicsJoints(const gePhysicsSystem* pSys)
{
	assert(pSys != NULL);

	return pSys->Joints;
}

GENESISAPI int GENESISCC gePhysicsSystem_GetNumPhysicsObjects(const gePhysicsSystem* pSys)
{
	assert(pSys != NULL);

	return pSys->PhysicsObjectCount;
}

GENESISAPI int GENESISCC gePhysicsSystem_GetNumPhysicsJoints(const gePhysicsSystem* pSys)
{
	assert(pSys != NULL);

	return pSys->PhysicsJointCount;
}

GENESISAPI int GENESISCC gePhysicsSystem_GetSumOfConstraintDimensions(const gePhysicsSystem* pSys)
{
	assert(pSys != NULL);

	return pSys->sumOfConstraintDimensions;
}

