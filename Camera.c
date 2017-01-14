/****************************************************************************************/
/*  Camera.c                                                                            */
/*                                                                                      */
/*  Author: John Pollard/Charles Bloom                                                  */
/*  Description: Creation/Transformation/projection code for a camera                   */
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
#include <math.h>
#include <Assert.h>
#include <Memory.h>		// memset

#include "Camera.h"
#include "Ram.h"
#include "ErrorLog.h"


#include "DCommon.h"

typedef struct geCamera 
{
	geFloat		Fov;						// Field of View for Camera
	geFloat		Width;						// Width of viewport
	geFloat		Height;						// Height of viewport
	geFloat		Left,Right,Top,Bottom;		// Clipping rect

	geFloat		XRatio;						// Screen Width / Fov
	geFloat		YRatio;						// Screen Height / Fov
	geFloat		Scale;						// X and Y Scale based on Fov
	geFloat		XCenter;					// Screen Width/2
	geFloat		YCenter;					// Screen Height/2

	geXForm3d	XForm;						// View transform

	geBoolean	UseVisPov;					// == GE_TRUE is vis uses VisXPov
	geXForm3d	VisXForm;
	geXForm3d	TransposeVisXForm;
	geVec3d		VisPov;						// For vising info

	geXForm3d	TransposeXForm;				// Original model xform...
	
	geVec3d		Pov;						// Un rotated Pov in XForm

	geFloat		SinViewAngleX;				// sin(2.0 /Fov * Scale/XRatio);
	geFloat		CosViewAngleX;				// cos(2.0 /Fov * Scale/XRatio);
	geFloat		SinViewAngleY;				// sin(2.0 /Fov * Scale/YRatio);
	geFloat		CosViewAngleY;				// cos(2.0 /Fov * Scale/YRatio);

	geFloat		ZScale;						// Projected Z Scalar value
	
	geBoolean	ZFarEnable;					// GE_TRUE == Use ZFar clipplane
	geFloat		ZFar;						// ZFar clip plane distance
	
} geCamera;


#ifndef max
	#define max(AA,BB)  (  ((AA)>(BB)) ?(AA):(BB)  )
#endif

#define CAMERA_MINIMUM_PROJECTION_DISTANCE (0.010f)

//=====================================================================================
//	geCamera_Create
//=====================================================================================
GENESISAPI geCamera *GENESISCC geCamera_Create(geFloat Fov, const geRect *Rect)
{
	geCamera *Camera;

	assert( Rect != NULL );

	Camera = GE_RAM_ALLOCATE_STRUCT(geCamera);
	if (Camera == NULL)
		{
			geErrorLog_Add(-1, NULL); //FIXME
			return NULL;
		}

	memset(Camera, 0, sizeof(geCamera));

	Camera->ZScale = 0.5f;

#if 0
	Camera->ZFar = 1000.0f;
	Camera->ZFarEnable = GE_TRUE;
#endif

	geCamera_SetAttributes(Camera,Fov,Rect);
	return Camera;
}

//=====================================================================================
//	geCamera_Destroy
//=====================================================================================
GENESISAPI void GENESISCC geCamera_Destroy(geCamera **pCamera)
{
	assert( pCamera  != NULL );
	assert( *pCamera != NULL );
	geRam_Free(*pCamera);
	*pCamera = NULL;
}

//=====================================================================================
//	geCamera_SetZScale
//=====================================================================================
GENESISAPI void GENESISCC geCamera_SetZScale(geCamera *Camera, geFloat ZScale)
{
	assert(Camera);
	Camera->ZScale = ZScale;
}

//=====================================================================================
//	geCamera_GetZScale
//=====================================================================================
GENESISAPI geFloat GENESISCC geCamera_GetZScale(const geCamera *Camera)
{
	assert(Camera);
	return Camera->ZScale;
}

//=====================================================================================
//	geCamera_SetFarClipPlane
//=====================================================================================
GENESISAPI void GENESISCC geCamera_SetFarClipPlane(geCamera *Camera, geBoolean Enable, geFloat ZFar)
{
	assert(Camera);

	Camera->ZFarEnable = Enable;
	Camera->ZFar = ZFar;
}

