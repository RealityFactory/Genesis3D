/****************************************************************************************/
/*  Vis.c                                                                               */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Code to vis the world from a given pov                                 */
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

#include "BaseType.h"
#include "World.h"
#include "Plane.h"
#include "Vec3d.h"
#include "Ram.h"
#include "Surface.h"
#include "Trace.h"
#include "Camera.h"
#include "Frustum.h"
#include "System.h"

#include "Fog.h"

#ifdef _TSC
#include "tsc.h"
#endif

//#define SUPER_VIS1

static void MarkVisibleParents(geWorld *World, int32 Leaf);
static void FindParents(World_BSP *Bsp);
static void VisFog(geEngine *Engine, geWorld *World, const geCamera *Camera, Frustum_Info *Fi, int32 Area);

//=====================================================================================
//	Vis_WorldInit
//=====================================================================================
geBoolean Vis_WorldInit(geWorld *World)
{
	World_BSP	*BSP;
	int32		i;
	static int32 StartupParams[]={0x696C6345,0x21657370};
	
	assert(World  != NULL);

	BSP = World->CurrentBSP;

	assert(BSP != NULL);

	if (!BSP)
		return GE_FALSE;

	BSP->NodeVisFrame = GE_RAM_ALLOCATE_ARRAY(int32, BSP->BSPData.NumGFXNodes);

	if (!BSP->NodeVisFrame)
		goto Error;

	BSP->ClusterVisFrame = GE_RAM_ALLOCATE_ARRAY(int32, BSP->BSPData.NumGFXClusters);

	if (!BSP->ClusterVisFrame)
		goto Error;

	BSP->AreaVisFrame = GE_RAM_ALLOCATE_ARRAY(int32, BSP->BSPData.NumGFXAreas);

	if (!BSP->AreaVisFrame)
		goto Error;

	BSP->NodeParents = GE_RAM_ALLOCATE_ARRAY(int32, BSP->BSPData.NumGFXNodes);

	if (!BSP->NodeParents)
		goto Error;

	memset(BSP->NodeVisFrame, 0, sizeof(int32)*BSP->BSPData.NumGFXNodes);
	memset(BSP->ClusterVisFrame, 0, sizeof(int32)*BSP->BSPData.NumGFXClusters);
	memset(BSP->AreaVisFrame, 0, sizeof(int32)*BSP->BSPData.NumGFXAreas);

	memset(BSP->NodeParents, 0, sizeof(int32)*BSP->BSPData.NumGFXNodes);
	
	FindParents(World->CurrentBSP);

	// Set the identity on the AreaMatrix
	for (i=0; i<256; i++)
		World->CurrentBSP->AreaConnections[i][i] = 1;

	return GE_TRUE;

	Error:
		if (BSP->NodeVisFrame)
			geRam_Free(BSP->NodeVisFrame);
		if (BSP->ClusterVisFrame)
			geRam_Free(BSP->ClusterVisFrame);
		if (BSP->AreaVisFrame)
			geRam_Free(BSP->AreaVisFrame);
		if (BSP->NodeParents)
			geRam_Free(BSP->NodeParents);

		BSP->NodeVisFrame = NULL;
		BSP->ClusterVisFrame = NULL;
		BSP->AreaVisFrame = NULL;
		BSP->NodeParents = NULL;
		return GE_FALSE;
}

//=====================================================================================
//	Vis_WorldShutdown
//=====================================================================================
void Vis_WorldShutdown(geWorld *World)
{
	World_BSP	*BSP;
	
	assert(World  != NULL);

	if (!World->CurrentBSP)
		return;

	BSP = World->CurrentBSP;
	
	if (BSP->NodeVisFrame)
		geRam_Free(BSP->NodeVisFrame);
	if (BSP->ClusterVisFrame)
		geRam_Free(BSP->ClusterVisFrame);
	if (BSP->AreaVisFrame)
		geRam_Free(BSP->AreaVisFrame);
	if (BSP->NodeParents)
		geRam_Free(BSP->NodeParents);

	BSP->NodeVisFrame = NULL;
	BSP->ClusterVisFrame = NULL;
	BSP->AreaVisFrame = NULL;
	BSP->NodeParents = NULL;
}

