/****************************************************************************************/
/*  render.c                                                                            */
/*                                                                                      */
/*  Author:       John Pollard, Ken Baird                                               */
/*  Description:  Poly raster calls and data structures                                 */
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

/*
Code fragments from Chris Hecker's texture mapping articles used with
permission.  http://www.d6.com/users/checker 
*/

#include <Windows.h>
#include <Stdio.h>
#include <Assert.h>
#include <Math.h>

#include "SoftDrv.h"
#include "DCommon.h"
#include "Render.h"
#include "Span.h"
#include "Scene.h"
#include "Sal.h"
#include "Register.h"
#include "3dnowspan.h"
#include "x86span565.h"
#include "x86span555.h"
#include "XForm3d.h"
#include "Vec3d.h"
#include "dmodes.h"




extern 		CPUInfo			ProcessorInfo;

typedef struct EdgeAsmWorldTag
{
	int		X, y, Height;
	float	x, r, g, b;
	float	xstep, rstep, gstep, bstep;
	U32		R, G, B, X2, pad;	//x2 for clip check
} EdgeAsmWorld;

typedef struct EdgeAsmTag
{
	int		X, y, Height;
	float	x, u, v, z, r, g, b;
	float	xstep, ustep, vstep, zstep;
	float	rstep, gstep, bstep;
} EdgeAsm;

typedef struct EdgeAsmFPUTag
{
	int		X, y, Height;
	float	x, u, v, z, r, g, b;
	float	xstep, ustep, vstep, zstep;
	float	rstep, gstep, bstep;
	U32		R, G, B;
} EdgeAsmFPU;

//globals used in span rendering routines
__int64		Bucket, Bucket2, Bucket3, Magic, Red, Green;
uint32		Blue, UMask, VShift, VMask, widTemp, TDest, TempPix;

float		BlueMask	=0x001f001f;
float		GreenMask	=0x07e007e0;
float		MiniRedMask	=0xf800f800;
double		RedMask		=0xf800f800;
float		GreenMask2	=0x03e003e0;
float		MiniRedMask2=0x7c007c00;
double		RedMask2	=0x7c007c00;


S32				SpanMode;
S32				PolyMode;
BOOL			PolyIsTrans;
BOOL			bStipple;

BOOL			PolyVisible;
S32				ActuallVisible;
S32				NumPixels;

S32				SMIN, SMAX;
U8				GMipLevel;				// Miplevel passed by the latest rendering routine
int32			GMipLevel4, GMipLevel20, GMipLevel4_8;
S32				GLMapAdd;

DRV_LInfo			*GLInfo;
DRV_Bitmap			*GBitmap;
geRDriver_THandle	*GTexture;

U16				*pScrPtr16bpp;
U32				*pScrPtr32bpp;
extern			U16	*pZBufferPtr=0;

int32			GLightWidth;
uint8			*GLightData;

int32			DeltaX, Remaining, N_Runs, PixelCount;
uint16			*Source, *Dest;
int32			U2, V2, StepU, StepV;
float			UDivZ, VDivZ, Zi, Z, Dx, Dy, PixelEnd;
int32			TxWhole, TyWhole, TxFract, TyFract;
float			UDivZnStepX, VDivZnStepX, ZinStepX;
int32			Junk[2];

float			Real16 = 16.0f;	
float			Real65536 = (float)65536;

int32			U, V;

S32				GW, GWMask, GWMaskShifted;
S32				GH, GHMask, GHMaskShifted, GHMaskShifted16;
__int64			QNegAlpha	=0x00ff00ff00ff00ff;
U32				SolidColor	=0xffffffff;
U8				*GBitPtr;
U8				*GBitPtrHalf;
U8				*ABitPtr;
U16				*GBitPtr16;
U8				*TexPal;
U8				*ATexPal;
__int64			UV16=0, UVLeft=0, UVLeftW=0;
__int64			UVDivZ16StepX=0, ARL=0, GBL=0;
__int64			WrapMask=0;
__int64			UVLeft2=0;
__int64			UVDivZOrigin=0, UVR=0;
__int64			ZIR=0, RGBADelta=0;
__int64			UVDivZStepX=0, UVDivZStepY;
__int64			Zero=0, UVZ=0, UVAdjustL=0;
__int64			GLMapMulUV=0, UVL16=0, UV162=0;
__int64			LMapMask8=0x000000ff000000ff;
__int64			UVAdjust=0, UVAdjust2=0;
__int64			QGMip20=0, QGMip4_8=0;
__int64			QDibCan=0, QZCan=0, VertAlpha=0;
__int64			QDibOrCan=0, QZOrCan=0;
__int64			QZVal=0, QZDelta=0, UV16V=0;
__int64			QZOut=0, QDibOut=0, QShiftV=0;
__int64			QZVal32_0=0, QZVal32_1=0;
float			QFixedScaleLUT[34]={ 0.0f, 0.0f, 65536.0f, 65536.0f, 32768.0f, 32768.0f,
									21845.3333f, 21845.3333f, 16384.0f, 16384.0f,
									13107.2f, 13107.2f, 10922.6666f, 10922.6666f,
									9362.2857f, 9362.2857f, 8192.0f, 8192.0f,
									7281.7777f, 7281.7777f, 6553.6f, 6553.6f, 
									5957.8181f, 5957.8181f, 5461.3333f, 5461.3333f,
									5041.2307f, 5041.2307f, 4681.1428f, 4681.1428f,
									4369.0666f, 4369.0666f, 4096.0f, 4096.0f };
int				SCan[16];	//for zbuffer fake ptr
float			GLMapMulU;	//lightscale
float			GLMapMulV;	//lightscale

// 16bit zbuffer
U16				*ZBuffer;
uint16			*ZBufLine;

// Gradients info
float	UDivZStepX;
float	UDivZStepY;
float	VDivZStepX;
float	VDivZStepY;

float	UDivZOrigin;
float	VDivZOrigin;
float	UDivZ16StepX, VDivZ16StepX, Zi16StepX;
float	UDivZ32StepX, VDivZ32StepX, Zi32StepX;

float	ZiStepX;
float	ZiStepY;
float	ZiOrigin;

GInfo *GlobalInfo;

Fixed16	UAdjust;
Fixed16 VAdjust;
Fixed16	UAdjustL;
Fixed16 VAdjustL;
Fixed16	UAdjust1;
Fixed16 VAdjust1;
Fixed16	UAdjust2;
Fixed16 VAdjust2;

Fixed16	MaxU;
Fixed16	MaxV;

//magic numbers for float to int conversion
//only works on values between -4194304 and 4194303
float FistMagic=12582912.0f;
float FistTruncate=0.5f;


void TmapTriangle_32(DRV_TLVertex *verts);
void TmapTriangle_16(DRV_TLVertex *verts);

typedef void (* MapperPtr) (EdgeAsm *, EdgeAsm *);

static MapperPtr	CurMapper;

typedef void (* MapperPtrFPU) (EdgeAsmFPU *, EdgeAsmFPU *);

static MapperPtrFPU	CurMapperFPU;

typedef void (* MapperPtrWorld) (int32 x1, int32 x2, int32 y, float r1, float g1, float b1, float r2, float g2, float b2);

static MapperPtrWorld	CurMapperWorld;

typedef void (* MapperPtrWorldFPU) (int32 x1, int32 x2, int32 y, int32 r1, int32 g1, int32 b1, int32 r2, int32 g2, int32 b2);

static MapperPtrWorldFPU	CurMapperWorldFPU;

static	geVec3d		TVectU, TVectV;
static	geVec3d		TVectUL, TVectVL;

static void	CalcGradients(S32 MipLevel, float DrawScaleU, float DrawScaleV)
{
	float		MipScale;
	geVec3d		SOrigin;
	float		t, dsui, dsvi;
	Fixed16		UAdjust1L, VAdjust1L;
	int			GW_log2;
	geVec3d		Temp	=GlobalInfo->Pov;
	float		distinv, ZScaleInv;

	distinv		=1.0f / (GlobalInfo->ZScale * (GlobalInfo->PlaneDist - geVec3d_DotProduct(&Temp, &GlobalInfo->PlaneNormal)));
	ZScaleInv	=1.0f / GlobalInfo->ZScale;
	dsui		=1.0f / DrawScaleU;
	dsvi		=1.0f / DrawScaleV;

	MipScale	=(1.0f / (float)(1 << MipLevel));

	geXForm3d_Rotate(&GlobalInfo->CXForm, &GlobalInfo->VecU, &TVectUL);
	geXForm3d_Rotate(&GlobalInfo->CXForm, &GlobalInfo->VecV, &TVectVL);
	geVec3d_Scale(&TVectUL, dsui, &TVectU);
	geVec3d_Scale(&TVectVL, dsvi, &TVectV);

	t			=GlobalInfo->XScaleInv * ZScaleInv * MipScale;

	UDivZStepX	=TVectU.X * t;
	VDivZStepX	=TVectV.X * t;

	t			=GlobalInfo->YScaleInv * ZScaleInv * MipScale;

	UDivZStepY	=-TVectU.Y * t;
	VDivZStepY	=-TVectV.Y * t;

	GLMapMulU	=DrawScaleU;
	GLMapMulV	=DrawScaleV;

	*((float *)(&GLMapMulUV))		=DrawScaleV;
	*(((float *)((&GLMapMulUV)))+1)	=DrawScaleU;

	UDivZOrigin	=TVectU.Z * ZScaleInv * MipScale
		- GlobalInfo->XCenter * UDivZStepX
		- GlobalInfo->YCenter * UDivZStepY;

	VDivZOrigin	=TVectV.Z * ZScaleInv * MipScale
		- GlobalInfo->XCenter * VDivZStepX
		- GlobalInfo->YCenter * VDivZStepY;

	geVec3d_Scale(&GlobalInfo->CPov, MipScale, &SOrigin);

	t			=65536.0f * MipScale;
	UAdjust1	=((Fixed16)(geVec3d_DotProduct(&SOrigin, &TVectU) * 65536.0f + 0.5f));
	UAdjust1L	=((Fixed16)(geVec3d_DotProduct(&SOrigin, &TVectUL) * 65536.0f + 0.5f));
	UAdjust2	=((GlobalInfo->TexMinsX << 16) >> MipLevel);

	VAdjust1	=((Fixed16)(geVec3d_DotProduct(&SOrigin, &TVectV) * 65536.0f + 0.5f));
	VAdjust1L	=((Fixed16)(geVec3d_DotProduct(&SOrigin, &TVectVL) * 65536.0f + 0.5f));
	VAdjust2	=((GlobalInfo->TexMinsY << 16) >> MipLevel);

	UAdjustL	=UAdjust1L - UAdjust2;
	VAdjustL	=VAdjust1L - VAdjust2;

	UAdjust2	=(int)(((float)UAdjust2) * dsui);
	VAdjust2	=(int)(((float)VAdjust2) * dsvi);

	*((int *)(&UVAdjustL))		=VAdjustL;
	*(((int *)((&UVAdjustL)))+1)=UAdjustL;

	UAdjust		=UAdjust1 - UAdjust2;
	VAdjust		=VAdjust1 - VAdjust2;

	*((int *)(&UVAdjust))		=VAdjust;
	*(((int *)((&UVAdjust)))+1)	=UAdjust;

	UAdjust1	+=(Fixed16)(GlobalInfo->TexShiftX * t);
	VAdjust1	+=(Fixed16)(GlobalInfo->TexShiftY * t);
	UAdjust2	+=(Fixed16)(GlobalInfo->TexShiftX * t);
	VAdjust2	+=(Fixed16)(GlobalInfo->TexShiftY * t);

	*((int *)(&UVAdjust2))		=VAdjust2;
	*(((int *)((&UVAdjust2)))+1)=UAdjust2;

	MaxU		=(((int)((GlobalInfo->TexWidth << 16) * dsui)) >> MipLevel);
	MaxV		=(((int)((GlobalInfo->TexHeight << 16) * dsvi)) >> MipLevel);

	*((int *)(&WrapMask))		=GWMask<<16;
	*(((int *)((&WrapMask)))+1)	=GWMask<<16;

	//find log2 of the texture width
	for(GW_log2=1;((1<<GW_log2) < GW); GW_log2++);

	VShift			=GW_log2;
	GHMaskShifted	=GHMask << (GW_log2);
	GHMaskShifted16	=GHMaskShifted << 16;

	QShiftV	=(__int64)(16 - GW_log2);


	ZiStepX		=GlobalInfo->RPlaneNormal.X * GlobalInfo->XScaleInv * distinv;
	ZiStepY		=-GlobalInfo->RPlaneNormal.Y * GlobalInfo->YScaleInv * distinv;
	ZiOrigin	=GlobalInfo->RPlaneNormal.Z * distinv -
					GlobalInfo->XCenter * ZiStepX - GlobalInfo->YCenter * ZiStepY;

	// Get 16 step Gradients
	UDivZ16StepX	=UDivZStepX * 16.0f;
	VDivZ16StepX	=VDivZStepX * 16.0f;
	Zi16StepX		=ZiStepX * 16.0f;

	*((U32 *)(&QZDelta))		=(U32)(ZiStepX * -ZBUFFER_PREC);
	*(((U32 *)((&QZDelta)))+1)	=(U32)(ZiStepX * -ZBUFFER_PREC);

	*(((float *)((&UVDivZ16StepX)))+1)	=UDivZ16StepX;
	*((float *)(&UVDivZ16StepX))		=VDivZ16StepX;

	*(((float *)((&UVDivZStepX)))+1)	=UDivZStepX;
	*((float *)(&UVDivZStepX))			=VDivZStepX;

	*(((float *)((&UVDivZStepY)))+1)	=UDivZStepY;
	*((float *)(&UVDivZStepY))			=VDivZStepY;

	*(((float *)((&UVDivZOrigin)))+1)	=UDivZOrigin;
	*((float *)(&UVDivZOrigin))			=VDivZOrigin;
}


