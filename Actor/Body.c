/****************************************************************************************/
/*  Body.c                                                                              */
/*                                                                                      */
/*  Author: Mike Sandige                                                                */
/*  Description: Actor body implementation.                                             */
/*                                                                                      */
/*  Edit History:                                                                       */
/*  02/21/2004 Wendell Buckner                                                          */
/*   DOT3 BUMPMAPPING                                                                   */
/*  02/18/2004 Wendell Buckner                                                          */
/*   DOT3 BUMPMAPPING                                                                   */
/*  02/17/2004 Wendell Buckner                                                          */
/*   DOT3 BUMPMAPPING                                                                   */
/*  01/13/2004 Wendell Buckner                                                          */
/*   DOT3 BUMPMAPPING                                                                   */
/*  04/08/2003 Wendell Buckner 	                                                        */
/*   BUMPMAPPING                                                                        */
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
/*  Genesis3D Version 1.1 released November 15, 1999                                    */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved                            */
/*                                                                                      */
/****************************************************************************************/

#include <assert.h>						//assert()
#include <string.h>						//strlen(), strcpy()
#include <math.h> 						//fabs()
#include <stdio.h>						//sscanf

#include "Body.h"
#include "Body._h"
#include "Ram.h"
#include "ErrorLog.h"


#define MAX(aa,bb)   ( (aa)>(bb)?(aa):(bb) )
#define MIN(aa,bb)   ( (aa)<(bb)?(aa):(bb) )

/*	02/18/2004 Wendell Buckner
	DOT3 BUMPMAPPING */
geBoolean GENESISCC geBody_GetMaterialByName(const geBody *B, const char *FindMaterialName, int *MaterialIndex,
										const char **MaterialName,
										geBitmap **Bitmap, geFloat *Red, geFloat *Green, geFloat *Blue);

/*	02/17/2004 Wendell Buckner
	DOT3 BUMPMAPPING */
geBoolean GENESISCC geBody_CreateBumpmapByNameDot3(const geBody *B, const char *BumpmapName)
{
	int MaterialIndex = 0;
	const char *MaterialName = NULL;
	geBitmap *BaseBmp        = NULL;
	geBitmap *BumpBmp        = NULL;
	geBitmap *SpecularBmp    = NULL;
	geFloat Red   = 0.0f;
	geFloat Green = 0.0f;
	geFloat Blue  = 0.0f;
	char FindMaterialName[1024];
	geBoolean BumpmapCreated = GE_FALSE;

	do
	{
		if(!B) break;
		if(!BumpmapName) break;
		if(!geBitmap_IsBumpmapNameDot3(BumpmapName)) break;

		strcpy(FindMaterialName,BumpmapName);

		if(!geBody_GetMaterialByName(B, &FindMaterialName[0], &MaterialIndex, &MaterialName, &BaseBmp, &Red, &Green, &Blue)) break;

		strcpy(FindMaterialName,BumpmapName);
		memcpy(FindMaterialName,"BNB",3);

		if(!geBody_GetMaterialByName(B, &FindMaterialName[0], &MaterialIndex, &MaterialName, &BumpBmp, &Red, &Green, &Blue)) break;

		strcpy(FindMaterialName,BumpmapName);
		memcpy(FindMaterialName,"BNS",3);

		geBody_GetMaterialByName(B, &FindMaterialName[0], &MaterialIndex, &MaterialName, &SpecularBmp, &Red, &Green, &Blue);

		BumpmapCreated = geBitmap_CreateBumpmapDot3 ( BaseBmp, BumpBmp, SpecularBmp );
	}
	while(GE_FALSE);

	return BumpmapCreated;
}

/*	01/13/2004 Wendell Buckner
	DOT3 BUMPMAPPING */
geBoolean GENESISCC geBody_CreateTangentSpace( const geBody *B )
{
	geBody_Index VertexCount = B->XSkinVertexCount;

	assert( B      != NULL );
	assert( geBody_IsValid(B) != GE_FALSE );

	if ( B->XSkinTangentSpace ) return GE_FALSE;

	if ( !B->XSkinTangentSpace )
	{
		geBody_TangentSpace **tempTS;
		tempTS = (geBody_TangentSpace **) &B->XSkinTangentSpace;
		*tempTS = GE_RAM_ALLOCATE_ARRAY ( geBody_TangentSpace, VertexCount );
	}

	if ( !B->XSkinTangentSpace ) return GE_FALSE;

	geBody_Index FaceCount = B->SkinFaces[GE_BODY_HIGHEST_LOD].FaceCount;
	const geBody_Triangle     *SF = B->SkinFaces[GE_BODY_HIGHEST_LOD].FaceArray;
	const geBody_XSkinVertex  *SV = B->XSkinVertexArray;
	const geBody_Normal       *SN = B->SkinNormalArray;

	memset(B->XSkinTangentSpace,0,sizeof(geBody_TangentSpace) * VertexCount );

	{
		geBody_TangentSpace       *TS = B->XSkinTangentSpace;
		geBody_Index i;
		geBody_Index j;
		geBody_Index k;

#ifdef _DEBUG // changed QD bug fix
		{
			FILE *fp = fopen("c:\\cube.txt","at");

			if(fp)
			{
				fprintf(fp,"Vertex List\n");

				for ( i= 0; i < VertexCount; i++ )
					fprintf(fp,"%i,  X = %f Y = %f Z= %f U = %f V = %f \n", i, SV[i].XPoint.X,SV[i].XPoint.Y,SV[i].XPoint.Z,SV[i].XU,SV[i].XV);

				fprintf(fp,"Normal List\n");

				for ( i= 0; i < B->SkinNormalCount; i++ )
					fprintf(fp,"%i,  X = %f Y = %f Z = %f\n", i, SN[i].Normal.X,SN[i].Normal.Y,SN[i].Normal.Z);

				for ( j = 0; j < FaceCount; j++ )
					fprintf(fp,"%i,  V1 = %i V2 = %i V3 = %i N1 = %i N2 = %i N3 = %i\n",j, SF[j].VtxIndex[0],SF[j].VtxIndex[1],SF[j].VtxIndex[2],SF[j].NormalIndex[0],SF[j].NormalIndex[1],SF[j].NormalIndex[2]);

				fclose(fp);
			}
		}
#endif // end change

		for ( i= 0; i < VertexCount; i++ )
		{
			geBoolean TangentSpaceCreated = GE_FALSE;

			for ( j = 0; j < FaceCount; j++ )
			{
				for ( k = 0; k < 3; k++)
				{
					if ( i  != SF[j].VtxIndex[k] ) continue;

//Compute tangent space for the vertex...

					{
						geBody_Index v1 = SF[j].VtxIndex[0];
						geBody_Index v2 = SF[j].VtxIndex[1];
						geBody_Index v3 = SF[j].VtxIndex[2];
						const geBody_XSkinVertex *p1 = &SV[v1];
						const geBody_XSkinVertex *p2 = &SV[v2];
						const geBody_XSkinVertex *p3 = &SV[v3];
						geFloat p1U = (geFloat) fabs(p1->XU) + 10.0f;
						geFloat p1V = (geFloat) fabs(p1->XV) + 10.0f;
						geFloat p2U = (geFloat) fabs(p2->XU) + 10.0f;
						geFloat p2V = (geFloat) fabs(p2->XV) + 10.0f;
						geFloat p3U = (geFloat) fabs(p3->XU) + 10.0f;
						geFloat p3V = (geFloat) fabs(p3->XV) + 10.0f;
						geBody_Index n1 = SF[j].NormalIndex[k];
						const geBody_Normal *BodyNormal = &SN[n1];
						const geVec3d *Normal = &BodyNormal->Normal;
						geBody_Index vt = SF[j].VtxIndex[k];
						geBody_TangentSpace *TSP1 = &TS[vt];
						geVec3d AlignedVector;
						geVec3d BaseVector1;
						geVec3d BaseVector2;
						geVec3d Tangent;
						geVec3d Binormal;
						geFloat UV[2];

						geVec3d_Subtract(&p2->XPoint,&p1->XPoint,&BaseVector1);
						geVec3d_Subtract(&p3->XPoint,&p1->XPoint,&BaseVector2);

						if ( (p2U == p1U) || (p3U == p1U) )
						{
							if ( p2U == p1U )
							{
								UV[0] = ( p1V < p2V ) ?  1.0f : -1.0f;
								UV[1] = 0.0f;
							}
							else if	( p3U == p1U )
							{
								UV[0] = 0.0f;
								UV[1] = ( p1V < p3V ) ?  -1.0f : 1.0f;
							}
						}
						else
						{
							UV[0] = -p1U/(p2U-p1U);
							UV[1] = -p1U/(p3U-p1U);

							if ( UV[0] * UV[1] < 0.0f )
							{
								UV[0] = -UV[0];
								UV[1] = -UV[1];
							}
						}

						geVec3d_Scale(&BaseVector2,UV[1],&BaseVector2);
						geVec3d_Scale(&BaseVector1,UV[0],&BaseVector1);
						geVec3d_Subtract(&BaseVector2,&BaseVector1,&AlignedVector);

						geVec3d_CrossProduct(&AlignedVector,Normal,&Tangent);
						geVec3d_CrossProduct(&Tangent,Normal,&Binormal);

						if ( ( (p1U-p3U) * (p2V-p3V) ) > ( (p2U-p3U) *(p1V - p3V) ) )
							geVec3d_Scale(&Binormal,-1.0f,&Binormal);

						geVec3d_Normalize(&Tangent);
						geVec3d_Normalize(&Binormal);

						geVec3d_Set(&TSP1->T,Tangent.X,Tangent.Y,Tangent.Z);
						geVec3d_Set(&TSP1->B,Binormal.X,Binormal.Y,Binormal.Z);
						geVec3d_Set(&TSP1->N,Normal->X,Normal->Y,Normal->Z);

						TangentSpaceCreated = GE_TRUE;
					}

//Compute tangent space for the vertex...

					if ( TangentSpaceCreated ) break;
				}

				if ( TangentSpaceCreated ) break;
			}

		}
	}

	return GE_TRUE;
}

