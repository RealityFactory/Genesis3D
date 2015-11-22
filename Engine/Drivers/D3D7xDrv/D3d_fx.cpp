/****************************************************************************************/
/*  D3D_Fx.cpp                                                                          */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: D3D renderstate wrapper                                                */
/*                                                                                      */
/*  TODO: PIXEL FOG                                                                     */
/*  Edit History:                                                                       */             
/*  12/27/2003 Wendell Buckner                                                          */
/*   CONFIG DRIVER - Allow full-scene anti-aliasing                                     */
/*  08/06/2003 Wendell Buckner                                                          */
/*   BUMPMAPPING                                                                        */       
/*	  BUG - This is causing bumpmapping affect not to work, basically D3DTexWrap was    */
/*    causing the second texture to be mismatch... setting the first stage to wrap and  */
/*    second to clamp causing the effect                                                */    
/*	not to work...                                                                      */
/*  04/08/2003 Wendell Buckner                                                          */ 
/*    BUMPMAPPING                                                                       */
/*  02/20/2003 Wendell Bucker                                                           */
/*    This was causing the first chance exception in the debugger!                      */
/*  01/28/2003 Wendell Buckner                                                          */             
/*   Moving ALL triangles into another buffer is resulting in minimum speed increase    */
/*    (+1FPS increase)...	                                                            */
/*  01/18/2003 Wendell Buckner                                                          */
/*   Optimization from GeForce_Optimization2.doc                                        */
/*   9.	Do not duplicate render state commands.  Worse is useless renderstates.  Do not */
/*      set a renderstate unless it is needed.                                          */
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

//======================================================================================================
//======================================================================================================

/* 12/27/2003 Wendell Buckner
    CONFIG DRIVER - Allow full-scene anti-aliasing  */
extern int FSAntiAliasing;

BOOL FSAA = FALSE;

void D3DFullSceneAntiAliasing ( BOOL Enable )
{
 if (!FSAntiAliasing ) return;

 if (!(AppInfo.Drivers[AppInfo.CurrentDriver].Desc.dpcLineCaps.dwRasterCaps & D3DPRASTERCAPS_ANTIALIASSORTINDEPENDENT) )  return;

 if ( FSAA == Enable) return;

 if ( Enable )
  AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_ANTIALIAS,D3DANTIALIAS_SORTINDEPENDENT ); 
 else
  AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_ANTIALIAS,D3DANTIALIAS_NONE );

  FSAA = Enable;
}

//======================================================================================================
//======================================================================================================
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
 HRESULT				Hr;
/* 06/19/2003 Wendell Buckner
	BUMPMAPPING              *
	DDSURFACEDESC2		ddsd;

    char * c = NULL;
    static bool f = FALSE;

    if(Texture)
	{

	memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
    Hr = Texture->GetSurfaceDesc(&ddsd);
    c = D3DErrorToString(Hr);

	{
	 bool foobar = FALSE;
     if (ddsd.ddpfPixelFormat.dwFlags & DDPF_BUMPDUDV)
	 {
         HRESULT	ddrval;
		 USHORT     *pTempBits;
		 BYTE       *pTempBits2;

	     ddrval = Texture->Lock(NULL, &ddsd, DDLOCK_WAIT | DDLOCK_WRITEONLY, NULL);

         pTempBits = (USHORT *) ddsd.lpSurface;

		 pTempBits2 = (BYTE *) ddsd.lpSurface;

		 if ( !f)
		 {
		  FILE * braw = fopen("c:\\BMB_EARTH.RAW","rb");

		  while(!feof(braw))
		  {
		   BYTE b;
		   fread(&b,sizeof(b),1,braw);
		   *pTempBits2 = b;
		   pTempBits2++;
		  }

          fclose(braw);
		  f = TRUE;
		 }

		 ddrval = Texture->Unlock(NULL);
 		 foobar = TRUE;
	 }

	 if( ddsd.ddpfPixelFormat.dwFlags & DDPF_BUMPLUMINANCE)
  	 {
		 foobar = TRUE;
	 }

     if( ddsd.ddpfPixelFormat.dwBumpBitCount == 16 )
	 {
		 foobar = TRUE;
	 }

     if( ddsd.ddpfPixelFormat.dwBumpDuBitMask  & 0x0000001f)
	 {
		 foobar = TRUE;
	 }

	 if( ddsd.ddpfPixelFormat.dwBumpDvBitMask  & 0x000003e0 )
	 {
		 foobar = TRUE;
	 }

     if( ddsd.ddpfPixelFormat.dwBumpLuminanceBitMask & 0x0000fc00)
	 {
		 foobar = TRUE;
	 }

	}
	}*/

	if (Texture == OldTexture[Stage])
		return;

	OldTexture[Stage] = Texture;

	Hr = AppInfo.lpD3DDevice->SetTexture(Stage, Texture);
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
//======================================================================================================

