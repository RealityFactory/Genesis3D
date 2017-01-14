/****************************************************************************************/
/*  Trace.c                                                                             */
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
/*Genesis3D Version 1.1 released November 15, 1999                            */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#include <Assert.h>

#include "XForm3d.h"
#include "BaseType.h"
#include "GBSPFile.h"
#include "World.h"
#include "System.h"
#include "Plane.h"
#include "Trace.h"
#include "ExtBox.h"
#include "Actor.h"

#define ON_EPSILON	(0.1f)

//=====================================================================================
//	Local Static Globals
//=====================================================================================

// Globals returned in the bsp subdivision code
static	int32			GPlaneNum;
static	GFX_Plane		GlobalPlane;
static	int32			GlobalNode;
static	int32			GlobalSide;
static	geVec3d			GlobalI;
static	float			GRatio;
static	int32			GlobalLeaf;
static	int32			GlobalLeaf;

static	GBSP_BSPData	*BSPData;

// Globals passed to the bsp subdivision code
static	uint32			gContents;			// Contents that we should collide with for the current collision tests...

static	BOOL			UseMinsMaxs;		// If MiscCollision should use mins maxs...

static	BOOL			HitSet;
static  int32			GlobalNNode[2]={0x696C6345,0x21657370};

int32	NumExactCast;
int32	NumBBoxCast;
int32	NumGetContents;

//=====================================================================================
//	Local Static Function Prototypes
//=====================================================================================
static geBoolean BSPIntersect(geVec3d *Front, geVec3d *Back, int32 Node);



static geActor *Trace_ActorCollide(geWorld *World,
								   const geVec3d *Mins, const geVec3d *Maxs,
								   const geVec3d *Front,const geVec3d *Back, 
								   geVec3d *CollisionPoint,GFX_Plane *BestPlane,
								   uint32 UserFlags, GE_CollisionCB *CollisionCB, void *Context, geFloat *BestD  )
{
	int i,Count;
	World_Actor *WA;
	geVec3d RayDirection;
	geFloat RayLength;
	geFloat Dist;
	geActor *BestActor = NULL;
	geVec3d	FakeMins = {-1.0f, -1.0f, -1.0f};
	geVec3d	FakeMaxs = { 1.0f,  1.0f,  1.0f};
	int32	k;
	geVec3d	OMins, OMaxs;
	
	if (!Mins)
		Mins = &FakeMins;
	
	if (!Maxs)
		Maxs = &FakeMaxs;

	geVec3d_Subtract(Back,Front,&RayDirection);
	RayLength = geVec3d_Normalize(&RayDirection);

	Count = World->ActorCount;
	WA = &(World->ActorArray[0]);

	Trace_GetMoveBox(Mins, Maxs, Front, Back, &OMins, &OMaxs);

	for (i=0; i<Count; i++, WA++)
	{
		geExtBox B;
		geVec3d Normal;
		
		// Reject if not active or if userflags don't accept...
		if (!(WA->Flags & GE_ACTOR_COLLIDE) || !(WA->UserFlags & UserFlags) )
			continue;

		if (CollisionCB && !CollisionCB(NULL, WA->Actor, Context))
			continue;

		if (!geActor_GetExtBox(WA->Actor, &B))
			continue;
							
		for (k=0; k<3; k++)
		{
			if (geVec3d_GetElement(&OMaxs, k) < geVec3d_GetElement(&B.Min, k))
				break;

			if (geVec3d_GetElement(&OMins, k) > geVec3d_GetElement(&B.Max, k))
				break;
		}
		
		if (k != 3)
			continue;
		
		geVec3d_Subtract(&B.Min, Maxs, &B.Min);
		geVec3d_Subtract(&B.Max, Mins, &B.Max);
							
		if (!geExtBox_RayCollision( &B, Front, Back, &Dist, &Normal ))
			continue;

		//Dist -= 0.01f;
		if (Dist < 0.0f)
			Dist = 0.0f;
		if (Dist > 1.0f)
			Dist = 1.0f;

		Dist *= RayLength;

		if (Dist < *BestD)
		{
			BestActor = WA->Actor;
			*BestD = Dist;
			BestPlane->Normal = Normal;
											
			geVec3d_AddScaled(Front,&RayDirection,Dist,CollisionPoint);
			BestPlane->Dist = geVec3d_DotProduct(CollisionPoint,&Normal);
											
			BestPlane->Type = PLANE_ANY;
		}
	}

	return BestActor;
}



//=====================================================================================
//	Trace_GEWorldCollision
//	Function specially designed for GE UI...
//=====================================================================================
geBoolean Trace_GEWorldCollision(geWorld *World, const geVec3d *Mins, const geVec3d *Maxs, const geVec3d *Front, const geVec3d *Back, uint32 Contents, uint32 CollideFlags, uint32 UserFlags, GE_CollisionCB *CollisionCB, void *Context, GE_Collision *Col)
{
	geVec3d		I;
	GFX_Plane	Plane;
	geWorld_Model	*Model;
	Mesh_RenderQ	*Mesh;
	geActor     *Actor;

	assert(World != NULL);
	assert(World->CurrentBSP != NULL);
	assert(Front != NULL);
	assert(Back!= NULL);
	assert(Contents);			// It does not make sense to collide with nothing!!!
	
	// Set the global contents to collide with
	gContents = Contents;

	BSPData = &World->CurrentBSP->BSPData;

	// Reset all the collision feedback pointers
	Model = NULL;
	Mesh = NULL;

	Col->Model = NULL;
	Col->Mesh = NULL;

	if (Mins && Maxs)
	{
		NumBBoxCast++;
		if (Trace_WorldCollisionBBox(World, Mins, Maxs, Front, Back, CollideFlags, &I, &Plane, &Model, &Mesh, &Actor, UserFlags, CollisionCB, Context))
		{
			
			Col->Impact = I;
			Col->Plane.Normal =	Plane.Normal;
			Col->Plane.Dist = Plane.Dist;
			
			Col->Model = Model;
			Col->Mesh = (geMesh*)Mesh;
			Col->Actor = Actor;

			Col->Ratio = GRatio;
			return GE_TRUE;
		}
	}
	else 
	{
		NumExactCast++;

		if (Trace_WorldCollisionExact(World, Front, Back, CollideFlags, &I, &Plane, &Model, &Mesh, &Actor, UserFlags, CollisionCB, Context))
		{
			Col->Impact = I;
			Col->Plane.Normal = Plane.Normal;
			Col->Plane.Dist = Plane.Dist;

			Col->Model = Model;
			Col->Mesh = (geMesh*)Mesh;
			Col->Actor = Actor;

			Col->Ratio = GRatio;

			return GE_TRUE;
		}
	}
	return GE_FALSE;
}

//=====================================================================================
//	Trace_WorldCollisionExact
//=====================================================================================
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
									void *Context)
{
	int32			i, b;
	geVec3d			NewFront1, NewBack1;
	geVec3d			NewFront2, NewBack2;
	geWorld_Model	*Models, *BestModel;
	GFX_Plane		BestPlane, Plane2;
	Mesh_RenderQ	*BestMesh;
	geActor			*BestActor;
	geVec3d			OMins, OMaxs, Vect, Impact2, BestI;
	static geVec3d	MMins = {-1.0f, -1.0f, -1.0f};
	static geVec3d	MMaxs = { 1.0f,  1.0f,  1.0f};
	geBoolean		Hit;
	float			Dist, BestD;

	assert(World != NULL);
	assert(World->CurrentBSP != NULL);
	assert(Front != NULL);
	assert(Back!= NULL);
	
	BSPData = &World->CurrentBSP->BSPData;
	Models = World->CurrentBSP->Models;
	
	// Clear mesh/model collision pointers
	if (Model)
		*Model = NULL;
	if (Mesh)
		*Mesh = NULL;
	if (Actor)
		*Actor = NULL;

	BestD = 99999.0f;
	BestModel = NULL;
	BestMesh = NULL;
	Hit = GE_FALSE;

	if (Flags & GE_COLLIDE_ACTORS)
	{
		BestActor = Trace_ActorCollide(World,NULL, NULL, Front,Back,&Impact2, &Plane2, UserFlags, CollisionCB, Context, &BestD);
		if (BestActor != NULL)
		{
			BestI = Impact2;
			BestPlane = Plane2;
			Hit = GE_TRUE;
		}
	}

	if (!(Flags & GE_COLLIDE_MODELS))
		goto NoModels;

	Trace_GetMoveBox(&MMins, &MMaxs, Front, Back, &OMins, &OMaxs);

	for (i = 0; i < BSPData->NumGFXModels; i++, Models++)
	{
		// First, give the caller a chance to reject the model
		if (CollisionCB && !CollisionCB(Models, NULL, Context))
			continue;

		for (b=0; b<3; b++)
		{
			if (VectorToSUB(OMaxs, b) < VectorToSUB(Models->TMins, b))
				break;
			if (VectorToSUB(OMins, b) > VectorToSUB(Models->TMaxs, b))
				break;
		}

		if (b != 3)
			continue;

		// Move to models center of rotation
		geVec3d_Subtract(Front, &Models->Pivot, &NewFront1);
		geVec3d_Subtract(Back , &Models->Pivot, &NewBack1);

		// InverseTransform the point about models center of rotation
		geXForm3d_TransposeTransform(&Models->XForm, &NewFront1, &NewFront2);
		geXForm3d_TransposeTransform(&Models->XForm, &NewBack1 , &NewBack2);

		// push back into world
		geVec3d_Add(&NewFront2, &Models->Pivot, &NewFront1);
		geVec3d_Add(&NewBack2 , &Models->Pivot, &NewBack1);
		
		HitSet = FALSE;

		if (BSPIntersect(&NewFront1, &NewBack1, BSPData->GFXModels[i].RootNode[0]))
		{
			// Rotate the impact plane
			geXForm3d_Rotate(&Models->XForm, &GlobalPlane.Normal, &GlobalPlane.Normal);
			
			// Rotate the impact point
			geVec3d_Subtract(&GlobalI, &Models->Pivot, &GlobalI);
			geXForm3d_Transform(&Models->XForm, &GlobalI, &GlobalI);
			geVec3d_Add(&GlobalI, &Models->Pivot, &GlobalI);
			
			// Find the new plane distance based on the new impact point with the new plane
			GlobalPlane.Dist = geVec3d_DotProduct(&GlobalPlane.Normal, &GlobalI);

			geVec3d_Subtract(&GlobalI, Front, &Vect);
			Dist = geVec3d_Length(&Vect);

			if (Dist < BestD)
			{
				BestD = Dist;
				BestI = GlobalI;
			
				BestPlane = GlobalPlane;
				if (GlobalSide)
				{
					geVec3d_Inverse(&BestPlane.Normal);
					BestPlane.Dist = -BestPlane.Dist;
					BestPlane.Type = PLANE_ANY;
				}

				BestMesh = NULL;		// Clear hit mesh
				BestActor = NULL;
				BestModel = Models;
				Hit = GE_TRUE;
			}
		}

		if ((Flags & GE_COLLIDE_NO_SUB_MODELS))
			goto NoModels;
	}

	NoModels:

	if (Hit)
	{
		if (Impact)
			*Impact = BestI;
		if (Plane)
			*Plane = BestPlane;
		if (Model)
			*Model = BestModel;
		if (Mesh)
			*Mesh = BestMesh;
		if (Actor)
			*Actor = BestActor;
		return GE_TRUE;
	}

	return GE_FALSE;
}