/*	02/21/2004 Wendell Buckner
	DOT3 BUMPMAPPING */
void GENESISCC geBody_DestroyTangentSpace( geBody *B )
{
 assert( B      != NULL );
 assert( geBody_IsValid(B) != GE_FALSE );

 if ( !B->XSkinTangentSpace ) return;

 if ( B->XSkinTangentSpace ) geRam_Free (B->XSkinTangentSpace);
}

void GENESISCC geBody_SetVertexColorDot3( geVec3d LightPosition, const geXForm3d *Mdl2WldXFA, const geBody *B, int16 VertexIndex, geFloat *ColorDot3, int16 LightType, geBoolean *Reset )
{
	geBoolean NewLightPosition = GE_FALSE;
	geBoolean NewMsLightPosition = GE_FALSE;
	geBoolean NewMdl2Wld = GE_FALSE;
	geBoolean NewV = GE_FALSE;
	geBoolean NewTgt2Mdl = GE_FALSE;
	geBoolean NewB = GE_FALSE;
	geBoolean NewLT = GE_FALSE;
	static geBody_Index BoneIndex;
	static int16 OldVertexIndex;
	static geVec3d Position;
	static geVec3d msLightPosition;
	static geVec3d tsLightPosition;
	static geVec3d tempv;
	static geVec3d v;
	static geXForm3d Mdl2Wld;
	static geXForm3d Mdl2Tgt;
	static geXForm3d invWld2Mdl;
	static const geBody *tempB;
	const geBody_TangentSpace *Mdl2TgtTS;
	const geBody_XSkinVertex *XVA;

	if ( !B->XSkinTangentSpace ) return;

	#define geDirectionalLightDot3 0
	#define gePointLightDot3       1

	if ( LightType < geDirectionalLightDot3 ) LightType = 0;
	if ( LightType > gePointLightDot3)        LightType = 1;

	Mdl2TgtTS = B->XSkinTangentSpace;

	XVA = B->XSkinVertexArray;

	NewB = ( B != tempB ) || *Reset;

	if ( NewB )
	{
		BoneIndex = -1;
		OldVertexIndex = -1;
		geVec3d_Clear(&Position);
		geVec3d_Clear(&msLightPosition);
		geVec3d_Clear(&tsLightPosition);
		geVec3d_Clear(&tempv);
		geVec3d_Clear(&v);
		geXForm3d_SetIdentity(&Mdl2Wld);
		geXForm3d_SetIdentity(&Mdl2Tgt);
		geXForm3d_SetIdentity(&invWld2Mdl);
		tempB = B;
	}

//1. Compute model space light vector

//	a. Get light position
	NewLightPosition = (!geVec3d_Compare(&Position,&LightPosition,0.05f));

	if ( NewLightPosition )
		geVec3d_Set(&Position,LightPosition.X, LightPosition.Y,LightPosition.Z);

//	b. Compute model space
	NewMdl2Wld = ( BoneIndex != XVA[VertexIndex].BoneIndex ) || NewB;

	if ( NewMdl2Wld  )
	{
		BoneIndex = XVA[VertexIndex].BoneIndex;
		geXForm3d_Copy(&Mdl2WldXFA[BoneIndex],&Mdl2Wld);
		geXForm3d_GetTranspose (&Mdl2Wld,&invWld2Mdl);
	}

	NewMsLightPosition = ( NewLightPosition || NewMdl2Wld );

	if (NewMsLightPosition )
	{
		geXForm3d_Transform (&invWld2Mdl, &Position, &msLightPosition);
	}

//	c. multiply model space by the vertex
//	NewV = ( !geVec3d_Compare(&tempv,&XVA[VertexIndex].XPoint,0.05f) );
	NewV = ( OldVertexIndex != VertexIndex ) || NewMsLightPosition;

	if ( NewV )
	{
		OldVertexIndex = VertexIndex;
	}

	NewLT = NewV && ( LightType == gePointLightDot3 );

	if ( NewLT )
	{
		geVec3d_Set(&tempv,XVA[VertexIndex].XPoint.X, XVA[VertexIndex].XPoint.Y,XVA[VertexIndex].XPoint.Z);
		geVec3d_Set(&v,tempv.X, tempv.Y,tempv.Z);
//TODO: point light and directional light don't seem much different, but in powervr they are... hhmmm...
		geVec3d_Subtract(&msLightPosition,&v,&msLightPosition);
	}

	NewTgt2Mdl = ( NewV );

	if ( NewTgt2Mdl )
	{
		Mdl2Tgt.AX = Mdl2TgtTS[VertexIndex].T.X;
		Mdl2Tgt.AY = Mdl2TgtTS[VertexIndex].T.Y;
		Mdl2Tgt.AZ = Mdl2TgtTS[VertexIndex].T.Z;

		Mdl2Tgt.BX = Mdl2TgtTS[VertexIndex].B.X;
		Mdl2Tgt.BY = Mdl2TgtTS[VertexIndex].B.Y;
		Mdl2Tgt.BZ = Mdl2TgtTS[VertexIndex].B.Z;

		Mdl2Tgt.CX = Mdl2TgtTS[VertexIndex].N.X;
		Mdl2Tgt.CY = Mdl2TgtTS[VertexIndex].N.Y;
		Mdl2Tgt.CZ = Mdl2TgtTS[VertexIndex].N.Z;
	}

//2. Compute tangent space light vector
	if ( NewTgt2Mdl || NewMsLightPosition)
	{
		geVec3d_Normalize(&msLightPosition);
		geXForm3d_RotateNoOrthogonal (&Mdl2Tgt, &msLightPosition, &tsLightPosition);
	}

//3. Convert light vector to color
	ColorDot3[0] = tsLightPosition.X * 127.5f + 127.5f;
	ColorDot3[1] = tsLightPosition.Y * 127.5f + 127.5f;
	ColorDot3[2] = tsLightPosition.Z * 127.5f + 127.5f;
	ColorDot3[3] = 0;

	*Reset = GE_FALSE;
}

/* 04/08/2003 Wendell Buckner
    BUMPMAPPING */

//****************************************************************************************

geBoolean GENESISCC geBody_GetMaterialByName(const geBody *B, const char *FindMaterialName, int *MaterialIndex,
										const char **MaterialName,
										geBitmap **Bitmap, geFloat *Red, geFloat *Green, geFloat *Blue)
{
	geBoolean MaterialNameFound = GE_FALSE;

	assert( B      != NULL );
	assert( geBody_IsValid(B) != GE_FALSE );
	assert( Red    != NULL );
	assert( Green  != NULL );
	assert( Blue   != NULL );
	assert( Bitmap != NULL );
	assert( MaterialIndex >= 0 );
	assert( *MaterialIndex < B->MaterialCount );
	assert( MaterialName != NULL );

	for(*MaterialIndex=0; *MaterialIndex < B->MaterialCount; (*MaterialIndex)++)
	{
		*MaterialName = geStrBlock_GetString(B->MaterialNames,*MaterialIndex);
		if(memcmp(*MaterialName,FindMaterialName,strlen(FindMaterialName)) == 0)
		{
			geBody_Material *M = &(B->MaterialArray[*MaterialIndex]);
			*Bitmap = M->Bitmap;
			*Red    = M->Red;
			*Green  = M->Green;
			*Blue   = M->Blue;
			MaterialNameFound = GE_TRUE;
			break;
		}
	}

	return MaterialNameFound;
}