BOOL DRIVERCC RenderGouraudPoly(DRV_TLVertex *Pnts, S32 NumPoints, U32 Flags)
{
	S32				i;
	DRV_TLVertex	TriVerts[3];

	assert(NumPoints > 2);

	if(!bActive)
	{
		return	GE_TRUE;
	}

	if(ProcessorInfo.Has3DNow)
	{
		if(Flags & DRV_RENDER_NO_ZMASK)
		{
			if(Flags & DRV_RENDER_NO_ZWRITE)
			{
				CurMapper	=DrawScanLineGouraudNoZSolid_Asm3DNow;
			}
			else
			{
				CurMapper	=DrawScanLineGouraudNoZBufferZWriteSolid_Asm3DNow;
			}
		}
		else
		{
			if(Flags & DRV_RENDER_NO_ZWRITE)
			{
				CurMapper	=DrawScanLineGouraudZBufferNoZWriteSolid_Asm3DNow;
			}
			else
			{
				CurMapper	=DrawScanLineGouraudZBufferSolid_Asm3DNow;
			}
		}

		if(NumPoints==3)
		{
			TriVerts[2]	=Pnts[0];
			TriVerts[1]	=Pnts[1];
			TriVerts[0]	=Pnts[2];

			TmapTriangle_32(TriVerts);
		}
		else
		{
			for(i=0;i < NumPoints-2;i++)
			{
				TriVerts[0]	=Pnts[0];
				TriVerts[1]	=Pnts[1+i];
				TriVerts[2]	=Pnts[2+i];

				TmapTriangle_32(TriVerts);
			}
		}
		return GE_TRUE;
	}
	else
	{
		if(ClientWindow.G_mask == 0x03e0)
		{
			if(Flags & DRV_RENDER_NO_ZMASK)
			{
				if(Flags & DRV_RENDER_NO_ZWRITE)
				{
					CurMapperFPU	=DrawScanLineGouraudNoZSolid_Asm555X86FPU;
				}
				else
				{
					CurMapperFPU	=DrawScanLineGouraudNoZBufferZWriteSolid_Asm555X86FPU;
				}
			}
			else
			{
				if(Flags & DRV_RENDER_NO_ZWRITE)
				{
					CurMapperFPU	=DrawScanLineGouraudZBufferNoZWriteSolid_Asm555X86FPU;
				}
				else
				{
					CurMapperFPU	=DrawScanLineGouraudZBufferSolid_Asm555X86FPU;
				}
			}
		}
		else
		{
			if(Flags & DRV_RENDER_NO_ZMASK)
			{
				if(Flags & DRV_RENDER_NO_ZWRITE)
				{
					CurMapperFPU	=DrawScanLineGouraudNoZSolid_AsmX86FPU;
				}
				else
				{
					CurMapperFPU	=DrawScanLineGouraudNoZBufferZWriteSolid_AsmX86FPU;
				}
			}
			else
			{
				if(Flags & DRV_RENDER_NO_ZWRITE)
				{
					CurMapperFPU	=DrawScanLineGouraudZBufferNoZWriteSolid_AsmX86FPU;
				}
				else
				{
					CurMapperFPU	=DrawScanLineGouraudZBufferSolid_AsmX86FPU;
				}
			}
		}
		if(NumPoints==3)
		{
			TriVerts[2]	=Pnts[0];
			TriVerts[1]	=Pnts[1];
			TriVerts[0]	=Pnts[2];

			TmapTriangle_16(TriVerts);
		}
		else
		{
			for(i=0;i < NumPoints-2;i++)
			{
				TriVerts[0]	=Pnts[0];
				TriVerts[1]	=Pnts[1+i];
				TriVerts[2]	=Pnts[2+i];

				TmapTriangle_16(TriVerts);
			}
		}
		return GE_TRUE;
	}
}

void StepWorld(EdgeAsmWorld *edge)
{
	int	r,g,b;
	_asm
	{
		push	ecx
		push	ebx
		push	esi
		push	edi
		push	edx

		mov     eax,edge
		fld		[eax]EdgeAsmWorld.x		; x
		fadd    [eax]EdgeAsmWorld.xstep	; xd
		fld		[eax]EdgeAsmWorld.r		; r   xd
		fadd	[eax]EdgeAsmWorld.rstep	; rd  xd
		fld		[eax]EdgeAsmWorld.g		; g   rd  xd
		fadd	[eax]EdgeAsmWorld.gstep	; gd  rd  xd
		fld		[eax]EdgeAsmWorld.b		; b   gd  rd  xd
		fadd	[eax]EdgeAsmWorld.bstep	; bd  gd  rd  xd
		fxch	st(2)					; rd  gd  bd  xd
		fst		[eax]EdgeAsmWorld.r
		fadd	[FistMagic]			; rdk gd  bd  xd
		fxch	st(2)				; bd  gd  rdk xd
		fst		[eax]EdgeAsmWorld.b
		fadd	[FistMagic]			; bdk gd  rdk xd
		fxch	st(1)				; gd  bdk rdk xd
		fst		[eax]EdgeAsmWorld.g
		fadd	[FistMagic]			; gdk bdk rdk xd
		fxch	st(2)				; rdk bdk gdk xd
		fstp	[r]					; bdk gdk xd
		fstp	[b]					; gdk xd
		fstp	[g]					; xd

		mov		edx,[FistMagic]
		mov		ebx,[r]

		mov		ecx,[g]
		sub		ebx,edx
		
		mov		esi,[b]
		sub		ecx,edx

		sub		esi,edx
		and		ebx,0fch

		fst		[eax]EdgeAsmWorld.x		; xd

		and		ecx,0fch
		and		esi,0fch

		fsub    [FistTruncate]      ; xdt

		mov		[eax]EdgeAsmWorld.R,ebx
		mov		[eax]EdgeAsmWorld.G,ecx

		fadd    [FistMagic]         ; xnt

		mov		[eax]EdgeAsmWorld.B,esi

		fstp    [eax]EdgeAsmWorld.X

		sub     [eax]EdgeAsmWorld.X,edx

		pop		edx
		pop		edi
		pop		esi
		pop		ebx
		pop		ecx
	}
	edge->y++;
	edge->Height--;
}

void setup_edgeWorld(EdgeAsmWorld *edge, DRV_TLVertex *verts, int top, int end)
{
	int		ey,ty;
	float	yd;

	_asm
	{
		push	ebx
		mov		eax,end
		mov     edx,top

		shl     eax,2
		mov     ebx,verts

		shl     edx,2
		lea		eax,[8*eax+eax]
		lea		edx,[8*edx+edx]

		fld		[ebx+eax]DRV_TLVertex.y		; endy
		fld     [ebx+edx]DRV_TLVertex.y		; topy endy
		fsub    st(1),st	                ; topy yd
		fld     [ebx+eax]DRV_TLVertex.y		; endy topy yd
		fadd    [FistTruncate]
		fadd    [FistMagic]					; eyi  topy yd
		fxch    st(2)                       ; yd   topy eyi
		fstp    [yd]						; topy eyi
		fadd    [FistTruncate]
		fadd    [FistMagic]	                ; tyi  eyi
		fxch    st(1)                       ; eyi  tyi
		fstp    [ey]                        ; tyi
		fld     [ebx+edx]DRV_TLVertex.x     ; x    tyi
		fxch    st(1)                       ; tyi  x
		fstp    [ty]                        ; x
		fadd    [FistMagic]                 ; xi

		mov     ebx,[FistMagic]
		mov     eax,edge

		sub     [ey],ebx

		fstp    [eax]EdgeAsm.X

		sub     [ty],ebx

		sub     [eax]EdgeAsm.X, ebx
		pop		ebx
	}

	edge->Height	=ey-ty;

//	yd		=(edge->Height >0)? 1.0f/edge->Height : 1;
	yd		=(edge->Height >0)? 1.0f/(verts[end].y - verts[top].y) : 1;
	edge->y	=ty;

	edge->x		=verts[top].x;
	edge->xstep	=(verts[end].x-edge->x)*yd;

	edge->r		=verts[top].r;
	edge->rstep	=(verts[end].r-edge->r)*yd;
	edge->R		=(int)(verts[top].r)&0xfc;

	edge->g		=verts[top].g;
	edge->gstep	=(verts[end].g-edge->g)*yd;
	edge->G		=(int)(verts[top].g)&0xfc;

	edge->b		=verts[top].b;
	edge->bstep	=(verts[end].b-edge->b)*yd;
	edge->B		=(int)(verts[top].b)&0xfc;
}

void AddSpanDraw(EdgeAsmWorld *pLeft, EdgeAsmWorld *pRight)
{
	int		x1, x2, y;
	BOOL	Dynamic;

	x1	=pLeft->X2;
	x2	=pRight->X2;
	y	=pLeft->y;

    if (!PolyVisible)
	{
		PolyVisible = 1;
		ActuallVisible++;
		
		if (GLInfo)
		{
			if(ProcessorInfo.Has3DNow)
			{
				Femms3DNow();

				SOFTDRV.SetupLightmap(GLInfo, &Dynamic);

				Femms3DNow();
			}
			else
			{
				SOFTDRV.SetupLightmap(GLInfo, &Dynamic);
			}

			GLightData = (uint8*)GLInfo->RGBLight[0];
		}
		else
		{
			GLightData	=NULL;
		}
	}
	if(PolyIsTrans)
	{
		if(!(y & 1))
		{
			return;
		}
	}

    NumPixels += x2 - x1 + 1;

	if(ProcessorInfo.Has3DNow)
	{
		switch(SpanMode)
		{
			case 0:
			{
				DrawSpan32_AsmLitZWrite3DNow(x1, x2, y);
				break;
			}
			case 1:
			{
				DrawSpan32_AsmGouraudZWrite3DNow(x1, x2, y, pLeft->r, pLeft->g, pLeft->b, pRight->r, pRight->g, pRight->b);
				break;
			}
			case 2:
			{
				DrawSpan32_AsmLitZBuffer3DNow(x1, x2, y);
				break;
			}
			case 3:
			{
				DrawSpan32_AsmGouraudZBuffer3DNow(x1, x2, y, pLeft->r, pLeft->g, pLeft->b, pRight->r, pRight->g, pRight->b);
				break;
			}
		}
	}
	else
	{
		pZBufferPtr	=ZBufLine;
		if(ClientWindow.G_mask == 0x03e0)
		{
			switch(SpanMode)
			{
				case 0:
				{
					DrawSpan16_AsmLitZWrite555X86FPU(x1, x2, y);
					break;
				}
				case 1:
				{
					DrawSpan16_AsmGouraudZWrite555X86FPU(x1, x2, y, pLeft->R, pLeft->G, pLeft->B, pRight->R, pRight->G, pRight->B);
					break;
				}
				case 2:
				{
					DrawSpan16_AsmLitZBuffer555X86FPU(x1, x2, y);
					break;
				}
				case 3:
				{
					DrawSpan16_AsmGouraudZBuffer555X86FPU(x1, x2, y, pLeft->R, pLeft->G, pLeft->B, pRight->R, pRight->G, pRight->B);
					break;
				}
			}
		}
		else
		{
			switch(SpanMode)
			{
				case 0:
				{
					DrawSpan16_AsmLitZWriteX86FPU(x1, x2, y);
					break;
				}
				case 1:
				{
					DrawSpan16_AsmGouraudZWriteX86FPU(x1, x2, y, pLeft->R, pLeft->G, pLeft->B, pRight->R, pRight->G, pRight->B);
					break;
				}
				case 2:
				{
					DrawSpan16_AsmLitZBufferX86FPU(x1, x2, y);
					break;
				}
				case 3:
				{
					DrawSpan16_AsmGouraudZBufferX86FPU(x1, x2, y, pLeft->R, pLeft->G, pLeft->B, pRight->R, pRight->G, pRight->B);
					break;
				}
			}
		}
	}
}

