/****************************************************************************************/
/*  GBSPFile.c                                                                          */
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
/*Genesis3D Version 1.1 released November 15, 1999                            */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#include <Stdio.h>
#include <assert.h>

#include "GBSPFile.h"
#include "Vec3d.h"
#include "Ram.h"
#include "System.h"

#include "ErrorLog.h"
#include "VFile.h"

static geBoolean LoadMotions(GBSP_BSPData *BSP, geVFile *f)
{
	char			KeyS[100], ValueS[100];
	char			line[200];
	int32			NumMotions, ModelNum, i;
	GFX_Model		*Models;

	if	(geVFile_GetS(f, line, sizeof(line)) == GE_FALSE)
		goto fail;

	if (sscanf(line, "%s %s\n", KeyS, ValueS) != 2)
		goto fail;

	if (strcmp(KeyS, "Genesis_Motion_File"))
		goto fail;

	if	(geVFile_GetS(f, line, sizeof(line)) == GE_FALSE)
		goto fail;

	if (sscanf(line, "%s %i\n", KeyS, &NumMotions) != 2)
		goto fail;

	if (strcmp(KeyS, "NumMotions"))
		goto fail;

	Models = BSP->GFXModels;

	for (i=0; i< NumMotions; i++)
	{
		if	(geVFile_GetS(f, line, sizeof(line)) == GE_FALSE)
			goto fail;

		if (sscanf(line, "%s %i\n", KeyS, &ModelNum) != 2)
			goto fail;

		if (strcmp(KeyS, "ModelNum"))
			goto fail;

		Models[ModelNum].Motion = geMotion_CreateFromFile(f);

		if	(!Models[ModelNum].Motion)
			goto fail;
	}

	return GE_TRUE;

fail:
	geErrorLog_Add(GE_ERR_INVALID_MODEL_MOTION_FILE, NULL);
	return GE_FALSE;
}

//========================================================================================
// ReadChunkData
//========================================================================================
static geBoolean ReadChunkData(GBSP_Chunk *Chunk, void *Data, geVFile *f)
{
	return geVFile_Read(f, Data, Chunk->Size * Chunk->Elements);
}