geBitmap * GENESISCC geBody_CreateBumpmapByName(const geBody *B, const char *BumpmapName, gePixelFormat BumpFormat)
{
	int MaterialIndex = 0;
	const char *MaterialName = NULL;
	geBitmap *BaseBmp        = NULL;
	geBitmap *BumpBmp        = NULL;
	geBitmap *SpecularBmp    = NULL;
	geFloat Red   = 0.0f;
	geFloat Green = 0.0f;
	geFloat Blue  = 0.0f;
	char FindMaterialName[1024];
	geBitmap * BumpmapCreated = NULL;

	do
	{
		if(!B) break;
		if(!BumpmapName) break;
		if(!geBitmap_IsBumpmapName(BumpmapName)) break;

		strcpy(FindMaterialName,BumpmapName);

		if(!geBody_GetMaterialByName(B, &FindMaterialName[0], &MaterialIndex, &MaterialName, &BaseBmp, &Red, &Green, &Blue)) break;

		strcpy(FindMaterialName,BumpmapName);
		memcpy(FindMaterialName,"BMB",3);

		if(!geBody_GetMaterialByName(B, &FindMaterialName[0], &MaterialIndex, &MaterialName, &BumpBmp, &Red, &Green, &Blue)) break;

		strcpy(FindMaterialName,BumpmapName);
		memcpy(FindMaterialName,"BMS",3);

		geBody_GetMaterialByName(B, &FindMaterialName[0], &MaterialIndex, &MaterialName, &SpecularBmp, &Red, &Green, &Blue);

		BumpmapCreated = geBitmap_CreateBumpmap ( BaseBmp, BumpBmp, SpecularBmp, BumpFormat );
	}
	while(GE_FALSE);

	return BumpmapCreated;
}

geBitmap * GENESISCC geBody_GetBumpMapAltByName(const geBody *B, const char *BumpmapName)
{
	int MaterialIndex = 0;
	const char *MaterialName = NULL;
	geBitmap *BaseBmp        = NULL;
	geBitmap *BumpBmpAlt     = NULL;
	geFloat Red   = 0.0f;
	geFloat Green = 0.0f;
	geFloat Blue  = 0.0f;
	char FindMaterialName[1024];

	do
	{
		if(!B) break;
		if(!BumpmapName) break;
		if(!geBitmap_IsBumpmapName(BumpmapName)) break;

		strcpy(FindMaterialName,BumpmapName);

		if(!geBody_GetMaterialByName(B, &FindMaterialName[0], &MaterialIndex, &MaterialName, &BaseBmp, &Red, &Green, &Blue)) break;

		BumpBmpAlt = geBitmap_GetBumpMapAlt ( BaseBmp );
	}
	while(GE_FALSE);

	return BumpBmpAlt;
}

//****************************************************************************************


#if defined(DEBUG) || !defined(NDEBUG)
static geBoolean GENESISCC geBody_SanityCheck(const geBody *B)
{
	int i,j,k;
	int Lod,FaceCount,VertexCount,NormalCount,BoneCount;
	geBody_XSkinVertex *SV;
	geBody_Bone *Bone;
	geBody_Normal *N;

	Lod = B->LevelsOfDetail;
	VertexCount = B->XSkinVertexCount;
	NormalCount = B->SkinNormalCount;
	BoneCount   = B->BoneCount;

	if (B->MaterialNames == NULL )
		return GE_FALSE;
	if (B->MaterialCount != geStrBlock_GetCount(B->MaterialNames))
		return GE_FALSE;

	if (B->BoneNames == NULL)
		return GE_FALSE;
	if (B->BoneCount != geStrBlock_GetCount(B->BoneNames))
		return GE_FALSE;

	if ((B->XSkinVertexArray == NULL) && (B->XSkinVertexCount>0))
		return GE_FALSE;
	if ((B->SkinNormalArray == NULL) && (B->SkinNormalCount>0))
		return GE_FALSE;
	if ((B->BoneArray == NULL) && (B->BoneCount>0))
		return GE_FALSE;
	if ((B->MaterialArray == NULL) && (B->MaterialCount>0))
		return GE_FALSE;


	for (i=0; i<Lod; i++)
	{
		geBody_Triangle *F;
		FaceCount = B->SkinFaces[i].FaceCount;
		for (j=0,F=B->SkinFaces[i].FaceArray; j<FaceCount; j++,F++)
		{
			for (k=0; k<3; k++)
			{
				if ((F->VtxIndex[k]    < 0) || (F->VtxIndex[k]    >= VertexCount  ))
					return GE_FALSE;
				if ((F->NormalIndex[k] < 0) || (F->NormalIndex[k] >= NormalCount  ))
					return GE_FALSE;
				if ((F->MaterialIndex  < 0) || (F->MaterialIndex  >= B->MaterialCount))
					return GE_FALSE;
			}
		}
	}
	for (i=0,SV = B->XSkinVertexArray; i<VertexCount; i++,SV++)
	{
		if ((SV->BoneIndex < 0) || (SV->BoneIndex >= BoneCount))
			return GE_FALSE;
	}

	for (i=0,N = B->SkinNormalArray; i<NormalCount; i++,N++)
	{
		if ((N->BoneIndex < 0) || (N->BoneIndex >= BoneCount))
			return GE_FALSE;
	}

	for (i=0,Bone = B->BoneArray; i<BoneCount; i++,Bone++)
	{
		if (Bone->ParentBoneIndex != GE_BODY_NO_PARENT_BONE)
		{
			if ((Bone->ParentBoneIndex < 0) || (Bone->ParentBoneIndex > i))
				return GE_FALSE;
		}
	}

	return GE_TRUE;

}
#endif


geBoolean GENESISCC geBody_IsValid(const geBody *B)
{
	if ( B == NULL )
		return GE_FALSE;
	if ( B -> IsValid != B )
		return GE_FALSE;
	assert( geBody_SanityCheck(B) != GE_FALSE) ;
	return GE_TRUE;
}


static geBody *GENESISCC geBody_CreateNull(void)
{
	geBody *B;
	int i;

	B = GE_RAM_ALLOCATE_STRUCT(geBody);
	if ( B == NULL)
	{
		geErrorLog_Add(ERR_BODY_ENOMEM, NULL);
		return NULL;
	}
	B->IsValid          = NULL;
	B->XSkinVertexCount	= 0;
	B->XSkinVertexArray	= NULL;

	B->SkinNormalCount	= 0;
	B->SkinNormalArray	= NULL;

	B->BoneCount		= 0;
	B->BoneArray		= NULL;
	B->BoneNames		= NULL;

	B->MaterialCount	= 0;
	B->MaterialArray	= NULL;
	B->MaterialNames	= NULL;
	for (i=0; i<GE_BODY_NUMBER_OF_LOD; i++)
	{
		B->SkinFaces[i].FaceCount = 0;
		B->SkinFaces[i].FaceArray = NULL;
	}
	B->LevelsOfDetail = 1;
	B->IsValid = B;

	geVec3d_Set(&(B->BoundingBoxMin),0.0f,0.0f,0.0f);
	geVec3d_Set(&(B->BoundingBoxMax),0.0f,0.0f,0.0f);

/*	01/13/2004 Wendell Buckner                                                          */
/*   DOT3 BUMPMAPPING                                                                   */
	B->XSkinTangentSpace = NULL;

	return B;
}

static void GENESISCC geBody_DestroyPossiblyIncompleteBody( geBody **PB )
{
	geBody *B;
	int i;

	B = *PB;
	B->IsValid = NULL;
	if (B->XSkinVertexArray != NULL)
	{
		geRam_Free( B->XSkinVertexArray );
		B->XSkinVertexArray = NULL;
	}
	if (B->SkinNormalArray != NULL)
	{
		geRam_Free( B->SkinNormalArray );
		B->SkinNormalArray = NULL;
	}
	if (B->BoneNames != NULL)
	{
		geStrBlock_Destroy(&(B->BoneNames));
		B->BoneNames = NULL;
	}
	if (B->BoneArray != NULL)
	{
		geRam_Free(B->BoneArray);
		B->BoneArray = NULL;
	}
	if (B->MaterialArray != NULL)
	{
		for (i=0; i<B->MaterialCount; i++)
		{
			// <> CB ; see note above
			// this doesn't seem to prevent us from crashing here
			//	when an actor has an error during _Create
			if ( (uint32)(B->MaterialArray[i].Bitmap) > 1 )
				geBitmap_Destroy(&(B->MaterialArray[i].Bitmap));
			B->MaterialArray[i].Bitmap = NULL;
		}
		geRam_Free( B->MaterialArray );
		B->MaterialArray = NULL;
	}
	if (B->MaterialNames != NULL)
	{
		geStrBlock_Destroy(&(B->MaterialNames));
		B->MaterialNames = NULL;
	}

	for (i=0; i<GE_BODY_NUMBER_OF_LOD; i++)
	{
		if (B->SkinFaces[i].FaceArray != NULL)
		{
			geRam_Free(B->SkinFaces[i].FaceArray);
			B->SkinFaces[i].FaceArray = NULL;
		}
	}
	geRam_Free(*PB);
	*PB = NULL;
}