// changed QD Shadows
void D3DDrawShadow(geFloat r, geFloat g, geFloat b, geFloat a)
{
	D3DZEnable(FALSE);
	D3DStencilEnable(TRUE);

	// Turn on Alphablending
	D3DBlendEnable(TRUE);
	D3DBlendFunc (D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);

	// Only write where stencil val >= 1 (count indicates # of shadows that
    // overlap that pixel)
	AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_STENCILREF, 0x1);

	D3DStencilFunc(D3DCMP_LESSEQUAL);

	AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_STENCILPASS, D3DSTENCILOP_KEEP);

	// Get viewport dimensions for big, gray square
	D3DVIEWPORT7 vport;
	AppInfo.lpD3DDevice->GetViewport(&vport);
	FLOAT sx = (FLOAT)vport.dwWidth;
    FLOAT sy = (FLOAT)vport.dwHeight;

	// Draw a big, gray square
	D3DXYZRHWVERTEX/*D3DTLVERTEX*/ vBigGraySquare[4];
	for (int i=0; i<4; i++)
	{
		vBigGraySquare[i].sx = 0.0f;
		vBigGraySquare[i].sy = 0.0f;
		vBigGraySquare[i].sz = 0.0f;	
		vBigGraySquare[i].rhw = 1.0f;
		vBigGraySquare[i].color = ((uint32)a<<24) | ((uint32)r<<16) | ((uint32)g<<8) | (uint32)b;

	}
	vBigGraySquare[0].sy = sy;
	vBigGraySquare[2].sx = sx;
	vBigGraySquare[2].sy = sy;
	vBigGraySquare[3].sx = sx;

	AppInfo.lpD3DDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, 
										vBigGraySquare, 4, NULL);

	// Restore render states
	D3DZEnable(TRUE);
	D3DStencilEnable(FALSE);
	D3DBlendEnable(FALSE);
}
// end change

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
//	D3DTexturedPoly3
//======================================================================================================
/* 01/28/2003 Wendell Buckner 
   Moving ALL triangles into another buffer is resulting in minimum speed increase (+1FPS increase)...	*/
