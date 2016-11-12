/****************************************************************************************/
/*  SPRITE.C                                                                            */
/*                                                                                      */
/*  Author: Michael R. Brumm	                                                          */
/*  Description: Sprite implementation                                                  */
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
/****************************************************************************************/

#include <assert.h>
#include <math.h>

#include "World.h"
#include "Trace.h"

#include "Sprite.h"

#include "ErrorLog.h"


#define BIG_DISTANCE 30000.0f

extern geBoolean geBitmap_IsValid(const geBitmap *Bmp);

// sprites are rectangular
#define SPRITE_NUM_CORNERS 4

// unit corners used to create scaled corners
#define UNIT_CORNERS	{ {0.5f, -0.5f}, {-0.5f, -0.5f}, {-0.5f, 0.5f}, {0.5f, 0.5f} }
const geCoordinate	UnitCorners[SPRITE_NUM_CORNERS] = UNIT_CORNERS;


// dynamic light information is reused between sprites
// saving resources and allocation time
typedef struct geSprite_DynamicLight
{
	geVec3d		Normal;
	geColor		Color;
	geFloat		Distance;
	geFloat		Radius;
} geSprite_DynamicLight;

static geSprite_DynamicLight geSpriteDynamicLights[MAX_DYNAMIC_LIGHTS];


// frustum clipping arrays are reused between sprites
// saving resources and allocation time
#define MAX_TEMP_VERTS	30

static Surf_TexVert		UnclippedUVRGBA[MAX_TEMP_VERTS];

static geVec3d			FrustumClippedVertexes1[MAX_TEMP_VERTS];
static Surf_TexVert		FrustumClippedUVRGBA1[MAX_TEMP_VERTS];
static geVec3d			FrustumClippedVertexes2[MAX_TEMP_VERTS];
static Surf_TexVert		FrustumClippedUVRGBA2[MAX_TEMP_VERTS];

static int32			FrustumNumClippedTexturedLitVertices;

static Surf_TLVertex	FrustumClippedTexturedLitVertexes[MAX_TEMP_VERTS];


typedef struct geSprite
{
	// number of owners
	int32				RefCount;				

	// bitmaps used to texture the sprite
	geBitmap *	Bitmap;
	geBitmap *	BackfaceBitmap;

	// backface properties
	geBoolean		BackfaceEnabled;
	geBoolean		BackfaceMirrorImage;

	// makes the sprite always face the camera
	geBoolean		AlwaysFaceCamera;

	// transform affects position and orientation of the sprite
	geXForm3d		Transform;

	// internal transform also affects position and orientation of the sprite
	// allows the sprite to be offset from its center of rotation
	geBoolean		InternalTransformUsed;
	geXForm3d		InternalTransform;

	// actual center of sprite (both external and internal transforms added)
	geVec3d			Position;

	// scale of the sprite
	// allows sprite to be sized without changing
	geFloat			ScaleX;
	geFloat			ScaleY;

	// cached corners and vertexes
	// corners are used to build the vertexes
	// vertexes are used to build final screen polys
	geCoordinate	Corners[SPRITE_NUM_CORNERS];
	geVec3d			Vertexes[SPRITE_NUM_CORNERS];

	// texture parameters
	geFloat			TextureOffsetX;
	geFloat			TextureOffsetY;
	geFloat			TextureScaleX;
	geFloat			TextureScaleY;

	// cached texture mapping information
	geUV			UVs[SPRITE_NUM_CORNERS];
	geUV			BackfaceUVs[SPRITE_NUM_CORNERS];

	// lighting parameters
	// used to build cached lighting information
	geColor			AmbientLight;
	geBoolean		UseFillLight;
	geColor			FillLight;
	geVec3d			FillLightNormal;
	geBoolean		UseLightFromFloor;
	int32			MaximumDynamicLightsToUse;

	// cached lighting information
	GE_RGBA			RGBA;
	GE_RGBA			BackfaceRGBA;

	// surface normal of the sprite's face
	geVec3d			SurfaceNormal;
	geBoolean		LightingUsesSurfaceNormal;

	// bounding box which other objects collide against
	geVec3d			BoundingBoxMinCorner;
	geVec3d			BoundingBoxMaxCorner;

	// user data
	void *			UserData;

	// tracks changes made and whether cached data needs
	// to be updated
	geBoolean		TransformChanged;
	geBoolean		LightingChanged;

} geSprite;


	// these are useful globals to monitor resources
int32 geSprite_Count       = 0;
int32 geSprite_RefCount    = 0;


__inline static void geSprite_UpdatePosition(geSprite *S)
{
	if (S->AlwaysFaceCamera)
		S->Position = S->Transform.Translation;
	else
		geVec3d_Add( &(S->Transform.Translation), &(S->InternalTransform.Translation), &(S->Position) );
}


__inline static void geSprite_UpdateCorners(geSprite *S)
{
	int i;

	for (i = 0; i < SPRITE_NUM_CORNERS; i++)
	{
		S->Corners[i].X = UnitCorners[i].X * S->ScaleX;
		S->Corners[i].Y = UnitCorners[i].Y * S->ScaleY;
	}
}


__inline static void geSprite_UpdateVertexes(geSprite *S)
{
	int i;

	// create a rectangle of the correct size around the origin
	for (i = 0; i < SPRITE_NUM_CORNERS; i++)
	{
		S->Vertexes[i].X = S->Corners[i].X;
		S->Vertexes[i].Y = S->Corners[i].Y;
		S->Vertexes[i].Z = 0.0f;
	}

	// apply the internal and external transform if an internal transform is used
	if (S->InternalTransformUsed)
		geXForm3d_TransformArray(&(S->InternalTransform), S->Vertexes, S->Vertexes, SPRITE_NUM_CORNERS);

	// apply the sprites external transform
	geXForm3d_TransformArray(&(S->Transform), S->Vertexes, S->Vertexes, SPRITE_NUM_CORNERS);
}


