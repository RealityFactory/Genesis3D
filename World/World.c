/****************************************************************************************/
/*  World.c                                                                             */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Code to render the world, and distribute work to other modules         */
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
#include <Math.h>

#include "World.h"
#include "System.h"
#include "Ram.h"
#include "BaseType.h"
#include "GBSPFile.h"
#include "Camera.h"
#include "Plane.h"
#include "Surface.h"
#include "Light.h"
#include "WBitmap.h"
#include "Frustum.h"
#ifdef	MESHES
#include "Mesh.h"
#endif
#include "Entities.h"
#include "Vis.h"
#include "User.h"
#include "VFile.h"

#include "Trace.h"

#include "list.h"

#include "Bitmap.h"
#include "Bitmap._h"

//#define BSP_BACK_TO_FRONT

//#define DO_ADDREMOVE_MESSAGES

#ifndef _DEBUG
#undef DO_ADDREMOVE_MESSAGES
#endif

//============================================================================
// Dirty HACKS that need to be removed
//============================================================================
#pragma message ("HACK!!! remove geCamera_FillDriverInfo (uses GlobalInfo)")

int32				MirrorRecursion;					// GLOBAL!!!

GInfo				GlobalInfo;
void				geCamera_FillDriverInfo(geCamera *Camera);

extern geVec3d		GlobalEyePos;

Frustum_Info		g_HackFrustum;
//============================================================================
//	**END** HACK section
//============================================================================
geBoolean geWorld_BitmapListInit(geWorld *World);
geBoolean geWorld_BitmapListShutdown(geWorld *World);
geBoolean geWorld_AddBitmap(geWorld *World, geBitmap *Bitmap);
geBoolean geWorld_RemoveBitmap(geWorld *World,geBitmap *Bitmap);

//=====================================================================================
//	Local Static Globals
//=====================================================================================
typedef struct
{
	geCamera			*Camera;
	Frustum_Info		*Frustum;
	geWorld_SkyBoxTData	*SkyTData;

} geWorld_RenderInfo;

static	geEngine			*CEngine = NULL;
static	geWorld				*CWorld = NULL;
static	World_BSP			*CBSP;
static	GBSP_BSPData		*BSPData = NULL;	// This is in globals, but is also kept here for speed
static	geBoolean			CanDoMirrors;
static  geWorld_DebugInfo	*CDebugInfo;
static	Frustum_Info		*CFrustumInfo;

static	Surf_SurfInfo		*pSurfInfo;
static	DRV_Driver			*RDriver;
static	geWorld_Model		*g_CurrentModel;

// Temp trans poly structure
#define MAX_TRANS_POLYS		256
#define MAX_CACHE_VERTS		16

#define TRANS_MIRROR		(1<<0)

typedef struct
{
	U32				Flags;
	S32				Face;						// Face this trans poly belongs too
	S32				NumVerts;
	DRV_TLVertex	TLVerts[MAX_CACHE_VERTS];	// Screen points
} World_TransPoly;

World_TransPoly		TransPolys[MAX_TRANS_POLYS];
S32					NumTransPolys[MAX_MIRROR_RECURSION+1];
S32					FirstTransPolys[MAX_MIRROR_RECURSION+1];

static void RenderTransPoly(geCamera *Camera, World_TransPoly *pPoly);

// GList.c
#define GLIST_MAX_OPERATIONS		1024

typedef struct
{
	uint8		Type;
	uint32		Data;
} GList_Operation;

typedef struct
{
	GList_Operation		GListOperations[GLIST_MAX_OPERATIONS];
	int32				NumGListOperations;
} GList;

GList_Operation		GListOperations[GLIST_MAX_OPERATIONS];
int32				FirstGListOperations[MAX_MIRROR_RECURSION+1];
int32				NumGListOperations[MAX_MIRROR_RECURSION+1];

//=====================================================================================
//=====================================================================================
GList *GList_Create(geEngine *Engine, geWorld *World)
{
	return NULL;
}

//=====================================================================================
//=====================================================================================
void GList_Destroy(GList *GList)
{
	assert(GList);
}

//=====================================================================================
//	GList_AddOperation
//=====================================================================================
void GList_AddOperation(uint8 Type, uint32 Data)
{
	int32		Op;

	if (NumGListOperations[MirrorRecursion] >= GLIST_MAX_OPERATIONS)
		return;		// Oh well...

	Op = FirstGListOperations[MirrorRecursion] + NumGListOperations[MirrorRecursion];

	GListOperations[Op].Type = Type;
	GListOperations[Op].Data = Data;

	NumGListOperations[MirrorRecursion]++;
}

//========================================================================================
// GList_RenderOperations
//========================================================================================
geBoolean GList_RenderOperations(geCamera *Camera)
{
	int32			i;

	// Render the list from back to front
	for (i=NumGListOperations[MirrorRecursion]-1; i>= 0; i--)
	{
		int32		Op;

		Op = FirstGListOperations[MirrorRecursion] + i;

		switch(GListOperations[Op].Type)
		{
			case 0:
				RenderTransPoly(Camera, (World_TransPoly*)GListOperations[Op].Data);
				break;
			case 1:
				User_RenderPolyList((gePoly*)GListOperations[Op].Data);
				break;

			default:
				assert(0);
		}
	}

	NumGListOperations[MirrorRecursion] = 0;

	return GE_TRUE;
}

//=====================================================================================
//	Local Static Functions
//=====================================================================================
static void CalcBSPModelInfo(World_BSP *BSP);
static geBoolean RenderScene(geEngine *Engine, geWorld *World, geCamera *Camera, Frustum_Info *FrustumInfo);
static void RenderBSPFrontBack_r(int32 Node, const geWorld_RenderInfo *RenderInfo, int32 ClipFlags);
static void RenderBSPFrontBackMirror_r(int32 Node, geCamera *Camera, Frustum_Info *Fi, int32 ClipFlags);
static void RenderFace(int32 Face, const geWorld_RenderInfo *RenderInfo, int32 ClipFlags);
static geBoolean RenderWorldModel(geCamera *Camera, Frustum_Info *FrustumInfo, geWorld_SkyBoxTData *SkyTData);
static geBoolean RenderSubModels(geCamera *Camera, Frustum_Info *FrustumInfo, geWorld_SkyBoxTData *SkyTData);
static geBoolean WorldSetGBSP(geWorld *World, World_BSP *BSP);
static World_BSP *CreateGBSP(geVFile *File);

static geBoolean CreateStaticFogList(geWorld *World);

// SkyBox functions
static		geBoolean BuildSkyBox(World_SkyBox *SkyBox, const GFX_SkyData *SkyData);
static void RenderSkyThroughFrustum(World_SkyBox *SkyBox, geWorld_SkyBoxTData *SkyTData, geCamera *Camera, Frustum_Info *Fi);
static void SetupSkyBoxFaceForScene(World_SkyBox *SkyBox, int32 Face, const geXForm3d *XForm, Frustum_Info *Fi, geWorld_SkyBoxTData *SkyTData);
static void SetupSkyForScene(World_SkyBox *SkyBox, geCamera *Camera, Frustum_Info *Fi, geWorld_SkyBoxTData *SkyTData);

//=====================================================================================
//	World_EngineInit
//=====================================================================================
geBoolean World_EngineInit(geEngine *Engine)
{
	return GE_TRUE;
}

//=====================================================================================
//	World_EngineShutdown
//=====================================================================================
void World_EngineShutdown(geEngine *Engine)
{
	CEngine = NULL;
	CWorld = NULL;
	BSPData = NULL;
}

//=====================================================================================
//	World_CreateFromBox
//	Fill in a world with a blank bsp, with one leaf
//=====================================================================================
World_BSP *World_CreateBSPFromBox(const geVec3d *Mins, const geVec3d *Maxs)
{
	World_BSP		*WorldBSP;
	GBSP_BSPData	*pBSP;
	GFX_Leaf		*pLeaf;
	GFX_Model		*pModel;
	GFX_Node		*pNode;
	int32			i;

	WorldBSP = GE_RAM_ALLOCATE_STRUCT(World_BSP);

	if (!WorldBSP)
	{
		geErrorLog_Add(GE_ERR_OUT_OF_MEMORY, NULL);
		return NULL;
	}

	memset(WorldBSP, 0, sizeof(World_BSP));

	pBSP = &WorldBSP->BSPData;

	memset(pBSP, 0, sizeof(GBSP_BSPData));

	pBSP->NumGFXModels = 1;
	pBSP->NumGFXNodes = 1;
	pBSP->NumGFXBNodes = 0;
	pBSP->NumGFXLeafs = 1;
	pBSP->NumGFXClusters = 1;
	pBSP->NumGFXAreas = 1;
	pBSP->NumGFXAreaPortals = 0;
	pBSP->NumGFXPlanes = 0;
	pBSP->NumGFXFaces = 0;
	pBSP->NumGFXLeafFaces = 0;
	pBSP->NumGFXLeafSides = 0;
	pBSP->NumGFXVerts = 0;
	pBSP->NumGFXVertIndexList = 0;

	pBSP->NumGFXEntData = 0;
	pBSP->NumGFXTextures = 0;
	pBSP->NumGFXTexInfo = 0;
	pBSP->NumGFXTexData = 0;
	pBSP->NumGFXPalettes = 0;
	
	pBSP->NumGFXLightData = 0;
	pBSP->NumGFXVisData = 0;
	pBSP->NumGFXPortals = 0;

	pBSP->GFXNodes = GE_RAM_ALLOCATE_ARRAY(GFX_Node, pBSP->NumGFXNodes);
	if (!pBSP->GFXNodes)
		return NULL;

	pBSP->GFXModels = GE_RAM_ALLOCATE_ARRAY(GFX_Model, pBSP->NumGFXModels);
	if (!pBSP->GFXModels)
		return NULL;

	pBSP->GFXLeafs = GE_RAM_ALLOCATE_ARRAY(GFX_Leaf, pBSP->NumGFXLeafs);
	if (!pBSP->GFXLeafs)
		return NULL;

	pBSP->GFXClusters = GE_RAM_ALLOCATE_ARRAY(GFX_Cluster, pBSP->NumGFXClusters);
	if (!pBSP->GFXClusters)
		return NULL;

	// Setup node 0 (point it to the only leaf in the level)
	pNode = &pBSP->GFXNodes[0];
	memset(pNode, 0, sizeof(*pNode));
	pNode->Children[0] = -1;
	pNode->Children[1] = -1;
	pNode->PlaneNum = -1;

	// Setup the worlds only model
	pModel = &pBSP->GFXModels[0];

	memset(pModel, 0, sizeof(GFX_Model));

	pModel->RootNode[0] = -1; 
	pModel->Mins = *Mins;
	pModel->Maxs = *Maxs;

	// Setup the leaf
	pLeaf = &pBSP->GFXLeafs[0];			
	memset(pLeaf, 0, sizeof(GFX_Leaf));

	pLeaf->Contents = 0;
	pLeaf->Mins = *Mins;
	pLeaf->Maxs = *Maxs;

	pLeaf->Cluster = 0;
	pLeaf->Area = 0;

	// Setup the cluster that the above leaf points to
	pBSP->GFXClusters[pLeaf->Cluster].VisOfs = -1;				// No visinfo for this cluster

	// Set up NULL sky
	for (i=0; i<6; i++)
		pBSP->GFXSkyData.Textures[i] = -1;

	return WorldBSP;
}

//=====================================================================================
//	geWorld_Create
//=====================================================================================
GENESISAPI geWorld *geWorld_Create(geVFile *File)
{
	geWorld			*NewWorld;
	int32			i;
	geWorld_Model	*Models;

	NewWorld = GE_RAM_ALLOCATE_STRUCT(geWorld);

	if (!NewWorld)
	{
		geErrorLog_Add(GE_ERR_OUT_OF_MEMORY, NULL);
		return NULL;
	}

	memset(NewWorld, 0, sizeof(geWorld));

	// Create a ref on the world now, so if there is an error, it will free what is in the world...
	geWorld_CreateRef(NewWorld);

	if ( ! List_Start() )
		goto Error;

	if (!File)
	{
		geVec3d	TMins = {-1000.0f, -1000.0f, -1000.0f};
		geVec3d	TMaxs = { 1000.0f,  1000.0f,  1000.0f};

		NewWorld->CurrentBSP = World_CreateBSPFromBox(&TMins, &TMaxs);
	}
	else
	{
		assert(File != NULL);

		NewWorld->CurrentBSP = CreateGBSP(File);
	}

	// The world has changed
	NewWorld->Changed = GE_TRUE;

	if (!NewWorld->CurrentBSP)
		goto Error;

	assert(NewWorld->CurrentBSP->BSPData.NumGFXLeafs > 0);

	// Create the leafdata array
	NewWorld->CurrentBSP->LeafData = GE_RAM_ALLOCATE_ARRAY(geWorld_Leaf, NewWorld->CurrentBSP->BSPData.NumGFXLeafs);

	if (!NewWorld->CurrentBSP->LeafData)
		goto Error;

	memset(NewWorld->CurrentBSP->LeafData, 0, sizeof(geWorld_Leaf)*NewWorld->CurrentBSP->BSPData.NumGFXLeafs);

	if (!Light_WorldInit(NewWorld))
		goto Error;

	if (!Ent_WorldInit(NewWorld))
		goto Error;

	if (!Vis_WorldInit(NewWorld))
		goto Error;

	if (!Surf_WorldInit(NewWorld))
		goto Error;

	// Create the wbitmaps out of the GFXTexData
	NewWorld->CurrentBSP->WBitmapPool = geWBitmap_Pool_Create(&NewWorld->CurrentBSP->BSPData);

	if (!NewWorld->CurrentBSP->WBitmapPool)
		goto Error;

	#if 1
		// HACK
		// We can now free the texturedata in the BSP that was loaded off disk.
		// Eventually, the BSP disk format will be bitmaps, and no conversion will be needed, JP.
		if (NewWorld->CurrentBSP->BSPData.GFXTexData)	// Not all worlds have tex data!!!
		{
			geRam_Free(NewWorld->CurrentBSP->BSPData.GFXTexData);
			NewWorld->CurrentBSP->BSPData.GFXTexData = NULL;	// This is to assure that FreeGBSPFile does not touch this again
			NewWorld->CurrentBSP->BSPData.NumGFXTexData = 0;
		}
	#endif

	// Add all the bitmaps in the WBitmapPool to the world
	if (!geWorld_BitmapListInit(NewWorld))
	{
		geErrorLog_AddString(-1, "geWorld_WorldCreate:  geWorld_BitmapListInit failed.", NULL);
		return GE_FALSE;
	}

	// Init user stuff
	if (!User_WorldInit(NewWorld))
		goto Error;
	
	Models = NewWorld->CurrentBSP->Models;

	//#pragma message ("Fixed number of models supported")
	for (i=0; i< MAX_MODELS; i++)
	{
		memset(&Models[i], 0, sizeof(geWorld_Model));

		Models[i].VisFrame = -1;
		
		geXForm3d_SetIdentity(&Models[i].XForm);
	}
	
	CalcBSPModelInfo(NewWorld->CurrentBSP);

	if (!BuildSkyBox(&NewWorld->SkyBox, &NewWorld->CurrentBSP->BSPData.GFXSkyData))
		goto Error;

	NewWorld->CurrentLeaf = -1;			// Make sure the level gets vised for the first time...

	NewWorld->ActorCount = 0;
	NewWorld->ActorArray = NULL;

	if (!CreateStaticFogList(NewWorld))
	{
		geErrorLog_AddString(-1,"Failed to create static FogList", NULL);
		goto Error;
	}

	return NewWorld;

	Error:;
		geWorld_Free(NewWorld);

	return NULL;
}