//=====================================================================================
//	Trace_WorldCollisionExact2
//	Internal only/ does not chek meshes/ returns index numbers into bsp structures for models
//	FIXME:  This can be replaced by callinf Trace_WorldCollisionExact with the GE_COLLIDE_MODELS
//	flag only...
//=====================================================================================
geBoolean Trace_WorldCollisionExact2(	geWorld *World, 
										const geVec3d *Front, 
										const geVec3d *Back, 
										geVec3d *Impact,
										int32 *Node,
										int32 *Plane,
										int32 *Side)
{
	int32			i;
	geVec3d			NewFront1, NewBack1;
	geVec3d			NewFront2, NewBack2;
	geWorld_Model		*Models;

	assert(World != NULL);
	assert(World->CurrentBSP != NULL);
	assert(Front != NULL);
	assert(Back!= NULL);
	
	BSPData = &World->CurrentBSP->BSPData;
	Models = World->CurrentBSP->Models;
	
	GPlaneNum = -1;

	for (i = 0; i < BSPData->NumGFXModels; i++)
	{
		
		// Move to models center of rotation
		geVec3d_Subtract(Front, &Models[i].Pivot, &NewFront1);
		geVec3d_Subtract(Back , &Models[i].Pivot, &NewBack1);

		// InverseTransform the point about models center of rotation
		geXForm3d_TransposeTransform(&Models[i].XForm, &NewFront1, &NewFront2);
		geXForm3d_TransposeTransform(&Models[i].XForm, &NewBack1 , &NewBack2);

		// push back into world
		geVec3d_Add(&NewFront2, &Models[i].Pivot, &NewFront1);
		geVec3d_Add(&NewBack2 , &Models[i].Pivot, &NewBack1);
		
		HitSet = FALSE;
		
		if (BSPIntersect(&NewFront1, &NewBack1, BSPData->GFXModels[i].RootNode[0]))
		{
			if (GPlaneNum == -1)
				return FALSE;

			if (Impact) *Impact = GlobalI;
			if (Node) *Node = GlobalNode;
			if (Plane) *Plane = GPlaneNum;
			if (Side) *Side = GlobalSide;

			return GE_TRUE;
		}
	}

	return GE_FALSE;
}

//=====================================================================================
//	Local static support functions
//=====================================================================================

//=====================================================================================
//	BSPIntersect
//	Shoot a ray through the tree finding out what solid leafs it passed through
//=====================================================================================
static geBoolean BSPIntersect(geVec3d *Front, geVec3d *Back, int32 Node)
{
    float		Fd, Bd, Dist;
    int32		Side;
    geVec3d		I;
	GFX_Plane	*Plane;
	int32		Contents;

	if (Node < 0)
	{
		Contents = BSPData->GFXLeafs[-(Node+1)].Contents;

		if (Contents & gContents)
		    return GE_TRUE;						// Ray collided with solid space

		return GE_FALSE;
	}

	Plane = &BSPData->GFXPlanes[BSPData->GFXNodes[Node].PlaneNum];

    Fd = Plane_PlaneDistanceFast(Plane, Front);
    Bd = Plane_PlaneDistanceFast(Plane, Back);

    if (Fd >= 0 && Bd >= 0) 
        return(BSPIntersect(Front, Back, BSPData->GFXNodes[Node].Children[0]));
    if (Fd < 0 && Bd < 0)
        return(BSPIntersect(Front, Back, BSPData->GFXNodes[Node].Children[1]));

    Side = Fd < 0;
    Dist = Fd / (Fd - Bd);

    I.X = Front->X + Dist * (Back->X - Front->X);
    I.Y = Front->Y + Dist * (Back->Y - Front->Y);
    I.Z = Front->Z + Dist * (Back->Z - Front->Z);

    // Work our way to the front, from the back side.  As soon as there
	// is no more collisions, we can assume that we have the front portion of the
	// ray that is in empty space.  Once we find this, and see that the back half is in
	// solid space, then we found the front intersection point...
	if (BSPIntersect(Front, &I, BSPData->GFXNodes[Node].Children[Side]))
        return GE_TRUE;
    else if (BSPIntersect(&I, Back, BSPData->GFXNodes[Node].Children[!Side]))
	{
		if (!HitSet)
		{
			GPlaneNum = BSPData->GFXNodes[Node].PlaneNum;
			GlobalPlane = BSPData->GFXPlanes[GPlaneNum];
			GlobalSide = Side;
			GlobalI = I;
			GlobalNode = Node;
			GRatio = Dist;
			HitSet = TRUE;
		}
		return GE_TRUE;
	}

	return GE_FALSE;
}

//=====================================================================================
//	BSPIntersectMesh
//=====================================================================================
static GFX_BNode		*MiscBNodes;
static GFX_Node			*MiscNodes;
static GFX_Plane		*MiscPlanes;
static GFX_Leaf			*MiscLeafs;
static GFX_LeafSide		*MiscSides;

static geVec3d			GMins1, GMaxs1;
static geVec3d			GMins2, GMaxs2;
static geVec3d			GFront, GBack;
static BOOL				LeafHit;
static float			BestDist;

static geBoolean BSPIntersectMisc(geVec3d *Front, geVec3d *Back, int32 Node)
{
    float		Fd, Bd, Dist;
    uint8		Side;
	GFX_Plane	Plane;
    geVec3d		I;

	if (Node == BSP_CONTENTS_SOLID)
        return GE_TRUE;					// Ray collided with solid space
    if (Node < 0)						
        return GE_FALSE;				// Ray collided with empty space

	Plane = MiscPlanes[MiscBNodes[Node].PlaneNum];
	Plane.Type = PLANE_ANY;
	
	if (UseMinsMaxs)
	{
		if (Plane.Normal.X > 0)
			Plane.Dist -= Plane.Normal.X * GMins1.X;
		else	 
			Plane.Dist -= Plane.Normal.X * GMaxs1.X;
	
		if (Plane.Normal.Y > 0)
			Plane.Dist -= Plane.Normal.Y * GMins1.Y;
		else
			Plane.Dist -= Plane.Normal.Y * GMaxs1.Y;

		if (Plane.Normal.Z > 0)
			Plane.Dist -= Plane.Normal.Z * GMins1.Z;
		else							 
			Plane.Dist -= Plane.Normal.Z * GMaxs1.Z;
	}

    Fd = Plane_PlaneDistanceFast(&Plane, Front);
    Bd = Plane_PlaneDistanceFast(&Plane, Back);

    if (Fd >= 0 && Bd >= 0) 
        return(BSPIntersectMisc(Front, Back, MiscBNodes[Node].Children[0]));
    if (Fd < 0 && Bd < 0)
        return(BSPIntersectMisc(Front, Back, MiscBNodes[Node].Children[1]));

    Side = Fd < 0;
    Dist = Fd / (Fd - Bd);

    I.X = Front->X + Dist * (Back->X - Front->X);
    I.Y = Front->Y + Dist * (Back->Y - Front->Y);
    I.Z = Front->Z + Dist * (Back->Z - Front->Z);

    // Work our way to the front, from the back side.  As soon as there
	// is no more collisions, we can assume that we have the front portion of the
	// ray that is in empty space.  Once we find this, and see that the back half is in
	// solid space, then we found the front intersection point...
	if (BSPIntersectMisc(Front, &I, MiscBNodes[Node].Children[Side]))
        return TRUE;
    else if (BSPIntersectMisc(&I, Back, MiscBNodes[Node].Children[!Side]))
	{
		if (!HitSet)
		{
			GPlaneNum = MiscBNodes[Node].PlaneNum;
			GlobalPlane = Plane;
			GlobalSide = Side;
			GlobalI = I;
			GRatio = Dist;
			HitSet = TRUE;
		}
		return GE_TRUE;
	}

	return GE_FALSE;
}

