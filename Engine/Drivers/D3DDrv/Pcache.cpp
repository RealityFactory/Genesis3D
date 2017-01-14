/****************************************************************************************/
/*  PCache.cpp                                                                          */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: D3D poly cache                                                         */
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
#include <Windows.h>
#include <stdio.h>

#include "D3DCache.h"
#include "D3D_Fx.h"

#include "PCache.h"
#include "D3DDrv.h"
#include "THandle.h"
#include "D3D_Err.h"


//#define D3D_MANAGE_TEXTURES
#define SUPER_FLUSH

//====================================================================================
//	Local static variables
//====================================================================================

DRV_CacheInfo						CacheInfo;

//
//	World Cache
//

#if 1

#define MAX_WORLD_POLYS				256
#define MAX_WORLD_POLY_VERTS		1024

#define MAX_MISC_POLYS				256
#define MAX_MISC_POLY_VERTS			1024

#else

#define MAX_WORLD_POLYS				256
#define MAX_WORLD_POLY_VERTS		4096

#define MAX_MISC_POLYS				256
#define MAX_MISC_POLY_VERTS			4096

#endif

typedef struct
{
	float		u;
	float		v;
	//float		a;
	uint32		Color;
} PCache_TVert;

typedef struct
{
	geRDriver_THandle	*THandle;

	DRV_LInfo	*LInfo;						// Original pointer to linfo
	uint32		Flags;						// Flags for this poly
	float		ShiftU;
	float		ShiftV;
	float		ScaleU;
	float		ScaleV;
	int32		MipLevel;
	uint32		SortKey;
	int32		FirstVert;
	int32		NumVerts;
} World_Poly;

#define MAX_TEXTURE_STAGES		2			// Up to 2 tmu's (stages)

// Verts we defined in the D3D flexible vertex format (FVF)
// This is a transformed and lit vertex definition, with up to 8 sets of uvs
typedef struct
{
	float			u,v;
} PCache_UVSet;

typedef struct
{
	float			x,y,z;					// Screen x, y, z
	float			rhw;					// homogenous w
	DWORD			color;					// color
	DWORD			specular;
	PCache_UVSet	uv[MAX_TEXTURE_STAGES];	// uv sets for each stage
} PCache_Vert;

typedef struct
{
	World_Poly		Polys[MAX_WORLD_POLYS];
	World_Poly		*SortedPolys[MAX_WORLD_POLYS];
	World_Poly		*SortedPolys2[MAX_WORLD_POLYS];
	PCache_Vert		Verts[MAX_WORLD_POLY_VERTS];

	PCache_TVert	TVerts[MAX_WORLD_POLY_VERTS];		// Original uv

	int32			NumPolys;
	int32			NumPolys2;
	int32			NumVerts;
} World_Cache;

static World_Cache		WorldCache;

#define PREP_WORLD_VERTS_NORMAL			1				// Prep verts as normal
#define PREP_WORLD_VERTS_LMAP			2				// Prep verts as lightmaps
#define PREP_WORLD_VERTS_SINGLE_PASS	3				// Prep verts for a single pass

#define RENDER_WORLD_POLYS_NORMAL		1				// Render polys as normal
#define RENDER_WORLD_POLYS_LMAP			2				// Render polys as lightmaps
#define RENDER_WORLD_POLYS_SINGLE_PASS	3

//
//	Misc cache
//

typedef struct
{
	geRDriver_THandle	*THandle;
	uint32		Flags;						// Flags for this poly
	int32		MipLevel;
	int32		FirstVert;
	int32		NumVerts;

	uint32		SortKey;
} Misc_Poly;

typedef struct
{
	Misc_Poly		Polys[MAX_MISC_POLYS];
	Misc_Poly		*SortedPolys[MAX_MISC_POLYS];
	PCache_Vert		Verts[MAX_MISC_POLY_VERTS];
	//float			ZVert[MAX_MISC_POLY_VERTS];

	int32			NumPolys;
	int32			NumVerts;
} Misc_Cache;

static Misc_Cache		MiscCache;

//====================================================================================
//	Local static functions prototypes
//====================================================================================
geBoolean World_PolyPrepVerts(World_Poly *pPoly, int32 PrepMode, int32 Stage1, int32 Stage2);

static BOOL RenderWorldPolys(int32 RenderMode);
static BOOL ClearWorldCache(void);
static int32 GetMipLevel(DRV_TLVertex *Verts, int32 NumVerts, float ScaleU, float ScaleV, int32 MaxMipLevel);

#include <Math.h>

