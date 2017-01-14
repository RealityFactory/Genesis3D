/****************************************************************************************/
/*  Surface.c                                                                           */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Creates the surfaces for a BSP from the GFX data                       */
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
#include <Math.h>


#include "BaseType.h"
#include "System.h"
#include "World.h"
#include "Ram.h"
#include "Surface.h"
#include "WBitmap.h"	
#include "Vec3d.h"
#include "Vis.h"

#include "Light.h"

//================================================================================
//	local static globals
//================================================================================
static	geEngine		*CEngine;
static	geWorld			*CWorld;
static	GBSP_BSPData	*BSPData;		// This is in globals, but is also kept here for speed


//================================================================================
//	local static functions
//================================================================================
static geBoolean GetTexVerts(World_BSP *BSP);
static geBoolean GetSurfInfo(World_BSP *BSP);
static geBoolean GetRGBVerts(World_BSP *BSP);


//=====================================================================================
//	Surf_EngineInit
//=====================================================================================
geBoolean Surf_EngineInit(geEngine *Engine)
{
	return GE_TRUE;
}

//=====================================================================================
//	Surf_EngineShutdown
//=====================================================================================
void Surf_EngineShutdown(geEngine *Engine)
{
	CEngine = NULL;
	CWorld = NULL;
	BSPData = NULL;
}

//=====================================================================================
//	Surf_SetEngine
//	Lets this module know that the engine has changed
//=====================================================================================
geBoolean Surf_SetEngine(geEngine *Engine)
{
	assert (Engine != NULL);

	CEngine = Engine;

	return GE_TRUE;
}

//=====================================================================================
//	Surf_SetWorld
//	Lets this module know that the world has changed
//=====================================================================================
geBoolean Surf_SetWorld(geWorld *World)
{
	assert(World != NULL);
	
	CWorld = World;

	return GE_TRUE;
}

//=====================================================================================
//	Surf_SetGBSP
//=====================================================================================
geBoolean Surf_SetGBSP(World_BSP *BSP)
{
	assert(BSP != NULL);

	// Make quick pointer to the world bsp data
	BSPData = &BSP->BSPData;

	return GE_TRUE;
}

void CalcSurfVectors (World_BSP *BSP);

//================================================================================
//	Surf_WorldInit
//================================================================================
geBoolean Surf_WorldInit(geWorld *World)
{
	World_BSP	*BSP;

	assert(World != NULL);
	
	BSP = World->CurrentBSP;

	assert(BSP != NULL);

	// Make sure we free the old ones...
	if (BSP->TexVerts)
		geRam_Free(BSP->TexVerts);
	
	if (BSP->SurfInfo)
		geRam_Free(BSP->SurfInfo);

	// Create new TexVerts and FaceInfo structure for this bsp
	BSP->TexVerts = GE_RAM_ALLOCATE_ARRAY(Surf_TexVert, BSP->BSPData.NumGFXVertIndexList);
	BSP->SurfInfo = GE_RAM_ALLOCATE_ARRAY(Surf_SurfInfo, BSP->BSPData.NumGFXFaces);
	
	// Fill in info needed to render this tree
	if (!GetTexVerts(BSP))			// Calc texture uv's at vertices...
		return GE_FALSE;

	if (!GetSurfInfo(BSP))			// Get surface info
		return GE_FALSE;

	Light_SetWorld(World);
	Light_SetGBSP(BSP);

	if (!GetRGBVerts(BSP))			// Calc RGB values at vertices
		return GE_FALSE;

	CalcSurfVectors (BSP);

	return GE_TRUE;
}

//================================================================================
//	Surf_WorldShutdown
//================================================================================
void Surf_WorldShutdown(geWorld *World)
{
	World_BSP *BSP;

	assert(World != NULL);

	BSP = World->CurrentBSP;

	if (!BSP)
		return;

	if (BSP->TexVerts)
		geRam_Free(BSP->TexVerts);
	if (BSP->SurfInfo)
		geRam_Free(BSP->SurfInfo);

	BSP->TexVerts = NULL;
	BSP->SurfInfo = NULL;
}