void AddSpan(EdgeAsmWorld *pLeft, EdgeAsmWorld *pRight)
{
	S32		y, x1, x2;
    SList	*LineStart	=NULL;
    SList	*Current;

	y	=pLeft->y;
	x1	=pLeft->X;
	pLeft->X2	=x1;
	x2	=pRight->X;
	pRight->X2	=x2;

	assert(y >=0 && y < MAXSCANLINES);

	if(RenderMode == RENDER_MODELS)
	{
		AddSpanDraw(pLeft, pRight);
		return;
	}

    Current = SMinMax[y].First;

	// Check to see if there are spans
    if (!Current) 
	{        
		SMinMax[y].First = NewSList();     // in the list yet...
        (SMinMax[y].First)->Last = NULL;
        (SMinMax[y].First)->Next = NULL;
        (SMinMax[y].First)->Min = x1;
        (SMinMax[y].First)->Max = x2;
    }
	else while (Current)
    {
		// See if this line is totally hidden...
        if(x1 >= Current->Min && x2 <= Current->Max)
		{
			return;	// Yep
		}

		//if falls before the entire min, max
		if(!LineStart)
		{
			if(Current == SMinMax[y].First)
			{
				if(x2 < Current->Min)
				{
					SList	*NewMinMax	=NewSList();
					NewMinMax->Next		=Current;
					NewMinMax->Last		=NULL;
					Current->Last		=NewMinMax;
					SMinMax[y].First	=NewMinMax;
					NewMinMax->Min		=x1;
					NewMinMax->Max		=x2;
					break;
				}
			}

            //if falls in the middle (but not touching)
            if(Current->Next)
			{
				if(x1 > Current->Max && x2 < (Current->Next)->Min)
				{
					SList	*NewMinMax		=NewSList();
					NewMinMax->Next			=Current->Next;
					NewMinMax->Last			=Current;
					(Current->Next)->Last	=NewMinMax;
					Current->Next			=NewMinMax;
					NewMinMax->Min			=x1;
					NewMinMax->Max			=x2;
					break;
				}
			}

            //if it falls to the right of all spans
            if(!Current->Next)
			{
				if(x1 > Current->Max)
				{
					SList	*NewMinMax	=NewSList();
					Current->Next		=NewMinMax;
					NewMinMax->Next		=NULL;
					NewMinMax->Last		=Current;
					NewMinMax->Min		=x1;
					NewMinMax->Max		=x2;
					break;
				}
			}
		}

        //if we have already started crossing spans, and we find out
        // that we are in front of a span, then we can bail out...
		if (LineStart != NULL)
			if (x2 < Current->Min)
				break;
		
        // We now know that we have not fallen into any empty holes.
        // We must now check to see what spans, we've crossed...

        // if split by a min/max
        if (x1 < Current->Min && x2 > Current->Max) 
		{
            pRight->X2		=Current->Min;
            Current->Min	=pLeft->X2	=x1;

			AddSpanDraw(pLeft, pRight);

            pLeft->X2		=x1			=Current->Max;
			Current->Max	=pRight->X2	=x2;

			if (LineStart!=NULL) 
				LineStart->Max	=x2;
            else
				LineStart = Current;

            Current = Current->Next;
			continue;
		}

		if (x1 <= Current->Max && x2 > Current->Max) 
		{
            pLeft->X2		=x1			=Current->Max;//+1;
			Current->Max	=pRight->X2	=x2;
            LineStart = Current;

			Current = Current->Next;

			continue;
		}
		if (x1 < Current->Min && x2 >= Current->Min) 
		{
            pRight->X2	=x2				=Current->Min;
            pLeft->X2	=Current->Min	=x1;
            if (LineStart!=NULL) 
				LineStart->Max = Current->Max;
			break;
		}
		Current = Current->Next;
    }

	AddSpanDraw(pLeft, pRight);
}

void SpanOutWorldTri(DRV_TLVertex *verts)
{
	int			Top, Middle, Bottom, MiddleForCompare;
	int			MiddleIsLeft, BottomForCompare, Height;
	float		Y0=verts[0].y, Y1=verts[1].y, Y2=verts[2].y;
	EdgeAsmWorld	TopToBottom, TopToMiddle, MiddleToBottom;
	EdgeAsmWorld	*pLeft, *pRight;

	// sort vertices in y
	if(Y0 < Y1)
	{
		if(Y2 < Y0)
		{
			Top		=2;
			Middle	=MiddleForCompare	=0;
			Bottom	=BottomForCompare	=1;
		}
		else
		{
			Top	=0;
			if(Y1 < Y2)
			{
				Middle	=MiddleForCompare	=1;
				Bottom	=BottomForCompare	=2;
			}
			else
			{
				Middle	=MiddleForCompare	=2;
				Bottom	=BottomForCompare	=1;
			}
		}
	}
	else
	{
		if(Y2 < Y1)
		{
			Top		=2;
			Middle	=MiddleForCompare	=1;
			Bottom	=BottomForCompare	=0;
		}
		else
		{
			Top	=1;
			if(Y0 < Y2)
			{
				Middle	=0;
				Bottom	=BottomForCompare	=2;
				MiddleForCompare			=3;
			}
			else
			{
				BottomForCompare			=3;
				Middle	=MiddleForCompare	=2;
				Bottom	=0;
			}
		}
	}

	setup_edgeWorld(&TopToBottom, verts, Top, Bottom);
	setup_edgeWorld(&TopToMiddle, verts, Top, Middle);
	setup_edgeWorld(&MiddleToBottom, verts, Middle, Bottom);

	if(BottomForCompare > MiddleForCompare)
	{
		MiddleIsLeft = 0;
		pLeft = &TopToBottom; pRight = &TopToMiddle;
	}
	else
	{
		MiddleIsLeft = 1;
		pLeft = &TopToMiddle; pRight = &TopToBottom;
	}

	Height	=TopToMiddle.Height;

	Dest		=(U16 *)(ClientWindow.Buffer + (pLeft->y * ClientWindow.PixelPitch));
	ZBufLine	=ZBuffer + (pLeft->y * ClientWindow.Width);
	
	pZBufferPtr	=ZBufLine;
	while(Height--)
	{
		if(bStipple)
		{
			if(pLeft->y & 1)
			{
				AddSpan(pLeft, pRight);
			}
		}
		else
		{
			AddSpan(pLeft, pRight);
		}
		StepWorld(pLeft);
		StepWorld(pRight);

		Dest		+=ClientWindow.PixelPitch / 2;
		ZBufLine	+=ClientWindow.Width;
		pZBufferPtr	=ZBufLine;
	}

	Height	=MiddleToBottom.Height;

	if(MiddleIsLeft)
	{
		pLeft	=&MiddleToBottom;
		pRight	=&TopToBottom;
	}
	else
	{
		pLeft	=&TopToBottom;
		pRight	=&MiddleToBottom;
	}
	
	while(Height--)
	{
		if(bStipple)
		{
			if(pLeft->y & 1)
			{
				AddSpan(pLeft, pRight);
			}
		}
		else
		{
			AddSpan(pLeft, pRight);
		}
		StepWorld(pLeft);
		StepWorld(pRight);

		Dest		+=ClientWindow.PixelPitch / 2;
		ZBufLine	+=ClientWindow.Width;
		pZBufferPtr	=ZBufLine;
	}
}

void SpanOutWorldTri3DNow(DRV_TLVertex *verts)
{
	int			Top, Middle, Bottom, MiddleForCompare;
	int			MiddleIsLeft, BottomForCompare, Height;
	float		Y0=verts[0].y, Y1=verts[1].y, Y2=verts[2].y;
	EdgeAsmWorld	TopToBottom, TopToMiddle, MiddleToBottom;
	EdgeAsmWorld	*pLeft, *pRight;

	// sort vertices in y
	if(Y0 < Y1)
	{
		if(Y2 < Y0)
		{
			Top		=2;
			Middle	=MiddleForCompare	=0;
			Bottom	=BottomForCompare	=1;
		}
		else
		{
			Top	=0;
			if(Y1 < Y2)
			{
				Middle	=MiddleForCompare	=1;
				Bottom	=BottomForCompare	=2;
			}
			else
			{
				Middle	=MiddleForCompare	=2;
				Bottom	=BottomForCompare	=1;
			}
		}
	}
	else
	{
		if(Y2 < Y1)
		{
			Top		=2;
			Middle	=MiddleForCompare	=1;
			Bottom	=BottomForCompare	=0;
		}
		else
		{
			Top	=1;
			if(Y0 < Y2)
			{
				Middle	=0;
				Bottom	=BottomForCompare	=2;
				MiddleForCompare			=3;
			}
			else
			{
				BottomForCompare			=3;
				Middle	=MiddleForCompare	=2;
				Bottom	=0;
			}
		}
	}

	setup_edgeWorld(&TopToBottom, verts, Top, Bottom);
	setup_edgeWorld(&TopToMiddle, verts, Top, Middle);
	setup_edgeWorld(&MiddleToBottom, verts, Middle, Bottom);

	Femms3DNow();

	if(BottomForCompare > MiddleForCompare)
	{
		MiddleIsLeft = 0;
		pLeft = &TopToBottom; pRight = &TopToMiddle;
	}
	else
	{
		MiddleIsLeft = 1;
		pLeft = &TopToMiddle; pRight = &TopToBottom;
	}

	Height	=TopToMiddle.Height;

	Dest		=(U16 *)(ClientWindow.Buffer + (pLeft->y * ClientWindow.PixelPitch));
	
	while(Height--)
	{
		AddSpan(pLeft, pRight);

		StepWorld3DNow(pLeft);
		StepWorld3DNow(pRight);

		Dest		+=ClientWindow.PixelPitch / 2;
	}

	Height	=MiddleToBottom.Height;

	if(MiddleIsLeft)
	{
		pLeft	=&MiddleToBottom;
		pRight	=&TopToBottom;
	}
	else
	{
		pLeft	=&TopToBottom;
		pRight	=&MiddleToBottom;
	}
	
	while(Height--)
	{
		AddSpan(pLeft, pRight);

		StepWorld3DNow(pLeft);
		StepWorld3DNow(pRight);

		Dest		+=ClientWindow.PixelPitch / 2;
	}
	Femms3DNow();
}

void SpanOutWorldTriAlpha(DRV_TLVertex *verts)
{
	int			Top, Middle, Bottom, MiddleForCompare;
	int			MiddleIsLeft, BottomForCompare, Height;
	float		Y0=verts[0].y, Y1=verts[1].y, Y2=verts[2].y;
	EdgeAsmWorld	TopToBottom, TopToMiddle, MiddleToBottom;
	EdgeAsmWorld	*pLeft, *pRight;

	// sort vertices in y
	if(Y0 < Y1)
	{
		if(Y2 < Y0)
		{
			Top		=2;
			Middle	=MiddleForCompare	=0;
			Bottom	=BottomForCompare	=1;
		}
		else
		{
			Top	=0;
			if(Y1 < Y2)
			{
				Middle	=MiddleForCompare	=1;
				Bottom	=BottomForCompare	=2;
			}
			else
			{
				Middle	=MiddleForCompare	=2;
				Bottom	=BottomForCompare	=1;
			}
		}
	}
	else
	{
		if(Y2 < Y1)
		{
			Top		=2;
			Middle	=MiddleForCompare	=1;
			Bottom	=BottomForCompare	=0;
		}
		else
		{
			Top	=1;
			if(Y0 < Y2)
			{
				Middle	=0;
				Bottom	=BottomForCompare	=2;
				MiddleForCompare			=3;
			}
			else
			{
				BottomForCompare			=3;
				Middle	=MiddleForCompare	=2;
				Bottom	=0;
			}
		}
	}

	setup_edgeWorld(&TopToBottom, verts, Top, Bottom);
	setup_edgeWorld(&TopToMiddle, verts, Top, Middle);
	setup_edgeWorld(&MiddleToBottom, verts, Middle, Bottom);

	if(BottomForCompare > MiddleForCompare)
	{
		MiddleIsLeft = 0;
		pLeft = &TopToBottom; pRight = &TopToMiddle;
	}
	else
	{
		MiddleIsLeft = 1;
		pLeft = &TopToMiddle; pRight = &TopToBottom;
	}

	Height	=TopToMiddle.Height;

	Dest		=(U16 *)(ClientWindow.Buffer + (pLeft->y * ClientWindow.PixelPitch));
	ZBufLine	=ZBuffer + (pLeft->y * ClientWindow.Width);
	
	pZBufferPtr	=ZBufLine;
	while(Height--)
	{
		if(bStipple)
		{
			if(pLeft->y & 1)
			{
				CurMapperWorldFPU(pLeft->X, pRight->X, pLeft->y
					, pLeft->R, pLeft->G, pLeft->B,
					pRight->R, pRight->G, pRight->B);
			}
		}
		else
		{
			CurMapperWorldFPU(pLeft->X, pRight->X, pLeft->y
				, pLeft->R, pLeft->G, pLeft->B,
				pRight->R, pRight->G, pRight->B);
		}
		StepWorld(pLeft);
		StepWorld(pRight);

		Dest		+=ClientWindow.PixelPitch / 2;
		ZBufLine	+=ClientWindow.Width;
		pZBufferPtr	=ZBufLine;
	}

	Height	=MiddleToBottom.Height;

	if(MiddleIsLeft)
	{
		pLeft	=&MiddleToBottom;
		pRight	=&TopToBottom;
	}
	else
	{
		pLeft	=&TopToBottom;
		pRight	=&MiddleToBottom;
	}
	
	while(Height--)
	{
		if(bStipple)
		{
			if(pLeft->y & 1)
			{
				CurMapperWorldFPU(pLeft->X, pRight->X, pLeft->y
					, pLeft->R, pLeft->G, pLeft->B,
					pRight->R, pRight->G, pRight->B);
			}
		}
		else
		{
			CurMapperWorldFPU(pLeft->X, pRight->X, pLeft->y
				, pLeft->R, pLeft->G, pLeft->B,
				pRight->R, pRight->G, pRight->B);
		}
		StepWorld(pLeft);
		StepWorld(pRight);

		Dest		+=ClientWindow.PixelPitch / 2;
		ZBufLine	+=ClientWindow.Width;
		pZBufferPtr	=ZBufLine;
	}
}