void D3DTexturedPoly3(void *Pnts, int32 NumPoints)
{
/* 02/28/2001 Wendell Buckner
   The flags at the end of DrawPrimitive are no longer supported in  d3d 7.0, use the combination of flags in 
   the renderstate call */
	HRESULT Hr;

	Hr = AppInfo.lpD3DDevice->DrawPrimitive(	D3DPT_TRIANGLELIST, 
										D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX2, 
										Pnts, 
										NumPoints, 
										NULL);

	if( Hr != DD_OK ) D3DMain_Log("D3DTexturedPoly3 failed...\n\n");
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
//  vport.dwSize = sizeof(D3DVIEWPORT7);

/*   07/16/2000 Wendell Buckner
/*    Convert to Directx7...    
    AppInfo.lpD3DViewport->GetViewport2(&vport); */
    AppInfo.lpD3DDevice->GetViewport(&vport);

    vport.dwX = x;

/* 02/20/2003 Wendell Bucker
    This was causing the first chance exception in the debugger!
    vport.dwY = AppInfo.OldHeight - (y + height);              */
    vport.dwY = y;

    vport.dwWidth = width;
    vport.dwHeight = height;
	vport.dvMinZ = 0; 
    vport.dvMaxZ = 1;  

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

/* 08/06/2003 Wendell Buckner
    BUMPMAPPING 
	BUG - This is causing bumpmapping affect not to work, basically D3DTexWrap was causing the second
	texture to be mismatch... setting the first stage to wrap and second to clamp causing the effect 
	not to work...
static BOOL OldWrap = FALSE; */
static BOOL OldWrap0 = FALSE;
static BOOL OldWrap1 = FALSE;
static BOOL OldWrap2 = FALSE;

void D3DTexWrap(DWORD Stage, BOOL Wrap)
{

/* 08/06/2003 Wendell Buckner
    BUMPMAPPING 
	BUG - This is causing bumpmapping affect not to work, basically D3DTexWrap was causing the second
	texture to be mismatch... setting the first stage to wrap and second to clamp causing the effect 
	not to work...
	if (OldWrap == Wrap)
		return; 
    OldWrap0 = Wrap; */
	switch(Stage) 
	{
		case 0:
			 if (OldWrap0 == Wrap) 
				 return;
			 else
			 {
				 OldWrap0 = Wrap;
				 break;
			 }
		case 1:
			 if (OldWrap1 == Wrap) 
				 return;
			 else
			 {
				 OldWrap1 = Wrap;
				 break;
			 }
		case 2:
			 if (OldWrap2 == Wrap) 
				 return;
			 else
			 {
				 OldWrap2 = Wrap;
				 break;
			 }
		default:
             return;
	}	

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

// changed QD Shadows
static D3DCMPFUNC OldStencilFunc = D3DCMP_LESSEQUAL;
void D3DStencilFunc (D3DCMPFUNC Func)
{
  	if (OldStencilFunc == Func)
  		return;
  
  	OldStencilFunc = Func;

	AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_STENCILFUNC, Func);

}

static BOOL OldStencilEnable = FALSE;
void D3DStencilEnable(BOOL Enable)
{
  	if (OldStencilEnable == Enable)
  		return;
  
  	OldStencilEnable = Enable;
  
  	AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_STENCILENABLE, Enable);
}
// end change

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

/* TODO: PIXEL FOG
	AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_FOGTABLEMODE,D3DFOG_LINEAR);
	AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_FOGSTART, *(DWORD *) &AppInfo.FogStart);
	AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_FOGEND,*(DWORD *) &AppInfo.FogEnd); */
}

/* 01/18/2003 Wendell Buckner
    Optimization from GeForce_Optimization2.doc  
9.	Do not duplicate render state commands.  Worse is useless renderstates.  Do not set a renderstate unless it is needed. */


static unsigned long OldTexCoordTSS0  = -1; 
static unsigned long OldColorArg1TSS0 = D3DTA_TEXTURE; 
static unsigned long OldColorArg2TSS0 = D3DTA_CURRENT;
static D3DTEXTUREOP  OldColorOpTSS0   = D3DTOP_MODULATE;
static unsigned long OldAlphaArg1TSS0 = D3DTA_TEXTURE; 
static unsigned long OldAlphaArg2TSS0 = D3DTA_CURRENT;
static D3DTEXTUREOP  OldAlphaOpTSS0   = D3DTOP_SELECTARG1;