//=====================================================================================
//	geCamera_GetFarClipPlane
//=====================================================================================
GENESISAPI void GENESISCC geCamera_GetFarClipPlane(const geCamera *Camera, geBoolean *Enable, geFloat *ZFar)
{
	assert(Camera);

	*Enable = Camera->ZFarEnable;
	*ZFar = Camera->ZFar;
}

//=====================================================================================
//	geCamera_GetClippingRect
//=====================================================================================
GENESISAPI void GENESISCC geCamera_GetClippingRect(const geCamera *Camera, geRect *Rect)
{
	assert( Camera != NULL );
	assert( Rect != NULL );
	Rect->Left   = (int32)Camera->Left;
	Rect->Right  = (int32)Camera->Right;
	Rect->Top    = (int32)Camera->Top;
	Rect->Bottom = (int32)Camera->Bottom;
}

//=====================================================================================
//	geCamera_GetWidthHeight
//=====================================================================================
void GENESISCC geCamera_GetWidthHeight(const geCamera *Camera,geFloat *Width,geFloat *Height)
{
	assert( Width  != NULL );
	assert( Height != NULL );
	assert( Camera != NULL );

	*Width  = Camera->Width;
	*Height = Camera->Height;
}
		
//=====================================================================================
//	geCamera_GetScale
//=====================================================================================
float GENESISCC geCamera_GetScale(const geCamera *Camera)
{
	assert( Camera != NULL );

	return Camera->Scale;
}

//=====================================================================================
//	geCamera_SetAttributes
//=====================================================================================
GENESISAPI void GENESISCC geCamera_SetAttributes(geCamera *Camera, geFloat Fov, const geRect *Rect)
{

#define TOO_SMALL (0.0001f)		// width and Fov must be >= TOO_SMALL

	geFloat	Width, Height;
	geFloat OneOverFov;

	assert (Camera != NULL);
	assert (Rect != NULL);
	assert (! ((Fov < TOO_SMALL) && (Fov > -TOO_SMALL)));

	Width  = (geFloat)(Rect->Right - Rect->Left)+1.0f;
	Height = (geFloat)(Rect->Bottom - Rect->Top)+1.0f;

	assert( Width > 0.0f  );
	assert( Height > 0.0f );

	Camera->Width   = Width;
	Camera->Height  = Height;
	
	Camera->Fov		= Fov;
	
	if ((Camera->Fov < TOO_SMALL) && (Camera->Fov > -TOO_SMALL))
		Camera->Fov = TOO_SMALL;		// Just in case

	OneOverFov  = 1.0f/Fov;

	if (Width <=0.0f)
		Width = TOO_SMALL;				// Just in case
	if (Height <=0.0f)
		Height = TOO_SMALL;				// Just in case

	Camera->XRatio  = Width  * OneOverFov;
	Camera->YRatio  = Height * OneOverFov;
	
	Camera->Scale   = max(Camera->XRatio, Camera->YRatio);
	//Camera->YScale = Camera->XScale;

	Camera->Left    = (geFloat)Rect->Left;
	Camera->Right   = (geFloat)Rect->Right;
	Camera->Top     = (geFloat)Rect->Top;
	Camera->Bottom  = (geFloat)Rect->Bottom;

	Camera->XCenter = Camera->Left + ( Width  * 0.5f ) - 0.5f;
	Camera->YCenter = Camera->Top  + ( Height * 0.5f ) - 0.5f;

	{
		geFloat Numerator = 2.0f * OneOverFov * Camera->Scale;
		double Angle;
		Angle =  atan(Numerator / Camera->XRatio);
		Camera->CosViewAngleX = (geFloat)cos(Angle);
		Camera->SinViewAngleX = (geFloat)sin(Angle);

		Angle =  atan(Numerator / Camera->YRatio);
		Camera->CosViewAngleY = (geFloat)cos(Angle);
		Camera->SinViewAngleY = (geFloat)sin(Angle);
	}
}