void SpanOutWorldTriAlpha3DNow(DRV_TLVertex *verts)
{
	int			Top, Middle, Bottom, MiddleForCompare;
	int			MiddleIsLeft, BottomForCompare, Height;
	float		Y0=verts[0].y, Y1=verts[1].y, Y2=verts[2].y;
	EdgeAsmWorld	TopToBottom, TopToMiddle, MiddleToBottom;
	EdgeAsmWorld	*pLeft, *pRight;

	// sort vertices in y
	if(Y0 < Y1)
	{
		if(Y2 < Y0)
		{
			Top		=2;
			Middle	=MiddleForCompare	=0;
			Bottom	=BottomForCompare	=1;
		}
		else
		{
			Top	=0;
			if(Y1 < Y2)
			{
				Middle	=MiddleForCompare	=1;
				Bottom	=BottomForCompare	=2;
			}
			else
			{
				Middle	=MiddleForCompare	=2;
				Bottom	=BottomForCompare	=1;
			}
		}
	}
	else
	{
		if(Y2 < Y1)
		{
			Top		=2;
			Middle	=MiddleForCompare	=1;
			Bottom	=BottomForCompare	=0;
		}
		else
		{
			Top	=1;
			if(Y0 < Y2)
			{
				Middle	=0;
				Bottom	=BottomForCompare	=2;
				MiddleForCompare			=3;
			}
			else
			{
				BottomForCompare			=3;
				Middle	=MiddleForCompare	=2;
				Bottom	=0;
			}
		}
	}

	setup_edgeWorld(&TopToBottom, verts, Top, Bottom);
	setup_edgeWorld(&TopToMiddle, verts, Top, Middle);
	setup_edgeWorld(&MiddleToBottom, verts, Middle, Bottom);

	Femms3DNow();

	if(BottomForCompare > MiddleForCompare)
	{
		MiddleIsLeft = 0;
		pLeft = &TopToBottom; pRight = &TopToMiddle;
	}
	else
	{
		MiddleIsLeft = 1;
		pLeft = &TopToMiddle; pRight = &TopToBottom;
	}

	Height	=TopToMiddle.Height;

	Dest		=(U16 *)(ClientWindow.Buffer + (pLeft->y * ClientWindow.PixelPitch));
	
	while(Height--)
	{
		CurMapperWorld(pLeft->X, pRight->X, pLeft->y
			, pLeft->r, pLeft->g, pLeft->b,
			pRight->r, pRight->g, pRight->b);

		StepWorld3DNow(pLeft);
		StepWorld3DNow(pRight);

		Dest		+=ClientWindow.PixelPitch / 2;
	}

	Height	=MiddleToBottom.Height;

	if(MiddleIsLeft)
	{
		pLeft	=&MiddleToBottom;
		pRight	=&TopToBottom;
	}
	else
	{
		pLeft	=&TopToBottom;
		pRight	=&MiddleToBottom;
	}
	
	while(Height--)
	{
		CurMapperWorld(pLeft->X, pRight->X, pLeft->y
			, pLeft->r, pLeft->g, pLeft->b,
			pRight->r, pRight->g, pRight->b);

		StepWorld3DNow(pLeft);
		StepWorld3DNow(pRight);

		Dest		+=ClientWindow.PixelPitch / 2;
	}
	Femms3DNow();
}

BOOL DRIVERCC RenderWorldPoly(DRV_TLVertex *Pnts, S32 NumPoints, geRDriver_THandle *THandle, DRV_TexInfo *TexInfo, DRV_LInfo *LInfo, U32 Flags)
{
	U8				MipLevel = 0;
	int32			i;
	DRV_TLVertex	TriVerts[3];

	if(!bActive)
	{
		return	GE_TRUE;
	}

	assert(THandle != NULL);
	assert(Pnts != NULL);
	assert(NumPoints > 2);
	
	GlobalInfo = SOFTDRV.GlobalInfo;
	assert(GlobalInfo);

	//
	//	Get the mip level
	//
	{
		float	du, dv, dx, dy, MipScale;

		du	=Pnts[1].u - Pnts[0].u;
		dv	=Pnts[1].v - Pnts[0].v;
		dx	=Pnts[1].x - Pnts[0].x;
		dy	=Pnts[1].y - Pnts[0].y;

		du	/=TexInfo->DrawScaleU;
		dv	/=TexInfo->DrawScaleV;

		MipScale	=((du*du)+(dv*dv)) / ((dx*dx)+(dy*dy));

		if(MipScale <= 5)		// 2, 6, 12
		{
			MipLevel	=0;
		}
		else if(MipScale <= 20)
		{
			MipLevel	=1;
		}
		else if(MipScale <= 45)
		{
			MipLevel	=2;
		}
		else
		{
			MipLevel	=3;
		}
	}

	if(MipLevel >= THandle->MipLevels)
	{
		MipLevel	=THandle->MipLevels-1;
	}

	assert(THandle->BitPtr[MipLevel] != NULL);

	GW = THandle->Width >> MipLevel;
	GH = THandle->Height >> MipLevel;
	GWMask = GW-1;
	GHMask = GH-1;

	CalcGradients(MipLevel, TexInfo->DrawScaleU, TexInfo->DrawScaleV);

	// Assign some global variables...
	GTexture	=THandle;
	GLInfo		=LInfo;

	if(ProcessorInfo.Has3DNow)
	{
		GBitPtr		=(U8 *)THandle->BitPtr[MipLevel];
		TexPal		=(U8 *)THandle->PalHandle->BitPtr[0];
	}
	else
	{
		GBitPtr16	=THandle->BitPtr[MipLevel];
		GWMaskShifted=(GWMask<<16) |0xffff;
	}

	GMipLevel = (U8)MipLevel;
	GMipLevel4 = 4-MipLevel;
	GMipLevel4_8 = GMipLevel4+8;
	GMipLevel20 = 20-MipLevel;

	QGMip20	=GMipLevel20;
	QGMip4_8=GMipLevel4_8;

	if(LInfo)
	{
		GLMapAdd = LInfo->Width*3 - 6;
		GLightWidth = LInfo->Width;

		GLightData = (uint8*)LInfo->RGBLight[0];

		if(RenderMode == RENDER_MODELS)
			SpanMode = 2;
		else
			SpanMode = 0;
	}
	else
	{
		if(RenderMode == RENDER_MODELS)
			SpanMode = 3;
		else
			SpanMode = 1;
	}
	
	if(Flags & DRV_RENDER_ALPHA)
	{
		PolyIsTrans	=GE_TRUE;
	}
	else
	{
		PolyIsTrans	=FALSE;
	}

	// Reset the polyvisible flag
	PolyVisible = 0;

/*	if(THandle->PixelFormat.Flags & RDRIVER_PF_HAS_ALPHA)
	{
		PolyIsTrans	=GE_TRUE;
	}
	else if(THandle->PalHandle)
	{
		PolyIsTrans	=gePixelFormat_HasAlpha(THandle->PalHandle->PixelFormat.PixelFormat);
	}
	else
	{
		PolyIsTrans	=GE_FALSE;
	}
*/
	if(RenderMode == RENDER_WORLD || RenderMode == RENDER_MODELS)
	{
		assert(!(Flags & DRV_RENDER_ALPHA));
		if(NumPoints==3)
		{
			TriVerts[0]	=Pnts[0];
			TriVerts[1]	=Pnts[1];
			TriVerts[2]	=Pnts[2];

			if(ProcessorInfo.Has3DNow)
			{
				SpanOutWorldTri3DNow(TriVerts);
			}
			else
			{
				SpanOutWorldTri(TriVerts);
			}
		}
		else
		{
			for(i=0;i < NumPoints-2;i++)
			{
				TriVerts[0]	=Pnts[0];
				TriVerts[1]	=Pnts[1+i];
				TriVerts[2]	=Pnts[2+i];
				
				if(ProcessorInfo.Has3DNow)
				{
					SpanOutWorldTri3DNow(TriVerts);
				}
				else
				{
					SpanOutWorldTri(TriVerts);
				}
			}
		}
	}
	else
	{
		bStipple	=GE_FALSE;
		if(ProcessorInfo.Has3DNow)
		{
/*			if(LInfo)	//lit alpha not done yet
			{
				//check for argb alpha faces (no seperate right now)
				if(GTexture->PixelFormat.PixelFormat==GE_PIXELFORMAT_32BIT_ARGB)
				{
					CurMapperWorld	=DrawSpan32_AsmGouraudZBufferAlphaARGB3DNow;
				}
				else
				{
					CurMapperWorld	=DrawSpan32_AsmLitZBuffer3DNow;
				}
			}
			else
*/			{
				//vertex alpha overrides argb 
				if(Pnts[0].a < 255.0f)
				{
					i	=(int)Pnts[0].a;
					*((U32 *)(&VertAlpha))		=(i << 16) | i;
					*(((U32 *)((&VertAlpha)))+1)=(i << 16) | i;
					CurMapperWorld	=DrawSpan32_AsmGouraudZBufferVertexAlpha3DNow;
				}
				else
				{
					//check for argb alpha faces (no seperate right now)
					if(GTexture->PalHandle->PixelFormat.PixelFormat==GE_PIXELFORMAT_32BIT_ARGB)
					{
						CurMapperWorld	=DrawSpan32_AsmGouraudZBufferAlphaARGB3DNow;
					}
					else
					{
						CurMapperWorld	=DrawSpan32_AsmGouraudZBuffer3DNow;
					}
				}
			}
			if(NumPoints==3)
			{
				TriVerts[0]	=Pnts[0];
				TriVerts[1]	=Pnts[1];
				TriVerts[2]	=Pnts[2];

				SpanOutWorldTriAlpha3DNow(TriVerts);
			}
			else
			{
				for(i=0;i < NumPoints-2;i++)
				{
					TriVerts[0]	=Pnts[0];
					TriVerts[1]	=Pnts[1+i];
					TriVerts[2]	=Pnts[2+i];
					
					SpanOutWorldTriAlpha3DNow(TriVerts);
				}
			}
		}
		else
		{
/*			if(LInfo)	//lit alpha not done yet
			{
				//check for argb alpha faces (no seperate right now)
				if(GTexture->PixelFormat.PixelFormat==GE_PIXELFORMAT_32BIT_ARGB)
				{
					CurMapperWorld	=DrawSpan32_AsmGouraudZBufferAlphaARGB3DNow;
				}
				else
				{
					CurMapperWorld	=DrawSpan32_AsmLitZBuffer3DNow;
				}
			}
			else
*/
			if(ClientWindow.G_mask == 0x03e0)
			{
				if(Pnts[0].a > 254.0f)
				{
					CurMapperWorldFPU	=DrawSpan16_AsmGouraudZBufferTrans555X86FPU;
				}
				else
				{
					CurMapperWorldFPU	=DrawSpan16_AsmGouraudZBuffer555X86FPU;
				}
				bStipple			=GE_TRUE;
			}
			else
			{
				if(Pnts[0].a > 254.0f)
				{
					CurMapperWorldFPU	=DrawSpan16_AsmGouraudZBufferTransX86FPU;
				}
				else
				{
					CurMapperWorldFPU	=DrawSpan16_AsmGouraudZBufferX86FPU;
				}
				bStipple			=GE_TRUE;
			}
			if(NumPoints==3)
			{
				TriVerts[0]	=Pnts[0];
				TriVerts[1]	=Pnts[1];
				TriVerts[2]	=Pnts[2];

				SpanOutWorldTriAlpha(TriVerts);
			}
			else
			{
				for(i=0;i < NumPoints-2;i++)
				{
					TriVerts[0]	=Pnts[0];
					TriVerts[1]	=Pnts[1+i];
					TriVerts[2]	=Pnts[2+i];
					
					SpanOutWorldTriAlpha(TriVerts);
				}
			}
		}

		bStipple	=GE_FALSE;
	}

	return GE_TRUE;
}

