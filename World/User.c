/****************************************************************************************/
/*  User.c                                                                              */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: User poly's                                                            */
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
#include <Windows.h>

#include "User.h"
#include "World.h"
#include "Ram.h"
#include "ErrorLog.h"
#include "System.h"
#include "Surface.h"
#include "Genesis.h"
#include "Camera.h"
#include "Frustum.h"
#include "Plane.h"

#include "DCommon.h"

#include "Bitmap._h"

extern int32	MirrorRecursion;					// GLOBAL!!! in World.c

//=====================================================================================
//	Local static globals
//=====================================================================================
static	geEngine		*gEngine;
static	geWorld			*gWorld;
static  geCamera		*gCamera;
static	GFX_Leaf		*gGFXLeafs;
static	GFX_Model		*gGFXModels;
static	World_BSP		*gBSP;
static	Frustum_Info	gWorldSpaceFrustum;

static	gePoly			*SortedPolys[USER_MAX_SORTED_POLYS];

//=====================================================================================
//	Local Static Function Prototypes
//=====================================================================================
static geBoolean RenderTexturedPoint(DRV_Driver *RDriver, gePoly *Poly, Frustum_Info *FInfo, geCamera *Camera);
static void RenderTexturedPoly(DRV_Driver *RDriver, gePoly *Poly, Frustum_Info *FInfo, geCamera *Camera);
static void RenderGouraudPoly(DRV_Driver *RDriver, gePoly *Poly, Frustum_Info *FInfo, geCamera *Camera);

static geBoolean RenderUserPoly(geCamera *Camera, gePoly *Poly);

static void geWorld_LinkPolyToLeaf(const geWorld *World, gePoly *Poly);
static void geWorld_UnLinkPolyFromLeaf(gePoly *Poly);

#ifdef _DEBUG
geBoolean geWorld_PolyIsValid(gePoly *Poly)
{
	if (!Poly)
		return GE_FALSE;

	if (Poly->Self1 != Poly)
		return GE_FALSE;

	if (Poly->Self2 != Poly)
		return GE_FALSE;

	return GE_TRUE;
}
#endif

//=====================================================================================
//	User_EngineInit
//=====================================================================================
geBoolean User_EngineInit(geEngine *Engine)
{
	User_Info		*Info;

	assert(Engine->UserInfo == NULL);

	Info = GE_RAM_ALLOCATE_STRUCT(User_Info);

	if (!Info)
	{
		geErrorLog_Add(GE_ERR_OUT_OF_MEMORY, NULL);
		return GE_FALSE;;
	}

	memset(Info, 0, sizeof(User_Info));

	Engine->UserInfo = Info;

	return GE_TRUE;
}

//=====================================================================================
//	User_EngineShutdown
//=====================================================================================
void User_EngineShutdown(geEngine *Engine)
{
	User_Info			*Info;

	assert(Engine != NULL);

	Info = Engine->UserInfo;

	if (!Info)
		return;		// Nothing to do...

	geRam_Free(Info);

	Engine->UserInfo = NULL;
}

//=====================================================================================
//	User_WorldInit
//=====================================================================================
geBoolean User_WorldInit(geWorld *World)
{
	User_Info			*Info;

	assert(World != NULL);

	Info = GE_RAM_ALLOCATE_STRUCT(User_Info);
	
	assert(Info != NULL);

	if (!Info)
	{
		geErrorLog_Add(GE_ERR_OUT_OF_MEMORY, NULL);
		return GE_FALSE;
	}

	memset(Info, 0, sizeof(User_Info));

	World->UserInfo = Info;

	return GE_TRUE;	
}

//=====================================================================================
//	User_WorldShutdown
//=====================================================================================
void User_WorldShutdown(geWorld *World)
{
	User_Info			*Info;

	assert(World != NULL);

	Info = World->UserInfo;

	if (!Info)
		return;		// Nothing to do...

	geRam_Free(Info);

	World->UserInfo = NULL;
}

//=====================================================================================
//	
//=====================================================================================
static int PolyComp(const void *a, const void *b)
{
	float	z1, z2;

	z1 = (*(gePoly**)a)->ZOrder;
	z2 = (*(gePoly**)b)->ZOrder;

	if ( z1 == z2)
		return 0;

	if (z1 < z2)
		return -1;

	return 1;
}

