/****************************************************************************************/
/*  GBSPFile.h                                                                          */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: BSP loader                                                             */
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
#ifndef GE_GBSPFILE_H
#define GE_GBSPFILE_H

#include <Stdio.h>

#include "BaseType.h"
#include "Vec3d.h"
#include <windows.h>
#include "DCommon.h"
#include "VFile.h"
#include "Motion.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GBSP_VERSION				15	

#define GBSP_CHUNK_HEADER			0

#define GBSP_CHUNK_MODELS			1
#define GBSP_CHUNK_NODES			2
#define GBSP_CHUNK_BNODES			3
#define GBSP_CHUNK_LEAFS			4
#define GBSP_CHUNK_CLUSTERS			5
#define GBSP_CHUNK_AREAS			6	
#define GBSP_CHUNK_AREA_PORTALS		7	
#define GBSP_CHUNK_LEAF_SIDES		8
#define GBSP_CHUNK_PORTALS			9
#define GBSP_CHUNK_PLANES			10
#define GBSP_CHUNK_FACES			11
#define GBSP_CHUNK_LEAF_FACES		12
#define GBSP_CHUNK_VERT_INDEX		13
#define GBSP_CHUNK_VERTS			14
#define GBSP_CHUNK_RGB_VERTS		15
#define GBSP_CHUNK_ENTDATA			16
									 
#define GBSP_CHUNK_TEXINFO			17
#define GBSP_CHUNK_TEXTURES			18 
#define GBSP_CHUNK_TEXDATA			19

#define GBSP_CHUNK_LIGHTDATA		20
#define GBSP_CHUNK_VISDATA			21
#define GBSP_CHUNK_SKYDATA			22
#define GBSP_CHUNK_PALETTES			23
#define GBSP_CHUNK_MOTIONS			24

#define GBSP_CHUNK_END				0xffff

#ifndef GE_CONTENTS_TYPES
#define GE_CONTENTS_TYPES

//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
//	IF THESE FLAGS CHANGE, THEY MUST CHANGE IN GBSPFILE.H in Genesis AND GBSPLIB, and Genesis.H!!!!!
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
#define GE_CONTENTS_SOLID			(1<<0)		// Solid (Visible)
#define GE_CONTENTS_WINDOW			(1<<1)		// Window (Visible)
#define GE_CONTENTS_EMPTY			(1<<2)		// Empty but Visible (water, lava, etc...)

#define GE_CONTENTS_TRANSLUCENT		(1<<3)		// Vis will see through it
#define GE_CONTENTS_WAVY			(1<<4)		// Wavy (Visible)
#define GE_CONTENTS_DETAIL			(1<<5)		// Won't be included in vis oclusion

#define GE_CONTENTS_CLIP			(1<<6)		// Structural but not visible
#define GE_CONTENTS_HINT			(1<<7)		// Primary splitter (Non-Visible)
#define GE_CONTENTS_AREA			(1<<8)		// Area seperator leaf (Non-Visible)

#define GE_CONTENTS_FLOCKING		(1<<9)
#define GE_CONTENTS_SHEET			(1<<10)
#define GE_RESERVED3				(1<<11)
#define GE_RESERVED4				(1<<12)
#define GE_RESERVED5				(1<<13)
#define GE_RESERVED6				(1<<14)
#define GE_RESERVED7				(1<<15)

// 16-31 reserved for user contents
#define GE_CONTENTS_USER1			(1<<16)
#define GE_CONTENTS_USER2			(1<<17)
#define GE_CONTENTS_USER3			(1<<18)
#define GE_CONTENTS_USER4			(1<<19)
#define GE_CONTENTS_USER5			(1<<20)
#define GE_CONTENTS_USER6			(1<<21)
#define GE_CONTENTS_USER7			(1<<22)
#define GE_CONTENTS_USER8			(1<<23)
#define GE_CONTENTS_USER9			(1<<24)
#define GE_CONTENTS_USER10			(1<<25)
#define GE_CONTENTS_USER11			(1<<26)
#define GE_CONTENTS_USER12			(1<<27)
#define GE_CONTENTS_USER13			(1<<28)
#define GE_CONTENTS_USER14			(1<<29)
#define GE_CONTENTS_USER15			(1<<30)
#define GE_CONTENTS_USER16			(1<<31)
// 16-31 reserved for user contents