//=====================================================================================
//	Trace_MiscCollision
//	Does a collision with a given tree, ray and transform
//	(The tree will be rotated by the transform, well actually the
//	 ray will be inverse rotated)
//=====================================================================================
geBoolean Trace_MiscCollision(GFX_BNode *BNodes, GFX_Plane *Planes, const geVec3d *Mins, const geVec3d *Maxs, const geVec3d *Front, const geVec3d *Back, geXForm3d *XForm, geVec3d *I, GFX_Plane *P)
{
	geVec3d	NewFront, NewBack;
	geVec3d	Trans;

	GPlaneNum	=	-1;		//Safeguard to prevent a bad index 

	// Set Global misc vars
	MiscBNodes = BNodes;
	MiscPlanes = Planes;

	if (Mins && Maxs)
	{
		GMins1 = *Mins;
		GMaxs1 = *Maxs;
		UseMinsMaxs = TRUE;
	}
	else
		UseMinsMaxs = FALSE;
	
	// Move ray into tree space
	Trans.X = XForm->Translation.X;
	Trans.Y = XForm->Translation.Y;
	Trans.Z = XForm->Translation.Z;

	geVec3d_Subtract(Front, &Trans, &NewFront);
	geVec3d_Subtract(Back, &Trans, &NewBack);
	
	HitSet = FALSE;
	
	if (BSPIntersectMisc(&NewFront, &NewBack, 0))
	{
		if (!HitSet)					// Was in solid, but did not cross any planes...
			return GE_FALSE;			// So just return false...

		geVec3d_Add(&GlobalI, &Trans, &GlobalI);

		if (I) *I = GlobalI;			// Set the intersection point
		if (P)
		{ 
			*P = GlobalPlane;
			// Adjust so plane is at impact point
			P->Dist += geVec3d_DotProduct(&Trans, &P->Normal);
		}

		return GE_TRUE;
	}

	return GE_FALSE;
}

//=====================================================================================
//	BSPIntersectMisc2
//=====================================================================================
static geBoolean BSPIntersectMisc2(const geVec3d *Front, const geVec3d *Back, int32 Node)
{
    float		Fd, Bd, Dist;
    uint8		Side;
	GFX_Plane	Plane;
    geVec3d		I;

	if (Node == BSP_CONTENTS_SOLID)
        return GE_TRUE;					// Ray collided with solid space
    if (Node < 0)						
        return GE_FALSE;				// Ray collided with empty space

	Plane = MiscPlanes[MiscBNodes[Node].PlaneNum];
	Plane.Type = PLANE_ANY;
	
    Fd = Plane_PlaneDistanceFast(&Plane, Front);
    Bd = Plane_PlaneDistanceFast(&Plane, Back);

    if (Fd >= 0 && Bd >= 0) 
        return(BSPIntersectMisc2(Front, Back, MiscBNodes[Node].Children[0]));
    if (Fd < 0 && Bd < 0)
        return(BSPIntersectMisc2(Front, Back, MiscBNodes[Node].Children[1]));

    Side = Fd < 0;
    Dist = Fd / (Fd - Bd);

    I.X = Front->X + Dist * (Back->X - Front->X);
    I.Y = Front->Y + Dist * (Back->Y - Front->Y);
    I.Z = Front->Z + Dist * (Back->Z - Front->Z);

    // Work our way to the front, from the back side.  As soon as there
	// is no more collisions, we can assume that we have the front portion of the
	// ray that is in empty space.  Once we find this, and see that the back half is in
	// solid space, then we found the front intersection point...
	if (BSPIntersectMisc2(Front, &I, MiscBNodes[Node].Children[Side]))
        return TRUE;
    else if (BSPIntersectMisc2(&I, Back, MiscBNodes[Node].Children[!Side]))
	{
		if (!HitSet)
		{
			GPlaneNum = MiscBNodes[Node].PlaneNum;
			GlobalPlane = Plane;
			GlobalSide = Side;
			GlobalI = I;
			GRatio = Dist;
			HitSet = TRUE;
		}
		return GE_TRUE;
	}

	return GE_FALSE;
}

//=====================================================================================
//	Trace_MiscCollision2
//=====================================================================================
geBoolean Trace_MiscCollision2(GFX_BNode *BNodes, GFX_Plane *Planes, const geVec3d *Front, const geVec3d *Back, geVec3d *I, int32 *P)
{
	GPlaneNum	=	-1;		//Safeguard to prevent a bad index 

	// Set Global misc vars
	MiscBNodes = BNodes;
	MiscPlanes = Planes;

	HitSet = FALSE;
	
	if (BSPIntersectMisc2(Front, Back, 0))
	{
		if (!HitSet || GPlaneNum == -1)	// Was in solid, but did not cross any planes...
			return GE_FALSE;			// So just return false...

		if (I) 
			*I = GlobalI;				// Set the intersection point
		if (P)
			*P = GPlaneNum;

		return GE_TRUE;
	}

	return GE_FALSE;
}

//=====================================================================================
//	Trace_BoxOnPlaneSide
//	
//	Returns PSIDE_FRONT, PSIDE_BACK, or PSIDE_BOTH
//=====================================================================================
int32 Trace_BoxOnPlaneSide(const geVec3d *Mins, const geVec3d *Maxs, GFX_Plane *Plane)
{
	int32	Side;
	int32	i;
	geVec3d	Corners[2];
	float	Dist1, Dist2;

	// Axial planes are easy
	if (Plane->Type < PLANE_ANYX)
	{
		Side = 0;
		if (VectorToSUB(*Maxs, Plane->Type) >= Plane->Dist)
			Side |= PSIDE_FRONT;
		if (VectorToSUB(*Mins, Plane->Type) < Plane->Dist)
			Side |= PSIDE_BACK;
		return Side;
	}

	// Create the proper leading and trailing verts for the box
	for (i=0 ; i<3 ; i++)
	{
		if (VectorToSUB(Plane->Normal, i) < 0)
		{
			VectorToSUB(Corners[0], i) = VectorToSUB(*Mins, i);
			VectorToSUB(Corners[1], i) = VectorToSUB(*Maxs, i);
		}
		else
		{
			VectorToSUB(Corners[1], i) = VectorToSUB(*Mins, i);
			VectorToSUB(Corners[0], i) = VectorToSUB(*Maxs, i);
		}
	}

	Dist1 = geVec3d_DotProduct(&Plane->Normal, &Corners[0]) - Plane->Dist;
	Dist2 = geVec3d_DotProduct(&Plane->Normal, &Corners[1]) - Plane->Dist;
	
	Side = 0;
	if (Dist1 >= 0)
		Side = PSIDE_FRONT;
	if (Dist2 < 0)
		Side |= PSIDE_BACK;

	return Side;
}

//=====================================================================================
//	Trace_ExpandPlaneForBox
//	Pushes a plan out by the side of the box it is looking at
//=====================================================================================
void Trace_ExpandPlaneForBox(GFX_Plane *Plane, geVec3d *Mins, geVec3d *Maxs)
{
	geVec3d		*Normal;

	Normal = &Plane->Normal;
	
	if (Normal->X > 0)
		Plane->Dist -= Normal->X * Mins->X;
	else	 
		Plane->Dist -= Normal->X * Maxs->X;
	
	if (Normal->Y > 0)
		Plane->Dist -= Normal->Y * Mins->Y;
	else
		Plane->Dist -= Normal->Y * Maxs->Y;

	if (Normal->Z > 0)
		Plane->Dist -= Normal->Z * Mins->Z;
	else							 
		Plane->Dist -= Normal->Z * Maxs->Z;
}

//=====================================================================================
//	IntersectLeafSides
//=====================================================================================
static geBoolean PointInLeafSides(const geVec3d *Pos, const GFX_Leaf *Leaf)
{
	int32		i, f;
	GFX_Plane	Plane;
	float		Dist;

	f = Leaf->FirstSide;

	for (i=0; i< Leaf->NumSides; i++)
	{
		Plane = MiscPlanes[MiscSides[i+f].PlaneNum];
		Plane.Type = PLANE_ANY;
	
		if (MiscSides[i+f].PlaneSide)
		{
			geVec3d_Inverse(&Plane.Normal);
			Plane.Dist = -Plane.Dist;
		}

		// Simulate the point having a box, by pushing the plane out by the box size
		Trace_ExpandPlaneForBox(&Plane, &GMins1, &GMaxs1);

		Dist = Plane_PlaneDistanceFast(&Plane, Pos);
		
		if (Dist >= 0.0f)
			return GE_FALSE;		// Since leafs are convex, it must be outside...
	}

	return GE_TRUE;
}