__inline static void geSprite_UpdateVertexesToFaceCamera(geSprite *S, geCamera *Camera)
{
	int i;
	const geXForm3d *CameraXForm;
	geVec3d Left;
	geVec3d Up;
	geVec3d In;
	geFloat Dot;

	// optimized out:
	//
	//		geXForm3d FaceCameraXForm;

	// get the camera's transform
	CameraXForm = geCamera_GetWorldSpaceXForm(Camera);

	// get the vector opposite to the direction the camera is looking
	//
	// optimized from:
	//
	//		geXForm3d_GetIn(CameraXForm, &In);
	//		In.X = -In.X;
	//		In.Y = -In.Y;
	//		In.Z = -In.Z;
	//
	In.X = CameraXForm->AZ;
	In.Y = CameraXForm->BZ;
	In.Z = CameraXForm->CZ;

	// get the up direction of the camera
	geXForm3d_GetUp(CameraXForm, &Up);

	Dot = geVec3d_DotProduct(&Up, &In);

	Up.X = Up.X - (Dot * In.X);
	Up.Y = Up.Y - (Dot * In.Y);
	Up.Z = Up.Z - (Dot * In.Z);

	geVec3d_Normalize(&Up);

	// get the left vector direction for the sprite based on the 'in' and 'up'
	//
	// optimized from:
	//
	//		geVec3d_CrossProduct(&Up, &In, &Left);
	//
	Left.X = (Up.Z * In.Y) - (Up.Y * In.Z);
	Left.Y = (Up.X * In.Z) - (Up.Z * In.X);
	Left.Z = (Up.Y * In.X) - (Up.X * In.Y);

	// build the transform based on the left, up, in, and position of the sprite
	//
	// optimized out:
	//
	//		geXForm3d_SetFromLeftUpIn(&RotationXForm, &Left, &Up, &In);
	//		FaceCameraXForm.Translation = S->Transform.Translation;

	// modify all the vertexes
	for (i = 0; i < SPRITE_NUM_CORNERS; i++)
	{
		// optimized from:
		//
		//		S->Vertexes[i].X = S->Corners[i].X;
		//		S->Vertexes[i].Y = S->Corners[i].Y;
		//		S->Vertexes[i].Z = 0.0f;
		//
		//		geXForm3d_Transform(&FaceCameraXForm, &(S->Vertexes[i]), &(S->Vertexes[i]));
		//
		S->Vertexes[i].X = (S->Corners[i].X * Left.X) + (S->Corners[i].Y * Up.X) + S->Transform.Translation.X;
		S->Vertexes[i].Y = (S->Corners[i].X * Left.Y) + (S->Corners[i].Y * Up.Y) + S->Transform.Translation.Y;
		S->Vertexes[i].Z = (S->Corners[i].X * Left.Z) + (S->Corners[i].Y * Up.Z) + S->Transform.Translation.Z;
	}
}


__inline static void geSprite_UpdateSurfaceNormal(geSprite *S)
{
	if (S->InternalTransformUsed)
	{
		// rotate the surface normal by the internal transform
		//
		// optimized from:
		//
		//		S->SurfaceNormal.X = 0.0f;
		//		S->SurfaceNormal.Y = 0.0f;
		//		S->SurfaceNormal.Z = -1.0f;
		// 
		//		geXForm3d_Rotate(&(S->InternalTransform), &(S->SurfaceNormal), &(S->SurfaceNormal));
		//
		S->SurfaceNormal.X = -S->InternalTransform.AZ;
		S->SurfaceNormal.Y = -S->InternalTransform.BZ;
		S->SurfaceNormal.Z = -S->InternalTransform.CZ;

		// rotate the surface normal by the external transform
		geXForm3d_Rotate(&(S->Transform), &(S->SurfaceNormal), &(S->SurfaceNormal));

	}

	else
	{
		// rotate the surface normal by the external transform
		//
		// optimized from:
		//
		//		S->SurfaceNormal.X = 0.0f;
		//		S->SurfaceNormal.Y = 0.0f;
		//		S->SurfaceNormal.Z = -1.0f;
		// 
		//		geXForm3d_Rotate(&(S->Transform), &(S->SurfaceNormal), &(S->SurfaceNormal));
		//
		S->SurfaceNormal.X = -S->Transform.AZ;
		S->SurfaceNormal.Y = -S->Transform.BZ;
		S->SurfaceNormal.Z = -S->Transform.CZ;
	}
}


__inline static void geSprite_UpdateSurfaceNormalToFaceCamera(geSprite *S, geCamera *Camera)
{
	// get the camera's transform
	const geXForm3d *CameraXForm;
	CameraXForm = geCamera_GetWorldSpaceXForm(Camera);

	// get the vector opposite to the direction the camera is looking
	//
	// optimized from:
	//
	//		geXForm3d_GetIn(CameraXForm, &(S->SurfaceNormal));
	//		S->SurfaceNormal.X = -S->SurfaceNormal.X;
	//		S->SurfaceNormal.Y = -S->SurfaceNormal.Y;
	//		S->SurfaceNormal.Z = -S->SurfaceNormal.Z;
	//
	S->SurfaceNormal.X = CameraXForm->AZ;
	S->SurfaceNormal.Y = CameraXForm->BZ;
	S->SurfaceNormal.Z = CameraXForm->CZ;
}