//================================================================================
//	Surf_InSurfBoundingBox
//================================================================================
BOOL Surf_InSurfBoundingBox(Surf_SurfInfo *Surf, geVec3d *Pos, float Box)
{
   assert(Surf != NULL);
   assert(Pos != NULL);

   if (Pos->X+Box >= Surf->VMins.X && Pos->X-Box <= Surf->VMaxs.X)
   if (Pos->Y+Box >= Surf->VMins.Y && Pos->Y-Box <= Surf->VMaxs.Y)
   if (Pos->Z+Box >= Surf->VMins.Z && Pos->Z-Box <= Surf->VMaxs.Z)
        return TRUE;

   return FALSE;
}

//================================================================================
//	GetTexVerts
//================================================================================
static geBoolean GetTexVerts(World_BSP *BSP)
{
	int32			i, v, vn;
	int32			*pIndex, TexInfo;
	geVec3d			*pVert, *pVecU, *pVecV;
	float			U, V;
	Surf_TexVert	*TexVerts;
	GFX_TexInfo		*pTexInfo;
	GFX_Face		*pFace;

	TexVerts = BSP->TexVerts;
	
	for (i=0; i< BSP->BSPData.NumGFXFaces; i++)
	{
		pFace = &BSP->BSPData.GFXFaces[i];
		TexInfo = BSP->BSPData.GFXFaces[i].TexInfo;
		pTexInfo = &BSP->BSPData.GFXTexInfo[TexInfo];
		
		pVecU = &pTexInfo->Vecs[0];
		pVecV = &pTexInfo->Vecs[1];

		pIndex = &BSP->BSPData.GFXVertIndexList[pFace->FirstVert];
		vn = pFace->FirstVert;

		for (v= 0; v< pFace->NumVerts; v++, pIndex++, vn++)
		{
			pVert = &BSP->BSPData.GFXVerts[*pIndex];

			U = geVec3d_DotProduct(pVert, pVecU);
			V = geVec3d_DotProduct(pVert, pVecV);

			TexVerts[vn].u = U;
			TexVerts[vn].v = V;
		}
	}

	return GE_TRUE;
}