// These contents are all solid types
#define GE_CONTENTS_SOLID_CLIP		(GE_CONTENTS_SOLID | GE_CONTENTS_WINDOW | GE_CONTENTS_CLIP)
#define GE_CONTENTS_CANNOT_OCCUPY	GE_CONTENTS_SOLID_CLIP

// These contents are all visible types
#define GE_VISIBLE_CONTENTS			(	GE_CONTENTS_SOLID | \
										GE_CONTENTS_EMPTY | \
										GE_CONTENTS_WINDOW | \
										GE_CONTENTS_WAVY)

#endif

#define BSP_CONTENTS_SOLID			-1			
#define BSP_CONTENTS_EMPTY			-2

#define	PLANE_X						0
#define	PLANE_Y						1
#define	PLANE_Z						2
#define	PLANE_ANYX					3
#define	PLANE_ANYY					4
#define	PLANE_ANYZ					5
#define	PLANE_ANY					6		

typedef struct
{
	int32				Type;						// Type of chunk
	int32				Size;						// Size of each element
	int32				Elements;					// Number of elements
} GBSP_Chunk;

typedef struct
{
	int32				Type;
	int32				Size;
	int32				Elements;
	void				*Data;
} GBSP_ChunkData;

typedef struct
{
	char				TAG[5];						// 'G','B','S','P','0'
	int32				Version;
	SYSTEMTIME			BSPTime;
} GBSP_Header;

typedef struct
{
	geVec3d				Axis;						// Axis of rotation
	float				Dpm;						// Degres per minute
	int32				Textures[6];				// Texture indexes for all six sides...
	float				DrawScale;
} GFX_SkyData;

typedef struct
{
	geVec3d			Normal;
	geFloat			Dist;
	int32			Type;						// Defined in MathLib.h (PLANE_X, PLANE_Y, etc...)
} GFX_Plane;

typedef struct
{
	int32			Children[2];				// Children, indexed into GFXNodes, < 0 = Leaf
	int32			NumFaces;					// Num faces
	int32			FirstFace;					// First face
	int32			PlaneNum;					// 
	geVec3d			Mins;						// For BBox frustum culling
	geVec3d			Maxs;
} GFX_Node;

typedef struct
{
	int32			Children[2];				// Children, indexed into GFXBNodes, < 0 = Contents
	int32			PlaneNum;					// 
	//int32			PlaneSide;
} GFX_BNode;

typedef struct
{
	int32	ModelNum;
	int32	Area;
} GFX_AreaPortal;

typedef struct
{
	int32	NumAreaPortals;
	int32	FirstAreaPortal;
} GFX_Area;

typedef struct
{
	int32			Contents;					// Contents of leaf
	geVec3d			Mins;						// For BBox vis testing
	geVec3d			Maxs;
	int32			FirstFace;					// First face in GFXLeafFaces 
	int32			NumFaces;
	int32			FirstPortal;				// Number of portals
	int32			NumPortals;					// First portal

	int32			Cluster;					// Cluster area for this leaf
	int32			Area;						// -1 = Area, 0 = Solid > 0 = Area number

	int32			FirstSide;					// Beveled sides for BBox collisions
	int32			NumSides;
} GFX_Leaf;

typedef struct
{
	int32			VisOfs;
} GFX_Cluster;

typedef struct
{
	int32			PlaneNum;
	int32			PlaneSide;
} GFX_LeafSide;

typedef struct
{
	int32			FirstVert;					// First vertex indexed in GFXVertices
	int32			NumVerts;					// Number of vertices in face
	int32			PlaneNum;					// PlaneNum 
	int32			PlaneSide;					// 0 = Same direction of plane normal
	int32			TexInfo;
	int32			LightOfs;					// Offset info GFXLightData, -1 = No light
	int32			LWidth;						// Lightmap width
	int32			LHeight;					// Lightmap height
	uint8			LTypes[4];
} GFX_Face;