//========================================================================================
//	ReadChunk
//========================================================================================
static geBoolean ReadChunk(GBSP_BSPData *BSP, GBSP_Chunk *Chunk, geVFile *f)
{
	int	i;

	if (geVFile_Read(f, Chunk, sizeof(GBSP_Chunk)) == GE_FALSE)
	{
		return GE_FALSE;
	}

	switch(Chunk->Type)
	{
		case GBSP_CHUNK_HEADER:
		{
//		printf("GBSP_CHUNK_HEADER\n");
			if (sizeof(GBSP_Header) != Chunk->Size)
			{
				geErrorLog_Add(GE_ERR_BAD_BSP_FILE_CHUNK_SIZE, NULL);
				return GE_FALSE;
			}
			if (!ReadChunkData(Chunk, (void*)&BSP->GBSPHeader, f))
				return GE_FALSE;
			if (strcmp(BSP->GBSPHeader.TAG, "GBSP"))
			{
				geErrorLog_Add(GE_ERR_INVALID_BSP_TAG, NULL);
				return GE_FALSE;
			}
			if (BSP->GBSPHeader.Version != GBSP_VERSION)
			{
				geErrorLog_Add(GE_ERR_INVALID_BSP_VERSION, NULL);
				return GE_FALSE;
			}
			break;
		}
		case GBSP_CHUNK_MODELS:
		{
//		printf("GBSP_CHUNK_MODELS\n");
			if (sizeof(GFX_Model) != Chunk->Size)
			{
				geErrorLog_Add(GE_ERR_BAD_BSP_FILE_CHUNK_SIZE, NULL);
				return GE_FALSE;
			}
			BSP->NumGFXModels = Chunk->Elements;
			BSP->GFXModels = GE_RAM_ALLOCATE_ARRAY(GFX_Model, BSP->NumGFXModels);
			if (!ReadChunkData(Chunk, (void*)BSP->GFXModels, f))
				return GE_FALSE;
			// Walk the models and zero out the motion pointers
			for	(i = 0; i < BSP->NumGFXModels; i++)
				BSP->GFXModels[i].Motion = NULL;
			break;
		}
		case GBSP_CHUNK_NODES:
		{
//		printf("GBSP_CHUNK_NODES\n");
			if (sizeof(GFX_Node) != Chunk->Size)
			{
				geErrorLog_Add(GE_ERR_BAD_BSP_FILE_CHUNK_SIZE, NULL);
				return GE_FALSE;
			}
			BSP->NumGFXNodes = Chunk->Elements;
			BSP->GFXNodes = (GFX_Node*)geRam_Allocate(sizeof(GFX_Node)*BSP->NumGFXNodes);
			if (!ReadChunkData(Chunk, (void*)BSP->GFXNodes, f))
				return GE_FALSE;
			break;
		}
		case GBSP_CHUNK_BNODES:
		{
//		printf("GBSP_CHUNK_BNODES\n");
			if (sizeof(GFX_BNode) != Chunk->Size)
			{
				geErrorLog_Add(GE_ERR_BAD_BSP_FILE_CHUNK_SIZE, NULL);
				return GE_FALSE;
			}
			BSP->NumGFXBNodes = Chunk->Elements;
			if (BSP->NumGFXBNodes)
			{
				BSP->GFXBNodes = (GFX_BNode*)geRam_Allocate(sizeof(GFX_BNode)*BSP->NumGFXBNodes);
				if (!ReadChunkData(Chunk, (void*)BSP->GFXBNodes, f))
					return GE_FALSE;
			}
			break;
		}
		case GBSP_CHUNK_LEAFS:
		{
//		printf("GBSP_CHUNK_LEAFS\n");
			if (sizeof(GFX_Leaf) != Chunk->Size)
			{
				geErrorLog_Add(GE_ERR_BAD_BSP_FILE_CHUNK_SIZE, NULL);
				return GE_FALSE;
			}
			BSP->NumGFXLeafs = Chunk->Elements;
			BSP->GFXLeafs = (GFX_Leaf*)geRam_Allocate(sizeof(GFX_Leaf)*BSP->NumGFXLeafs);
			if (!ReadChunkData(Chunk, (void*)BSP->GFXLeafs, f))
				return GE_FALSE;
			break;
		}
		case GBSP_CHUNK_CLUSTERS:
		{
//		printf("GBSP_CHUNK_CLUSTERS\n");
			if (sizeof(GFX_Cluster) != Chunk->Size)
			{
				geErrorLog_Add(GE_ERR_BAD_BSP_FILE_CHUNK_SIZE, NULL);
				return GE_FALSE;
			}
			BSP->NumGFXClusters = Chunk->Elements;
			//BSP->GFXClusters = GE_RAM_ALLOCATE_ARRAY(GFX_Cluster, BSP->NumGFXClusters);
			BSP->GFXClusters = (GFX_Cluster*)geRam_Allocate(sizeof(GFX_Cluster)*BSP->NumGFXClusters);
			if (!ReadChunkData(Chunk, (void*)BSP->GFXClusters, f))
				return GE_FALSE;
			break;
		}
		case GBSP_CHUNK_AREAS:
		{
//		printf("GBSP_CHUNK_AREAS\n");
			if (sizeof(GFX_Area) != Chunk->Size)
			{
				geErrorLog_Add(GE_ERR_BAD_BSP_FILE_CHUNK_SIZE, NULL);
				return GE_FALSE;
			}
			BSP->NumGFXAreas = Chunk->Elements;
			BSP->GFXAreas = GE_RAM_ALLOCATE_ARRAY(GFX_Area, BSP->NumGFXAreas);
			if (!ReadChunkData(Chunk, BSP->GFXAreas, f))
				return GE_FALSE;
			break;
		}
		case GBSP_CHUNK_AREA_PORTALS:
		{
//		printf("GBSP_CHUNK_AREA_PORTALS\n");
			if (sizeof(GFX_AreaPortal) != Chunk->Size)
			{
				geErrorLog_Add(GE_ERR_BAD_BSP_FILE_CHUNK_SIZE, NULL);
				return GE_FALSE;
			}
			BSP->NumGFXAreaPortals = Chunk->Elements;
			BSP->GFXAreaPortals = GE_RAM_ALLOCATE_ARRAY(GFX_AreaPortal, BSP->NumGFXAreaPortals);
			if (!ReadChunkData(Chunk, BSP->GFXAreaPortals, f))
				return GE_FALSE;
			break;
		}
		case GBSP_CHUNK_PORTALS:
		{
//		printf("GBSP_CHUNK_PORTALS\n");
			if (sizeof(GFX_Portal) != Chunk->Size)
			{
				geErrorLog_Add(GE_ERR_BAD_BSP_FILE_CHUNK_SIZE, NULL);
				return GE_FALSE;
			}
			BSP->NumGFXPortals = Chunk->Elements;
			BSP->GFXPortals = (GFX_Portal*)geRam_Allocate(sizeof(GFX_Portal)*BSP->NumGFXPortals);
			if (!ReadChunkData(Chunk, (void*)BSP->GFXPortals, f))
				return GE_FALSE;
			break;
		}
		case GBSP_CHUNK_PLANES:
		{
//		printf("GBSP_CHUNK_PLANES\n");
			if (sizeof(GFX_Plane) != Chunk->Size)
			{
				geErrorLog_Add(GE_ERR_BAD_BSP_FILE_CHUNK_SIZE, NULL);
				return GE_FALSE;
			}
			BSP->NumGFXPlanes = Chunk->Elements;
			BSP->GFXPlanes = (GFX_Plane*)geRam_Allocate(sizeof(GFX_Plane)*BSP->NumGFXPlanes);
			if (!ReadChunkData(Chunk, (void*)BSP->GFXPlanes, f))
				return GE_FALSE;
			break;
		}
		case GBSP_CHUNK_FACES:
		{
//		printf("GBSP_CHUNK_FACES\n");
			if (sizeof(GFX_Face) != Chunk->Size)
			{
				geErrorLog_Add(GE_ERR_BAD_BSP_FILE_CHUNK_SIZE, NULL);
				return GE_FALSE;
			}
			BSP->NumGFXFaces = Chunk->Elements;
			BSP->GFXFaces = (GFX_Face*)geRam_Allocate(sizeof(GFX_Face)*BSP->NumGFXFaces);
			if (!ReadChunkData(Chunk, (void*)BSP->GFXFaces, f))
				return GE_FALSE;
			break;
		}
		case GBSP_CHUNK_LEAF_FACES:
		{
//		printf("GBSP_CHUNK_LEAF_FACES\n");
			if (sizeof(int32) != Chunk->Size)
			{
				geErrorLog_Add(GE_ERR_BAD_BSP_FILE_CHUNK_SIZE, NULL);
				return GE_FALSE;
			}
			BSP->NumGFXLeafFaces = Chunk->Elements;
			BSP->GFXLeafFaces = (int32*)geRam_Allocate(sizeof(int32)*BSP->NumGFXLeafFaces);
			if (!ReadChunkData(Chunk, (void*)BSP->GFXLeafFaces, f))
				return GE_FALSE;
			break;
		}
		case GBSP_CHUNK_LEAF_SIDES:
		{
//		printf("GBSP_CHUNK_LEAF_SIDES\n");
			if (sizeof(GFX_LeafSide) != Chunk->Size)
			{
				geErrorLog_Add(GE_ERR_BAD_BSP_FILE_CHUNK_SIZE, NULL);
				return GE_FALSE;
			}
			BSP->NumGFXLeafSides = Chunk->Elements;
			BSP->GFXLeafSides = (GFX_LeafSide*)geRam_Allocate(sizeof(GFX_LeafSide)*BSP->NumGFXLeafSides);
			if (!ReadChunkData(Chunk, (void*)BSP->GFXLeafSides, f))
				return GE_FALSE;
			break;
		}
		case GBSP_CHUNK_VERTS:
		{
//		printf("GBSP_CHUNK_VERTS\n");
			if (sizeof(geVec3d) != Chunk->Size)
			{
				geErrorLog_Add(GE_ERR_BAD_BSP_FILE_CHUNK_SIZE, NULL);
				return GE_FALSE;
			}
			BSP->NumGFXVerts = Chunk->Elements;
			BSP->GFXVerts = (geVec3d*)geRam_Allocate(sizeof(geVec3d)*BSP->NumGFXVerts);
			if (!ReadChunkData(Chunk, (void*)BSP->GFXVerts, f))
				return GE_FALSE;
			break;
		}
		case GBSP_CHUNK_VERT_INDEX:
		{
//		printf("GBSP_CHUNK_VERT_INDEX\n");
			if (sizeof(int32) != Chunk->Size)
			{
				geErrorLog_Add(GE_ERR_BAD_BSP_FILE_CHUNK_SIZE, NULL);
				return GE_FALSE;
			}

			BSP->NumGFXVertIndexList = Chunk->Elements;
			BSP->GFXVertIndexList = (int32*)geRam_Allocate(sizeof(int32)*BSP->NumGFXVertIndexList);
			if (!ReadChunkData(Chunk, (void*)BSP->GFXVertIndexList, f))
				return GE_FALSE;
			break;
		}

		case GBSP_CHUNK_RGB_VERTS:
		{
//		printf("GBSP_CHUNK_RGB_VERTS\n");
			if (sizeof(geVec3d) != Chunk->Size)
			{
				geErrorLog_Add(GE_ERR_BAD_BSP_FILE_CHUNK_SIZE, NULL);
				return GE_FALSE;
			}
			BSP->NumGFXRGBVerts = Chunk->Elements;
			BSP->GFXRGBVerts = (geVec3d*)geRam_Allocate(sizeof(geVec3d)*BSP->NumGFXRGBVerts);
			if (!ReadChunkData(Chunk, (void*)BSP->GFXRGBVerts, f))
				return GE_FALSE;
			break;
		}

		case GBSP_CHUNK_TEXINFO:
		{
//		printf("GBSP_CHUNK_TEXINFO\n");
			if (sizeof(GFX_TexInfo) != Chunk->Size)
			{
				geErrorLog_Add(GE_ERR_BAD_BSP_FILE_CHUNK_SIZE, NULL);
				return GE_FALSE;
			}
			BSP->NumGFXTexInfo = Chunk->Elements;
			BSP->GFXTexInfo = (GFX_TexInfo*)geRam_Allocate(sizeof(GFX_TexInfo)*BSP->NumGFXTexInfo);
			if (!ReadChunkData(Chunk, (void*)BSP->GFXTexInfo, f))
				return GE_FALSE;
			break;
		}

		case GBSP_CHUNK_TEXTURES:
		{
//		printf("GBSP_CHUNK_TEXTURES\n");
			if (sizeof(GFX_Texture) != Chunk->Size)
			{
				geErrorLog_Add(GE_ERR_BAD_BSP_FILE_CHUNK_SIZE, NULL);
				return GE_FALSE;
			}
			BSP->NumGFXTextures = Chunk->Elements;
			BSP->GFXTextures = (GFX_Texture*)geRam_Allocate(sizeof(GFX_Texture)*BSP->NumGFXTextures);
			if (!ReadChunkData(Chunk, (void*)BSP->GFXTextures, f))
				return GE_FALSE;
			break;
		}

		case GBSP_CHUNK_TEXDATA:
		{
//		printf("GBSP_CHUNK_TEXDATA\n");
			if (sizeof(uint8) != Chunk->Size)
			{
				geErrorLog_Add(GE_ERR_BAD_BSP_FILE_CHUNK_SIZE, NULL);
				return GE_FALSE;
			}
			BSP->NumGFXTexData = Chunk->Elements;
			BSP->GFXTexData = (uint8*)geRam_Allocate(sizeof(uint8)*BSP->NumGFXTexData);
			if (!ReadChunkData(Chunk, (void*)BSP->GFXTexData, f))
				return GE_FALSE;
			break;
		}
		case GBSP_CHUNK_ENTDATA:
		{
//		printf("GBSP_CHUNK_ENTDATA\n");
			if (sizeof(uint8) != Chunk->Size)
			{
				geErrorLog_Add(GE_ERR_BAD_BSP_FILE_CHUNK_SIZE, NULL);
				return GE_FALSE;
			}
			BSP->NumGFXEntData = Chunk->Elements;
			BSP->GFXEntData = (uint8*)geRam_Allocate(sizeof(uint8)*BSP->NumGFXEntData);
			if (!ReadChunkData(Chunk, (void*)BSP->GFXEntData, f))
				return GE_FALSE;
			break;
		}
		case GBSP_CHUNK_LIGHTDATA:
		{
//		printf("GBSP_CHUNK_LIGHTDATA\n");
			if (sizeof(uint8) != Chunk->Size)
			{
				geErrorLog_Add(GE_ERR_BAD_BSP_FILE_CHUNK_SIZE, NULL);
				return GE_FALSE;
			}
			BSP->NumGFXLightData = Chunk->Elements;
			BSP->GFXLightData = (uint8*)geRam_Allocate(sizeof(uint8)*BSP->NumGFXLightData);
			if (!ReadChunkData(Chunk, (void*)BSP->GFXLightData, f))
				return GE_FALSE;
			break;
		}
		case GBSP_CHUNK_VISDATA:
		{
//		printf("GBSP_CHUNK_VISDATA\n");
			if (sizeof(uint8) != Chunk->Size)
			{
				geErrorLog_Add(GE_ERR_BAD_BSP_FILE_CHUNK_SIZE, NULL);
				return GE_FALSE;
			}
			BSP->NumGFXVisData = Chunk->Elements;
			BSP->GFXVisData = (uint8*)geRam_Allocate(sizeof(uint8)*BSP->NumGFXVisData);
			if (!ReadChunkData(Chunk, (void*)BSP->GFXVisData, f))
				return GE_FALSE;
			break;
		}

		case GBSP_CHUNK_SKYDATA:
		{
//		printf("GBSP_CHUNK_SKYDATA\n");
			if (sizeof(GFX_SkyData) != Chunk->Size)
			{
				geErrorLog_Add(GE_ERR_BAD_BSP_FILE_CHUNK_SIZE, NULL);
				return GE_FALSE;
			}
			if (!ReadChunkData(Chunk, (void*)&BSP->GFXSkyData, f))
				return GE_FALSE;
			break;
		}

		case GBSP_CHUNK_PALETTES:
		{
//		printf("GBSP_CHUNK_PALETTES\n");
			if (sizeof(DRV_Palette) != Chunk->Size)
			{
				geErrorLog_Add(GE_ERR_BAD_BSP_FILE_CHUNK_SIZE, NULL);
				return GE_FALSE;
			}
			BSP->NumGFXPalettes = Chunk->Elements;
			BSP->GFXPalettes = (DRV_Palette*)geRam_Allocate(sizeof(DRV_Palette)*BSP->NumGFXPalettes);
			if	(BSP->GFXPalettes == NULL)
				return GE_FALSE;
			if (!ReadChunkData(Chunk, (void*)BSP->GFXPalettes, f))
				return GE_FALSE;
			break;
		}

		case GBSP_CHUNK_MOTIONS:
		{
//		printf("GBSP_CHUNK_MOTIONS\n");
			return LoadMotions(BSP, f);
		}

		case GBSP_CHUNK_END:
		{
//		printf("GBSP_CHUNK_END\n");
			break;
		}
		default:
//		printf("Don't know what this chunk is\n");
			return GE_FALSE;
	}

	return TRUE;
}