//====================================================================================
//	PCache_InsertWorldPoly
//====================================================================================
BOOL PCache_InsertWorldPoly(DRV_TLVertex *Verts, int32 NumVerts, geRDriver_THandle *THandle, DRV_TexInfo *TexInfo, DRV_LInfo *LInfo, uint32 Flags)
{
	int32			Mip;
	float			ZRecip, DrawScaleU, DrawScaleV;
	World_Poly		*pCachePoly;
	DRV_TLVertex	*pVerts;
	PCache_TVert	*pTVerts;
	PCache_Vert		*pD3DVerts;
	int32			i;
	uint32			Alpha;

	#ifdef _DEBUG
		if (LInfo)
		{
			assert(LInfo->THandle);
		}
	#endif

	if ((WorldCache.NumVerts + NumVerts) >= MAX_WORLD_POLY_VERTS)
	{
		// If the cache is full, we must flush it before going on...
		if (!PCache_FlushWorldPolys())
			return GE_FALSE;
	}
	else if (WorldCache.NumPolys+1 >= MAX_WORLD_POLYS)
	{
		// If the cache is full, we must flush it before going on...
		if (!PCache_FlushWorldPolys())
			return GE_FALSE;
	}

	DrawScaleU = 1.0f / TexInfo->DrawScaleU;
	DrawScaleV = 1.0f / TexInfo->DrawScaleV;

	Mip = GetMipLevel(Verts, NumVerts, DrawScaleU, DrawScaleV, THandle->NumMipLevels-1);

	// Get a pointer to the original polys verts
	pVerts = Verts;
	
	// Store info about this poly in the cache
	pCachePoly = &WorldCache.Polys[WorldCache.NumPolys];

	pCachePoly->THandle = THandle;
	pCachePoly->LInfo = LInfo;
	pCachePoly->Flags = Flags;
	pCachePoly->FirstVert = WorldCache.NumVerts;
	pCachePoly->NumVerts = NumVerts;
	pCachePoly->ShiftU = TexInfo->ShiftU;
	pCachePoly->ShiftV = TexInfo->ShiftV;
	pCachePoly->ScaleU = DrawScaleU;
	pCachePoly->ScaleV = DrawScaleV;
	pCachePoly->MipLevel = Mip;

	// Don't forget the sort key:
	pCachePoly->SortKey = ((THandle - TextureHandles)<<4)+Mip;

	// Get a pointer into the world verts
	pD3DVerts = &WorldCache.Verts[WorldCache.NumVerts];
	pTVerts = &WorldCache.TVerts[WorldCache.NumVerts];

	if (Flags & DRV_RENDER_ALPHA)
		Alpha = (uint32)pVerts->a<<24;
	else
		Alpha = (uint32)(255<<24);

	for (i=0; i< NumVerts; i++)
	{
		ZRecip = 1.0f/(pVerts->z);

		pD3DVerts->x = pVerts->x;
		pD3DVerts->y = pVerts->y;

		pD3DVerts->z = (1.0f - ZRecip);	// ZBUFFER
		pD3DVerts->rhw = ZRecip;

		if (AppInfo.FogEnable)
		{
			DWORD	FogVal;
			float	Val;

			Val = pVerts->z;

			if (Val > AppInfo.FogEnd)
				Val = AppInfo.FogEnd;

			FogVal = (DWORD)((AppInfo.FogEnd-Val)/(AppInfo.FogEnd-AppInfo.FogStart)*255.0f);
		
			if (FogVal < 0)
				FogVal = 0;
			else if (FogVal > 255)
				FogVal = 255;
		
			pD3DVerts->specular = (FogVal<<24);		// Alpha component in specular is the fog value (0...255)
		}
		else
			pD3DVerts->specular = 0;
		
		// Store the uv's so the prep pass can use them...
		pTVerts->u = pVerts->u;
		pTVerts->v = pVerts->v;

		pTVerts->Color = Alpha | ((uint32)pVerts->r<<16) | ((uint32)pVerts->g<<8) | (uint32)pVerts->b;

		pTVerts++;
		pVerts++;	 
		pD3DVerts++;

	}
	
	// Update globals about the world poly cache
	WorldCache.NumVerts += NumVerts;
	WorldCache.NumPolys++;

	return TRUE;
}

//====================================================================================
//	PCache_FlushWorldPolys
//====================================================================================
BOOL PCache_FlushWorldPolys(void)
{
	if (!WorldCache.NumPolys)
		return TRUE;

	if (!THandle_CheckCache())
		return GE_FALSE;
	
	if (AppInfo.CanDoMultiTexture)
	{
		RenderWorldPolys(RENDER_WORLD_POLYS_SINGLE_PASS);
	}
	else
	{
		// Render them as normal
		if (!RenderWorldPolys(RENDER_WORLD_POLYS_NORMAL))
			return GE_FALSE;

		// Render them as lmaps
		RenderWorldPolys(RENDER_WORLD_POLYS_LMAP);
	}

	ClearWorldCache();

	return TRUE;
}

//====================================================================================
//====================================================================================
static int MiscBitmapHandleComp(const void *a, const void *b)
{
	uint32	Id1, Id2;

	Id1 = (uint32)(*(Misc_Poly**)a)->SortKey;
	Id2 = (uint32)(*(Misc_Poly**)b)->SortKey;

	if ( Id1 == Id2)
		return 0;

	if (Id1 < Id2)
		return -1;

	return 1;
}

//====================================================================================
//====================================================================================
static void SortMiscPolysByHandle(void)
{
	Misc_Poly	*pPoly;
	int32		i;

	pPoly = MiscCache.Polys;

	for (i=0; i<MiscCache.NumPolys; i++)
	{
		MiscCache.SortedPolys[i] = pPoly;
		pPoly++;
	}
	
	// Sort the polys
	qsort(&MiscCache.SortedPolys, MiscCache.NumPolys, sizeof(MiscCache.SortedPolys[0]), MiscBitmapHandleComp);
}

//=====================================================================================
//	FillLMapSurface
//=====================================================================================
static void FillLMapSurface(DRV_LInfo *LInfo, int32 LNum)
{
	U16					*pTempBits;
	int32				w, h, Width, Height, Size;
	U8					*pBitPtr;
	RGB_LUT				*Lut;
	geRDriver_THandle	*THandle;
	int32				Extra;

	THandle = LInfo->THandle;

	pBitPtr = (U8*)LInfo->RGBLight[LNum];

	Width = LInfo->Width;
	Height = LInfo->Height;
	Size = 1<<THandle->Log;

	Lut = &AppInfo.Lut1;

	THandle_Lock(THandle, 0, (void**)&pTempBits);

	Extra = Size - Width;

	for (h=0; h< Height; h++)
	{
		for (w=0; w< Width; w++)
		{
			U8	R, G, B;
			U16	Color;
			R = *pBitPtr++;
			G = *pBitPtr++;
			B =  *pBitPtr++;
			
			Color = (U16)(Lut->R[R] | Lut->G[G] | Lut->B[B]);

			*pTempBits++ = Color;
		}
		pTempBits += Extra;
	}

	THandle_UnLock(THandle, 0);
}