/* 04/08/2003 Wendell Buckner
    BUMPMAPPING 
void D3DSetTextureStageState0(unsigned long ColorArg1, unsigned long ColorArg2,D3DTEXTUREOP ColorOp, unsigned long AlphaArg1, unsigned long AlphaArg2, D3DTEXTUREOP AlphaOp) */
void D3DSetTextureStageState0(unsigned long ColorArg1, unsigned long ColorArg2,D3DTEXTUREOP ColorOp, unsigned long AlphaArg1, unsigned long AlphaArg2, D3DTEXTUREOP AlphaOp, unsigned long TexCoordTSS0)
{
/* 04/08/2003 Wendell Buckner
    BUMPMAPPING *
	if ( (OldColorArg1TSS0 == ColorArg1) && (OldColorArg2TSS0 == ColorArg2) && (OldColorOpTSS0 == ColorOp) && */
	if ( (OldTexCoordTSS0  == TexCoordTSS0) && 
		 (OldColorArg1TSS0 == ColorArg1)    && (OldColorArg2TSS0 == ColorArg2) && (OldColorOpTSS0 == ColorOp) && 
		 (OldAlphaArg1TSS0 == AlphaArg1)    && (OldAlphaArg2TSS0 == AlphaArg2) && (OldAlphaOpTSS0 == AlphaOp) )
		 return;


/* 04/08/2003 Wendell Buckner
    BUMPMAPPING *
 	if ( (OldTexCoordTSS0 != 0) ) */
	if ( (OldTexCoordTSS0 != TexCoordTSS0) )
	{
/* 04/08/2003 Wendell Buckner
    BUMPMAPPING 
     OldTexCoordTSS0 = 0;
     AppInfo.lpD3DDevice->SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, 0 ); */
     OldTexCoordTSS0 = TexCoordTSS0;
     AppInfo.lpD3DDevice->SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, TexCoordTSS0 );
	}

  	if ( (OldColorArg1TSS0 != ColorArg1) )
	{
     OldColorArg1TSS0 = ColorArg1;
  	 AppInfo.lpD3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, ColorArg1 );
	}

  	if ( (OldColorArg2TSS0 != ColorArg2) )
	{
  	 OldColorArg2TSS0 = ColorArg2;
  	 AppInfo.lpD3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, ColorArg2 );
	}

	if ( (OldColorOpTSS0 != ColorOp) )
	{
  	 OldColorOpTSS0 = ColorOp;
  	 AppInfo.lpD3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP,	 ColorOp );
	}
			 
  	if ( (OldAlphaArg1TSS0 != AlphaArg1) )
	{
  	 OldAlphaArg1TSS0 = AlphaArg1;
     AppInfo.lpD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, AlphaArg1 );
	}

  	if ( (OldAlphaArg2TSS0 != AlphaArg2) )
	{
  	 OldAlphaArg2TSS0 = AlphaArg2;
  	 AppInfo.lpD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, AlphaArg2 );
	}

	if ( (OldAlphaOpTSS0 != AlphaOp) )
	{
	 OldAlphaOpTSS0 = AlphaOp;
	 AppInfo.lpD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,	AlphaOp );
	}

}


/* 01/18/2003 Wendell Buckner
    Optimization from GeForce_Optimization2.doc  
9.	Do not duplicate render state commands.  Worse is useless renderstates.  Do not set a renderstate unless it is needed. */
static unsigned long          OldTexCoordTSS1  = -1; 
static unsigned long          OldColorArg1TSS1 = D3DTA_TEXTURE; 
static unsigned long          OldColorArg2TSS1 = D3DTA_CURRENT;
static D3DTEXTUREOP           OldColorOpTSS1   = D3DTOP_DISABLE;
static unsigned long          OldAlphaArg1TSS1 = D3DTA_TEXTURE; 
static unsigned long          OldAlphaArg2TSS1 = D3DTA_CURRENT;
static D3DTEXTUREOP           OldAlphaOpTSS1   = D3DTOP_DISABLE;

