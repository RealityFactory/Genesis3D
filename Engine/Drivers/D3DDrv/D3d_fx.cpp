/****************************************************************************************/
/*  D3D_Fx.cpp                                                                          */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: D3D renderstate wrapper                                                */
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
#include <DDraw.h>
#include <D3D.h>

#include "D3DDrv.h"
#include "DCommon.h"
#include "Render.h"
#include "Scene.h"
#include "D3D_FX.h"
#include "D3D_Main.h"
#include "D3D_Err.h"

static D3DTEXTUREHANDLE OldTexHandle = NULL;

//======================================================================================================
//======================================================================================================
void D3DSetTexHandle(D3DTEXTUREHANDLE TexHandle)
{
	if (TexHandle == OldTexHandle)
		return;

	OldTexHandle = TexHandle;

	AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, TexHandle);
}
	
static LPDIRECT3DTEXTURE2 OldTexture[8];

//======================================================================================================
//======================================================================================================
void D3DSetTexture(int32 Stage, LPDIRECT3DTEXTURE2 Texture)
{
	if (Texture == OldTexture[Stage])
		return;

	OldTexture[Stage] = Texture;

	AppInfo.lpD3DDevice->SetTexture(Stage, Texture);
}
		
//======================================================================================================
//======================================================================================================
void D3DBilinearFilter(D3DTEXTUREFILTER Min, D3DTEXTUREFILTER Mag)
{
	AppInfo.lpD3DDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTFN_LINEAR);
	AppInfo.lpD3DDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);

	if (AppInfo.CanDoMultiTexture)
	{
		AppInfo.lpD3DDevice->SetTextureStageState(1, D3DTSS_MINFILTER, D3DTFN_LINEAR);
		AppInfo.lpD3DDevice->SetTextureStageState(1, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
	}
}

//======================================================================================================
// Old one uses D3DTLVERTEX vertex format
//======================================================================================================
void D3DTexturedPolyOld(void *Pnts, int32 NumPoints)
{
	AppInfo.lpD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, D3DFVF_TLVERTEX, Pnts, NumPoints, D3DDP_DONOTUPDATEEXTENTS | D3DDP_DONOTLIGHT | D3DDP_DONOTCLIP);
}	

//======================================================================================================
//	D3DTexturedPoly
//======================================================================================================
void D3DTexturedPoly(void *Pnts, int32 NumPoints)
{
	AppInfo.lpD3DDevice->DrawPrimitive(	D3DPT_TRIANGLEFAN, 
										//D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX2, 
										D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX2, 
										Pnts, 
										NumPoints, 
										D3DDP_DONOTUPDATEEXTENTS | D3DDP_DONOTLIGHT | D3DDP_DONOTCLIP);
										//D3DDP_DONOTUPDATEEXTENTS | D3DDP_DONOTCLIP);
}	

//======================================================================================================
//	D3DViewport
//======================================================================================================
void D3DViewport (int32 x, int32 y, int32 width, int32 height)
{
	D3DVIEWPORT2 vport;

	return;

    vport.dwSize = sizeof(D3DVIEWPORT2);
    AppInfo.lpD3DViewport->GetViewport2(&vport);
    vport.dwX = x;
    vport.dwY = AppInfo.OldHeight - (y + height);
    vport.dwWidth = width;
    vport.dwHeight = height;
    vport.dvClipX = -1.0f;
    vport.dvClipY = 1.0f;
    vport.dvClipWidth = (float)width /2.0f;
    vport.dvClipHeight = (float)height/2.0f;
    AppInfo.lpD3DViewport->SetViewport2(&vport);
}

//======================================================================================================
//======================================================================================================
void D3DDepthRange (float zNear, float zFar)
{
    D3DVIEWPORT2 vport;
    vport.dwSize = sizeof(D3DVIEWPORT2);
    AppInfo.lpD3DViewport->GetViewport2(&vport);
    vport.dvMinZ = (D3DVALUE)(((-1.0) * (zFar + zNear)) / (zFar - zNear));
    vport.dvMaxZ = (D3DVALUE)(((-1.0) * (zFar + zNear - 2.0)) / (zFar - zNear));
    AppInfo.lpD3DViewport->SetViewport2(&vport);
}

static D3DBLEND OldSFunc = D3DBLEND_ONE;
static D3DBLEND OldDFunc = D3DBLEND_ONE;

//======================================================================================================
//======================================================================================================
void D3DBlendFunc (D3DBLEND SFunc, D3DBLEND DFunc)
{
	if (SFunc != OldSFunc)
	{
		AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, SFunc);
		OldSFunc = SFunc;
	}
	if (DFunc != OldDFunc)
	{
		AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND, DFunc);
		OldDFunc = DFunc;
	}

}

static BOOL OldBlend = FALSE;

//======================================================================================================
//======================================================================================================
void D3DBlendEnable(BOOL Enable)
{
	if (OldBlend == Enable)
		return;

	AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, Enable);

	OldBlend = Enable;
}

static BOOL OldWrap = FALSE;

void D3DTexWrap(DWORD Stage, BOOL Wrap)
{
	if (OldWrap == Wrap)
		return;

	OldWrap = Wrap;

	if (Wrap)
	{
		AppInfo.lpD3DDevice->SetTextureStageState(Stage, D3DTSS_ADDRESS, D3DTADDRESS_WRAP);
		AppInfo.lpD3DDevice->SetTextureStageState(Stage, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
		AppInfo.lpD3DDevice->SetTextureStageState(Stage, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
	}
	else
	{
		AppInfo.lpD3DDevice->SetTextureStageState(Stage, D3DTSS_ADDRESS, D3DTADDRESS_CLAMP);
		AppInfo.lpD3DDevice->SetTextureStageState(Stage, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
		AppInfo.lpD3DDevice->SetTextureStageState(Stage, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
	}
}

void D3DZWriteEnable (BOOL Enable)
{
	AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, Enable);
}

void D3DZFunc (D3DCMPFUNC Func)
{
	AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_ZFUNC, Func);
}

void D3DZEnable(BOOL Enable)
{
	AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_ZENABLE, Enable);
}

void D3DPolygonMode (D3DFILLMODE Mode)
{
	AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_FILLMODE, D3DFILL_SOLID);
}



