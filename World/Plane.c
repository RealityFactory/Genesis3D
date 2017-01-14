/****************************************************************************************/
/*  Plane.c                                                                             */
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
/*Genesis3D Version 1.1 released November 15, 1999                            */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#include <Assert.h>
#include <Windows.h>
#include <Math.h>

#include "BaseType.h"
#include "System.h"
#include "World.h"
#include "GBSPFile.h"
#include "Vec3d.h"
#include "XForm3d.h"
#include "Plane.h"

//=====================================================================================
//	static local globals
//=====================================================================================
static	geEngine		*CEngine;		
static	geWorld			*CWorld;
static	GBSP_BSPData	*BSPData;		// This is in globals, but is also kept here for speed


//=====================================================================================
//	Plane_SetEngine
//	Lets this module know that the engine has changed
//=====================================================================================
geBoolean GENESISCC Plane_SetEngine(geEngine *Engine)
{
	assert (Engine != NULL);

	CEngine = Engine;

	return GE_TRUE;
}

//=====================================================================================
//	Plane_SetWorld
//	Lets this module know that the world has changed
//=====================================================================================
geBoolean GENESISCC Plane_SetWorld(geWorld *World)
{
	assert(World != NULL);
	
	CWorld = World;

	return GE_TRUE;
}

//=====================================================================================
//	Plane_SetGBSP
//=====================================================================================
geBoolean GENESISCC Plane_SetGBSP(World_BSP *BSP)
{
	assert(BSP != NULL);

	// Make quick pointer to the world bsp data
	BSPData = &BSP->BSPData;

	return GE_TRUE;
}

//=====================================================================================
//	Plane_FindLeaf
//=====================================================================================
int32 GENESISCC Plane_FindLeaf(const geWorld *World, int32 Node, const geVec3d *POV)
{
    float		Dist;
	GFX_Node	*GFXNodes;
	GFX_Plane	*GFXPlanes;
	int32		Leaf;

	GFXNodes = World->CurrentBSP->BSPData.GFXNodes;
	GFXPlanes = World->CurrentBSP->BSPData.GFXPlanes;

    while (Node >= 0) 
	{
		assert(Node >= 0 && Node < World->CurrentBSP->BSPData.NumGFXNodes);
	
		Dist = Plane_PlaneDistanceFast(&GFXPlanes[GFXNodes[Node].PlaneNum], POV);
        
		if (Dist < 0) 
            Node = GFXNodes[Node].Children[1];
		else
            Node = GFXNodes[Node].Children[0];
    }
	
	// We are now in a leaf
	Leaf = -(Node+1);

	assert(Leaf >=0 && Leaf < World->CurrentBSP->BSPData.NumGFXLeafs);
	
	return Leaf;
}

//=====================================================================================
//	Plane_PlaneDistanceFast
//	Fast axial aligned plane distance
//=====================================================================================
float GENESISCC Plane_PlaneDistanceFast(const GFX_Plane *Plane, const geVec3d *Point)
{
	float	Dist, PDist;

	assert(Plane != NULL);
	assert(Point != NULL);

	PDist = Plane->Dist;

	switch (Plane->Type)
	{
		case PLANE_X:
			Dist = (Point->X - PDist);
			break;
		case PLANE_Y:
			Dist = (Point->Y - PDist);
			break;
		case PLANE_Z:
			Dist = (Point->Z - PDist);
			break;
	      
		default:
			Dist = geVec3d_DotProduct(Point, &Plane->Normal) - PDist;
			break;
	}

	return Dist;
}

//=====================================================================================
//	Plane_PlaneDistance
//	Normal (slow) plane distance
//=====================================================================================
float GENESISCC Plane_PlaneDistance(const GFX_Plane *Plane, const geVec3d *Point)
{
	float	Dist;

	assert(Plane != NULL);
	assert(Point != NULL);

	Dist = geVec3d_DotProduct(Point, &Plane->Normal) - Plane->Dist;

	return Dist;
}

//=====================================================================================
//	Plane_PlaneDistanceFast
//	Fast axial aligned face distance
//=====================================================================================
float GENESISCC Plane_FaceDistanceFast(const GFX_Face *Face, const geVec3d *Point)
{
	float		Dist, PDist;
	GFX_Plane	*Plane;

	assert(Face != NULL);
	assert(Point != NULL);

	assert(BSPData != NULL);
	assert(BSPData->GFXPlanes != NULL);
	
	Plane = &BSPData->GFXPlanes[Face->PlaneNum];
	PDist = Plane->Dist;

	switch (Plane->Type)
	{
		case PLANE_X:
			Dist = (Point->X - PDist);
			break;
		case PLANE_Y:
			Dist = (Point->Y - PDist);
			break;
		case PLANE_Z:
			Dist = (Point->Z - PDist);
			break;
	      
		default:
			Dist = geVec3d_DotProduct(Point, &Plane->Normal) - PDist;
			break;
	}

	if (Face->PlaneSide)
		Dist = -Dist;

	return Dist;
}

//====================================================================================
//	gePlane_SetFromVerts
//====================================================================================
void gePlane_SetFromVerts(GFX_Plane *Plane, const geVec3d *V1, const geVec3d *V2, const geVec3d *V3)
{
	geVec3d		Vect1, Vect2;
	
	// Get the 2 vectors to derive the normal
	geVec3d_Subtract(V1, V2, &Vect1);
	geVec3d_Subtract(V3, V2, &Vect2);
	
	// The normal is the cross between these 2 vectors
	geVec3d_CrossProduct(&Vect1, &Vect2, &Plane->Normal);
	geVec3d_Normalize(&Plane->Normal);

	// Get the planes distance from the origin, by projecting a vert on the plane
	// along the plane normal, to the origin...
	Plane->Dist = geVec3d_DotProduct(V1, &Plane->Normal);

	// Finally, get the plane type
	Plane->Type = PLANE_ANY;
}