//========================================================================================
//	geCamera_FillDriverInfo
//	HACK!!!!
//========================================================================================
void geCamera_FillDriverInfo(geCamera *Camera)
{
	// this is for the software driver to cache out some stuff

#pragma message ("Camera.c : remove _FillDriverInfo, and thereby GlobalInfo!" )
extern GInfo GlobalInfo;
	assert(Camera);

	GlobalInfo.XScale		=-Camera->Scale;
	GlobalInfo.YScale		=-Camera->Scale;
	GlobalInfo.XScaleInv	=1.0f / GlobalInfo.XScale;
	GlobalInfo.YScaleInv	=1.0f / GlobalInfo.YScale;
	GlobalInfo.XCenter		=Camera->XCenter;
	GlobalInfo.YCenter		=Camera->YCenter;

	// Temp hack
	GlobalInfo.CXForm		=Camera->XForm;
	GlobalInfo.Pov			=Camera->Pov;
	GlobalInfo.ZScale		=Camera->ZScale;

	geXForm3d_Rotate(&Camera->XForm, &GlobalInfo.Pov, &GlobalInfo.CPov);
}

//========================================================================================
//	geCamera_ScreenPointToWorld
//========================================================================================
GENESISAPI void GENESISCC geCamera_ScreenPointToWorld (	const geCamera	*Camera,
														int32			 ScreenX,
														int32			 ScreenY,
														geVec3d			*Vector)
// Takes a screen X and Y pair, and a camera and generates a vector pointing
// in the direction from the camera position to the screen point.
{
	geVec3d In,Left,Up;
	geVec3d ScaledIn,ScaledLeft,ScaledUp ;
	float	XCenter ;
	float	YCenter ;
	float	Scale ;
	const geXForm3d *pM;

	pM = &(Camera->TransposeXForm);
	XCenter = Camera->XCenter ;
	YCenter = Camera->YCenter ;
	Scale   = Camera->Scale ;

	geXForm3d_GetIn( pM, &In ) ;
	geXForm3d_GetLeft( pM, &Left ) ;
	geXForm3d_GetUp( pM, &Up ) ;
	
	geVec3d_Scale(&In,   Scale, &ScaledIn);
	geVec3d_Scale(&Left, XCenter - ((geFloat)ScreenX), &ScaledLeft );
	geVec3d_Scale(&Up,   YCenter - ((geFloat)ScreenY), &ScaledUp   );

	geVec3d_Copy(&ScaledIn, Vector);
	geVec3d_Add(Vector,		&ScaledLeft,	Vector );
	geVec3d_Add(Vector,		&ScaledUp,		Vector );
	geVec3d_Normalize(Vector);
}


//========================================================================================
//	geCamera_Project
//========================================================================================
GENESISAPI void GENESISCC geCamera_Project(const geCamera *Camera, 
								const geVec3d *PointInCameraSpace, 
								geVec3d *ProjectedPoint)
	// project from camera space to projected space
	// projected space is left-handed.
	// projection is onto x-y plane  x is right, y is down, z is in
{
	geFloat ScaleOverZ;
	geFloat Z;
	assert( Camera != NULL );
	assert( PointInCameraSpace != NULL );
	assert( ProjectedPoint != NULL );

	Z = -PointInCameraSpace->Z;   

	if (Z < CAMERA_MINIMUM_PROJECTION_DISTANCE)
		{
			Z = CAMERA_MINIMUM_PROJECTION_DISTANCE; 
		}

	ScaleOverZ = Camera->Scale / Z;

	ProjectedPoint->Z = Z*Camera->ZScale;   

	ProjectedPoint->X = ( PointInCameraSpace->X * ScaleOverZ ) + Camera->XCenter;
	
	ProjectedPoint->Y = Camera->YCenter - ( PointInCameraSpace->Y * ScaleOverZ );
}