geBody *GENESISCC geBody_Create(void)
{
	geBody *B;

	B = geBody_CreateNull();
	if ( B == NULL)
	{
		geErrorLog_Add(ERR_BODY_ENOMEM, NULL);
		return NULL;
	}

	B->BoneNames = geStrBlock_Create();
	if (B->BoneNames == NULL)
	{
		geErrorLog_Add(ERR_BODY_ENOMEM, NULL);
		geBody_DestroyPossiblyIncompleteBody(&B);
		return NULL;
	}
	B->MaterialNames	= geStrBlock_Create();

	if (B->MaterialNames == NULL)
	{
		geErrorLog_Add(ERR_BODY_ENOMEM, NULL);
		geBody_DestroyPossiblyIncompleteBody(&B);
		return NULL;
	}

	assert( geBody_SanityCheck(B) != GE_FALSE );
	return B;
}

void GENESISCC geBody_Destroy(geBody **PB)
{
	assert(  PB != NULL );
	assert( *PB != NULL );
	assert( geBody_IsValid(*PB) != GE_FALSE );
	geBody_DestroyPossiblyIncompleteBody( PB );
}


geBoolean GENESISCC geBody_GetGeometryStats(const geBody *B, int lod, int *Vertices, int *Faces, int *Normals)
{
	assert( geBody_IsValid(B) == GE_TRUE );
	assert( ( lod >=0 ) && ( lod < GE_BODY_NUMBER_OF_LOD ) );
	*Vertices = B->XSkinVertexCount;
	*Faces    = B->SkinFaces[lod].FaceCount;
	*Normals  = B->SkinNormalCount;
	return GE_TRUE;
}



int GENESISCC geBody_GetBoneCount(const geBody *B)
{
	assert( B != NULL );
	assert( geBody_IsValid(B) != GE_FALSE );
	return B->BoneCount;
}

void GENESISCC geBody_GetBone(const geBody *B,
	int BoneIndex,
	const char **BoneName,
	geXForm3d *Attachment,
	int *ParentBoneIndex)
{
	assert( B != NULL );
	assert( geBody_IsValid(B) != GE_FALSE );
	assert( Attachment != NULL );
	assert( ParentBoneIndex != NULL );
	assert(  BoneName != NULL );

	assert( BoneIndex >=0 );
	assert( BoneIndex < B->BoneCount );
	*Attachment = B->BoneArray[BoneIndex].AttachmentMatrix;
	*ParentBoneIndex = B->BoneArray[BoneIndex].ParentBoneIndex;
	*BoneName = geStrBlock_GetString(B->BoneNames,BoneIndex);
}

int32 GENESISCC geBody_GetBoneNameChecksum(const geBody *B)
{
	assert( geBody_IsValid(B) != GE_FALSE );

	if (B->BoneNames != NULL)
	{
		return geStrBlock_GetChecksum( B->BoneNames );
	}
	else
		return 0;
}


geBoolean GENESISCC geBody_GetBoundingBox( const geBody *B,
							int BoneIndex,
							geVec3d *MinimumBoxCorner,
							geVec3d *MaximumBoxCorner)
{
	assert( B != NULL);
	assert( MinimumBoxCorner != NULL );
	assert( MaximumBoxCorner != NULL );
	assert( (BoneIndex >=0)            || (BoneIndex == GE_BODY_ROOT));
	assert( (BoneIndex < B->BoneCount) || (BoneIndex == GE_BODY_ROOT));
	if (BoneIndex == GE_BODY_ROOT)
	{
	#pragma message ("discontinue this?")
		*MinimumBoxCorner = B->BoundingBoxMin;
		*MaximumBoxCorner = B->BoundingBoxMax;
	}
	else
	{
		geBody_Bone *Bone = &(B->BoneArray[BoneIndex]);

		if (Bone->BoundingBoxMin.X > Bone->BoundingBoxMax.X)
		{
			return GE_FALSE;
		}
		*MinimumBoxCorner = Bone->BoundingBoxMin;
		*MaximumBoxCorner = Bone->BoundingBoxMax;
	}
	return GE_TRUE;
}

void GENESISCC geBody_SetBoundingBox( geBody *B,
							int BoneIndex,
							const geVec3d *MinimumBoxCorner,
							const geVec3d *MaximumBoxCorner)
{
	assert( B != NULL);
	assert( MinimumBoxCorner != NULL );
	assert( MaximumBoxCorner != NULL );
	assert( (BoneIndex >=0)            || (BoneIndex == GE_BODY_ROOT));
	assert( (BoneIndex < B->BoneCount) || (BoneIndex == GE_BODY_ROOT));
	if (BoneIndex == GE_BODY_ROOT)
	{
		B->BoundingBoxMin = *MinimumBoxCorner;
		B->BoundingBoxMax = *MaximumBoxCorner;
	}
	else
	{
		B->BoneArray[BoneIndex].BoundingBoxMin = *MinimumBoxCorner;
		B->BoneArray[BoneIndex].BoundingBoxMax = *MaximumBoxCorner;
	}
}




geBoolean GENESISCC geBody_GetBoneByName(const geBody* B,
	const char* BoneName,
	int* pBoneIndex,
	geXForm3d* Attachment,
	int* pParentBoneIndex)
{
	assert( B != NULL );
	assert( geBody_IsValid(B) != GE_FALSE );
	assert( Attachment != NULL );
	assert( pParentBoneIndex != NULL );
	assert( pBoneIndex != NULL );
	assert(  BoneName != NULL );

	if(geStrBlock_FindString(B->BoneNames, BoneName, pBoneIndex) == GE_TRUE)
	{
		*Attachment = B->BoneArray[*pBoneIndex].AttachmentMatrix;
		*pParentBoneIndex = B->BoneArray[*pBoneIndex].ParentBoneIndex;

		return GE_TRUE;
	}

	return GE_FALSE;
}

int GENESISCC geBody_GetMaterialCount(const geBody *B)
{
	assert( B != NULL );
	assert( geBody_IsValid(B) != GE_FALSE );
	return B->MaterialCount;
}

#define GE_BODY_TOLERANCE (0.001f)

static geBoolean GENESISCC geBody_XSkinVertexCompare(
	const geBody_XSkinVertex *SV1,
	const geBody_XSkinVertex *SV2)
{
	assert( SV1 != NULL );
	assert( SV2 != NULL );
	if (geVec3d_Compare( &(SV1->XPoint), &(SV2->XPoint),
						GE_BODY_TOLERANCE) == GE_FALSE)
	{
		return GE_FALSE;
	}
	if (geVec3d_Compare( &(SV1->XPoint), &(SV2->XPoint),
						GE_BODY_TOLERANCE) == GE_FALSE)
	{
		return GE_FALSE;
	}
	if (fabs(SV1->XU - SV2->XU) > GE_BODY_TOLERANCE)
	{
		return GE_FALSE;
	}
	if (fabs(SV1->XV - SV2->XV) > GE_BODY_TOLERANCE)
	{
		return GE_FALSE;
	}
	return GE_TRUE;
}


static void GENESISCC geBody_SwapVertexIndices( geBody *B, geBody_Index Index1, geBody_Index Index2)
	// zips through all triangles, and swaps index1 and index2.
{
	int i,j,lod;
	geBody_Index Count;
	geBody_Triangle *T;

	assert( B!=NULL );
	for (lod = 0; lod< GE_BODY_NUMBER_OF_LOD; lod++)
	{
		Count = B->SkinFaces[lod].FaceCount;
		for (i=0,T=B->SkinFaces[lod].FaceArray;
				i<Count;
				i++,T++)
		{
			for (j=0; j<3; j++)
			{
				if (T->VtxIndex[j] == Index1)
				{
					T->VtxIndex[j] = Index2;
				}
				else
				{
					if (T->VtxIndex[j] == Index2)
					{
						T->VtxIndex[j] = Index1;
					}
				}
			}
		}
	}
}

static void GENESISCC geBody_ChangeVertexIndex( geBody *B, geBody_Index Index1, geBody_Index Index2)
	// zips through all triangles, and changes index1 to index2.
{
	int i,j,lod;
	geBody_Index Count;
	geBody_Triangle *T;

	assert( B!=NULL );
	for (lod = 0; lod< GE_BODY_NUMBER_OF_LOD; lod++)
	{
		Count = B->SkinFaces[lod].FaceCount;
		for (i=0,T=B->SkinFaces[lod].FaceArray;
				i<Count;
				i++,T++)
		{
			for (j=0; j<3; j++)
			{
				if (T->VtxIndex[j] == Index1)
				{
					T->VtxIndex[j] = Index2;
				}
			}
		}
	}
}