typedef struct
{
	int32			RootNode[2];				// Top level Node in GFXNodes/GFXBNodes
	geVec3d			Mins;
	geVec3d			Maxs;
	geVec3d			Origin;
	int32			FirstFace;					// First face in GFXFaces
	int32			NumFaces;					// Number of faces
	int32			FirstLeaf;					// First leaf in GFXLeafs;
	int32			NumLeafs;					// Number of leafs (including solid leafs)
	int32			FirstCluster;				// First leaf cluster ijn GFXCLusters
	int32			NumClusters;				// Number of clusters in this model
	int32			Areas[2];
	geMotion *		Motion;
} GFX_Model;

typedef struct
{
	char			Name[32];
	uint32			Flags;
	int32			Width;
	int32			Height;
	int32			Offset;
	int32			PaletteIndex;
} GFX_Texture;

typedef struct
{
	geVec3d			Vecs[2];
	float			Shift[2];
	float			DrawScale[2];
	int32			Flags;
	float			FaceLight;			// Used in radiosity satge only (remove?)
	float			ReflectiveScale;
	float			Alpha;
	float			MipMapBias;
	int32			Texture;
} GFX_TexInfo;

typedef struct
{
	geVec3d			Origin;						// Center of portal
	int32			LeafTo;						// Leaf looking into
} GFX_Portal;

typedef struct
{
	GBSP_Header		GBSPHeader;			// Header
	GFX_SkyData		GFXSkyData;			// Sky data
	GFX_Model		*GFXModels;			// Model data
	GFX_Node		*GFXNodes;			// Nodes
	GFX_BNode		*GFXBNodes;			// Bevel Clip Nodes
	GFX_Leaf		*GFXLeafs;			// Leafs
	GFX_Cluster		*GFXClusters;		
	GFX_Area		*GFXAreas;		
	GFX_AreaPortal	*GFXAreaPortals;
	GFX_Plane		*GFXPlanes;			// Planes
	GFX_Face		*GFXFaces;			// Faces
	int32			*GFXLeafFaces;
	GFX_LeafSide	*GFXLeafSides;
	geVec3d			*GFXVerts;			// Verts
	int32			*GFXVertIndexList;	// Index list
	geVec3d			*GFXRGBVerts;		// RGBVerts

	uint8			*GFXEntData;
	GFX_Texture		*GFXTextures;		// Textures
	GFX_TexInfo		*GFXTexInfo;		// TexInfo
	uint8			*GFXTexData;		// TexData
	DRV_Palette		*GFXPalettes;		// Texture palettes

	uint8			*GFXLightData;		// Lightmap data
	uint8			*GFXVisData;		// Vis data
	GFX_Portal		*GFXPortals;		// Portal data

	int32			NumGFXModels;
	int32			NumGFXNodes;
	int32			NumGFXBNodes;
	int32			NumGFXLeafs;
	int32			NumGFXClusters;
	int32			NumGFXAreas;
	int32			NumGFXAreaPortals;
	int32			NumGFXPlanes;
	int32			NumGFXFaces;
	int32			NumGFXLeafFaces;
	int32			NumGFXLeafSides;
	int32			NumGFXVerts;
	int32			NumGFXVertIndexList;
	int32			NumGFXRGBVerts;

	int32			NumGFXEntData;
	int32			NumGFXTextures;
	int32			NumGFXTexInfo;
	int32			NumGFXTexData;
	int32			NumGFXPalettes;

	int32			NumGFXLightData;
	int32			NumGFXVisData;
	int32			NumGFXPortals;

} GBSP_BSPData;

geBoolean GBSP_LoadGBSPFile(geVFile *File, GBSP_BSPData *BSP);
geBoolean GBSP_FreeGBSPFile(GBSP_BSPData *BSP);

#ifdef __cplusplus
}
#endif

#endif