#ifdef USE_TPAGES
//=====================================================================================
//	FillLMapSurface
//=====================================================================================
static void FillLMapSurface2(DRV_LInfo *LInfo, int32 LNum)
{
	U16					*pTempBits;
	int32				w, h, Width, Height, Stride;
	U8					*pBitPtr;
	RGB_LUT				*Lut;
	geRDriver_THandle	*THandle;
	HRESULT				Result;
	const RECT			*pRect;
    DDSURFACEDESC2		SurfDesc;
	LPDIRECTDRAWSURFACE4	Surface;
	int32				Extra;

	THandle = LInfo->THandle;

	pBitPtr = (U8*)LInfo->RGBLight[LNum];

	Width = LInfo->Width;
	Height = LInfo->Height;

	Lut = &AppInfo.Lut1;

    pRect = TPage_BlockGetRect(THandle->Block);
	Surface = TPage_BlockGetSurface(THandle->Block);

    memset(&SurfDesc, 0, sizeof(DDSURFACEDESC2));
    SurfDesc.dwSize = sizeof(DDSURFACEDESC2);

	Result = Surface->Lock((RECT*)pRect, &SurfDesc, DDLOCK_WAIT, NULL);

	assert(Result == DD_OK);

	Stride = SurfDesc.dwWidth;

	pTempBits = (U16*)SurfDesc.lpSurface;

	Extra = Stride - Width; 

	for (h=0; h< Height; h++)
	{
		for (w=0; w< Width; w++)
		{
			U8	R, G, B;
			U16	Color;
			R = *pBitPtr++;
			G = *pBitPtr++;
			B = *pBitPtr++;
			
			Color = (U16)(Lut->R[R] | Lut->G[G] | Lut->B[B]);

			*pTempBits++ = Color;
		}
		pTempBits += Extra;
	}

    Result = Surface->Unlock((RECT*)pRect);

	assert(Result == DD_OK);
}
#endif

//=====================================================================================
//	LoadLMapFromSystem
//=====================================================================================
static void LoadLMapFromSystem(DRV_LInfo *LInfo, int32 Log, int32 LNum)
{
	U16					*pTempBits;
	int32				w, h, Width, Height, Size, Extra;
	U8					*pBitPtr;
	LPDIRECTDRAWSURFACE4 Surface;
	RGB_LUT				*Lut;
    DDSURFACEDESC2		ddsd;
    HRESULT				ddrval;

	pBitPtr = (U8*)LInfo->RGBLight[LNum];

	Width = LInfo->Width;
	Height = LInfo->Height;
	Size = 1<<Log;

	Extra = Size - Width;

	Lut = &AppInfo.Lut1;

	Surface = SystemToVideo[Log].Surface;

    memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
    ddsd.dwSize = sizeof(DDSURFACEDESC2);
    ddrval = Surface->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);

	assert(ddrval == DD_OK);

	pTempBits = (USHORT*)ddsd.lpSurface;

	for (h=0; h< Height; h++)
	{
		for (w=0; w< Width; w++)
		{
			U8	R, G, B;
			U16	Color;
			R = *pBitPtr++;
			G = *pBitPtr++;
			B =  *pBitPtr++;
			
			Color = (U16)(Lut->R[R] | Lut->G[G] | Lut->B[B]);
			
			*pTempBits++ = Color;
		}
		pTempBits += Extra;
	}

    ddrval = Surface->Unlock(NULL);
	assert(ddrval == DD_OK);
}

static BOOL IsKeyDown(int KeyCode)
{
	if (GetAsyncKeyState(KeyCode) & 0x8000)
		return TRUE;

	return FALSE;
}

extern uint32 CurrentLRU;

//=====================================================================================
//	SetupMipData
//=====================================================================================
geBoolean SetupMipData(THandle_MipData *MipData)
{
	if (!MipData->Slot || D3DCache_SlotGetUserData(MipData->Slot) != MipData)
	{
		MipData->Slot = D3DCache_TypeFindSlot(MipData->CacheType);
		assert(MipData->Slot);

		D3DCache_SlotSetUserData(MipData->Slot, MipData);

	#ifdef SUPER_FLUSH
		AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_FLUSHBATCH, 0);
	#endif

		return GE_FALSE;
	}

	return GE_TRUE;
}