__inline static void geSprite_CreateFrustumClippedScreenPoly(geSprite *S, geCamera *Camera, Frustum_Info *FInfo, geBoolean *Render, geBoolean *RenderBackface)
{
	int i;

	const geXForm3d *CameraXForm;

	GFX_Plane *FPlanes;

	geVec3d *pVerts1;
	geVec3d *pVerts2;
	Surf_TexVert *pTexs1;
	Surf_TexVert *pTexs2;
	int32 Length;

	geVec3d CameraNormal;

	// if the face is always facing the camera, the backface will not
	// be rendered
	if (S->AlwaysFaceCamera)
	{
		*RenderBackface = GE_FALSE;
	}

	// if the sprite is not always facing the camera, figure out which
	// face is facing the camera
	else
	{
		// get the camera's transform
		CameraXForm = geCamera_GetWorldSpaceXForm(Camera);

		// get the direction vector from the camera to the sprite
		geVec3d_Subtract(&(S->Position), &(CameraXForm->Translation), &CameraNormal);
		geVec3d_Normalize(&CameraNormal);

		// check to see how similar the sprite's surface normal is to the direction
		// between the camera and the sprite, and determine if the backface is being
		// shown to the camera
		*RenderBackface = (geVec3d_DotProduct(&CameraNormal, &(S->SurfaceNormal)) > 0);

		// if the backface is facing the camera, and it is disabled, then
		// don't render it
		if ( (*RenderBackface) && (!S->BackfaceEnabled) )
		{
			*Render = GE_FALSE;
			return;
		}
	}

	// no sense in rendering a completely transparent face
	if ( ((!(*RenderBackface)) && (S->RGBA.a == 0.0f)) || 
			 ((*RenderBackface) && (S->BackfaceRGBA.a == 0.0f)) )
	{
		*Render = GE_FALSE;
		return;
	}

	// copy the correct texture mappings
	if (*RenderBackface)
	{
		for (i = 0; i < SPRITE_NUM_CORNERS; i++)
		{
			UnclippedUVRGBA[i].u = S->BackfaceUVs[i].u;
			UnclippedUVRGBA[i].v = S->BackfaceUVs[i].v;
		}
	}
	else
	{
		for (i = 0; i < SPRITE_NUM_CORNERS; i++)
		{
			UnclippedUVRGBA[i].u = S->UVs[i].u;
			UnclippedUVRGBA[i].v = S->UVs[i].v;
		}
	}

	// initialize pointers for frustum clipping
	FPlanes = FInfo->Planes;
	pVerts1 = S->Vertexes;
	pTexs1 = UnclippedUVRGBA;
	pVerts2 = FrustumClippedVertexes1;
	pTexs2 = FrustumClippedUVRGBA1;
	Length = SPRITE_NUM_CORNERS;

	// clip the vertexes (including their texture and lighting) to the frustum
	for (i = 0; i < FInfo->NumPlanes; i++, FPlanes++)
	{
		
		if (!Frustum_ClipToPlaneUV(FPlanes, pVerts1, pVerts2, pTexs1, pTexs2, Length, &FrustumNumClippedTexturedLitVertices))
			break;

		assert(FrustumNumClippedTexturedLitVertices < MAX_TEMP_VERTS);
		
		// this is hard to read, but essentially what is happening is that
		// source data is swapping with destination data every frustum clip.
		// in this way, vertexes are further clipped each time
		if (pVerts2 == FrustumClippedVertexes1)
		{
			pVerts1 = FrustumClippedVertexes1;
			pVerts2 = FrustumClippedVertexes2;
			pTexs1 = FrustumClippedUVRGBA1;
			pTexs2 = FrustumClippedUVRGBA2;
		}
		else
		{
			pVerts1 = FrustumClippedVertexes2;
			pVerts2 = FrustumClippedVertexes1;
			pTexs1 = FrustumClippedUVRGBA2;
			pTexs2 = FrustumClippedUVRGBA1;
		}

		Length = FrustumNumClippedTexturedLitVertices;
	}
			
	assert(FrustumNumClippedTexturedLitVertices < MAX_TEMP_VERTS);

	// Not visible or not enough vertexes
	if ( (i != FInfo->NumPlanes) || (FrustumNumClippedTexturedLitVertices < 3) )
	{
		*Render = GE_FALSE;
		return;
	}

	// Transform the face to camera space
	geCamera_TransformArray(Camera, pVerts1, pVerts1, FrustumNumClippedTexturedLitVertices);

	// Project the face, and combine vertex and texture and lighting data into one structure
	Frustum_ProjectRGBA(pVerts1, pTexs1, (DRV_TLVertex*)&FrustumClippedTexturedLitVertexes, FrustumNumClippedTexturedLitVertices, Camera);

	*Render = GE_TRUE;
}


__inline static void geSprite_UpdateBackfaceTextureMap(geSprite *S)
{
	if (S->BackfaceMirrorImage)
	{
		S->BackfaceUVs[0].u = S->UVs[0].u;
		S->BackfaceUVs[0].v = S->UVs[0].v;
		S->BackfaceUVs[1].u = S->UVs[1].u;
		S->BackfaceUVs[1].v = S->UVs[1].v;
		S->BackfaceUVs[2].u = S->UVs[2].u;
		S->BackfaceUVs[2].v = S->UVs[2].v;
		S->BackfaceUVs[3].u = S->UVs[3].u;
		S->BackfaceUVs[3].v = S->UVs[3].v;
	}
	else
	{
		S->BackfaceUVs[0].u = S->UVs[1].u;
		S->BackfaceUVs[0].v = S->UVs[1].v;
		S->BackfaceUVs[1].u = S->UVs[0].u;
		S->BackfaceUVs[1].v = S->UVs[0].v;
		S->BackfaceUVs[2].u = S->UVs[3].u;
		S->BackfaceUVs[2].v = S->UVs[3].v;
		S->BackfaceUVs[3].u = S->UVs[2].u;
		S->BackfaceUVs[3].v = S->UVs[2].v;
	}
}


__inline static void geSprite_UpdateTextureMap(geSprite *S)
{
/*
	S->UVs[0].u = TextureOffsetX;
	S->UVs[0].v = 1 - TextureOffsetY;

	S->UVs[1].u = TextureOffsetX + TextureScaleX;
	S->UVs[1].v = 1 - TextureOffsetY;

	S->UVs[2].u = TextureOffsetX + TextureScaleX;
	S->UVs[2].v = 1 - TextureOffsetY - TextureScaleY;

	S->UVs[3].u = TextureOffsetX;
	S->UVs[3].v = 1 - TextureOffsetY - TextureScaleY;
*/
	// optimized (compiler probably would figure it out, but why take the chance)
	S->UVs[3].u = S->UVs[0].u = S->TextureOffsetX;
	S->UVs[1].v = S->UVs[0].v = 1 - S->TextureOffsetY;

	S->UVs[2].u = S->UVs[1].u = S->TextureOffsetX + S->TextureScaleX;
	S->UVs[1].v = 1 - S->TextureOffsetY;

	S->UVs[3].v = S->UVs[2].v = 1 - S->TextureOffsetY - S->TextureScaleY;

	geSprite_UpdateBackfaceTextureMap(S);
}