//========================================================================================
//	geCamera_ProjectZ
//========================================================================================
GENESISAPI void GENESISCC geCamera_ProjectZ(const geCamera *Camera, 
								const geVec3d *PointInCameraSpace, 
								geVec3d *ProjectedPoint)
	// project from camera space to projected space
	// projected space is not right-handed.
	// projection is onto x-y plane  x is right, y is down, z is in
	// projected point.z is set to 1/Z
{
	geFloat OneOverZ;
	geFloat ScaleOverZ;
	geFloat Z;
	assert( Camera != NULL );
	assert( PointInCameraSpace != NULL );
	assert( ProjectedPoint != NULL );

	Z = -PointInCameraSpace->Z;   

	if (Z < CAMERA_MINIMUM_PROJECTION_DISTANCE)
		{
			Z = CAMERA_MINIMUM_PROJECTION_DISTANCE; 
		}

	OneOverZ = 1.0f / Z;
	ScaleOverZ = Camera->Scale *  (OneOverZ);

	ProjectedPoint->Z = OneOverZ;   

	ProjectedPoint->X = ( PointInCameraSpace->X * ScaleOverZ ) + Camera->XCenter;
	
	ProjectedPoint->Y = Camera->YCenter - ( PointInCameraSpace->Y * ScaleOverZ );
}




//========================================================================================
//	geCamera_ProjectAndClamp
//========================================================================================
void GENESISCC geCamera_ProjectAndClamp(const geCamera *Camera, 
										const geVec3d *PointInCameraSpace, 
										geVec3d *ProjectedPoint)
	// project from camera space to projected space
	// projected space is not right-handed.
	// projection is onto x-y plane  x is right, y is down, z is in
	// points outside the clipping rect are clamped to the clipping rect
{
	geFloat ScaleOverZ;
	geFloat X,Y,Z;
	assert( Camera != NULL );
	assert( PointInCameraSpace != NULL );
	assert( ProjectedPoint != NULL );

	Z = -PointInCameraSpace->Z;   

	if (Z < CAMERA_MINIMUM_PROJECTION_DISTANCE)
	{
		Z = CAMERA_MINIMUM_PROJECTION_DISTANCE; 
	}

	ScaleOverZ = Camera->Scale / Z;

	ProjectedPoint->Z = Z*Camera->ZScale;   

	X = ( PointInCameraSpace->X * ScaleOverZ ) + Camera->XCenter;
	
	if (X < Camera->Left)
		X = Camera->Left;
	else if (X > Camera->Right)	
		X = Camera->Right;

	ProjectedPoint->X = X;
	
	Y = Camera->YCenter - ( PointInCameraSpace->Y * ScaleOverZ );

	if (Y < Camera->Top)
		Y = Camera->Top;
	else if (Y > Camera->Bottom) 
		Y = Camera->Bottom;
	
	ProjectedPoint->Y = Y;
}


//========================================================================================
//	geCamera_GetViewAngleXSinCos
//========================================================================================
void GENESISCC geCamera_GetViewAngleXSinCos( const geCamera *Camera, geFloat *SinAngle, geFloat *CosAngle )
{
	assert( Camera != NULL );
	assert( SinAngle );
	assert( CosAngle );
	*SinAngle = Camera->SinViewAngleX;
	*CosAngle = Camera->CosViewAngleX;
}

//========================================================================================
//	geCamera_GetViewAngleYSinCos
//========================================================================================
void GENESISCC geCamera_GetViewAngleYSinCos( const geCamera *Camera, geFloat *SinAngle, geFloat *CosAngle )
{
	assert( Camera != NULL );
	assert( SinAngle );
	assert( CosAngle );
	*SinAngle = Camera->SinViewAngleY;
	*CosAngle = Camera->CosViewAngleY;
}

//============================================================================================
//	geCamera_Transform
//============================================================================================
GENESISAPI void GENESISCC geCamera_Transform(const geCamera *Camera, 
						const geVec3d *WorldSpacePoint, 
						      geVec3d *CameraSpacePoint)
{
	assert( Camera );
	assert( WorldSpacePoint );
	assert( CameraSpacePoint );

	// Would be better if xform3d_transform was assembly, or a macro, or anything
	geXForm3d_Transform(&(Camera->XForm),WorldSpacePoint,CameraSpacePoint);
}


//============================================================================================
//	geCamera_TransformArray
//============================================================================================
GENESISAPI void GENESISCC geCamera_TransformArray(const geCamera *Camera, 
						const geVec3d *WorldSpacePointPtr, 
						      geVec3d *CameraSpacePointPtr, int count)
{
	assert( Camera );
	assert( WorldSpacePointPtr );
	assert( CameraSpacePointPtr );
	// use transformarray!!
	while(count--)
	{
		geXForm3d_Transform(&(Camera->XForm),WorldSpacePointPtr++,CameraSpacePointPtr++);
	}
}