//=====================================================================================
//	SetupLMap
//=====================================================================================
geBoolean SetupLMap(int32 Stage, DRV_LInfo *LInfo, int32 LNum, geBoolean Dynamic)
{
#ifdef D3D_MANAGE_TEXTURES
	#ifdef USE_TPAGES
	{
		geRDriver_THandle		*THandle;

		THandle = LInfo->THandle;

		if (Dynamic)
			THandle->Flags |= THANDLE_UPDATE;

		if (!THandle->Block)
		{
			THandle->Block = TPage_MgrFindOptimalBlock(TPageMgr, CurrentLRU);
			THandle->Flags |= THANDLE_UPDATE;
			TPage_BlockSetUserData(THandle->Block, THandle);
			assert(THandle->Block);
		}
		else if (TPage_BlockGetUserData(THandle->Block) != THandle)
		{
			// Find another block
			THandle->Block = TPage_MgrFindOptimalBlock(TPageMgr, CurrentLRU);
			assert(THandle->Block);

			THandle->Flags |= THANDLE_UPDATE;
			TPage_BlockSetUserData(THandle->Block, THandle);
		}

		if (THandle->Flags & THANDLE_UPDATE)
			FillLMapSurface2(LInfo, LNum);

		TPage_BlockSetLRU(THandle->Block, CurrentLRU);
		D3DSetTexture(Stage, TPage_BlockGetTexture(THandle->Block));
	
		if (Dynamic)
			THandle->Flags |= THANDLE_UPDATE;
		else
			THandle->Flags &= ~THANDLE_UPDATE;

		return GE_TRUE;
	}
	#else
	{
		geRDriver_THandle		*THandle;

		THandle = LInfo->THandle;

		if (Dynamic)
			THandle->MipData[0].Flags |= THANDLE_UPDATE;

		if (THandle->MipData[0].Flags & THANDLE_UPDATE)
			FillLMapSurface(LInfo, LNum);

		D3DSetTexture(Stage, THandle->MipData[0].Texture);
	
		if (Dynamic)
			THandle->MipData[0].Flags |= THANDLE_UPDATE;
		else
			THandle->MipData[0].Flags &= ~THANDLE_UPDATE;

		return GE_TRUE;
	}
	#endif

#else
	geRDriver_THandle	*THandle;
	THandle_MipData		*MipData;

	THandle = LInfo->THandle;
	MipData = &THandle->MipData[0];

	if (Dynamic)
		MipData->Flags |= THANDLE_UPDATE;

	if (!SetupMipData(MipData))
	{
		MipData->Flags |= THANDLE_UPDATE;		// Force an upload
		CacheInfo.LMapMisses++;
	}

	if (MipData->Flags & THANDLE_UPDATE)
	{
		HRESULT					Error;
		LPDIRECTDRAWSURFACE4	Surface;

		assert(MipData->Slot);
		
		Surface = D3DCache_SlotGetSurface(MipData->Slot);

		assert(Surface);
		assert(THandle->Log < MAX_LMAP_LOG_SIZE);
		assert(SystemToVideo[THandle->Log].Surface);

		LoadLMapFromSystem(LInfo, THandle->Log, LNum);

		Error = Surface->BltFast(0, 0, SystemToVideo[THandle->Log].Surface, NULL, DDBLTFAST_WAIT);
		//Error = Surface->BltFast(0, 0, SystemToVideo[THandle->Log].Surface, NULL, 0);
		//Error = Surface->Blt(NULL, SystemToVideo[THandle->Log].Surface, NULL, DDBLT_WAIT, NULL);
		//Error = Surface->Blt(NULL, SystemToVideo[THandle->Log].Surface, NULL, 0, NULL);
		
		if (Error != DD_OK)
		{
			if(Error==DDERR_SURFACELOST)
			{
				if (!D3DMain_RestoreAllSurfaces())
					return GE_FALSE;
			}
			else
			{
				D3DMain_Log("SetupTexture: System to Video cache Blt failed.\n %s", D3DErrorToString(Error));
				return GE_FALSE;
			}
		}
	}

	if (Dynamic)		// If it was dynmamic, force an update for one more frame
		MipData->Flags |= THANDLE_UPDATE;
	else
		MipData->Flags &= ~THANDLE_UPDATE;

	D3DCache_SlotSetLRU(MipData->Slot, CurrentLRU);
	D3DSetTexture(Stage, D3DCache_SlotGetTexture(MipData->Slot));

	return GE_TRUE;
#endif
}

//=====================================================================================
//	SetupTexture
//=====================================================================================
geBoolean SetupTexture(int32 Stage, geRDriver_THandle *THandle, int32 MipLevel)
{
#ifdef D3D_MANAGE_TEXTURES
	D3DSetTexture(Stage, THandle->MipData[MipLevel].Texture);
	return GE_TRUE;
#else
	THandle_MipData		*MipData;

	MipData = &THandle->MipData[MipLevel];
	
	if (!SetupMipData(MipData))
	{
		MipData->Flags |= THANDLE_UPDATE;		// Force an upload
		CacheInfo.TexMisses++;
	}

	if (MipData->Flags & THANDLE_UPDATE)
	{
		HRESULT					Error;
		LPDIRECTDRAWSURFACE4	Surface;

		Surface = D3DCache_SlotGetSurface(MipData->Slot);

		Error = Surface->BltFast(0, 0, MipData->Surface, NULL, DDBLTFAST_WAIT);

		if (Error != DD_OK)
		{
			if(Error==DDERR_SURFACELOST)
			{
				if (!D3DMain_RestoreAllSurfaces())
					return FALSE;
			}
			else
			{
				D3DMain_Log("SetupTexture: System to Video cache Blt failed.\n %s", D3DErrorToString(Error));
				return GE_FALSE;
			}
		}
	}

	MipData->Flags &= ~THANDLE_UPDATE;

	D3DCache_SlotSetLRU(MipData->Slot, CurrentLRU);
	D3DSetTexture(Stage, D3DCache_SlotGetTexture(MipData->Slot));

	return GE_TRUE;
#endif
}