//=====================================================================================
//	geWorld_Free
//=====================================================================================
GENESISAPI void geWorld_Free(geWorld *World)
{
	int				i;
	geFog			*Fog, *Next;

	assert(World);
	assert(World->RefCount > 0);

	World->RefCount--;

	if (World->RefCount > 0)
		return;			// No need to destroy till ref count goes to zero...

if (World->CurrentBSP)
{
	// Shutdown actors
	if (World->ActorCount>0)
		{
			assert( World->ActorArray != NULL );
			for (i=0; i< World->ActorCount; i++)
				{
					geActor_Destroy( &( World->ActorArray[i].Actor ) );
				}
			World->ActorCount = 0;
	}
	if (World->ActorArray != NULL)
		{
			geRam_Free( World->ActorArray );
			World->ActorArray = NULL;
		}
	
	assert( World->ActorArray == NULL );
	
	// Call other modules to release info from the world that they created...
#ifdef	MESHES
	Mesh_WorldShutdown(World);
#endif
	Light_WorldShutdown(World);
	Ent_WorldShutdown(World);
	Vis_WorldShutdown(World);
	Surf_WorldShutdown(World);

	User_WorldShutdown(World);

	if (World->CurrentBSP->WBitmapPool)
	{
		geWBitmap_Pool_Destroy(World->CurrentBSP->WBitmapPool);
		World->CurrentBSP->WBitmapPool = NULL;
	}

	// Make sure we free all the fog
	for (Fog = World->FogList; Fog; Fog = Next)
	{
		geWorld_FogData		*FogData;

		Next = Fog->Next;

		FogData = (geWorld_FogData*)geFog_GetUserData(Fog);

		if (FogData)
			geRam_Free(FogData);

		geFog_SetUserData(Fog, NULL);		// Just in case...

		geFog_Destroy(Fog);
	}

	// Free the leaf data array in the world
	if (World->CurrentBSP->LeafData)
	{
		int32			l;
		geWorld_Leaf	*pLeafData;
		
		pLeafData = World->CurrentBSP->LeafData;

		for (l=0; l< World->CurrentBSP->BSPData.NumGFXLeafs; l++, pLeafData++)
		{
			if (pLeafData->PolyList)
			{
				User_DestroyPolyList(World, pLeafData->PolyList);
				pLeafData->PolyList = NULL;
			}
		}
	
		geRam_Free(World->CurrentBSP->LeafData);
		World->CurrentBSP->LeafData = NULL;
	}

	GBSP_FreeGBSPFile(&World->CurrentBSP->BSPData);
	geRam_Free(World->CurrentBSP);

	// Shutdown the bitmaplist	(this should be done last, to give others a chance to remove their bitmaps)
	geWorld_BitmapListShutdown(World);
}

	geRam_Free(World);

	List_Stop();
}

//================================================================================
//	geWorld_CreateRef
//================================================================================
geBoolean geWorld_CreateRef(geWorld *World)
{
	World->RefCount++;

	return GE_TRUE;
}

//=====================================================================================
//	World_SetEngine
//	Lets this module and all its children know that the engine has changed
//=====================================================================================
geBoolean World_SetEngine(geEngine *Engine)
{
	assert (Engine != NULL);

	CEngine = Engine;

	// Let all sub modules know what's going on...
	if (!Light_SetEngine(Engine))
		return GE_FALSE;
	if (!Plane_SetEngine(Engine))
		return GE_FALSE;
	if (!Surf_SetEngine(Engine))
		return GE_FALSE;

	return GE_TRUE;
}

//=====================================================================================
//	World_SetWorld
//	Lets this module (and all of its children) know that the world has changed
//=====================================================================================
geBoolean World_SetWorld(geWorld *World)
{
	assert(World != NULL);
	
	CWorld = World;

	// Let all sub modules know what's going on...
	if (!Light_SetWorld(World))
		return GE_FALSE;
	if (!Plane_SetWorld(World))
		return GE_FALSE;
	if (!Surf_SetWorld(World))
		return GE_FALSE;

	return GE_TRUE;
}

//=====================================================================================
//	World_SetGBSP
//	Tells each module bsp data they are dealing with, and lets them update
//	various pointers to the bsp, in their local statics
//=====================================================================================
geBoolean World_SetGBSP(World_BSP *BSP)
{
	assert(BSP != NULL);

	// Make quick pointer to the world bsp data
	CBSP = BSP;
	BSPData = &BSP->BSPData;

	// Let all sub modules know what's going on...
	if (!Light_SetGBSP(BSP))
		return GE_FALSE;
	if (!Plane_SetGBSP(BSP))
		return GE_FALSE;

	if (!Surf_SetGBSP(BSP))
		return GE_FALSE;

	return GE_TRUE;
}

//=====================================================================================
//	SetupStaticData
//=====================================================================================
void SetupStaticData(void)
{
	pSurfInfo = CBSP->SurfInfo;
	RDriver = CEngine->DriverInfo.RDriver;
}

//=====================================================================================
//	World_RenderQ
//	Render the worlds render Q using the supplied engine, world, and camera
//=====================================================================================
geBoolean World_WorldRenderQ(geEngine *Engine, geWorld *World, geCamera *Camera)
{
	Frustum_Info		FrustumInfo;
	geFloat				Rpm;
	World_SkyBox		*pSkyBox;

	assert(Engine != NULL);
	assert(World != NULL);
	assert(Camera!= NULL);

	World->CurFrameDynamic++;

	MirrorRecursion = 0;
	NumTransPolys[MirrorRecursion] = 0;
	NumGListOperations[MirrorRecursion] = 0;
	
	// Clear the debug info for this world
	//memset(&World->DebugInfo, 0, sizeof(geWorld_DebugInfo));
	CDebugInfo = &World->DebugInfo;

	// Setup modules that need info that we don't want to lug around with us...
	// NOTE - This might screw up multi-threading, so it might need to be changed
	// around a little to work with multi-threading
	World_SetEngine(Engine);
	World_SetWorld(World);
	World_SetGBSP(World->CurrentBSP);
	
	// FIXME:  REMOVE!!!!!
	GlobalEyePos = *geCamera_GetPov(Camera);

	// Setup some globals for this pass (this must be done before anyone uses them, or KABOOM)
	SetupStaticData();

	CFrustumInfo = &FrustumInfo;

	// Se if we can do mirrors with this driver
	CanDoMirrors = (RDriver->EngineSettings->PreferenceFlags & DRV_PREFERENCE_NO_MIRRORS)==0;

	// Setup the View Frustum to default window from the camera
	Frustum_SetFromCamera(&FrustumInfo, Camera);

	// Have the Vis module setup all vising info
	Vis_VisWorld(Engine, World, Camera, &FrustumInfo);

	// Setup the dynamic lights, etc...
	if (!Light_SetupLights(World))
		return GE_FALSE;

	g_HackFrustum = FrustumInfo;

	// Render the entire scene through the DEFAULT FRUSTUM
	if (!RenderScene(Engine, World, Camera, &FrustumInfo))
		return GE_FALSE;

	// Adjust current sky angle 
	pSkyBox = &World->SkyBox;
	Rpm = (pSkyBox->Dpm/180.0f)*3.14159f;		// Get radiuns per minute
	pSkyBox->Angle += Rpm*(1/30.0f);			// Assume 30 fps for now :)

	// Little hack to flush the scene
	RDriver->BeginModels();
	RDriver->EndModels();

	if (!User_DestroyOncePolys(World))
		return GE_FALSE;

#if 1
	// <> CB remember the last camera we rendered with,
	//	so we can avoiding redoing view-dependent calculation
	World->LastCameraXForm = * geCamera_GetCameraSpaceXForm(Camera);
#endif

	return GE_TRUE;
}


GENESISAPI geBoolean GENESISCC geWorld_IsActorPotentiallyVisible(const geWorld *World, const geActor *Actor, const geCamera *Camera)
		// if the actor doesn't have a render hint box, we assume it's potentially visible
{
	#pragma message ("This is a fairly poor test: ")
		// mirrors aren't checked.
		// this doesn't check the extents of the actor, just the center point.
		// if the render hint box isn't set, should this return true, or find the DynamicExtBox?
	geExtBox		Box;
	geBoolean		Enabled;
	geVec3d			Center;
	const geXForm3d	*CameraTransform;
	int32			Leaf,CameraLeaf;
		
	assert( World != NULL );
	assert( geActor_IsValid(Actor)!= GE_FALSE );
	assert( Camera != NULL );

	geActor_GetRenderHintExtBox(Actor,&Box,&Enabled);
	if (Enabled == GE_FALSE)
		return GE_TRUE;
	geExtBox_GetTranslation ( &Box, &Center );
	// NOTE - We are not taking into acount that a actor may live in more than one leaf...
	geWorld_GetLeaf(World, &Center, &Leaf);

	CameraTransform = geCamera_GetWorldSpaceVisXForm( Camera );
	geWorld_GetLeaf(World, &(CameraTransform->Translation), &CameraLeaf);

	#pragma message ("geWorld_MightSeeLeaf would be WAY FASTER here, but would be a frame behind...")
	if (geWorld_LeafMightSeeLeaf(World, Leaf, CameraLeaf, 0 )==GE_FALSE)
		return GE_FALSE;

	{
		  // perhaps this should be a 'geCamera_IsExtBoxOnScreen(Camera,Box)' function?
		// see if the hint box is visible on the screen.  
		// (transform and project it to the screen, then check extents of that projection
		//  against the clipping rect)
		#pragma message ("This should use frustum in world space, and use Trace_BoxOnPlaneSides with frustum planes...")
		geRect ClippingRect;
		geVec3d BoxCorners[8];
		const geXForm3d *ObjectToCamera;
		geVec3d Maxs,Mins;
		#define BIG_NUMBER (99e9f)  
		int i;

		geCamera_GetClippingRect(Camera,&ClippingRect);
		BoxCorners[0] = Box.Min;
		BoxCorners[1] = BoxCorners[0];  BoxCorners[1].X = Box.Max.X;
		BoxCorners[2] = BoxCorners[0];  BoxCorners[2].Y = Box.Max.Y;
		BoxCorners[3] = BoxCorners[0];  BoxCorners[3].Z = Box.Max.Z;
		BoxCorners[4] = Box.Max;
		BoxCorners[5] = BoxCorners[4];  BoxCorners[5].X = Box.Min.X;
		BoxCorners[6] = BoxCorners[4];  BoxCorners[6].Y = Box.Min.Y;
		BoxCorners[7] = BoxCorners[4];  BoxCorners[7].Z = Box.Min.Z;

		ObjectToCamera = geCamera_GetCameraSpaceXForm(Camera);
		assert( ObjectToCamera );

		geVec3d_Set(&Maxs,-BIG_NUMBER,-BIG_NUMBER,-BIG_NUMBER);
		geVec3d_Set(&Mins, BIG_NUMBER, BIG_NUMBER, BIG_NUMBER);
		for (i=0; i<8; i++)
			{
				geVec3d V;
				geXForm3d_Transform(  ObjectToCamera,&(BoxCorners[i]),&(BoxCorners[i]));
				geCamera_Project(  Camera,&(BoxCorners[i]),&V);
				if (V.X > Maxs.X ) Maxs.X = V.X;
				if (V.X < Mins.X ) Mins.X = V.X;
				if (V.Y > Maxs.Y ) Maxs.Y = V.Y;
				if (V.Y < Mins.Y ) Mins.Y = V.Y;
				if (V.Z > Maxs.Z ) Maxs.Z = V.Z;
				if (V.Z < Mins.Z ) Mins.Z = V.Z;
			}

		if (   (Maxs.X < ClippingRect.Left) 
			|| (Mins.X > ClippingRect.Right)
			|| (Maxs.Y < ClippingRect.Top) 
			|| (Mins.Y > ClippingRect.Bottom)
			|| (Maxs.Z < 0.0f) )
			{
				return GE_FALSE;
			}
	}

	return GE_TRUE;

}