//=====================================================================================
//	IntersectLeafSides
//=====================================================================================
BOOL IntersectLeafSides_r(geVec3d *Front, geVec3d *Back, int32 Leaf, int32 Side, int32 PSide)
{
	float		Fd, Bd, Dist;
	GFX_Plane	Plane;
	int32		RSide, Side2;
	geVec3d		I, Vec;

	if (!PSide)
		return FALSE;

	if (Side >= MiscLeafs[Leaf].NumSides)
		return TRUE;		// if it lands behind all planes, it is inside

	RSide = MiscLeafs[Leaf].FirstSide + Side;

	Plane = MiscPlanes[MiscSides[RSide].PlaneNum];
	Plane.Type = PLANE_ANY;
	
	if (MiscSides[RSide].PlaneSide)
	{
		geVec3d_Inverse(&Plane.Normal);
		Plane.Dist = -Plane.Dist;
	}
	
	// Simulate the point having a box, by pushing the plane out by the box size
	Trace_ExpandPlaneForBox(&Plane, &GMins1, &GMaxs1);

	Fd = Plane_PlaneDistanceFast(&Plane, Front);
	Bd = Plane_PlaneDistanceFast(&Plane, Back);

#if 1
	if (Fd >= 0 && Bd >= 0)	// Leaf sides are convex hulls, so front side is totally outside
		return IntersectLeafSides_r(Front, Back, Leaf, Side+1, 0);

	if (Fd < 0 && Bd < 0)
		return IntersectLeafSides_r(Front, Back, Leaf, Side+1, 1);
#else
	if ((Fd >= ON_EPSILON && Bd >= ON_EPSILON) || (Bd > Fd && Fd >= 0) )
		return IntersectLeafSides_r(Front, Back, Leaf, Side+1, 0);

	if ((Fd < -ON_EPSILON && Bd < -ON_EPSILON) || (Bd < Fd && Fd <= 0))
		return IntersectLeafSides_r(Front, Back, Leaf, Side+1, 1);
#endif

	// We have an intersection

	//Dist = Fd / (Fd - Bd);

    Side2 = Fd < 0;
	
	if (Fd < 0)
		Dist = (Fd + ON_EPSILON)/(Fd-Bd);
	else
		Dist = (Fd - ON_EPSILON)/(Fd-Bd);

	if (Dist < 0.0f)
		Dist = 0.0f;
	
	if (Dist > 1.0f)
		Dist = 1.0f;

    I.X = Front->X + Dist * (Back->X - Front->X);
    I.Y = Front->Y + Dist * (Back->Y - Front->Y);
    I.Z = Front->Z + Dist * (Back->Z - Front->Z);

	// Only go down the back side, since the front side is empty in a convex tree
	if (IntersectLeafSides_r(Front, &I, Leaf, Side+1, Side2))
	{
		LeafHit = TRUE;
		return TRUE;
	}
	else if (IntersectLeafSides_r(&I, Back, Leaf, Side+1, !Side2))
	{
		geVec3d_Subtract(&I, &GFront, &Vec);
		Dist = geVec3d_Length(&Vec);

		// Record the intersection closest to the start of ray
		if (Dist < BestDist && !HitSet)
		{
			GlobalI = I;
			GlobalLeaf = Leaf;
			BestDist = Dist;
			GlobalPlane = Plane;
			GRatio = Dist;
			HitSet = TRUE;
		}
		LeafHit = TRUE;
		return TRUE;
	}
	
	return FALSE;	
}

//=====================================================================================
//	IntersectLeafSides2
//=====================================================================================
BOOL IntersectLeafSides2(geVec3d *Pos, int32 Leaf)
{
	GFX_Plane	Plane;
	int32		i;
	float		Dist;

	for (i=0; i< MiscLeafs[Leaf].NumSides; i++)
	{
		Plane = MiscPlanes[MiscSides[MiscLeafs[Leaf].FirstSide+i].PlaneNum];
		
		if (!MiscSides[MiscLeafs[Leaf].FirstSide+i].PlaneSide)
		{
			geVec3d_Inverse(&Plane.Normal);
			Plane.Dist = -Plane.Dist;
		}

		Dist = geVec3d_DotProduct(&Plane.Normal, Pos) - Plane.Dist;

		if (Dist >= 25.0f)
			return FALSE;
	}

	LeafHit = TRUE;
	return TRUE;
}

//=====================================================================================
//	FindClosestLeafIntersection_r
//=====================================================================================
static	void FindClosestLeafIntersection_r(int32 Node)
{
	int32		Leaf, Side, Contents;

	if (Node < 0)
	{
		Leaf = -(Node+1);
		Contents = MiscLeafs[Leaf].Contents;

		//if (Contents != BSP_CONTENTS_SOLID && Contents != BSP_CONTENTS_WINDOW)
		if (!(Contents & gContents))
			return;		// Only solid leafs contain side info...

		HitSet = FALSE;
		
		if (!MiscLeafs[Leaf].NumSides)
			return;

		IntersectLeafSides_r(&GFront, &GBack, Leaf, 0, 1);
		//IntersectLeafSides2(&GBack, Leaf);

		return;
	}

	Side = Trace_BoxOnPlaneSide(&GMins2, &GMaxs2, &MiscPlanes[MiscNodes[Node].PlaneNum]);

	// Go down the sides that the box lands in
	if (Side & PSIDE_FRONT)
		FindClosestLeafIntersection_r(MiscNodes[Node].Children[0]);

	if (Side & PSIDE_BACK)
		FindClosestLeafIntersection_r(MiscNodes[Node].Children[1]);
}

#define SIDE_SPACE		0.1f
//=====================================================================================
//	Trace_WorldCollisionBBox
//	Shoots a ray through the world, using the expandable leaf hull
//  The hull is expanded by the input BBox to simulate the points having volume...
//=====================================================================================
geBoolean Trace_WorldCollisionBBox(	geWorld	*World,
									const	geVec3d *Mins, const geVec3d *Maxs, 
									const	geVec3d *Front, const geVec3d *Back,
									uint32	Flags,
									geVec3d *I, GFX_Plane *P,
									geWorld_Model **Model,
									Mesh_RenderQ **Mesh,
									geActor **Actor,
									uint32 UserFlags,
									GE_CollisionCB *CollisionCB,
									void *Context)
{
	geWorld_Model	*Models;
	geVec3d			NewFront, NewBack, OMins, OMaxs, BestI, Vect;
	geVec3d			Impact;
	int32			i, b;
	float			Dist, BestD;
	Mesh_RenderQ	*BestMesh;
	#ifdef MESHES
	Mesh_RenderQ    *Mesh2;
	#endif
	geActor			*BestActor;
	geWorld_Model	*BestModel;
	GFX_Plane		BestPlane, Plane2;
	geBoolean		Hit;

	if (Model)
		*Model = NULL;
	if (Mesh)
		*Mesh = NULL;
	if (Actor)
		*Actor = NULL;

	BestD = 99999.0f;
	BestMesh = NULL;
	BestModel = NULL;
	BestActor = NULL;
	Hit = GE_FALSE;				// Have not hit nothing yet...
	
	// Test meshes first... (record the closest collision)
	if (Flags & GE_COLLIDE_MESHES)
	{
#ifdef	MESHES
		if (Mesh_MeshCollisionAll(World, Mins, Maxs, Front, Back, &Impact, &Plane2, &Mesh2, UserFlags))
		{
			geVec3d_Subtract(&Impact, Front, &Vect);
			Dist = geVec3d_Length(&Vect);
			if (Dist < BestD)
			{
				BestD = Dist;
				BestI = Impact;
				BestMesh = Mesh2;
				BestPlane = Plane2;
				Hit = GE_TRUE;			// We hit somthing
			}
		}
#endif
	}

	
	if (Flags & GE_COLLIDE_ACTORS)
		{
			BestActor = Trace_ActorCollide(World, Mins,Maxs, Front,Back,&Impact,&Plane2,UserFlags, CollisionCB, Context, &BestD);
			if (BestActor != NULL)
				{
					BestI = Impact;
					BestPlane = Plane2;
					Hit = GE_TRUE;
				}
		}

	
	// GMins1/GMaxs1 is what is used to exapand the plane out with
	GMins1 = *Mins;
	GMaxs1 = *Maxs;
	
	GFront = *Front;
	GBack = *Back;

	BSPData = &World->CurrentBSP->BSPData;
	Models = World->CurrentBSP->Models;

	MiscNodes = BSPData->GFXNodes;
	MiscPlanes = BSPData->GFXPlanes;
	MiscLeafs = BSPData->GFXLeafs;
	MiscSides = BSPData->GFXLeafSides;

	assert(MiscNodes != NULL);
	assert(MiscPlanes != NULL);
	assert(MiscLeafs != NULL);
	assert(MiscSides != NULL);

	if (!(Flags & GE_COLLIDE_MODELS))
		goto NoModels;
	
	Trace_GetMoveBox(Mins, Maxs, Front, Back, &OMins, &OMaxs);

	// Then test the world bsp(all models are the world bsp)
	// Go through each model, and find out what leafs we hit, keeping the closest intersection
	for (i = 0; i < BSPData->NumGFXModels; i++, Models++)
	{
		
		// First, see if the user wants to reject it...
		if (CollisionCB && !CollisionCB(Models, NULL, Context))
			continue;

		for (b=0; b<3; b++)
		{
			if (VectorToSUB(OMaxs, b) < VectorToSUB(Models->TMins, b))
				break;
			if (VectorToSUB(OMins, b) > VectorToSUB(Models->TMaxs, b))
				break;
		}

		if (b != 3)
			continue;
		

		// Reset flags
		BestDist = 9999.0f;
		LeafHit = FALSE;
		
		geVec3d_Subtract(Front, &Models->Pivot, &GFront);
		geVec3d_Subtract(Back , &Models->Pivot, &GBack);

		// InverseTransform the point about models center of rotation
		geXForm3d_TransposeTransform(&Models->XForm, &GFront, &NewFront);
		geXForm3d_TransposeTransform(&Models->XForm, &GBack , &NewBack);

		// push back into world
		geVec3d_Add(&NewFront, &Models->Pivot, &GFront);
		geVec3d_Add(&NewBack , &Models->Pivot, &GBack);
		
		// Make out box out of this move so we only check the leafs it intersected with...
		Trace_GetMoveBox(Mins, Maxs, &GFront, &GBack, &GMins2, &GMaxs2);

		FindClosestLeafIntersection_r(BSPData->GFXModels[i].RootNode[0]);

		if (LeafHit)
		{
			
			// Rotate the impact plane
			geXForm3d_Rotate(&Models->XForm, &GlobalPlane.Normal, &GlobalPlane.Normal);
			
			// Rotate the impact point
			geVec3d_Subtract(&GlobalI, &Models->Pivot, &GlobalI);
			geXForm3d_Transform(&Models->XForm, &GlobalI, &NewFront);
			//geXForm3d_Rotate(&Models->XForm, &GlobalI, &NewFront);
			geVec3d_Add(&NewFront, &Models->Pivot, &GlobalI);
			
			// Find the new plane distance based on the new impact point with the new plane
			GlobalPlane.Dist = geVec3d_DotProduct(&GlobalPlane.Normal, &GlobalI);

			geVec3d_Subtract(&GlobalI, Front, &Vect);
			Dist = geVec3d_Length(&Vect);
			if (Dist < BestD)
			{
				BestD = Dist;
				BestI = GlobalI;
				BestPlane = GlobalPlane;
				BestModel = Models;
				BestMesh = NULL;			// Reset the mesh flag...
				BestActor = NULL;
				Hit = GE_TRUE;
			}
		}

		if ((Flags & GE_COLLIDE_NO_SUB_MODELS))
			goto NoModels;

	}

	NoModels:

	if (Hit)
	{
		if (I) 
			*I = BestI;
		if (P) 
			*P = BestPlane;
		if (Mesh) 
			*Mesh = BestMesh;
		if (Model) 
			*Model = BestModel;
		if (Actor)
			*Actor = BestActor;
		return GE_TRUE;
	}

	return GE_FALSE;
}