//====================================================================================
//	PCache_FlushMiscPolys
//====================================================================================
BOOL PCache_FlushMiscPolys(void)
{
	int32				i;
	Misc_Poly			*pPoly;

	if (!MiscCache.NumPolys)
		return TRUE;

	if (!THandle_CheckCache())
		return GE_FALSE;

	// Set the render states
	if (AppInfo.CanDoMultiTexture)
	{
		AppInfo.lpD3DDevice->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
		AppInfo.lpD3DDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );
		D3DSetTexture(1, NULL);		// Reset texture stage 1
	}
	
	AppInfo.lpD3DDevice->SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, 0);

	AppInfo.lpD3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	AppInfo.lpD3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	AppInfo.lpD3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
	AppInfo.lpD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
	AppInfo.lpD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
	AppInfo.lpD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);

	D3DBlendFunc (D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
	D3DBlendEnable(TRUE);

	// Sort the polys by handle
	SortMiscPolysByHandle();

	for (i=0; i< MiscCache.NumPolys; i++)
	{
		pPoly = MiscCache.SortedPolys[i];

		if (pPoly->Flags & DRV_RENDER_NO_ZMASK)		// We are assuming that this is not going to change all that much
			D3DZEnable(FALSE);
		else
			D3DZEnable(TRUE);

		if (pPoly->Flags & DRV_RENDER_NO_ZWRITE)	// We are assuming that this is not going to change all that much
			D3DZWriteEnable(FALSE);	
		else
			D3DZWriteEnable(TRUE);
									  
		if (pPoly->Flags & DRV_RENDER_CLAMP_UV)
			D3DTexWrap(0, FALSE);
		else
			D3DTexWrap(0, TRUE);

		if (!SetupTexture(0, pPoly->THandle, pPoly->MipLevel))
			return GE_FALSE;

		D3DTexturedPoly(&MiscCache.Verts[pPoly->FirstVert], pPoly->NumVerts);
	}

	// Turn z stuff back on...
	D3DZWriteEnable (TRUE);
	D3DZEnable(TRUE);
	
	MiscCache.NumPolys = 0;
	MiscCache.NumVerts = 0;

#ifdef SUPER_FLUSH
	AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_FLUSHBATCH, 0);
	AppInfo.lpD3DDevice->EndScene();
	AppInfo.lpD3DDevice->BeginScene();
#endif

	return TRUE;
}

//====================================================================================
//	PCache_InsertMiscPoly
//====================================================================================
BOOL PCache_InsertMiscPoly(DRV_TLVertex *Verts, int32 NumVerts, geRDriver_THandle *THandle, uint32 Flags)
{
	int32			Mip;
	float			ZRecip, u, v, ScaleU, ScaleV, InvScale;
	Misc_Poly		*pCachePoly;
	DRV_TLVertex	*pVerts;
	PCache_Vert		*pD3DVerts;
	int32			i, SAlpha;

	if ((MiscCache.NumVerts + NumVerts) >= MAX_MISC_POLY_VERTS)
	{
		// If the cache is full, we must flush it before going on...
		PCache_FlushMiscPolys();
	}
	else if (MiscCache.NumPolys+1 >= MAX_MISC_POLYS)
	{
		// If the cache is full, we must flush it before going on...
		PCache_FlushMiscPolys();
	}

	Mip = GetMipLevel(Verts, NumVerts, (float)THandle->Width, (float)THandle->Height, THandle->NumMipLevels-1);

	// Store info about this poly in the cache
	pCachePoly = &MiscCache.Polys[MiscCache.NumPolys];

	pCachePoly->THandle = THandle;
	pCachePoly->Flags = Flags;
	pCachePoly->FirstVert = MiscCache.NumVerts;
	pCachePoly->NumVerts = NumVerts;
	pCachePoly->MipLevel = Mip;
	pCachePoly->SortKey = ((THandle - TextureHandles)<<4)+Mip;

	// Get scale value for vertices
	//TCache_GetUVInvScale(Bitmap, Mip, &InvScale);
	InvScale = 1.0f / (float)((1<<THandle->Log));

	// Convert them to take account that the vertices are allready from 0 to 1
	ScaleU = (float)THandle->Width * InvScale;
	ScaleV = (float)THandle->Height * InvScale;

	// Precompute the alpha value...
	SAlpha = ((int32)Verts->a)<<24;

	// Get a pointer to the original polys verts
	pVerts = Verts;
	// Get a pointer into the world verts
	pD3DVerts = &MiscCache.Verts[MiscCache.NumVerts];

	for (i=0; i< NumVerts; i++)
	{
		ZRecip = 1/(pVerts->z);

		pD3DVerts->x = pVerts->x;
		pD3DVerts->y = pVerts->y;

		pD3DVerts->z = (1.0f - ZRecip);		// ZBUFFER
		pD3DVerts->rhw = ZRecip;
		
		u = pVerts->u * ScaleU;
		v = pVerts->v * ScaleV;

		pD3DVerts->uv[0].u = u;
		pD3DVerts->uv[0].v = v;

		pD3DVerts->color = SAlpha | ((int32)pVerts->r<<16) | ((int32)pVerts->g<<8) | (int32)pVerts->b;

		if (AppInfo.FogEnable)		// We might get hit on this first "if" but it should predict pretty well in the rest of the tight loop
		{
			DWORD	FogVal;
			float	Val;

			Val = pVerts->z;

			if (Val > AppInfo.FogEnd)
				Val = AppInfo.FogEnd;

			FogVal = (DWORD)((AppInfo.FogEnd-Val)/(AppInfo.FogEnd-AppInfo.FogStart)*255.0f);
		
			if (FogVal < 0)
				FogVal = 0;
			else if (FogVal > 255)
				FogVal = 255;
		
			pD3DVerts->specular = (FogVal<<24);		// Alpha component in specular is the fog value (0...255)
		}
		else
			pD3DVerts->specular = 0;

		pVerts++;
		pD3DVerts++;
	}
	
	// Update globals about the misc poly cache
	MiscCache.NumVerts += NumVerts;
	MiscCache.NumPolys++;

	return TRUE;
}

//====================================================================================
//	**** LOCAL STATIC FUNCTIONS *****
//====================================================================================