//=====================================================================================
//	RenderScene
//	This can be recursivly re-entered
//=====================================================================================
static geBoolean RenderScene(geEngine *Engine, geWorld *World, geCamera *Camera, Frustum_Info *FrustumInfo)
{
	geWorld_SkyBoxTData		SkyTData;

	if (MirrorRecursion > 0)
	{
		FirstTransPolys[MirrorRecursion] = NumTransPolys[MirrorRecursion-1];
		FirstGListOperations[MirrorRecursion] = NumGListOperations[MirrorRecursion-1];
	}
	else
	{
		FirstTransPolys[MirrorRecursion] = 0;
		FirstGListOperations[MirrorRecursion] = 0;
	}

	NumTransPolys[MirrorRecursion] = 0;
	NumGListOperations[MirrorRecursion] = 0;

	memset(&SkyTData, 0, sizeof(SkyTData));

	//
	// Setup the sky for this scene
	//
	SetupSkyForScene(&World->SkyBox, Camera, FrustumInfo, &SkyTData);

	//
	// Render the world...
	//
	if (!RenderWorldModel(Camera, FrustumInfo, &SkyTData))
		return GE_FALSE;

	//
	// Then render the Sub models of the world
	//
	if (!RenderSubModels(Camera, FrustumInfo, &SkyTData))
		return GE_FALSE;

	//
	//	Render the actors
	//
	{	
		int i;
		World_Actor		*WActor;
		//geXForm3d		XForm;
		geVec3d			Center;
		int32			Leaf;
		Frustum_Info	ActorFrustum;

		// Make the frustum go to world space for actors
		Frustum_TransformToWorldSpace(FrustumInfo, Camera, &ActorFrustum);

		// Tell the driver we want to render meshes
		if (!Engine->DriverInfo.RDriver->BeginMeshes())
		{
			geErrorLog_Add(GE_ERR_BEGIN_MESHES_FAILED, NULL);
			return GE_FALSE;
		}

		// We were using the actor array alot, so I though I'd move it out...
		// There were also going to be a lot of nested if's, so they are continues now...
		WActor = World->ActorArray;

		for (i=0; i< World->ActorCount; i++, WActor++)
			{
				if (MirrorRecursion == 0 && !(WActor->Flags & (GE_ACTOR_RENDER_NORMAL | GE_ACTOR_RENDER_ALWAYS)))
					continue;		// Not visible in normal views, skip it
				if (MirrorRecursion > 0 && !(WActor->Flags & (GE_ACTOR_RENDER_MIRRORS | GE_ACTOR_RENDER_ALWAYS)))
					continue;		// Not visible in mirros, skip it

				{
					geExtBox Box;
					geBoolean Enabled;
					geActor_GetRenderHintExtBox(WActor->Actor,&Box,&Enabled);
					if (Enabled == GE_TRUE)
						{
							geExtBox_GetTranslation ( &Box, &Center );
							if	(!(WActor->Flags & GE_ACTOR_RENDER_ALWAYS))
								{
									// NOTE - We are not taking into acount that a actor may live in more than one leaf...
									geWorld_GetLeaf(World, &Center, &Leaf);
					
									if (World->CurrentBSP->LeafData[Leaf].VisFrame != World->CurFrameStatic)
										continue;		// Not in PVS, skip it
								}
						}
				}

				if (MirrorRecursion == 0)
				{
					geActor_Render( WActor->Actor, Engine, World, Camera);
					// For debugging...
					//  This is set inside Actor/Puppet.  Engine->DebugInfo.NumActors++;
				}
				else
				{
					geActor_RenderThroughFrustum( WActor->Actor, Engine, World, Camera, &ActorFrustum);
					// For debugging...
					Engine->DebugInfo.NumActors++;
				}

			}

		if (!Engine->DriverInfo.RDriver->EndMeshes())
		{
			geErrorLog_Add(GE_ERR_END_MESHES_FAILED, NULL);
			return GE_FALSE;
		}
	}

	// Hack...  Must restore the camera hack for trans world polys
	geCamera_FillDriverInfo(Camera);
	
	// Setup the user stuff with the world for this scene
	if (!User_SetCameraInfo(Engine, World, Camera, FrustumInfo))
		return GE_FALSE;

	// Render all the translucent polys last (on top of everything)....
	if (!GList_RenderOperations(Camera))
		return GE_FALSE;

	return GE_TRUE;
}

//=====================================================================================
//	RenderBSPFrontBack_r2
//	Fast traverser, that only traverses to visible leafs, nothing else.
//=====================================================================================
static void RenderBSPFrontBack_r2(int32 Node, geCamera *Camera)
{
	float			Dist1;
	GFX_Node		*pNode;
	int32			Side;

	if (Node < 0)		// At leaf, no more recursing
	{
		int32		Leaf;
		gePoly		*PolyList;

		Leaf = -(Node+1);

		assert(Leaf >= 0 && Leaf < CWorld->CurrentBSP->BSPData.NumGFXLeafs);

		PolyList = CWorld->CurrentBSP->LeafData[Leaf].PolyList;

		if (PolyList)
		{
			CDebugInfo->NumLeafsWithUserPolys++;
			GList_AddOperation(1, (uint32)PolyList);
		}

		CDebugInfo->NumLeafsHit2++;
		
		return;
	}

	if (CBSP->NodeVisFrame[Node] != CWorld->CurFrameStatic)		
	{
		if (CWorld->VisInfo)
			return;
	}
	
	CDebugInfo->NumNodesTraversed2++;

	pNode = &BSPData->GFXNodes[Node];
	
	// Get the distance that the eye is from this plane
	Dist1 = Plane_PlaneDistanceFast(&BSPData->GFXPlanes[pNode->PlaneNum], geCamera_GetPov(Camera));

	if (Dist1 < 0)
		Side = 1;
	else
		Side = 0;
	
	// Go down the side we are on first, then the other side
	RenderBSPFrontBack_r2(pNode->Children[Side], Camera);
	RenderBSPFrontBack_r2(pNode->Children[!Side], Camera);
}

//=====================================================================================
//	RenderBSPFrontBack_r
//=====================================================================================
static void RenderBSPFrontBack_r(int32 Node, const geWorld_RenderInfo *RenderInfo, int32 ClipFlags)
{
	float			Dist1;
	int32			i;
	int32			k, f, Side;
	GFX_Node		*pNode;
	GFX_Face		*pFace;
	Surf_SurfInfo	*pSurfInfo2;

	if (Node < 0)		// At leaf, no more recursing
	{
		int32		Leaf;
		gePoly		*PolyList;

		Leaf = -(Node+1);

		assert(Leaf >= 0 && Leaf < CWorld->CurrentBSP->BSPData.NumGFXLeafs);

		PolyList = CWorld->CurrentBSP->LeafData[Leaf].PolyList;

		if (PolyList)
		{
			CDebugInfo->NumLeafsWithUserPolys++;
			GList_AddOperation(1, (uint32)PolyList);
		}

		CDebugInfo->NumLeafsHit1++;

		return;
	}

	if (CBSP->NodeVisFrame[Node] != CWorld->CurFrameStatic)		
	{
		if (CWorld->VisInfo)
			return;
	}
	
	CDebugInfo->NumNodesTraversed1++;

	pNode = &BSPData->GFXNodes[Node];
	
	if (ClipFlags)	
	{
		float			*MinMaxs;
		int32			*Index;
		float			Dist;
		geVec3d			Pnt;
		GFX_Plane		*Planes;
		Frustum_Info	*Fi;

		Fi = RenderInfo->Frustum;

		MinMaxs = (float*)&pNode->Mins;
		Planes = Fi->Planes;

		for (k=0; k< Fi->NumPlanes; k++)
		{
			if (!(ClipFlags & (1<<k)) )
				continue;
			
			Index = Fi->pFrustumBBoxIndexes[k];

			Pnt.X = MinMaxs[Index[0]];
			Pnt.Y = MinMaxs[Index[1]];
			Pnt.Z = MinMaxs[Index[2]];
			
			Dist = geVec3d_DotProduct(&Pnt, &Planes[k].Normal);
			Dist -= Fi->Planes[k].Dist;

			if (Dist <= 0)
			{
				// We have no more visible nodes from this POV, so just traverse to leafs from here
				RenderBSPFrontBack_r2(Node, RenderInfo->Camera);
				return;
			}

			Pnt.X = MinMaxs[Index[3+0]];
			Pnt.Y = MinMaxs[Index[3+1]];
			Pnt.Z = MinMaxs[Index[3+2]];

			Dist = geVec3d_DotProduct(&Pnt, &Planes[k].Normal);
			Dist -= Planes[k].Dist;

			if (Dist >= 0)		
				ClipFlags &= ~(1<<k);		// Don't need to clip to this plane anymore
		}
	}
	
	// Get the distance that the eye is from this plane
	Dist1 = Plane_PlaneDistanceFast(&BSPData->GFXPlanes[pNode->PlaneNum], geCamera_GetPov(RenderInfo->Camera));

	pSurfInfo2 = &pSurfInfo[pNode->FirstFace];
	pFace = &BSPData->GFXFaces[pNode->FirstFace];

	if (Dist1 < 0)
		Side = 1;		// Back side first
	else
		Side = 0;		// Front side first

	// Render the side of the node we are on first
	RenderBSPFrontBack_r(pNode->Children[Side], RenderInfo, ClipFlags);
		
	// Setup the global driver info about this plane (all the faces share it for this run)
	// FIXME:  Software driver needs to calculate gradients from uv's, so we can QUIT doing this here...
	GlobalInfo.PlaneNormal = BSPData->GFXPlanes[pNode->PlaneNum].Normal;
	GlobalInfo.PlaneDist = BSPData->GFXPlanes[pNode->PlaneNum].Dist;
	geXForm3d_Rotate(geCamera_GetCameraSpaceXForm(RenderInfo->Camera), &GlobalInfo.PlaneNormal, &GlobalInfo.RPlaneNormal);

	// Render faces on this node
	for (i = 0; i < pNode->NumFaces; i++, pSurfInfo2++, pFace++)
	{
		f = i + pNode->FirstFace;
			
		if (pSurfInfo2->VisFrame != CWorld->CurFrameStatic && CWorld->VisInfo)
			continue;
		
		if (pFace->PlaneSide != Side)
			continue;
		
		CEngine->DebugInfo.TraversedPolys++;
		RenderFace(f, RenderInfo, ClipFlags);
	}

	// Render faces on the other side of the node
	RenderBSPFrontBack_r(pNode->Children[!Side], RenderInfo, ClipFlags);
}

void GENESISCC geXForm3d_SetLeft(geXForm3d *M, const geVec3d *Left)
	// Gets a vector that is 'left' in the frame of reference of M (facing -Z)
{
	assert( M     != NULL );
	assert( Left != NULL );
	
	M->AX = -Left->X;
	M->BX = -Left->Y;
	M->CX = -Left->Z;
}

void GENESISCC geXForm3d_SetUp(geXForm3d *M, const geVec3d *Up)
	// Gets a vector that is 'up' in the frame of reference of M (facing -Z)
{
	assert( M  != NULL );
	assert( Up != NULL );
	
	M->AY = Up->X;
	M->BY = Up->Y;
	M->CY = Up->Z;
}

void GENESISCC geXForm3d_SetIn(geXForm3d *M,const geVec3d *In)
	// Gets a vector that is 'in' in the frame of reference of M (facing -Z)
{
	assert( M    != NULL );
	assert( In != NULL );

	M->AZ = -In->X;
	M->BZ = -In->Y;
	M->CZ = -In->Z;
}

//================================================================================
//	BackRotateVector
//	Rotate a vector from viewspace to worldspace.
//================================================================================
static void BackRotateVector(const geVec3d *In, geVec3d *Out, const geXForm3d *XForm)
{
    geVec3d	VRight, VUp, VIn;
	
	//	Get the 3 vectors that make up the Xform axis 
	VRight.X = XForm->AX; VRight.Y = XForm->AY; VRight.Z = XForm->AZ;
	VUp.X    = XForm->BX; VUp.Y    = XForm->BY; VUp.Z    = XForm->BZ;
	VIn.X    = XForm->CX; VIn.Y    = XForm->CY; VIn.Z    = XForm->CZ;

    Out->X = (In->X * VRight.X) + (In->Y * VUp.X) + (In->Z * VIn.X);
    Out->Y = (In->X * VRight.Y) + (In->Y * VUp.Y) + (In->Z * VIn.Y);
    Out->Z = (In->X * VRight.Z) + (In->Y * VUp.Z) + (In->Z * VIn.Z);
}

#define GOURAUD_SHADING