static void GENESISCC geBody_SortSkinVertices( geBody *B )
{
	int i,j;
	int Count;
	geBoolean AnyChanges = GE_FALSE;
	assert( B != NULL );

	Count = B->XSkinVertexCount;
	for (i=0; i<Count; i++)
	{
		for (j=0; j<Count-1; j++)
		{
			if (B->XSkinVertexArray[j].BoneIndex > B->XSkinVertexArray[j+1].BoneIndex)
			{
				geBody_XSkinVertex Swap;

				Swap= B->XSkinVertexArray[j];
				B->XSkinVertexArray[j] = B->XSkinVertexArray[j+1];
				B->XSkinVertexArray[j+1] = Swap;
				geBody_SwapVertexIndices(B,(geBody_Index)j,(geBody_Index)(j+1));
				AnyChanges = GE_TRUE;
			}
		}
		if (AnyChanges != GE_TRUE)
		{
			break;
		}
		AnyChanges = GE_FALSE;
	}
}


static geBoolean GENESISCC geBody_AddSkinVertex(	geBody *B,
	const geVec3d *Vertex,
	geFloat U, geFloat V,
	geBody_Index BoneIndex,
	geBody_Index *Index)
{
	geBody_Bone *Bone;
	geBody_XSkinVertex *SV;
	geBody_XSkinVertex NewSV;
	int i;
	assert( B != NULL );
	assert( Vertex != NULL );
	assert( Index != NULL );
	assert( geBody_IsValid(B) != GE_FALSE );

	assert( B->XSkinVertexCount+1 > 0 );

	NewSV.XPoint = *Vertex;
	NewSV.XU     =  U;
	NewSV.XV     =  V;
	NewSV.LevelOfDetailMask = GE_BODY_HIGHEST_LOD_MASK;
	NewSV.BoneIndex = BoneIndex;

	assert( B->BoneCount > BoneIndex );
	Bone = &(B->BoneArray[BoneIndex]);


	// see if new Vertex is alreay in XSkinVertexArray
	for (i=0; i<B->XSkinVertexCount; i++)
	{
		SV = &(B->XSkinVertexArray[i]);
		if (SV->BoneIndex == BoneIndex)
		{
			if (geBody_XSkinVertexCompare(SV,&NewSV) == GE_TRUE )
			{
				*Index = (geBody_Index)i;
				return GE_TRUE;
			}
		}
	}
	// new Vertex needs to be added to XSkinVertexArray
	SV = GE_RAM_REALLOC_ARRAY( B->XSkinVertexArray ,
					geBody_XSkinVertex, (B->XSkinVertexCount + 1) );
	if ( SV == NULL )
	{
		geErrorLog_Add(ERR_BODY_ENOMEM, NULL);
		return GE_FALSE;
	}
	B->XSkinVertexArray = SV;

	B->XSkinVertexArray[B->XSkinVertexCount] = NewSV;
	*Index = B->XSkinVertexCount;

	Bone->BoundingBoxMin.X = MIN(Bone->BoundingBoxMin.X,NewSV.XPoint.X);
	Bone->BoundingBoxMin.Y = MIN(Bone->BoundingBoxMin.Y,NewSV.XPoint.Y);
	Bone->BoundingBoxMin.Z = MIN(Bone->BoundingBoxMin.Z,NewSV.XPoint.Z);
	Bone->BoundingBoxMax.X = MAX(Bone->BoundingBoxMax.X,NewSV.XPoint.X);
	Bone->BoundingBoxMax.Y = MAX(Bone->BoundingBoxMax.Y,NewSV.XPoint.Y);
	Bone->BoundingBoxMax.Z = MAX(Bone->BoundingBoxMax.Z,NewSV.XPoint.Z);

	B->XSkinVertexCount ++ ;
	return GE_TRUE;
}

static geBoolean GENESISCC geBody_AddNormal( geBody *B,
		const geVec3d *Normal,
		geBody_Index BoneIndex,
		geBody_Index *Index )
{
	geBody_Normal *NewNormalArray;
	geBody_Normal *N;
	geVec3d NNorm;
	int i;

	assert(      B != NULL );
	assert( Normal != NULL );
	assert(  Index != NULL );
	assert( geBody_IsValid(B) != GE_FALSE );

	assert( B->SkinNormalCount+1 > 0 );
	NNorm = *Normal;
	geVec3d_Normalize(&NNorm);
	// see if new normal is alreay in SkinNormalArray
	for (i=0, N = B->SkinNormalArray; i<B->SkinNormalCount; i++,N++)
	{
		if (N->BoneIndex == BoneIndex)
		{
			if ( geVec3d_Compare( &(N->Normal),&NNorm,GE_BODY_TOLERANCE ) == GE_TRUE )
			{
				*Index = (geBody_Index)i;
				return GE_TRUE;
			}
		}
	}

	//  new normal needs to be added to SkinNormalArray
	NewNormalArray = GE_RAM_REALLOC_ARRAY( B->SkinNormalArray,
						geBody_Normal,(B->SkinNormalCount+1));
	if (NewNormalArray == NULL)
	{
		geErrorLog_Add(ERR_BODY_ENOMEM, NULL);
		return GE_FALSE;
	}
	B->SkinNormalArray = NewNormalArray;
	B->SkinNormalArray[ B->SkinNormalCount ].Normal    = NNorm;
	B->SkinNormalArray[ B->SkinNormalCount ].BoneIndex = BoneIndex;
	B->SkinNormalArray[ B->SkinNormalCount ].LevelOfDetailMask = GE_BODY_HIGHEST_LOD_MASK;
	*Index = B->SkinNormalCount;
	B->SkinNormalCount ++ ;
	return GE_TRUE;
}

static geBoolean GENESISCC geBody_AddToFaces( geBody *B, geBody_Triangle *F, int DetailLevel )
{
	geBody_Triangle *NewFaceArray;
	geBody_TriangleList *FL;

	assert( B != NULL );
	assert( F != NULL );
	assert( DetailLevel >= 0);
	assert( DetailLevel < GE_BODY_NUMBER_OF_LOD );
	assert( geBody_IsValid(B) != GE_FALSE );

	FL = &( B->SkinFaces[DetailLevel] );

	assert( F->MaterialIndex >= 0 );
	assert( F->MaterialIndex < B->MaterialCount );

	NewFaceArray = GE_RAM_REALLOC_ARRAY( FL->FaceArray,
						geBody_Triangle,(FL->FaceCount+1) );
	if ( NewFaceArray == NULL )
	{
		geErrorLog_Add(ERR_BODY_ENOMEM, NULL);
		return GE_FALSE;
	}

	FL->FaceArray = NewFaceArray;

	{
		int i;
		// insertion sort new face into FaceArray keyed on MaterialIndex
		geBody_Index MaterialIndex = F->MaterialIndex;
		for (i=FL->FaceCount; i>=1; i--)
		{
			if (FL->FaceArray[i-1].MaterialIndex <= MaterialIndex)
				break;
			FL->FaceArray[i] = FL->FaceArray[i-1];
		}

		FL->FaceArray[i] = *F;
	}
	FL->FaceCount ++;

	return GE_TRUE;
}