#pragma warning(disable : 4700 )
__inline static void geSprite_UpdateLighting(geSprite *S, geWorld *World)
{
	int32 i, j;

	geFloat Intensity;

	geBoolean DoBackface;

	geSprite_DynamicLight TempSwap;

	geFloat Scale;

	geVec3d PositionBelowFloor;

	geBoolean	InsideWorldModel;
	geBoolean	FloorBeneath;

	geVec3d Impact;
	int32		Node;
	int32		Plane;

	GFX_Node			*GFXNodes;
	Surf_SurfInfo	*Surf;
	GE_RGBA				FaceLightmapColor;


	// first apply the ambient light
	S->RGBA.r = S->AmbientLight.r;
	S->RGBA.g = S->AmbientLight.g;
	S->RGBA.b = S->AmbientLight.b;

	// apply the ambient to the backface, if there is one
	DoBackface = ( (S->BackfaceEnabled) && !(S->AlwaysFaceCamera) );
	if (DoBackface)
	{
		S->BackfaceRGBA.r = S->AmbientLight.r;
		S->BackfaceRGBA.g = S->AmbientLight.g;
		S->BackfaceRGBA.b = S->AmbientLight.b;
	}

	// calculate the fill light, if it applies
	if (S->UseFillLight)
	{
		Intensity = geVec3d_DotProduct( &(S->FillLightNormal), &(S->SurfaceNormal) );

		if (Intensity < 0.0f)
		{
			S->RGBA.r -= (Intensity * S->FillLight.r);
			S->RGBA.g -= (Intensity * S->FillLight.g);
			S->RGBA.b -= (Intensity * S->FillLight.b);
		}
		else
		{
			// apply the fill light to the backface, if there is one
			if (DoBackface)
			{
				S->BackfaceRGBA.r += (Intensity * S->FillLight.r);
				S->BackfaceRGBA.g += (Intensity * S->FillLight.g);
				S->BackfaceRGBA.b += (Intensity * S->FillLight.b);
			}
		}
	}

	if (S->MaximumDynamicLightsToUse > 0)
	{
		// a pointer to make things easier
		Light_DLight *DynamicLights = World->LightInfo->DynamicLights;
		
		// start out with no dynamic lights available for lighting
		int32 DLCount = 0;

		// get all the dynamic lights that are active
		for (i = 0; i < MAX_DYNAMIC_LIGHTS; i++)
		{
			if (DynamicLights[i].Active)
			{
				// the normal is a vector distance
				geVec3d Normal;
				geVec3d_Subtract(&(S->Position), &(DynamicLights[i].Pos), &Normal);

				// which is why it can be used to calculate distance (squared)
				geSpriteDynamicLights[DLCount].Distance = (Normal.X * Normal.X) + 
																									(Normal.Y * Normal.Y) + 
																									(Normal.Z * Normal.Z);

				// if the sprite is inside the active dynamic light's radius, then add it to the array
				if (geSpriteDynamicLights[DLCount].Distance < (DynamicLights[i].Radius * DynamicLights[i].Radius))
				{
					// changed QuestOfDreams DSpotLight
					if(DynamicLights[i].Spot)
					{
						geFloat Angle = -geVec3d_DotProduct(&Normal, &DynamicLights[i].Normal);
						if(Angle < DynamicLights[i].Angle)
							continue;
					}
					// end change QuestOfDreams	
					geSpriteDynamicLights[DLCount].Color.r = DynamicLights[i].Color.r;
					geSpriteDynamicLights[DLCount].Color.g = DynamicLights[i].Color.g;
					geSpriteDynamicLights[DLCount].Color.b = DynamicLights[i].Color.b;
					geSpriteDynamicLights[DLCount].Radius = DynamicLights[i].Radius;
					// this normal will be normalized later
					geSpriteDynamicLights[DLCount].Normal = Normal;
					DLCount++;
				}
			}
		}

		// try to eliminate some easy out possibilities
		if (DLCount > 1)
		{
			// if there is only one light needed, just make sure the closest
			// is in the first location
			if ( (S->MaximumDynamicLightsToUse == 1) )
			{
				for (i = 1; i < DLCount; i++)
				{
					if (geSpriteDynamicLights[i].Distance < geSpriteDynamicLights[0].Distance)
						geSpriteDynamicLights[0] = geSpriteDynamicLights[i];
				}
			}
			// if there is not just one light to get out of the array, then
			// sort the active dynamic lights by distance (squared)
			else
			{
				for (i = 0; i < DLCount; i++)
				{
					for (j = 0; j < (DLCount - 1); j++)
					{
						if (geSpriteDynamicLights[j].Distance > geSpriteDynamicLights[j+1].Distance)
						{
							TempSwap = geSpriteDynamicLights[j];
							geSpriteDynamicLights[j] = geSpriteDynamicLights[j+1];
							geSpriteDynamicLights[j+1] = TempSwap;
						}
					}
				}
			}
		}

		// use whatever dynamic lights are available, under the maximum number
		if (DLCount > S->MaximumDynamicLightsToUse)
			DLCount = S->MaximumDynamicLightsToUse;

		// calculate the effect the closest lights have on the face
		for (i = 0; i < DLCount; i++)
		{
			// makes the code more readable
			geVec3d *LightNormal = &(geSpriteDynamicLights[i].Normal);

			// get the real distance (not the distance squared)
			geFloat Distance = (geFloat)sqrt(geSpriteDynamicLights[i].Distance);

			// see, I told you it would get normalized (although not perfectly)
			if (Distance > 1.0f)
				geVec3d_Scale(LightNormal, (1.0f / Distance), LightNormal);
			else
				Distance = 1.0f;

			Scale = 1.0f - (Distance / geSpriteDynamicLights[i].Radius);

			Intensity	=	geVec3d_DotProduct( LightNormal, &(S->SurfaceNormal) );

			if (Intensity < 0.0f)
			{
				S->RGBA.r -= (Intensity * geSpriteDynamicLights[i].Color.r * Scale);
				S->RGBA.g -= (Intensity * geSpriteDynamicLights[i].Color.g * Scale);
				S->RGBA.b -= (Intensity * geSpriteDynamicLights[i].Color.b * Scale);
			}
			else
			{
				if (DoBackface)
				{
					S->BackfaceRGBA.r += (Intensity * geSpriteDynamicLights[i].Color.r * Scale);
					S->BackfaceRGBA.g += (Intensity * geSpriteDynamicLights[i].Color.g * Scale);
					S->BackfaceRGBA.b += (Intensity * geSpriteDynamicLights[i].Color.b * Scale);
				}
			}
		}
	}

	if (S->UseLightFromFloor)
	{
		PositionBelowFloor.X = S->Position.X;
		PositionBelowFloor.Y = S->Position.Y - BIG_DISTANCE;
		PositionBelowFloor.Z = S->Position.Z;

		// Get shadow hit plane impact point
		InsideWorldModel = Trace_WorldCollisionExact2(World, &(S->Position), &(S->Position), &Impact, &Node, &Plane, NULL);
		FloorBeneath = Trace_WorldCollisionExact2(World, &(S->Position), &PositionBelowFloor, &Impact, &Node, &Plane, NULL);

		// Now find the color of the mesh by getting the lightmap point he is standing on...
		if ( (!InsideWorldModel) && FloorBeneath)
		{
			// make things a bit more readable
			GFXNodes = World->CurrentBSP->BSPData.GFXNodes;

			// get the surface infomation for the node below the sprite
			Surf = &(World->CurrentBSP->SurfInfo[GFXNodes[Node].FirstFace]);

			// if there are lightmap faces, then find the light mapped face below the sprite
			if (Surf->LInfo.Face >= 0)
			{
				// go through each face looking for one below the sprite
				for (i = 0; i < GFXNodes[Node].NumFaces; i++)
				{
					// if the surface is located at the point below the sprite,
					// get the lightmap
					if (Surf_InSurfBoundingBox(Surf, &Impact, 20.0f))
					{
						// setup the lightmap for the surface
						Light_SetupLightmap(&Surf->LInfo, NULL);

						// get the lightmap color at the point and add it to the
						// light of the sprite, and stop looking for any more light
						if (Light_GetLightmapRGB(Surf, &Impact, &FaceLightmapColor))
						{
							S->RGBA.r += FaceLightmapColor.r;
							S->RGBA.g += FaceLightmapColor.g;
							S->RGBA.b += FaceLightmapColor.b;

							if (DoBackface)
							{
								S->BackfaceRGBA.r += FaceLightmapColor.r;
								S->BackfaceRGBA.g += FaceLightmapColor.g;
								S->BackfaceRGBA.b += FaceLightmapColor.b;
							}

							break;
						}
					}
					
					// go to the next surface
					Surf++;
				}
			}
		}
	}

	// fix up any over or under flow
	if (S->RGBA.r > 255.0f)
		S->RGBA.r = 255.0f;
	else if (S->RGBA.r < 0.0f)
		S->RGBA.r = 0.0f;

	if (S->RGBA.g > 255.0f)
		S->RGBA.g = 255.0f;
	else if (S->RGBA.g < 0.0f)
		S->RGBA.g = 0.0f;

	if (S->RGBA.b > 255.0f)
		S->RGBA.b = 255.0f;
	else if (S->RGBA.b < 0.0f)
		S->RGBA.b = 0.0f;

	if (DoBackface)
	{
		// fix up any over or under flow
		if (S->BackfaceRGBA.r > 255.0f)
			S->BackfaceRGBA.r = 255.0f;
		else if (S->BackfaceRGBA.r < 0.0f)
			S->BackfaceRGBA.r = 0.0f;

		if (S->BackfaceRGBA.g > 255.0f)
			S->BackfaceRGBA.g = 255.0f;
		else if (S->BackfaceRGBA.g < 0.0f)
			S->BackfaceRGBA.g = 0.0f;

		if (S->BackfaceRGBA.b > 255.0f)
			S->BackfaceRGBA.b = 255.0f;
		else if (S->BackfaceRGBA.b < 0.0f)
			S->BackfaceRGBA.b = 0.0f;
	}
}
#pragma warning(default : 4700 )