//=====================================================================================
//	User_RenderPolyList
//=====================================================================================
geBoolean User_RenderPolyList(gePoly *PolyList)
{
	int32			i, NumSortedPolys;
	gePoly			*Poly;

	assert(PolyList);

	NumSortedPolys = 0;

	for (Poly = PolyList; Poly; Poly = Poly->Next)
	{
		assert(geWorld_PolyIsValid(Poly));

		if ((Poly->RenderFlags & GE_RENDER_DEPTH_SORT_BF) && NumSortedPolys < USER_MAX_SORTED_POLYS)
		{
			// Sorted polys (within this list) go in the SortedPoly list, and are sorted and drawn below
			geVec3d		Src;
			geVec3d		Dest;

			Src.X = Poly->Verts->X;
			Src.Y = Poly->Verts->Y;
			Src.Z = Poly->Verts->Z;

			geCamera_Transform(gCamera, &Src, &Dest);
			Poly->ZOrder = Dest.Z;

			SortedPolys[NumSortedPolys++] = Poly;
			continue;
		}

		// If it is not a sorted poly, render it now...
		RenderUserPoly(gCamera, Poly);
	}

	if (!NumSortedPolys)		// nothing more to do, if no sorted polys
		return GE_TRUE;

	// Now render all sorted polys
	// Sort the polys
	qsort(&SortedPolys, NumSortedPolys, sizeof(SortedPolys[0]), PolyComp);

	// Render them
	for (i=0; i< NumSortedPolys; i++)
	{
		Poly = SortedPolys[i];

		RenderUserPoly(gCamera, Poly);
	}

	return GE_TRUE;	
}

//=====================================================================================
//	User_DestroyPolyOnceList
//=====================================================================================
void User_DestroyPolyOnceList(geWorld *World, gePoly *List)
{
	gePoly	*Poly, *Next;

	// Clear out all the AddPolyOnce polys...
	for (Poly = List; Poly; Poly = Next)
	{
		Next = Poly->AddOnceNext;

		assert(geWorld_PolyIsValid(Poly));

		geWorld_RemovePoly(World, Poly);
	}
}

//=====================================================================================
//	User_DestroyPolyList
//=====================================================================================
void User_DestroyPolyList(geWorld *World, gePoly *List)
{
	gePoly	*Poly, *Next;

	// Clear out all the AddPolyOnce polys...
	for (Poly = List; Poly; Poly = Next)
	{
		Next = Poly->Next;

		assert(geWorld_PolyIsValid(Poly));

		geWorld_RemovePoly(World, Poly);
	}
}

//=====================================================================================
//	User_SetCameraInfo
//=====================================================================================
geBoolean User_SetCameraInfo(geEngine *Engine, geWorld *World, geCamera *Camera, Frustum_Info *Fi)
{
	assert(Engine != NULL);
	assert(World != NULL);
	assert(World->UserInfo != NULL);
	assert(Camera != NULL);
	assert(Fi != NULL);

	gWorld = World;
	gEngine = Engine;
	gCamera = Camera;
	gBSP = World->CurrentBSP;
	gGFXLeafs = World->CurrentBSP->BSPData.GFXLeafs;
	gGFXModels = World->CurrentBSP->BSPData.GFXModels;

	// Make the frustum go to World/Model space
	Frustum_TransformToWorldSpace(Fi, Camera, &gWorldSpaceFrustum);

	return GE_TRUE;	
}

//=====================================================================================
//	User_DestroyOncePolys
//=====================================================================================
geBoolean User_DestroyOncePolys(geWorld *World)
{
	if (World->UserInfo->AddPolyOnceList)
	{
		User_DestroyPolyOnceList(World, World->UserInfo->AddPolyOnceList);
		World->UserInfo->AddPolyOnceList = NULL;
	}

	return GE_TRUE;
}