void Step(EdgeAsm *edge)
{
	_asm
	{
		mov     eax,edge
		fld		[eax]EdgeAsm.x		; x
		fadd    [eax]EdgeAsm.xstep	; xd
		fld     [eax]EdgeAsm.u		; u   xd
		fadd    [eax]EdgeAsm.ustep	; ud  xd
		fld     [eax]EdgeAsm.v		; v   ud  xd
		fadd    [eax]EdgeAsm.vstep  ; vd  ud  xd
		fld     [eax]EdgeAsm.z	    ; z   vd  ud  xd
		fadd    [eax]EdgeAsm.zstep  ; zd  vd  ud  xd
		fld		[eax]EdgeAsm.r		; r   zd  vd  ud  xd
		fadd	[eax]EdgeAsm.rstep	; rd  zd  vd  ud  xd
		fld		[eax]EdgeAsm.g		; g   rd  zd  vd  ud  xd
		fadd	[eax]EdgeAsm.gstep	; gd  rd  zd  vd  ud  xd
		fld		[eax]EdgeAsm.b		; b   gd  rd  zd  vd  ud  xd
		fadd	[eax]EdgeAsm.bstep	; bd  gd  rd  zd  vd  ud  xd
		fxch	st(2)				; rd  gd  bd  zd  vd  ud  xd
		fstp	[eax]EdgeAsm.r		; gd  bd  zd  vd  ud  xd
		fstp	[eax]EdgeAsm.g		; bd  zd  vd  ud  xd
		fstp	[eax]EdgeAsm.b		; zd  vd  ud  xd
		fstp	[eax]EdgeAsm.z		; vd  ud  xd
		fstp	[eax]EdgeAsm.v		; ud  xd
		fstp	[eax]EdgeAsm.u		; xd
		fst		[eax]EdgeAsm.x		; xd
		fsub    [FistTruncate]      ; xdt
		fadd    [FistMagic]         ; xnt
		fstp    [eax]EdgeAsm.X

		mov     edx,[FistMagic]
		sub     [eax]EdgeAsm.X,edx
	}
	edge->y++;
	edge->Height--;
}

//special handling of colors for fpu blend
void StepFPU(EdgeAsmFPU *edge)
{
	int	r,g,b;
	_asm
	{
		push	ecx
		push	ebx
		push	esi

		mov     eax,edge
		fld		[eax]EdgeAsmFPU.x		; x
		fadd    [eax]EdgeAsmFPU.xstep	; xd
		fld     [eax]EdgeAsmFPU.u		; u   xd
		fadd    [eax]EdgeAsmFPU.ustep	; ud  xd
		fld     [eax]EdgeAsmFPU.v		; v   ud  xd
		fadd    [eax]EdgeAsmFPU.vstep  ; vd  ud  xd
		fld     [eax]EdgeAsmFPU.z	    ; z   vd  ud  xd
		fadd    [eax]EdgeAsmFPU.zstep  ; zd  vd  ud  xd
		fld		[eax]EdgeAsmFPU.r		; r   zd  vd  ud  xd
		fadd	[eax]EdgeAsmFPU.rstep	; rd  zd  vd  ud  xd
		fld		[eax]EdgeAsmFPU.g		; g   rd  zd  vd  ud  xd
		fadd	[eax]EdgeAsmFPU.gstep	; gd  rd  zd  vd  ud  xd
		fld		[eax]EdgeAsmFPU.b		; b   gd  rd  zd  vd  ud  xd
		fadd	[eax]EdgeAsmFPU.bstep	; bd  gd  rd  zd  vd  ud  xd
		fxch	st(2)				; rd  gd  bd  zd  vd  ud  xd
		fst		[eax]EdgeAsmFPU.r
		fadd	[FistMagic]			; rdk gd  bd  zd  vd  ud  xd
		fxch	st(2)				; bd  gd  rdk zd  vd  ud  xd
		fst		[eax]EdgeAsmFPU.b
		fadd	[FistMagic]			; bdk gd  rdk zd  vd  ud  xd
		fxch	st(1)				; gd  bdk rdk zd  vd  ud  xd
		fst		[eax]EdgeAsmFPU.g
		fadd	[FistMagic]			; gdk bdk rdk zd  vd  ud  xd
		fxch	st(2)				; rdk bdk gdk zd  vd  ud  xd
		fstp	[r]					; bdk gdk zd  vd  ud  xd
		fstp	[b]					; gdk zd  vd  ud  xd
		fstp	[g]					; zd  vd  ud  xd

		mov		edx,[FistMagic]
		mov		ebx,[r]

		fstp	[eax]EdgeAsmFPU.z		; vd  ud  xd

		mov		ecx,[g]
		sub		ebx,edx
		
		fstp	[eax]EdgeAsmFPU.v		; ud  xd

		mov		esi,[b]
		sub		ecx,edx

		fstp	[eax]EdgeAsmFPU.u		; xd

		sub		esi,edx
		and		ebx,0fch

		fst		[eax]EdgeAsmFPU.x		; xd

		and		ecx,0fch
		and		esi,0fch

		fsub    [FistTruncate]      ; xdt

		mov		[eax]EdgeAsmFPU.R,ebx
		mov		[eax]EdgeAsmFPU.G,ecx

		fadd    [FistMagic]         ; xnt

		mov		[eax]EdgeAsmFPU.B,esi

		fstp    [eax]EdgeAsmFPU.X

		sub     [eax]EdgeAsmFPU.X,edx

		pop		esi
		pop		ebx
		pop		ecx
	}
	edge->y++;
	edge->Height--;
}

void setup_edge(EdgeAsm *edge, DRV_TLVertex *verts, int top, int end)
{
	int		ey,ty;
	float	yd;

	_asm
	{
		push	ebx
		mov		eax,end
		mov     edx,top

		shl     eax,2
		mov     ebx,verts

		shl     edx,2
		lea		eax,[8*eax+eax]
		lea		edx,[8*edx+edx]

		fld		[ebx+eax]DRV_TLVertex.y		; endy
		fld     [ebx+edx]DRV_TLVertex.y		; topy endy
		fsub    st(1),st	                ; topy yd
		fld     [ebx+eax]DRV_TLVertex.y		; endy topy yd
		fsub    [FistTruncate]
		fadd    [FistMagic]					; eyi  topy yd
		fxch    st(2)                       ; yd   topy eyi
		fstp    [yd]						; topy eyi
		fsub    [FistTruncate]
		fadd    [FistMagic]	                ; tyi  eyi
		fxch    st(1)                       ; eyi  tyi
		fstp    [ey]                        ; tyi
		fld     [ebx+edx]DRV_TLVertex.x     ; x    tyi
		fxch    st(1)                       ; tyi  x
		fstp    [ty]                        ; x
		fsub    [FistTruncate]
		fadd    [FistMagic]                 ; xi

		mov     ebx,[FistMagic]
		mov     eax,edge

		sub     [ey],ebx

		fstp    [eax]EdgeAsm.X

		sub     [ty],ebx

		sub     [eax]EdgeAsm.X, ebx
		pop		ebx
	}

	edge->Height	=ey-ty;

	yd		=(edge->Height >0)? 1.0f/edge->Height : 1;
	edge->y	=ty;

	edge->x		=verts[top].x;
	edge->xstep	=(verts[end].x-edge->x)*yd;

	edge->u		=verts[top].u;
	edge->ustep	=(verts[end].u-edge->u)*yd;

	edge->v		=verts[top].v;
	edge->vstep	=(verts[end].v-edge->v)*yd;

	edge->z		=(1.0f / verts[top].z) * (float)ZBUFFER_PREC;
	edge->zstep	=(((1.0f / verts[end].z) * (float)ZBUFFER_PREC)-edge->z)*yd;

	edge->r		=verts[top].r;
	edge->rstep	=(verts[end].r-edge->r)*yd;

	edge->g		=verts[top].g;
	edge->gstep	=(verts[end].g-edge->g)*yd;

	edge->b		=verts[top].b;
	edge->bstep	=(verts[end].b-edge->b)*yd;
}

void setup_edgeFPU(EdgeAsmFPU *edge, DRV_TLVertex *verts, int top, int end)
{
	int		ey,ty;
	float	yd;

	_asm
	{
		push	ebx
		mov		eax,end
		mov     edx,top

		shl     eax,2
		mov     ebx,verts

		shl     edx,2
		lea		eax,[8*eax+eax]
		lea		edx,[8*edx+edx]

		fld		[ebx+eax]DRV_TLVertex.y		; endy
		fld     [ebx+edx]DRV_TLVertex.y		; topy endy
		fsub    st(1),st	                ; topy yd
		fld     [ebx+eax]DRV_TLVertex.y		; endy topy yd
		fsub    [FistTruncate]
		fadd    [FistMagic]					; eyi  topy yd
		fxch    st(2)                       ; yd   topy eyi
		fstp    [yd]						; topy eyi
		fsub    [FistTruncate]
		fadd    [FistMagic]	                ; tyi  eyi
		fxch    st(1)                       ; eyi  tyi
		fstp    [ey]                        ; tyi
		fld     [ebx+edx]DRV_TLVertex.x     ; x    tyi
		fxch    st(1)                       ; tyi  x
		fstp    [ty]                        ; x
		fsub    [FistTruncate]
		fadd    [FistMagic]                 ; xi

		mov     ebx,[FistMagic]
		mov     eax,edge

		sub     [ey],ebx

		fstp    [eax]EdgeAsm.X

		sub     [ty],ebx

		sub     [eax]EdgeAsm.X, ebx
		pop		ebx
	}

	edge->Height	=ey-ty;

	yd		=(edge->Height >0)? 1.0f/edge->Height : 1;
	edge->y	=ty;

	edge->x		=verts[top].x;
	edge->xstep	=(verts[end].x-edge->x)*yd;

	edge->u		=verts[top].u;
	edge->ustep	=(verts[end].u-edge->u)*yd;

	edge->v		=verts[top].v;
	edge->vstep	=(verts[end].v-edge->v)*yd;

	edge->z		=(1.0f / verts[top].z) * (float)ZBUFFER_PREC;
	edge->zstep	=(((1.0f / verts[end].z) * (float)ZBUFFER_PREC)-edge->z)*yd;

	edge->r		=verts[top].r;
	edge->rstep	=(verts[end].r-edge->r)*yd;
	edge->R		=(int)(verts[top].r)&0xfc;

	edge->g		=verts[top].g;
	edge->gstep	=(verts[end].g-edge->g)*yd;
	edge->G		=(int)(verts[top].g)&0xfc;

	edge->b		=verts[top].b;
	edge->bstep	=(verts[end].b-edge->b)*yd;
	edge->B		=(int)(verts[top].b)&0xfc;
}

//#define	WIREFRAMEHACK

