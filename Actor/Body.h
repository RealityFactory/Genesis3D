/****************************************************************************************/
/*  BODY.H                                                                              */
/*                                                                                      */
/*  Author: Mike Sandige	                                                            */
/*  Description: Actor body interface.		                                            */
/*                                                                                      */
/*  Edit History:                                                                       */
/*	02/21/2004 Wendell Buckner                                                            */
/*   DOT3 BUMPMAPPING                                                                   */
/*	02/18/2004 Wendell Buckner                                                          */
/*   DOT3 BUMPMAPPING                                                                   */
/*	01/13/2004 Wendell Buckner                                                          */ 
/*   DOT3 BUMPMAPPING                                                                   */
/*  04/08/2003 Wendell Buckner 	                                                        */
/*   BUMPMAPPING                                                                        */                                    
/*                                                                                      */
/*  The contents of this file are subject to the Genesis3D Public License               */
/*  Version 1.01 (the "License"); you may not use this file except in                   */
/*  compliance with the License. You may obtain a copy of the License at                */
/*  http://www.genesis3d.com                                                          */
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

#ifndef GE_BODY_H
#define GE_BODY_H 

/* This object is for managing the data associated with a skeletal-based mesh, 
   a 'body'.
   This object holds the geometry for the body and the list of materials needed.
*/

#include "BaseType.h"
#include "XForm3d.h"
#include "VFile.h"
#include "Bitmap.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GE_BODY_NUMBER_OF_LOD			(4)		// 0 is highest detail
#define GE_BODY_NO_PARENT_BONE         (-1)   
#define GE_BODY_HIGHEST_LOD            (0)

#define GE_BODY_ROOT					(-1)	// for specifying 'root' bounding box.

typedef struct geBody geBody;



GENESISAPI geBody *GENESISCC geBody_Create(void);

GENESISAPI void GENESISCC geBody_Destroy(geBody **B);

GENESISAPI geBoolean GENESISCC geBody_IsValid(const geBody *B);


GENESISAPI geBoolean GENESISCC geBody_GetGeometryStats(const geBody *B, int lod, int *Vertices, int *Faces, int *Normals);

GENESISAPI geBoolean GENESISCC geBody_AddFace(	geBody *B,
								const geVec3d *Vertex1, const geVec3d *Normal1, 
								geFloat U1, geFloat V1, int BoneIndex1,
								const geVec3d *Vertex2, const geVec3d *Normal2, 
									geFloat U2, geFloat V2, int BoneIndex2,
								const geVec3d *Vertex3, const geVec3d *Normal3, 
									geFloat U3, geFloat V3, int BoneIndex3,
								int MaterialIndex);

			// Bitmap is added to body.  It's reference count is increased.  Caller still owns a pointer
			// to the bitmap, and is responsible for destroying it.
GENESISAPI geBoolean GENESISCC geBody_AddMaterial( geBody *B, 
									const char *MaterialName, 
									geBitmap *Bitmap,
									geFloat Red, 
									geFloat Green, 
									geFloat Blue,
									int *MaterialIndex);

			// returned bitmap is a pointer to the bitmap in the body's list.  It may not be destroyed.
			// if caller would like to 'own' a copy of that bitmap pointer, it should call geBitmap_CreateRef()
GENESISAPI geBoolean GENESISCC geBody_GetMaterial(const geBody *Body, int MaterialIndex,
										const char **MaterialName,
										geBitmap **Bitmap, geFloat *Red, geFloat *Green, geFloat *Blue);

			// Bitmap is set into the body.  It's reference count is increased.  Caller still owns a pointer
			// to the bitmap, and is responsible for destroying it.
GENESISAPI geBoolean GENESISCC geBody_SetMaterial(geBody *Body, int MaterialIndex,
										geBitmap *Bitmap,  geFloat Red,  geFloat Green,  geFloat Blue);

GENESISAPI int GENESISCC geBody_GetMaterialCount(const geBody *B);

GENESISAPI geBoolean GENESISCC geBody_AddBone( geBody *B, 
							int ParentBoneIndex,
							const char *BoneName, 
							const geXForm3d *AttachmentMatrix,
							int *BoneIndex);

GENESISAPI geBoolean GENESISCC geBody_ComputeLevelsOfDetail( geBody *B ,int Levels);

GENESISAPI int GENESISCC geBody_GetBoneCount(const geBody *B);

GENESISAPI void GENESISCC geBody_GetBone(	const geBody *B, 
						int BoneIndex, 
						const char **BoneName,
						geXForm3d *Attachment, 
						int *ParentBoneIndex);

GENESISAPI int32 GENESISCC geBody_GetBoneNameChecksum(const geBody *B);

GENESISAPI void GENESISCC geBody_SetBoundingBox( geBody *B,
							int BoneIndex,		// GE_BODY_ROOT for specifing 'root' bounding box.
							const geVec3d *MinimumBoxCorner,
							const geVec3d *MaximumBoxCorner);
 

GENESISAPI geBoolean GENESISCC geBody_GetBoundingBox( const geBody *B, 
							int BoneIndex,		// GE_BODY_ROOT for specifing 'root' bounding box.
							geVec3d *MinimumBoxCorner,
							geVec3d *MaximumBoxCorner);

GENESISAPI geBoolean GENESISCC geBody_GetBoneByName(const geBody* B,
	const char* BoneName,
	int* pBoneIndex,
	geXForm3d* Attachment,
	int* pParentBoneIndex);

GENESISAPI geBoolean GENESISCC geBody_WriteToFile(const geBody *B, geVFile *pFile);
GENESISAPI geBody  *GENESISCC  geBody_CreateFromFile(geVFile *pFile);


/* 04/08/2003 Wendell Buckner 	
    BUMPMAPPING */
GENESISAPI geBitmap *GENESISCC geBody_CreateBumpmapByName(const geBody *B, const char *BumpmapName, gePixelFormat BumpFormat);
GENESISAPI geBitmap *GENESISCC geBody_GetBumpMapAltByName(const geBody *B, const char *BumpmapName);

/*	02/21/2004 Wendell Buckner
    DOT3 BUMPMAPPING */
GENESISAPI void GENESISCC geBody_DestroyTangentSpace( geBody *B );

/*	02/18/2004 Wendell Buckner
    DOT3 BUMPMAPPING */
GENESISAPI geBoolean GENESISCC geBody_CreateBumpmapByNameDot3(const geBody *B, const char *BumpmapName);

/*	01/13/2004 Wendell Buckner
    DOT3 BUMPMAPPING */
GENESISAPI geBoolean GENESISCC geBody_CreateTangentSpace( const geBody *B );
GENESISAPI void GENESISCC geBody_SetVertexColorDot3( geVec3d LightPosition, const geXForm3d *Mdl2WldXFA, const geBody *B, int16 VertexIndex, geFloat *ColorDot3, int16 LightType, geBoolean *Reset );

#ifdef __cplusplus
}
#endif

#endif
					