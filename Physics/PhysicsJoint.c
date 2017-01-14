/****************************************************************************************/
/*  PHYSICSJOINT.H                                                                      */
/*                                                                                      */
/*  Author: Jason Wood                                                                  */
/*  Description: Rigid body joint implementation                                        */
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

#include "vec3d.h"
#include "xform3d.h"
#include "ram.h"
#include "matrix33.h"
#include "quatern.h"

#include "PhysicsObject.h"
#include "PhysicsJoint.h"

typedef struct gePhysicsJoint
{
	gePhysicsJoint_Kind	Type;
	geVec3d				locationA;
	geVec3d				locationB;
	geVec3d				locationAInWorldSpace;
	geVec3d				locationBInWorldSpace;
	gePhysicsObject *		Object1;
	gePhysicsObject *		Object2;
	float				assemblyRate;
}	gePhysicsJoint;

#define JOINT_ASSEMBLY_RATE_MULTIPLIER (4.f)

GENESISAPI gePhysicsJoint * GENESISCC gePhysicsJoint_Create(gePhysicsJoint_Kind Kind, const geVec3d *Location, 
	float assemblyRate, gePhysicsObject *PS1, gePhysicsObject *PS2, float physicsScale)
{
	gePhysicsJoint*	pPhysjnt;
	geVec3d		POLocation;
	geVec3d		physicsSpaceLocation;

	pPhysjnt = NULL;
	pPhysjnt = GE_RAM_ALLOCATE_STRUCT(gePhysicsJoint);
	if (pPhysjnt == NULL)
	{
		return NULL;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	// make sure the joint makes sense

	pPhysjnt->Type = Kind;
	pPhysjnt->assemblyRate = assemblyRate * JOINT_ASSEMBLY_RATE_MULTIPLIER;
	pPhysjnt->Object1 = PS1;
	pPhysjnt->Object2 = PS2;

	geVec3d_Scale(Location, physicsScale, &physicsSpaceLocation);

	switch (Kind)
	{
		case JT_WORLD:
			if (PS1 == NULL)
			{
				/*
				GenVSI_Error(VSI, 
					GE_FALSE, 
					"Joint_Spawn: World joint needs non-NULL gePhysicsObject1 field.\n");
				*/
				return NULL;
			}
			#if 0
			if (pJoint->Next == pJoint)
			{
				/*
				GenVSI_Error(VSI, 
					GE_FALSE, 
					"Joint_Spawn: Next field points to parent.\n");
				*/
				return NULL;
			}
			#endif
			
			gePhysicsObject_GetLocation(PS1, &POLocation, 0);

			geVec3d_Subtract(&physicsSpaceLocation,
				&POLocation,
				&pPhysjnt->locationA);
			geVec3d_Copy(&physicsSpaceLocation, &pPhysjnt->locationB);
			break;

		case JT_SPHERICAL:
			if (PS1 == NULL || PS2 == NULL)
			{
				/*
				GenVSI_Error(VSI, 
					GE_FALSE, 
					"Joint_Spawn: Spherical joint needs 2 non-NULL gePhysicsObjects.\n");
				*/
				return NULL;
			}
			#if 0
			if (pJoint->Next == pJoint)
			{
				/*
				GenVSI_Error(VSI, 
					GE_FALSE, 
					"Joint_Spawn: Next field points to parent.\n");
				*/
			}
			#endif
			if (PS1 == PS2)
			{
				/*
				GenVSI_Error(VSI, 
					GE_FALSE, 
					"Joint_Spawn: Spherical joint: need 2 distinct gePhysicsObjects.\n");
				*/
				return NULL;
			}

			gePhysicsObject_GetLocation(PS1, &POLocation, 0);
			geVec3d_Subtract(&physicsSpaceLocation,
				&POLocation,
				&pPhysjnt->locationA);

			gePhysicsObject_GetLocation(PS2, &POLocation, 0);
			geVec3d_Subtract(&physicsSpaceLocation,
				&POLocation,
				&pPhysjnt->locationB);
			break;

		default:
			/*
			GenVSI_Error(VSI, 
				GE_FALSE, 
				"Joint_Spawn: unsupported joint type %d.\n", ij->jointType);
			*/
			return NULL;
	}

	return pPhysjnt;
}

GENESISAPI geBoolean GENESISCC gePhysicsJoint_Destroy(gePhysicsJoint** ppPhysjnt)
{
	assert(ppPhysjnt != NULL);
	assert(*ppPhysjnt != NULL);

	geRam_Free(*ppPhysjnt);

	*ppPhysjnt = NULL;

	return GE_TRUE;
}

GENESISAPI gePhysicsJoint_Kind GENESISCC gePhysicsJoint_GetType(const gePhysicsJoint* pPhysjnt)
{
	assert(pPhysjnt != NULL);

	return pPhysjnt->Type;
}

GENESISAPI void GENESISCC gePhysicsJoint_GetLocationA(const gePhysicsJoint* pPhysjnt, geVec3d* pLoc)
{
	assert(pPhysjnt != NULL);
	assert(pLoc != NULL);
	
	geVec3d_Copy(&pPhysjnt->locationA, pLoc);
}

GENESISAPI void GENESISCC gePhysicsJoint_GetLocationB(const gePhysicsJoint* pPhysjnt, geVec3d* pLoc)
{
	assert(pPhysjnt != NULL);
	assert(pLoc != NULL);
	
	geVec3d_Copy(&pPhysjnt->locationB, pLoc);
}

GENESISAPI void GENESISCC gePhysicsJoint_SetLocationA(gePhysicsJoint* pPhysjnt, const geVec3d* pLoc)
{
	assert(pPhysjnt != NULL);
	assert(pLoc != NULL);
	
	geVec3d_Copy(pLoc, &pPhysjnt->locationA);
}

GENESISAPI void GENESISCC gePhysicsJoint_SetLocationB(gePhysicsJoint* pPhysjnt, const geVec3d* pLoc)
{
	assert(pPhysjnt != NULL);
	assert(pLoc != NULL);
	
	geVec3d_Copy(pLoc, &pPhysjnt->locationB);
}

GENESISAPI void GENESISCC gePhysicsJoint_GetLocationAInWorldSpace(const gePhysicsJoint* pPhysjnt, geVec3d* pLoc)
{
	assert(pPhysjnt != NULL);
	assert(pLoc != NULL);
	
	geVec3d_Copy(&pPhysjnt->locationAInWorldSpace, pLoc);
}

GENESISAPI void GENESISCC gePhysicsJoint_GetLocationBInWorldSpace(const gePhysicsJoint* pPhysjnt, geVec3d* pLoc)
{
	assert(pPhysjnt != NULL);
	assert(pLoc != NULL);
	
	geVec3d_Copy(&pPhysjnt->locationBInWorldSpace, pLoc);
}

GENESISAPI void GENESISCC gePhysicsJoint_SetLocationAInWorldSpace(gePhysicsJoint* pPhysjnt, const geVec3d* pLoc)
{
	assert(pPhysjnt != NULL);
	assert(pLoc != NULL);
	
	geVec3d_Copy(pLoc, &pPhysjnt->locationAInWorldSpace);
}

GENESISAPI void GENESISCC gePhysicsJoint_SetLocationBInWorldSpace(gePhysicsJoint* pPhysjnt, const geVec3d* pLoc)
{
	assert(pPhysjnt != NULL);
	assert(pLoc != NULL);
	
	geVec3d_Copy(pLoc, &pPhysjnt->locationBInWorldSpace);
}

GENESISAPI gePhysicsObject* GENESISCC gePhysicsJoint_GetObject1(const gePhysicsJoint* pPhysjnt)
{
	assert(pPhysjnt != NULL);

	return pPhysjnt->Object1;
}

GENESISAPI gePhysicsObject* GENESISCC gePhysicsJoint_GetObject2(const gePhysicsJoint* pPhysjnt)
{
	assert(pPhysjnt != NULL);

	return pPhysjnt->Object2;
}

GENESISAPI float GENESISCC gePhysicsJoint_GetAssemblyRate(const gePhysicsJoint* pPhysjnt)
{
	assert(pPhysjnt != NULL);

	return pPhysjnt->assemblyRate;
}

GENESISAPI void GENESISCC gePhysicsJoint_SetAssemblyRate(gePhysicsJoint* pPhysjnt, float assemblyRate)
{
	assert(pPhysjnt != NULL);
	assert(assemblyRate >= (float)(1e-5));

	pPhysjnt->assemblyRate = assemblyRate;
}