//=====================================================================================
//	RenderFace
//=====================================================================================
static void RenderFace(int32 Face, const geWorld_RenderInfo *RenderInfo, int32 ClipFlags)
{
	geVec3d				Dest1[MAX_RENDERFACE_VERTS], Dest2[MAX_RENDERFACE_VERTS];
	geVec3d				*pDest1, *pDest2;
	Surf_TexVert		Tex1[MAX_RENDERFACE_VERTS], Tex2[MAX_RENDERFACE_VERTS];
	Surf_TexVert		*pTex1, *pTex2;
	DRV_TLVertex		Clipped1[MAX_RENDERFACE_VERTS];
	geVec3d				*pGFXVerts;
	int32				Length1, Length2;
	int32				i, p;
	int32				*pIndex;
	int32				TexFlags;
	int32				NumVerts;
	geBitmap			*pBitmap;
	GFX_Face			*pFace;
	GFX_TexInfo			*pTexInfo;
	const geXForm3d		*CXForm;
	GFX_Plane			*FPlanes;
	uint32				RenderFlags;
	DRV_TexInfo			DrvTexInfo;
	geWBitmap			*pWBitmap;
	geRDriver_THandle	*THandle;
	Frustum_Info		*Fi;
	geCamera			*Camera;

	Fi = RenderInfo->Frustum;
	Camera = RenderInfo->Camera;

	if (pSurfInfo[Face].LInfo.Face == -1)
		return;

	pFace = &BSPData->GFXFaces[Face];
	pGFXVerts = BSPData->GFXVerts;

	NumVerts = pFace->NumVerts;

	assert(NumVerts < MAX_RENDERFACE_VERTS);

	pTexInfo = &BSPData->GFXTexInfo[pFace->TexInfo];
	TexFlags = pTexInfo->Flags;
	
	pDest1 = Dest1;
	pIndex = &BSPData->GFXVertIndexList[pFace->FirstVert];

	if (pSurfInfo[Face].Flags & SURFINFO_WAVY)
	{
		for (i = 0; i < NumVerts; i++)
		{
			int32 Offs1, Offs2;

			// HACK 
			Offs1 = (CEngine->WaveTable[*pIndex & 15]-75) / 25;
			Offs2 = (CEngine->WaveTable[*pIndex & 15]-75) / 20;

			pDest1->X = pGFXVerts[*pIndex].X + Offs1;
			pDest1->Y = pGFXVerts[*pIndex].Y;
			pDest1->Z = pGFXVerts[*pIndex].Z + Offs2;
			pDest1++;
			pIndex++;
		}
	}
	else 
	{
		for (i = 0; i < NumVerts; i++)
		{
			pDest1->X = pGFXVerts[*pIndex].X;
			pDest1->Y = pGFXVerts[*pIndex].Y;
			pDest1->Z = pGFXVerts[*pIndex].Z;
			pDest1++;
			pIndex++;
		}
	}

	pDest1 = Dest1;
	pDest2 = Dest2;
	pTex1 = &CBSP->TexVerts[pFace->FirstVert];
	pTex2 = Tex2;
	Length1 = NumVerts;

	FPlanes = Fi->Planes;

#if 0		// Test
	//
	//	Apply any fog to the faces verts
	//
	TexFlags |= TEXINFO_GOURAUD;
	pTexInfo->Flags |= TEXINFO_NO_LIGHTMAP;

	for (i=0; i< Length1; i++)
	{
		pTex1[i].r = 20.0f;
		pTex1[i].g = 20.0f;
		pTex1[i].b = 20.0f;
	}
	for (i=0; i<CWorld->NumVisibleFog; i++)
	{
		Light_FogVerts(CWorld->VisibleFog[i], geCamera_GetPov(Camera), pDest1, pTex1, Length1);
	}
	for (i=0; i< Length1; i++)
	{
		if (pTex1[i].r > 255.0f)
			pTex1[i].r = 255.0f;
		if (pTex1[i].g > 255.0f)
			pTex1[i].g = 255.0f;
		if (pTex1[i].b > 255.0f)
			pTex1[i].b = 255.0f;
	}
#endif

	if (ClipFlags)
	{
		for (p=0; p< Fi->NumPlanes; p++)
		{
			// Only do clipping if we have to
			if (!(ClipFlags & (1<<p)) )
				continue;

			if (TexFlags & TEXINFO_GOURAUD)
			{
				if (!Frustum_ClipToPlaneUVRGB(&FPlanes[p], pDest1, pDest2, pTex1, pTex2, Length1, &Length2))
					return;
			}
			else
			{
				if (!Frustum_ClipToPlaneUV(&FPlanes[p], pDest1, pDest2, pTex1, pTex2, Length1, &Length2))
					return;
			}

			if (pDest1 == Dest2)
			{
				pDest1 = Dest1;
				pDest2 = Dest2;
				pTex1 = Tex1;
				pTex2 = Tex2;
			}
			else
			{
				pDest1 = Dest2;
				pDest2 = Dest1;
				pTex1 = Tex2;
				pTex2 = Tex1;
			}
			Length1 = Length2;
		}
	}
	  
	if (Length1 < 3)
		return;			// Poly was clipped away

	// This bitmap is being used, so set its vis frame to the worlds...
	pWBitmap = geWBitmap_Pool_GetWBitmapByIndex(CBSP->WBitmapPool, pTexInfo->Texture);
	assert(pWBitmap);
	geWBitmap_SetVisFrame(pWBitmap, CWorld->CurFrameDynamic);

	//
	// Get the camera XForm
	//
	CXForm = geCamera_GetCameraSpaceXForm(Camera);
	
	//
	// Transform the array of verts
	//
	geXForm3d_TransformArray(CXForm, pDest1, pDest2, Length1);
	
	if (TexFlags & TEXINFO_SKY)		// If this is a sky face, then render the sky through it, and return
	{
		Frustum_Info	SkyFrustum;

		// Create a frustum from the poly
		Frustum_SetFromPoly(&SkyFrustum, pDest2, Length1, MirrorRecursion&1);

		// Render the sky through the poly's frustum
		RenderSkyThroughFrustum(&CWorld->SkyBox, RenderInfo->SkyTData, Camera, &SkyFrustum);
		return;		// Once the sky was rendered through the face, return
	}
	else if (TexFlags & TEXINFO_GOURAUD)
	{
		Frustum_ProjectRGB(pDest2, pTex1, Clipped1, Length1, Camera);
	}
	else
	{
		Frustum_Project(pDest2, pTex1, Clipped1, Length1, Camera);

		for (i=0; i< Length1; i++)
		{
			Clipped1[i].r = 255.0f;
			Clipped1[i].g = 255.0f;
			Clipped1[i].b = 255.0f;
		}
	}

	// If we hit a mirror face, render the world through the mirror's POV, then draw the mirror poly on top of the 
	//	hole made by the mirror  (NOTE - we only do this if the Driver wants to do recursive scenes)
	if ((TexFlags & TEXINFO_MIRROR) && CanDoMirrors && MirrorRecursion < MAX_MIRROR_RECURSION)
	{
		Frustum_Info	MirrorFrustum;
		geXForm3d		MirrorXForm, OldXForm;
		GFX_Plane		*pPlane;
		geVec3d			FaceNormal;
		float			FaceDist;
		
		//
		// Create the mirror frustum, for the mirrored scene.
		// Use the transformed data, since the Frustum is expected to start out in camera space
		//
		if (!Frustum_SetFromPoly(&MirrorFrustum, pDest2, Length1, MirrorRecursion&1))
			return;

		if (MirrorFrustum.NumPlanes+1 >= MAX_FCP)
			return;		// Oh well...

		// Add the transformed face plane to the frustum, so we only draw what is on the front
		// side of the mirror
		pPlane = &MirrorFrustum.Planes[MirrorFrustum.NumPlanes++];

		if (MirrorRecursion&1)
			gePlane_SetFromVerts(pPlane, &pDest2[0], &pDest2[1], &pDest2[2]);
		else
			gePlane_SetFromVerts(pPlane, &pDest2[2], &pDest2[1], &pDest2[0]);

		//
		//	Mirror the camera
		//	Use the world space face for this
		//
		#pragma message ("Rotated models are broken in mirrors.  Quick fix:  Rotate the plane against the models xform")

		FaceNormal = BSPData->GFXPlanes[pFace->PlaneNum].Normal;
		FaceDist = BSPData->GFXPlanes[pFace->PlaneNum].Dist;

		if (pFace->PlaneSide)
		{
			geVec3d_Inverse(&FaceNormal);
			FaceDist = -FaceDist;
		}

		// Save old camera xform
		OldXForm = *geCamera_GetWorldSpaceXForm(Camera);

		// Create the xform that will mirror the camera, by taking the surfaca plane of this face
		geXForm3d_Mirror(&OldXForm, &FaceNormal, FaceDist, &MirrorXForm);

		// Mirror the camera using this xform
		geCamera_SetWorldSpaceXForm(Camera, &MirrorXForm);

		MirrorRecursion++;
		CEngine->DebugInfo.NumMirrors++;
		
		// Render the world through the poly, from the mirrored camera 
		RenderScene(CEngine, CWorld, Camera, &MirrorFrustum);

		MirrorRecursion--;

		// Restore the camera
		geCamera_SetWorldSpaceXForm(Camera, &OldXForm);
	}

	// Get a pointer to the bitmap (texture)
	pBitmap = geWBitmap_GetBitmap(pWBitmap);
	assert(geWorld_HasBitmap(CWorld, pBitmap));

	RenderFlags = 0;

	CEngine->DebugInfo.SentPolys++;

	// All transparent polys (either some alpha translucency, or color key) will be drawn last, and sorted.
	//	They are then added to the TransPoly list (front to back), then when all is done, the Trans polys are drawn
	//  in reverse order (back to front) last.  
	//	NOTE - Mirrors are not put in this list.  They are drawn below, to cover up the "hole" made by the mirror...
	if ((pSurfInfo[Face].Flags & SURFINFO_TRANS) && !(pTexInfo->Flags & TEXINFO_MIRROR))
	{
		DRV_TLVertex	*pVerts;
		World_TransPoly	*pPoly;
		int32			Start;

		Start = FirstTransPolys[MirrorRecursion]+NumTransPolys[MirrorRecursion];

		if (Start+1 >= MAX_TRANS_POLYS || Length1 > MAX_CACHE_VERTS) 
			return;		// Oh well...

		pPoly = &TransPolys[Start];

		pVerts = pPoly->TLVerts;

		for (i = 0; i < Length1; i++, pVerts++)
			*pVerts = Clipped1[i];

		pPoly->Flags = 0;
		pPoly->Face = Face;
		pPoly->NumVerts = Length1;

		// Add this TransPoly to the GList (Geometry List) that is rendered after the current subscene is done
		GList_AddOperation(0, (uint32)pPoly);

		NumTransPolys[MirrorRecursion]++;

		// Just return, the Trans poly will get rendered at the end of the scene in GList_RenderOperations...
		return;
	}

	// If this surface is a mirror, and we can do mirrors, then render it with some alpha
	if ((pTexInfo->Flags & TEXINFO_MIRROR) && CanDoMirrors)
	{
		RenderFlags |= DRV_RENDER_ALPHA | DRV_RENDER_FLUSH;
		Clipped1[0].a = pTexInfo->Alpha;
	}
	else
	{
		// Else, don't render with any alpha
		Clipped1[0].a = 255.0f;
	}

	DrvTexInfo.ShiftU = pSurfInfo[Face].ShiftU;
	DrvTexInfo.ShiftV = pSurfInfo[Face].ShiftV;
	//DrvTexInfo.ShiftU = pTexInfo->Shift[0];
	//DrvTexInfo.ShiftV = pTexInfo->Shift[1];
	DrvTexInfo.DrawScaleU = pTexInfo->DrawScale[0];
	DrvTexInfo.DrawScaleV = pTexInfo->DrawScale[1];

	GlobalInfo.VecU = pTexInfo->Vecs[0];
	GlobalInfo.VecV = pTexInfo->Vecs[1];
	GlobalInfo.TexShiftX = pTexInfo->Shift[0];
	GlobalInfo.TexShiftY = pTexInfo->Shift[1];

	// Get the THandle from the bitmap
	THandle = geBitmap_GetTHandle(pBitmap);
	assert(THandle);

	if (pTexInfo->Flags & TEXINFO_NO_LIGHTMAP)
	{
		RDriver->RenderWorldPoly(Clipped1, Length1, THandle, &DrvTexInfo, NULL, RenderFlags);
	}
	else
	{
		DRV_LInfo			*pLInfo = &pSurfInfo[Face].LInfo;

		// The camera is set up at the beginning of the world...
		GlobalInfo.TexMinsX = pLInfo->MinU;
		GlobalInfo.TexMinsY = pLInfo->MinV;
		GlobalInfo.TexWidth = pLInfo->Width<<4;
		GlobalInfo.TexHeight = pLInfo->Height<<4;

		RDriver->RenderWorldPoly(Clipped1, Length1, THandle, &DrvTexInfo, &pSurfInfo[Face].LInfo, RenderFlags);
	}
}

