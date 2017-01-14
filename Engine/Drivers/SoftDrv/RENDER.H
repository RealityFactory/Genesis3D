/****************************************************************************************/
/*  render.h                                                                            */
/*                                                                                      */
/*  Author:       John Pollard, Ken Baird                                               */
/*  Description:  header for render.c                                                   */
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
#ifndef RENDER_H
#define RENDER_H

#include <Windows.h>

#include "DCommon.h"
#include "Span.h"
#include "Register.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int32			Fixed16;

#define					ZBUFFER_PREC	(65536.0f * 65536.0f)
//#define				ZBUFFER_PREC	(32768.0f * 65536.0f)

extern int32				SpanMode;
extern int32				PolyMode;
extern BOOL				PolyIsTrans;

extern BOOL				PolyVisible;
extern int32				ActuallVisible;
extern int32				NumPixels;

extern int32				SMIN, SMAX;

extern uint8				GMipLevel;				// Miplevel passed by the latest rendering routine
extern int32			GMipLevel4, GMipLevel20, GMipLevel4_8;
extern int32				GLMapAdd;
extern DRV_LInfo		*GLInfo;
extern DRV_Bitmap		*GBitmap;
extern geRDriver_THandle	*GTexture;
extern uint16				*pScrPtr16bpp;

extern int32			GLightWidth;
extern uint8			*GLightData;

extern int32			DeltaX, Remaining, N_Runs, PixelCount;
extern uint16			*Source, *Dest;
extern int32			U2, V2, StepU, StepV;
extern float			UDivZ, VDivZ, Zi, Z, Dx, Dy, PixelEnd;
extern int32			TxWhole, TyWhole, TxFract, TyFract;
extern float			UDivZnStepX, VDivZnStepX, ZinStepX;
extern int32			Junk[2];

extern float			Real16 ;	
extern float			Real65536;

extern int32			U, V;

extern int32				GW, GWMask;
extern int32				GH, GHMask;
extern uint8				*GBitPtr;
extern uint16				*GBitPtr16;

// 16bit zbuffer
extern uint16				*ZBuffer;

extern float			UDivZStepX;
extern float			UDivZStepY;
extern float			VDivZStepX;
extern float			VDivZStepY;

extern float			UDivZOrigin;
extern float			VDivZOrigin;
extern float			UDivZ16StepX, VDivZ16StepX, Zi16StepX;
extern float			UDivZ32StepX, VDivZ32StepX, Zi32StepX;

extern float			ZiStepX;
extern float			ZiStepY;
extern float			ZiOrigin;

extern Fixed16			UAdjust;
extern Fixed16			VAdjust;
extern Fixed16			UAdjustL;
extern Fixed16			VAdjustL;
extern Fixed16			UAdjust1;
extern Fixed16			VAdjust1;
extern Fixed16			UAdjust2;
extern Fixed16			VAdjust2;

extern Fixed16			MaxU;
extern Fixed16			MaxV;

#define ZBUFFER_SHIFT				21
#define ZBUFFER_RIGHT_SHIFT			10

// 16bit zbuffer
extern uint16			*ZBuffer;

#ifdef __cplusplus
}
#endif

BOOL RenderInit(DRV_Window *Window);
BOOL RenderShutdown(void);
void ClearZBuffer(DRV_Window *Window);

// Render funtions
BOOL DRIVERCC RenderGouraudPoly(DRV_TLVertex *Pnts, int32 NumPoints, uint32 Flags);
BOOL DRIVERCC RenderWorldPoly(DRV_TLVertex *Pnts, int32 NumPoints, geRDriver_THandle *THandle, DRV_TexInfo *TexInfo, DRV_LInfo *LInfo, uint32 Flags);
BOOL DRIVERCC RenderMiscTexturePoly(DRV_TLVertex *Pnts, int32 NumPoints, geRDriver_THandle *THandle, uint32 Flags);
BOOL DRIVERCC DrawDecal(geRDriver_THandle *THandle, RECT *SRect, int32 x, int32 y);

#endif