geBoolean GENESISCC geBody_AddFace(	geBody *B,
	const geVec3d *Vertex1, const geVec3d *Normal1,
		geFloat U1, geFloat V1, int BoneIndex1,
	const geVec3d *Vertex2, const geVec3d *Normal2,
		geFloat U2, geFloat V2, int BoneIndex2,
	const geVec3d *Vertex3, const geVec3d *Normal3,
		geFloat U3, geFloat V3, int BoneIndex3,
	int MaterialIndex)
{
	geBody_Triangle F;

	assert( B != NULL );
	assert( Vertex1 != NULL );
	assert( Normal1 != NULL );
	assert( geBody_IsValid(B) != GE_FALSE );

	assert( BoneIndex1 >= 0 );
	assert( BoneIndex1 < B->BoneCount );

	assert( Vertex2 != NULL );
	assert( Normal2 != NULL );
	assert( BoneIndex2 >= 0 );
	assert( BoneIndex2 < B->BoneCount );

	assert( Vertex3 != NULL );
	assert( Normal3 != NULL );
	assert( BoneIndex3 >= 0 );
	assert( BoneIndex3 < B->BoneCount );

	assert( MaterialIndex >= 0 );
	assert(	MaterialIndex < B->MaterialCount );

	if (geBody_AddSkinVertex(B,Vertex1,U1,V1,(geBody_Index)BoneIndex1,&(F.VtxIndex[0]))==GE_FALSE)
	{	// error already recorded
		return GE_FALSE;
	}
	if (geBody_AddSkinVertex(B,Vertex2,U2,V2,(geBody_Index)BoneIndex2,&(F.VtxIndex[1]))==GE_FALSE)
	{	// error already recorded
		return GE_FALSE;
	}
	if (geBody_AddSkinVertex(B,Vertex3,U3,V3,(geBody_Index)BoneIndex3,&(F.VtxIndex[2]))==GE_FALSE)
	{	// error already recorded
		return GE_FALSE;
	}

	if (geBody_AddNormal( B, Normal1, (geBody_Index)BoneIndex1, &(F.NormalIndex[0]) ) == GE_FALSE)
	{	// error already recorded
		return GE_FALSE;
	}
	if (geBody_AddNormal( B, Normal2, (geBody_Index)BoneIndex2, &(F.NormalIndex[1]) ) == GE_FALSE)
	{	// error already recorded
		return GE_FALSE;
	}
	if (geBody_AddNormal( B, Normal3, (geBody_Index)BoneIndex3, &(F.NormalIndex[2]) ) == GE_FALSE)
	{	// error already recorded
		return GE_FALSE;
	}

	F.MaterialIndex = (geBody_Index)MaterialIndex;
	if (geBody_AddToFaces( B, &F, GE_BODY_HIGHEST_LOD ) == GE_FALSE)
	{	// error already recorded
		return GE_FALSE;
	}

	geBody_SortSkinVertices(B);

	return GE_TRUE;

}


geBoolean GENESISCC geBody_AddMaterial( geBody *B,
	const char *MaterialName,
	geBitmap *Bitmap,
	geFloat Red, geFloat Green, geFloat Blue,
	int *MaterialIndex)
{
	int FoundIndex;
	geBody_Material *NewMaterial;
	assert( B != NULL );
	assert( MaterialIndex != NULL );
	assert( geBody_IsValid(B) != GE_FALSE );
	assert( B->MaterialCount >= 0 );

	if (MaterialName == NULL)
	{
		geErrorLog_AddString(-1,"Can't add material - name can not be NULL", NULL);
		return GE_FALSE;
	}
	if (MaterialName[0] == 0)
	{
		geErrorLog_AddString(-1,"Can't add material - name must have > 0 length", NULL);
		return GE_FALSE;
	}
	if (geStrBlock_FindString(B->MaterialNames, MaterialName, &FoundIndex) == GE_TRUE)
	{
		geErrorLog_AddString(-1,"Can't add material - name already used", NULL);
		return GE_FALSE;
	}


	NewMaterial = GE_RAM_REALLOC_ARRAY( B->MaterialArray,
						geBody_Material,(B->MaterialCount+1) );
	if ( NewMaterial == NULL )
	{
		geErrorLog_Add(ERR_BODY_ENOMEM, NULL);
		return GE_FALSE;
	}


	B->MaterialArray = NewMaterial;
	if (geStrBlock_Append(&(B->MaterialNames),MaterialName) == GE_FALSE)
	{
		geErrorLog_Add(ERR_BODY_ENOMEM, NULL);
		return GE_FALSE;
	}

	{
		geBody_Material *M = &(B->MaterialArray[B->MaterialCount]);
		M->Bitmap = Bitmap;
		if (Bitmap != NULL)
			geBitmap_CreateRef(Bitmap);
		M->Red    = Red;
		M->Green  = Green;
		M->Blue   = Blue;

	}
	*MaterialIndex = B->MaterialCount;
	B->MaterialCount ++;
	return GE_TRUE;
}

geBoolean GENESISCC geBody_GetMaterial(const geBody *B, int MaterialIndex,
										const char **MaterialName,
										geBitmap **Bitmap, geFloat *Red, geFloat *Green, geFloat *Blue)
{
	assert( B      != NULL );
	assert( geBody_IsValid(B) != GE_FALSE );
	assert( Red    != NULL );
	assert( Green  != NULL );
	assert( Blue   != NULL );
	assert( Bitmap != NULL );
	assert( MaterialIndex >= 0 );
	assert( MaterialIndex < B->MaterialCount );
	assert( MaterialName != NULL );
	*MaterialName      = geStrBlock_GetString(B->MaterialNames,MaterialIndex);

	{
		geBody_Material *M = &(B->MaterialArray[MaterialIndex]);
		*Bitmap = M->Bitmap;
		*Red    = M->Red;
		*Green  = M->Green;
		*Blue   = M->Blue;
	}
	return GE_TRUE;
}

geBoolean GENESISCC geBody_SetMaterial(geBody *B, int MaterialIndex,
										geBitmap *Bitmap,  geFloat Red,  geFloat Green,  geFloat Blue)
{
	assert( geBody_IsValid(B) != GE_FALSE );
	assert( MaterialIndex >= 0 );
	assert( MaterialIndex < B->MaterialCount );
	{
		geBody_Material *M = &(B->MaterialArray[MaterialIndex]);
		M->Bitmap = Bitmap;
		if (Bitmap != NULL)
			geBitmap_CreateRef(Bitmap);
		M->Red    = Red;
		M->Green  = Green;
		M->Blue   = Blue;
	}
	return GE_TRUE;
}




geBoolean GENESISCC geBody_AddBone( geBody *B,
	int ParentBoneIndex,
	const char *BoneName,
	const geXForm3d *AttachmentMatrix,
	int *BoneIndex)
{
	geBody_Bone *NewBones;
	assert( B != NULL );
	assert( BoneName != NULL );
	assert( BoneIndex != NULL );
	assert( geBody_IsValid(B) != GE_FALSE );

	assert( ParentBoneIndex < B->BoneCount );
	assert( ( ParentBoneIndex >= 0)  || (ParentBoneIndex == GE_BODY_NO_PARENT_BONE));
	assert( B->BoneCount >= 0 );

	NewBones = GE_RAM_REALLOC_ARRAY( B->BoneArray,
						geBody_Bone, (B->BoneCount+1) );
	if ( NewBones == NULL )
	{
		geErrorLog_Add(ERR_BODY_ENOMEM, NULL);
		return GE_FALSE;
	}

	B->BoneArray = NewBones;
	if (geStrBlock_Append(&(B->BoneNames),BoneName) == GE_FALSE)
	{
		geErrorLog_Add(ERR_BODY_ENOMEM, NULL);
		return GE_FALSE;
	}

	{
		geBody_Bone *Bone = &(B->BoneArray[B->BoneCount]);
		geVec3d_Set(&(Bone->BoundingBoxMin),
			GE_BODY_REALLY_BIG_NUMBER,GE_BODY_REALLY_BIG_NUMBER,GE_BODY_REALLY_BIG_NUMBER);
		geVec3d_Set(&(Bone->BoundingBoxMax),
			-GE_BODY_REALLY_BIG_NUMBER,-GE_BODY_REALLY_BIG_NUMBER,-GE_BODY_REALLY_BIG_NUMBER);
		Bone->AttachmentMatrix = *AttachmentMatrix;
		Bone->ParentBoneIndex = (geBody_Index)ParentBoneIndex;
	}
	*BoneIndex = B->BoneCount;
	B->BoneCount++;
	return GE_TRUE;
}



geBoolean GENESISCC geBody_ComputeLevelsOfDetail( geBody *B ,int Levels)
{
	assert( B != NULL);
	assert( Levels >= 0 );
	assert( Levels < GE_BODY_NUMBER_OF_LOD );
	assert( geBody_IsValid(B) != GE_FALSE );
	#pragma message ("LOD code goes here:")
	B->LevelsOfDetail = GE_BODY_HIGHEST_LOD_MASK; // Levels
	Levels;
	return GE_TRUE;
}



#define GE_BODY_GEOMETRY_NAME "Geometry"
#define GE_BODY_BITMAP_DIRECTORY_NAME "Bitmaps"

#define GE_BODY_FILE_TYPE 0x5E444F42     // 'BODY'
#define GE_BODY_FILE_VERSION 0x00F1		// Restrict version to 16 bits