//================================================================================
//	GetSurfInfo
//================================================================================
static geBoolean GetSurfInfo(World_BSP *BSP)
{
	int32			NumLTypes;
	int32			i, k, v;
	int32			vn, Index;
	float			U, V;
	float			Mins[2], Maxs[2];
	geVec3d			VMins, VMaxs;
	int32			Size[2];
	Surf_SurfInfo	*SurfInfo;
	Surf_TexVert	*TexVerts;
	GFX_Texture		*pTexture;
	GFX_TexInfo		*pTexInfo;

	SurfInfo = BSP->SurfInfo;
	TexVerts = BSP->TexVerts;

	assert(SurfInfo != NULL);
	assert(TexVerts != NULL);

	memset(SurfInfo, 0, sizeof(Surf_SurfInfo)*(BSP->BSPData.NumGFXFaces));

	for (i=0; i< BSP->BSPData.NumGFXFaces; i++)
	{
		float	XScale, YScale;

		// Find number of styles
		NumLTypes = 0;
		for (NumLTypes = 0; NumLTypes < 4; NumLTypes++) 
		{
			if (BSP->BSPData.GFXFaces[i].LTypes[NumLTypes]==255) 
				break;

			if (BSP->BSPData.GFXFaces[i].LTypes[NumLTypes] != 0) 
				SurfInfo[i].Flags |= SURFINFO_LTYPED;
		}

		SurfInfo[i].NumLTypes = NumLTypes;

		SurfInfo[i].TexInfo = BSP->BSPData.GFXFaces[i].TexInfo;

		pTexInfo = &BSP->BSPData.GFXTexInfo[BSP->BSPData.GFXFaces[i].TexInfo];
		pTexture = &BSP->BSPData.GFXTextures[BSP->BSPData.GFXTexInfo[SurfInfo[i].TexInfo].Texture];
		
		if (pTexInfo->Flags & TEXINFO_TRANS)
			SurfInfo[i].Flags |= SURFINFO_TRANS;

		// Set up lightmap scaling values for dlights
		k = BSP->BSPData.GFXFaces[i].TexInfo;

		XScale = geVec3d_Length(&BSP->BSPData.GFXTexInfo[k].Vecs[0]);
		YScale = geVec3d_Length(&BSP->BSPData.GFXTexInfo[k].Vecs[1]);

		SurfInfo[i].XStep = (int32)((16.0f / XScale) * (1<<10));
		SurfInfo[i].YStep = (int32)((16.0f / YScale) * (1<<10));
		SurfInfo[i].XScale = (int32)((1.0f/XScale) * (1<<10));
		SurfInfo[i].YScale = (int32)((1.0f/YScale) * (1<<10));
	}

	//
	//	Find face/texvert min/max
	//
	for (i=0; i< BSP->BSPData.NumGFXFaces; i++)
	{
		pTexInfo = &BSP->BSPData.GFXTexInfo[BSP->BSPData.GFXFaces[i].TexInfo];
		pTexture = &BSP->BSPData.GFXTextures[BSP->BSPData.GFXTexInfo[SurfInfo[i].TexInfo].Texture];

		for (k=0; k<2; k++)
		{
			Mins[k] = 99999.0f;
			Maxs[k] =-99999.0f;
		}
		for (k=0; k<3; k++)
		{
			VectorToSUB(VMins, k) = 99999.0f;
			VectorToSUB(VMaxs, k) =-99999.0f;
		}

		for (v= 0; v< BSP->BSPData.GFXFaces[i].NumVerts; v++)
		{
			vn =  v + BSP->BSPData.GFXFaces[i].FirstVert;
			U = TexVerts[vn].u;
			V = TexVerts[vn].v;

			if (U < Mins[0])
				Mins[0] = U;
			if (U > Maxs[0])
				Maxs[0] = U;
			if (V < Mins[1])
				Mins[1] = V;
			if (V > Maxs[1])
				Maxs[1] = V;

			Index = BSP->BSPData.GFXVertIndexList[vn];

			for (k=0; k<3; k++)
			{
				if (VectorToSUB(BSP->BSPData.GFXVerts[Index], k) < VectorToSUB(VMins, k))
					VectorToSUB(VMins, k) = VectorToSUB(BSP->BSPData.GFXVerts[Index], k);
				if (VectorToSUB(BSP->BSPData.GFXVerts[Index], k) > VectorToSUB(VMaxs, k))
					VectorToSUB(VMaxs, k) = VectorToSUB(BSP->BSPData.GFXVerts[Index], k);
			}
		}

		// Calculate Shift values
		#if 1
		{
			int32			Width, Height;
			float			au, av, ScaleU, ScaleV;

			#if 0
				pTexInfo->DrawScale[0] = 1.0f;		// For testing
				pTexInfo->DrawScale[1] = 1.0f;
			#endif

			ScaleU = 1.0f/pTexInfo->DrawScale[0];
			ScaleV = 1.0f/pTexInfo->DrawScale[1];
			
			Width = pTexture->Width;
			Height = pTexture->Height;

			// Interpret the uv's the same way the drivers will
			au = (float)(((int32)((Mins[0]*ScaleU+pTexInfo->Shift[0])/Width ))*Width);
			av = (float)(((int32)((Mins[1]*ScaleV+pTexInfo->Shift[1])/Height))*Height);
			//au = (float)(((int32)((Mins[0]*ScaleU)/Width ))*Width);
			//av = (float)(((int32)((Mins[1]*ScaleV)/Height))*Height);

			SurfInfo[i].ShiftU = pTexInfo->Shift[0] - au;
			SurfInfo[i].ShiftV = pTexInfo->Shift[1] - av;
		}
		#else
		{
			SurfInfo[i].ShiftU = pTexInfo->Shift[0];
			SurfInfo[i].ShiftV = pTexInfo->Shift[1];
		}
		#endif

		SurfInfo[i].VMins = VMins;
		SurfInfo[i].VMaxs = VMaxs;

		if (BSP->BSPData.GFXTexInfo[BSP->BSPData.GFXFaces[i].TexInfo].Flags & TEXINFO_NO_LIGHTMAP)
			continue;
		
		for (k=0; k< 2; k++)
		{
			Mins[k] = (float)floor(Mins[k]/16);
			Maxs[k] = (float)ceil(Maxs[k]/16);
			
			Size[k] = (S32)(Maxs[k] - Mins[k]) + 1;
			
			if (Size[k] > MAX_LMAP_SIZE)
			{
				geErrorLog_Add(GE_ERR_BAD_LMAP_EXTENTS, NULL);
				return GE_FALSE;
			}
		}

		//if (Size[0] != BSP->BSPData.GFXFaces[i].LWidth)
		//	return FALSE;

		//if (Size[1] != BSP->BSPData.GFXFaces[i].LHeight)
		//	return FALSE;

		Size[0] = BSP->BSPData.GFXFaces[i].LWidth;
		Size[1] = BSP->BSPData.GFXFaces[i].LHeight;

		SurfInfo[i].LInfo.Width = (int16)Size[0];
		SurfInfo[i].LInfo.Height = (int16)Size[1];

		SurfInfo[i].LInfo.MinU = (S32)(Mins[0] * 16);
		SurfInfo[i].LInfo.MinV = (S32)(Mins[1] * 16);
		SurfInfo[i].LInfo.Face = i;

		#if 0
		{
			int32		LightOffset, p;
			uint8		*LightData;

			LightOffset = BSP->BSPData.GFXFaces[i].LightOfs;

			if (LightOffset >= 0)
			{
				LightData = &BSP->BSPData.GFXLightData[LightOffset+1];

				for (p=0; p< SurfInfo[i].LInfo.lWidth*SurfInfo[i].LInfo.lHeight; p++)
				{
					LightData++;
					if (*LightData)
						return GE_FALSE;
					//*LightData = 0;
					LightData++;
					LightData++;
				}
			}
		}
		#endif
		
		#if 0
		//
		//	Make sure the lightmap u,v's are legit...
		//
		for (v= 0; v< BSP->BSPData.GFXFaces[i].NumVerts; v++)
		{
			vn =  v + BSP->BSPData.GFXFaces[i].FirstVert;

			U = TexVerts[vn].u - SurfInfo[i].LInfo.MinU;
			V = TexVerts[vn].v - SurfInfo[i].LInfo.MinV;
			
			if (U < 0 || V > 16*128)
			{
				SurfInfo[i].LInfo.Face = -1;
				//geErrorLog_Add(GE_ERR_BAD_LMAP_EXTENTS, NULL);
				//return GE_FALSE;
			}	
			if (V < 0 || V > 16*128)
			{
				SurfInfo[i].LInfo.Face = -1;
				//geErrorLog_Add(GE_ERR_BAD_LMAP_EXTENTS, NULL);
				//return GE_FALSE;
			}
			
		}
		#endif

		SurfInfo[i].Flags |= SURFINFO_LIGHTMAP;
		//CalcGFXFaceVectors(i);
	}

	if (!Vis_MarkWaterFaces(BSP))
		return GE_FALSE;
	
	return GE_TRUE;
}