void TmapTriangle_32(DRV_TLVertex *verts)
{
	int		Top, Middle, Bottom, MiddleForCompare;
	int		MiddleIsLeft, BottomForCompare, Height;
	EdgeAsm	TopToBottom, TopToMiddle, MiddleToBottom;
	EdgeAsm	*pLeft, *pRight;
	float	Y0=verts[0].y, Y1=verts[1].y, Y2=verts[2].y;

	// sort vertices in y
	if(Y0 < Y1)
	{
		if(Y2 < Y0)
		{
			Top		=2;
			Middle	=MiddleForCompare	=0;
			Bottom	=BottomForCompare	=1;
		}
		else
		{
			Top	=0;
			if(Y1 < Y2)
			{
				Middle	=MiddleForCompare	=1;
				Bottom	=BottomForCompare	=2;
			}
			else
			{
				Middle	=MiddleForCompare	=2;
				Bottom	=BottomForCompare	=1;
			}
		}
	}
	else
	{
		if(Y2 < Y1)
		{
			Top		=2;
			Middle	=MiddleForCompare	=1;
			Bottom	=BottomForCompare	=0;
		}
		else
		{
			Top	=1;
			if(Y0 < Y2)
			{
				Middle	=0;
				Bottom	=BottomForCompare	=2;
				MiddleForCompare			=3;
			}
			else
			{
				BottomForCompare			=3;
				Middle	=MiddleForCompare	=2;
				Bottom	=0;
			}
		}
	}

	setup_edge((EdgeAsm *)&TopToBottom, verts, Top, Bottom);
	setup_edge((EdgeAsm *)&TopToMiddle, verts, Top, Middle);
	setup_edge((EdgeAsm *)&MiddleToBottom, verts, Middle, Bottom);

	if(BottomForCompare > MiddleForCompare)
	{
		MiddleIsLeft = 0;
		pLeft = &TopToBottom; pRight = &TopToMiddle;
	}
	else
	{
		MiddleIsLeft = 1;
		pLeft = &TopToMiddle; pRight = &TopToBottom;
	}

	Height		=TopToMiddle.Height;
	Dest			=(U16 *)(ClientWindow.Buffer + (pLeft->y * ClientWindow.PixelPitch));
	ZBufLine	=ZBuffer + (pLeft->y * ClientWindow.Width);
	pZBufferPtr		=ZBufLine;

#ifdef WIREFRAMEHACK
	Top=1;
#endif

	while(Height--)
	{
#ifdef WIREFRAMEHACK
		if(!Height || Top)
		{
			Top	=0;
			if(pLeft->X <= pRight->X)
			{
				memset(((U32 *)Dest)+pLeft->X, 0xff, (pRight->X - pLeft->X)<<2);
			}
			else
			{
				memset(((U32 *)Dest)+pRight->X, 0xff, (pLeft->X - pRight->X)<<2);
			}
		}
		else
		{
			//uncomment for zbuffered wire
//		if(*(ZBufLine + pLeft->X) > ((U16)pLeft->z))
//		{
			*(((U32 *)Dest) + pLeft->X)	=0xffffffff;
//		}
//		if(*(ZBufLine + pRight->X) > ((U16)pRight->z))
//		{
			*(((U32 *)Dest) + pRight->X)=0xffffffff;
//		}
		}
#else
		if(bStipple)
		{
			if(pLeft->y & 1)
			{
				if(pLeft->X <= pRight->X)
				{
					CurMapper(pLeft, pRight);
				}
				else
				{
					CurMapper(pRight, pLeft);
				}
			}
		}
		else
		{
			if(pLeft->X <= pRight->X)
			{
				CurMapper(pLeft, pRight);
			}
			else
			{
				CurMapper(pRight, pLeft);
			}
		}
#endif
		Step(pLeft);
		Step(pRight);

		Dest		+=ClientWindow.PixelPitch / 2;
		ZBufLine	+=ClientWindow.Width;
		pZBufferPtr	=ZBufLine;
	}

	Height	=MiddleToBottom.Height;

	if(MiddleIsLeft)
	{
		pLeft	=&MiddleToBottom;
		pRight	=&TopToBottom;
	}
	else
	{
		pLeft	=&TopToBottom;
		pRight	=&MiddleToBottom;
	}
	
	while(Height--)
	{
#ifdef WIREFRAMEHACK
		if(!Height || Top)
		{
			Top	=0;
			if(pLeft->X <= pRight->X)
			{
				memset(((U32 *)Dest)+pLeft->X, 0xff, (pRight->X - pLeft->X)<<2);
			}
			else
			{
				memset(((U32 *)Dest)+pRight->X, 0xff, (pLeft->X - pRight->X)<<2);
			}
		}
		else
		{
			//uncomment for zbuffered wire
//		if(*(ZBufLine + pLeft->X) > ((U16)pLeft->z))
//		{
			*(((U32 *)Dest) + pLeft->X)	=0xffffffff;
//		}
//		if(*(ZBufLine + pRight->X) > ((U16)pRight->z))
//		{
			*(((U32 *)Dest) + pRight->X)=0xffffffff;
//		}
		}
#else
		if(bStipple)
		{
			if(pLeft->y & 1)
			{
				if(pLeft->X <= pRight->X)
				{
					CurMapper(pLeft, pRight);
				}
				else
				{
					CurMapper(pRight, pLeft);
				}
			}
		}
		else
		{
			if(pLeft->X <= pRight->X)
			{
				CurMapper(pLeft, pRight);
			}
			else
			{
				CurMapper(pRight, pLeft);
			}
		}
#endif
		Step(pLeft);
		Step(pRight);

		Dest		+=ClientWindow.PixelPitch / 2;
		ZBufLine	+=ClientWindow.Width;
		pZBufferPtr	=ZBufLine;
	}
}

//16 bit fpu version
void TmapTriangle_16(DRV_TLVertex *verts)
{
	int			Top, Middle, Bottom, MiddleForCompare;
	int			MiddleIsLeft, BottomForCompare, Height;
	EdgeAsmFPU	TopToBottom, TopToMiddle, MiddleToBottom;
	EdgeAsmFPU	*pLeft, *pRight;
	float		Y0=verts[0].y, Y1=verts[1].y, Y2=verts[2].y;

	// sort vertices in y
	if(Y0 < Y1)
	{
		if(Y2 < Y0)
		{
			Top		=2;
			Middle	=MiddleForCompare	=0;
			Bottom	=BottomForCompare	=1;
		}
		else
		{
			Top	=0;
			if(Y1 < Y2)
			{
				Middle	=MiddleForCompare	=1;
				Bottom	=BottomForCompare	=2;
			}
			else
			{
				Middle	=MiddleForCompare	=2;
				Bottom	=BottomForCompare	=1;
			}
		}
	}
	else
	{
		if(Y2 < Y1)
		{
			Top		=2;
			Middle	=MiddleForCompare	=1;
			Bottom	=BottomForCompare	=0;
		}
		else
		{
			Top	=1;
			if(Y0 < Y2)
			{
				Middle	=0;
				Bottom	=BottomForCompare	=2;
				MiddleForCompare			=3;
			}
			else
			{
				BottomForCompare			=3;
				Middle	=MiddleForCompare	=2;
				Bottom	=0;
			}
		}
	}

	setup_edgeFPU(&TopToBottom, verts, Top, Bottom);
	setup_edgeFPU(&TopToMiddle, verts, Top, Middle);
	setup_edgeFPU(&MiddleToBottom, verts, Middle, Bottom);

	if(BottomForCompare > MiddleForCompare)
	{
		MiddleIsLeft = 0;
		pLeft = &TopToBottom; pRight = &TopToMiddle;
	}
	else
	{
		MiddleIsLeft = 1;
		pLeft = &TopToMiddle; pRight = &TopToBottom;
	}

	Height	=TopToMiddle.Height;

	Dest			=(U16 *)(ClientWindow.Buffer + (pLeft->y * ClientWindow.PixelPitch));
	ZBufLine	=ZBuffer + (pLeft->y * ClientWindow.Width);
	
	pZBufferPtr		=ZBufLine;
	while(Height--)
	{
		if(bStipple)
		{
			if(pLeft->y & 1)
			{
				if(pLeft->X <= pRight->X)
				{
					CurMapperFPU(pLeft, pRight);
				}
				else
				{
					CurMapperFPU(pRight, pLeft);
				}
			}
		}
		else
		{
			if(pLeft->X <= pRight->X)
			{
				CurMapperFPU(pLeft, pRight);
			}
			else
			{
				CurMapperFPU(pRight, pLeft);
			}
		}
		StepFPU(pLeft);
		StepFPU(pRight);

		Dest		+=ClientWindow.PixelPitch / 2;
		ZBufLine	+=ClientWindow.Width;
		pZBufferPtr	=ZBufLine;
	}

	Height	=MiddleToBottom.Height;

	if(MiddleIsLeft)
	{
		pLeft	=&MiddleToBottom;
		pRight	=&TopToBottom;
	}
	else
	{
		pLeft	=&TopToBottom;
		pRight	=&MiddleToBottom;
	}
	
	while(Height--)
	{
		if(bStipple)
		{
			if(pLeft->y & 1)
			{
				if(pLeft->X <= pRight->X)
				{
					CurMapperFPU(pLeft, pRight);
				}
				else
				{
					CurMapperFPU(pRight, pLeft);
				}
			}
		}
		else
		{
			if(pLeft->X <= pRight->X)
			{
				CurMapperFPU(pLeft, pRight);
			}
			else
			{
				CurMapperFPU(pRight, pLeft);
			}
		}
		StepFPU(pLeft);
		StepFPU(pRight);

		Dest		+=ClientWindow.PixelPitch / 2;
		ZBufLine	+=ClientWindow.Width;
		pZBufferPtr	=ZBufLine;
	}
}