//=====================================================================================
//	ModelVisible
//=====================================================================================
geBoolean ModelVisible(geWorld *World, geWorld_Model *Model)
{
	return Trace_BBoxInVisibleLeaf(World, &Model->TMins, &Model->TMaxs);
}

//=====================================================================================
//	Vis_FloodAreas_r
//=====================================================================================
void Vis_FloodAreas_r(geWorld *World, int32 Area)
{
	GBSP_BSPData	*BSP;
	GFX_Area		*a;
	GFX_AreaPortal	*p;
	int32			i;

	if (World->CurrentBSP->AreaVisFrame[Area] == World->CurFrameStatic)
		return;		// Area already set
	
	World->CurrentBSP->AreaVisFrame[Area] = World->CurFrameStatic;		// Mark this area visible
	
	BSP = &World->CurrentBSP->BSPData;

	a = &BSP->GFXAreas[Area];

	p = &BSP->GFXAreaPortals[a->FirstAreaPortal];

	for (i=0; i< a->NumAreaPortals; i++, p++)
	{
		geWorld_Model	*Model;

		Model = &World->CurrentBSP->Models[p->ModelNum];

		if (Model->Open)
			Vis_FloodAreas_r(World, p->Area);
	}
}

//=====================================================================================
//	Vis_VisWorld
//=====================================================================================
geBoolean Vis_VisWorld(geEngine *Engine, geWorld *World, const geCamera *Camera, Frustum_Info *Fi)
{
	uint8			*VisData;
	int32			k, i, Area;
	GFX_Node		*GFXNodes;
	GFX_Leaf		*GFXLeafs;
	Surf_SurfInfo	*SurfInfo;
	uint8			*GFXVisData;
	int32			Leaf, Cluster;
	GFX_Model		*GFXModels;
	int32			*GFXLeafFaces;
	GFX_Cluster		*GFXClusters;
	GBSP_BSPData	*BSPData;
	geWorld_Model	*Models;
	const geVec3d	*Pos;
	GFX_Leaf		*pLeaf;

#ifdef _TSC
	pushTSC();
#endif

	Pos = geCamera_GetVisPov(Camera);

	BSPData = &World->CurrentBSP->BSPData;

	GFXNodes = BSPData->GFXNodes;
	GFXLeafs = BSPData->GFXLeafs;
	GFXClusters = BSPData->GFXClusters;
	GFXVisData = BSPData->GFXVisData;
	GFXModels = BSPData->GFXModels;
	GFXLeafFaces = BSPData->GFXLeafFaces;

	SurfInfo = World->CurrentBSP->SurfInfo;

	Leaf = Plane_FindLeaf(World, GFXModels[0].RootNode[0], Pos);
	Area = GFXLeafs[Leaf].Area;

	// Check to see if we cen get rid of most of the work load by seeing if the leaf has not changed...
	if (World->CurrentLeaf == Leaf && !World->ForceVis)
		goto LeafDidNotChange;

	World->ForceVis = GE_FALSE;			// Reset force vis flag
	
	World->CurrentLeaf = Leaf;

	World->CurFrameStatic++;			// Make all old vis info obsolete

	Cluster = GFXLeafs[Leaf].Cluster;

	if (Cluster == -1 || GFXClusters[Cluster].VisOfs == -1)
	{
		World->VisInfo = GE_FALSE;
		return GE_TRUE;
	}

	if (Area)
		Vis_FloodAreas_r(World, Area);

	World->VisInfo = GE_TRUE;

	VisData = &GFXVisData[GFXClusters[Cluster].VisOfs];

	// Mark all visible clusters
	for (i=0; i<GFXModels[0].NumClusters; i++)
	{
		if (VisData[i>>3] & (1<<(i&7)) )
			World->CurrentBSP->ClusterVisFrame[i] = World->CurFrameStatic;
	}

	pLeaf = &GFXLeafs[GFXModels[0].FirstLeaf];

	// Go through and find all visible leafs based on the visible clusters the leafs are in
	for (i=0; i< GFXModels[0].NumLeafs; i++, pLeaf++)
	{
		int32	*pFace;

		Cluster = pLeaf->Cluster;

		if (Cluster == -1)		// No cluster info for this leaf (must be solid)
			continue;

		// If the cluster is not visible, then the leaf is not visible
		if (World->CurrentBSP->ClusterVisFrame[Cluster] != World->CurFrameStatic)
			continue;
		
		// If the area is not visible, then the leaf is not visible
		if (World->CurrentBSP->AreaVisFrame[pLeaf->Area] != World->CurFrameStatic)
			continue;

		// Mark all visible nodes by bubbling up the tree from the leaf
		MarkVisibleParents(World, i);

		// Mark the leafs vis frame to worlds current frame
		World->CurrentBSP->LeafData[i].VisFrame = World->CurFrameStatic;
			
		pFace = &GFXLeafFaces[pLeaf->FirstFace];

		// Go ahead and vis surfaces here...
		for (k=0; k< pLeaf->NumFaces; k++)
		{
			// Update each surface infos visframe thats touches each visible leaf
			SurfInfo[*pFace++].VisFrame = World->CurFrameStatic;
		}
	}

	LeafDidNotChange:

	// The world is always visible as a model
	World->CurrentBSP->Models[0].VisFrame = World->CurFrameDynamic;

	Models = &World->CurrentBSP->Models[1];
	// Do models, skipping world models (it's always visible)
	for (i=1; i< BSPData->NumGFXModels; i++, Models++)
	{
	#if 0
		int32		Cluster;

		// First, lets cheat, and see if the center is in a valid location to test for vis
		Leaf = Plane_FindLeaf(World, GFXModels[0].RootNode[0], &Models->Pivot);

		Cluster = GFXLeafs[Leaf].Cluster;

		if (Cluster >= 0 && GFXClusters[Cluster].VisOfs >= 0)		// If there is vis data for this leaf
		{
			if (World->CurrentBSP->LeafData[Leaf].VisFrame == World->CurFrameStatic)
				Models->VisFrame = World->CurFrameDynamic;
			else
			{
				GFX_Model	*pModel;

				pModel = &BSPData->GFXModels[i];

				if (pModel->Areas[0] == Area || pModel->Areas[1] == Area && 
					World->CurrentBSP->ClusterVisFrame[Cluster] == World->CurFrameStatic)
					Models->VisFrame = World->CurFrameDynamic;
			}
		}
		else if (ModelVisible(World, Models))
			Models->VisFrame = World->CurFrameDynamic;
	#else
		
		if (ModelVisible(World, Models))
			Models->VisFrame = World->CurFrameDynamic;
		
	#endif
		
		Models->ChangedFlags &= ~MODEL_CHANGED_XFORM;
	}
	
	VisFog(Engine, World, Camera, Fi, Area);

#ifdef _TSC
	showPopTSC("Vis_VisWorld");
#endif
	
	return GE_TRUE;
}