//================================================================================
//	GetRGBVerts
//================================================================================
static geBoolean GetRGBVerts(World_BSP *BSP)
{
	Surf_TexVert	*pTexVerts;
	GFX_Face		*pGFXFace;
	int32			i;
	geVec3d			*pGFXRGBVerts;

	pTexVerts = BSP->TexVerts;
	pGFXFace = BSP->BSPData.GFXFaces;
	pGFXRGBVerts = BSP->BSPData.GFXRGBVerts;

	for (i=0; i< BSP->BSPData.NumGFXFaces; i++, pGFXFace++)
	{
		GFX_TexInfo		*pTexInfo;
		int32			v;

		pTexInfo = &BSP->BSPData.GFXTexInfo[pGFXFace->TexInfo];

		if (!(pTexInfo->Flags & TEXINFO_GOURAUD) && !(pTexInfo->Flags & TEXINFO_FLAT))
		{
			for (v= 0; v< pGFXFace->NumVerts; v++)
			{
				int32		vn;
			
				vn = pGFXFace->FirstVert + v;

				pTexVerts[vn].r = 255.0f;
				pTexVerts[vn].g = 255.0f;
				pTexVerts[vn].b = 255.0f;
			}
			continue;
		}

		for (v= 0; v< pGFXFace->NumVerts; v++)
		{
			int32		vn;

			vn = pGFXFace->FirstVert + v;

			pTexVerts[vn].r = pGFXRGBVerts[vn].X;
			pTexVerts[vn].g = pGFXRGBVerts[vn].Y;
			pTexVerts[vn].b = pGFXRGBVerts[vn].Z;
		}
	}
	
	return GE_TRUE;
}

