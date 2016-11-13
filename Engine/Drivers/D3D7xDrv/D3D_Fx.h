/****************************************************************************************/
/*  D3D_Fx.h                                                                            */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: D3D renderstate wrapper                                                */
/*                                                                                      */
/*  Edit History:                                                                       */
/*  04/08/2003 Wendell Buckner                                                          */
/*   BUMPMAPPING                                                                        */
/*  01/28/2003 Wendell Buckner                                                          */
/*   Moving ALL triangles into another buffer is resulting in minimum speed increase    */
/*    (+1FPS increase)...	                                                            */
/*  01/18/2003 Wendell Buckner                                                          */
/*   Optimization from GeForce_Optimization2.doc                                        */
/*   9.	Do not duplicate render state commands.  Worse is useless renderstates.  Do not */
/*      set a renderstate unless it is needed.                                          */
/*  01/13/2003 Wendell Buckner                                                          */
/*    Optimization from GeForce_Optimization2.doc                                       */
/*   9.	Do not duplicate render state commands.  Worse is useless renderstates.  Do not */
/*      set a renderstate unless it is needed.                                          */
/*  07/16/2000 Wendell Buckner                                                          */
/*   Convert to Directx7...                                                             */
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
/*  Genesis3D Version 1.1 released November 15, 1999                                    */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved                            */
/*                                                                                      */
/****************************************************************************************/
#ifndef D3D_FX_H
#define D3D_FX_H

#include <windows.h>
#include <ddraw.h>
#include <d3d.h>

#include "D3D_Main.h"
#include "DCommon.h"

void D3DSetTexHandle(D3DTEXTUREHANDLE TexHandle);

/*   07/16/2000 Wendell Buckner
/*    Convert to Directx7...
void D3DSetTexture(int32 Stage, LPDIRECT3DTEXTURE2   Texture);  */
void D3DSetTexture(int32 Stage, LPDIRECTDRAWSURFACE7 Texture);

// changed QD Shadows
void D3DDrawShadow(geFloat r, geFloat g, geFloat b, geFloat a);
// end change

void D3DTexturedPolyOld(void *Pnts, int32 NumPoints);
void D3DTexturedPoly(void *Pnts, int32 NumPoints);

/* 01/28/2003 Wendell Buckner
   Moving ALL triangles into another buffer is resulting in minimum speed increase (+1FPS increase)...	*/
void D3DTexturedPoly3(void *Pnts, int32 NumPoints);

void D3DBilinearFilter(D3DTEXTUREFILTER Min, D3DTEXTUREFILTER Mag);
void D3DBlendEnable(BOOL Enable);

void D3DBlendFunc (D3DBLEND SFunc, D3DBLEND DFunc);

void D3DZWriteEnable (BOOL Enable);
void D3DZFunc (D3DCMPFUNC Func);
void D3DZEnable(BOOL Enable);

// changed QD Shadows
void D3DStencilFunc (D3DCMPFUNC Func);
void D3DStencilEnable(BOOL Enable);
// end change

void D3DTexWrap(DWORD Stage, BOOL Wrap);

void D3DPolygonMode (D3DFILLMODE Mode);

void D3DViewport (int32 x, int32 y, int32 width, int32 height);
void D3DDepthRange (float zNear, float zFar);

/* 01/13/2003 Wendell Buckner
	Optimization from GeForce_Optimization2.doc
9.	Do not duplicate render state commands.  Worse is useless renderstates.  Do not set a renderstate unless it is needed. */
void D3DFogEnable (BOOL Enable, DWORD Color);

/* 04/08/2003 Wendell Buckner
	BUMPMAPPING
/* 01/18/2003 Wendell Buckner
	Optimization from GeForce_Optimization2.doc
9.	Do not duplicate render state commands.  Worse is useless renderstates.  Do not set a renderstate unless it is needed.
void D3DSetTextureStageState0(unsigned long ColorArg1, unsigned long ColorArg2,D3DTEXTUREOP ColorOp, unsigned long AlphaArg1, unsigned long AlphaArg2, D3DTEXTUREOP AlphaOp);
void D3DSetTextureStageState1(unsigned long ColorArg1, unsigned long ColorArg2,D3DTEXTUREOP ColorOp, unsigned long AlphaArg1, unsigned long AlphaArg2, D3DTEXTUREOP AlphaOp);*/
void D3DSetTextureStageState0(unsigned long ColorArg1, unsigned long ColorArg2,D3DTEXTUREOP ColorOp, unsigned long AlphaArg1, unsigned long AlphaArg2, D3DTEXTUREOP AlphaOp,unsigned long TexCoordTSS0);
void D3DSetTextureStageState1(unsigned long ColorArg1, unsigned long ColorArg2,D3DTEXTUREOP ColorOp, unsigned long AlphaArg1, unsigned long AlphaArg2, D3DTEXTUREOP AlphaOp,unsigned long TexCoordTSS1);
void D3DSetTextureStageState2(unsigned long ColorArg1, unsigned long ColorArg2,D3DTEXTUREOP ColorOp, unsigned long AlphaArg1, unsigned long AlphaArg2, D3DTEXTUREOP AlphaOp,unsigned long TexCoordTSS2);
void D3DSetTextureStageState1BM (float BumpEnvMat00TSS1, float BumpEnvMat01TSS1, float BumpEnvMat10TSS1, float BumpEnvMat11TSS1, float BumpEnvLScaleTSS1, float BumpEnvLOffsetTSS1);

/* 12/27/2003 Wendell Buckner
	CONFIG DRIVER - Allow full-scene anti-aliasing  */
void D3DFullSceneAntiAliasing ( BOOL Enable );

#endif