//=====================================================================================
//	Trace_TestModelMove
//=====================================================================================
geBoolean Trace_TestModelMove(	geWorld			*World, 
								geWorld_Model	*Model, 
								const geXForm3d	*DXForm, 
								const geVec3d	*Mins, const geVec3d *Maxs,
								const geVec3d	*In, geVec3d *Out)
{
	geVec3d		NewFront, NewBack, Original;

	assert(World != NULL);
	assert(Model != NULL);

	BSPData = &World->CurrentBSP->BSPData;

	MiscNodes = BSPData->GFXNodes;
	MiscPlanes = BSPData->GFXPlanes;
	MiscLeafs = BSPData->GFXLeafs;
	MiscSides = BSPData->GFXLeafSides;

	assert(MiscNodes != NULL);
	assert(MiscPlanes != NULL);
	assert(MiscLeafs != NULL);
	assert(MiscSides != NULL);

	Original = *In;		// Save original

	GMins1 = *Mins;
	GMaxs1 = *Maxs;
	
	// Put point about models origin
	geVec3d_Subtract(In, &Model->Pivot, &GFront);
	GBack = GFront;

	// InverseTransform the points about models center of rotation
	geXForm3d_TransposeTransform(&Model->XForm, &GFront, &NewFront);
	// The back gets applied by the dest XForm
	geXForm3d_TransposeTransform(DXForm, &GBack, &NewBack);

	// push back into world
	geVec3d_Add(&NewFront, &Model->Pivot, &GFront);
	geVec3d_Add(&NewBack , &Model->Pivot, &GBack);

	// Make out box out of this move so we only check the leafs it intersected with...
	Trace_GetMoveBox(Mins, Maxs, &GFront, &GBack, &GMins2, &GMaxs2);
	
	BestDist = 9999.0f;
	LeafHit = FALSE;

	FindClosestLeafIntersection_r(BSPData->GFXModels[Model->GFXModelNum].RootNode[0]);

	if (LeafHit)
	{
		GE_Collision	Collision;

		// Rotate the impact plane
		geXForm3d_Rotate(DXForm, &GlobalPlane.Normal, &GlobalPlane.Normal);
			
		// Rotate the impact point
		geVec3d_Subtract(&GlobalI, &Model->Pivot, &NewFront);
		geXForm3d_Transform(DXForm, &NewFront, &GlobalI);
		geVec3d_Add(&GlobalI, &Model->Pivot, &NewFront);
		GlobalI = NewFront;

		// Find the new plane distance based on the new impact point with the new plane
		GlobalPlane.Dist = geVec3d_DotProduct(&GlobalPlane.Normal, &GlobalI);

		geVec3d_MA(&GlobalI, ON_EPSILON, &GlobalPlane.Normal, &GlobalI);
		
		// If the point gets pushed into the world as a result of the move, then cancel it out...
		if (Trace_GEWorldCollision(World, Mins, Maxs, In, &GlobalI, GE_CONTENTS_SOLID_CLIP, GE_COLLIDE_ALL, 0xffffffff, NULL, NULL, &Collision))
		{
			*Out = Original;
			return GE_FALSE;
		}

		*Out = GlobalI;

		return GE_TRUE;
	}

	*Out = Original;

	return GE_TRUE;
}

//=====================================================================================
//	Trace_ModelCollisionBBox
//=====================================================================================
static
geBoolean Trace_ModelCollisionBBox(geWorld			*World, 
									geWorld_Model		*Model, 
									const geXForm3d	*DXForm, 
									const geVec3d	*Mins, const geVec3d *Maxs,
									const geVec3d	*In,
									geVec3d			*ImpactPoint)
{
	geVec3d		NewFront, NewBack, Original;

	assert(World != NULL);
	assert(Model != NULL);

	BSPData = &World->CurrentBSP->BSPData;

	MiscNodes = BSPData->GFXNodes;
	MiscPlanes = BSPData->GFXPlanes;
	MiscLeafs = BSPData->GFXLeafs;
	MiscSides = BSPData->GFXLeafSides;

	assert(MiscNodes != NULL);
	assert(MiscPlanes != NULL);
	assert(MiscLeafs != NULL);
	assert(MiscSides != NULL);

	Original = *In;		// Save original

	GMins1 = *Mins;
	GMaxs1 = *Maxs;
	
	// Put point about models origin
	geVec3d_Subtract(In, &Model->Pivot, &GFront);
	GBack = GFront;

	// InverseTransform the points about models center of rotation
	geXForm3d_TransposeTransform(&Model->XForm, &GFront, &NewFront);
	// The back gets applied by the dest XForm
	geXForm3d_TransposeTransform(DXForm, &GBack, &NewBack);

	// push back into world
	geVec3d_Add(&NewFront, &Model->Pivot, &GFront);
	geVec3d_Add(&NewBack , &Model->Pivot, &GBack);

	// Make out box out of this move so we only check the leafs it intersected with...
	Trace_GetMoveBox(Mins, Maxs, &GFront, &GBack, &GMins2, &GMaxs2);
	
	BestDist = 9999.0f;
	LeafHit = FALSE;

	FindClosestLeafIntersection_r(BSPData->GFXModels[Model->GFXModelNum].RootNode[0]);

	if (LeafHit)
	{
		// Rotate the impact plane
		geXForm3d_Rotate(DXForm, &GlobalPlane.Normal, &GlobalPlane.Normal);
			
		// Rotate the impact point
		geVec3d_Subtract(&GlobalI, &Model->Pivot, &NewFront);
		geXForm3d_Transform(DXForm, &NewFront, &GlobalI);
		geVec3d_Add(&GlobalI, &Model->Pivot, &NewFront);
		GlobalI = NewFront;

		// Find the new plane distance based on the new impact point with the new plane
		GlobalPlane.Dist = geVec3d_DotProduct(&GlobalPlane.Normal, &GlobalI);

		geVec3d_MA(&GlobalI, ON_EPSILON, &GlobalPlane.Normal, &GlobalI);

		*ImpactPoint = GlobalI;

		return GE_TRUE;
	}

	return GE_FALSE;
}