BOOL DRIVERCC RenderMiscTexturePoly(DRV_TLVertex *Pnts, S32 NumPoints, geRDriver_THandle *THandle, U32 Flags)
{
	int					i, GW_log2, MipLevel=0;
	DRV_TLVertex		TriVerts[3];

	if(!bActive)
	{
		return	GE_TRUE;
	}

	assert(Pnts != NULL);
	assert(NumPoints > 2);

	assert(THandle->BitPtr[MipLevel] != NULL);

	// Assign some global variables...
	GMipLevel	=(U8)MipLevel;
	GW			=THandle->Width >> MipLevel;
	GH			=THandle->Height >> MipLevel;
	GWMask		=GW-1;
	GHMask		=GH-1;
	
	if(THandle->Flags & THANDLE_TRANS)
	{
		PolyMode	=1;
	}
	else
	{
		PolyMode	=0;
	}

	if(ProcessorInfo.Has3DNow)
	{
		*((int *)(&WrapMask))		=GHMask<<16;
		*(((int *)((&WrapMask)))+1)	=GWMask<<16;

		*((float *)(&GBL))			=(float)GH;//Mask;
		*(((float *)((&GBL)))+1)	=(float)GW;//Mask;

		//find log2 of the texture width
		for(GW_log2=1;((1<<GW_log2) < GW); GW_log2++);

		QShiftV	=(__int64)(16 - GW_log2);
		TexPal	=(U8 *)THandle->PalHandle->BitPtr[0];
		GBitPtr = (U8 *)THandle->BitPtr[MipLevel];

		bStipple	=FALSE;
		if(THandle->PixelFormat.Flags & RDRIVER_PF_HAS_ALPHA)
		{
			if(THandle->AlphaHandle)
			{
				ABitPtr	=(U8 *)THandle->AlphaHandle->BitPtr[0];
				ATexPal	=(U8 *)THandle->AlphaHandle->PalHandle->BitPtr[0];

				CurMapper	=DrawScanLineGouraudNoZAlphaTex_Asm3DNow;
				if(Flags & DRV_RENDER_NO_ZMASK)
				{
					if(Flags & DRV_RENDER_NO_ZWRITE)
					{
						CurMapper	=DrawScanLineGouraudNoZAlphaTex_Asm3DNow;
					}
					else
					{
						CurMapper	=DrawScanLineGouraudNoZBufferZWriteAlphaTex_Asm3DNow;
					}
				}
				else
				{
					if(Flags & DRV_RENDER_NO_ZWRITE)
					{
						CurMapper	=DrawScanLineGouraudZBufferNoZWriteAlphaTex_Asm3DNow;
					}
					else
					{
						CurMapper	=DrawScanLineGouraudZBufferAlphaTex_Asm3DNow;
					}
				}
			}
			else
			{
				assert(THandle->PixelFormat.PixelFormat==GE_PIXELFORMAT_32BIT_ARGB);
				CurMapper	=DrawScanLineGouraudNoZAlphaARGB_Asm3DNow;
/*				if(Flags & DRV_RENDER_NO_ZMASK)
				{
					if(Flags & DRV_RENDER_NO_ZWRITE)
					{
						CurMapper	=DrawScanLineGouraudNoZAlphaTex_Asm3DNow;
					}
					else
					{
						CurMapper	=DrawScanLineGouraudNoZBufferZWriteAlphaTex_Asm3DNow;
					}
				}
				else
				{
					if(Flags & DRV_RENDER_NO_ZWRITE)
					{
						CurMapper	=DrawScanLineGouraudZBufferNoZWriteAlphaTex_Asm3DNow;
					}
					else
					{
						CurMapper	=DrawScanLineGouraudZBufferAlphaTex_Asm3DNow;
					}
				}*/
			}
		}
		else
		{
			if(Flags & DRV_RENDER_NO_ZMASK)
			{
				if(Flags & DRV_RENDER_NO_ZWRITE)
				{
					if(PolyMode)
					{
						CurMapper	=DrawScanLineGouraudNoZTrans_Asm3DNow;
					}
					else
					{
						CurMapper	=DrawScanLineGouraudNoZ_Asm3DNow;
					}
				}
				else
				{
					if(PolyMode)
					{
						CurMapper	=DrawScanLineGouraudNoZBufferZWriteTrans_Asm3DNow;
					}
					else
					{
						CurMapper	=DrawScanLineGouraudNoZBufferZWrite_Asm3DNow;
					}
				}
			}
			else
			{
				if(Flags & DRV_RENDER_NO_ZWRITE)
				{
					if(PolyMode)
					{
						CurMapper	=DrawScanLineGouraudZBufferNoZWriteTrans_Asm3DNow;
					}
					else
					{
						CurMapper	=DrawScanLineGouraudZBufferNoZWrite_Asm3DNow;
					}
				}
				else
				{
					if(PolyMode)
					{
						CurMapper	=DrawScanLineGouraudZBufferTrans_Asm3DNow;
					}
					else
					{
						CurMapper	=DrawScanLineGouraudZBuffer_Asm3DNow;
					}
				}
			}
		}
		if(NumPoints==3)
		{
			TriVerts[2]	=Pnts[0];
			TriVerts[1]	=Pnts[1];
			TriVerts[0]	=Pnts[2];

			TmapTriangle_32(TriVerts);
		}
		else
		{
			for(i=0;i < NumPoints-2;i++)
			{
				TriVerts[0]	=Pnts[0];
				TriVerts[1]	=Pnts[1+i];
				TriVerts[2]	=Pnts[2+i];

				TmapTriangle_32(TriVerts);
			}
		}
		bStipple	=FALSE;
		return GE_TRUE;
	}
	else
	{
		for(GW_log2=1;((1<<GW_log2) < GW); GW_log2++);

		GHMaskShifted	=GHMask << GW_log2;
		VShift			=(16 - GW_log2);
		GBitPtrHalf		=(U8 *)THandle->BitPtr[MipLevel];
		GBitPtrHalf		=(U8 *)((U32)(GBitPtrHalf)>>1);
		bStipple		=FALSE;

		if(ClientWindow.G_mask == 0x03e0)
		{
			if(THandle->AlphaHandle)
			{
				bStipple	=GE_TRUE;
	/*				if(bWindowed)
				{
					CurMapperFPU	=DrawScanLineGouraudNoZAlphaTex_AsmX86FPU;
					if(Flags & DRV_RENDER_NO_ZMASK)
					{
						if(Flags & DRV_RENDER_NO_ZWRITE)
						{
							CurMapperFPU	=DrawScanLineGouraudNoZAlphaTex_AsmX86FPU;
						}
						else
						{
							CurMapperFPU	=DrawScanLineGouraudNoZBufferZWriteAlphaTex_AsmX86FPU;
						}
					}
					else
					{
						if(Flags & DRV_RENDER_NO_ZWRITE)
						{
							CurMapperFPU	=DrawScanLineGouraudZBufferNoZWriteAlphaTex_AsmX86FPU;
						}
						else
						{
							CurMapperFPU	=DrawScanLineGouraudZBufferAlphaTex_AsmX86FPU;
						}
					}
				}
				else
				{*/
					CurMapperFPU	=DrawScanLineGouraudNoZTrans_Asm555X86FPU;
					if(Flags & DRV_RENDER_NO_ZMASK)
					{
						if(Flags & DRV_RENDER_NO_ZWRITE)
						{
							CurMapperFPU	=DrawScanLineGouraudNoZTrans_Asm555X86FPU;
						}
						else
						{
							CurMapperFPU	=DrawScanLineGouraudNoZBufferZWriteTrans_Asm555X86FPU;
						}
					}
					else
					{
						if(Flags & DRV_RENDER_NO_ZWRITE)
						{
							CurMapperFPU	=DrawScanLineGouraudZBufferNoZWriteTrans_Asm555X86FPU;
						}
						else
						{
							CurMapperFPU	=DrawScanLineGouraudZBufferTrans_Asm555X86FPU;
						}
					}
	//			}
			}
			else
			{
				if(Flags & DRV_RENDER_NO_ZMASK)
				{
					if(Flags & DRV_RENDER_NO_ZWRITE)
					{
						if(PolyMode)
						{
							CurMapperFPU	=DrawScanLineGouraudNoZTrans_Asm555X86FPU;
						}
						else
						{
							CurMapperFPU	=DrawScanLineGouraudNoZ_Asm555X86FPU;
						}
					}
					else
					{
						if(PolyMode)
						{
							CurMapperFPU	=DrawScanLineGouraudNoZBufferZWriteTrans_Asm555X86FPU;
						}
						else
						{
							CurMapperFPU	=DrawScanLineGouraudNoZBufferZWrite_Asm555X86FPU;
						}
					}
				}
				else
				{
					if(Flags & DRV_RENDER_NO_ZWRITE)
					{
						if(PolyMode)
						{
							CurMapperFPU	=DrawScanLineGouraudZBufferNoZWriteTrans_Asm555X86FPU;
						}
						else
						{
							CurMapperFPU	=DrawScanLineGouraudZBufferNoZWrite_Asm555X86FPU;
						}
					}
					else
					{
						if(PolyMode)
						{
							CurMapperFPU	=DrawScanLineGouraudZBufferTrans_Asm555X86FPU;
						}
						else
						{
							CurMapperFPU	=DrawScanLineGouraudZBuffer_Asm555X86FPU;
						}
					}
				}
			}
		}
		else
		{
			if(THandle->AlphaHandle)
			{
				bStipple	=GE_TRUE;
	/*				if(bWindowed)
				{
					CurMapperFPU	=DrawScanLineGouraudNoZAlphaTex_AsmX86FPU;
					if(Flags & DRV_RENDER_NO_ZMASK)
					{
						if(Flags & DRV_RENDER_NO_ZWRITE)
						{
							CurMapperFPU	=DrawScanLineGouraudNoZAlphaTex_AsmX86FPU;
						}
						else
						{
							CurMapperFPU	=DrawScanLineGouraudNoZBufferZWriteAlphaTex_AsmX86FPU;
						}
					}
					else
					{
						if(Flags & DRV_RENDER_NO_ZWRITE)
						{
							CurMapperFPU	=DrawScanLineGouraudZBufferNoZWriteAlphaTex_AsmX86FPU;
						}
						else
						{
							CurMapperFPU	=DrawScanLineGouraudZBufferAlphaTex_AsmX86FPU;
						}
					}
				}
				else
				{*/
					CurMapperFPU	=DrawScanLineGouraudNoZTrans_AsmX86FPU;
					if(Flags & DRV_RENDER_NO_ZMASK)
					{
						if(Flags & DRV_RENDER_NO_ZWRITE)
						{
							CurMapperFPU	=DrawScanLineGouraudNoZTrans_AsmX86FPU;
						}
						else
						{
							CurMapperFPU	=DrawScanLineGouraudNoZBufferZWriteTrans_AsmX86FPU;
						}
					}
					else
					{
						if(Flags & DRV_RENDER_NO_ZWRITE)
						{
							CurMapperFPU	=DrawScanLineGouraudZBufferNoZWriteTrans_AsmX86FPU;
						}
						else
						{
							CurMapperFPU	=DrawScanLineGouraudZBufferTrans_AsmX86FPU;
						}
					}
	//			}
			}
			else
			{
				if(Flags & DRV_RENDER_NO_ZMASK)
				{
					if(Flags & DRV_RENDER_NO_ZWRITE)
					{
						if(PolyMode)
						{
							CurMapperFPU	=DrawScanLineGouraudNoZTrans_AsmX86FPU;
						}
						else
						{
							CurMapperFPU	=DrawScanLineGouraudNoZ_AsmX86FPU;
						}
					}
					else
					{
						if(PolyMode)
						{
							CurMapperFPU	=DrawScanLineGouraudNoZBufferZWriteTrans_AsmX86FPU;
						}
						else
						{
							CurMapperFPU	=DrawScanLineGouraudNoZBufferZWrite_AsmX86FPU;
						}
					}
				}
				else
				{
					if(Flags & DRV_RENDER_NO_ZWRITE)
					{
						if(PolyMode)
						{
							CurMapperFPU	=DrawScanLineGouraudZBufferNoZWriteTrans_AsmX86FPU;
						}
						else
						{
							CurMapperFPU	=DrawScanLineGouraudZBufferNoZWrite_AsmX86FPU;
						}
					}
					else
					{
						if(PolyMode)
						{
							CurMapperFPU	=DrawScanLineGouraudZBufferTrans_AsmX86FPU;
						}
						else
						{
							CurMapperFPU	=DrawScanLineGouraudZBuffer_AsmX86FPU;
						}
					}
				}
			}
		}
		if(NumPoints==3)
		{
			TriVerts[0]	=Pnts[0];
			TriVerts[1]	=Pnts[1];
			TriVerts[2]	=Pnts[2];
			TriVerts[0].u	*=GWMask;
			TriVerts[0].v	*=GHMask;
			TriVerts[1].u	*=GWMask;
			TriVerts[1].v	*=GHMask;
			TriVerts[2].u	*=GWMask;
			TriVerts[2].v	*=GHMask;

			TmapTriangle_16(TriVerts);
		}
		else
		{
			for(i=0;i < NumPoints-2;i++)
			{
				TriVerts[0]	=Pnts[0];
				TriVerts[1]	=Pnts[1+i];
				TriVerts[2]	=Pnts[2+i];

				TriVerts[0].u	*=GWMask;
				TriVerts[0].v	*=GHMask;
				TriVerts[1].u	*=GWMask;
				TriVerts[1].v	*=GHMask;
				TriVerts[2].u	*=GWMask;
				TriVerts[2].v	*=GHMask;

				TmapTriangle_16(TriVerts);
			}
		}
		bStipple	=FALSE;
		return	GE_TRUE;
	}

}


#if 0
BOOL DRIVERCC BlitDecal(geRDriver_THandle *THandle, RECT *SRect, int32 x, int32 y)
{
	S32		w, h, Width, Height, Stride;

	return GE_TRUE;

	if(x >= ClientWindow.PixelPitch)
		return GE_TRUE;
	if(y >= ClientWindow.Height)
		return GE_TRUE;

	if(!THandle)
	{
		SetLastDrvError(DRV_ERROR_INVALID_PARMS, "SOFT_DrawDecal:  NULL THandle parm.");
		return FALSE;
	}
	
	if(THandle->Id < 0 || THandle->Id > MAX_DECALS || !Decals[THandle->Id].Used)
	{
		SetLastDrvError(DRV_ERROR_GENERIC, "SOFT_DrawDecal:  Invalid decal.");
		return FALSE;
	}

	if(ProcessorInfo.Has3DNow)
	{
		U8		*BitPtr;
		U32		*pScrPtr2;
		DRV_RGB	*PalPtr	=THandle->Palette;

		BitPtr	=(U8 *)Decals[THandle->Id].BitPtr;
		Width	=Stride	=Decals[THandle->Id].Width;
		Height	=Decals[THandle->Id].Height;
		
		if(SRect)
		{
			BitPtr += SRect->top*Width + SRect->left;
			Height = SRect->bottom - SRect->top;
			Width = SRect->right - SRect->left;
		}

		if(x < 0)
		{
			if(x+Width <= 0)
				return GE_TRUE;
			BitPtr -= x;
			Width += x;
			x=0;
		}

		if(y < 0)
		{
			if(y+Height <= 0)
				return GE_TRUE;
			BitPtr -= (y*Stride);
			Height += y;
			y=0;
		}
		
		if(x + Width >= (ClientWindow.PixelPitch-1))
			Width -= (x+Width) - (ClientWindow.PixelPitch-1);

		if(y + Height >= (ClientWindow.Height-1))
			Height -=  (y+Height) - (ClientWindow.Height-1);

		pScrPtr32bpp	=(U32*)(ClientWindow.Buffer);
		pScrPtr32bpp	=&pScrPtr32bpp[y *ClientWindow.PixelPitch + x];
		pScrPtr2		=pScrPtr32bpp;

		Stride -= Width;

		for (h=0; h< Height; h++)
		{
			for (w=0; w< Width; w++)
			{
				U8	idx	=*BitPtr++;
				if(idx != 0xff)
					*pScrPtr32bpp	=*((U32 *)&PalPtr[idx]);
				
				pScrPtr32bpp++;
			}
			BitPtr	+=Stride;
			pScrPtr2+=ClientWindow.PixelPitch;
			pScrPtr32bpp	=pScrPtr2;
		}
	}
	else
	{
		U16		*BitPtr;
		U16		*pScrPtr2;

		BitPtr = Decals[THandle->Id].BitPtr;
		Width = Stride = Decals[THandle->Id].Width;
		Height = Decals[THandle->Id].Height;
		
		if(SRect)
		{
			BitPtr += SRect->top*Width + SRect->left;
			Height = SRect->bottom - SRect->top;
			Width = SRect->right - SRect->left;
		}

		if(x < 0)
		{
			if(x+Width <= 0)
				return GE_TRUE;
			BitPtr -= x;
			Width += x;
			x=0;
		}

		if(y < 0)
		{
			if(y+Height <= 0)
				return GE_TRUE;
			BitPtr -= (y*Stride);
			Height += y;
			y=0;
		}
		
		if(x + Width >= (ClientWindow.PixelPitch-1))
			Width -= (x+Width) - (ClientWindow.PixelPitch-1);

		if(y + Height >= (ClientWindow.Height-1))
			Height -=  (y+Height) - (ClientWindow.Height-1);

		pScrPtr16bpp = (U16*)(ClientWindow.Buffer);
		pScrPtr16bpp = &pScrPtr16bpp[y *ClientWindow.PixelPitch + x];
		pScrPtr2 = pScrPtr16bpp;

		Stride -= Width;

		for (h=0; h< Height; h++)
		{
			for (w=0; w< Width; w++)
			{
				U16 Color = *BitPtr++;
				if(Color != 0xffff)
					*pScrPtr16bpp = Color;
				
				pScrPtr16bpp++;
			}
			BitPtr += Stride;
			pScrPtr2 += ClientWindow.PixelPitch;
			pScrPtr16bpp = pScrPtr2;
		}
	}
	return GE_TRUE;
}
#endif