//=====================================================================================
//	RenderTexturedPoint
//=====================================================================================
static geBoolean RenderTexturedPoint(DRV_Driver *RDriver, gePoly *Poly, Frustum_Info *FInfo, geCamera *Camera)
{
	assert(geWorld_PolyIsValid(Poly));

	if (MirrorRecursion > 0)
	{
		GE_LVertex		*pVerts, Save;
		geVec3d			Up, Left, Start;
		float			Scale, XScale, YScale;
		const geXForm3d	*MXForm;

		pVerts = Poly->Verts;

		Poly->NumVerts = 4;

		Start.X = pVerts[0].X;
		Start.Y = pVerts[0].Y;
		Start.Z = pVerts[0].Z;

		Save = pVerts[1] = pVerts[2] = pVerts[3] = pVerts[0];

		MXForm = geCamera_GetWorldSpaceXForm(Camera);

		geXForm3d_GetLeft(MXForm, &Left);
		geXForm3d_GetUp(MXForm, &Up);

		Scale = Poly->Scale * 0.5f;

		XScale = (float)geBitmap_Width(Poly->Bitmap) * Scale;
		YScale = (float)geBitmap_Height(Poly->Bitmap) * Scale;

		geVec3d_Scale(&Left, XScale, &Left);
		geVec3d_Scale(&Up, YScale, &Up);

		pVerts->X = Start.X - Left.X + Up.X;
		pVerts->Y = Start.Y - Left.Y + Up.Y;
		pVerts->Z = Start.Z - Left.Z + Up.Z;
		pVerts->u = 0.0f;
		pVerts->v = 0.0f;

		pVerts++;
	
		pVerts->X = Start.X + Left.X + Up.X;
		pVerts->Y = Start.Y + Left.Y + Up.Y;
		pVerts->Z = Start.Z + Left.Z + Up.Z;
		pVerts->u = 1.0f;
		pVerts->v = 0.0f;
	
		pVerts++;
	
		pVerts->X = Start.X + Left.X - Up.X;
		pVerts->Y = Start.Y + Left.Y - Up.Y;
		pVerts->Z = Start.Z + Left.Z - Up.Z;
		pVerts->u = 1.0f;
		pVerts->v = 1.0f;

		pVerts++;
	
		pVerts->X = Start.X - Left.X - Up.X;
		pVerts->Y = Start.Y - Left.Y - Up.Y;
		pVerts->Z = Start.Z - Left.Z - Up.Z;
		pVerts->u = 1.0f;
		pVerts->v = 0.0f;

		RenderTexturedPoly(RDriver, Poly, FInfo, Camera);

		Poly->NumVerts = 1;		// Restore the poly
		Poly->Verts[0] = Save;
	}
	else
	{
		//GFX_Plane		*Planes;
		geVec3d			Src, Dest;
		GE_LVertex		*pVerts;
		DRV_TLVertex	ScreenPnts[4];
		geBitmap		*Bitmap;
		float			Sx, Sy, z, UVAdd, Width, Height;
		float			Left, Right, Top, Bottom;
		float			Scale;
		uint32			RenderFlags;
		int32			i;
		
		assert(Poly != NULL);
		assert(Camera != NULL);

		pVerts = &Poly->Verts[0];
	
		// Xform the point 
		Src.X = pVerts->X;
		Src.Y = pVerts->Y;
		Src.Z = pVerts->Z;

		geCamera_Transform(Camera,&Src,&Dest);

		geCamera_Project(Camera, &Dest, &Src);

		for (i=0; i<4; i++)
		{
			ScreenPnts[i].x = Src.X;
			ScreenPnts[i].y = Src.Y;
			ScreenPnts[i].z = Src.Z;
			ScreenPnts[i].r = pVerts->r;
			ScreenPnts[i].g = pVerts->g;
			ScreenPnts[i].b = pVerts->b;
			ScreenPnts[i].a = pVerts->a;
		}
	
		z = -Dest.Z;

		if (z < 1)
			return GE_TRUE;

		{
			geRect Rect;
			geCamera_GetClippingRect(Camera,&Rect);

			Left   = (float)Rect.Left;
			Right  = (float)Rect.Right+1.0f;
			Top    = (float)Rect.Top;
			Bottom = (float)Rect.Bottom+1.0f;
		}

		Scale = ((geCamera_GetScale(Camera) / z) * Poly->Scale);

		Bitmap = Poly->Bitmap;
		Width = (float)geBitmap_Width(Bitmap) * Scale;
		Height = (float)geBitmap_Height(Bitmap) * Scale;

		Sx = Width * 0.5f;
		Sy = Height * 0.5f;

		// Build the screen poly from the point
		ScreenPnts[0].x -= Sx;
		ScreenPnts[0].y -= Sy;

		ScreenPnts[1].x += Sx;
		ScreenPnts[1].y -= Sy;

		ScreenPnts[2].x += Sx;
		ScreenPnts[2].y += Sy;

		ScreenPnts[3].x -= Sx;
		ScreenPnts[3].y += Sy;

		ScreenPnts[0].u = 0.0f + pVerts->u;
		ScreenPnts[0].v = 0.0f + pVerts->v;
		ScreenPnts[1].u = 1.0f + pVerts->u;
		ScreenPnts[1].v = 0.0f + pVerts->v;
		ScreenPnts[2].u = 1.0f + pVerts->u;
		ScreenPnts[2].v = 1.0f + pVerts->v;
		ScreenPnts[3].u = 0.0f + pVerts->u;
		ScreenPnts[3].v = 1.0f + pVerts->v;

		// Now, clip it against the 2d camera viewport
		if (ScreenPnts[0].x < Left)
		{
			if (ScreenPnts[1].x <= Left)
				return GE_TRUE;

			UVAdd = (Left-ScreenPnts[0].x) / Width;
			Width -= Left-ScreenPnts[0].x;
		
			ScreenPnts[0].u += UVAdd;
			ScreenPnts[3].u += UVAdd;

			ScreenPnts[0].x = Left;
			ScreenPnts[3].x = Left;
		}
		if (ScreenPnts[0].y < Top)
		{
			if (ScreenPnts[2].y <= Top)
				return GE_TRUE;

			UVAdd = (Top-ScreenPnts[0].y) / Height;
			Height -= (Top-ScreenPnts[0].y);
		
			ScreenPnts[0].v += UVAdd;
			ScreenPnts[1].v += UVAdd;

			ScreenPnts[0].y = Top;
			ScreenPnts[1].y = Top;
		}
		if (ScreenPnts[1].x >= Right)
		{
			if (ScreenPnts[0].x >= Right)
				return GE_TRUE;
	
			UVAdd = (ScreenPnts[1].x-Right) / Width;
			Width -= (ScreenPnts[1].x-Right);
		
			ScreenPnts[1].u -= UVAdd;
			ScreenPnts[2].u -= UVAdd;
		
			ScreenPnts[1].x	= Right-1;
			ScreenPnts[2].x	= Right-1;
		}
		if (ScreenPnts[2].y >= Bottom)
		{
			if (ScreenPnts[0].y >= Bottom)
				return GE_TRUE;

			UVAdd = (ScreenPnts[2].y-Bottom) / Height;
			Height -= (ScreenPnts[2].x-Bottom);
		
			ScreenPnts[2].v -= UVAdd;
			ScreenPnts[3].v -= UVAdd;

			ScreenPnts[2].y	= Bottom-1;
			ScreenPnts[3].y	= Bottom-1;
		}

		// Lastly, render it...
		ScreenPnts[0].a = pVerts->a;
		// Fixed bug where i fogot to set RGB's...
		ScreenPnts[0].r = pVerts->r;
		ScreenPnts[0].g = pVerts->g;
		ScreenPnts[0].b = pVerts->b;

		if (Poly->RenderFlags & GE_RENDER_DO_NOT_OCCLUDE_OTHERS)
			RenderFlags = DRV_RENDER_NO_ZWRITE;
		else
			RenderFlags = 0;

		if (Poly->RenderFlags & GE_RENDER_DO_NOT_OCCLUDE_SELF)
			RenderFlags |= DRV_RENDER_NO_ZMASK;

		if (pVerts->a != 255.0f)
			RenderFlags |= DRV_RENDER_ALPHA;

		if (Poly->RenderFlags & GE_RENDER_DEPTH_SORT_BF)
			RenderFlags |= DRV_RENDER_FLUSH;

		if (Poly->RenderFlags & GE_RENDER_CLAMP_UV)
			RenderFlags |= DRV_RENDER_CLAMP_UV;

		assert(geWorld_HasBitmap(gWorld, Bitmap));
		assert(geBitmap_GetTHandle(Bitmap));

		RDriver->RenderMiscTexturePoly((DRV_TLVertex*)ScreenPnts, 4, geBitmap_GetTHandle(Bitmap), RenderFlags);
	}

	return GE_TRUE;
}