//============================================================================================
//	geCamera_TransformAndProjectArray
//============================================================================================
GENESISAPI void GENESISCC geCamera_TransformAndProjectArray(const geCamera *Camera, 
						const geVec3d *WorldSpacePointPtr, 
						      geVec3d *ProjectedSpacePointPtr,
							int count)
{
	assert( Camera );
	assert( WorldSpacePointPtr );
	assert( ProjectedSpacePointPtr );
	while(count--)
	{
		geCamera_TransformAndProject(Camera,WorldSpacePointPtr++,ProjectedSpacePointPtr++);
	}
}

//============================================================================================
//	geCamera_TransformAndProjectLArray
//============================================================================================
GENESISAPI void GENESISCC geCamera_TransformAndProjectLArray(const geCamera *Camera, 
						const GE_LVertex *WorldSpacePointPtr, 
						      GE_TLVertex *ProjectedSpacePointPtr, int count)
{
	assert( Camera );
	assert( WorldSpacePointPtr );
	assert( ProjectedSpacePointPtr );
	while(count--)
	{
		geCamera_TransformAndProjectL(Camera,WorldSpacePointPtr++,ProjectedSpacePointPtr++);
	}
}

//============================================================================================
//	geCamera_TransformAndProject
//============================================================================================
GENESISAPI void GENESISCC geCamera_TransformAndProject(const geCamera *Camera,
								const geVec3d *Point, 
								geVec3d *ProjectedPoint)
	// project from *WORLD* space to projected space
	// projected space is not right-handed.
	// projection is onto x-y plane  x is right, y is down, z is in
{
	geFloat Z;

	assert( Camera );
	assert( Point );
	assert( ProjectedPoint );

	geXForm3d_Transform(&(Camera->XForm),Point,ProjectedPoint);

	Z = - ProjectedPoint->Z;

	Z = max(Z,CAMERA_MINIMUM_PROJECTION_DISTANCE);

	ProjectedPoint->Z = Z*Camera->ZScale;

	Z = Camera->Scale / Z;

	ProjectedPoint->X =   ( ProjectedPoint->X * Z ) + Camera->XCenter;
	ProjectedPoint->Y = - ( ProjectedPoint->Y * Z ) + Camera->YCenter;
}


//============================================================================================
//	geCamera_TransformAndProjectL
//============================================================================================
GENESISAPI void GENESISCC geCamera_TransformAndProjectL(const geCamera *Camera,
								const GE_LVertex *Point, 
								GE_TLVertex *ProjectedPoint)
	// project from *WORLD* space to projected space
	// projected space is not right-handed.
	// projection is onto x-y plane  x is right, y is down, z is in
{
	geFloat ScaleOverZ;
	geFloat Z;

	assert( Camera );
	assert( Point );
	assert( ProjectedPoint );

	geXForm3d_Transform(&(Camera->XForm),(geVec3d *)Point,(geVec3d *)ProjectedPoint);

	Z = - ProjectedPoint->z;

	Z = max(Z,CAMERA_MINIMUM_PROJECTION_DISTANCE);

	ScaleOverZ = Camera->Scale / Z;

	ProjectedPoint->z = Z*Camera->ZScale;
	ProjectedPoint->x =   ( ProjectedPoint->x * ScaleOverZ ) + Camera->XCenter;
	ProjectedPoint->y = - ( ProjectedPoint->y * ScaleOverZ ) + Camera->YCenter;

	ProjectedPoint->u = Point->u;
	ProjectedPoint->v = Point->v;
	ProjectedPoint->r = Point->r;
	ProjectedPoint->g = Point->g;
	ProjectedPoint->b = Point->b;
	ProjectedPoint->a = Point->a;
}