static S32		BWidth, BHeight, BStride;
static S32		DrawWidth, DrawHeight;
static S32		EbpAdd, EdiAdd;
static U16		*BBitPtr16;
static U8		*BBitPtr;
extern VidModeList	*cmode;
extern RECT			FrontRect;

BOOL DRIVERCC DrawDecal(geRDriver_THandle *THandle, RECT *SRect, int32 x, int32 y)
{
	U32	*PalPtr;

	if(!bActive)
	{
		return	GE_TRUE;
	}
	/*
	if(SRect)
	{
		OutputDebugString("What the !?!?!\n");
		return	DDrawBlitDecal(Decals[Bitmap->Id].Surface, SRect, x, y);
	}
	else
	{
		RECT	SrcRect;
		SrcRect.left=0;
		SrcRect.top=0;
		SrcRect.right=Bitmap->Width-1;
		SrcRect.bottom=Bitmap->Height-1;
		OutputDebugString("NULL rect2\n");
//		return	DDrawBlitDecal(Decals[Bitmap->Id].Surface, &SrcRect, x, y);
		DDrawBlitDecal(Decals[Bitmap->Id].Surface, &SrcRect, x, y);

	}
	*/
//	POINT	uleft;
//	uleft.x	=x;
//	uleft.y	=y;

/*	if(!bWindowed)
	{
		if(cmode->flags & STRETCHMODE)
		{
			if(!PtInRect(&FrontRect, uleft))
			{
				DDrawBlitDecalToFront(Decals[Bitmap->Id].Surface, SRect, x, y);
				return	GE_TRUE;
			}
		}
		else if((!bBackLocked && !bWindowed))
		{
			DDrawBlitDecalDelayed(Decals[Bitmap->Id].Surface, SRect, x, y);
			return	GE_TRUE;
//		}
	}
//	return GE_TRUE;
*/
	
	BWidth		=THandle->Width;
	BHeight		=THandle->Height;
	DrawWidth	=THandle->Width;
	DrawHeight	=THandle->Height;
	
	if(ProcessorInfo.Has3DNow )
	{
		BBitPtr		=(U8 *)THandle->BitPtr[0];

		if ( ! THandle->PalHandle )
			return 0;

		PalPtr	=(U32 *)THandle->PalHandle->BitPtr[0];

		if(SRect)
		{
			BBitPtr		+=SRect->top * DrawWidth + SRect->left;
			DrawHeight	=(SRect->bottom - SRect->top);
			DrawWidth	=(SRect->right - SRect->left);
		}

		if(x < 0)
		{
			if(x + DrawWidth <= 0)
			{
				return	GE_TRUE;
			}
			BBitPtr		-=x;
			DrawWidth	+=x;
			x			=0;
		}

		if(y < 0)
		{
			if(y + DrawHeight <= 0)
			{
				return	GE_TRUE;
			}
			BBitPtr		-=y * BWidth;
			DrawHeight	+=y;
			y			=0;
		}
		
		if(x >= ClientWindow.Width)
		{
			return	GE_TRUE;
		}
		if(y >= ClientWindow.Height)
		{
			return	GE_TRUE;
		}

		if(x + DrawWidth >= (ClientWindow.Width-1))
			DrawWidth -= (x+DrawWidth) - (ClientWindow.Width-1);

		if(y + DrawHeight >= (ClientWindow.Height-1))
			DrawHeight -=  (y+DrawHeight)- (ClientWindow.Height-1);

		if(DrawWidth <= 0)
			return GE_TRUE;
		
		if(DrawHeight <= 0)
			return GE_TRUE;

		pScrPtr32bpp	=(U32 *)(ClientWindow.Buffer);
		pScrPtr32bpp	=&pScrPtr32bpp[y * ClientWindow.Width + x];

		__asm
		{
			push ecx
			push esi
			push edi
			push ebp

			mov	ebx,PalPtr

			mov ebp, pScrPtr32bpp
			mov edi, BBitPtr

			mov ecx, DrawWidth

			mov edx, ClientWindow.PixelPitch
			shl ecx,2
			sub edx, ecx
			mov EbpAdd, edx

			mov edx, BWidth
			mov ecx, DrawHeight
			sub edx, DrawWidth
			mov EdiAdd, edx


		NextHeight:
		
			push ecx
			mov ecx, DrawWidth

			lea	ebp,[4*ecx+ebp]

			add edi, ecx
//			shl ecx, 2
//			add ebp, ecx

			neg ecx

			NextWidth:
				xor	eax,eax
				mov al, [edi+ecx]
				
				cmp al,0ffh
				je Skip

//				cmp al,01h
//				je Skip

				mov	eax,[ebx+eax*4]

				mov [ebp+ecx*4], eax

			Skip:
				inc ecx
				jnz NextWidth

			add ebp, EbpAdd
			add edi, EdiAdd

			pop ecx
			
			sub ecx, 1
			jnz NextHeight

			pop ebp
			pop edi
			pop esi
			pop ecx
		}
	}
	else
	{
		BBitPtr16	=THandle->BitPtr[0];
		if(SRect)
		{
			BBitPtr16	+=SRect->top * DrawWidth + SRect->left;
			DrawHeight	=(SRect->bottom - SRect->top);
			DrawWidth	=(SRect->right - SRect->left);
		}

		if(x < 0)
		{
			if(x + DrawWidth <= 0)
			{
				return	GE_TRUE;
			}
			BBitPtr16	-=x;
			DrawWidth	+=x;
			x			=0;
		}

		if(y < 0)
		{
			if(y + DrawHeight <= 0)
			{
				return	GE_TRUE;
			}
			BBitPtr16	-=y*BWidth;
			DrawHeight	+=y;
			y			=0;
		}
		
		if(x >= ClientWindow.Width)
		{
			return	GE_TRUE;
		}
		if(y >= ClientWindow.Height)
		{
			return	GE_TRUE;
		}

		if(x + DrawWidth >= (ClientWindow.Width - 1))
		{
			DrawWidth	-=(x + DrawWidth) - (ClientWindow.Width - 1);
		}
		if(y + DrawHeight >= (ClientWindow.Height - 1))
		{
			DrawHeight	-=(y + DrawHeight) - (ClientWindow.Height - 1);
		}

		if(DrawWidth <= 0)
		{
			return	GE_TRUE;
		}
		if(DrawHeight <= 0)
		{
			return	GE_TRUE;
		}

		pScrPtr16bpp	=(U16 *)(ClientWindow.Buffer);
		pScrPtr16bpp	=&pScrPtr16bpp[y * (ClientWindow.PixelPitch >> 1) + x];

		__asm
		{
			push ecx
			push esi
			push edi
			push ebp

			mov ebp, pScrPtr16bpp
			mov edi, BBitPtr16

			mov ecx, DrawWidth

			mov edx, ClientWindow.PixelPitch
			sub edx, ecx
			sub edx, ecx
			mov EbpAdd, edx

			mov edx, BWidth
			add edx, BWidth
			sub edx, ecx
			sub edx, ecx
			mov EdiAdd, edx

			mov ecx, DrawHeight

		NextHeightA:
		
			push ecx
			mov ecx, DrawWidth

			shl ecx, 1
			add ebp, ecx
			add edi, ecx

			neg ecx

			NextWidthA:
				mov ax, [edi+ecx]
				
				cmp ax, 0x1
				je SkipA

				mov [ebp+ecx], ax

			SkipA:
				add ecx, 2
				jnz NextWidthA

			add ebp, EbpAdd
			add edi, EdiAdd

			pop ecx
			
			sub ecx, 1
			jnz NextHeightA

			pop ebp
			pop edi
			pop esi
			pop ecx
		}
	}
	return GE_TRUE;
}

static Fixed16	UStep, VStep, ZStep;
static int32	Length;
static uint16	Color;
static int32	ESPSave;

void ClearZBuffer(DRV_Window *Window)
{
	//hahhah memcpy is faster than anything I can do
	//return;
	S32	ZBSize;
/*	if(0 && ProcessorInfo.Has3DNow)
	{
		ZBSize	=(Window->Width*Window->Height)>>5;

		_asm	_emit	0x0f	\
		_asm	_emit	0x0e	\
		_asm	mov		eax,[ZBuffer]
		_asm	mov		ebx,[ZBuffer]
		_asm	mov		ecx,dword ptr[ZBSize]
//		_asm	_emit	0x0f				\
//		_asm	_emit	0x0d				\
//		_asm	_emit	0x00
		_asm	movq	mm0,[Zero]
		_asm	movq	mm1,[Zero]
//		_asm	movq	mm2,[Zero]
//		_asm	movq	mm3,[Zero]
//		_asm	movq	mm4,[Zero]
//		_asm	movq	mm5,[Zero]
//		_asm	movq	mm6,[Zero]
//		_asm	movq	mm7,[Zero]
//		_asm	_emit	0x0f				\
//		_asm	_emit	0x0d				\
//		_asm	_emit	0x40				\
//		_asm	_emit	0x20				
ClearLoop3DNow:
		_asm	movq	[eax],mm0
		_asm	movq	[ebx+32],mm1
//		_asm	_emit	0x0f				\
//		_asm	_emit	0x0d				\
//		_asm	_emit	0x40				\
//		_asm	_emit	0x40				
		_asm	movq	[eax+8],mm0
		_asm	movq	[ebx+40],mm1
		_asm	movq	[eax+16],mm0
		_asm	movq	[ebx+48],mm1
		_asm	movq	[eax+24],mm0
		_asm	movq	[ebx+56],mm1
		_asm	add		eax,64
		_asm	add		ebx,64
//		_asm	_emit	0x0f				\
//		_asm	_emit	0x0d				\
//		_asm	_emit	0x40				\
//		_asm	_emit	0x60				
		_asm	dec		ecx
		_asm	jnz		ClearLoop3DNow
		_asm	_emit	0x0f	\
		_asm	_emit	0x0e	\
	}
	else if(0 && ProcessorInfo.HasMMX)
	{
		ZBSize	=(Window->Width*Window->Height)>>4;

		_asm	emms
		_asm	mov		eax,[ZBuffer]
		_asm	mov		ecx,dword ptr[ZBSize]
		_asm	movq	mm0,[Zero]
ClearLoop:
		_asm	movq	[eax],mm0
		_asm	movq	[eax+8],mm0
		_asm	movq	[eax+16],mm0
		_asm	movq	[eax+24],mm0
		_asm	add		eax,32
		_asm	dec		ecx
		_asm	jnz		ClearLoop
		_asm	emms
	}
	else
*/	{
		ZBSize = (Window->Width*Window->Height)<<1;
		memset(ZBuffer, 0, ZBSize);
	}
}

//===========================================================================================
//	Render Sys init, de-init code
//===========================================================================================
BOOL RenderInit(DRV_Window *Window)
{
//	U32	OldFlags	=0;

	ZBuffer	=(U16 *)malloc(ClientWindow.Width * ClientWindow.Height * 2);

/*	if(!VirtualProtect((U8 *)ZBuffer,
		(ClientWindow.Width * ClientWindow.Height)*2,
		PAGE_READWRITE | PAGE_NOCACHE,
		&OldFlags))
	{
		ErrorPrintf("Failed to set zbuffer page uncacheable\n");
	}
*/
	if(!ZBuffer)
	{
		SetLastDrvError(DRV_ERROR_NO_MEMORY, "SOFT_RenderInit:  Not enough memory for ZBuffer.");
		return FALSE;
	}
	
	#define Mul(a) (uint32)((1.0f/(float)(a))*65536*32768)
	//#define Mul(a) ((uint32)((1.0f/(float)(a))*(1<<31)))

	return GE_TRUE;
}

BOOL RenderShutdown(void)
{
	if(ZBuffer)
	{
		free(ZBuffer);
		ZBuffer	=NULL;
	}
	return	GE_TRUE;
}