//=====================================================================================
//	RenderTexturedPoly
//=====================================================================================
static void RenderTexturedPoly(DRV_Driver *RDriver, gePoly *Poly, Frustum_Info *FInfo, geCamera *Camera)
{
	geVec3d			Dest1[30], Dest2[30], *pDest1, *pDest2, *pDest3, *pTempDest;
	GE_LVertex		*pLVert;
	Surf_TexVert	Tex1[30], Tex2[30];
	Surf_TexVert	*pTex1, *pTex2, *pTempTex;
	DRV_TLVertex	Clipped1[90];
	int32			Length1, Length2;
	geBitmap		*pBitmap;
	GFX_Plane		*pFPlanes;
	int32			i, p;
	uint32			RenderFlags;

	assert(geWorld_PolyIsValid(Poly));

	pFPlanes = FInfo->Planes;
	
	pDest1 = Dest1;
	pTex1 = Tex1;
	pLVert = Poly->Verts;

	for (i=0; i< Poly->NumVerts; i++)
	{
		pDest1->X = pLVert->X;
		pDest1->Y = pLVert->Y;
		pDest1->Z = pLVert->Z;

		pTex1->u = pLVert->u;
		pTex1->v = pLVert->v;
		pTex1->r = pLVert->r;
		pTex1->g = pLVert->g;
		pTex1->b = pLVert->b;
		
		pDest1++;
		pLVert++;
		pTex1++;
	}

	pDest1 = Dest1;
	pDest2 = Dest2;
	pTex1 = Tex1;
	pTex2 = Tex2;
	Length1 = Poly->NumVerts;

	for (p=0; p< FInfo->NumPlanes; p++, pFPlanes++)
	{
		if (!Frustum_ClipToPlaneUVRGB(pFPlanes, pDest1, pDest2, pTex1, pTex2, Length1, &Length2))
			return;

		// Swap them
		pTempDest = pDest1;
		pDest1 = pDest2;
		pDest2 = pTempDest;
	
		pTempTex = pTex1;
		pTex1 = pTex2;
		pTex2 = pTempTex;

		Length1 = Length2;
	}

	if (Length1 < 3)
		return;

	pDest3 = pDest2;
	for (i=0; i< Length1; i++)
	{
		//geXForm3d_Transform(&Camera->XForm, pDest1, pDest2);
		geCamera_Transform(Camera,pDest1,pDest2);
		pDest1++;
		pDest2++;
	}

	Frustum_ProjectRGB(pDest3, pTex1, Clipped1, Length1, Camera);

	pBitmap = Poly->Bitmap;
		
	Clipped1[0].a = Poly->Verts[0].a;

	if (Poly->RenderFlags & GE_RENDER_DO_NOT_OCCLUDE_OTHERS)
		RenderFlags = DRV_RENDER_NO_ZWRITE;
	else
		RenderFlags = 0;

	if (Poly->RenderFlags & GE_RENDER_DO_NOT_OCCLUDE_SELF)
		RenderFlags |= DRV_RENDER_NO_ZMASK;
	
	if (Clipped1[0].a != 255.0f)
		RenderFlags |= DRV_RENDER_ALPHA;

	if (Poly->RenderFlags & GE_RENDER_DEPTH_SORT_BF)
		RenderFlags |= DRV_RENDER_FLUSH;

	if (Poly->RenderFlags & GE_RENDER_CLAMP_UV)
		RenderFlags |= DRV_RENDER_CLAMP_UV;

	// Render it...
	assert(geWorld_HasBitmap(gWorld, pBitmap));
	assert(geBitmap_GetTHandle(pBitmap));

	RDriver->RenderMiscTexturePoly(Clipped1, Length1, geBitmap_GetTHandle(pBitmap), RenderFlags);

}