GENESISAPI int32 GENESISCC geSprite_GetCount()
{
	return geSprite_Count;
}


GENESISAPI geSprite *GENESISCC geSprite_Create(geBitmap *SpriteBitmap, geBitmap *SpriteBackfaceBitmap)
{
	geSprite *S;

	if (SpriteBitmap)
	{
		if ( geBitmap_IsValid(SpriteBitmap) == GE_FALSE )
		{
			geErrorLog_Add( ERR_SPRITE_INVALIDBITMAP , NULL);
			return NULL;
		}
	}

	if ( (SpriteBackfaceBitmap) && (SpriteBackfaceBitmap != SpriteBitmap) )
	{
		if ( geBitmap_IsValid(SpriteBackfaceBitmap) == GE_FALSE )
		{
			geErrorLog_Add( ERR_SPRITE_INVALIDBITMAP , NULL);
			return NULL;
		}
	}


	S = GE_RAM_ALLOCATE_STRUCT( geSprite );
	if ( S == NULL )
	{
		geErrorLog_Add( ERR_SPRITE_ENOMEM , NULL);
		return NULL;
	}
	
	S->RefCount = 0;

	S->Bitmap = SpriteBitmap;
	if (SpriteBitmap)
		geBitmap_CreateRef(SpriteBitmap);

	S->BackfaceBitmap = SpriteBackfaceBitmap;
	if ( (SpriteBackfaceBitmap) && (SpriteBitmap != SpriteBackfaceBitmap) )
		geBitmap_CreateRef(SpriteBackfaceBitmap);

	S->BackfaceEnabled = GE_TRUE;
	S->BackfaceMirrorImage = GE_TRUE;

	S->AlwaysFaceCamera = GE_FALSE;
	
	geXForm3d_SetIdentity(&(S->Transform));

	S->InternalTransformUsed = GE_FALSE;
	geXForm3d_SetIdentity(&(S->InternalTransform));

	geSprite_UpdatePosition(S);

	S->ScaleX = 1.0f;
	S->ScaleY = 1.0f;
	geSprite_UpdateCorners(S);

	S->TextureOffsetX = 0.0f;
	S->TextureOffsetY = 0.0f;
	S->TextureScaleX = 1.0f;
	S->TextureScaleY = 1.0f;
	geSprite_UpdateTextureMap(S);

	geVec3d_Clear(&(S->BoundingBoxMinCorner));
	geVec3d_Clear(&(S->BoundingBoxMaxCorner));

	S->AmbientLight.r = 0.1f;		
	S->AmbientLight.g = 0.1f;			
	S->AmbientLight.b = 0.1f;
	S->UseFillLight = GE_FALSE;
	S->UseLightFromFloor = GE_FALSE;
	S->MaximumDynamicLightsToUse = 0;

	S->LightingUsesSurfaceNormal = GE_FALSE;

	S->RGBA.a = 255.0f;
	S->BackfaceRGBA.a = 255.0f;

	S->UserData = NULL;

	S->TransformChanged = GE_TRUE;
	S->LightingChanged = GE_TRUE;

	assert( geSprite_IsValid(S) );

	geSprite_Count++;

	return S;
}	


