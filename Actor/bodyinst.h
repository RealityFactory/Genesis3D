/****************************************************************************************/
/*  BODYINST.H                                                                          */
/*                                                                                      */
/*  Author: Mike Sandige	                                                            */
/*  Description: Actor body instance interface.		                                    */
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
#ifndef GE_BODYINST_H
#define GE_BODYINST_H 

/* This object is for accessing and retrieving an 'instance' of the geometry
   for a body.  
   
   The retrieval is a list of drawing commands in world space or 
   in camera space.  

   An array of transforms that corresponds to the bones in the body is needed.
 */


#include "basetype.h"
#include "xform3d.h"
#include "body.h"
#include "XFArray.h"
#include "camera.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef struct geBodyInst geBodyInst;

typedef int16 geBodyInst_Index;

typedef enum 
{
	GE_BODYINST_FACE_TRIANGLE,
	GE_BODYINST_FACE_TRISTRIP,
	GE_BODYINST_FACE_TRIFAN
} geBodyInst_FaceType;


typedef struct geBodyInst_SkinVertex
{
	geVec3d SVPoint;
	geFloat SVU,SVV;
	int	ReferenceBoneIndex;
} geBodyInst_SkinVertex;

typedef struct geBodyInst_Geometry 
{
	geBodyInst_Index		 SkinVertexCount;
	geBodyInst_SkinVertex	*SkinVertexArray;

	geBodyInst_Index		 NormalCount;
	geVec3d					*NormalArray;

	geBodyInst_Index		 FaceCount;
	int32					 FaceListSize;
	geBodyInst_Index		*FaceList;

	geVec3d					 Maxs, Mins;
}	geBodyInst_Geometry;

/* format for geBodyInst_Geometry.FaceList:
	primitive type (GE_BODY_FACE_TRIANGLE,	  GE_BODY_FACE_TRISTRIP,  GE_BODY_FACE_TRIFAN )
	followed by material index
	followed by...
	case primitive 
		GE_BODY_FACE_TRIANGLE:
		  vertex index 1, normal index 1
		  vertex index 2, normal index 2
		  vertex index 3, normal index 3
		  (next primitive)
		GE_BODY_FACE_TRISTRIP:
		  triangle count
		  vertex index 1, normal index 1
		  vertex index 2, normal index 2
		  vertex index 3, normal index 3
		  vertex index 4, normal index 4
		  ...  # vertices is triangle count+2
		  (next primitive)
		GE_BODY_FACE_TRIFAN:
		  triangle count
		  vertex index 1, normal index 1
		  vertex index 2, normal index 2
		  vertex index 3, normal index 3
		  vertex index 4, normal index 4
		  ...  # vertices is triangle count+2
		  (next primitive)
*/




geBodyInst *GENESISCC geBodyInst_Create( const geBody *B );
void GENESISCC geBodyInst_Destroy(geBodyInst **BI);

const geBodyInst_Geometry *GENESISCC geBodyInst_GetGeometry( 
								const geBodyInst *BI,
								const geVec3d *Scale,
								const geXFArray *BoneXformArray,
								int LevelOfDetail,
								const geCamera *Camera);


#ifdef __cplusplus
}
#endif

#endif
		