//=====================================================================================
//	Trace_ModelCollision
//=====================================================================================
geBoolean Trace_ModelCollision(geWorld			*World, 
								geWorld_Model	*Model, 
								const geXForm3d	*DXForm,
								GE_Collision	*Collision,
								geVec3d			*ImpactPoint)
{
	geExtBox ExtBox;
	geVec3d	Pos;
#ifdef MESHES
	Mesh_RenderQ *				CollidableMesh;
	Mesh_CollidableMeshIterator	Iter;
#endif
	memset(Collision, 0, sizeof(GE_Collision));

	// Fixed bug that mike pointed out.  I was using 0, instead of 0xffffffff
#ifdef	MESHES
	CollidableMesh = Mesh_FirstCollidableMesh(World, &Iter, 0xffffffff);
	while	(CollidableMesh)
	{
		Mesh_MeshGetBox(World, CollidableMesh->MeshDef, &Mins, &Maxs);
		Mesh_MeshGetPosition(CollidableMesh, &Pos);
		if	(Trace_ModelCollisionBBox(World, Model, DXForm, &Mins, &Maxs, &Pos, ImpactPoint))
		{
			Collision->Mesh = (geMesh *)CollidableMesh;
			return GE_TRUE;
		}
		CollidableMesh = Mesh_NextCollidableMesh(&Iter, 0xffffffff);
	}
#endif

	{
		int i,Count;
		World_Actor *WA;
			
		Count = World->ActorCount;
		WA = &(World->ActorArray[0]);

		for (i=0; i<Count; i++, WA++)
			{
				// if it's active	(ignore userflags?)
				if ( (WA->Flags & GE_ACTOR_COLLIDE) )	
					{
						if (geActor_GetExtBox(WA->Actor,&ExtBox)!=GE_FALSE)
							{
								if (Trace_ModelCollisionBBox(World, Model, DXForm, &(ExtBox.Min), &(ExtBox.Max), &Pos, ImpactPoint))
								{
									Collision->Actor = WA->Actor;
									return GE_TRUE;
								}
							}
					}
			}
	}

	return GE_FALSE;
}

//=====================================================================================
//	MoveBox
//	Creates a box around the entire move
//=====================================================================================
void Trace_GetMoveBox(const geVec3d *Mins, const geVec3d *Maxs, const geVec3d *Front, const geVec3d *Back, geVec3d *OMins, geVec3d *OMaxs)
{
	int32		i;

	assert(Mins);
	assert(Maxs);
	assert(Front);
	assert(Back);
	
	for (i=0 ; i<3 ; i++)
	{
		if (VectorToSUB(*Back, i) > VectorToSUB(*Front, i))
		{
			VectorToSUB(*OMins, i) = VectorToSUB(*Front, i) + VectorToSUB(*Mins, i) - 1.0f;
			VectorToSUB(*OMaxs, i) = VectorToSUB(*Back, i) + VectorToSUB(*Maxs, i) + 1.0f;
		}
		else
		{
			VectorToSUB(*OMins, i) = VectorToSUB(*Back, i) + VectorToSUB(*Mins, i) - 1.0f;
			VectorToSUB(*OMaxs, i) = VectorToSUB(*Front, i) + VectorToSUB(*Maxs, i) + 1.0f;
		}
	}
}


static geBoolean	VisibleLeaf;
//=====================================================================================
//	BBoxInVisibleLeaf_r
//=====================================================================================
static void BBoxInVisibleLeaf_r(geWorld *World, geVec3d *Mins, geVec3d *Maxs, int32 Node)
{
	int32		Leaf, Side;

	if (VisibleLeaf)
		return;
	
	if (Node < 0)		// At a leaf, see if it's visible
	{
		Leaf = -(Node+1);

		if (World->CurrentBSP->LeafData[Leaf].VisFrame == World->CurFrameStatic)
			VisibleLeaf = TRUE;
		
		return;
	}

	Side = Trace_BoxOnPlaneSide(Mins, Maxs, &MiscPlanes[MiscNodes[Node].PlaneNum]);

	// Go down the sides that the box lands in
	if (Side & PSIDE_FRONT)
		BBoxInVisibleLeaf_r(World, Mins, Maxs, MiscNodes[Node].Children[0]);

	if (Side & PSIDE_BACK)
		BBoxInVisibleLeaf_r(World, Mins, Maxs, MiscNodes[Node].Children[1]);
}

geBoolean Trace_BBoxInVisibleLeaf(geWorld *World, geVec3d *Mins, geVec3d *Maxs)
{
	VisibleLeaf = FALSE;

	MiscNodes = World->CurrentBSP->BSPData.GFXNodes;
	MiscPlanes = World->CurrentBSP->BSPData.GFXPlanes;
	
	BBoxInVisibleLeaf_r(World, Mins, Maxs, 0);

	return VisibleLeaf;
}

//===================================================================================
//	Trace_InverseTreeFromBox
//	Builds am inside out collision tree from a box
//===================================================================================
geBoolean Trace_InverseTreeFromBox(geVec3d *Mins, geVec3d *Maxs, GFX_BNode *BNodes, GFX_Plane *Planes)
{
	int32		i,j,n;
	float		Bounds[2][3];
	GFX_Plane	*Plane;

	for (i=0; i< 3; i++)
	{
		Bounds[0][i] = VectorToSUB(*Mins, i);
		Bounds[1][i] = VectorToSUB(*Maxs, i);
	}

	// Build the boxs planes
	for (i=0; i< 3; i++)
	{
		for (j=0; j< 2; j++)
		{
			n = j*3 + i;
			
			Plane = &Planes[n];

			memset(Plane, 0, sizeof(GFX_Plane));

			if (!j)		// Inside out
			{
				VectorToSUB(Plane->Normal, i) = 1.0f;
				Plane->Dist = Bounds[j][i];
			}
			else
			{
				VectorToSUB(Plane->Normal, i) = -1.0f;
				Plane->Dist = -Bounds[j][i];
			}

			Plane->Type = PLANE_ANY;

			//
			// Build the tree
			//
			BNodes[n].PlaneNum = n;

			BNodes[n].Children[1] = BSP_CONTENTS_SOLID;
			
			if (n == 5)		
				BNodes[n].Children[0] = BSP_CONTENTS_EMPTY;
			else		
				BNodes[n].Children[0] = n+1;
		}
	}
	
	return GE_TRUE;
}

//=====================================================================================
//	Trace_FindBNodeContents
//	BNodes are special.  Instead of having a negative index to a leaf, the negative
//  node number represents the contents of the leaf...
//=====================================================================================
int32 Plane_FindBNodeContents(	const GFX_BNode *Nodes, 
								const GFX_Plane *Planes, 
								int32 Node, 
								const geVec3d *POV)
{
    float		Dist;

    while (Node >= 0)		// < 0 == leaf with contents
	{
		Dist = Plane_PlaneDistanceFast(&Planes[Nodes[Node].PlaneNum], POV);
        
		if (Dist < 0) 
            Node = Nodes[Node].Children[1];
		else
            Node = Nodes[Node].Children[0];
    }
	
	return Node;		// Return the contents
}

//=====================================================================================
//	FillContents_r
//	Traverses the leafs and or's all the contents together
//=====================================================================================
static void FillContents_r(int32 Node, const geVec3d *Pos, uint32 *Contents)
{
	int32		Side;

	if (Node < 0)		// At a leaf, fill contens and return
	{
		int32		Leaf;

		Leaf = -(Node+1);
		if (PointInLeafSides(Pos, &MiscLeafs[Leaf]))
			*Contents |= MiscLeafs[Leaf].Contents;
		return;
	}

	Side = Trace_BoxOnPlaneSide(&GMins2, &GMaxs2, &MiscPlanes[MiscNodes[Node].PlaneNum]);

	// Go down the sides that the box lands in
	if (Side & PSIDE_FRONT)
		FillContents_r(MiscNodes[Node].Children[0], Pos, Contents);

	if (Side & PSIDE_BACK)
		FillContents_r(MiscNodes[Node].Children[1], Pos, Contents);
}