GENESISAPI void GENESISCC geSprite_CreateRef(geSprite *S)
{
	assert( geSprite_IsValid(S) );

	S->RefCount++;
	geSprite_RefCount++;
}


GENESISAPI void GENESISCC geSprite_Destroy(geSprite **pS)
{
	geSprite *S;
	assert(  pS != NULL );
	assert( *pS != NULL );
	assert( geSprite_IsValid(*pS) );
	
	S = *pS;
	if (S->RefCount > 0)
	{
		S->RefCount --;
		geSprite_RefCount--;
		return;
	}

	if (S->Bitmap)
	{
		geBitmap_Destroy(&(S->Bitmap));
	}

	if ( (S->BackfaceBitmap) && (S->BackfaceBitmap != S->Bitmap) )
	{
		geBitmap_Destroy(&(S->BackfaceBitmap));
	}

	geRam_Free(*pS);
	geSprite_Count--;
	*pS = NULL;
}


GENESISAPI geBoolean GENESISCC geSprite_IsValid(const geSprite *S)
{
	if (S == NULL)
		return GE_FALSE;

	return GE_TRUE;
}


GENESISAPI geBitmap *GENESISCC geSprite_GetBitmap(const geSprite *S)
{
	assert ( (S->Bitmap == NULL) || (geBitmap_IsValid(S->Bitmap)) );

	return S->Bitmap;
}


GENESISAPI geBitmap *GENESISCC geSprite_GetBackfaceBitmap(const geSprite *S)
{
	assert ( (S->BackfaceBitmap == NULL) || (geBitmap_IsValid(S->BackfaceBitmap)) );

	return S->BackfaceBitmap;
}


GENESISAPI void GENESISCC geSprite_GetBackface(const geSprite *S, geBoolean *Enabled, geBoolean *MirrorImage)
{
	assert ( Enabled );
	assert ( MirrorImage );

	*Enabled = S->BackfaceEnabled;
	*MirrorImage = S->BackfaceMirrorImage;
}


GENESISAPI void GENESISCC geSprite_SetBackface(geSprite *S, const geBoolean Enabled, const geBoolean MirrorImage)
{
	assert( geSprite_IsValid(S) );

	// because lighting the backface requires a lot of extra calculations, the
	// backface doesn't have correct lighting if it has been turned off. so, if it
	// is turned on, lighting needs to be updated
	S->LightingChanged = (S->LightingChanged || ((Enabled) && (!S->BackfaceEnabled)) );

	// if the mirror imaging is changed, then update the texture map for the backface
	if (S->BackfaceMirrorImage != MirrorImage)
	{
		S->BackfaceMirrorImage = MirrorImage;
		geSprite_UpdateBackfaceTextureMap(S);
	}

	S->BackfaceEnabled = Enabled;
}


GENESISAPI void GENESISCC geSprite_GetFaceCamera(const geSprite *S, geBoolean *Enabled)
{
	assert( geSprite_IsValid(S) );
	assert( Enabled != NULL );

	*Enabled = S->AlwaysFaceCamera;
}


GENESISAPI void GENESISCC geSprite_SetFaceCamera(geSprite *S, geBoolean Enabled)
{
	assert( geSprite_IsValid(S) );

	// if facing the camera is being disabled, then force an
	// update on everything the transform affects
	if ( (S->AlwaysFaceCamera) && (!Enabled) )
	{
		S->TransformChanged = GE_TRUE;
	}

	S->AlwaysFaceCamera = Enabled;

	geSprite_UpdatePosition(S);
}


GENESISAPI void GENESISCC geSprite_GetPosition(const geSprite *S, geVec3d *Pos)
{
	assert( geSprite_IsValid(S) );
	assert( Pos != NULL );
	assert( geXForm3d_IsOrthonormal(&(S->Transform)) );

	*Pos = S->Transform.Translation;
}


GENESISAPI void GENESISCC geSprite_SetPosition(geSprite *S, const geVec3d *Pos)
{
	assert( geSprite_IsValid(S) );
	assert( Pos != NULL );
	assert( geXForm3d_IsOrthonormal(&(S->Transform)) );

	S->Transform.Translation = *Pos;

	geSprite_UpdatePosition(S);

	S->TransformChanged = GE_TRUE;
}


GENESISAPI void GENESISCC geSprite_GetTransform(const geSprite *S, geXForm3d *Transform)
{
	assert( geSprite_IsValid(S) );
	assert( Transform!= NULL );

	*Transform = S->Transform;
}


GENESISAPI void GENESISCC geSprite_SetTransform(geSprite *S, const geXForm3d *Transform)
{
	assert( geSprite_IsValid(S) );
	assert( Transform );

	S->Transform = *Transform;
	
	geSprite_UpdatePosition(S);

	S->TransformChanged = GE_TRUE;
}


GENESISAPI void GENESISCC geSprite_GetInternalTransform(const geSprite *S, geXForm3d *Transform)
{
	assert( geSprite_IsValid(S) );
	assert( Transform );

	*Transform = S->InternalTransform;
}


GENESISAPI void GENESISCC geSprite_SetInternalTransform(geSprite *S, const geXForm3d *Transform)
{
	assert( geSprite_IsValid(S) );
	assert( Transform );

	S->InternalTransform = *Transform;
	S->InternalTransformUsed = !( geXForm3d_IsIdentity(Transform) );

	geSprite_UpdatePosition(S);

	S->TransformChanged = GE_TRUE;
}


GENESISAPI void GENESISCC geSprite_GetScale(const geSprite *S, geFloat *ScaleX, geFloat *ScaleY)
{
	assert( geSprite_IsValid(S)!=GE_FALSE );

	*ScaleX = S->ScaleX;
	*ScaleY = S->ScaleY;
}


GENESISAPI void GENESISCC geSprite_SetScale(geSprite *S, geFloat ScaleX, geFloat ScaleY)
{
	assert( geSprite_IsValid(S)!=GE_FALSE );

	if ( (S->ScaleX != ScaleX) || (S->ScaleY != ScaleY) )
	{
		S->ScaleX = ScaleX;
		S->ScaleY = ScaleY;

		geSprite_UpdateCorners(S);

		S->TransformChanged = GE_TRUE;
	}
}