//=====================================================================================
//	RenderWorldModel
//	Renders model 0 (the world model)
//=====================================================================================
static geBoolean RenderWorldModel(geCamera *Camera, Frustum_Info *FrustumInfo, geWorld_SkyBoxTData *SkyTData)
{
	geXForm3d			OldXForm,NewXForm, CXForm;
	geWorld_Model		*Models;
	Frustum_Info		WorldSpaceFrustum;
	uint32				StartClipFlags;
	geWorld_RenderInfo	RenderInfo;
	
	assert(CWorld != NULL);			// Asser that some globals are true (hopefully)...
	assert(CBSP != NULL);

	Models = CBSP->Models;		

	g_CurrentModel = Models;

	if (!RDriver->BeginWorld())
	{
		geErrorLog_Add(GE_ERR_BEGIN_WORLD_FAILED, NULL);
		return GE_FALSE;
	}

	OldXForm = *geCamera_GetWorldSpaceXForm(Camera); //Camera->MXForm;	// Save old camera for this model

	NewXForm = OldXForm;

	// Put the camera model about models origin
	geVec3d_Subtract(&NewXForm.Translation, &Models[0].Pivot, &NewXForm.Translation);

	// Transform the cameras xform against the models xform

	// treat the model as a camera, and apply it to the camera model
	geCamera_ConvertWorldSpaceToCameraSpace(&Models[0].XForm, &CXForm);

	geXForm3d_Multiply(&CXForm, &NewXForm, &NewXForm);

	// Put back into camera space
	geVec3d_Add(&NewXForm.Translation, &Models[0].Pivot, &NewXForm.Translation);

	// Convert the NewXForm back into a real camera
	geCamera_SetWorldSpaceXForm(Camera, &NewXForm);

	// Make the frustum go to world space
	Frustum_TransformToWorldSpace(FrustumInfo, Camera, &WorldSpaceFrustum);

	geCamera_FillDriverInfo(Camera);
	
	// Make a ClipFlags bits for for each side of the frustum...
	assert(WorldSpaceFrustum.NumPlanes < 32);

	StartClipFlags = (1<<WorldSpaceFrustum.NumPlanes)-1;

	RenderInfo.Camera = Camera;
	RenderInfo.Frustum = &WorldSpaceFrustum;
	RenderInfo.SkyTData = SkyTData;

	// Render the tree through the frustum
	RenderBSPFrontBack_r(	BSPData->GFXModels[0].RootNode[0], 
							&RenderInfo,
							StartClipFlags);

	// Restore the camera
	geCamera_SetWorldSpaceXForm(Camera, &OldXForm);
	
	if (!RDriver->EndWorld())
	{
		geErrorLog_Add(GE_ERR_END_WORLD_FAILED, NULL);
		return GE_FALSE;
	}
	return GE_TRUE;
}

//=====================================================================================
//	RenderSubModels
//	Renders all other models besides world model
//=====================================================================================
static geBoolean RenderSubModels(geCamera *Camera, Frustum_Info *FrustumInfo, geWorld_SkyBoxTData *SkyTData)
{
	int32				i;
	BOOL				OldVis;
	geWorld_Model		*Model;
	geXForm3d			OldXForm, NewXForm, CXForm;
	Frustum_Info		ModelSpaceFrustum;
	uint32				StartClipFlags;
	geWorld_RenderInfo	RenderInfo;


	if (!RDriver->BeginModels())
	{
		geErrorLog_Add(GE_ERR_BEGIN_MODELS_FAILED, NULL);
		return GE_FALSE;
	}

	assert(CWorld != NULL);			// Assert that some globals are true (hopefully)...
	assert(CBSP != NULL);
	
	OldVis = CWorld->VisInfo;		// Save old vis info flag

	CWorld->VisInfo = FALSE;		// Fake no vis info so ALL model faces/modes will draw
	
	Model = &CBSP->Models[1];		// Start with the model (skip the world, Models[0])
	
	// Render all sub models
	for (i=1; i< BSPData->NumGFXModels; i++, Model++)
	{
		g_CurrentModel = Model;

		if (Model->VisFrame != CWorld->CurFrameDynamic)
			continue;
		if (MirrorRecursion == 0 && !(Model->Flags & (GE_MODEL_RENDER_NORMAL | GE_MODEL_RENDER_ALWAYS)))
			continue;
		if (MirrorRecursion > 0 && !(Model->Flags & (GE_MODEL_RENDER_MIRRORS | GE_MODEL_RENDER_ALWAYS)))
			continue;

		CEngine->DebugInfo.NumModels++;

		OldXForm = *geCamera_GetWorldSpaceXForm(Camera);//Camera->MXForm;	// Save old camera for this model

		NewXForm = OldXForm;

		// Put the camera model about models origin
		geVec3d_Subtract(&NewXForm.Translation, &Model->Pivot, &NewXForm.Translation);

		// Transform the cameras xform against the models xform

		// treat the model as a camera, and apply it to the camera model
		geCamera_ConvertWorldSpaceToCameraSpace(&Model->XForm, &CXForm);

		geXForm3d_Multiply(&CXForm, &NewXForm, &NewXForm);

		// Put back into camera space
		geVec3d_Add(&NewXForm.Translation, &Model->Pivot, &NewXForm.Translation);

		// Convert the NewXForm back into a real camera
		geCamera_SetWorldSpaceXForm(Camera, &NewXForm);

		// Make the frustum go to World/Model space
		Frustum_TransformToWorldSpace(FrustumInfo, Camera, &ModelSpaceFrustum);

		geCamera_FillDriverInfo(Camera);
	
		// Make a ClipFlags bits for for each side of the frustum...
		StartClipFlags = (1<<ModelSpaceFrustum.NumPlanes)-1;

		RenderInfo.Camera = Camera;
		RenderInfo.Frustum = &ModelSpaceFrustum;
		RenderInfo.SkyTData = SkyTData;

		// Render the tree through the frustum
		RenderBSPFrontBack_r(	BSPData->GFXModels[i].RootNode[0], 
								&RenderInfo,
								StartClipFlags);

		// Restore the camera
		geCamera_SetWorldSpaceXForm(Camera, &OldXForm);
	}

	CWorld->VisInfo = OldVis;		// Restore original vis info
	
	if (!RDriver->EndModels())
	{
		geErrorLog_Add(GE_ERR_END_MODELS_FAILED, NULL);
		return GE_FALSE;
	}
	return GE_TRUE;
}

//========================================================================================
//	CreateGBSP
//========================================================================================
static World_BSP *CreateGBSP(geVFile *File)	
{
	World_BSP	*NewBSP;

	assert(File != NULL);

	// Create a new bsp World
	NewBSP = GE_RAM_ALLOCATE_STRUCT(World_BSP);

	if (!NewBSP)
	{
		geErrorLog_Add(GE_ERR_OUT_OF_MEMORY, NULL);
		return NULL;
	}
	
	memset(NewBSP, 0, sizeof(World_BSP));
	
	if (!GBSP_LoadGBSPFile(File, &NewBSP->BSPData))
	{
		geErrorLog_Add(GE_ERR_GBSP_LOAD_FAILURE, NULL);
		return NULL;
	}

	return NewBSP;
}

//========================================================================================
//	RenderTransPoly
//========================================================================================
static void RenderTransPoly(geCamera *Camera, World_TransPoly *pPoly)
{
	int32			Face;
	DRV_TLVertex	*pTLVerts;
	GFX_Face		*pFace;
	GFX_TexInfo		*pTexInfo;
	geBitmap		*pBitmap;
	int32			NumVerts;
	Surf_SurfInfo	*pSurfInfo2;
	DRV_LInfo		*pLInfo;
	DRV_TexInfo		DrvTexInfo;
	
	Face = pPoly->Face;
		
	pFace = &BSPData->GFXFaces[Face];

	pTLVerts = pPoly->TLVerts;
	NumVerts = pPoly->NumVerts;
		
	pTexInfo = &BSPData->GFXTexInfo[pFace->TexInfo];
	pSurfInfo2 = &pSurfInfo[Face];
	pLInfo = &pSurfInfo2->LInfo;

	// Get a pointer to the bitmap (texture)
	pBitmap = geWBitmap_Pool_GetBitmapByIndex(CBSP->WBitmapPool, pTexInfo->Texture);

	assert(geWorld_HasBitmap(CWorld, pBitmap));

	pTLVerts->a = pTexInfo->Alpha;

	DrvTexInfo.ShiftU = pSurfInfo2->ShiftU;
	DrvTexInfo.ShiftV = pSurfInfo2->ShiftV;
	//DrvTexInfo.ShiftU = pTexInfo->Shift[0];
	//DrvTexInfo.ShiftV = pTexInfo->Shift[1];
	DrvTexInfo.DrawScaleU = pTexInfo->DrawScale[0];
	DrvTexInfo.DrawScaleV = pTexInfo->DrawScale[1];

	// Setup the global info (pass this in the driver the right way!!!)
	GlobalInfo.VecU = pTexInfo->Vecs[0];
	GlobalInfo.VecV = pTexInfo->Vecs[1];
	GlobalInfo.TexShiftX = pTexInfo->Shift[0];
	GlobalInfo.TexShiftY = pTexInfo->Shift[1];

	GlobalInfo.PlaneNormal = BSPData->GFXPlanes[pFace->PlaneNum].Normal;
	GlobalInfo.PlaneDist = BSPData->GFXPlanes[pFace->PlaneNum].Dist;
	geXForm3d_Rotate(geCamera_GetCameraSpaceXForm(Camera), &GlobalInfo.PlaneNormal, &GlobalInfo.RPlaneNormal);

	if (pTexInfo->Flags & TEXINFO_NO_LIGHTMAP)
	{
		geRDriver_THandle	*THandle;

		THandle = geBitmap_GetTHandle(pBitmap);

		assert(THandle);

		RDriver->RenderWorldPoly(pTLVerts, NumVerts, THandle, &DrvTexInfo, NULL, DRV_RENDER_ALPHA | DRV_RENDER_FLUSH);
	}
	else
	{
		geRDriver_THandle	*THandle;

		// This global info only needs to be set up for faces that have lightmaps...
		GlobalInfo.TexMinsX = pLInfo->MinU;
		GlobalInfo.TexMinsY = pLInfo->MinV;
		GlobalInfo.TexWidth = pLInfo->Width<<4;
		GlobalInfo.TexHeight = pLInfo->Height<<4;

		THandle = geBitmap_GetTHandle(pBitmap);

		assert(THandle);

		RDriver->RenderWorldPoly(pTLVerts, NumVerts, THandle, &DrvTexInfo, pLInfo, DRV_RENDER_ALPHA | DRV_RENDER_FLUSH);
	}
}

//========================================================================================
//	CalcBSPModelInfo
//	Calculates the center of each BModel by taking the center of their bounding boxs...
//	Calcs other info as well...
//========================================================================================
static void CalcBSPModelInfo(World_BSP *BSP)
{
	int32			m, Node;
	geWorld_Model	*Models;
	GBSP_BSPData	*BData;
	geVec3d			Mins, Maxs;
	
	assert(BSP != NULL);

	Models = BSP->Models;
	BData = &BSP->BSPData;

	for (m=0; m< BData->NumGFXModels; m++)
	{
		int32		i;

		Node = BData->GFXModels[m].RootNode[0];

		if (Node < 0 )
			continue;

		Mins = BData->GFXNodes[Node].Mins;
		Maxs = BData->GFXNodes[Node].Maxs;

		// Need this so that we can get the motions later.  Horrible.
		Models[m].BSPModel = &BData->GFXModels[m];

		// Get the models REAL center
		for (i=0; i<3; i++)
			VectorToSUB(Models[m].RealCenter, i) = 
				(VectorToSUB(Mins, i) + VectorToSUB(Maxs, i)) * 0.5f;

		// Get the models rotational pivot point
		Models[m].Pivot = BData->GFXModels[m].Origin;
		
		Models[m].GFXModelNum = m;

		Models[m].Mins = Mins;
		Models[m].Maxs = Maxs;

		// Set the translated mins/maxs to some default value...
		Models[m].TMins = Mins;
		Models[m].TMaxs = Maxs;

		Models[m].Flags = GE_MODEL_RENDER_NORMAL | GE_MODEL_RENDER_MIRRORS;
		Models[m].ChangedFlags = MODEL_CHANGED_XFORM;
	}
}

// FIXME:  Put all this model stuff into Model.c
#pragma message ("Fix naming convention in models i.e: geWorld_SetModelXForm ---> geWorld_ModelSetXForm...")
//========================================================================================
//	World_SetModelXForm
//========================================================================================
GENESISAPI geBoolean geWorld_SetModelXForm(geWorld *World, geWorld_Model *Model, const geXForm3d *XForm)
{
	geVec3d	AxisVecs[3], Center;
	int		i, j;

	assert(World != NULL);
	assert(Model != NULL);

	Model->XForm = *XForm;
	memset(AxisVecs, 0, sizeof(geVec3d)*3);

	//grab the box center
	geVec3d_Add(&Model->Mins, &Model->Maxs, &Center);
	geVec3d_Scale(&Center, 0.5f, &Center);

	//build a local rotated axis based on extent vectors
	//(this could be simplified to less fmuls)
	for(i=0;i < 3;i++)
	{
		VectorToSUB(AxisVecs[i], i)	=VectorToSUB(Model->Maxs, i) - VectorToSUB(Center, i);
		geXForm3d_Rotate(XForm, &AxisVecs[i], &AxisVecs[i]);
	}

	//mask off the sign bits
	for(i=0;i < 3;i++)
	{
		for(j=0;j < 3;j++)
		{
			*((int *)(&AxisVecs[i])+j)	=*((int *)(&AxisVecs[i])+j) & 0x7fffffff;
		}
	}

	//add up vecs to get max
	for(i=0;i < 3;i++)
	{
		VectorToSUB(Model->TMaxs, i)
			=VectorToSUB(AxisVecs[0], i)
			+ VectorToSUB(AxisVecs[1], i)
			+ VectorToSUB(AxisVecs[2], i);
	}

	//local min is opposite of max
	Model->TMins	=Model->TMaxs;
	geVec3d_Inverse(&Model->TMins);

	//move back to world
	geVec3d_Add(&XForm->Translation, &Center, &Center);
	geVec3d_Add(&Model->TMins, &Center, &Model->TMins);
	geVec3d_Add(&Model->TMaxs, &Center, &Model->TMaxs);

	//add a small epsilon
	for(i=0;i < 3;i++)
	{
		VectorToSUB(Model->TMaxs, i)	+=1.0f;
		VectorToSUB(Model->TMins, i)	-=1.0f;
	}
	Model->ChangedFlags |= MODEL_CHANGED_XFORM;

	return GE_TRUE;
}

