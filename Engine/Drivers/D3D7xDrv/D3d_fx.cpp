/****************************************************************************************/
/*  D3D_Fx.cpp                                                                          */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: D3D renderstate wrapper                                                */
/*                                                                                      */
/*  01/13/2003 Wendell Buckner                                                          */
/*   Optimization from GeForce_Optimization2.doc                                        */
/*   9.	Do not duplicate render state commands.  Worse is useless renderstates.  Do not */
/*      set a renderstate unless it is needed.                                          */
/*  02/28/2001 Wendell Buckner                                                          */
/*   The flags at the end of DrawPrimitive are no longer supported in  d3d 7.0, use the */
/*   combination of flags in the renderstate call                                       */
/*   Optimization from GeForce_Optimization2.doc                                        */
/*    9. Do not duplicate render state commands.  Worse is useless renderstates.        */ 
/*       Do not set a renderstate unless it is needed.                                  */ 
/*   These render states are unsupported d3d 7.0                                        */
/*  07/16/2000 Wendell Buckner                                                          */
/*    Convert to Directx7...                                                            */
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

/*  07/16/2000 Wendell Buckner       
/*    Convert to Directx7...         
#include "D3DDrv.h"                */
#include "D3DDrv7x.h"

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

/* 02/28/2001 Wendell Buckner
   These render states are unsupported d3d 7.0
	AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, TexHandle);*/
}

/*   07/16/2000 Wendell Buckner
/*    Convert to Directx7...    
static LPDIRECT3DTEXTURE2    OldTexture[8];*/
static LPDIRECTDRAWSURFACE7  OldTexture[8];	


//======================================================================================================
//======================================================================================================

/*   07/16/2000 Wendell Buckner
/*    Convert to Directx7...    
void D3DSetTexture(int32 Stage, LPDIRECT3DTEXTURE2   Texture)  */
void D3DSetTexture(int32 Stage, LPDIRECTDRAWSURFACE7 Texture)

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
/* 02/28/2001 Wendell Buckner
   The flags at the end of DrawPrimitive are no longer supported in  d3d 7.0, use the combination of flags in 
   the renderstate call */
	HRESULT Hr;

/*	02/08/2002 Wendell Buckner
    Optimization from GeForce_Optimization2.doc
    9.	Do not duplicate render state commands.  Worse is useless renderstates.  Do not set a renderstate unless it is needed. 
	AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_CLIPPING,FALSE);
	AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_LIGHTING,FALSE);
	AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_EXTENTS,FALSE);                                                        
	AppInfo.lpD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, D3DFVF_TLVERTEX, Pnts, NumPoints, D3DDP_DONOTUPDATEEXTENTS | D3DDP_DONOTLIGHT | D3DDP_DONOTCLIP);*/
	Hr = AppInfo.lpD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, D3DFVF_TLVERTEX, Pnts, NumPoints, NULL);
	if( Hr != DD_OK ) D3DMain_Log("D3DTExturedPolOld failed...\n\n");
}	

//======================================================================================================
//	D3DTexturedPoly
//======================================================================================================
void D3DTexturedPoly(void *Pnts, int32 NumPoints)
{
/* 02/28/2001 Wendell Buckner
   The flags at the end of DrawPrimitive are no longer supported in  d3d 7.0, use the combination of flags in 
   the renderstate call */
	HRESULT Hr;

/*	02/08/2002 Wendell Buckner
    Optimization from GeForce_Optimization2.doc
    9.	Do not duplicate render state commands.  Worse is useless renderstates.  Do not set a renderstate unless it is needed. 
	AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_CLIPPING,FALSE);
	AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_LIGHTING,FALSE);
	AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_EXTENTS,FALSE);
	AppInfo.lpD3DDevice->DrawPrimitive(	D3DPT_TRIANGLEFAN, 
										//D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX2, 
										D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX2, 
										Pnts, 
										NumPoints, 
										D3DDP_DONOTUPDATEEXTENTS | D3DDP_DONOTLIGHT | D3DDP_DONOTCLIP);
										//D3DDP_DONOTUPDATEEXTENTS | D3DDP_DONOTCLIP);*/
	Hr = AppInfo.lpD3DDevice->DrawPrimitive(	D3DPT_TRIANGLEFAN, 
										D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX2, 
										Pnts, 
										NumPoints, 
										NULL);

	if( Hr != DD_OK ) D3DMain_Log("D3DTexturedPoly failed...\n\n");
}	

//======================================================================================================
//	D3DViewport
//======================================================================================================
void D3DViewport (int32 x, int32 y, int32 width, int32 height)
{
/*   07/16/2000 Wendell Buckner
/*    Convert to Directx7...    
	D3DVIEWPORT2 vport;         */ 
	D3DVIEWPORT7 vport;

//03/01/2002 Wendell Buckner	
//This wasn't commented in the original code but it was in my genesis 1.0 converted code...
//not sure I should do this.
//	return;
//
//  vport.dwSize = sizeof(D3DVIEWPORT2);

/*   07/16/2000 Wendell Buckner
/*    Convert to Directx7...    
    AppInfo.lpD3DViewport->GetViewport2(&vport); */
    AppInfo.lpD3DDevice->GetViewport(&vport);

    vport.dwX = x;
    vport.dwY = AppInfo.OldHeight - (y + height);
    vport.dwWidth = width;
    vport.dwHeight = height;

/*  07/16/2000 Wendell Buckner
    Convert to Directx7...    
    vport.dvClipX = -1.0f;
    vport.dvClipY = 1.0f;
    vport.dvClipWidth = (float)width /2.0f;
    vport.dvClipHeight = (float)height/2.0f;*/

/*   07/16/2000 Wendell Buckner
/*    Convert to Directx7...        
    AppInfo.lpD3DViewport->SetViewport2(&vport); */
	AppInfo.lpD3DDevice->SetViewport(&vport);
}

