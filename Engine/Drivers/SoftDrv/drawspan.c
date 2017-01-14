/****************************************************************************************/
/*  drawspan.c                                                                          */
/*                                                                                      */
/*  Author:       Ken Baird                                                             */
/*  Description:  Mostly unused code, a few needed vars, some renderstates that can     */
/*                be used for post poly zfill, and some z correctors with more accuracy */
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
#include <Assert.h>
#include <math.h>

#include "BaseType.h"
#include "Render.h"
#include "SoftDrv.h"
#include "drawspan.h"


double	MipMagic, MipMagic2;

int32	R1, B1, G1, R2, G2, B2;
int32	RR1, RR2, GG1, GG2, BB1, BB2;
int32	StepR, StepG, StepB;
int32	UDist, VDist;
int32	U1=0, V1=0, NumSpans=0;
int32	CKeyTest=0;

geFloat			FloatTemp, FTemp0, FTemp1, FTemp2;
geFloat			FTemp3, FTemp4, FTemp5, FTemp6, FTemp7, FTemp8;
float const		One			=1.0f;
float const		Two			=2.0f;

extern	U32 UMask, VShift, VMask;
int32	ZDelta, ZVal;
float	ZBufferPrec = (float)-ZBUFFER_PREC;
__int64	RedDelta, GreenDelta, BlueDelta;
uint32	NumASpans, RemainingCount;
double	DeltaU, DeltaV;
uint32	UFixed, VFixed;
uint8	*pTex;