/* 04/08/2003 Wendell Buckner
    BUMPMAPPING 
void D3DSetTextureStageState1(unsigned long ColorArg1, unsigned long ColorArg2,D3DTEXTUREOP ColorOp, unsigned long AlphaArg1, unsigned long AlphaArg2, D3DTEXTUREOP AlphaOp) */
void D3DSetTextureStageState1(unsigned long ColorArg1, unsigned long ColorArg2,D3DTEXTUREOP ColorOp, unsigned long AlphaArg1, unsigned long AlphaArg2, D3DTEXTUREOP AlphaOp, unsigned long TexCoordTSS1)
{
/* 04/08/2003 Wendell Buckner
    BUMPMAPPING 
	if ( (OldColorArg1TSS1 == ColorArg1) && (OldColorArg2TSS1 == ColorArg2) && (OldColorOpTSS1 == ColorOp) && */
	if ( (OldTexCoordTSS1  == TexCoordTSS1) && 
		 (OldColorArg1TSS1 == ColorArg1) && (OldColorArg2TSS1 == ColorArg2) && (OldColorOpTSS1 == ColorOp) && 
		 (OldAlphaArg1TSS1 == AlphaArg1)    && (OldAlphaArg2TSS1 == AlphaArg2) && (OldAlphaOpTSS1 == AlphaOp) )
		 return;

/* 04/08/2003 Wendell Buckner
    BUMPMAPPING 
 	if ( (OldTexCoordTSS1 != 1) ) */
	if ( (OldTexCoordTSS1 != TexCoordTSS1) )		
	{
/* 04/08/2003 Wendell Buckner
    BUMPMAPPING 
     OldTexCoordTSS1 = 1;
     AppInfo.lpD3DDevice->SetTextureStageState( 1, D3DTSS_TEXCOORDINDEX, 1 ); */
	 OldTexCoordTSS1 = TexCoordTSS1;
     AppInfo.lpD3DDevice->SetTextureStageState( 1, D3DTSS_TEXCOORDINDEX, TexCoordTSS1 );
	}

  	if ( (OldColorArg1TSS1 != ColorArg1) )
	{
     OldColorArg1TSS1 = ColorArg1;
  	 AppInfo.lpD3DDevice->SetTextureStageState( 1, D3DTSS_COLORARG1, ColorArg1 );
	}

  	if ( (OldColorArg2TSS1 != ColorArg2) )
	{
  	 OldColorArg2TSS1 = ColorArg2;
  	 AppInfo.lpD3DDevice->SetTextureStageState( 1, D3DTSS_COLORARG2, ColorArg2 );
	}

	if ( (OldColorOpTSS1 != ColorOp) )
	{
	 OldColorOpTSS1 = ColorOp;
	 AppInfo.lpD3DDevice->SetTextureStageState( 1, D3DTSS_COLOROP,	 ColorOp );
	}
			 
  	if ( (OldAlphaArg1TSS1 != AlphaArg1) )
	{
  	 OldAlphaArg1TSS1 = AlphaArg1;
  	 AppInfo.lpD3DDevice->SetTextureStageState( 1, D3DTSS_ALPHAARG1, AlphaArg1 );
	}

  	if ( (OldAlphaArg2TSS1 != AlphaArg2) )
	{
  	 OldAlphaArg2TSS1 = AlphaArg2;
  	 AppInfo.lpD3DDevice->SetTextureStageState( 1, D3DTSS_ALPHAARG2, AlphaArg2 );
	}

	if ( (OldAlphaOpTSS1 != AlphaOp) )
	{
	 OldAlphaOpTSS1 = AlphaOp;
	 AppInfo.lpD3DDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP,	AlphaOp );
	}

}

/* 01/18/2003 Wendell Buckner
    Optimization from GeForce_Optimization2.doc  
9.	Do not duplicate render state commands.  Worse is useless renderstates.  Do not set a renderstate unless it is needed. */

/* 04/08/2003 Wendell Buckner
    BUMPMAPPING */
static unsigned long          OldTexCoordTSS2  = -1; 
static unsigned long          OldColorArg1TSS2 = D3DTA_TEXTURE; 
static unsigned long          OldColorArg2TSS2 = D3DTA_CURRENT;
static D3DTEXTUREOP			  OldColorOpTSS2   = D3DTOP_DISABLE;
static unsigned long          OldAlphaArg1TSS2 = D3DTA_TEXTURE; 
static unsigned long          OldAlphaArg2TSS2 = D3DTA_CURRENT;
static D3DTEXTUREOP			  OldAlphaOpTSS2   = D3DTOP_DISABLE;