//========================================================================================
//	geCamera_SetWorldSpaceXForm
//========================================================================================
GENESISAPI geBoolean GENESISCC geCamera_SetWorldSpaceXForm(geCamera *Camera, const geXForm3d *XForm)
{
	assert(Camera != NULL);
	assert(XForm != NULL);

	Camera->TransposeXForm = *XForm;		// Make a copy of the model XForm
	
	// Convert the model transform into a camera xform...
	//geCamera_ConvertModelToCamera(XForm, &Camera->XForm);
	geXForm3d_GetTranspose(XForm, &Camera->XForm);

	Camera->Pov = XForm->Translation;

	return GE_TRUE;
}

//========================================================================================
//	geCamera_SetWorldSpaceVisXForm
//========================================================================================
GENESISAPI geBoolean GENESISCC geCamera_SetWorldSpaceVisXForm(geCamera *Camera, const geXForm3d *XForm)
{
	assert(Camera != NULL);
		
	if (XForm)
	{
		Camera->TransposeVisXForm = *XForm;		// Make a copy of the original XForm
		Camera->VisPov = XForm->Translation;
		geXForm3d_GetTranspose(XForm, &Camera->VisXForm);

		Camera->UseVisPov = GE_TRUE;
	}
	else
	{
		Camera->UseVisPov = GE_FALSE;
	}

	return GE_TRUE;
}

//============================================================================================
//	geCamera_GetWorldSpaceXForm
//	Gets the xform originally set by the user
//============================================================================================
GENESISAPI const geXForm3d * GENESISCC geCamera_GetWorldSpaceXForm( const geCamera *Camera)
{
	assert(Camera );
	return &(Camera->TransposeXForm);
}

//========================================================================================
//	geCamera_GetCameraSpaceXForm
//========================================================================================
const geXForm3d * GENESISCC geCamera_GetCameraSpaceXForm( const geCamera *Camera)
{
	assert(Camera != NULL);
	return &(Camera->XForm);
}

//============================================================================================
//	geCamera_GetCameraSpaceVisXForm
//============================================================================================
GENESISAPI const geXForm3d * GENESISCC geCamera_GetCameraSpaceVisXForm( const geCamera *Camera)
{
	assert(Camera != NULL);

	if (Camera->UseVisPov)
		return &(Camera->VisXForm);
	else
		return &(Camera->XForm);
}

//============================================================================================
//	geCamera_GetWorldSpaceVisXForm
//============================================================================================
GENESISAPI const geXForm3d * GENESISCC geCamera_GetWorldSpaceVisXForm( const geCamera *Camera)
{
	assert(Camera != NULL);

	if (Camera->UseVisPov)
		return &(Camera->TransposeVisXForm);
	else
		return &(Camera->TransposeXForm);
}

//=====================================================================================
//	geCamera_GetPov
//=====================================================================================
const geVec3d *GENESISCC geCamera_GetPov(const geCamera *Camera)
{
	assert( Camera != NULL );
	return &(Camera->Pov);
}

//=====================================================================================
//	geCamera_GetVisPov
//=====================================================================================
const geVec3d *GENESISCC geCamera_GetVisPov(const geCamera *Camera)
{
	assert( Camera != NULL );

	if (Camera->UseVisPov)
		return &(Camera->VisPov);
	else
		return &(Camera->Pov);
}

//========================================================================================
//	geCamera_ConvertWorldSpaceToCameraSpace
//	Converts a worldspace Xform to a cameraspace xform
//========================================================================================
GENESISAPI geBoolean GENESISCC geCamera_ConvertWorldSpaceToCameraSpace(const geXForm3d *WXForm, geXForm3d *CXForm)
{
	// The rotation portion is just the transpose of the model xform
	CXForm->AX = WXForm->AX;
	CXForm->AY = WXForm->BX;
	CXForm->AZ = WXForm->CX;

	CXForm->BX = WXForm->AY;
	CXForm->BY = WXForm->BY;
	CXForm->BZ = WXForm->CY;

	CXForm->CX = WXForm->AZ;
	CXForm->CY = WXForm->BZ;
	CXForm->CZ = WXForm->CZ;

	CXForm->Translation = WXForm->Translation;

	geVec3d_Inverse(&CXForm->Translation);

	// Rotate the translation in the new camera matrix
	geXForm3d_Rotate(CXForm, &CXForm->Translation, &CXForm->Translation);

	return GE_TRUE;
}
