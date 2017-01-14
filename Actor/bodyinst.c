/****************************************************************************************/
/*  BODYINST.C                                                                          */
/*                                                                                      */
/*  Author: Mike Sandige	                                                            */
/*  Description: Actor body instance implementation.                                    */
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
#include <assert.h>						//assert()

#include "body._h"
#include "bodyinst.h"
#include "ram.h"
#include "errorlog.h"
#include "strblock.h"



typedef struct geBodyInst
{
	const geBody			*BodyTemplate;
	geBodyInst_Geometry		 ExportGeometry;
	int						 LastLevelOfDetail;
	geBodyInst_Index		 FaceCount;
} geBodyInst;



void GENESISCC geBodyInst_PostScale(const geXForm3d *M,const geVec3d *S,geXForm3d *Scaled)
{
	Scaled->AX = M->AX * S->X;
	Scaled->BX = M->BX * S->X;
	Scaled->CX = M->CX * S->X;

	Scaled->AY = M->AY * S->Y;
	Scaled->BY = M->BY * S->Y;
	Scaled->CY = M->CY * S->Y;

	Scaled->AZ = M->AZ * S->Z;
	Scaled->BZ = M->BZ * S->Z;
	Scaled->CZ = M->CZ * S->Z;
	Scaled->Translation = M->Translation;
}


geBodyInst *GENESISCC geBodyInst_Create(const geBody *B)
{
	geBodyInst *BI;
	assert( B != NULL );
	assert( geBody_IsValid(B) != GE_FALSE );
	
	BI = GE_RAM_ALLOCATE_STRUCT(geBodyInst);
	if (BI == NULL)
		{
			geErrorLog_Add(ERR_BODY_ENOMEM, NULL);
			return NULL;
		}
	BI->BodyTemplate = B;
	{
		geBodyInst_Geometry *G = &(BI->ExportGeometry);
		G->SkinVertexCount =0;
		G->SkinVertexArray = NULL;
		
		G->NormalCount = 0;
		G->NormalArray = NULL;
		
		G->FaceCount = (geBody_Index) 0;
		G->FaceListSize = 0; 
		G->FaceList = NULL;
	}

	BI->LastLevelOfDetail   = -1;
	BI->FaceCount =  0;

	return BI;
}
			

void GENESISCC geBodyInst_Destroy( geBodyInst **BI)
{
	geBodyInst_Geometry *G;
	assert( BI != NULL );
	assert( *BI != NULL );
	G = &( (*BI)->ExportGeometry );
	if (G->SkinVertexArray != NULL )
		{
			geRam_Free( G->SkinVertexArray );
			G->SkinVertexArray = NULL;
		}
	if (G->NormalArray != NULL )
		{
			geRam_Free( G->NormalArray );
			G->NormalArray = NULL;
		}
	if (G->FaceList != NULL )
		{
			geRam_Free( G->FaceList );
			G->FaceList = NULL;
		}
	geRam_Free( *BI );
	*BI = NULL;
}



#define GE_BODYINST_FACELIST_SIZE_FOR_TRIANGLE (8)

static geBodyInst_Geometry *GENESISCC geBodyInst_GetGeometryPrep(	
	geBodyInst *BI, 
	int LevelOfDetail)
{
	const geBody *B;
	geBodyInst_Geometry *G;
	
	assert( BI != NULL );
	assert( geBody_IsValid(BI->BodyTemplate) != GE_FALSE );
	B = BI->BodyTemplate;

	G = &(BI->ExportGeometry);
	assert( G  != NULL );

	if (G->SkinVertexCount != B->XSkinVertexCount)
		{
			if (G->SkinVertexArray!=NULL)
				{
					geRam_Free(G->SkinVertexArray);
				}
			G->SkinVertexArray = GE_RAM_ALLOCATE_ARRAY(geBodyInst_SkinVertex,B->XSkinVertexCount);
			if ( G->SkinVertexArray == NULL )
				{
					geErrorLog_Add(ERR_BODY_ENOMEM, NULL);
					G->SkinVertexCount = 0;
					return NULL;
				}
			G->SkinVertexCount  = B->XSkinVertexCount;
		}

	if (G->NormalCount != B->SkinNormalCount)
		{
			if (G->NormalArray!=NULL)
				{
					geRam_Free(G->NormalArray);
				}
			G->NormalArray = GE_RAM_ALLOCATE_ARRAY( geVec3d,B->SkinNormalCount);
			if ( G->NormalArray == NULL )
				{
					geErrorLog_Add(ERR_BODY_ENOMEM, NULL);
					G->NormalCount = 0;
					return NULL;
				}
			G->NormalCount  = B->SkinNormalCount;
		}

	if (BI->FaceCount != B->SkinFaces[GE_BODY_HIGHEST_LOD].FaceCount)
		{
			if (G->FaceList!=NULL)
				{
					geRam_Free(G->FaceList);
				}
			G->FaceListSize = sizeof(geBody_Index) * 
					B->SkinFaces[GE_BODY_HIGHEST_LOD].FaceCount * 
					GE_BODYINST_FACELIST_SIZE_FOR_TRIANGLE;
			G->FaceList = GE_RAM_ALLOCATE_ARRAY(geBody_Index,
							B->SkinFaces[GE_BODY_HIGHEST_LOD].FaceCount * 
							GE_BODYINST_FACELIST_SIZE_FOR_TRIANGLE);
			if ( G->FaceList == NULL )
				{
					geErrorLog_Add(ERR_BODY_ENOMEM, NULL);
					BI->FaceCount = 0;
					return NULL;
				}
			BI->FaceCount = B->SkinFaces[GE_BODY_HIGHEST_LOD].FaceCount;
		}
	return G;
}