void D3DSetTextureStageState2(unsigned long ColorArg1, unsigned long ColorArg2,D3DTEXTUREOP ColorOp, unsigned long AlphaArg1, unsigned long AlphaArg2, D3DTEXTUREOP AlphaOp, unsigned long TexCoordTSS2)
{
	if ( (OldTexCoordTSS2 == TexCoordTSS2) && 
		 (OldColorArg1TSS2 == ColorArg1)   && (OldColorArg2TSS2 == ColorArg2) && (OldColorOpTSS2 == ColorOp) && 
		 (OldAlphaArg1TSS2 == AlphaArg1)   && (OldAlphaArg2TSS2 == AlphaArg2) && (OldAlphaOpTSS2 == AlphaOp) )
		 return;

	if ( (OldTexCoordTSS2 != TexCoordTSS2) )		
	{
	 OldTexCoordTSS2 = TexCoordTSS2;
     AppInfo.lpD3DDevice->SetTextureStageState( 2, D3DTSS_TEXCOORDINDEX, TexCoordTSS2 );
	}

	if ( (OldColorArg1TSS2 != ColorArg1) )
	{
     OldColorArg1TSS2 = ColorArg1;
	 AppInfo.lpD3DDevice->SetTextureStageState( 2, D3DTSS_COLORARG1, ColorArg1 );
	}

	if ( (OldColorArg2TSS2 != ColorArg2) )
	{
	 OldColorArg2TSS2 = ColorArg2;
	 AppInfo.lpD3DDevice->SetTextureStageState( 2, D3DTSS_COLORARG2, ColorArg2 );
	}

	if ( (OldColorOpTSS2 != ColorOp) )
	{
	 OldColorOpTSS2 = ColorOp;
	 AppInfo.lpD3DDevice->SetTextureStageState( 2, D3DTSS_COLOROP,	 ColorOp );
	}
			 
	if ( (OldAlphaArg1TSS2 != AlphaArg1) )
	{
	 OldAlphaArg1TSS2 = AlphaArg1;
	 AppInfo.lpD3DDevice->SetTextureStageState( 2, D3DTSS_ALPHAARG1, AlphaArg1 );
	}

	if ( (OldAlphaArg2TSS2 != AlphaArg2) )
	{
	 OldAlphaArg2TSS2 = AlphaArg2;
	 AppInfo.lpD3DDevice->SetTextureStageState( 2, D3DTSS_ALPHAARG2, AlphaArg2 );
	}

	if ( (OldAlphaOpTSS2 != AlphaOp) )
	{
	 OldAlphaOpTSS2 = AlphaOp;
	 AppInfo.lpD3DDevice->SetTextureStageState( 2, D3DTSS_ALPHAOP,	AlphaOp );
	}

}

static DWORD         OldBumpEnvMat00TSS1   = 0;
static DWORD         OldBumpEnvMat01TSS1   = 0;
static DWORD         OldBumpEnvMat10TSS1   = 0;
static DWORD         OldBumpEnvMat11TSS1   = 0;
static DWORD         OldBumpEnvLScaleTSS1  = 0;
static DWORD         OldBumpEnvLOffsetTSS1 = 0;

__inline DWORD F2DW( FLOAT f ) { return *((DWORD*)&f); }
/* following code doesn't work here ...
__inline DWORD F2DW(float f)
{
   DWORD            retval = 0;

   _asm {
	fld			f				; load float into stack
	lea			eax, [retval]	; Load effective address to register
	fistp		dword ptr[eax]
   }

   return retval;
}/**/