//=====================================================================================
//	RenderGouraudPoly
//=====================================================================================
static void RenderGouraudPoly(DRV_Driver *RDriver, gePoly *Poly, Frustum_Info *FInfo, geCamera *Camera)
{
	geVec3d			Verts[30], *pVert;
	GE_LVertex		*pLVert;
	geVec3d			Dest1[30], Dest2[30], *pDest1, *pDest2, *pDest3;
	Surf_TexVert	Tex1[30], Tex2[30];
	Surf_TexVert	*pTex1, *pTex2;
	DRV_TLVertex	Clipped1[90];
	int32			Length1, Length2;
	GFX_Plane		*pFPlanes;
	int32			i, p;

	assert(geWorld_PolyIsValid(Poly));

	pFPlanes = FInfo->Planes;
	
	pVert = Verts;
	pLVert = Poly->Verts;
	pTex1 = Tex1;

	for (i=0; i< Poly->NumVerts; i++)
	{
		pVert->X = pLVert->X;
		pVert->Y = pLVert->Y;
		pVert->Z = pLVert->Z;

		pTex1->r = pLVert->r;
		pTex1->g = pLVert->g;
		pTex1->b = pLVert->b;
		
		pVert++;
		pLVert++;
		pTex1++;
	}

	pDest1 = Verts;
	pDest2 = Dest2;
	pTex1 = Tex1;
	pTex2 = Tex2;
	Length1 = Poly->NumVerts;

	for (p=0; p< 4; p++)
	{
		if (!Frustum_ClipToPlaneRGB(&pFPlanes[p], pDest1, pDest2, pTex1, pTex2, Length1, &Length2))
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

	pDest3 = pDest2;
	for (i=0; i< Length1; i++)
	{
		//geXForm3d_Transform(&Camera->XForm, pDest1, pDest2);
		geCamera_Transform(Camera,pDest1,pDest2);
		pDest1++;
		pDest2++;
	}

	Frustum_ProjectRGB(pDest3, pTex1, Clipped1, Length1, Camera);
		
	Clipped1[0].a = Poly->Verts[0].a;

	// Render it...
	if (Clipped1[0].a != 255.0f)
		RDriver->RenderGouraudPoly(Clipped1, Length1, DRV_RENDER_ALPHA);
	else
		RDriver->RenderGouraudPoly(Clipped1, Length1, 0);

}

//=====================================================================================
//	User_EngineFillRect
//=====================================================================================
void User_EngineFillRect(geEngine *Engine, const GE_Rect *Rect, const GE_RGBA *Color)
{
	DRV_TLVertex	DrvVertex[4];
	DRV_Driver *	RDriver;

	RDriver = Engine->DriverInfo.RDriver;

	assert(RDriver != NULL);
#define NEARZ	0.5f
	DrvVertex[0].x = (float)Rect->Left;
	DrvVertex[0].y = (float)Rect->Top;
	DrvVertex[0].z = NEARZ;
	DrvVertex[0].u =
	DrvVertex[0].v = 0.0f;
	DrvVertex[0].r = Color->r;
	DrvVertex[0].g = Color->g;
	DrvVertex[0].b = Color->b;
	DrvVertex[0].a = Color->a;

	DrvVertex[1].x = (float)Rect->Right;
	DrvVertex[1].y = (float)Rect->Top;
	DrvVertex[1].z = NEARZ;
	DrvVertex[1].u =
	DrvVertex[1].v = 0.0f;
	DrvVertex[1].r = Color->r;
	DrvVertex[1].g = Color->g;
	DrvVertex[1].b = Color->b;
	DrvVertex[1].a = Color->a;

	DrvVertex[2].x = (float)Rect->Right;
	DrvVertex[2].y = (float)Rect->Bottom;
	DrvVertex[2].z = NEARZ;
	DrvVertex[2].u =
	DrvVertex[2].v = 0.0f;
	DrvVertex[2].r = Color->r;
	DrvVertex[2].g = Color->g;
	DrvVertex[2].b = Color->b;
	DrvVertex[2].a = Color->a;

	DrvVertex[3].x = (float)Rect->Left;
	DrvVertex[3].y = (float)Rect->Bottom;
	DrvVertex[3].z = NEARZ;
	DrvVertex[3].u =
	DrvVertex[3].v = 0.0f;
	DrvVertex[3].r = Color->r;
	DrvVertex[3].g = Color->g;
	DrvVertex[3].b = Color->b;
	DrvVertex[3].a = Color->a;

	if (Color->a != 255.0f)
		RDriver->RenderGouraudPoly(DrvVertex, 4, DRV_RENDER_FLUSH);
	else
		RDriver->RenderGouraudPoly(DrvVertex, 4, DRV_RENDER_ALPHA | DRV_RENDER_FLUSH);
}

//=====================================================================================
//	RenderUserPoly
//=====================================================================================
static geBoolean RenderUserPoly(geCamera *Camera, gePoly *Poly)
{
	DRV_Driver	*RDriver;

	assert(Camera);
	assert(geWorld_PolyIsValid(Poly));

	RDriver = gEngine->DriverInfo.RDriver;

	gWorld->DebugInfo.NumUserPolys++;

	assert(RDriver != NULL);

	switch(Poly->Type)
	{
		case GE_TEXTURED_POLY:
			RenderTexturedPoly(RDriver, Poly, &gWorldSpaceFrustum, Camera);
			break;

		case GE_GOURAUD_POLY:
			RenderGouraudPoly(RDriver, Poly, &gWorldSpaceFrustum, Camera);
			break;

		case GE_TEXTURED_POINT:
			RenderTexturedPoint(RDriver, Poly, &gWorldSpaceFrustum, Camera);
			break;

		default:
			//geErrorLog_Add(GE_ERR_, NULL);
			return GE_FALSE;
	}
	return GE_TRUE;
}

//=====================================================================================
//	geWorld_LinkPolyToLeaf
//=====================================================================================
void geWorld_LinkPolyToLeaf(const geWorld *World, gePoly *Poly)
{
#if SEARCH_ALL_VERTS_FOR_LEAF
	int32				i;
#endif
	geWorld_Leaf		*pLeafData;
	int32				Leaf;
	GE_LVertex			*Verts;

	assert(World);
	assert(Poly);
	assert(geWorld_PolyIsValid(Poly));
	assert(Poly->LeafData == NULL);

	Verts = Poly->Verts;

#if SEARCH_ALL_VERTS_FOR_LEAF
	// Take the first vert that is in a valid leaf
	for (i=0; i<Poly->NumVerts; i++, Verts++)
#endif
	{
		geVec3d		Src;

		Src.X = Verts->X;
		Src.Y = Verts->Y;
		Src.Z = Verts->Z;

		Leaf = Plane_FindLeaf(World, gGFXModels[0].RootNode[0], &Src);

#if SEARCH_ALL_VERTS_FOR_LEAF
		if (!(gGFXLeafs[Leaf].Contents & GE_CONTENTS_SOLID))	// Try to find the first leaf NOT in solid!!!
			break;
#endif
	}

	assert(Leaf >=0 && Leaf < World->CurrentBSP->BSPData.NumGFXLeafs);
	
	pLeafData = &World->CurrentBSP->LeafData[Leaf];

	// Insert into the beginning of the list
	if (pLeafData->PolyList)
		pLeafData->PolyList->Prev = Poly;

	Poly->Next = pLeafData->PolyList;
	Poly->Prev = NULL;					// This is TRUE cause the poly is always added to the front of the list
	pLeafData->PolyList = Poly;

	Poly->LeafData = pLeafData;		// Save the leaf data
}

//=====================================================================================
//	geWorld_UnLinkPolyFromLeaf
//=====================================================================================
void geWorld_UnLinkPolyFromLeaf(gePoly *Poly)
{
	geWorld_Leaf	*pLeafData;

	assert(Poly);
	assert(geWorld_PolyIsValid(Poly));

	pLeafData = Poly->LeafData;
	assert(pLeafData);		// Polys are allways in some kind of leaf, solid or not!!! (they just don't always render)

	if (Poly->Prev)
	{
		assert(Poly->Prev->Next == Poly);
		Poly->Prev->Next = Poly->Next;
	}
	if (Poly->Next)
	{
		assert(Poly->Next->Prev == Poly);
		Poly->Next->Prev = Poly->Prev;
	}

	// Cut the poly from the leaf it was inserted into originally
	if (Poly == pLeafData->PolyList)
	{
		assert(Poly->Prev == NULL);
		pLeafData->PolyList = Poly->Next;
	}

	Poly->LeafData = NULL;
	Poly->Next = NULL;
	Poly->Prev = NULL;
}

//=====================================================================================
//		**** Public GENESISAPI's ****
//=====================================================================================

//=====================================================================================
//	geWorld_AddPoly
//=====================================================================================
GENESISAPI gePoly *geWorld_AddPoly(		geWorld *World, 
										GE_LVertex *Verts, 
										int32 NumVerts, 
										geBitmap *Bitmap,
										gePoly_Type Type, 
										uint32 RenderFlags,
										float Scale)
{
	gePoly		*Poly;

	assert(World != NULL);
	assert(World->UserInfo != NULL);

	assert(Bitmap != NULL || Type == GE_GOURAUD_POLY);
	assert(Verts != NULL);
	assert(NumVerts <= MAX_USER_VERTS);
	
	Poly = GE_RAM_ALLOCATE_STRUCT(gePoly);

	if (!Poly)
		return NULL;

	if ( World->CurrentBSP )
		gGFXModels = World->CurrentBSP->BSPData.GFXModels;
	else
		gGFXModels = NULL;

#ifdef _DEBUG
	Poly->Self1 = Poly;
	Poly->Self2 = Poly;
#endif

	Poly->NumVerts = NumVerts;
	Poly->Bitmap = Bitmap;
	Poly->Type = Type;
	Poly->RenderFlags = RenderFlags;
	Poly->Scale = Scale;
	Poly->World = World;		// I could think of no other way!!!  Poly needs world in SetLVertex.  Should we require it as a parm???
	Poly->AddOnceNext = NULL;
	Poly->Next = NULL;
	Poly->Prev = NULL;
	Poly->LeafData = NULL;

	memcpy(Poly->Verts, Verts, sizeof(GE_LVertex)*NumVerts);

	// Link the poly to the leaf it is in
	geWorld_LinkPolyToLeaf(World, Poly);

	World->ActiveUserPolys++;

	return Poly;
}

//=====================================================================================
//	geWorld_AddPolyOnce
//=====================================================================================
GENESISAPI gePoly *geWorld_AddPolyOnce(	geWorld *World, 
										GE_LVertex *Verts, 
										int32 NumVerts, 
										geBitmap *Bitmap,
										gePoly_Type Type,
										uint32 RenderFlags,
										float Scale)
{
	gePoly		*Poly;

	assert(World != NULL);
	assert(World->UserInfo != NULL);
	
	// For AddPOlyOnce, do an AddPoly, then put it in the list to be removed at the end of the frame
	Poly = geWorld_AddPoly(World, Verts, NumVerts, Bitmap, Type, RenderFlags, Scale);

	if (!Poly)
		return NULL;

	// Insert add poly once polys into the AddPolyOnceList so they can be removed at the end of the frame
	Poly->AddOnceNext = World->UserInfo->AddPolyOnceList;
	World->UserInfo->AddPolyOnceList = Poly;

	return Poly;
}

//=====================================================================================
//	geWorld_RemovePoly
//=====================================================================================
GENESISAPI void geWorld_RemovePoly(geWorld *World, gePoly *Poly)
{
	assert(World != NULL);
	assert(Poly != NULL);
	assert(geWorld_PolyIsValid(Poly));

	// Remove the poly from the leaf that it is in (it must be in one. or it would not have been added to the world)
	geWorld_UnLinkPolyFromLeaf(Poly);

	World->ActiveUserPolys--;

	geRam_Free(Poly);
}

//=====================================================================================
//	gePoly_GetLVertex
//=====================================================================================
GENESISAPI geBoolean gePoly_GetLVertex(gePoly *Poly, int32 Index, GE_LVertex *LVert)
{
	assert (Poly != NULL);
	assert(LVert != NULL);
	assert(geWorld_PolyIsValid(Poly));

	assert(Index >= 0);
	assert(Index < Poly->NumVerts);

	*LVert= Poly->Verts[Index];

	return GE_TRUE;
}

//=====================================================================================
//	gePoly_SetLVertex
//=====================================================================================
GENESISAPI geBoolean gePoly_SetLVertex(gePoly *Poly, int32 Index, const GE_LVertex *LVert)
{
	assert (Poly != NULL);
	assert(LVert != NULL);
	assert(geWorld_PolyIsValid(Poly));

	assert(Index >= 0);
	assert(Index < Poly->NumVerts);

	Poly->Verts[Index] = *LVert;

	assert(Poly->LeafData);		// A poly does not get created UNLESS it gets attaches to a leaf!!!

	geWorld_UnLinkPolyFromLeaf(Poly);			// Detach the poly from it's current leaf
	geWorld_LinkPolyToLeaf(Poly->World, Poly);	// Re-attach to the new leaf (if any, check for cohearency!)
	
	return GE_TRUE;
}