//========================================================================================
//	World_GetModelXForm
//========================================================================================
GENESISAPI geBoolean geWorld_GetModelXForm(const geWorld *World, const geWorld_Model *Model, geXForm3d *XForm)
{
	assert(World != NULL);
	assert(Model != NULL);
	assert(XForm != NULL);

	*XForm = Model->XForm;

	return GE_TRUE;
}

//=====================================================================================
//	FillAreas_r
//=====================================================================================
void FillAreas_r(geWorld *World, uint32 Area, uint8 *List, uint32 *Count)
{
	GBSP_BSPData	*BSP;
	GFX_Area		*a;
	GFX_AreaPortal	*p;
	int32			i;

	List[*Count] = (uint8)Area;
	*Count++;
	
	BSP = &World->CurrentBSP->BSPData;

	a = &BSP->GFXAreas[Area];

	p = &BSP->GFXAreaPortals[a->FirstAreaPortal];

	for (i=0; i< a->NumAreaPortals; i++, p++)
	{
		geWorld_Model	*Model;

		Model = &World->CurrentBSP->Models[p->ModelNum];

		if (Model->Open)
			FillAreas_r(World, p->Area, List, Count);
	}
}

//========================================================================================
//	geWorld_OpenModel
//========================================================================================
GENESISAPI geBoolean geWorld_OpenModel(geWorld *World, geWorld_Model *Model, geBoolean Open)
{
	World_BSP	*WBSP;
	int32		a0, a1;
	GFX_Model	*GFXModel;
#if 0
	int32		i0, i1;
	int32		NumWorkAreas0;
	uint8		WorkAreas0[256];
	int32		NumWorkAreas1;
	uint8		WorkAreas1[256];
#endif

	assert(World);
	assert(Model);

	if (Model->Open == Open)
		return GE_TRUE;			// Nothing changed

	Model->Open = Open;
	World->ForceVis = GE_TRUE;			// Force an update

	WBSP = World->CurrentBSP;

	GFXModel = &WBSP->BSPData.GFXModels[Model->GFXModelNum];

	a0 = GFXModel->Areas[0];
	a1 = GFXModel->Areas[1];

	// We know these 2 can see each other
	WBSP->AreaConnections[a0][a1] = Open;
	WBSP->AreaConnections[a1][a0] = Open;

#if 0
	NumWorkAreas0 = NumWorkAreas1 = 0;
	
	if (Open)
	{
		// Combine list into one, and combine vis sets
		FillAreas_r(World, a0, WorkAreas0, &NumWorkAreas0);
		FillAreas_r(World, a1, WorkAreas0, &NumWorkAreas0);

		// Connect all areas that were flooded into on each side
		for (i0=0; i0<NumWorkAreas0; i0++)
		{
			uint8		*pWork0, *pWork1;

			pWork0 = &WorkAreas0[i0];
			pWork1 = WorkAreas0;

			for (i1=0; i1<NumWorkAreas0; i1++, pWork1++)
			{
				WBSP->AreaConnections[*pWork0][*pWork1] = 1;
				WBSP->AreaConnections[*pWork1][*pWork0] = 1;
			}
		}
	}
	else
	{
		// Seperate list into two list, and seperate vis list
		FillAreas_r(World, a0, WorkAreas0, &NumWorkAreas0);
		FillAreas_r(World, a1, WorkAreas1, &NumWorkAreas1);

		// Seperate visiblity from one side to the other
		for (i0=0; i0<NumWorkAreas0; i0++)
		{
			uint8		*pWork0, *pWork1;

			pWork0 = &WorkAreas0[i0];
			pWork1 = WorkAreas1;

			for (i1=0; i1<NumWorkAreas1; i1++, pWork1++)
			{
				WBSP->AreaConnections[*pWork0][*pWork1] = 0;
				WBSP->AreaConnections[*pWork1][*pWork0] = 0;
			}
		}
	}
 #endif

	return GE_TRUE;
}

//========================================================================================
//========================================================================================
GENESISAPI geBoolean geWorld_GetModelRotationalCenter(
	const geWorld *			World,
	const geWorld_Model *	Model,
	geVec3d *				Center)
{
	assert(World != NULL);
	assert(Model != NULL);
	assert(Center != NULL);

	*Center = Model->Pivot;

	return GE_TRUE;
}

//========================================================================================
//	World_ModelGetBBox
//========================================================================================
GENESISAPI geBoolean geWorld_ModelGetBBox(const geWorld *World, const geWorld_Model *Model, geVec3d *Mins, geVec3d *Maxs)
{
	int32		i;

	assert(World != NULL);
	assert(Model != NULL);
	assert(Mins);
	assert(Maxs);

	*Mins = Model->Mins;
	*Maxs = Model->Maxs;

	// Translate the BBox to object space 
	for (i=0; i<3; i++)
	{
		VectorToSUB(*Mins, i) -= VectorToSUB(Model->Pivot, i);
		VectorToSUB(*Maxs, i) -= VectorToSUB(Model->Pivot, i);
	}

	return GE_TRUE;
}

//========================================================================================
//	World_ModelGetMotion
//========================================================================================
GENESISAPI geMotion * geWorld_ModelGetMotion(geWorld_Model *Model)
{
	assert(Model);
	return Model->BSPModel->Motion;
}

//========================================================================================
//========================================================================================
GENESISAPI void * geWorld_ModelGetUserData(const geWorld_Model *Model)
{
	assert(Model);
	return Model->UserData;
}

//========================================================================================
//========================================================================================
GENESISAPI void geWorld_ModelSetUserData(geWorld_Model *Model, void *UserData)
{
	assert(Model);
	Model->UserData = UserData;
}

//========================================================================================
//========================================================================================
GENESISAPI void geWorld_ModelSetFlags(geWorld_Model *Model, uint32 ModelFlags)
{
	assert(Model);
	Model->Flags = ModelFlags;
}

//========================================================================================
//========================================================================================
GENESISAPI uint32 geWorld_ModelGetFlags(geWorld_Model *Model)
{
	assert(Model);

	return Model->Flags;
}

//========================================================================================
//========================================================================================
GENESISAPI geBoolean geWorld_AddActor( geWorld *World, geActor *Actor, uint32 Flags, uint32 UserFlags)
{
	World_Actor *NewArray;
	assert( World != NULL );
	assert( geActor_IsValid(Actor) != GE_FALSE );
	assert( World->ActorCount >= 0 );

	NewArray = GE_RAM_REALLOC_ARRAY( World->ActorArray, World_Actor , World->ActorCount+1);
	if (NewArray == NULL)	
		{
			geErrorLog_AddString(-1,"Failed to grow world actor array", NULL);
			return GE_FALSE;
		}
	World->ActorArray = NewArray;
	World->ActorArray[World->ActorCount].Actor		= Actor;
	World->ActorArray[World->ActorCount].Flags		= Flags;
	World->ActorArray[World->ActorCount].UserFlags	= UserFlags;
	if (geActor_RenderPrep( Actor,World )==GE_FALSE)
		{
			geErrorLog_AddString(-1,"Failed to prepare the actor for rendering", NULL);
			return GE_FALSE;
		}
	World->ActorCount++;
	geActor_CreateRef(Actor);

	#ifdef DO_ADDREMOVE_MESSAGES	
	{
	char str[100];
		sprintf(str,"World_AddActor : %08X\n",Actor);
		OutputDebugString(str);
	}
	#endif
	
	return GE_TRUE;
}


//========================================================================================
//========================================================================================
GENESISAPI geBoolean geWorld_RemoveActor(geWorld *World, geActor *Actor)
{
	int i,Count;
	assert( World != NULL );
	assert( geActor_IsValid(Actor) != GE_FALSE );
	assert( World->ActorCount >= 0 );

	Count = World->ActorCount;

	#ifdef DO_ADDREMOVE_MESSAGES	
	{
	char str[100];
		sprintf(str,"World_RemoveActor : %08X\n",Actor);
		OutputDebugString(str);
	}
	#endif

	for (i=0; i<Count; i++)
		{
			if (World->ActorArray[i].Actor == Actor)
				{
					geActor_Destroy( &Actor );
					World->ActorArray[i] = World->ActorArray[Count-1];
					World->ActorArray[Count-1].Actor = NULL;
					World->ActorArray[Count-1].Flags = 0;
					World->ActorCount--;
					return GE_TRUE;
				}
		}
	geErrorLog_AddString(-1,"Failed to find actor in actor list", NULL);
	
	return GE_FALSE;
}

//========================================================================================
//========================================================================================
GENESISAPI geBoolean geWorld_SetActorFlags(geWorld *World, geActor *Actor, uint32 Flags)
{
	int i,Count;
	assert( World != NULL );
	assert( geActor_IsValid(Actor) != GE_FALSE );

	Count = World->ActorCount;

	for (i=0; i<Count; i++)
		{
			if (World->ActorArray[i].Actor == Actor)
				{
					World->ActorArray[i].Flags = Flags;
					return GE_TRUE;
				}
		}
	geErrorLog_AddString(-1,"Failed to find actor in actor list", NULL);
	return GE_FALSE;
}

//========================================================================================
//	geWorld_GetLeaf
//========================================================================================
GENESISAPI geBoolean geWorld_GetLeaf(const geWorld *World, const geVec3d *Pos, int32 *Leaf)
{
	assert(World);
	assert(Leaf);

	// Return the leaf that is model 0 (the main world model)
	*Leaf = Plane_FindLeaf(World, World->CurrentBSP->BSPData.GFXModels[0].RootNode[0], Pos);

	return GE_TRUE;
}

//========================================================================================
//	geWorld_MightSeeLeaf
//========================================================================================
GENESISAPI geBoolean geWorld_MightSeeLeaf(const geWorld *World, int32 Leaf)
{
	assert(World);

	assert(Leaf >= 0 && Leaf < World->CurrentBSP->BSPData.NumGFXLeafs);

	if (World->CurrentBSP->LeafData[Leaf].VisFrame == World->CurFrameStatic)
		return GE_TRUE;

	return GE_FALSE;
}

//========================================================================================
//	geWorld_LeafMightSeeLeaf
//========================================================================================
GENESISAPI geBoolean geWorld_LeafMightSeeLeaf(const geWorld *World, int32 Leaf1, int32 Leaf2, uint32 VisFlags)
{
	GBSP_BSPData	*BSPData;
	int32			Cluster1, Cluster2, VisOfs;
	uint8			*VisData;

	assert(World);
	assert(World->CurrentBSP);
	assert(VisFlags == 0);		// VisFlags are not used, and must be set to 0 for future use...

	BSPData = &World->CurrentBSP->BSPData;

	assert(Leaf1 >= 0 && Leaf1 < BSPData->NumGFXLeafs);
	assert(Leaf2 >= 0 && Leaf2 < BSPData->NumGFXLeafs);

	// Get the clusters that the leafs are in...
	Cluster1 = BSPData->GFXLeafs[Leaf1].Cluster;
	Cluster2 = BSPData->GFXLeafs[Leaf2].Cluster;

	if (Cluster1 == -1 || Cluster2 == -1)
		return GE_FALSE;		// If either on is in solid space, thern assume they can't see each other...

	assert(Cluster1 >= 0 && Cluster1 < BSPData->NumGFXClusters);
	assert(Cluster2 >= 0 && Cluster2 < BSPData->NumGFXClusters);

	VisOfs = BSPData->GFXClusters[Cluster1].VisOfs;

	// If no vis data for cluster 1, then assume no vis data in entire map, and just return TRUE...
	// This lets them run the map with no vis info, they will just suffer...
	if (VisOfs == -1)
		return GE_TRUE;

	assert(VisOfs >=0 && VisOfs < BSPData->NumGFXVisData);

	VisData = &BSPData->GFXVisData[VisOfs];

	// See if Cluster2's bit is set in Cluster1's vis data set
	if (VisData[Cluster2>>3] & (1<<(Cluster2&7)) )		
	{
		int32	Area1, Area2;

		Area1 = BSPData->GFXLeafs[Leaf1].Area;
		Area2 = BSPData->GFXLeafs[Leaf2].Area;

		assert(Area1 > 0 && Area2 > 0);

		if (World->CurrentBSP->AreaConnections[Area1][Area2])
			return GE_TRUE;				// They can see each other...
	}

	return GE_FALSE;				// They cannot see each other...
}

//========================================================================================
//	geWorld_GetSetByClass
//========================================================================================
GENESISAPI geEntity_EntitySet *geWorld_GetEntitySet(geWorld *World, const char *ClassName)
{
	geWorld_EntClassSet		*WSets;
	int32					i;

	assert(World);

	// No classname, just return the main set of all entities
	if (!ClassName)
	{
		assert(World->EntClassSets[0].Set);

		return World->EntClassSets[0].Set;
	}

	WSets = World->EntClassSets;
	
	for (i=1; i< World->NumEntClassSets; i++)
	{
		assert(WSets[i].Set);

		if (!stricmp(WSets[i].ClassName, ClassName))
			return WSets[i].Set;
	}

	return NULL;
}

//====================================================================================
//	geWorld_GetNextModel
//====================================================================================
GENESISAPI geWorld_Model *geWorld_GetNextModel(geWorld *World, geWorld_Model *Start)
{
	int32			MNum;
	GBSP_BSPData	*BSPData;
	World_BSP		*WorldBSP;

	assert(World != NULL);

	WorldBSP = World->CurrentBSP;

	assert(WorldBSP != NULL);

	BSPData = &WorldBSP->BSPData;

	if (Start)
	{
		MNum = (Start - WorldBSP->Models);

		assert(MNum >= 0);
		assert(MNum < BSPData->NumGFXModels);

		MNum++;
	}
	else
		MNum = 0;

	if (MNum >= BSPData->NumGFXModels)
		return NULL;		// No more models

	return &WorldBSP->Models[MNum];
}