//=====================================================================================
//	Vis_MarkWaterFaces
//=====================================================================================
geBoolean Vis_MarkWaterFaces(World_BSP *WBSP)
{
	GFX_Leaf		*GFXLeafs;
	int32			i, f, FNum;
	int32			NumLeafs, Contents;
	Surf_SurfInfo	*SurfInfo;
	S32				*GFXLeafFaces;

	assert(WBSP != NULL);
	
	GFXLeafs = WBSP->BSPData.GFXLeafs;
	GFXLeafFaces = WBSP->BSPData.GFXLeafFaces;

	NumLeafs = WBSP->BSPData.GFXModels[0].NumLeafs;
	
	SurfInfo = WBSP->SurfInfo;

	assert(SurfInfo != NULL);

	for (i = 0; i< NumLeafs; i++)
	{
		Contents = GFXLeafs[i].Contents;

		if (!(Contents & GE_CONTENTS_WAVY))
			continue;

		for (f=0; f< GFXLeafs[i].NumFaces; f++)
		{
			FNum = GFXLeafFaces[GFXLeafs[i].FirstFace + f];

			// Disable for now, not working quite right...
			// There is a problem with the leafs...
			SurfInfo[FNum].Flags |= SURFINFO_WAVY;
		}
	}

	return GE_TRUE;
}