//====================================================================================
//	World_PolyPrepVerts
//====================================================================================
geBoolean World_PolyPrepVerts(World_Poly *pPoly, int32 PrepMode, int32 Stage1, int32 Stage2)
{
	float			InvScale, u, v;
	PCache_TVert	*pTVerts;
	PCache_Vert		*pVerts;
	float			ShiftU, ShiftV, ScaleU, ScaleV;
	int32			j;

	switch (PrepMode)
	{
		case PREP_WORLD_VERTS_NORMAL:
		{
			pTVerts = &WorldCache.TVerts[pPoly->FirstVert];

			ShiftU = pPoly->ShiftU;
			ShiftV = pPoly->ShiftV;
		 	ScaleU = pPoly->ScaleU;
			ScaleV = pPoly->ScaleV;

			// Get scale value for vertices
			InvScale = 1.0f / (float)((1<<pPoly->THandle->Log));

			pVerts = &WorldCache.Verts[pPoly->FirstVert];
			
			for (j=0; j< pPoly->NumVerts; j++)
			{
				u = pTVerts->u*ScaleU+ShiftU;
				v = pTVerts->v*ScaleV+ShiftV;

				pVerts->uv[Stage1].u = u * InvScale;
				pVerts->uv[Stage1].v = v * InvScale;

				pVerts->color = pTVerts->Color;

				pTVerts++;
				pVerts++;
			}

			break;
		}

		case PREP_WORLD_VERTS_LMAP:
		{
			if (!pPoly->LInfo)
				return GE_TRUE;

			ShiftU = (float)-pPoly->LInfo->MinU + 8.0f;
			ShiftV = (float)-pPoly->LInfo->MinV + 8.0f;

			// Get scale value for vertices
			InvScale = 1.0f/(float)((1<<pPoly->LInfo->THandle->Log)<<4);
				
			pTVerts = &WorldCache.TVerts[pPoly->FirstVert];
			pVerts = &WorldCache.Verts[pPoly->FirstVert];

			for (j=0; j< pPoly->NumVerts; j++)
			{
				u = pTVerts->u + ShiftU;
				v = pTVerts->v + ShiftV;

				pVerts->uv[Stage1].u = u * InvScale;
				pVerts->uv[Stage1].v = v * InvScale;

				pVerts->color = 0xffffffff;

				pTVerts++;
				pVerts++;
			}
			break;
		}

		case PREP_WORLD_VERTS_SINGLE_PASS:
		{
			float InvScale2, ShiftU2, ShiftV2;

			assert(pPoly->LInfo);

			pTVerts = &WorldCache.TVerts[pPoly->FirstVert];

			// Set up shifts and scaled for texture uv's
			ShiftU = pPoly->ShiftU;
			ShiftV = pPoly->ShiftV;
		 	ScaleU = pPoly->ScaleU;
			ScaleV = pPoly->ScaleV;

			// Get scale value for vertices
			InvScale = 1.0f / (float)((1<<pPoly->THandle->Log));

			// Set up shifts and scaled for lightmap uv's
			ShiftU2 = (float)-pPoly->LInfo->MinU + 8.0f;
			ShiftV2 = (float)-pPoly->LInfo->MinV + 8.0f;
			InvScale2 = 1.0f/(float)((1<<pPoly->LInfo->THandle->Log)<<4);

			pVerts = &WorldCache.Verts[pPoly->FirstVert];

			for (j=0; j< pPoly->NumVerts; j++)
			{
				u = pTVerts->u*ScaleU+ShiftU;
				v = pTVerts->v*ScaleV+ShiftV;

				pVerts->uv[Stage1].u = u * InvScale;
				pVerts->uv[Stage1].v = v * InvScale;
			
				u = pTVerts->u + ShiftU2;
				v = pTVerts->v + ShiftV2;

				pVerts->uv[Stage2].u = u * InvScale2;
				pVerts->uv[Stage2].v = v * InvScale2;

				pVerts->color = pTVerts->Color;

				pTVerts++;
				pVerts++;
			}

			break;
		}

		default:
			return FALSE;
	}

	return TRUE;
}

//====================================================================================
//====================================================================================
static int BitmapHandleComp(const void *a, const void *b)
{
	int32	Id1, Id2;

	Id1 = (*(World_Poly**)a)->SortKey;
	Id2 = (*(World_Poly**)b)->SortKey;

	if ( Id1 == Id2)
		return 0;

	if (Id1 < Id2)
		return -1;

	return 1;
}

//====================================================================================
//====================================================================================
static void SortWorldPolysByHandle(void)
{
	World_Poly	*pPoly;
	int32		i;

	pPoly = WorldCache.Polys;

	for (i=0; i<WorldCache.NumPolys; i++)
	{
		WorldCache.SortedPolys[i] = pPoly;
		pPoly++;
	}
	
	// Sort the polys
	qsort(&WorldCache.SortedPolys, WorldCache.NumPolys, sizeof(WorldCache.SortedPolys[0]), BitmapHandleComp);
}

#define TSTAGE_0			0
#define TSTAGE_1			1