static geBoolean GENESISCC geBody_ReadGeometry(geBody *B, geVFile *pFile)
{
	uint32 u;
	int i;

	assert( B != NULL );
	assert( pFile != NULL );
	if(geVFile_Read(pFile, &u, sizeof(u)) == GE_FALSE)
	{	geErrorLog_Add( ERR_BODY_FILE_READ , NULL);	return GE_FALSE; }
	if (u!=GE_BODY_FILE_TYPE)
	{	geErrorLog_Add( ERR_BODY_FILE_PARSE , NULL);  return GE_FALSE; }


	if(geVFile_Read(pFile, &u, sizeof(u)) == GE_FALSE)
	{	geErrorLog_Add( ERR_BODY_FILE_READ , NULL);	return GE_FALSE; }
	if (u!=GE_BODY_FILE_VERSION)
	{	geErrorLog_Add( ERR_BODY_FILE_READ , NULL);   return GE_FALSE; }


	if(geVFile_Read(pFile, &(B->BoundingBoxMin), sizeof(B->BoundingBoxMin)) == GE_FALSE)
	{	geErrorLog_Add( ERR_BODY_FILE_READ , NULL);	 return GE_FALSE; }

	if(geVFile_Read(pFile, &(B->BoundingBoxMax), sizeof(B->BoundingBoxMax)) == GE_FALSE)
	{	geErrorLog_Add( ERR_BODY_FILE_READ , NULL);	 return GE_FALSE; }

	if(geVFile_Read(pFile, &(B->XSkinVertexCount), sizeof(B->XSkinVertexCount)) == GE_FALSE)
	{	geErrorLog_Add( ERR_BODY_FILE_READ , NULL);	 return GE_FALSE; }

	if (B->XSkinVertexCount>0)
	{
		u = sizeof(geBody_XSkinVertex) * B->XSkinVertexCount;
		B->XSkinVertexArray = geRam_Allocate(u);
		if (B->XSkinVertexArray == NULL)
		{	geErrorLog_Add( ERR_BODY_ENOMEM , NULL);   return GE_FALSE;  }
		if(geVFile_Read(pFile, B->XSkinVertexArray, u) == GE_FALSE)
		{	geErrorLog_Add( ERR_BODY_FILE_READ , NULL);	 return GE_FALSE; }
	}

	if(geVFile_Read(pFile, &(B->SkinNormalCount), sizeof(B->SkinNormalCount)) == GE_FALSE)
	{	geErrorLog_Add( ERR_BODY_FILE_READ , NULL);	 return GE_FALSE; }

	if (B->SkinNormalCount>0)
	{
		u = sizeof(geBody_Normal) * B->SkinNormalCount;
		B->SkinNormalArray = geRam_Allocate(u);
		if (B->SkinNormalArray == NULL)
		{	geErrorLog_Add( ERR_BODY_ENOMEM , NULL);   return GE_FALSE;  }
		if(geVFile_Read(pFile, B->SkinNormalArray, u) == GE_FALSE)
		{	geErrorLog_Add( ERR_BODY_FILE_READ , NULL);	return GE_FALSE; }
	}

	if(geVFile_Read(pFile, &(B->BoneCount), sizeof(B->BoneCount)) == GE_FALSE)
	{	geErrorLog_Add( ERR_BODY_FILE_READ , NULL);  return GE_FALSE; }

	if (B->BoneCount>0)
	{
		u = sizeof(geBody_Bone) * B->BoneCount;
		B->BoneArray = geRam_Allocate(u);
		if (B->BoneArray == NULL)
		{	geErrorLog_Add( ERR_BODY_ENOMEM , NULL);   return GE_FALSE;  }
		if(geVFile_Read(pFile, B->BoneArray, u) == GE_FALSE)
		{	geErrorLog_Add( ERR_BODY_FILE_READ , NULL);  return GE_FALSE; }
	}

	B->BoneNames = geStrBlock_CreateFromFile(pFile);
	if (B->BoneNames==NULL)
	{	geErrorLog_Add( ERR_BODY_FILE_READ , NULL); 	 return GE_FALSE; }

	if(geVFile_Read(pFile, &(B->MaterialCount), sizeof(B->MaterialCount)) == GE_FALSE)
	{	geErrorLog_Add( ERR_BODY_FILE_READ , NULL);	 return GE_FALSE; }

	if (B->MaterialCount>0)
	{
		u = sizeof(geBody_Material) * B->MaterialCount;
		B->MaterialArray = geRam_Allocate(u);
		if (B->MaterialArray == NULL)
		{	geErrorLog_Add( ERR_BODY_ENOMEM , NULL);   return GE_FALSE;  }
		if(geVFile_Read(pFile, B->MaterialArray, u) == GE_FALSE)
		{	geErrorLog_Add( ERR_BODY_FILE_READ , NULL);	 return GE_FALSE; }

		// CB added this nastiness because it seems the Bitmap pointer is
		//	read in with the Material array, and is later used as a boolean
		//	for "should this material have a texture"
		for(u=0;u<(uint32)B->MaterialCount;u++)
		{
			if ( B->MaterialArray[u].Bitmap )
				B->MaterialArray[u].Bitmap = (geBitmap *)1;
		}
	}

	B->MaterialNames = geStrBlock_CreateFromFile(pFile);
	if ( B->MaterialNames == NULL )
	{	geErrorLog_Add( ERR_BODY_FILE_READ , NULL); 	 return GE_FALSE; }

	if(geVFile_Read(pFile, &(B->LevelsOfDetail), sizeof(B->LevelsOfDetail)) == GE_FALSE)
	{	geErrorLog_Add( ERR_BODY_FILE_READ , NULL);	 return GE_FALSE; }

	if (B->LevelsOfDetail > GE_BODY_NUMBER_OF_LOD)
	{	geErrorLog_Add( ERR_BODY_FILE_READ , NULL);	 return GE_FALSE; }

	for (i=0; i<B->LevelsOfDetail; i++)
	{
		if(geVFile_Read(pFile, &(u), sizeof(u)) == GE_FALSE)
		{	geErrorLog_Add( ERR_BODY_FILE_READ , NULL);	 return GE_FALSE; }
		B->SkinFaces[i].FaceCount = (geBody_Index)u;

		if (u>0)
		{
			u = sizeof(geBody_Triangle) * u;
			B->SkinFaces[i].FaceArray = geRam_Allocate(u);
			if (B->SkinFaces[i].FaceArray == NULL)
			{	geErrorLog_Add( ERR_BODY_ENOMEM , NULL);   return GE_FALSE;  }
			if(geVFile_Read(pFile, B->SkinFaces[i].FaceArray, u) == GE_FALSE)
			{	geErrorLog_Add( ERR_BODY_FILE_READ , NULL);	 return GE_FALSE; }
		}
	}

	assert( geBody_IsValid(B) != GE_FALSE );
	return GE_TRUE;
}

geBody *GENESISCC geBody_CreateFromFile(geVFile *pFile)
{
	geBody  *B;
	int i;

	geVFile *VFile;
	geVFile *SubFile=NULL;
	geVFile *BitmapDirectory=NULL;

	assert( pFile != NULL );

	VFile = geVFile_OpenNewSystem(pFile,GE_VFILE_TYPE_VIRTUAL, NULL,
									NULL, GE_VFILE_OPEN_DIRECTORY | GE_VFILE_OPEN_READONLY);
	if (VFile == NULL)
	{	geErrorLog_Add( ERR_BODY_FILE_READ , NULL);	goto CreateError;}

	SubFile = geVFile_Open(VFile,GE_BODY_GEOMETRY_NAME,GE_VFILE_OPEN_READONLY);
	if (SubFile == NULL)
	{	geErrorLog_Add( ERR_BODY_FILE_READ , NULL);	goto CreateError;}

	B = geBody_CreateNull();
	if (B==NULL)
	{	geErrorLog_Add( ERR_BODY_ENOMEM , NULL);  goto CreateError;  }

	if (geBody_ReadGeometry(B,SubFile)==GE_FALSE)
	{	geErrorLog_Add( ERR_BODY_FILE_READ , NULL);	goto CreateError;}
	geVFile_Close(SubFile);

	BitmapDirectory = geVFile_Open(VFile,GE_BODY_BITMAP_DIRECTORY_NAME,
									GE_VFILE_OPEN_DIRECTORY | GE_VFILE_OPEN_READONLY);
	if (BitmapDirectory == NULL)
	{	geErrorLog_Add( ERR_BODY_FILE_READ , NULL);	goto CreateError;}

	for (i=0; i<B->MaterialCount; i++)
	{
		geBody_Material *M;
		M = &(B->MaterialArray[i]);

		if (M->Bitmap != NULL)
		{
			char FName[1000];
			sprintf(FName,"%d",i);

			SubFile = geVFile_Open(BitmapDirectory,FName,GE_VFILE_OPEN_READONLY);
			if (SubFile == NULL)
			{	geErrorLog_Add( ERR_BODY_FILE_READ , NULL);	goto CreateError;}

			M->Bitmap = geBitmap_CreateFromFile(SubFile);

			if (M->Bitmap == NULL)
			{	geErrorLog_Add( ERR_BODY_FILE_READ , NULL);	goto CreateError;}

		#if 1
			// Set the number of mips to 4
			if (!geBitmap_SetMipCount(M->Bitmap, 4))
			{
				geErrorLog_Add( ERR_BODY_FILE_READ , NULL);
				goto CreateError;
			}
		#endif

			geVFile_Close(SubFile);
		}
	}
	geVFile_Close(BitmapDirectory);
	geVFile_Close(VFile);
	return B;

CreateError:
	geBody_DestroyPossiblyIncompleteBody(&B);
	if (SubFile != NULL)
		geVFile_Close(SubFile);
	if (BitmapDirectory != NULL)
		geVFile_Close(BitmapDirectory);
	if (VFile != NULL)
		geVFile_Close(VFile);
	return NULL;
}