void D3DSetTextureStageState1BM (float BumpEnvMat00TSS1, float BumpEnvMat01TSS1, float BumpEnvMat10TSS1, float BumpEnvMat11TSS1, float BumpEnvLScaleTSS1, float BumpEnvLOffsetTSS1)
{
	DWORD dwBumpEnvMat00TSS1   = F2DW(BumpEnvMat00TSS1);
	DWORD dwBumpEnvMat01TSS1   = F2DW(BumpEnvMat01TSS1);
	DWORD dwBumpEnvMat10TSS1   = F2DW(BumpEnvMat10TSS1);
	DWORD dwBumpEnvMat11TSS1   = F2DW(BumpEnvMat11TSS1);
	DWORD dwBumpEnvLScaleTSS1  = F2DW(BumpEnvLScaleTSS1);
	DWORD dwBumpEnvLOffsetTSS1 = F2DW(BumpEnvLOffsetTSS1);

	if (   (OldBumpEnvMat00TSS1 == dwBumpEnvMat00TSS1)
		&& (OldBumpEnvMat01TSS1 == dwBumpEnvMat01TSS1)
		&& (OldBumpEnvMat10TSS1 == dwBumpEnvMat10TSS1)
		&& (OldBumpEnvMat11TSS1 == dwBumpEnvMat11TSS1)
		&& (OldBumpEnvLScaleTSS1 == dwBumpEnvLScaleTSS1)
		&& (OldBumpEnvLOffsetTSS1 == dwBumpEnvLOffsetTSS1) )
		 return;

	if ( (OldBumpEnvMat00TSS1 != dwBumpEnvMat00TSS1) )
	{
		OldBumpEnvMat00TSS1 = dwBumpEnvMat00TSS1;
		AppInfo.lpD3DDevice->SetTextureStageState( 1, D3DTSS_BUMPENVMAT00, dwBumpEnvMat00TSS1 );
	}

	if ( (OldBumpEnvMat01TSS1 != dwBumpEnvMat01TSS1) )
	{
		OldBumpEnvMat01TSS1 = dwBumpEnvMat01TSS1;
		AppInfo.lpD3DDevice->SetTextureStageState( 1, D3DTSS_BUMPENVMAT01, dwBumpEnvMat01TSS1 );
	}

	if ( (OldBumpEnvMat10TSS1 != dwBumpEnvMat10TSS1) )
	{
		OldBumpEnvMat10TSS1 = dwBumpEnvMat10TSS1;
		AppInfo.lpD3DDevice->SetTextureStageState( 1, D3DTSS_BUMPENVMAT10, dwBumpEnvMat10TSS1 );
	}

	if ( (OldBumpEnvMat11TSS1 != dwBumpEnvMat11TSS1) )
	{
		OldBumpEnvMat11TSS1 = dwBumpEnvMat11TSS1;
		AppInfo.lpD3DDevice->SetTextureStageState( 1, D3DTSS_BUMPENVMAT11, dwBumpEnvMat11TSS1 );
	}

	// If you set the bump mapping operation to include luminance (D3DTOP_BUMPENVMAPLUMINANCE),
	// you must set the luminance controls. The luminance controls configure how the system computes
	// luminance before modulating the color from the texture in the next stage.
	// (For details, see Bump Mapping Formulas.)

	if ( (OldBumpEnvLScaleTSS1 != dwBumpEnvLScaleTSS1) )
	{
		OldBumpEnvLScaleTSS1 = dwBumpEnvLScaleTSS1;
		AppInfo.lpD3DDevice->SetTextureStageState( 1, D3DTSS_BUMPENVLSCALE, dwBumpEnvLScaleTSS1 );
	}

	if ( (OldBumpEnvLOffsetTSS1 != dwBumpEnvLOffsetTSS1) )
	{
		OldBumpEnvLOffsetTSS1 = dwBumpEnvLOffsetTSS1;
		AppInfo.lpD3DDevice->SetTextureStageState( 1, D3DTSS_BUMPENVLOFFSET, dwBumpEnvLOffsetTSS1 );
	}
}    