//======================================================================================================
//======================================================================================================
void D3DDepthRange (float zNear, float zFar)
{
/*   07/16/2000 Wendell Buckner
/*    Convert to Directx7...         
    D3DVIEWPORT2 vport;*/
	D3DVIEWPORT7 vport;

//03/01/2002 Wendell Buckner	
//This wasn't commented in the original code but it was in my genesis 1.0 converted code...
//not sure I should do this.
//  vport.dwSize = sizeof(D3DVIEWPORT2);

/*   07/16/2000 Wendell Buckner
/*    Convert to Directx7...     
    AppInfo.lpD3DViewport->GetViewport2(&vport);*/
	AppInfo.lpD3DDevice->GetViewport(&vport);

//03/01/2002 Wendell Buckner	
//This wasn't commented in the original code but it was in my genesis 1.0 converted code...
//not sure I should do this.
//  vport.dvMinZ = (D3DVALUE)(((-1.0) * (zFar + zNear)) / (zFar - zNear));
//  vport.dvMaxZ = (D3DVALUE)(((-1.0) * (zFar + zNear - 2.0)) / (zFar - zNear));
    vport.dvMinZ = zNear; 
    vport.dvMaxZ = zFar;  


/*   07/16/2000 Wendell Buckner
/*    Convert to Directx7...     
	AppInfo.lpD3DViewport->SetViewport2(&vport);*/
    AppInfo.lpD3DDevice->SetViewport(&vport);
    
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

/* 01/13/2003 Wendell Buckner
    Optimization from GeForce_Optimization2.doc  
9.	Do not duplicate render state commands.  Worse is useless renderstates.  Do not set a renderstate unless it is needed. */
static BOOL OldZWriteEnable = TRUE;

void D3DZWriteEnable (BOOL Enable)
{
/* 01/13/2003 Wendell Buckner
    Optimization from GeForce_Optimization2.doc  
9.	Do not duplicate render state commands.  Worse is useless renderstates.  Do not set a renderstate unless it is needed. */
  	if (OldZWriteEnable == Enable)
  		return;
  
  	OldZWriteEnable = Enable;

	AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, Enable);
}

/* 01/13/2003 Wendell Buckner
    Optimization from GeForce_Optimization2.doc  
9.	Do not duplicate render state commands.  Worse is useless renderstates.  Do not set a renderstate unless it is needed.*/
static D3DCMPFUNC OldZFunc = D3DCMP_LESSEQUAL;

void D3DZFunc (D3DCMPFUNC Func)
{
/* 01/13/2003 Wendell Buckner
    Optimization from GeForce_Optimization2.doc  
9.	Do not duplicate render state commands.  Worse is useless renderstates.  Do not set a renderstate unless it is needed. */
  	if (OldZFunc == Func)
  		return;
  
  	OldZFunc = Func;

	AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_ZFUNC, Func);
}

/* 01/13/2003 Wendell Buckner
    Optimization from GeForce_Optimization2.doc  
9.	Do not duplicate render state commands.  Worse is useless renderstates.  Do not set a renderstate unless it is needed. 
    The default is true if there is a depth buffer attached, we assume there is one with genesis */
static BOOL OldZEnable = TRUE;

void D3DZEnable(BOOL Enable)
{
  	if (OldZEnable == Enable)
  		return;
  
  	OldZEnable = Enable;
  
  	AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_ZENABLE, Enable);
}

/* 01/13/2003 Wendell Buckner
    Optimization from GeForce_Optimization2.doc  
9.	Do not duplicate render state commands.  Worse is useless renderstates.  Do not set a renderstate unless it is needed. */
static D3DFILLMODE OldPolygonMode = D3DFILL_SOLID;

void D3DPolygonMode (D3DFILLMODE Mode)
{
/* 01/13/2003 Wendell Buckner
    Optimization from GeForce_Optimization2.doc  
9.	Do not duplicate render state commands.  Worse is useless renderstates.  Do not set a renderstate unless it is needed. */
  	if (OldPolygonMode == Mode)
  		return;
  
  	OldPolygonMode = Mode;

	AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_FILLMODE, D3DFILL_SOLID);
}


/* 01/13/2003 Wendell Buckner
    Optimization from GeForce_Optimization2.doc  
9.	Do not duplicate render state commands.  Worse is useless renderstates.  Do not set a renderstate unless it is needed. */
static BOOL  OldFogEnable = FALSE;
static DWORD OldFogColor  = 0;

void D3DFogEnable (BOOL Enable, DWORD Color)
{
  	if ( (OldFogEnable == Enable) && (OldFogColor == Color) )
  		return;

	if ( OldFogEnable != Enable )
	{
		OldFogEnable = Enable;
		AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_FOGENABLE , Enable);
	}

	if ( Enable && (OldFogColor != Color) ) 
	{
		OldFogColor = Color;
		AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_FOGCOLOR , Color);	
	}
}