//====================================================================================
//	*********** FOG stuff *************
//====================================================================================

//====================================================================================
//	FogSetAttributesCB
//	CB to be called whenever geFog_SetAttributes is called
//====================================================================================
static geBoolean geFog_SetAttributesCB(geFog *Fog)
{
	geWorld_FogData		*FogData;
	geVec3d				*Mins, *Maxs;

	//Fog->VolumeRadius = 1000.0f;		// Test

	FogData = geFog_GetUserData(Fog);

	// Record the leaf the fog is in...
	geWorld_GetLeaf(FogData->World, &Fog->Pos, &FogData->Leaf);

	// Set Mins/Maxs to fog radius
	Mins = &FogData->Mins;
	Maxs = &FogData->Maxs;

	Mins->X = -Fog->VolumeRadius;
	Mins->Y = -Fog->VolumeRadius;
	Mins->Z = -Fog->VolumeRadius;

	Maxs->X =  Fog->VolumeRadius;
	Maxs->Y =  Fog->VolumeRadius;
	Maxs->Z =  Fog->VolumeRadius;

	return GE_TRUE;
}

//====================================================================================
//	geWorld_AddFog
//====================================================================================
GENESISAPI geFog *geWorld_AddFog(geWorld *World)
{
	geFog				*Fog;
	geWorld_FogData		*FogData;

	assert(World);

	Fog = NULL;
	FogData = NULL;

	Fog = geFog_Create(geFog_SetAttributesCB);		// Create the fog

	if (!Fog)
		goto ExitWithError;

	// Insert at begining of list
	if (World->FogList)
		World->FogList->Prev = Fog;

	Fog->Next = World->FogList;
	World->FogList = Fog;

	// Set up fog user data for the engine to use ONLY
	FogData = GE_RAM_ALLOCATE_STRUCT(geWorld_FogData);

	if (!FogData)
		goto ExitWithError;

	FogData->World = World;				// Remember what world created the fog

	geFog_SetUserData(Fog, FogData);

	return Fog;

	ExitWithError:
	{
		if (FogData)
			geRam_Free(FogData);

		if (Fog)
			geWorld_RemoveFog(World, Fog);
	}

	return NULL;
}

//====================================================================================
//	geWorld_RemoveFog
//====================================================================================
GENESISAPI geBoolean geWorld_RemoveFog(geWorld *World, geFog *Fog)
{
	geWorld_FogData		*FogData;

	assert(World);
	assert(Fog);

	FogData = geFog_GetUserData(Fog);

	if (FogData)
		geRam_Free(FogData);

	geFog_SetUserData(Fog, NULL);		// Just in case
	
	if (Fog->Prev)
		Fog->Prev->Next = Fog->Next;

	if (Fog->Next)
		Fog->Next->Prev = Fog->Prev;

	if (Fog == World->FogList)
	{
		assert(Fog->Prev == NULL);
		World->FogList = Fog->Next;
	}

	geFog_Destroy(Fog);

	return GE_TRUE;
}

typedef struct
{
	geVec3d		Origin;
	GE_RGBA		Color;
	float		Brightness;
	float		Radius;

} MapFogData;

//====================================================================================
//	CreateStaticFogList
//====================================================================================
static geBoolean CreateStaticFogList(geWorld *World)
{
	geEntity			*Entity;
	geEntity_EntitySet	*EntitySet;
	geFog				*Fog;

	// ONly interested in "FogLight"'s

	if ( World->NumEntClassSets == 0 )
		return GE_TRUE;

	EntitySet = geWorld_GetEntitySet(World, "FogLight");

	if (!EntitySet)
		return GE_TRUE;

	Entity = NULL;

	while (1)
	{
		MapFogData	*Fd;

		Entity = geEntity_EntitySetGetNextEntity(EntitySet, Entity);

		if (!Entity)
			break;

		Fd = geEntity_GetUserData(Entity);

		// Must have user data set
		if (!Fd)
			goto ExitWithError;
		
		// Add the fog to the world
		Fog = geWorld_AddFog(World);

		if (!Fog)
			goto ExitWithError;

		if (!geFog_SetAttributes(Fog, &Fd->Origin, &Fd->Color, 1.0f, Fd->Brightness, Fd->Radius))
			goto ExitWithError;

	}

	return GE_TRUE;
	
	//=== ERROR
	ExitWithError:
	{
		if (Fog)
			geWorld_RemoveFog(World, Fog);

		return GE_FALSE;
	}
}

//================================================================================
//	***** SkyBox stuff ******
//================================================================================

#define SKYBOX_WIDTH	10000.0f
#define SKYBOX_HEIGHT	10000.0f
#define SKYBOX_DEPTH	10000.0f

//========================================================================================
//	BuildSkyBox
//========================================================================================
static geBoolean BuildSkyBox(World_SkyBox *SkyBox, const GFX_SkyData *SkyData)
{
	geVec3d			*Verts;
	int32			i;
	float			TexWidth, TexHeight;
	geVec3d			Zero;

	// Copy data over so it is more convenient
	SkyBox->Axis = SkyData->Axis;
	SkyBox->Dpm = SkyData->Dpm;
	SkyBox->DrawScale = SkyData->DrawScale;

	for (i=0; i<6; i++)
		SkyBox->Textures[i] = SkyData->Textures[i];

	geVec3d_Set(&Zero, 0.0f, 0.0f, 0.0f);
	if (geVec3d_Compare(&SkyBox->Axis, &Zero, 0.05f))
		geVec3d_Set(&SkyBox->Axis, 1.0f, 0.0f, 0.0f);

	// Build top
	Verts = SkyBox->Verts[0];
	
	Verts[0].X = -(SKYBOX_WIDTH/2); 
	Verts[0].Y =  (SKYBOX_HEIGHT/2); 
	Verts[0].Z =  (SKYBOX_DEPTH/2);

	Verts[1].X =  (SKYBOX_WIDTH/2); 
	Verts[1].Y =  (SKYBOX_HEIGHT/2);
	Verts[1].Z =  (SKYBOX_DEPTH/2);

	Verts[2].X =  (SKYBOX_WIDTH/2);
	Verts[2].Y =  (SKYBOX_HEIGHT/2);
	Verts[2].Z = -(SKYBOX_DEPTH/2);

	Verts[3].X = -(SKYBOX_WIDTH/2); 
	Verts[3].Y =  (SKYBOX_HEIGHT/2); 
	Verts[3].Z = -(SKYBOX_DEPTH/2);

	// Build Bottom
	Verts = SkyBox->Verts[1]; 
	Verts[0].X = -(SKYBOX_WIDTH/2); 
	Verts[0].Y = -(SKYBOX_HEIGHT/2); 
	Verts[0].Z = -(SKYBOX_DEPTH/2);

	Verts[1].X =  (SKYBOX_WIDTH/2);
	Verts[1].Y = -(SKYBOX_HEIGHT/2);
	Verts[1].Z = -(SKYBOX_DEPTH/2);

	Verts[2].X =  (SKYBOX_WIDTH/2);
	Verts[2].Y = -(SKYBOX_HEIGHT/2);
	Verts[2].Z =  (SKYBOX_DEPTH/2);

	Verts[3].X = -(SKYBOX_WIDTH/2);
	Verts[3].Y = -(SKYBOX_HEIGHT/2);
	Verts[3].Z =  (SKYBOX_DEPTH/2);

	// Build Left
	Verts = SkyBox->Verts[2];
	Verts[0].X = -(SKYBOX_WIDTH/2);
	Verts[0].Y =  (SKYBOX_HEIGHT/2);
	Verts[0].Z =  (SKYBOX_DEPTH/2);

	Verts[1].X = -(SKYBOX_WIDTH/2);
	Verts[1].Y =  (SKYBOX_HEIGHT/2);
	Verts[1].Z = -(SKYBOX_DEPTH/2);

	Verts[2].X = -(SKYBOX_WIDTH/2);
	Verts[2].Y = -(SKYBOX_HEIGHT/2);
	Verts[2].Z = -(SKYBOX_DEPTH/2);

	Verts[3].X = -(SKYBOX_WIDTH/2);
	Verts[3].Y = -(SKYBOX_HEIGHT/2);
	Verts[3].Z =  (SKYBOX_DEPTH/2);

	// Build Right
	Verts = SkyBox->Verts[3];
	Verts[0].X =  (SKYBOX_WIDTH/2);
	Verts[0].Y =  (SKYBOX_HEIGHT/2);
	Verts[0].Z = -(SKYBOX_DEPTH/2);

	Verts[1].X =  (SKYBOX_WIDTH/2);
	Verts[1].Y =  (SKYBOX_HEIGHT/2);
	Verts[1].Z =  (SKYBOX_DEPTH/2);

	Verts[2].X =  (SKYBOX_WIDTH/2);
	Verts[2].Y = -(SKYBOX_HEIGHT/2);
	Verts[2].Z =  (SKYBOX_DEPTH/2);

	Verts[3].X =  (SKYBOX_WIDTH/2);
	Verts[3].Y = -(SKYBOX_HEIGHT/2);
	Verts[3].Z = -(SKYBOX_DEPTH/2);

	// Build Front
	Verts = SkyBox->Verts[4];
	Verts[0].X = -(SKYBOX_WIDTH/2);
	Verts[0].Y =  (SKYBOX_HEIGHT/2);
	Verts[0].Z = -(SKYBOX_DEPTH/2);

	Verts[1].X =  (SKYBOX_WIDTH/2);
	Verts[1].Y =  (SKYBOX_HEIGHT/2);
	Verts[1].Z = -(SKYBOX_DEPTH/2);

	Verts[2].X =  (SKYBOX_WIDTH/2);
	Verts[2].Y = -(SKYBOX_HEIGHT/2);
	Verts[2].Z = -(SKYBOX_DEPTH/2);

	Verts[3].X = -(SKYBOX_WIDTH/2);
	Verts[3].Y = -(SKYBOX_HEIGHT/2);
	Verts[3].Z = -(SKYBOX_DEPTH/2);

	// Build back
	Verts = SkyBox->Verts[5];
	Verts[0].X =  (SKYBOX_WIDTH/2);
	Verts[0].Y =  (SKYBOX_HEIGHT/2);
	Verts[0].Z =  (SKYBOX_DEPTH/2);

	Verts[1].X = -(SKYBOX_WIDTH/2);
	Verts[1].Y =  (SKYBOX_HEIGHT/2);
	Verts[1].Z =  (SKYBOX_DEPTH/2);

	Verts[2].X = -(SKYBOX_WIDTH/2);
	Verts[2].Y = -(SKYBOX_HEIGHT/2);
	Verts[2].Z =  (SKYBOX_DEPTH/2);

	Verts[3].X =  (SKYBOX_WIDTH/2);
	Verts[3].Y = -(SKYBOX_HEIGHT/2);
	Verts[3].Z =  (SKYBOX_DEPTH/2);

	TexWidth = SkyBox->DrawScale;
	TexHeight = SkyBox->DrawScale;

	for (i=0; i<6; i++)
	{
		Surf_TexVert	*TexVerts;

		TexVerts = SkyBox->TexVerts[i];

		TexVerts[0].u = 0.0f;
		TexVerts[0].v = 0.0f;

		TexVerts[1].u = TexWidth;
		TexVerts[1].v = 0.0f;

		TexVerts[2].u = TexWidth;
		TexVerts[2].v = TexHeight;

		TexVerts[3].u = 0.0f;
		TexVerts[3].v = TexHeight;
	}

	return GE_TRUE;
}

//========================================================================================
//	RenderSkyThroughFrustum
//========================================================================================
static void RenderSkyThroughFrustum(World_SkyBox *SkyBox, geWorld_SkyBoxTData *SkyTData, geCamera *Camera, Frustum_Info *Fi)
{
	int32			i, p;
	DRV_TLVertex	Clipped1[30];
	geVec3d			*pDest1, *pDest2, Dest1[30], Dest2[30];
	Surf_TexVert	*pTex1, *pTex2, Tex1[30], Tex2[30];
	int32			Length1, Length2;
	geBitmap		*pBitmap;
	geVec3d			CameraPos = {0.0f, 0.0f, 0.0f};
	int32			TexNum;
	GFX_Plane		*Planes;
	uint32			SkyFlags;

	assert(SkyTData->NumTransformed >= 0);		// Now that we have a far clip plane, it is possible to have no sky...

	if (!SkyTData->NumTransformed)
		return;

	SkyFlags = DRV_RENDER_FLUSH;

	if (SkyBox->DrawScale <= 1.0f)
		SkyFlags |= DRV_RENDER_CLAMP_UV;

	Planes = Fi->Planes;

	for (i=0; i< SkyTData->NumTransformed; i++)
	{
		geRDriver_THandle	*THandle;

		// Get a pointer to the bitmap (texture)
		// NOTE - The sky box uses textures from the bsp file, but node that they were
		// created with RegisterMiscTexture with the driver.   The engine knew how to do this
		// by looking at the texture flags when it uploaded them...
		TexNum = SkyBox->Textures[SkyTData->OriginalFaces[i]];

		if (TexNum < 0)
			continue;

		pBitmap = geWBitmap_Pool_GetBitmapByIndex(CBSP->WBitmapPool, TexNum);

		assert(geWorld_HasBitmap(CWorld, pBitmap));

		pDest1 = SkyTData->TransformedVerts[i];
		pDest2 = Dest2;
		pTex1 = SkyTData->TransformedTexVerts[i];
		pTex2 = Tex2;
		Length1 = SkyTData->NumTransformedVerts[i];
						

		// NumVerts is the number of clip planes from the sky poly...
		for (p=0; p< Fi->NumPlanes; p++)
		{
			if (!Frustum_ClipToPlaneUV(&Planes[p], pDest1, pDest2, pTex1, pTex2, Length1, &Length2))
				break;

			if (pDest1 == Dest2)
			{
				pDest1 = Dest1;
				pDest2 = Dest2;
				pTex1 = Tex1;
				pTex2 = Tex2;
			}
			else
			{
				pDest1 = Dest2;
				pDest2 = Dest1;
				pTex1 = Tex2;
				pTex2 = Tex1;
			}
			Length1 = Length2;
		}

		if (p != Fi->NumPlanes)
			continue;

		if (Length1 < 3)
			continue;

		// Project them to screen space
		Frustum_Project(pDest1, pTex1, Clipped1, Length1, Camera);

		for (p=0; p<Length1; p++)
		{
			Clipped1[p].r = 255.0f;
			Clipped1[p].g = 255.0f;
			Clipped1[p].b = 255.0f;
		}

		Clipped1[0].a = 255.0f;

		THandle = geBitmap_GetTHandle(pBitmap);
		assert(THandle);

	#ifdef SKY_BOX_USE_WORLD_POLY
		{
			DRV_TexInfo	TexInfo;

			TexInfo.ShiftU = 0.0f;
			TexInfo.ShiftV = 0.0f;
			TexInfo.DrawScaleU = 1.0f;
			TexInfo.DrawScaleV = 1.0f;

			RDriver->RenderWorldPoly(Clipped1, Length1, THandle, &TexInfo, NULL, SkyFlags);
		}
	#else
		RDriver->RenderMiscTexturePoly(Clipped1, Length1, THandle, SkyFlags);
	#endif

	}
}

