/****************************************************************************************/
/*  World.h                                                                             */
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
/*  Genesis3D Version 1.1 released November 15, 1999                                 */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#ifndef GE_WORLD_H
#define GE_WORLD_H

#include "ErrorLog.h"
#include "PtrTypes.h"
#include "Genesis.h"
#include "GBSPFile.h"
#include "Motion.h"
#include "Surface.h"
#include "Fog.h"
#include "WBitmap.h"
#include "User.h"
#include "Light.h"

#include "Bitmaplist.h"

#include "Actor.h"			

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_MODELS	256

// Upper limits on misc things
#define	MAX_MIRROR_RECURSION		1
#define MAX_RENDERFACE_VERTS		64

//=====================================================================================
//	Structure defines
//=====================================================================================
#define MODEL_CHANGED_XFORM				(1<<0)

typedef struct geWorld_Model
{
	char			Name[64];							// Model's name
	int32			GFXModelNum;						// Model number in disk tree structure
	geXForm3d		XForm;								// Models transform	(Object Space)
	geVec3d			Mins, Maxs;							// Model BBox (World Space)
	geVec3d			TMins, TMaxs;						// Translated Model BBox (World Space)
	geVec3d			Pivot;								// Center of rotation (World Space)
	geVec3d			RealCenter;
	int32			VisFrame;							// == World->CurFrame if visible
	void			*UserData;							// Place for client to store data
	GFX_Model *		BSPModel;							// Oh, this is terrible.

	geBoolean		Open;								// Model Open/Closed	(Set by API, used for area vising)

	uint32			Flags;								// GE_MODEL_RENDER_NORMAL, etc... (getypes.h)

	uint32			ChangedFlags;
} geWorld_Model;

typedef struct geWorld_Leaf
{
	int32			VisFrame;
	int32			Parent;								// Parent nodes of all leafs

	gePoly			*PolyList;							// List of poly fragments to render for this leaf (geWorld_AddPoly)
} geWorld_Leaf;

#define MAX_VISIBLE_FOG		12	// Hope to God there is not this much visible at a time!!!

// This is set in the UserData of the fog the the world creates at load time
typedef struct
{
	int32			Leaf;

	geVec3d			Mins;								// - Radius
	geVec3d			Maxs;								// + Radius

	int32			VisFrame;							// == World->CurFrameDynamic when visible

	geWorld			*World;
} geWorld_FogData;

typedef struct World_BSP
{
	char			FileName[200];
	GBSP_BSPData	BSPData;				// Info in the BSP loaded directly off disk

	//	Extra info thats not in the disk bsp structure
	Surf_SurfInfo	*SurfInfo;							// Valid when GE_SetGBSPFile is called
	Surf_TexVert	*TexVerts;							//

	geWBitmap_Pool	*WBitmapPool;

	geWorld_Model	Models[MAX_MODELS];					// Extra info about models not in disk structure

	geWorld_Leaf	*LeafData;

	int32			*ClusterVisFrame;
	int32			*NodeVisFrame;
	int32			*AreaVisFrame;
	uint8			AreaConnections[256][256];

	int32			*NodeParents;						// Parent nodes of all leafs

} World_BSP;

typedef struct
{
	// Untransformed original data
	geVec3d			Verts[6][4];
	Surf_TexVert	TexVerts[6][4];
	
	geVec3d			Axis;								// Rotation axis
	float			Dpm;								// Degres per minute
	float			DrawScale;							// Texture drawscale
	int32			Textures[6];
	
	float			Angle;								// Current rotation angle around roation axis

} World_SkyBox;

typedef struct
{
	// Transformed data
	uint32			SkyFlags;

	geVec3d			TransformedVecs[6][2];
	GFX_Plane		TransformedPlanes[6];

	int32			NumTransformed;						// Num transformed and clipped
	geVec3d			TransformedVerts[6][10];			// Transformed verts
	Surf_TexVert	TransformedTexVerts[6][10];			// Transformed tex verts
	int32			NumTransformedVerts[6];				// Number of transformed verts/texverts
	int32			OriginalFaces[6];					// Indexes to original bitmap

} geWorld_SkyBoxTData;

typedef struct World_Actor
{
	geActor			*Actor;
	uint32			Flags;				// GE_ACTOR_RENDER_NORMAL, GE_ACTOR_RENDER_MIRRORS, GE_ACTOR_COLLIDE
	uint32			UserFlags;

	//int32			Leaf;				// Current leaf the actor is in (currently used for PVS occlusion)
} World_Actor;