void DrawSpan16_AsmX86FPU(int32 x1, int32 x2, int32 y)
{
	_asm
	{
		mov		eax,x1
		mov		ecx,x2
		sub		ecx,eax
		jle		Return16

		mov		edi,[GBitPtr16]
		inc		ecx
		shr		edi, 1					; keep texture >>1
		mov		pTex,edi

		fild	[y]						; y

		mov		edi, ClientWindow.Buffer
		mov		eax, y
		imul	eax, ClientWindow.Width
		add		eax, x1
		shl		eax, 1
		add		edi, eax
		mov		eax,ecx
		shr		ecx,4
		and		eax,15
		_emit 75h
		_emit 06h
		dec		ecx
		mov		eax,16

		mov		[NumASpans],ecx
		mov		[RemainingCount],eax

		fild [x1]						; x    y

		//decoder won't keep up with these huge instructions
		//need to find some int instructions to cram in here somewhere
		fld		[UDivZStepY]			; UZdY x    y
		fld		[UDivZStepX]			; UZdX UZdY x    y
		fmul	st,st(2)				; UZX  UZdY x    y
		fld		[VDivZStepY]			; VZdY UZX  UZdY x    y
		fld		[VDivZStepX]			; VZdX VZdY UZX  UZdY x    y
		fxch	st(3)					; UZdy VZdY UZX  VZdX x    y
		fmul	st,st(5)				; UZY  VZdY UZX  VZdX x    y
		fxch	st(2)					; UZX  VZdY UZY  VZdX x    y
		fadd	[UDivZOrigin]			; UZXS VZdY UZY  VZdX x    y
		fxch	st(3)					; VZdX VZdY UZY  UZXS x    y
		fmul	st,st(4)				; VZX  VZdY UZY  UZXS x    y
		fxch	st(2)					; UZY  VZdY VZX  UZXS x    y
		faddp	st(3),st				; VZdY VZX  UZ   x    y
		fmul	st,st(4)				; VZY  VZX  UZ   x    y
		fxch	st(1)					; VZX  VZY  UZ   x    y
		fadd	[VDivZOrigin]			; VZXS VZY  UZ   x    y
		fld		[ZiStepX]				; ZdX  VZXS VZY  UZ   x    y
		fmulp	st(4),st				; VZXS VZY  UZ   ZX   y
		faddp	st(1),st				; VZ   UZ   ZX   y
		fld		[ZiStepY]				; ZdY  VZ   UZ   ZX   y
		fmulp	st(4),st				; VZ   UZ   ZX   ZY
		fxch	st(2)					; ZX   UZ   VZ   ZY
		fadd	[ZiOrigin]				; ZXS  UZ   VZ   ZY

		//room for two cycles of int instructions here

		faddp	st(3),st				; UZ   VZ   Zi
		fld1							; 1    UZ   VZ   Zi
		fdiv	st,st(3)				; ZL   UZ   VZ   Zi

		//room for 18 cycles of int instructions here

		fld		st						; ZL   ZL   UZ   VZ   Zi
		fmul	st,st(3)				; VL   ZL   UZ   VZ   Zi
		fxch	st(4)					; Zi   ZL   UZ   VZ   VL
		fadd	[Zi16StepX]				; ZRi  ZL   UZ   VZ   VL
		fxch	st(1)					; ZL   ZRi  UZ   VZ   VL
		fmul	st,st(2)				; UL   ZRi  UZ   VZ   VL
		fxch	st(3)					; VZ   ZRi  UZ   UL   VL
		fadd	[VDivZ16StepX]			; VZR  ZRi  UZ   UL   VL
		fxch	st(2)					; UZ   ZRi  VZR  UL   VL
		fadd	[UDivZ16StepX]			; UZR  ZRi  VZR  UL   VL
		fld1							; 1    UZR  ZRi  VZR  UL   VL
		fdiv	st,st(2)				; ZR   UZR  ZRi  VZR  UL   VL

		//room for 18 cycles of int stuff here

		fld		st						; ZR   ZR   UZR  ZRi  VZR  UL   VL
		fmul	st,st(4)				; VR   ZR   UZR  ZRi  VZR  UL   VL
		fxch	st(1)					; ZR   VR   UZR  ZRi  VZR  UL   VL
		//fmul stall one cycle
		fmul	st,st(2)				; UR   VR   UZR  ZRi  VZR  UL   VL

		test	ecx,ecx
		jz		HandleLeftoverPixels16

SpanLoop16:
		//need one more stack spot
		fstp	dword ptr[FloatTemp]	; VR   UZR  ZRi  VZR  UL   VL
		fld		st(4)					; UL   VR   UZR  ZRi  VZR  UL   VL
		fmul	[GLMapMulU]				; ULL  VR   UZR  ZRi  VZR  UL   VL
		fld		st(5)					; UL   ULL  VR   UZR  ZRi  VZR  UL   VL
		fadd	qword ptr[MipMagic]		; ULk  ULL  VR   UZR  ZRi  VZR  UL   VL
		fxch	st(1)					; ULL  ULk  VR   UZR  ZRi  VZR  UL   VL
		fadd	qword ptr[MipMagic]		; ULLk ULk  VR   UZR  ZRi  VZR  UL   VL
		fxch	st(1)					; ULk  ULLk VR   UZR  ZRi  VZR  UL   VL
		fstp	qword ptr[Bucket]		; ULLk VR   UZR  ZRi  VZR  UL   VL
		fstp	qword ptr[Bucket2]		; VR   UZR  ZRi  VZR  UL   VL

		mov		ebx,dword ptr[Bucket]
		mov		eax,dword ptr[Bucket2]

		fld		st(5)					; VL   VR   UZR  ZRi  VZR  UL   VL
		fmul	[GLMapMulV]				; VLL  VR   UZR  ZRi  VZR  UL   VL

		add		ebx,dword ptr[UAdjust]
		add		eax,dword ptr[UAdjustL]

		mov		[U1],ebx
		mov		[UFixed],eax

		fld		st(6)					; VL   VLL  VR   UZR  ZRi  VZR  UL   VL
		fadd	qword ptr[MipMagic]		; VLk  VLL  VR   UZR  ZRi  VZR  UL   VL
		fxch	st(1)					; VLL  VLk  VR   UZR  ZRi  VZR  UL   VL
		fadd	qword ptr[MipMagic]		; VLLk VLk  VR   UZR  ZRi  VZR  UL   VL
		fxch	st(1)					; VLk  VLLk VR   UZR  ZRi  VZR  UL   VL
		fstp	qword ptr[Bucket]		; VLLk VR   UZR  ZRi  VZR  UL   VL
		fstp	qword ptr[Bucket2]		; VR   UZR  ZRi  VZR  UL   VL
		fsubr	st(5),st				; VR   UZR  ZRi  VZR  UL   dV

		mov		ebx,dword ptr[Bucket]
		mov		eax,dword ptr[Bucket2]

		fld		dword ptr[FloatTemp]	; UR   VR   UZR  ZRi  VZR  UL   dV

		add		ebx,dword ptr[VAdjust]
		add		eax,dword ptr[VAdjustL]

		mov		[V1],ebx
		mov		[VFixed],eax

		fsubr	st(5),st				; UR   VR   UZR  ZRi  VZR  dU   dV
		fxch	st(6)					; dV   VR   UZR  ZRi  VZR  dU   UR
		fadd	qword ptr[MipMagic2]	; dVk  VR   UZR  ZRi  VZR  dU   UR
		fxch	st(5)					; dU   VR   UZR  ZRi  VZR  dVk  UR
		fadd	qword ptr[MipMagic2]	; dUk  VR   UZR  ZRi  VZR  dVk  UR
		fxch	st(5)					; dVk  VR   UZR  ZRi  VZR  dUk  UR
		fstp	qword ptr[DeltaV]		; VR   UZR  ZRi  VZR  dUk  UR
		fxch	st(5)					; UR   UZR  ZRi  VZR  dUk  VR
		//gotta do this to get em lined back up right
		fxch	st(4)					; dUk  UZR  ZRi  VZR  UR   VR
		fstp	qword ptr[DeltaU]		; UZR  ZRi  VZR  UR   VR

		//right becomes left			; UZL  ZLi  VZL  UL   VL
		fadd	[UDivZ16StepX]			; UZR  ZLi  VZL  UL   VL
		fxch	st(1)					; ZLi  UZR  VZL  UL   VL
		fadd	[Zi16StepX]				; ZRi  UZR  VZL  UL   VL
		fxch	st(2)					; VZL  UZR  ZRi  UL   VL
		fadd	[VDivZ16StepX]			; VZR  UZR  ZRi  UL   VL
		fxch	st(2)					; ZRi  UZR  VZR  UL   VL
		fxch	st(1)					; UZR  ZRi  VZR  UL   VL
		//need those fxch to line things up for loops (bad)

		fld1							; 1    UZR  ZRi  VZR  UL   VL


		fdiv	st,st(2)			; ZR   UZR  ZRi  VZR  UL   VL

		mov		ebx,dword ptr[U1]
		mov		eax,dword ptr[V1]

		add		ebx,dword ptr[UAdjust2]
		add		eax,dword ptr[VAdjust2]

		mov		ecx,16
		mov		dword ptr[Bucket],ebx

		sub		ecx,[VShift]
		mov		dword ptr[Bucket2],eax

		shr		eax,cl

		push	ebp
		shr		ebx,16

		and		eax,[GHMaskShifted]
		mov		esi,[pTex]

		and		ebx,[GWMask]
		add		esi,eax

		mov		ecx,[VShift]
		add		esi,ebx

		mov		edx,dword ptr[Bucket2]
		mov		ebp,dword ptr[DeltaV]

		mov		ebx,dword ptr[Bucket]

		//do 16 pixels

		add		edx,ebp
		mov		ax,[2*esi]

		mov		esi,edx
		add		ebx,dword ptr[DeltaU]

		shl		esi,cl
		and		ebx,[GWMaskShifted]

		and		esi,[GHMaskShifted16]

		add		esi,ebx
		mov		[edi+0],ax

		shr		esi,16
		add		edx,ebp

		add		esi,pTex
		add		ebx,dword ptr[DeltaU]

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,[GWMaskShifted]
		mov		[edi+2],ax

		shl		esi,cl
		add		edx,ebp

		and		esi,[GHMaskShifted16]

		add		esi,ebx
		shr		esi,16

		add		ebx,dword ptr[DeltaU]
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,[GWMaskShifted]
		mov		[edi+4],ax

		shl		esi,cl
		add		edx,ebp

		and		esi,[GHMaskShifted16]

		add		esi,ebx
		shr		esi,16

		add		ebx,dword ptr[DeltaU]
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,[GWMaskShifted]
		mov		[edi+6],ax

		shl		esi,cl
		add		edx,ebp

		and		esi,[GHMaskShifted16]

		add		esi,ebx
		shr		esi,16

		add		ebx,dword ptr[DeltaU]
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,[GWMaskShifted]
		mov		[edi+8],ax

		shl		esi,cl
		add		edx,ebp

		and		esi,[GHMaskShifted16]

		add		esi,ebx
		shr		esi,16

		add		ebx,dword ptr[DeltaU]
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,[GWMaskShifted]
		mov		[edi+10],ax

		shl		esi,cl
		add		edx,ebp

		and		esi,[GHMaskShifted16]

		add		esi,ebx
		shr		esi,16

		add		ebx,dword ptr[DeltaU]
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,[GWMaskShifted]
		mov		[edi+12],ax

		shl		esi,cl
		add		edx,ebp

		and		esi,[GHMaskShifted16]

		add		esi,ebx
		shr		esi,16

		add		ebx,dword ptr[DeltaU]
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,[GWMaskShifted]
		mov		[edi+14],ax

		shl		esi,cl
		add		edx,ebp

		and		esi,[GHMaskShifted16]

		add		esi,ebx
		shr		esi,16

		add		ebx,dword ptr[DeltaU]
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,[GWMaskShifted]
		mov		[edi+16],ax

		shl		esi,cl
		add		edx,ebp

		and		esi,[GHMaskShifted16]

		add		esi,ebx
		shr		esi,16

		add		ebx,dword ptr[DeltaU]
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,[GWMaskShifted]
		mov		[edi+18],ax

		shl		esi,cl
		add		edx,ebp

		and		esi,[GHMaskShifted16]

		add		esi,ebx
		shr		esi,16

		add		ebx,dword ptr[DeltaU]
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,[GWMaskShifted]
		mov		[edi+20],ax

		shl		esi,cl
		add		edx,ebp

		and		esi,[GHMaskShifted16]

		add		esi,ebx
		shr		esi,16

		add		ebx,dword ptr[DeltaU]
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,[GWMaskShifted]
		mov		[edi+22],ax

		shl		esi,cl
		add		edx,ebp

		and		esi,[GHMaskShifted16]

		add		esi,ebx
		shr		esi,16

		add		ebx,dword ptr[DeltaU]
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,[GWMaskShifted]
		mov		[edi+24],ax

		shl		esi,cl
		add		edx,ebp

		and		esi,[GHMaskShifted16]

		add		esi,ebx
		shr		esi,16

		add		ebx,dword ptr[DeltaU]
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,[GWMaskShifted]
		mov		[edi+26],ax

		shl		esi,cl
		add		edx,ebp

		and		esi,[GHMaskShifted16]

		add		esi,ebx
		shr		esi,16

		add		ebx,dword ptr[DeltaU]
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,[GWMaskShifted]
		mov		[edi+28],ax

		shl		esi,cl
		add		edx,ebp

		and		esi,[GHMaskShifted16]

		add		esi,ebx
		shr		esi,16

		add		ebx,dword ptr[DeltaU]
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		pop     ebp
		mov		[edi+30],ax

		;get corrected right side deltas; st0  st1  st2  st3  st4  st5  st6  st7
										; ZR   UZR  ZRi  VZR  UL   VL
		fld		st						; ZR   ZR   UZR  ZRi  VZR  UL   VL
		fmul	st,st(4)				; VR   ZR   UZR  ZRi  VZR  UL   VL
		fxch	st(1)					; ZR   VR   UZR  ZRi  VZR  UL   VL
		fmul	st,st(2)				; UR   VR   UZR  ZRi  VZR  UL   VL

		add		edi,32
		dec		[NumASpans]			; dec num affine spans
		jnz		SpanLoop16				; loop back

HandleLeftoverPixels16:

		mov		esi,[pTex]


		cmp		[RemainingCount],0
		jz		FPUReturn16

		//need one more stack spot
		fstp	dword ptr[FloatTemp]	; VR   UZR  ZRi  VZR  UL   VL
		fld		st(4)					; UL   VR   UZR  ZRi  VZR  UL   VL
		fmul	[GLMapMulU]				; ULL  VR   UZR  ZRi  VZR  UL   VL
		fld		st(5)					; UL   ULL  VR   UZR  ZRi  VZR  UL   VL
		fadd	qword ptr[MipMagic]		; ULk  ULL  VR   UZR  ZRi  VZR  UL   VL
		fxch	st(1)					; ULL  ULk  VR   UZR  ZRi  VZR  UL   VL
		fadd	qword ptr[MipMagic]		; ULLk ULk  VR   UZR  ZRi  VZR  UL   VL
		fxch	st(1)					; ULk  ULLk VR   UZR  ZRi  VZR  UL   VL
		fstp	qword ptr[Bucket]		; ULLk VR   UZR  ZRi  VZR  UL   VL
		fstp	qword ptr[Bucket2]		; VR   UZR  ZRi  VZR  UL   VL

		mov		ebx,dword ptr[Bucket]
		mov		eax,dword ptr[Bucket2]

		fld		st(5)					; VL   VR   UZR  ZRi  VZR  UL   VL
		fmul	[GLMapMulV]				; VLL  VR   UZR  ZRi  VZR  UL   VL

		add		ebx,dword ptr[UAdjust]
		add		eax,dword ptr[UAdjustL]

		mov		[U1],ebx
		mov		[UFixed],eax

		fld		st(6)					; VL   VLL  VR   UZR  ZRi  VZR  UL   VL
		fadd	qword ptr[MipMagic]		; VLk  VLL  VR   UZR  ZRi  VZR  UL   VL
		fxch	st(1)					; VLL  VLk  VR   UZR  ZRi  VZR  UL   VL
		fadd	qword ptr[MipMagic]		; VLLk VLk  VR   UZR  ZRi  VZR  UL   VL
		fxch	st(1)					; VLk  VLLk VR   UZR  ZRi  VZR  UL   VL
		fstp	qword ptr[Bucket]		; VLLk VR   UZR  ZRi  VZR  UL   VL
		fstp	qword ptr[Bucket2]		; VR   UZR  ZRi  VZR  UL   VL

		mov		ebx,dword ptr[Bucket]
		mov		eax,dword ptr[Bucket2]

		fld		dword ptr[FloatTemp]	; UR   VR   UZR  ZRi  VZR  UL   dV

		add		ebx,dword ptr[VAdjust]
		add		eax,dword ptr[VAdjustL]

		mov		[V1],ebx
		mov		[VFixed],eax

		dec		[RemainingCount]
		jz		OnePixelSpan16


		//must get rid of this wasted time
		fstp	[FloatTemp]				; inv. inv. inv. inv. UL   VL
		fstp	[FloatTemp]				; inv. inv. inv. UL   VL
		fstp	[FloatTemp]				; inv. inv. UL   VL
		fstp	[FloatTemp]				; inv. UL   VL
		fstp	[FloatTemp]				; UL   VL
		fild	[y]						; y    UL   VL
		fild	[x2]					; xr   y    UL   VL

		fld		[UDivZStepY]			; UZdY xr   y    UL   VL
		fld		[UDivZStepX]			; UZdX UZdY xr   y    UL   VL
		fmul	st,st(2)				; UZX  UZdY xr   y    UL   VL
		fld		[VDivZStepY]			; VZdY UZX  UZdY xr   y    UL   VL
		fld		[VDivZStepX]			; VZdX VZdY UZX  UZdY xr   y    UL   VL
		fxch	st(3)					; UZdy VZdY UZX  VZdX xr   y    UL   VL
		fmul	st,st(5)				; UZY  VZdY UZX  VZdX xr   y    UL   VL
		fxch	st(2)					; UZX  VZdY UZY  VZdX xr   y    UL   VL
		fadd	[UDivZOrigin]			; UZXS VZdY UZY  VZdX xr   y    UL   VL
		fxch	st(3)					; VZdX VZdY UZY  UZXS xr   y    UL   VL
		fmul	st,st(4)				; VZX  VZdY UZY  UZXS xr   y    UL   VL
		fxch	st(2)					; UZY  VZdY VZX  UZXS xr   y    UL   VL
		faddp	st(3),st				; VZdY VZX  UZ   xr   y    UL   VL
		fmul	st,st(4)				; VZY  VZX  UZ   xr   y    UL   VL
		fxch	st(1)					; VZX  VZY  UZ   xr   y    UL   VL
		fadd	[VDivZOrigin]			; VZXS VZY  UZ   xr   y    UL   VL
		fld		[ZiStepX]				; ZdX  VZXS VZY  UZ   xr   y    UL   VL
		fmulp	st(4),st				; VZXS VZY  UZ   ZX   y    UL   VL
		faddp	st(1),st				; VZ   UZ   ZX   y    UL   VL
		fld		[ZiStepY]				; ZdY  VZ   UZ   ZX   y    UL   VL
		fmulp	st(4),st				; VZ   UZ   ZX   ZY   UL   VL
		fxch	st(2)					; ZX   UZ   VZ   ZY   UL   VL
		fadd	[ZiOrigin]				; ZXS  UZ   VZ   ZY   UL   VL

		faddp	st(3),st				; UZ   VZ   Zi   UL   VL
		fld1							; 1    UZ   VZ   Zi   UL   VL
		fdiv	st,st(3)				; ZR   UZ   VZ   Zi   UL   VL

		fld		st						; ZR   ZR   UZ   VZ   Zi   UL   VL
		fmul	st,st(3)				; VR   ZR   UZ   VZ   Zi   UL   VL
		fxch	st(1)					; ZR   VR   UZ   VZ   Zi   UL   VL
		fmul	st,st(2)				; UR   VR   UZ   VZ   Zi   UL   VL

		//lazy idiv below... should 1/int mul mul

		; calculate deltas				; st0  st1  st2  st3  st4  st5  st6  st7
		fsubr	st(5),st				; UR   VR   inv. inv. inv. dU   VL
		fxch	st(1)					; VR   UR   inv. inv. inv. dU   VL
		fsubr	st(6),st				; VR   UR   inv. inv. inv. dU   dV
		fxch	st(6)					; dV   UR   inv. inv. inv. dU   VR
		fidiv   dword ptr[RemainingCount];dv   UR   inv. inv. inv. dU   VR
		fadd	qword ptr[MipMagic]		; dvk  UR   inv. inv. inv. dU   VR
		fxch	st(5)					; dU   UR   inv. inv. inv. dvk  VR
		fidiv	dword ptr[RemainingCount];du   UR   inv. inv. inv. dvk  VR
		fadd	qword ptr[MipMagic]		; duk  UR   inv. inv. inv. dvk  VR
		fxch	st(5)					; dvk  UR   inv. inv. inv. duk  VR
		fstp	qword ptr[DeltaV]		; UR   inv. inv. inv. duk  VR
		fxch	st(4)					; duk  inv. inv. inv. UR   VR
		fstp	qword ptr[DeltaU]		; inv. inv. inv. UR   VR
		fld		st(1)					; inv. inv. inv. inv. UR   VR
		fld		st(2)					; inv. inv. inv. inv. inv. UR   VR

		mov		ebx,dword ptr[U1]
		mov		edx,dword ptr[V1]

		mov		ecx,16
		add		ebx,dword ptr[UAdjust2]

		sub		ecx,[VShift]
		add		edx,dword ptr[VAdjust2]

OnePixelSpan16:
		push	ebp

LeftoverLoop16:
		mov		eax,edx
		shr		eax,cl
		mov		ebp,ebx
		and		eax,[GHMaskShifted]
		shr		ebp,16
		and		ebp,[GWMask]
		add		eax,ebp
		add		eax,esi
		mov		ax,[2*eax]
		mov		[edi],ax
		add		ebx,dword ptr[DeltaU]
		add		edi,2
		add		edx,dword ptr[DeltaV]

		dec		[RemainingCount]
		jge		LeftoverLoop16

		pop		ebp

FPUReturn16:
		ffree	st(0)
		ffree	st(1)
		ffree	st(2)
		ffree	st(3)
		ffree	st(4)
		ffree	st(5)
		ffree	st(6)

Return16:
	}
}