//***************************************************************************************
//	Get Texture to WorldSpace vectors
//***************************************************************************************
void CalcSurfVectors (World_BSP *BSP)
{
	Surf_SurfInfo	*pSurfInfo, *Si;
	GFX_TexInfo		*Tex;
	geVec3d			TexNormal;
	geVec3d			FaceNormal;
	float			DistScale, PlaneDist;
	float			Dist, Len;
	geVec3d			Ws[3];
	int32			i, k;
	GBSP_BSPData	*BSPData;
	GFX_Plane		*GFXPlanes;
	GFX_Face		*pFace;
	int32			Startx, Starty;
	float			UU, VV;

	BSPData = &BSP->BSPData;
	
	pSurfInfo = BSP->SurfInfo;

	GFXPlanes = BSPData->GFXPlanes;

	for (i=0; i< BSP->BSPData.NumGFXFaces; i++)
	{
		Si = &pSurfInfo[i];

		pFace = &BSPData->GFXFaces[i];

		Tex = &BSPData->GFXTexInfo[pFace->TexInfo];
	
		geVec3d_CrossProduct(&Tex->Vecs[0], &Tex->Vecs[1], &TexNormal);
		geVec3d_Normalize(&TexNormal);

		// flip it towards plane normal
		FaceNormal = GFXPlanes[pFace->PlaneNum].Normal;
		PlaneDist = GFXPlanes[pFace->PlaneNum].Dist;

		if (pFace->PlaneSide)
		{
			geVec3d_Inverse(&FaceNormal);
			PlaneDist = -PlaneDist;
		}
	
		DistScale = geVec3d_DotProduct(&TexNormal, &FaceNormal);
	
		if (DistScale < 0)
		{
			geVec3d_Inverse(&TexNormal);
			DistScale = -DistScale;
		}	

		// distscale is the ratio of the distance along the texture normal to
		// the distance along the plane normal
		DistScale = 1/DistScale;

		// Get the tex to world vectors
		for (k=0 ; k<2 ; k++)
		{
			Len = geVec3d_Length(&Tex->Vecs[k]);
			Dist = geVec3d_DotProduct(&Tex->Vecs[k], &FaceNormal);
			Dist *= DistScale;
			geVec3d_MA(&Tex->Vecs[k], -Dist, &TexNormal, &Si->T2WVecs[k]);
			geVec3d_Scale(&Si->T2WVecs[k], (1/Len)*(1/Len), &Si->T2WVecs[k]);
		}


		for (k=0 ; k<3 ; k++)
			VectorToSUB(Si->TexOrg,k) = 
				- Tex->Vecs[0].Z * VectorToSUB(Si->T2WVecs[0], k) 
			    - Tex->Vecs[1].Z * VectorToSUB(Si->T2WVecs[1], k);

		Dist = geVec3d_DotProduct(&Si->TexOrg, &FaceNormal) - PlaneDist - 1;
		Dist *= DistScale;
		geVec3d_MA (&Si->TexOrg, -Dist, &TexNormal, &Si->TexOrg);

		Startx = Si->LInfo.MinU;
		Starty = Si->LInfo.MinV;
	
		UU = (float)Startx;
		VV = (float)Starty;
	
		Ws[0].X = Si->TexOrg.X + Si->T2WVecs[0].X*UU + Si->T2WVecs[1].X*VV;
		Ws[0].Y = Si->TexOrg.Y + Si->T2WVecs[0].Y*UU + Si->T2WVecs[1].Y*VV;
		Ws[0].Z = Si->TexOrg.Z + Si->T2WVecs[0].Z*UU + Si->T2WVecs[1].Z*VV;
		UU = (float)Startx+16.0f;
		VV = (float)Starty;
		Ws[1].X = Si->TexOrg.X + Si->T2WVecs[0].X*UU + Si->T2WVecs[1].X*VV;
		Ws[1].Y = Si->TexOrg.Y + Si->T2WVecs[0].Y*UU + Si->T2WVecs[1].Y*VV;
		Ws[1].Z = Si->TexOrg.Z + Si->T2WVecs[0].Z*UU + Si->T2WVecs[1].Z*VV;
		UU = (float)Startx;
		VV = (float)Starty+16.0f;
		Ws[2].X = Si->TexOrg.X + Si->T2WVecs[0].X*UU + Si->T2WVecs[1].X*VV;
		Ws[2].Y = Si->TexOrg.Y + Si->T2WVecs[0].Y*UU + Si->T2WVecs[1].Y*VV;
		Ws[2].Z = Si->TexOrg.Z + Si->T2WVecs[0].Z*UU + Si->T2WVecs[1].Z*VV;

		geVec3d_Subtract(&Ws[1], &Ws[0], &Si->T2WVecs[0]);
		geVec3d_Subtract(&Ws[2], &Ws[0], &Si->T2WVecs[1]);
		Si->TexOrg = Ws[0];
	}
   
}