//=====================================================================================
//	SetupSkyBoxFaceForScene
//=====================================================================================
static void SetupSkyBoxFaceForScene(World_SkyBox *SkyBox, int32 Face, const geXForm3d *XForm, Frustum_Info *Fi, geWorld_SkyBoxTData *SkyTData)
{
	geVec3d			Dest1[MAX_RENDERFACE_VERTS], Dest2[MAX_RENDERFACE_VERTS], *pDest1, *pDest2;
	Surf_TexVert	Tex1[MAX_RENDERFACE_VERTS], Tex2[MAX_RENDERFACE_VERTS];
	Surf_TexVert	*pTex1, *pTex2;
	int32			Length1, Length2;
	int32			p;
	GFX_Plane		*pFPlane;
	float			Width, Height;
	int32			TexNum;
	GFX_Texture		*pTexture;

	TexNum = CWorld->CurrentBSP->BSPData.GFXSkyData.Textures[Face];

	if (TexNum < 0)		// No texture on sky face
		return;

	pDest1 = SkyBox->Verts[Face];
	pDest2 = Dest2;
	pTex1 = SkyBox->TexVerts[Face];
	pTex2 = Tex2;
	Length1 = 4;

	pFPlane = Fi->Planes;

	for (p=0; p< Fi->NumPlanes; p++, pFPlane++)
	{
		if (!Frustum_ClipToPlaneUV(pFPlane, pDest1, pDest2, pTex1, pTex2, Length1, &Length2))
			return;

		if (pDest1 == Dest2)
		{
			pDest1 = Dest1;
			pDest2 = Dest2;
			pTex1 = Tex1;
			pTex2 = Tex2;
		}
		else
		{
			pDest1 = Dest2;
			pDest2 = Dest1;
			pTex1 = Tex2;
			pTex2 = Tex1;
		}
		Length1 = Length2;
	}
	  
	if (Length1 < 3)
		return;

	pDest2 = SkyTData->TransformedVerts[SkyTData->NumTransformed];
	pTex2 = SkyTData->TransformedTexVerts[SkyTData->NumTransformed];
	SkyTData->NumTransformedVerts[SkyTData->NumTransformed] = Length1;
	SkyTData->OriginalFaces[SkyTData->NumTransformed] = Face;

	pTexture = &CWorld->CurrentBSP->BSPData.GFXTextures[TexNum];

	Width = (float)pTexture->Width;
	Height = (float)pTexture->Height;
	
	//geXForm3d_RotateArray(CXForm, pDest1, pDest2, Length1);
	for (p=0; p<Length1; p++)
	{
		geXForm3d_Rotate(XForm, pDest1, pDest2);
		pDest1++;
		pDest2++;

		*pTex2 = *pTex1;

		#ifdef SKY_BOX_USE_WORLD_POLY
			pTex2->u *= Width;
			pTex2->v *= Height;
		#endif

		pTex1++;
		pTex2++;
	}

	SkyTData->NumTransformed++;		// Increase the number of sky polys transformed...
}

//=====================================================================================
//	SetupSkyForScene
//	Sets up sky for rendering through sky portals
//=====================================================================================
static void SetupSkyForScene(World_SkyBox *SkyBox, geCamera *Camera, Frustum_Info *Fi, geWorld_SkyBoxTData *SkyTData)
{
	int32			i;
	geXForm3d		XForm, OldXForm, QXForm;
	Frustum_Info	WorldSpaceFrustum;
	geQuaternion	Quat;
	
	// Reset the number of skypolys transformed...
	SkyTData->NumTransformed = 0;

	// Get the camera matrix
	XForm = *geCamera_GetWorldSpaceXForm(Camera);

	// Save it, so we can restore is when we are done
	OldXForm = XForm;

	// Rotate the camera's xform about the selected axis
	// First, build a quat that will do this
	geQuaternion_SetFromAxisAngle(&Quat, &SkyBox->Axis, -SkyBox->Angle);

	// Then convert the quat to a xform
	geQuaternion_ToMatrix(&Quat, &QXForm);
	
	geXForm3d_Multiply(&QXForm, &XForm, &XForm);

	// Put the XForm back into the camera
	geCamera_SetWorldSpaceXForm(Camera, &XForm);

	// Setup the View Frustum to look into the world
	Frustum_RotateToWorldSpace(Fi, Camera, &WorldSpaceFrustum);

	// NOTE - SetupSkyBoxFaceForScene only rotates the box, and does not translate...
	for (i=0; i<6; i++)
		SetupSkyBoxFaceForScene(SkyBox, i, geCamera_GetCameraSpaceXForm(Camera), &WorldSpaceFrustum, SkyTData);

	// Restore the camera
	geCamera_SetWorldSpaceXForm(Camera, &OldXForm);
}

//================================================================================
//	*** BitmapList stuff ***
//================================================================================

//================================================================================
//	geWorld_BitmapListInit
//	Initializes the world bitmaplist
//================================================================================
geBoolean geWorld_BitmapListInit(geWorld *World)
{
	assert(World);
	assert(World->AttachedBitmaps == NULL);

	if (World->AttachedBitmaps == NULL )
	{
		World->AttachedBitmaps = BitmapList_Create();

		if (!World->AttachedBitmaps )
		{
			geErrorLog_AddString(-1, "geWorld_BitmapListInit:  BitmapList_Create failed.", NULL);
			return GE_FALSE;
		}
	}

	// Only add bitmaps if the list is not NULL (could be an empty world with no textures yet)
	if (World->CurrentBSP->WBitmapPool)
	{
		int32		i, Count;
		Count = geWBitmap_Pool_GetWBitmapCount(World->CurrentBSP->WBitmapPool);

		for (i=0; i<Count; i++)
		{
			geBitmap		*pBitmap;
			geWBitmap		*pWBitmap;
			geBitmap_Info	Info;
			uint32			Flags;

			pWBitmap = geWBitmap_Pool_GetWBitmapByIndex(World->CurrentBSP->WBitmapPool, i);
			assert(pWBitmap);

			pBitmap = geWBitmap_GetBitmap(pWBitmap);
			assert(pBitmap);

			Flags = geWBitmap_GetFlags(pWBitmap);

			if (!geBitmap_GetInfo(pBitmap, &Info, NULL))
			{
				geErrorLog_AddString(-1, "geWorld_BitmapListInit:  geBitmap_GetInfo failed.", NULL);
				return GE_FALSE;
			}
			
			if (!geWorld_AddBitmap(World, pBitmap))
			{
				geErrorLog_AddString(-1, "geWorld_BitmapListInit:  geWorld_AddBitmap failed.", NULL);
				return GE_FALSE;
			}

			World->Changed = GE_TRUE;
		}
	}

	return GE_TRUE;
}

//================================================================================
//	geWorld_BitmapListShutdown
//================================================================================
geBoolean geWorld_BitmapListShutdown(geWorld *World)
{
	assert(World);

	if (World->AttachedBitmaps )
	{
		//BitmapList_DetachAll(World->AttachedBitmaps);
		// destroy detached for you!
		BitmapList_Destroy(World->AttachedBitmaps);
		World->AttachedBitmaps = NULL;
	}

	return GE_TRUE;
}

//================================================================================
//	geWorld_AddBitmap
//================================================================================
GENESISAPI geBoolean geWorld_AddBitmap(geWorld *World, geBitmap *Bitmap)
{
	assert(World);
	assert(Bitmap);
	assert(World->AttachedBitmaps);

	if (!World->AttachedBitmaps)
	{
		geErrorLog_AddString(-1, "geWorld_AddBitmap:  AttachedBitmapList is NULL.", NULL);
		return GE_FALSE;
	}

	geBitmap_SetDriverFlags(Bitmap, RDRIVER_PF_3D | RDRIVER_PF_COMBINE_LIGHTMAP);

	// Add bitmap to the list of bitmaps attached to the engine
	if ( BitmapList_Add(World->AttachedBitmaps, (void*)Bitmap) )
	{
		World->Changed = GE_TRUE;
		
		#ifdef DO_ADDREMOVE_MESSAGES	
		{
		char str[100];
			sprintf(str,"World_AddBitmap : %08X : new\n",Bitmap);
			OutputDebugString(str);
		}
		#endif
	}
	else
	{
		#ifdef DO_ADDREMOVE_MESSAGES	
		{
		char str[100];
			sprintf(str,"World_AddBitmap : %08X : old\n",Bitmap);
			OutputDebugString(str);
		}
		#endif
	}

	return GE_TRUE;
}

//================================================================================
//	geWorld_RemoveBitmap
//================================================================================
GENESISAPI geBoolean geWorld_RemoveBitmap(geWorld *World,geBitmap *Bitmap)
{

	assert(World);
	assert(Bitmap);
	assert(World->AttachedBitmaps);

	if (!World->AttachedBitmaps)
		return GE_FALSE;

	if ( BitmapList_Remove(World->AttachedBitmaps, Bitmap) )
	{
		World->Changed = GE_TRUE;
		
		#ifdef DO_ADDREMOVE_MESSAGES	
		{
		char str[100];
			sprintf(str,"World_RemoveBitmap : %08X : removed\n",Bitmap);
			OutputDebugString(str);
		}
		#endif
	}
	else
	{
		#ifdef DO_ADDREMOVE_MESSAGES	
		{
		char str[100];
			sprintf(str,"World_RemoveBitmap : %08X : left\n",Bitmap);
			OutputDebugString(str);
		}
		#endif
	}
		

	return GE_TRUE;
}

//================================================================================
//	geWorld_GetBitmapByName
//================================================================================
GENESISAPI geBitmap *geWorld_GetBitmapByName(geWorld *World, const char *BitmapName)
{
	if (!World->CurrentBSP)
		return NULL;

	assert(World->CurrentBSP->WBitmapPool);

	return geWBitmap_Pool_GetBitmapByName(World->CurrentBSP->WBitmapPool, BitmapName);
}

//================================================================================
//	geWorld_AttachAll
//================================================================================
geBoolean geWorld_AttachAll(geWorld *World, DRV_Driver *Driver, geFloat Gamma)
{
	assert(World);
	assert(World->AttachedBitmaps);
	assert(Driver);

	if (!BitmapList_AttachAll(World->AttachedBitmaps, Driver, Gamma))
	{
		geErrorLog_AddString(-1, "geWorld_AttachAll:  BitmapList_AttachAll failed.", NULL);
		return GE_FALSE;
	}
	
	return GE_TRUE;
}

//================================================================================
//	geWorld_DetachAll
//================================================================================
geBoolean geWorld_DetachAll(geWorld *World)
{
	assert(World);
	assert(World->AttachedBitmaps);

	if (!BitmapList_DetachAll(World->AttachedBitmaps))
	{
		geErrorLog_AddString(-1, "geWorld_DetachAll:  BitmapList_DetachAll failed.", NULL);
		return GE_FALSE;
	}
	return GE_TRUE;
}

//================================================================================
//	geWorld_HasBitmap
//================================================================================
GENESISAPI geBoolean geWorld_HasBitmap(const geWorld *World, const geBitmap *Bitmap)
{
	assert(World);
	assert(World->AttachedBitmaps);

	return BitmapList_Has((BitmapList*)World->AttachedBitmaps, (geBitmap*)Bitmap);
}
//================================================================================
//================================================================================
GENESISAPI geBoolean geWorld_BitmapIsVisible(geWorld *World, const geBitmap *Bitmap)
{
	geWBitmap	*pWBitmap;

	pWBitmap = geWBitmap_Pool_GetWBitmapByBitmap(World->CurrentBSP->WBitmapPool, Bitmap);

	if (!pWBitmap)			// Not in the list!  Should this be an error?????
		return GE_FALSE;

	if (geWBitmap_GetVisFrame(pWBitmap) == World->CurFrameDynamic)
		return GE_TRUE;

	return GE_FALSE;
}