//========================================================================================
//	GBSP_LoadGBSPFile
//========================================================================================
BOOL GBSP_LoadGBSPFile(geVFile *File, GBSP_BSPData *BSP)
{
	GBSP_Chunk	Chunk;

	assert(File);
	assert(BSP);

	while (1)
	{
		if (!ReadChunk(BSP, &Chunk, File))
		{
			geErrorLog_Add(GE_ERR_ERROR_READING_BSP_CHUNK, NULL);
			return GE_FALSE;
		}
		
		if (Chunk.Type == GBSP_CHUNK_END)
			break;
	}

	return TRUE;
}

//========================================================================================
//	GBSP_FreeGBSPFile
//========================================================================================
BOOL GBSP_FreeGBSPFile(GBSP_BSPData *BSP)
{
	if (BSP->GFXModels)
	{
		int i;
		for	(i = 0; i < BSP->NumGFXModels; i++)
			if (BSP->GFXModels[i].Motion != NULL)
				geMotion_Destroy(&(BSP->GFXModels[i].Motion));
		geRam_Free(BSP->GFXModels);
	}

	if (BSP->GFXNodes)
		geRam_Free(BSP->GFXNodes);
	if (BSP->GFXBNodes)
		geRam_Free(BSP->GFXBNodes);
	if (BSP->GFXLeafs)
		geRam_Free(BSP->GFXLeafs);
	if (BSP->GFXClusters)
		geRam_Free(BSP->GFXClusters);
	if (BSP->GFXAreas)
		geRam_Free(BSP->GFXAreas);
	if (BSP->GFXAreaPortals)
		geRam_Free(BSP->GFXAreaPortals);
	if (BSP->GFXPortals)
		geRam_Free(BSP->GFXPortals);
	if (BSP->GFXPlanes)
		geRam_Free(BSP->GFXPlanes);
	if (BSP->GFXFaces)
		geRam_Free(BSP->GFXFaces);
	if (BSP->GFXLeafFaces)
		geRam_Free(BSP->GFXLeafFaces);
	if (BSP->GFXLeafSides)
		geRam_Free(BSP->GFXLeafSides);
	if (BSP->GFXVerts)
		geRam_Free(BSP->GFXVerts);
	if (BSP->GFXVertIndexList)
		geRam_Free(BSP->GFXVertIndexList);
	if (BSP->GFXRGBVerts)
		geRam_Free(BSP->GFXRGBVerts);
	if (BSP->GFXTextures)
		geRam_Free(BSP->GFXTextures);
	if (BSP->GFXTexInfo)
		geRam_Free(BSP->GFXTexInfo);
	if (BSP->GFXTexData)
		geRam_Free(BSP->GFXTexData);
	if (BSP->GFXPalettes)
		geRam_Free(BSP->GFXPalettes);
	if (BSP->GFXEntData)
		geRam_Free(BSP->GFXEntData);
	if (BSP->GFXLightData)
		geRam_Free(BSP->GFXLightData);
	if (BSP->GFXVisData)
		geRam_Free(BSP->GFXVisData);

	BSP->GFXModels = NULL;
	BSP->GFXNodes = NULL;
	BSP->GFXBNodes = NULL;
	BSP->GFXLeafs = NULL;
	BSP->GFXClusters = NULL;
	BSP->GFXAreas = NULL;
	BSP->GFXAreaPortals = NULL;
	BSP->GFXPlanes = NULL;
	BSP->GFXFaces = NULL;
	BSP->GFXLeafFaces = NULL;
	BSP->GFXLeafSides = NULL;
	BSP->GFXVerts = NULL;
	BSP->GFXVertIndexList = NULL;
	BSP->GFXRGBVerts = NULL;
	BSP->GFXEntData = NULL;
	
	BSP->GFXTextures = NULL;
	BSP->GFXTexInfo = NULL;
	BSP->GFXTexData = NULL;
	BSP->GFXPalettes = NULL;

	BSP->GFXLightData = NULL;
	BSP->GFXVisData = NULL;
	BSP->GFXPortals = NULL;

	BSP->NumGFXModels = 0;
	BSP->NumGFXNodes = 0;
	BSP->NumGFXBNodes = 0;
	BSP->NumGFXLeafs = 0;
	BSP->NumGFXClusters = 0;
	BSP->NumGFXAreas = 0;
	BSP->NumGFXAreaPortals = 0;
	BSP->NumGFXPlanes = 0;
	BSP->NumGFXFaces = 0;
	BSP->NumGFXLeafFaces = 0;
	BSP->NumGFXLeafSides = 0;
	BSP->NumGFXVerts = 0;
	BSP->NumGFXVertIndexList = 0;
	BSP->NumGFXRGBVerts = 0;

	BSP->NumGFXEntData = 0;
	BSP->NumGFXTexInfo = 0;
	BSP->NumGFXTextures = 0;
	BSP->NumGFXTexData = 0;

	BSP->NumGFXLightData = 0;
	BSP->NumGFXVisData = 0;
	BSP->NumGFXPortals = 0;

	return TRUE;
}