//===================================================================================
//	Trace_GetContents
//	Fills a Contents structure with data and returns GE_TRUE if somthing was occupied
//	Otherwise, it returns GE_FALSE and nothing is assumed to be occupied
//===================================================================================
geBoolean Trace_GetContents(geWorld *World, const geVec3d *Pos, const geVec3d *Mins, const geVec3d *Maxs, uint32 Flags, uint32 UserFlags, GE_CollisionCB *CollisionCB, void *Context, GE_Contents *Contents)
{
	Mesh_RenderQ				*MeshHit;
	geActor						*ActorHit;
	geExtBox					MeshExtBox;
	geVec3d						TMins, TMaxs;
	geBoolean					Hit;
	int32						i, k;
	uint32						NewContents, FinalContents;
	geWorld_Model				*Models, *ModelHit;
	GFX_Model					*GFXModels;

	assert(World);
	assert(Contents);
	
	MeshHit = NULL;
	ModelHit = NULL;
	FinalContents = 0;
	Hit = GE_FALSE;

	BSPData = &World->CurrentBSP->BSPData;

	// Get the translated box from the input pos...
	geVec3d_Add(Mins, Pos, &TMins);
	geVec3d_Add(Maxs, Pos, &TMaxs);

	NumGetContents++;

	if (Flags & GE_COLLIDE_ACTORS)
	{
		int32 Count;
		World_Actor *WA;
			
		Count = World->ActorCount;
		WA = &(World->ActorArray[0]);

		for (i=0; i<Count; i++, WA++)
		{
				
			// Reject if not active or if userflags don't accept...
			if (!(WA->Flags & GE_ACTOR_COLLIDE) || !(WA->UserFlags & UserFlags) )
				continue;

			if (CollisionCB && !CollisionCB(NULL, WA->Actor, Context))
				continue;

				
			if (geActor_GetExtBox(WA->Actor,&MeshExtBox)==GE_FALSE)
				continue;

			for (k=0; k<3; k++)
			{
				if (geVec3d_GetElement(&TMaxs,k) < geVec3d_GetElement(&(MeshExtBox.Min),k)-1)
					break;
				if (geVec3d_GetElement(&TMins,k) > geVec3d_GetElement(&(MeshExtBox.Max),k)+1)
					break;
			}
				
			if (k != 3)
				continue;

			ActorHit = WA->Actor;
			Hit = GE_TRUE;
			break;				// Just return the first actor
		}
	}

	if (!(Flags & GE_COLLIDE_MODELS))
		goto NoModels;
	
	MiscNodes = World->CurrentBSP->BSPData.GFXNodes;
	MiscPlanes = World->CurrentBSP->BSPData.GFXPlanes;
	MiscLeafs = World->CurrentBSP->BSPData.GFXLeafs;
	MiscSides = BSPData->GFXLeafSides;

	Models = World->CurrentBSP->Models;
	GFXModels = World->CurrentBSP->BSPData.GFXModels;

	GMins1 = *Mins;
	GMaxs1 = *Maxs;

	GMins2 = TMins;
	GMaxs2 = TMaxs;

	for (i = 0; i < BSPData->NumGFXModels; i++, Models++, GFXModels++)
	{
		geVec3d	TPos;

		if (CollisionCB && !CollisionCB(Models, NULL, Context))
			continue;

		if (i > 0)		// Ignore model 0 box (main world, we should always try to collide with world)
		{
			for (k=0; k<3; k++)
			{
				if (VectorToSUB(TMaxs, k) < VectorToSUB(Models->TMins, k))
					break;
				if (VectorToSUB(TMins, k) > VectorToSUB(Models->TMaxs, k))
					break;
			}

			if (k != 3)			// Couldn't possibly hit if box's don't hit...
				continue;
		}

		geVec3d_Subtract(Pos, &Models->Pivot, &TPos);

		// InverseTransform the point about models center of rotation
		geXForm3d_TransposeTransform(&Models->XForm, &TPos, &TPos);

		// push back into world
		geVec3d_Add(&TPos, &Models->Pivot, &TPos);

		// Reset contents
		NewContents = 0;

		FillContents_r(GFXModels->RootNode[0], &TPos, &NewContents);

		if (NewContents && !ModelHit)
		{
			ModelHit = Models;						// First model hit for any arbritrary contents
			Hit = GE_TRUE;
		}

		if (NewContents & GE_CONTENTS_SOLID_CLIP)	// Solid has precedence
		{
			ModelHit = Models;
		}

		// Or final contents with this new contents
		FinalContents |= NewContents;

		if ((Flags & GE_COLLIDE_NO_SUB_MODELS))
			goto NoModels;
	}

	NoModels:

	if (Hit)
	{
		Contents->Contents = FinalContents;
		Contents->Mesh = (geMesh*)MeshHit;
		Contents->Model = ModelHit;
		Contents->Actor = ActorHit;
		return GE_TRUE;
	}

	// If nothing occupied, then make sure the return structure is cleared for cleanness
	memset(Contents, 0, sizeof(GE_Contents));

	return GE_FALSE;
}



//=====================================================================================
//	Trace_SetupIntersect
//=====================================================================================
static	GFX_Plane	*TreePlanes;
static	GFX_Node	*TreeNodes;
static	GFX_Leaf	*TreeLeafs;

void Trace_SetupIntersect(geWorld *World)
{
	BSPData = &World->CurrentBSP->BSPData;

	TreePlanes = BSPData->GFXPlanes;
	TreeNodes = BSPData->GFXNodes;
	TreeLeafs = BSPData->GFXLeafs;
}

//=====================================================================================
//	Trace_IntersectWorldBSP
//	Shoot a ray through the tree finding out what solid leafs it passed through
//=====================================================================================
geBoolean Trace_IntersectWorldBSP(geVec3d *Front, geVec3d *Back, int32 Node)
{
    float		Fd, Bd, Dist;
    int32		Side;
    geVec3d		I;
	GFX_Plane	*Plane;
	int32		Contents;

	gContents = GE_CONTENTS_SOLID_CLIP;

	if (Node < 0)
	{
		Contents = TreeLeafs[-(Node+1)].Contents;

		if (Contents & gContents)
		    return GE_TRUE;						// Ray collided with solid space
		return GE_FALSE;
	}

	Plane = &TreePlanes[TreeNodes[Node].PlaneNum];

    Fd = Plane_PlaneDistanceFast(Plane, Front);
    Bd = Plane_PlaneDistanceFast(Plane, Back);

    if (Fd >= 0 && Bd >= 0) 
        return(BSPIntersect(Front, Back, TreeNodes[Node].Children[0]));
    if (Fd < 0 && Bd < 0)
        return(BSPIntersect(Front, Back, TreeNodes[Node].Children[1]));

    Side = Fd < 0;
    Dist = Fd / (Fd - Bd);

    I.X = Front->X + Dist * (Back->X - Front->X);
    I.Y = Front->Y + Dist * (Back->Y - Front->Y);
    I.Z = Front->Z + Dist * (Back->Z - Front->Z);

    // Work our way to the front, from the back side.  As soon as there
	// is no more collisions, we can assume that we have the front portion of the
	// ray that is in empty space.  Once we find this, and see that the back half is in
	// solid space, then we found the front intersection point...
	if (BSPIntersect(Front, &I, TreeNodes[Node].Children[Side]))
        return GE_TRUE;
    else if (BSPIntersect(&I, Back, TreeNodes[Node].Children[!Side]))
	{
		return GE_TRUE;
	}

	return GE_FALSE;
}

static	geVec3d		gStart, gEnd;
static	GFX_Plane	gPlane;