void DrawSpan16_8AsmLitX86FPU(int32 x1, int32 x2, int32 y)
{
	_asm
	{
		mov		eax,x1
		mov		ecx,x2
		sub		ecx,eax
		jle		Return16

		mov		edi,[GBitPtr16]
		inc		ecx
		shr		edi, 1					; keep texture >>1
		mov		pTex,edi

		fild	[y]						; y

		mov		edi, ClientWindow.Buffer
		mov		eax, y
		imul	eax, ClientWindow.Width
		add		eax, x1
		shl		eax, 1
		add		edi, eax
		mov		eax,ecx
		shr		ecx,3
		and		eax,7
		_emit 75h
		_emit 06h
		dec		ecx
		mov		eax,8

		mov		[NumASpans],ecx
		mov		[RemainingCount],eax

		fild [x1]						; x    y

		//decoder won't keep up with these huge instructions
		//need to find some int instructions to cram in here somewhere
		fld		[UDivZStepY]			; UZdY x    y
		fld		[UDivZStepX]			; UZdX UZdY x    y
		fmul	st,st(2)				; UZX  UZdY x    y
		fld		[VDivZStepY]			; VZdY UZX  UZdY x    y
		fld		[VDivZStepX]			; VZdX VZdY UZX  UZdY x    y
		fxch	st(3)					; UZdy VZdY UZX  VZdX x    y
		fmul	st,st(5)				; UZY  VZdY UZX  VZdX x    y
		fxch	st(2)					; UZX  VZdY UZY  VZdX x    y
		fadd	[UDivZOrigin]			; UZXS VZdY UZY  VZdX x    y
		fxch	st(3)					; VZdX VZdY UZY  UZXS x    y
		fmul	st,st(4)				; VZX  VZdY UZY  UZXS x    y
		fxch	st(2)					; UZY  VZdY VZX  UZXS x    y
		faddp	st(3),st				; VZdY VZX  UZ   x    y
		fmul	st,st(4)				; VZY  VZX  UZ   x    y
		fxch	st(1)					; VZX  VZY  UZ   x    y
		fadd	[VDivZOrigin]			; VZXS VZY  UZ   x    y
		fld		[ZiStepX]				; ZdX  VZXS VZY  UZ   x    y
		fmulp	st(4),st				; VZXS VZY  UZ   ZX   y
		faddp	st(1),st				; VZ   UZ   ZX   y
		fld		[ZiStepY]				; ZdY  VZ   UZ   ZX   y
		fmulp	st(4),st				; VZ   UZ   ZX   ZY
		fxch	st(2)					; ZX   UZ   VZ   ZY
		fadd	[ZiOrigin]				; ZXS  UZ   VZ   ZY

		//room for two cycles of int instructions here

		faddp	st(3),st				; UZ   VZ   Zi
		fld1							; 1    UZ   VZ   Zi
		fdiv	st,st(3)				; ZL   UZ   VZ   Zi

		//room for 18 cycles of int instructions here

		fld		st						; ZL   ZL   UZ   VZ   Zi
		fmul	st,st(3)				; VL   ZL   UZ   VZ   Zi
		fxch	st(4)					; Zi   ZL   UZ   VZ   VL
		fadd	[Zi16StepX]				; ZRi  ZL   UZ   VZ   VL
		fxch	st(1)					; ZL   ZRi  UZ   VZ   VL
		fmul	st,st(2)				; UL   ZRi  UZ   VZ   VL
		fxch	st(3)					; VZ   ZRi  UZ   UL   VL
		fadd	[VDivZ16StepX]			; VZR  ZRi  UZ   UL   VL
		fxch	st(2)					; UZ   ZRi  VZR  UL   VL
		fadd	[UDivZ16StepX]			; UZR  ZRi  VZR  UL   VL
		fld1							; 1    UZR  ZRi  VZR  UL   VL
		fdiv	st,st(2)				; ZR   UZR  ZRi  VZR  UL   VL

		//room for 18 cycles of int stuff here

		fld		st						; ZR   ZR   UZR  ZRi  VZR  UL   VL
		fmul	st,st(4)				; VR   ZR   UZR  ZRi  VZR  UL   VL
		fxch	st(1)					; ZR   VR   UZR  ZRi  VZR  UL   VL
		//fmul stall one cycle
		fmul	st,st(2)				; UR   VR   UZR  ZRi  VZR  UL   VL

		test	ecx,ecx
		jz		HandleLeftoverPixels16

SpanLoop16:
		//need one more stack spot
		fstp	dword ptr[FloatTemp]	; VR   UZR  ZRi  VZR  UL   VL
		fld		st(4)					; UL   VR   UZR  ZRi  VZR  UL   VL
		fmul	[GLMapMulU]				; ULL  VR   UZR  ZRi  VZR  UL   VL
		fld		st(5)					; UL   ULL  VR   UZR  ZRi  VZR  UL   VL
		fadd	qword ptr[MipMagic]		; ULk  ULL  VR   UZR  ZRi  VZR  UL   VL
		fxch	st(1)					; ULL  ULk  VR   UZR  ZRi  VZR  UL   VL
		fadd	qword ptr[MipMagic]		; ULLk ULk  VR   UZR  ZRi  VZR  UL   VL
		fxch	st(1)					; ULk  ULLk VR   UZR  ZRi  VZR  UL   VL
		fstp	qword ptr[Bucket]		; ULLk VR   UZR  ZRi  VZR  UL   VL
		fstp	qword ptr[Bucket2]		; VR   UZR  ZRi  VZR  UL   VL

		mov		ebx,dword ptr[Bucket]
		mov		eax,dword ptr[Bucket2]

		fld		st(5)					; VL   VR   UZR  ZRi  VZR  UL   VL
		fmul	[GLMapMulV]				; VLL  VR   UZR  ZRi  VZR  UL   VL

		add		ebx,dword ptr[UAdjust]
		add		eax,dword ptr[UAdjustL]

		mov		[U1],ebx
		mov		[UFixed],eax

		fld		st(6)					; VL   VLL  VR   UZR  ZRi  VZR  UL   VL
		fadd	qword ptr[MipMagic]		; VLk  VLL  VR   UZR  ZRi  VZR  UL   VL
		fxch	st(1)					; VLL  VLk  VR   UZR  ZRi  VZR  UL   VL
		fadd	qword ptr[MipMagic]		; VLLk VLk  VR   UZR  ZRi  VZR  UL   VL
		fxch	st(1)					; VLk  VLLk VR   UZR  ZRi  VZR  UL   VL
		fstp	qword ptr[Bucket]		; VLLk VR   UZR  ZRi  VZR  UL   VL
		fstp	qword ptr[Bucket2]		; VR   UZR  ZRi  VZR  UL   VL
		fsubr	st(5),st				; VR   UZR  ZRi  VZR  UL   dV

		mov		ebx,dword ptr[Bucket]
		mov		eax,dword ptr[Bucket2]

		fld		dword ptr[FloatTemp]	; UR   VR   UZR  ZRi  VZR  UL   dV

		add		ebx,dword ptr[VAdjust]
		add		eax,dword ptr[VAdjustL]

		mov		[V1],ebx
		mov		[VFixed],eax

		fsubr	st(5),st				; UR   VR   UZR  ZRi  VZR  dU   dV
		fxch	st(6)					; dV   VR   UZR  ZRi  VZR  dU   UR
		fadd	qword ptr[MipMagic2]	; dVk  VR   UZR  ZRi  VZR  dU   UR
		fxch	st(5)					; dU   VR   UZR  ZRi  VZR  dVk  UR
		fadd	qword ptr[MipMagic2]	; dUk  VR   UZR  ZRi  VZR  dVk  UR
		fxch	st(5)					; dVk  VR   UZR  ZRi  VZR  dUk  UR
		fstp	qword ptr[DeltaV]		; VR   UZR  ZRi  VZR  dUk  UR
		fxch	st(5)					; UR   UZR  ZRi  VZR  dUk  VR
		//gotta do this to get em lined back up right
		fxch	st(4)					; dUk  UZR  ZRi  VZR  UR   VR
		fstp	qword ptr[DeltaU]		; UZR  ZRi  VZR  UR   VR

		//right becomes left			; UZL  ZLi  VZL  UL   VL
		fadd	[UDivZ16StepX]			; UZR  ZLi  VZL  UL   VL
		fxch	st(1)					; ZLi  UZR  VZL  UL   VL
		fadd	[Zi16StepX]				; ZRi  UZR  VZL  UL   VL
		fxch	st(2)					; VZL  UZR  ZRi  UL   VL
		fadd	[VDivZ16StepX]			; VZR  UZR  ZRi  UL   VL
		fxch	st(2)					; ZRi  UZR  VZR  UL   VL
		fxch	st(1)					; UZR  ZRi  VZR  UL   VL
		//need those fxch to line things up for loops (bad)

		// Clamp U/V
		mov		ebx,[UFixed]
		cmp		ebx,MaxU
		jle		TryClampU016
		mov		ecx,MaxU
		mov		dword ptr[UFixed],ecx
		jmp		NoClampU016

TryClampU016:
		cmp		ebx,0
		jge		NoClampU016
		mov		dword ptr[UFixed],0
NoClampU016:
		mov		eax,[VFixed]
		cmp		eax,MaxV
		jle		TryClampV016
		mov		ecx,MaxV
		mov		dword ptr[VFixed],ecx
		jmp		NoClampV016

TryClampV016:
		cmp		eax,0
		jge		NoClampV016
		mov		dword ptr[VFixed],0

NoClampV016:

		fld1							; 1    UZR  ZRi  VZR  UL   VL

		// Cache U1/V1
		mov		ebx,dword ptr[UFixed]

		fdiv	st,st(2)			; ZR   UZR  ZRi  VZR  UL   VL


		mov		eax,dword ptr[VFixed]

		mov		ebx,dword ptr[U1]
		mov		eax,dword ptr[V1]

		add		ebx,dword ptr[UAdjust2]
		add		eax,dword ptr[VAdjust2]

		mov		ecx,16
		mov		dword ptr[Bucket],ebx

		sub		ecx,[VShift]
		mov		dword ptr[Bucket2],eax

		shr		eax,cl

		push	ebp
		shr		ebx,16

		and		eax,[GHMaskShifted]
		mov		esi,[pTex]

		and		ebx,[GWMask]
		add		esi,eax

		mov		ecx,[VShift]
		add		esi,ebx

		mov		edx,dword ptr[Bucket2]
		mov		ebp,dword ptr[DeltaV]

		mov		ebx,dword ptr[Bucket]

		//do 8 pixels

		add		edx,ebp
		mov		ax,[2*esi]

		mov		esi,edx
		add		ebx,dword ptr[DeltaU]

		shl		esi,cl
		and		ebx,[GWMaskShifted]

		and		esi,[GHMaskShifted16]

		add		esi,ebx
		mov		[edi+0],ax

		shr		esi,16
		add		edx,ebp

		add		esi,pTex
		add		ebx,dword ptr[DeltaU]

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,[GWMaskShifted]
		mov		[edi+2],ax

		shl		esi,cl
		add		edx,ebp

		and		esi,[GHMaskShifted16]

		add		esi,ebx
		shr		esi,16

		add		ebx,dword ptr[DeltaU]
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,[GWMaskShifted]
		mov		[edi+4],ax

		shl		esi,cl
		add		edx,ebp

		and		esi,[GHMaskShifted16]

		add		esi,ebx
		shr		esi,16

		add		ebx,dword ptr[DeltaU]
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,[GWMaskShifted]
		mov		[edi+6],ax

		shl		esi,cl
		add		edx,ebp

		and		esi,[GHMaskShifted16]

		add		esi,ebx
		shr		esi,16

		add		ebx,dword ptr[DeltaU]
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,[GWMaskShifted]
		mov		[edi+8],ax

		shl		esi,cl
		add		edx,ebp

		and		esi,[GHMaskShifted16]

		add		esi,ebx
		shr		esi,16

		add		ebx,dword ptr[DeltaU]
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,[GWMaskShifted]
		mov		[edi+10],ax

		shl		esi,cl
		add		edx,ebp

		and		esi,[GHMaskShifted16]

		add		esi,ebx
		shr		esi,16

		add		ebx,dword ptr[DeltaU]
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,[GWMaskShifted]
		mov		[edi+12],ax

		shl		esi,cl
		add		edx,ebp

		and		esi,[GHMaskShifted16]

		add		esi,ebx
		shr		esi,16

		add		ebx,dword ptr[DeltaU]
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		pop     ebp
		mov		[edi+14],ax

		;get corrected right side deltas; st0  st1  st2  st3  st4  st5  st6  st7
										; ZR   UZR  ZRi  VZR  UL   VL
		fld		st						; ZR   ZR   UZR  ZRi  VZR  UL   VL
		fmul	st,st(4)				; VR   ZR   UZR  ZRi  VZR  UL   VL
		fxch	st(1)					; ZR   VR   UZR  ZRi  VZR  UL   VL
		fmul	st,st(2)				; UR   VR   UZR  ZRi  VZR  UL   VL

		add		edi,16
		dec		[NumASpans]			; dec num affine spans
		jnz		SpanLoop16				; loop back

HandleLeftoverPixels16:

		mov		esi,[pTex]


		cmp		[RemainingCount],0
		jz		FPUReturn16

		//need one more stack spot
		fstp	dword ptr[FloatTemp]	; VR   UZR  ZRi  VZR  UL   VL
		fld		st(4)					; UL   VR   UZR  ZRi  VZR  UL   VL
		fmul	[GLMapMulU]				; ULL  VR   UZR  ZRi  VZR  UL   VL
		fld		st(5)					; UL   ULL  VR   UZR  ZRi  VZR  UL   VL
		fadd	qword ptr[MipMagic]		; ULk  ULL  VR   UZR  ZRi  VZR  UL   VL
		fxch	st(1)					; ULL  ULk  VR   UZR  ZRi  VZR  UL   VL
		fadd	qword ptr[MipMagic]		; ULLk ULk  VR   UZR  ZRi  VZR  UL   VL
		fxch	st(1)					; ULk  ULLk VR   UZR  ZRi  VZR  UL   VL
		fstp	qword ptr[Bucket]		; ULLk VR   UZR  ZRi  VZR  UL   VL
		fstp	qword ptr[Bucket2]		; VR   UZR  ZRi  VZR  UL   VL

		mov		ebx,dword ptr[Bucket]
		mov		eax,dword ptr[Bucket2]

		fld		st(5)					; VL   VR   UZR  ZRi  VZR  UL   VL
		fmul	[GLMapMulV]				; VLL  VR   UZR  ZRi  VZR  UL   VL

		add		ebx,dword ptr[UAdjust]
		add		eax,dword ptr[UAdjustL]

		mov		[U1],ebx
		mov		[UFixed],eax

		fld		st(6)					; VL   VLL  VR   UZR  ZRi  VZR  UL   VL
		fadd	qword ptr[MipMagic]		; VLk  VLL  VR   UZR  ZRi  VZR  UL   VL
		fxch	st(1)					; VLL  VLk  VR   UZR  ZRi  VZR  UL   VL
		fadd	qword ptr[MipMagic]		; VLLk VLk  VR   UZR  ZRi  VZR  UL   VL
		fxch	st(1)					; VLk  VLLk VR   UZR  ZRi  VZR  UL   VL
		fstp	qword ptr[Bucket]		; VLLk VR   UZR  ZRi  VZR  UL   VL
		fstp	qword ptr[Bucket2]		; VR   UZR  ZRi  VZR  UL   VL

		mov		ebx,dword ptr[Bucket]
		mov		eax,dword ptr[Bucket2]

		fld		dword ptr[FloatTemp]	; UR   VR   UZR  ZRi  VZR  UL   dV

		add		ebx,dword ptr[VAdjust]
		add		eax,dword ptr[VAdjustL]

		mov		[V1],ebx
		mov		[VFixed],eax

		dec		[RemainingCount]
		jz		OnePixelSpan16


		//must get rid of this wasted time
		fstp	[FloatTemp]				; inv. inv. inv. inv. UL   VL
		fstp	[FloatTemp]				; inv. inv. inv. UL   VL
		fstp	[FloatTemp]				; inv. inv. UL   VL
		fstp	[FloatTemp]				; inv. UL   VL
		fstp	[FloatTemp]				; UL   VL
		fild	[y]						; y    UL   VL
		fild	[x2]					; xr   y    UL   VL

		fld		[UDivZStepY]			; UZdY xr   y    UL   VL
		fld		[UDivZStepX]			; UZdX UZdY xr   y    UL   VL
		fmul	st,st(2)				; UZX  UZdY xr   y    UL   VL
		fld		[VDivZStepY]			; VZdY UZX  UZdY xr   y    UL   VL
		fld		[VDivZStepX]			; VZdX VZdY UZX  UZdY xr   y    UL   VL
		fxch	st(3)					; UZdy VZdY UZX  VZdX xr   y    UL   VL
		fmul	st,st(5)				; UZY  VZdY UZX  VZdX xr   y    UL   VL
		fxch	st(2)					; UZX  VZdY UZY  VZdX xr   y    UL   VL
		fadd	[UDivZOrigin]			; UZXS VZdY UZY  VZdX xr   y    UL   VL
		fxch	st(3)					; VZdX VZdY UZY  UZXS xr   y    UL   VL
		fmul	st,st(4)				; VZX  VZdY UZY  UZXS xr   y    UL   VL
		fxch	st(2)					; UZY  VZdY VZX  UZXS xr   y    UL   VL
		faddp	st(3),st				; VZdY VZX  UZ   xr   y    UL   VL
		fmul	st,st(4)				; VZY  VZX  UZ   xr   y    UL   VL
		fxch	st(1)					; VZX  VZY  UZ   xr   y    UL   VL
		fadd	[VDivZOrigin]			; VZXS VZY  UZ   xr   y    UL   VL
		fld		[ZiStepX]				; ZdX  VZXS VZY  UZ   xr   y    UL   VL
		fmulp	st(4),st				; VZXS VZY  UZ   ZX   y    UL   VL
		faddp	st(1),st				; VZ   UZ   ZX   y    UL   VL
		fld		[ZiStepY]				; ZdY  VZ   UZ   ZX   y    UL   VL
		fmulp	st(4),st				; VZ   UZ   ZX   ZY   UL   VL
		fxch	st(2)					; ZX   UZ   VZ   ZY   UL   VL
		fadd	[ZiOrigin]				; ZXS  UZ   VZ   ZY   UL   VL

		faddp	st(3),st				; UZ   VZ   Zi   UL   VL
		fld1							; 1    UZ   VZ   Zi   UL   VL
		fdiv	st,st(3)				; ZR   UZ   VZ   Zi   UL   VL

		fld		st						; ZR   ZR   UZ   VZ   Zi   UL   VL
		fmul	st,st(3)				; VR   ZR   UZ   VZ   Zi   UL   VL
		fxch	st(1)					; ZR   VR   UZ   VZ   Zi   UL   VL
		fmul	st,st(2)				; UR   VR   UZ   VZ   Zi   UL   VL

		//lazy idiv below... should 1/int mul mul

		; calculate deltas				; st0  st1  st2  st3  st4  st5  st6  st7
		fsubr	st(5),st				; UR   VR   inv. inv. inv. dU   VL
		fxch	st(1)					; VR   UR   inv. inv. inv. dU   VL
		fsubr	st(6),st				; VR   UR   inv. inv. inv. dU   dV
		fxch	st(6)					; dV   UR   inv. inv. inv. dU   VR
		fidiv   dword ptr[RemainingCount];dv   UR   inv. inv. inv. dU   VR
		fadd	qword ptr[MipMagic]		; dvk  UR   inv. inv. inv. dU   VR
		fxch	st(5)					; dU   UR   inv. inv. inv. dvk  VR
		fidiv	dword ptr[RemainingCount];du   UR   inv. inv. inv. dvk  VR
		fadd	qword ptr[MipMagic]		; duk  UR   inv. inv. inv. dvk  VR
		fxch	st(5)					; dvk  UR   inv. inv. inv. duk  VR
		fstp	qword ptr[DeltaV]		; UR   inv. inv. inv. duk  VR
		fxch	st(4)					; duk  inv. inv. inv. UR   VR
		fstp	qword ptr[DeltaU]		; inv. inv. inv. UR   VR
		fld		st(1)					; inv. inv. inv. inv. UR   VR
		fld		st(2)					; inv. inv. inv. inv. inv. UR   VR

		// Clamp U/V
		mov		ebx,[UFixed]
		cmp		ebx,MaxU
		jle		TryClampU116
		mov		ecx,MaxU
		mov		dword ptr[UFixed],ecx
		jmp		NoClampU116

TryClampU116:
		cmp		ebx,0
		jge		NoClampU116
		mov		dword ptr[UFixed],0
NoClampU116:
		mov		eax,[VFixed]
		cmp		eax,MaxV
		jle		TryClampV116
		mov		ecx,MaxV
		mov		dword ptr[VFixed],ecx
		jmp		NoClampV116

TryClampV116:
		cmp		eax,0
		jge		NoClampV116
		mov		dword ptr[VFixed],0

NoClampV116:
		// Cache U1/V1
		mov		ebx,dword ptr[UFixed]
		mov		eax,dword ptr[VFixed]

		mov		ebx,dword ptr[U1]
		mov		edx,dword ptr[V1]

		mov		ecx,16
		add		ebx,dword ptr[UAdjust2]

		sub		ecx,[VShift]
		add		edx,dword ptr[VAdjust2]

//		mov		dword ptr[Bucket],ebx
//		mov		dword ptr[Bucket2],eax

OnePixelSpan16:
		push	ebp

LeftoverLoop16:
		mov		eax,edx
		shr		eax,cl
		mov		ebp,ebx
		and		eax,[GHMaskShifted]
		shr		ebp,16
		and		ebp,[GWMask]
		add		eax,ebp
		add		eax,esi
		mov		ax,[2*eax]
		mov		[edi],ax
		add		ebx,dword ptr[DeltaU]
		add		edi,2
		add		edx,dword ptr[DeltaV]

		dec		[RemainingCount]
		jge		LeftoverLoop16

		pop		ebp

FPUReturn16:
		ffree	st(0)
		ffree	st(1)
		ffree	st(2)
		ffree	st(3)
		ffree	st(4)
		ffree	st(5)
		ffree	st(6)

Return16:
	}
//	LightCachedSpan16_AsmLerpLUT(x1, x2, y);
//	LightCachedSpan16_AsmLerpFPU(x1, x2, y);
}