GENESISAPI void GENESISCC geSprite_GetExtBox(const geSprite *S, geExtBox *ExtBox)
{
	assert( geSprite_IsValid(S) );
	assert( ExtBox != NULL );
	assert( geXForm3d_IsOrthonormal(&(S->Transform)) );

	geVec3d_Add( &(S->Transform.Translation), &(S->BoundingBoxMinCorner), &(ExtBox->Min));
	geVec3d_Add( &(S->Transform.Translation), &(S->BoundingBoxMaxCorner), &(ExtBox->Max));
}


GENESISAPI void GENESISCC geSprite_GetNonWorldExtBox(const geSprite *S, geExtBox *ExtBox)
{
	assert( geSprite_IsValid(S) );
	assert( ExtBox != NULL );
	
	ExtBox->Min = S->BoundingBoxMinCorner;
	ExtBox->Max = S->BoundingBoxMaxCorner;
}


GENESISAPI void GENESISCC geSprite_SetExtBox(geSprite *S, const geExtBox *ExtBox)
{
	assert( geSprite_IsValid(S) );
	assert( geExtBox_IsValid(ExtBox) );

	S->BoundingBoxMinCorner = ExtBox->Min;
	S->BoundingBoxMaxCorner = ExtBox->Max;
}


GENESISAPI void GENESISCC geSprite_GetTextureParameters(const geSprite *S,
	geFloat *OffsetX,
	geFloat *OffsetY,
	geFloat *ScaleX,
	geFloat *ScaleY)
{
	assert( geSprite_IsValid(S) );
	assert( OffsetX != NULL );
	assert( OffsetY != NULL );
	assert( ScaleX != NULL );
	assert( ScaleY != NULL );

	*OffsetX = S->TextureOffsetX;
	*OffsetY = S->TextureOffsetY;
	*ScaleX = S->TextureScaleX;
	*ScaleY = S->TextureScaleY;
}


GENESISAPI void GENESISCC geSprite_SetTextureParameters(geSprite *S,
	geFloat OffsetX,
	geFloat OffsetY,
	geFloat ScaleX,
	geFloat ScaleY)
{
	assert( geSprite_IsValid(S) );

	S->TextureOffsetX = OffsetX;
	S->TextureOffsetY = OffsetY;
	S->TextureScaleX = ScaleX;
	S->TextureScaleY = ScaleY;

	geSprite_UpdateTextureMap(S);
}


GENESISAPI void GENESISCC geSprite_GetLightingOptions(const geSprite *S,
	geFloat *AmbientLightRed,			
	geFloat *AmbientLightGreen,			
	geFloat *AmbientLightBlue,			
	geBoolean *UseFillLight,
	geVec3d *FillLightNormal,
	geFloat *FillLightRed,				
	geFloat *FillLightGreen,				
	geFloat *FillLightBlue,				
	geBoolean *UseLightFromFloor,
	int32 *MaximumDynamicLightsToUse)
{
	assert( geSprite_IsValid(S) );

	assert( AmbientLightRed != NULL );
	assert( AmbientLightGreen != NULL );			
	assert( AmbientLightBlue != NULL );			
	assert( UseFillLight != NULL );
	assert( FillLightNormal != NULL );
	assert( FillLightRed != NULL );	
	assert( FillLightGreen != NULL );	
	assert( FillLightBlue != NULL );	
	assert( UseLightFromFloor != NULL );
	assert( MaximumDynamicLightsToUse != NULL );

	*AmbientLightRed = S->AmbientLight.r;
	*AmbientLightGreen = S->AmbientLight.g;
	*AmbientLightBlue = S->AmbientLight.b;
	*UseFillLight = S->UseFillLight;
	*FillLightNormal = S->FillLightNormal;
	*FillLightRed = S->FillLight.r;
	*FillLightGreen = S->FillLight.g;
	*FillLightBlue = S->FillLight.b;
	*UseLightFromFloor = S->UseLightFromFloor;
	*MaximumDynamicLightsToUse = S->MaximumDynamicLightsToUse;
}


GENESISAPI void GENESISCC geSprite_SetLightingOptions(geSprite *S,
	geFloat AmbientLightRed,			// 0 .. 255
	geFloat AmbientLightGreen,		// 0 .. 255
	geFloat AmbientLightBlue,			// 0 .. 255
	geBoolean UseFillLight,
	const geVec3d *FillLightNormal,
	geFloat FillLightRed,					// 0 .. 255
	geFloat FillLightGreen,				// 0 .. 255
	geFloat FillLightBlue,				// 0 .. 255
	geBoolean UseLightFromFloor,
	int32 MaximumDynamicLightsToUse	// 0 for none
)
{
	assert( geSprite_IsValid(S) );
	assert( geVec3d_IsValid(FillLightNormal) );

	S->AmbientLight.r = AmbientLightRed;
	S->AmbientLight.g = AmbientLightGreen;
	S->AmbientLight.b = AmbientLightBlue;
	S->UseFillLight = UseFillLight;
	S->FillLightNormal = *FillLightNormal;
	S->FillLight.r = FillLightRed;
	S->FillLight.g = FillLightGreen;
	S->FillLight.b = FillLightBlue;
	S->UseLightFromFloor = UseLightFromFloor;
	S->MaximumDynamicLightsToUse = MaximumDynamicLightsToUse;

	// if fill light or dynamic lights are now used, and they weren't before,
	// then the surface normal will be needed.
	//
	// if the transform has changed, then it will be updated next render, so
	// don't do it now
	//
	// if the camera always faces the camera, then this will be updated every
	// render (and using a different normal), so don't do it now.
	if ( (!S->TransformChanged) && 
			 (!S->AlwaysFaceCamera) &&
			 (!S->LightingUsesSurfaceNormal) && ((UseFillLight) || (MaximumDynamicLightsToUse > 0)) )
		geSprite_UpdateSurfaceNormal(S);

	// store whether vertexes are required for lighting (only for fill lights and dynamic lights)
	S->LightingUsesSurfaceNormal = (S->UseFillLight) || (S->MaximumDynamicLightsToUse > 0);
}


GENESISAPI void GENESISCC geSprite_GetAlpha(const geSprite *S, geFloat *Alpha, geFloat *BackfaceAlpha)
{
	assert( geSprite_IsValid(S) );
	assert( Alpha );
	assert( BackfaceAlpha );

	*Alpha = S->RGBA.a;
	*BackfaceAlpha = S->BackfaceRGBA.a;
}