geBoolean Trace_CollideBeam(int32 Node, geVec3d *s, geVec3d *e, geFloat Radius)
{
	float		dd, sDist, eDist;
	geVec3d		tempVec, tempVec2;
	geBoolean	FrontLeaf, BackLeaf;
	GFX_Plane	*Plane;

	if(Node < 0)
	{
		//leaf found, check contents
		return	!(!(BSPData->GFXLeafs[-(Node+1)].Contents & gContents));
	}
	Plane	=&BSPData->GFXPlanes[BSPData->GFXNodes[Node].PlaneNum];

	//startpoint and endpoint plane distances
	sDist	=Plane_PlaneDistanceFast(Plane, s);
	eDist	=Plane_PlaneDistanceFast(Plane, e);

	//check sides for start and end...
	if(*((uint32 *)&sDist) & 0x80000000)	//if sdist < 0
	{
		if(sDist < -Radius)
		{
			if(*((uint32 *)&eDist) & 0x80000000)	//if edist < 0
			{
				if(eDist < -Radius)
				{
					//nothing to front, all to back
					BackLeaf	=Trace_CollideBeam(BSPData->GFXNodes[Node].Children[1], s, e, Radius);
				}
				else
				{
					//impact to e to front, all back
					//find spot where dist==-Radius along motionVec
					//make this the front s
					dd=(sDist+Radius)/((sDist+Radius)-(eDist+Radius));

					//cap the push back factor (don't allow negative)
					if(*((uint32 *)&dd) & 0x80000000)	//is dd negative?
					{
						dd	=0.0f;
					}

					geVec3d_Subtract(e, s, &tempVec);
					geVec3d_Scale(&tempVec, dd, &tempVec2);
					geVec3d_Add(s, &tempVec2, &tempVec);

					//send new piece to front
					FrontLeaf	=Trace_CollideBeam(BSPData->GFXNodes[Node].Children[0], &tempVec, e, Radius);
					BackLeaf	=Trace_CollideBeam(BSPData->GFXNodes[Node].Children[1], s, e, Radius);
				}
			}
			else
			{
				if(eDist > Radius)
				{
					//impact to e to front, sdist to impact to back
					//find spot where dist==-Radius along motionVec
					//make this the front s
					dd=(sDist+Radius)/((sDist+Radius)-(eDist+Radius));

					geVec3d_Subtract(e, s, &tempVec);
					geVec3d_Scale(&tempVec, dd, &tempVec2);
					geVec3d_Add(s, &tempVec2, &tempVec);

					//send new piece to front
					FrontLeaf	=Trace_CollideBeam(BSPData->GFXNodes[Node].Children[0], &tempVec, e, Radius);

					//find spot where dist==Radius along motionVec
					//make this the back e
					dd=(sDist-Radius)/((sDist-Radius)-(eDist-Radius));

					//cap the push back factor (don't allow negative)
					if(*((uint32 *)&dd) & 0x80000000)	//is dd negative?
					{
						dd	=0.0f;
					}

					geVec3d_Subtract(e, s, &tempVec);
					geVec3d_Scale(&tempVec, dd, &tempVec2);
					geVec3d_Add(e, &tempVec2, &tempVec);

					//send new piece to back
					BackLeaf	=Trace_CollideBeam(BSPData->GFXNodes[Node].Children[1], s, &tempVec, Radius);
				}
				else
				{
					//impact to edist to front, all to back
					//find spot where dist==-Radius along motionVec
					//make this the front s
					dd=(sDist+Radius)/((sDist+Radius)-(eDist+Radius));

					//cap the push back factor (don't allow negative)
					if(*((uint32 *)&dd) & 0x80000000)	//is dd negative?
					{
						dd	=0.0f;
					}

					geVec3d_Subtract(e, s, &tempVec);
					geVec3d_Scale(&tempVec, dd, &tempVec2);
					geVec3d_Add(s, &tempVec2, &tempVec);

					//send new piece to front
					FrontLeaf	=Trace_CollideBeam(BSPData->GFXNodes[Node].Children[0], &tempVec, e, Radius);
					BackLeaf	=Trace_CollideBeam(BSPData->GFXNodes[Node].Children[1], s, e, Radius);
				}
			}
		}
		else
		{
			if(*((uint32 *)&eDist) & 0x80000000)	//if edist < 0
			{
				if(eDist < -Radius)
				{
					//sdist to impact to front, all back
					//find spot where dist==-Radius along motionVec
					//make this the front e
					dd=(sDist+Radius)/((sDist+Radius)-(eDist+Radius));

					//cap the push back factor (don't allow negative)
					if(*((uint32 *)&dd) & 0x80000000)	//is dd negative?
					{
						dd	=0.0f;
					}

					geVec3d_Subtract(e, s, &tempVec);
					geVec3d_Scale(&tempVec, dd, &tempVec2);
					geVec3d_Add(e, &tempVec2, &tempVec);

					//send new piece to front
					FrontLeaf	=Trace_CollideBeam(BSPData->GFXNodes[Node].Children[0], s, &tempVec, Radius);
					BackLeaf	=Trace_CollideBeam(BSPData->GFXNodes[Node].Children[1], s, e, Radius);
				}
				else
				{
					//all to front, all to back
					FrontLeaf	=Trace_CollideBeam(BSPData->GFXNodes[Node].Children[0], s, e, Radius);
					BackLeaf	=Trace_CollideBeam(BSPData->GFXNodes[Node].Children[1], s, e, Radius);
				}
			}
			else
			{
				if(eDist > Radius)
				{
					//all to front, sdist to impact to back
					FrontLeaf	=Trace_CollideBeam(BSPData->GFXNodes[Node].Children[0], s, e, Radius);

					//find the spot where dist==Radius along motionVec
					//make this the back e
					dd=(sDist-Radius)/((sDist-Radius)-(eDist-Radius));

					//cap the push back factor (don't allow negative)
					if(*((uint32 *)&dd) & 0x80000000)	//is dd negative?
					{
						dd	=0.0f;
					}

					geVec3d_Subtract(e, s, &tempVec);
					geVec3d_Scale(&tempVec, dd, &tempVec2);
					geVec3d_Add(e, &tempVec2, &tempVec);

					//send new piece to back
					BackLeaf	=Trace_CollideBeam(BSPData->GFXNodes[Node].Children[1], s, &tempVec, Radius);
				}
				else
				{
					//all to front, all to back
					FrontLeaf	=Trace_CollideBeam(BSPData->GFXNodes[Node].Children[0], s, e, Radius);
					BackLeaf	=Trace_CollideBeam(BSPData->GFXNodes[Node].Children[1], s, e, Radius);
				}
			}
		}
	}
	else
	{
		if(sDist > Radius)
		{
			if(!(*((uint32 *)&eDist) & 0x80000000))	//if edist > 0
			{
				if(eDist > Radius)
				{
					//all to front, none to back
					FrontLeaf	=Trace_CollideBeam(BSPData->GFXNodes[Node].Children[0], s, e, Radius);
				}
				else
				{
					//all to front, impact to edist to back
					FrontLeaf	=Trace_CollideBeam(BSPData->GFXNodes[Node].Children[0], s, e, Radius);

					//find spot where dist==Radius along motionVec
					//make this the back s
					dd=(sDist-Radius)/((sDist-Radius)-(eDist-Radius));

					//cap the push back factor (don't allow negative)
					if(*((uint32 *)&dd) & 0x80000000)	//is dd negative?
					{
						dd	=0.0f;
					}

					geVec3d_Subtract(e, s, &tempVec);
					geVec3d_Scale(&tempVec, dd, &tempVec2);
					geVec3d_Add(e, &tempVec2, &tempVec);

					//send new piece to back
					BackLeaf	=Trace_CollideBeam(BSPData->GFXNodes[Node].Children[1], &tempVec, e, Radius);
				}
			}
			else
			{
				if(eDist < -Radius)
				{
					//sdist to impact to front, impact to edist to back
					//find spot where dist==-Radius along motionVec
					//make this the front e
					dd=(sDist+Radius)/((sDist+Radius)-(eDist+Radius));

					geVec3d_Subtract(e, s, &tempVec);
					geVec3d_Scale(&tempVec, dd, &tempVec2);
					geVec3d_Add(e, &tempVec2, &tempVec);

					//send new piece to front
					FrontLeaf	=Trace_CollideBeam(BSPData->GFXNodes[Node].Children[0], s, &tempVec, Radius);

					//find spot where dist==Radius along motionVec
					//make this the back s
					dd=(sDist-Radius)/((sDist-Radius)-(eDist-Radius));

					//cap the push back factor (don't allow negative)
					if(*((uint32 *)&dd) & 0x80000000)	//is dd negative?
					{
						dd	=0.0f;
					}

					geVec3d_Subtract(e, s, &tempVec);
					geVec3d_Scale(&tempVec, dd, &tempVec2);
					geVec3d_Add(s, &tempVec2, &tempVec);

					//send new piece to back
					BackLeaf	=Trace_CollideBeam(BSPData->GFXNodes[Node].Children[1], &tempVec, e, Radius);
				}
				else
				{
					//all to front, impact to edist to back
					FrontLeaf	=Trace_CollideBeam(BSPData->GFXNodes[Node].Children[0], s, e, Radius);

					//find spot where dist==Radius along motionVec
					//make this the back s
					dd=(sDist-Radius)/((sDist-Radius)-(eDist-Radius));

					//cap the push back factor (don't allow negative)
					if(*((uint32 *)&dd) & 0x80000000)	//is dd negative?
					{
						dd	=0.0f;
					}

					geVec3d_Subtract(e, s, &tempVec);
					geVec3d_Scale(&tempVec, dd, &tempVec2);
					geVec3d_Add(s, &tempVec2, &tempVec);

					//send new piece to back
					BackLeaf	=Trace_CollideBeam(BSPData->GFXNodes[Node].Children[1], &tempVec, e, Radius);
				}
			}
		}
		else
		{
			if(!(*((uint32 *)&eDist) & 0x80000000))	//if edist > 0
			{
				if(eDist > Radius)
				{
					//all to front, sdist to impact to back
					FrontLeaf	=Trace_CollideBeam(BSPData->GFXNodes[Node].Children[0], s, e, Radius);

					//find the spot where dist==Radius along motionVec
					//make this the back e
					dd=(sDist-Radius)/((sDist-Radius)-(eDist-Radius));

					//cap the push back factor (don't allow negative)
					if(*((uint32 *)&dd) & 0x80000000)	//is dd negative?
					{
						dd	=0.0f;
					}

					geVec3d_Subtract(e, s, &tempVec);
					geVec3d_Scale(&tempVec, dd, &tempVec2);
					geVec3d_Add(e, &tempVec2, &tempVec);

					//send new piece to back
					BackLeaf	=Trace_CollideBeam(BSPData->GFXNodes[Node].Children[1], s, &tempVec, Radius);
				}
				else
				{
					//all to front, all to back
					FrontLeaf	=Trace_CollideBeam(BSPData->GFXNodes[Node].Children[0], s, e, Radius);
					BackLeaf	=Trace_CollideBeam(BSPData->GFXNodes[Node].Children[1], s, e, Radius);
				}
			}
			else
			{
				if(eDist < -Radius)
				{
					//sdist to impact to front, all to back
					//find spot where dist==-Radius along motionVec
					//make this the front e
					dd=(sDist+Radius)/((sDist+Radius)-(eDist+Radius));

					//cap the push back factor (don't allow negative)
					if(*((uint32 *)&dd) & 0x80000000)	//is dd negative?
					{
						dd	=0.0f;
					}

					geVec3d_Subtract(e, s, &tempVec);
					geVec3d_Scale(&tempVec, dd, &tempVec2);
					geVec3d_Add(e, &tempVec2, &tempVec);

					//send new piece to front
					FrontLeaf	=Trace_CollideBeam(BSPData->GFXNodes[Node].Children[0], s, &tempVec, Radius);
					BackLeaf	=Trace_CollideBeam(BSPData->GFXNodes[Node].Children[1], s, e, Radius);
				}
				else
				{
					//all to front, all to back
					FrontLeaf	=Trace_CollideBeam(BSPData->GFXNodes[Node].Children[0], s, e, Radius);
					BackLeaf	=Trace_CollideBeam(BSPData->GFXNodes[Node].Children[1], s, e, Radius);
				}
			}
		}
	}

	//bsp ordering makes the magic happen
	//stack based can early out methinks, but it's way too ugly
	if(BackLeaf && !FrontLeaf)
	{
		gStart	=*s;
		gEnd	=*e;
		gPlane	=*Plane;
	}

	//this will nullify farther collisions
	return	GE_FALSE;
}