static GFX_Node		*GFXNodes;
static int32		*NodeParents;
static geWorld_Leaf	*LeafData;

//=====================================================================================
// FindParents_r
//=====================================================================================
static void FindParents_r(int32 Node, int32 Parent)
{
	if (Node < 0)		// At a leaf, mark leaf parent and return
	{
		LeafData[-(Node+1)].Parent = Parent;
		return;
	}

	// At a node, mark node parent, and keep going till hitting a leaf
	NodeParents[Node] = Parent;

	// Go down front and back markinf parents on the way down...
	FindParents_r(GFXNodes[Node].Children[0], Node);
	FindParents_r(GFXNodes[Node].Children[1], Node);
}

//=====================================================================================
//	Vis_FindParents
//=====================================================================================
static void FindParents(World_BSP *Bsp)
{
	assert(Bsp != NULL);
	
	// Assign some static globals so they don't flood the stack...
	GFXNodes = Bsp->BSPData.GFXNodes;
	LeafData = Bsp->LeafData;
	NodeParents = Bsp->NodeParents;

	FindParents_r(Bsp->BSPData.GFXModels[0].RootNode[0], -1);
}

//=====================================================================================
//	MarkVisibleParents
//=====================================================================================
static void MarkVisibleParents(geWorld *World, int32 Leaf)
{
	int32		Node;
	World_BSP	*Bsp;

	assert(Leaf >= 0);
	assert(Leaf < World->CurrentBSP->BSPData.NumGFXLeafs);

	Bsp = World->CurrentBSP;

	// Find the leafs parent
	Node = Bsp->LeafData[Leaf].Parent;

	// Bubble up the tree from the current node, marking them as visible
	while (Node >= 0)
	{
		Bsp->NodeVisFrame[Node] = World->CurFrameStatic;
		Node = Bsp->NodeParents[Node];
	}
}

// FIXME:  Put the fog in Fog.c
//=====================================================================================
//	VisFog
//=====================================================================================
static void VisFog(geEngine *Engine, geWorld *World, const geCamera *Camera, Frustum_Info *Fi, int32 Area)
{
	GBSP_BSPData	*BSPData;
	GFX_Leaf		*GFXLeafs;
	geFog			*Fog;
	World_BSP		*CBSP;
	Frustum_Info	WorldSpaceFrustum;
	//geVec3d			*Pos;

	assert(World);
	assert(Camera);
	assert(Fi);

	World->NumVisibleFog = 0;

	if (!World->FogList)
		return;				// Don't waste time

	//return;

	// Make the frustum go to world space
	Frustum_TransformToWorldSpace(Fi, Camera, &WorldSpaceFrustum);

	CBSP = World->CurrentBSP;
	
	BSPData = &CBSP->BSPData;

	GFXLeafs = BSPData->GFXLeafs;
	
	for (Fog = World->FogList; Fog; Fog = Fog->Next)
	{
		geWorld_FogData	*FogData;

		if (World->NumVisibleFog >= MAX_VISIBLE_FOG)
			return;		// Oh well...
		
		FogData = (geWorld_FogData*)geFog_GetUserData(Fog);

		if (CBSP->LeafData[FogData->Leaf].VisFrame != World->CurFrameStatic)
			continue;		// might not be visible

		if (Area != GFXLeafs[FogData->Leaf].Area)
			continue;		// Fog only effects it's own area

		// If not in the view frustum, then tottaly ignore...
		if (!Frustum_PointInFrustum(&WorldSpaceFrustum, &Fog->Pos, Fog->VolumeRadius-1.0f))
			continue;
		
		FogData->VisFrame = World->CurFrameDynamic;

		// Add it to the list
		World->VisibleFog[World->NumVisibleFog++] = Fog;
		
		// For debugging...
		Engine->DebugInfo.NumFog++;
	}
}