geBoolean GENESISCC geBody_WriteGeometry(const geBody *B,geVFile *pFile)
{
	uint32 u;
	int i;

	assert( B != NULL );
	assert( pFile != NULL );
	assert( geBody_IsValid(B) != GE_FALSE );

	// Write the format flag
	u = GE_BODY_FILE_TYPE;
	if(geVFile_Write(pFile, &u, sizeof(u)) == GE_FALSE)
	{	geErrorLog_Add( ERR_BODY_FILE_WRITE , NULL);	return GE_FALSE; }

	// Write the version
	u = GE_BODY_FILE_VERSION;
	if(geVFile_Write(pFile, &u, sizeof(u)) == GE_FALSE)
	{	geErrorLog_Add( ERR_BODY_FILE_WRITE , NULL);	return GE_FALSE; }

	if(geVFile_Write(pFile, &(B->BoundingBoxMin), sizeof(B->BoundingBoxMin)) == GE_FALSE)
	{	geErrorLog_Add( ERR_BODY_FILE_WRITE , NULL);	return GE_FALSE; }

	if(geVFile_Write(pFile, &(B->BoundingBoxMax), sizeof(B->BoundingBoxMax)) == GE_FALSE)
	{	geErrorLog_Add( ERR_BODY_FILE_WRITE , NULL);	return GE_FALSE; }

	if(geVFile_Write(pFile, &(B->XSkinVertexCount), sizeof(B->XSkinVertexCount)) == GE_FALSE)
	{	geErrorLog_Add( ERR_BODY_FILE_WRITE , NULL);	return GE_FALSE; }

	assert( (B->XSkinVertexCount==0) || (B->XSkinVertexArray!=NULL));

	if (B->XSkinVertexCount>0)
	{
		u = sizeof(geBody_XSkinVertex) * B->XSkinVertexCount;
		if(geVFile_Write(pFile, B->XSkinVertexArray, u) == GE_FALSE)
		{	geErrorLog_Add( ERR_BODY_FILE_WRITE , NULL);	return GE_FALSE; }
	}

	if(geVFile_Write(pFile, &(B->SkinNormalCount), sizeof(B->SkinNormalCount)) == GE_FALSE)
	{	geErrorLog_Add( ERR_BODY_FILE_WRITE , NULL);	return GE_FALSE; }

	if (B->SkinNormalCount>0)
	{
		u = sizeof(geBody_Normal) * B->SkinNormalCount;
		if(geVFile_Write(pFile, B->SkinNormalArray, u) == GE_FALSE)
		{	geErrorLog_Add( ERR_BODY_FILE_WRITE , NULL);	return GE_FALSE; }
	}

	if(geVFile_Write(pFile, &(B->BoneCount), sizeof(B->BoneCount)) == GE_FALSE)
	{	geErrorLog_Add( ERR_BODY_FILE_WRITE , NULL);	return GE_FALSE; }

	if (B->BoneCount>0)
	{
		u = sizeof(geBody_Bone) * B->BoneCount;
		if(geVFile_Write(pFile, B->BoneArray, u) == GE_FALSE)
		{	geErrorLog_Add( ERR_BODY_FILE_WRITE , NULL);	return GE_FALSE; }
	}

	if (geStrBlock_WriteToBinaryFile(B->BoneNames,pFile)==GE_FALSE)
	{	geErrorLog_Add( ERR_BODY_FILE_WRITE , NULL); 	return GE_FALSE; }

	if(geVFile_Write(pFile, &(B->MaterialCount), sizeof(B->MaterialCount)) == GE_FALSE)
	{	geErrorLog_Add( ERR_BODY_FILE_WRITE , NULL);	return GE_FALSE; }

	if (B->MaterialCount>0)
	{
		u = sizeof(geBody_Material) * B->MaterialCount;
		if(geVFile_Write(pFile, B->MaterialArray, u) == GE_FALSE)
		{	geErrorLog_Add( ERR_BODY_FILE_WRITE , NULL);	return GE_FALSE; }
	}

	if (geStrBlock_WriteToBinaryFile(B->MaterialNames,pFile)==GE_FALSE)
	{	geErrorLog_Add( ERR_BODY_FILE_WRITE , NULL); 	return GE_FALSE; }

	if(geVFile_Write(pFile, &(B->LevelsOfDetail), sizeof(B->LevelsOfDetail)) == GE_FALSE)
	{	geErrorLog_Add( ERR_BODY_FILE_WRITE , NULL);	return GE_FALSE; }

	for (i=0; i<B->LevelsOfDetail; i++)
	{
		u = B->SkinFaces[i].FaceCount;
		if(geVFile_Write(pFile, &(u), sizeof(u)) == GE_FALSE)
		{	geErrorLog_Add( ERR_BODY_FILE_WRITE , NULL);	return GE_FALSE; }
		if (u>0)
		{
			u = sizeof(geBody_Triangle) * u;
			if(geVFile_Write(pFile, B->SkinFaces[i].FaceArray, u) == GE_FALSE)
			{	geErrorLog_Add( ERR_BODY_FILE_WRITE , NULL);	return GE_FALSE; }
		}
	}
	return GE_TRUE;
}


geBoolean GENESISCC geBody_WriteToFile(const geBody *B, geVFile *pFile)
{
	int i;
	geVFile *VFile;
	geVFile *SubFile;
	geVFile *BitmapDirectory;

	assert( geBody_IsValid(B) != GE_FALSE );
	assert( pFile != NULL );

	VFile = geVFile_OpenNewSystem(pFile,GE_VFILE_TYPE_VIRTUAL, NULL,
									NULL, GE_VFILE_OPEN_DIRECTORY | GE_VFILE_OPEN_CREATE);
	if (VFile == NULL)
	{	geErrorLog_Add( ERR_BODY_FILE_WRITE , NULL);	goto WriteError;}

	SubFile = geVFile_Open(VFile,GE_BODY_GEOMETRY_NAME,GE_VFILE_OPEN_CREATE);
	if (SubFile == NULL)
	{	geErrorLog_Add( ERR_BODY_FILE_WRITE , NULL);	goto WriteError;}

	if (geBody_WriteGeometry(B,SubFile)==GE_FALSE)
	{	geErrorLog_Add( ERR_BODY_FILE_WRITE , NULL);	goto WriteError;}
	if (geVFile_Close(SubFile)==GE_FALSE)
	{	geErrorLog_Add( ERR_BODY_FILE_WRITE , NULL);	goto WriteError;}

	BitmapDirectory = geVFile_Open(VFile,GE_BODY_BITMAP_DIRECTORY_NAME,
									GE_VFILE_OPEN_DIRECTORY | GE_VFILE_OPEN_CREATE);
	if (BitmapDirectory == NULL)
	{	geErrorLog_Add( ERR_BODY_FILE_WRITE , NULL);	goto WriteError;}

	for (i=0; i<B->MaterialCount; i++)
	{
		geBody_Material *M;
		M = &(B->MaterialArray[i]);

		if (M->Bitmap != NULL)
		{
			char FName[1000];
			sprintf(FName,"%d",i);

			SubFile = geVFile_Open(BitmapDirectory,FName,GE_VFILE_OPEN_CREATE);
			if (SubFile == NULL)
			{	geErrorLog_Add( ERR_BODY_FILE_WRITE , NULL);	goto WriteError;}

			if (geBitmap_WriteToFile(M->Bitmap,SubFile)==GE_FALSE)
			{	geErrorLog_Add( ERR_BODY_FILE_WRITE , NULL);	goto WriteError;}

			if (geVFile_Close(SubFile)==GE_FALSE)
			{	geErrorLog_Add( ERR_BODY_FILE_WRITE , NULL);	goto WriteError;}
		}
	}
	if (geVFile_Close(BitmapDirectory)==GE_FALSE)
	{	geErrorLog_Add( ERR_BODY_FILE_WRITE , NULL);	goto WriteError;}
	if (geVFile_Close(VFile)==GE_FALSE)
	{	geErrorLog_Add( ERR_BODY_FILE_WRITE , NULL);	goto WriteError;}

	return GE_TRUE;
WriteError:
	return GE_FALSE;
}