/******

Critial : A negative-numbered Node is a Leaf.
	Node (-1) is Leaf 0 ; (the first leaf is Leaf 0).
	Node (-N) is Leaf (N-1) = (-Node-1)

******/

#define MAX_WORLD_ENT_CLASS_SETS	256

typedef struct
{
	const char			*ClassName;		// NULL for main set
	geEntity_EntitySet	*Set;
} geWorld_EntClassSet;

typedef struct
{
	int32		NumNodesTraversed1;
	int32		NumNodesTraversed2;
	int32		NumLeafsHit1;
	int32		NumLeafsHit2;
	int32		NumLeafsWithUserPolys;
	int32		NumUserPolys;

} geWorld_DebugInfo;

typedef struct geWorld
{
	int32				CurFrameStatic;						// World CurrentFrame
	int32				CurFrameDynamic;					// World CurrentFrame
	float				CurTime;							// World CurrentTime
	int32				CurrentLeaf;
	geBoolean			ForceVis;

	geBoolean			VisInfo;

	// Info that each respective module fills in...
	World_BSP			*CurrentBSP;						// Valid when geWorld_SetGBSP is called

	World_SkyBox		SkyBox;

	Frustum_Info		*FrustumInfo;

	Light_LightInfo		*LightInfo;							// Info that the light module fills in

	Mesh_MeshInfo		*MeshInfo;							// Info that the mesh module fills in
	
	int32				ActorCount;							// Number of actors in world
	World_Actor			*ActorArray;						// Array of actors
	
	geWorld_EntClassSet	EntClassSets[MAX_WORLD_ENT_CLASS_SETS];
	int32				NumEntClassSets;

	User_Info			*UserInfo;

	// Debug info
	int32				ActiveUserPolys;
	geWorld_DebugInfo	DebugInfo;

	geFog				*FogList;							// Linked list of fog in the world currently
	
	geFog				*VisibleFog[MAX_VISIBLE_FOG];		// List of visible fog for this frame
	int32				NumVisibleFog;

	BitmapList			*AttachedBitmaps;						

	geXForm3d			LastCameraXForm;

	int32				RefCount;

	geBoolean			Changed;							// GE_TRUE if this world has changed

} geWorld;

//=====================================================================================
//	Function prototypes
//=====================================================================================
GENESISAPI		geWorld *geWorld_Create(geVFile *File);
GENESISAPI		void geWorld_Free(geWorld *World);
geBoolean		geWorld_CreateRef(geWorld *World);

geBoolean	World_EngineInit(geEngine *Engine);
void		World_EngineShutdown(geEngine *Engine);

geBoolean	World_SetEngine(geEngine *Engine);
geBoolean	World_SetWorld(geWorld *World);
geBoolean	World_SetGBSP(World_BSP *BSP);

geBoolean	World_WorldRenderQ(geEngine *Engine, geWorld *World, geCamera *Camera);

GENESISAPI geBoolean geWorld_SetModelXForm(
	geWorld *			World,
	geWorld_Model *		Model,
	const geXForm3d *	XForm);
GENESISAPI geBoolean geWorld_GetModelXForm(
	const geWorld *			World,
	const geWorld_Model *	Model,
	geXForm3d *				XForm);

GENESISAPI geBoolean geWorld_GetModelRotationalCenter(
	const geWorld *			World,
	const geWorld_Model *	Model,
	geVec3d *				Center);

GENESISAPI geMotion * geWorld_ModelGetMotion(geWorld_Model *Model);

GENESISAPI void * geWorld_ModelGetUserData(const geWorld_Model *Model);

GENESISAPI void	geWorld_ModelSetUserData(geWorld_Model *Model, void *UserData);

GENESISAPI geWorld_Model *geWorld_WorldGetNextModel(geWorld *World, geWorld_Model *Start);

GENESISAPI geBoolean geWorld_AddBitmap(geWorld *World, geBitmap *Bitmap);
GENESISAPI geBoolean geWorld_RemoveBitmap(geWorld *World,geBitmap *Bitmap);

GENESISAPI geBitmap *geWorld_GetBitmapByName(geWorld *World, const char *BitmapName);

geBoolean geWorld_AttachAll(geWorld *World, DRV_Driver *Driver, geFloat Gamma);
geBoolean geWorld_DetachAll(geWorld *World);

#ifdef __cplusplus
}
#endif
#endif