D3DTEXTUREHANDLE		OldId;
LPDIRECT3DTEXTURE2		OldTexture[8];
//====================================================================================
//	RenderWorldPolys
//====================================================================================
static BOOL RenderWorldPolys(int32 RenderMode)
{
	World_Poly			*pPoly;
	int32				i;

	if(!AppInfo.RenderingIsOK)
	{
		return	TRUE;
	}
	switch (RenderMode)
	{	
		case RENDER_WORLD_POLYS_NORMAL:
		{
			AppInfo.lpD3DDevice->SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, 0 );

			// Set the default state for the normal poly render mode for the world
			D3DBlendEnable(TRUE);
			D3DBlendFunc (D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
			
			// Get the first poly in the sorted list
			SortWorldPolysByHandle();
			
			for (i=0; i< WorldCache.NumPolys; i++)
			{
				pPoly = WorldCache.SortedPolys[i];

				if (pPoly->Flags & DRV_RENDER_CLAMP_UV)
					D3DTexWrap(0, FALSE);
				else
					D3DTexWrap(0, TRUE);

				if (!SetupTexture(0, pPoly->THandle, pPoly->MipLevel))
					return GE_FALSE;

				World_PolyPrepVerts(pPoly, PREP_WORLD_VERTS_NORMAL, 0, 0);

				D3DTexturedPoly(&WorldCache.Verts[pPoly->FirstVert], pPoly->NumVerts);
			}
			
			break;
		}
		
		case RENDER_WORLD_POLYS_LMAP:
		{
			AppInfo.lpD3DDevice->SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, 0);

			D3DTexWrap(0, FALSE);

			D3DBlendEnable(TRUE);
			D3DBlendFunc (D3DBLEND_DESTCOLOR, D3DBLEND_ZERO);

			pPoly = WorldCache.Polys;

			for (i=0; i< WorldCache.NumPolys; i++, pPoly++)
			{
				BOOL	Dynamic = 0;

				if (!pPoly->LInfo)
					continue;

				// Call the engine to set this sucker up, because it's visible...
				D3DDRV.SetupLightmap(pPoly->LInfo, &Dynamic);

				if (!SetupLMap(0, pPoly->LInfo, 0, Dynamic))
					return GE_FALSE;

				World_PolyPrepVerts(pPoly, PREP_WORLD_VERTS_LMAP, 0, 0);

				D3DTexturedPoly(&WorldCache.Verts[pPoly->FirstVert], pPoly->NumVerts);
				
				if (pPoly->LInfo->RGBLight[1])
				{
					AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_FOGENABLE, FALSE);

					D3DBlendFunc (D3DBLEND_ONE, D3DBLEND_ONE);				// Change to a fog state

					// For some reason, some cards can't upload data to the same texture twice, and have it take.
					// So we force Fog maps to use a different slot than the lightmap was using...
					pPoly->LInfo->THandle->MipData[0].Slot = NULL;

					if (!SetupLMap(0, pPoly->LInfo, 1, 1))	// Dynamic is 1, because fog is always dynamic
						return GE_FALSE;

					D3DTexturedPoly(&WorldCache.Verts[pPoly->FirstVert], pPoly->NumVerts);
		
					D3DBlendFunc (D3DBLEND_DESTCOLOR, D3DBLEND_ZERO);		// Restore state

					if (AppInfo.FogEnable)
						AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_FOGENABLE , TRUE);
				}
			}
			break;
		}

		case RENDER_WORLD_POLYS_SINGLE_PASS:
		{
			// Setup texture stage states
			AppInfo.lpD3DDevice->SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, 0 );
			AppInfo.lpD3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
			AppInfo.lpD3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
			AppInfo.lpD3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
			AppInfo.lpD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
			AppInfo.lpD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
			AppInfo.lpD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
									 
			AppInfo.lpD3DDevice->SetTextureStageState( 1, D3DTSS_TEXCOORDINDEX, 1 );
			AppInfo.lpD3DDevice->SetTextureStageState( 1, D3DTSS_COLORARG1, D3DTA_TEXTURE );
			AppInfo.lpD3DDevice->SetTextureStageState( 1, D3DTSS_COLORARG2, D3DTA_CURRENT ); 
			AppInfo.lpD3DDevice->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_MODULATE );
			AppInfo.lpD3DDevice->SetTextureStageState( 1, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE );
			AppInfo.lpD3DDevice->SetTextureStageState( 1, D3DTSS_ALPHAARG2, D3DTA_CURRENT );
			//AppInfo.lpD3DDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2 );
			AppInfo.lpD3DDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_MODULATE );

			// Setup frame buffer blend modes
			D3DBlendEnable(TRUE);
			D3DBlendFunc (D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);

			// Set the default state for the normal poly render mode for the world
			D3DTexWrap(TSTAGE_0, TRUE);
			D3DTexWrap(TSTAGE_1, FALSE);

			// Sort the list for front back operation to get the least number of world texture misses
			SortWorldPolysByHandle();
			
			// Reset non lightmaps faces to 0
			WorldCache.NumPolys2 = 0;

			for (i=0; i< WorldCache.NumPolys; i++)
			{
				BOOL	Dynamic = 0;

				pPoly = WorldCache.SortedPolys[i];

				if (!pPoly->LInfo)
				{
					// Put gouraud only polys in a seperate list, and render last
					WorldCache.SortedPolys2[WorldCache.NumPolys2++] = pPoly;
					continue;
				}

				if (pPoly->Flags & DRV_RENDER_CLAMP_UV)
					D3DTexWrap(TSTAGE_0, FALSE);
				else
					D3DTexWrap(TSTAGE_0, TRUE);

				if (!SetupTexture(TSTAGE_0, pPoly->THandle, pPoly->MipLevel))
					return GE_FALSE;				

				// Call the engine to set this sucker up, because it's visible...
				D3DDRV.SetupLightmap(pPoly->LInfo, &Dynamic);

				if (!SetupLMap(TSTAGE_1, pPoly->LInfo, 0, Dynamic))
					return GE_FALSE;
					
				// Prep the verts for a lightmap and texture map
				World_PolyPrepVerts(pPoly, PREP_WORLD_VERTS_SINGLE_PASS, TSTAGE_0, TSTAGE_1);

				// Draw the texture
				D3DTexturedPoly(&WorldCache.Verts[pPoly->FirstVert], pPoly->NumVerts);
				
				// Render any fog maps
				if (pPoly->LInfo->RGBLight[1])
				{
					D3DBlendFunc (D3DBLEND_ONE, D3DBLEND_ONE);				// Change to a fog state

				#if (TSTAGE_0 == 0)
					AppInfo.lpD3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
					AppInfo.lpD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

					AppInfo.lpD3DDevice->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
				#else
					AppInfo.lpD3DDevice->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
					AppInfo.lpD3DDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
				#endif

					// For some reason, some cards can't upload data to the same texture twice, and have it take.
					// So we force Fog maps to use a different slot other than what the lightmap was using...
					pPoly->LInfo->THandle->MipData[0].Slot = NULL;

					if (!SetupLMap(TSTAGE_1, pPoly->LInfo, 1, 1))	// Dynamic is 1, because fog is always dynamic
						return GE_FALSE;

					D3DTexturedPoly(&WorldCache.Verts[pPoly->FirstVert], pPoly->NumVerts);
	
					// Restore states to the last state before fag map
				#if (TSTAGE_0 == 0)
					AppInfo.lpD3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
					AppInfo.lpD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
	
					AppInfo.lpD3DDevice->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_MODULATE );
				#else
					AppInfo.lpD3DDevice->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_MODULATE );
					AppInfo.lpD3DDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2 );
				#endif

					D3DBlendFunc (D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
				}
				
				
			}
			
			// Setup for any non-lightmaped faces faces, turn tmu1 off
		#if (TSTAGE_0 == 0)
			AppInfo.lpD3DDevice->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE);
			AppInfo.lpD3DDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
		#else
			AppInfo.lpD3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
			AppInfo.lpD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
		#endif

			// Render all the faces without lightmaps
			for (i=0; i< WorldCache.NumPolys2; i++)
			{
				BOOL	Dynamic = 0;

				pPoly = WorldCache.SortedPolys2[i];

				if (pPoly->Flags & DRV_RENDER_CLAMP_UV)
					D3DTexWrap(TSTAGE_0, FALSE);
				else
					D3DTexWrap(TSTAGE_0, TRUE);

				if (!SetupTexture(TSTAGE_0, pPoly->THandle, pPoly->MipLevel))
					return GE_FALSE;				

				// Prep verts as if there was no lightmap
				World_PolyPrepVerts(pPoly, PREP_WORLD_VERTS_NORMAL, TSTAGE_0, TSTAGE_1);

				// Draw the texture
				D3DTexturedPoly(&WorldCache.Verts[pPoly->FirstVert], pPoly->NumVerts);
			}

			break;						 
		}

		default:
			return FALSE;
	}


	return TRUE;
}