const geBodyInst_Geometry *GENESISCC geBodyInst_GetGeometry(
	const geBodyInst *BI, 
	const geVec3d *ScaleVector,
	const geXFArray *BoneTransformArray,
	int LevelOfDetail,
	const geCamera *Camera)
{
	geBodyInst_Geometry *G;
	const geBody *B;
	geXForm3d *BoneXFArray;
	int      BoneXFCount;
	geBody_Index BoneIndex;

	geBoolean GottaUpdateFaces = GE_FALSE;
	assert( BI != NULL );
	assert( BoneTransformArray != NULL );
	assert( geBody_IsValid(BI->BodyTemplate) != GE_FALSE );
	
	G = geBodyInst_GetGeometryPrep((geBodyInst *)BI,LevelOfDetail);
	if (G == NULL)
		{
			return NULL;
		}
		

	B = BI->BodyTemplate;

	BoneXFArray = geXFArray_GetElements(BoneTransformArray,&BoneXFCount);
	if ( BoneXFArray == NULL)
		{
			geErrorLog_Add(ERR_BODY_BONEXFARRAY, NULL);
			return NULL;
		}
	if (BoneXFCount != B->BoneCount)
		{	
			geErrorLog_Add(ERR_BODY_BONEXFARRAY, NULL);
			return NULL;
		}


	{	
		int i,LevelOfDetailBit;
	
		if (Camera != NULL)
			{
				// transform and project all appropriate points
				geBody_XSkinVertex *S;
				geBodyInst_SkinVertex  *D;
				LevelOfDetailBit = 1 << LevelOfDetail;
				BoneIndex = -1;  // S->BoneIndex won't ever be this.
				geVec3d_Set(&(G->Maxs), -GE_BODY_REALLY_BIG_NUMBER, -GE_BODY_REALLY_BIG_NUMBER, -GE_BODY_REALLY_BIG_NUMBER );
				geVec3d_Set(&(G->Mins), GE_BODY_REALLY_BIG_NUMBER, GE_BODY_REALLY_BIG_NUMBER, GE_BODY_REALLY_BIG_NUMBER );
				for (i=B->XSkinVertexCount,S=B->XSkinVertexArray,D=G->SkinVertexArray; 
					 i>0; 
					 i--,S++,D++)
					{
						geXForm3d ObjectToCamera;
						if (S->BoneIndex!=BoneIndex)
							{ //Keep XSkinVertexArray sorted by BoneIndex for best performance
								BoneIndex = S->BoneIndex;
								geXForm3d_Multiply(		geCamera_GetCameraSpaceXForm(Camera), 
														&(BoneXFArray[BoneIndex]),
														&ObjectToCamera);
								geBodyInst_PostScale(&ObjectToCamera,ScaleVector,&ObjectToCamera);
							}
						if ( S->LevelOfDetailMask && LevelOfDetailBit )
							{
								geVec3d *VecDestPtr = &(D->SVPoint);
								geXForm3d_Transform(  &(ObjectToCamera),
													&(S->XPoint),VecDestPtr);
								#ifdef ONE_OVER_Z_PIPELINE
								geCamera_ProjectZ( Camera, VecDestPtr, VecDestPtr);
								#else
								geCamera_Project( Camera, VecDestPtr, VecDestPtr);
								#endif
								D->SVU = S->XU;
								D->SVV = S->XV;
								if (VecDestPtr->X > G->Maxs.X ) G->Maxs.X = VecDestPtr->X;
								if (VecDestPtr->X < G->Mins.X ) G->Mins.X = VecDestPtr->X;
								if (VecDestPtr->Y > G->Maxs.Y ) G->Maxs.Y = VecDestPtr->Y;
								if (VecDestPtr->Y < G->Mins.Y ) G->Mins.Y = VecDestPtr->Y;
								if (VecDestPtr->Z > G->Maxs.Z ) G->Maxs.Z = VecDestPtr->Z;
								if (VecDestPtr->Z < G->Mins.Z ) G->Mins.Z = VecDestPtr->Z;
								D->ReferenceBoneIndex=BoneIndex;
							}
					}
			}
		else
			{
				// transform all appropriate points
				geBody_XSkinVertex *S;
				geBodyInst_SkinVertex  *D;
				LevelOfDetailBit = 1 << LevelOfDetail;
				BoneIndex = -1;  // S->BoneIndex won't ever be this.
				geVec3d_Set(&(G->Maxs), -GE_BODY_REALLY_BIG_NUMBER, -GE_BODY_REALLY_BIG_NUMBER, -GE_BODY_REALLY_BIG_NUMBER );
				geVec3d_Set(&(G->Mins), GE_BODY_REALLY_BIG_NUMBER, GE_BODY_REALLY_BIG_NUMBER, GE_BODY_REALLY_BIG_NUMBER );
				
				for (i=B->XSkinVertexCount,S=B->XSkinVertexArray,D=G->SkinVertexArray; 
					 i>0; 
					 i--,S++,D++)
					{
						geXForm3d ObjectToWorld;
						if (S->BoneIndex!=BoneIndex)
							{ //Keep XSkinVertexArray sorted by BoneIndex for best performance
								BoneIndex = S->BoneIndex;
								geBodyInst_PostScale(&BoneXFArray[BoneIndex],ScaleVector,&ObjectToWorld);

							}
						if ( S->LevelOfDetailMask && LevelOfDetailBit )
							{
								geVec3d *VecDestPtr = &(D->SVPoint);
								geXForm3d_Transform(  &(ObjectToWorld),
													&(S->XPoint),VecDestPtr);
								D->SVU = S->XU;
								D->SVV = S->XV;
								if (VecDestPtr->X > G->Maxs.X ) G->Maxs.X = VecDestPtr->X;
								if (VecDestPtr->X < G->Mins.X ) G->Mins.X = VecDestPtr->X;
								if (VecDestPtr->Y > G->Maxs.Y ) G->Maxs.Y = VecDestPtr->Y;
								if (VecDestPtr->Y < G->Mins.Y ) G->Mins.Y = VecDestPtr->Y;
								if (VecDestPtr->Z > G->Maxs.Z ) G->Maxs.Z = VecDestPtr->Z;
								if (VecDestPtr->Z < G->Mins.Z ) G->Mins.Z = VecDestPtr->Z;
								D->ReferenceBoneIndex=BoneIndex;
							}
					}
			}

			{
				geBody_Normal *S;
				geVec3d *D;
				// rotate all appropriate normals
				for (i=B->SkinNormalCount,S=B->SkinNormalArray,D=G->NormalArray;
					 i>0; 
					 i--,S++,D++)
					{
						if ( S->LevelOfDetailMask && LevelOfDetailBit )
							{
								geXForm3d_Rotate(&(BoneXFArray[S->BoneIndex]),
											   &(S->Normal),D);
							}
					}
			}

	}


	if (LevelOfDetail != BI->LastLevelOfDetail)
	{
		// build face list to export
		int i,j;
		geBody_Index Count;
		const geBody_Triangle *T;
		geBody_Index *D;
		Count = B->SkinFaces[LevelOfDetail].FaceCount;

		for (i=0,T=B->SkinFaces[LevelOfDetail].FaceArray,D=G->FaceList;
				i<Count; 
				i++,T++,B++)
			{
				*D = GE_BODYINST_FACE_TRIANGLE;
				D++;
				*D = T->MaterialIndex;
				D++;
				for (j=0; j<3; j++)
					{
						*D = T->VtxIndex[j];
						D++;
						*D = T->NormalIndex[j];
						D++;
					}
			}
		assert( ((uint32)D) - ((uint32)G->FaceList) == (uint32)(G->FaceListSize) );
		G->FaceCount = Count;
		((geBodyInst *)BI)->LastLevelOfDetail = LevelOfDetail;
	}



	return G;
}	