GENESISAPI void GENESISCC geSprite_SetAlpha(geSprite *S, geFloat Alpha, geFloat BackfaceAlpha)
{
	assert( geSprite_IsValid(S) );

	S->RGBA.a = Alpha;
	S->BackfaceRGBA.a = BackfaceAlpha;

	// alphas cannot be less than 0 or greater than 255
	if (S->RGBA.a < 0)
		S->RGBA.a = 0.0f;
	else if (S->RGBA.a > 255.0f)
		S->RGBA.a = 255.0f;
	if (S->BackfaceRGBA.a < 0)
		S->BackfaceRGBA.a = 0.0f;
	else if (S->BackfaceRGBA.a > 255.0f)
		S->BackfaceRGBA.a = 255.0f;
}


GENESISAPI void *GENESISCC geSprite_GetUserData(const geSprite *S)
{
	assert( geSprite_IsValid(S) );

	return S->UserData;
}


GENESISAPI void GENESISCC geSprite_SetUserData(geSprite *S, void *UserData)
{
	assert( geSprite_IsValid(S) );

	S->UserData = UserData;
}


geBoolean GENESISCC geSprite_RenderPrep(geSprite *S, geWorld *World)
{
	assert( geSprite_IsValid(S) );

	// if the sprite uses a front face bitmap, add the bitmap to the world
	if (S->Bitmap)
	{
		if ( geWorld_AddBitmap(World, S->Bitmap) == GE_FALSE )
		{
			geErrorLog_AddString(-1, "Sprite_RenderPrep : World_AddBitmap", NULL);
			return GE_FALSE;
		}
	}

	// if the sprite uses a backface bitmap (and it is not the same as the front),
	// add the bitmap to the world
	if ( (S->BackfaceBitmap) && (S->BackfaceBitmap != S->Bitmap) )
	{
		if ( geWorld_AddBitmap(World, S->Bitmap) == GE_FALSE )
		{
			geErrorLog_AddString(-1, "Sprite_RenderPrep : World_AddBitmap", NULL);
			return GE_FALSE;
		}
	}
	
	return GE_TRUE;
}


geBoolean GENESISCC geSprite_RenderThroughFrustum(geSprite *S, geEngine *Engine, geWorld *World, geCamera *Camera, Frustum_Info *FInfo)
{
	int i;
	geBoolean Render;
	geBoolean RenderBackface;

	assert( geSprite_IsValid(S) );

	// if the sprite always faces the camera, the surface normal and lighting
	// may need to be updated
	if (S->AlwaysFaceCamera)
	{
		// vertexes are needed both to build the final screen poly which is rendered,
		// but also for lighting
		geSprite_UpdateVertexesToFaceCamera(S, Camera);

		// only modify the surface normal if lighting needs it
		if (S->LightingUsesSurfaceNormal)
		{
			geSprite_UpdateSurfaceNormalToFaceCamera(S, Camera);
			S->LightingChanged = GE_TRUE;
		}

		// if the sprite uses ambient light from the floor and its transform has
		// changed (transform includes location), then update the lighting
		else if ( (S->TransformChanged) && (S->UseLightFromFloor) )
		{
			S->LightingChanged = GE_TRUE;
		}
	}

	// otherwise only update the vertexes if the transform has changed
	else if (S->TransformChanged)
	{
		// vertexes are needed both to build the final screen poly which is rendered,
		// but also for lighting
		geSprite_UpdateVertexes(S);

		// update the surface normal
		// this is needed to determine lighting and which face is being viewed
		geSprite_UpdateSurfaceNormal(S);

		// if the sprite uses the surface normal to light its faces, then
		// update the surface normal and update the lighting
		//
		// if the sprite uses ambient light from the floor and its transform has
		// changed (transform includes location), then update the lighting
		if ( (S->LightingUsesSurfaceNormal) || (S->UseLightFromFloor) )
		{
			S->LightingChanged = GE_TRUE;
		}

		S->TransformChanged = GE_FALSE;
	}

	// generate the frustum clipped screen poly based on the vertexes for the camera
	geSprite_CreateFrustumClippedScreenPoly(S, Camera, FInfo, &Render, &RenderBackface);

	// only render if there is something to render
	if (Render)
	{
		// moved inside here because there is no sense in dynamically
		// lighting the vertexes if they won't be drawn.
		// update the lighting only if lighting has changed or
		// lighting uses dynamic lights (which may change)
		if ( (S->LightingChanged) || (S->MaximumDynamicLightsToUse > 0) )
		{
			geSprite_UpdateLighting(S, World);
			S->LightingChanged = GE_FALSE;
		}

		// render the poly using the front or backface data
		if (RenderBackface)
		{
			// add the lighting data to the poly
			for (i = 0; i < FrustumNumClippedTexturedLitVertices; i++)
			{
				FrustumClippedTexturedLitVertexes[i].r = S->BackfaceRGBA.r;
				FrustumClippedTexturedLitVertexes[i].g = S->BackfaceRGBA.g;
				FrustumClippedTexturedLitVertexes[i].b = S->BackfaceRGBA.b;
				FrustumClippedTexturedLitVertexes[i].a = S->BackfaceRGBA.a;
			}

			// render the poly using the backface bitmap
			geEngine_RenderPoly(Engine, (GE_TLVertex*)FrustumClippedTexturedLitVertexes, FrustumNumClippedTexturedLitVertices, S->BackfaceBitmap, 0);
		}
		else
		{
			// add the lighting data to the poly
			for (i = 0; i < FrustumNumClippedTexturedLitVertices; i++)
			{
				FrustumClippedTexturedLitVertexes[i].r = S->RGBA.r;
				FrustumClippedTexturedLitVertexes[i].g = S->RGBA.g;
				FrustumClippedTexturedLitVertexes[i].b = S->RGBA.b;
				FrustumClippedTexturedLitVertexes[i].a = S->RGBA.a;
			}

			// render the poly using the front bitmap
			geEngine_RenderPoly(Engine, (GE_TLVertex*)FrustumClippedTexturedLitVertexes, FrustumNumClippedTexturedLitVertices, S->Bitmap, 0);
		}
	}
	
	return GE_TRUE;
}