//====================================================================================
//	ClearWorldCache
//====================================================================================
static BOOL ClearWorldCache(void)
{
	WorldCache.NumPolys = 0;
	WorldCache.NumVerts = 0;

	return TRUE;
}

//====================================================================================
//====================================================================================
BOOL PCache_Reset(void)
{
	WorldCache.NumPolys = 0;
	WorldCache.NumVerts = 0;

	MiscCache.NumPolys = 0;
	MiscCache.NumVerts = 0;

	return TRUE;
}

//====================================================================================
//	GetMipLevel
//====================================================================================
static int32 GetMipLevel(DRV_TLVertex *Verts, int32 NumVerts, float ScaleU, float ScaleV, int32 MaxMipLevel)
{
	int32		Mip;

	if (MaxMipLevel == 0)
		return 0;

	//
	//	Get the MipLevel
	//
	{
		float		du, dv, dx, dy, MipScale;

	#if 1		// WAY slower, but more accurate
		int32		i;

		MipScale = 999999.0f;

		for (i=0; i< NumVerts; i++)
		{
			float			MipScaleT;
			DRV_TLVertex	*pVert0, *pVert1;
			int32			i2;

			i2 = i+1;

			if (i2 >= NumVerts)
				i2=0;

			pVert0 = &Verts[i];
			pVert1 = &Verts[i2];

			du = pVert1->u - pVert0->u;
			dv = pVert1->v - pVert0->v;
			dx = pVert1->x - pVert0->x;
			dy = pVert1->y - pVert0->y;
			
			du *= ScaleU;
			dv *= ScaleV;

			MipScaleT = ((du*du)+(dv*dv)) / ((dx*dx)+(dy*dy));

			if (MipScaleT < MipScale)
				MipScale = MipScaleT;		// Record the best MipScale (the one closest to the the eye)
		}
	#else		// Faster, less accurate
		du = Verts[1].u - Verts[0].u;
		dv = Verts[1].v - Verts[0].v;
		dx = Verts[1].x - Verts[0].x;
		dy = Verts[1].y - Verts[0].y;

		du *= ScaleU;
		dv *= ScaleV;

		MipScale = ((du*du)+(dv*dv)) / ((dx*dx)+(dy*dy));
	#endif

	#if 0
		if (MipScale <= 5)		// 2, 6, 12
			Mip = 0;
		else if (MipScale <= 20)
			Mip = 1;
		else if (MipScale <= 45)
			Mip = 2;
		else
			Mip = 3;
	#else
		if (MipScale <= 4)		// 2, 6, 12
			Mip = 0;
		else if (MipScale <= 15)
			Mip = 1;
		else if (MipScale <= 40)
			Mip = 2;
		else
			Mip = 3;
	#endif
	}

	if (Mip > MaxMipLevel)
		Mip = MaxMipLevel;

	return Mip;
}