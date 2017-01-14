/****************************************************************************************/
/*  x86span555.c                                                                        */
/*                                                                                      */
/*  Author:       Ken Baird                                                             */
/*  Description:  555 assembly calls for tons of renderstates                           */
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
//see notes in x86span565

/*
Code fragments from Chris Hecker's texture mapping articles used with
permission.  http://www.d6.com/users/checker 
*/


#include	"windows.h"		//I really didn't want to do this
#include	"softdrv.h"
#include	"basetype.h"
#include	"drawspan.h"
#include	"render.h"


typedef struct EdgeAsmFPUTag
{
	int		X, y, Height;
	float	x, u, v, z, r, g, b;
	float	xstep, ustep, vstep, zstep;
	float	rstep, gstep, bstep;
	uint32	R, G, B;
} EdgeAsmFPU;


void	DrawScanLineGouraudNoZ_Asm555X86FPU(EdgeAsmFPU *pLeft, EdgeAsmFPU *pRight)
{
	TDest	=Dest;
	Red		=Green	=0;
	_asm
	{
		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		ebx,pLeft
		mov     ecx,pRight
		mov     eax,[ebx]EdgeAsmFPU.X
		mov     edx,[ecx]EdgeAsmFPU.X
		sub     edx,eax
		jle		GouraudReturnNoZ

		mov		esi,eax
		inc		edx
		shl		eax,1
		add		TDest,eax
		test	esi,1					//dword align left side
		jz		NoSinglePixie

		//odd sized spans write one pixel to dword align
		fld     dword ptr [ebx]EdgeAsmFPU.u		; UL
		fmul    [Real65536]						; UL16
		fld     dword ptr [ebx]EdgeAsmFPU.v		; VL   UL16
		fmul	[Real65536]						; VL16 UL16
		fld		dword ptr [ebx]EdgeAsmFPU.r		; RL   VL16 UL16
		fld		dword ptr [ebx]EdgeAsmFPU.g		; GL   RL   VL16 UL16
		fld		dword ptr [ebx]EdgeAsmFPU.b		; BL   GL   RL   VL16 UL16
		fxch	st(4)							; UL16 GL   RL   VL16 BL
		fistp	[u16]							; GL   RL   VL16 BL
		fxch	st(2)							; VL16 RL   GL   BL
		fistp	[v16]							; RL   GL   BL

		mov     [widTemp],edx

		mov		ecx,[VShift]
		mov		ebx,[GHMaskShifted]

		mov		esi,[GWMask]
		mov		edi,[v16]

		mov		edx,dword ptr[u16]

		shr		edi,cl

		shr		edx,16
		xor		eax,eax

		and		edi,ebx
		and		edx,esi

		add		edi,edx

		add		edi,GBitPtrHalf
		mov		ax,word ptr[edi*2]

		mov		ebx,eax
		and		eax,REDMASK2

		mov		ecx,ebx
		and		ebx,GREENMASK2

		mov		dword ptr[Red],eax
		and		ecx,BLUEMASK2

		mov		dword ptr[Green],ebx
		mov		dword ptr[Blue],ecx

		fimul	dword ptr[Red]					; R    GL   BL
		fxch	st(1)							; GL   R    BL
		fimul	dword ptr[Green]				; G    R    BL
		fxch	st(1)							; R    G    BL
		fadd	qword ptr[Magic]				; Rk   G    BL
		fxch	st(2)							; BL   G    Rk
		fimul	[Blue]							; B    G    Rk
		fxch	st(1)							; G    B    Rk
		fadd	qword ptr[Magic]				; Gk   B    Rk
		fxch	st(2)							; Rk   B    Gk
		fstp	qword ptr[Bucket]				; B    Gk
		fadd	qword ptr[Magic]				; Bk   Gk
		fxch	st(1)							; Gk   Bk
		fstp	qword ptr[Bucket2]				; Bk

		mov		edx,dword ptr[Bucket]
		mov		eax,dword ptr[Bucket2]

		and		edx,REDMASK2
		and		eax,GREENMASK2

		fstp	qword ptr[Bucket]

		mov		ecx,TDest
		mov		ebx,dword ptr[Bucket]

		and		ebx,BLUEMASK2
		or		edx,eax

		add		TDest,2
		or		edx,ebx

		mov		ebx,pLeft
		mov		word ptr[ecx],dx
		mov     edx,[widTemp]
		mov     ecx,pRight
		dec		edx
		jz		GouraudReturnNoZ
		mov		esi,edx
		and		esi,1
		sub		edx,esi
		jz		GouraudReturnNoZ

NoSinglePixie:
		mov     [widTemp],edx                 ; just for a temp
		shr		edx,1

		; try to keep fmul fxch pairs seperated to avoid stalling
		; calc this scanlines steps           ; FPU Stack
		                                      ; st0  st1  st2  st3  st4  st5  st6  st7
		fild    dword ptr [widTemp]           ; WID

		mov     [widTemp],edx                 ; Color interps doubled

		fld     dword ptr [ecx]EdgeAsmFPU.u      ; UR   WID
		fld     dword ptr [ebx]EdgeAsmFPU.u      ; UL   UR   WID
		fsub    st(1), st                     ; UL   UD   WID
		fld     dword ptr [ecx]EdgeAsmFPU.v      ; VR   UL   UD   WID
		fxch    st(1)                         ; UL   VR   UD   WID
		fmul    [Real65536]                   ; UL16 VR   UD   WID
		fld     dword ptr [ebx]EdgeAsmFPU.v      ; VL   UL16 VR   UD   WID
		fsub    st(2), st                     ; VL   UL16 VD   UD   WID
		fild    dword ptr [ecx]EdgeAsmFPU.R      ; RR   VL   UL16 VD   UD   WID
		fxch    st(3)                         ; VD   VL   UL16 RR   UD   WID
		fmul    [Real65536]                   ; VD16 VL   UL16 RR   UD   WID
		fild    dword ptr [ebx]EdgeAsmFPU.R      ; RL   VD16 VL   UL16 RR   UD   WID
		fsub    st(4),st                      ; RL   VD16 VL   UL16 RD   UD   WID
		fxch    st(5)                         ; UD   VD16 VL   UL16 RD   RL   WID
		fmul    [Real65536]                   ; UD16 VD16 VL   UL16 RD   RL   WID
		fxch    st(2)						  ; VL   VD16 UD16 UL16 RD   RL   WID
		fmul    [Real65536]                   ; VL16 VD16 UD16 UL16 RD   RL   WID
		fxch	st(3)							; UL16 VD16 UD16 VL16 RD   RL   WID
		fistp	[u16]							; VD16 UD16 VL16 RD   RL   WID
		fxch	st(2)							; VL16 UD16 VD16 RD   RL   WID
		fistp	[v16]							; UD16 VD16 RD   RL   WID
		fld1									; 1    UD16 VD16 RD   RL   WID
		fdivrp	st(5),st						; UD16 VD16 RD   RL   DWID

		//let that cook

		fmul	st,st(4)						; USTP VD16 RD   RL   DWID
		fild	[ecx]EdgeAsmFPU.G					; RG   USTP VD16 RD   RL   DWID
		fild	[ebx]EdgeAsmFPU.G					; LG   RG   USTP VD16 RD   RL   DWID
		fsub	st(1),st						; LG   GD   USTP VD16 RD   RL   DWID
		fxch	st(2)							; USTP GD   LG   VD16 RD   RL   DWID
		fistp	[UStep]							; GD   LG   VD16 RD   RL   DWID
		fild	[ecx]EdgeAsmFPU.B					; RB   GD   LG   VD16 RD   RL   DWID
		fild	[ebx]EdgeAsmFPU.B					; LB   RB   GD   LG   VD16 RD   RL   DWID
		fsub	st(1),st						; LB   BD   GD   LG   VD16 RD   RL   DWID
		fxch	st(4)							; VD16 BD   GD   LG   LB   RD   RL   DWID
		fmul	st,st(7)						; VSTP BD   GD   LG   LB   RD   RL   DWID
		fxch	st(7)							; DWID BD   GD   LG   LB   RD   RL   VSTP
		fmul	dword ptr[Two]					; DW2  BD   GD   LG   LB   RD   RL   VSTP
		fxch	st(7)							; VSTP BD   GD   LG   LB   RD   RL   DW2
		fxch	st(5)							; RD   BD   GD   LG   LB   VSTP RL   DW2
		fmul	st,st(7)						; RI   BD   GD   LG   LB   VSTP RL   DW2
		frndint
		fxch	st(2)							; GD   BD   RI   LG   LB   VSTP RL   DW2
		fmul	st,st(7)						; GI   BD   RI   LG   LB   VSTP RL   DW2
		frndint
		fxch	st(7)							; DW2  BD   RI   LG   LB   VSTP RL   GI
		fmulp	st(1),st						; BI   RI   LG   LB   VSTP RL   GI
		frndint
		fxch	st(4)							; VSTP RI   LG   LB   BI   RL   GI
		fistp	[VStep]							; RI   LG   LB   BI   RL   GI

		push	ebp

PixieLoop:
		mov		ecx,[VShift]
		mov		ebx,[GHMaskShifted]

		mov		ebp,[GWMask]
		mov		edi,[v16]

		mov		edx,dword ptr[u16]
		mov		esi,edi

		mov		eax,edx

		shr		edi,cl
		add		esi,[VStep]

		shr		edx,16
		add		eax,[UStep]

		and		edi,ebx
		and		edx,ebp

		add		edi,edx
		mov		edx,eax

		add		edi,GBitPtrHalf
		add		eax,[UStep]

		mov		[u16],eax
		mov		ax,word ptr[edi*2]

		mov		edi,esi
		add		esi,[VStep]

		shr		edx,16
		mov		[v16],esi

		shr		edi,cl
		and		edx,ebp

		mov		esi,GBitPtrHalf
		and		edi,ebx

		shl		eax,16
		add		esi,edx

		add		esi,edi

		mov		ax,word ptr[esi*2]

		mov		ebx,eax
		and		eax,REDMASK2

		mov		ecx,ebx
		and		ebx,GREENMASK2

		mov		dword ptr[Red],eax
		and		ecx,BLUEMASK2

		mov		dword ptr[Green],ebx
		mov		dword ptr[Blue],ecx

		fild	qword ptr[Red]					; r    RI   LG   LB   BI   RL   GI
		fmul	st,st(5)						; R    RI   LG   LB   BI   RL   GI
		fild	qword ptr[Green]				; g    R    RI   LG   LB   BI   RL   GI
		fmul	st,st(3)						; G    R    RI   LG   LB   BI   RL   GI
		fxch	st(6)							; RL   R    RI   LG   LB   BI   G    GI
		fadd	st,st(2)						; RL2  R    RI   LG   LB   BI   G    GI
		fxch	st(3)							; LG   R    RI   RL2  LB   BI   G    GI
		fadd	st,st(7)						; LG2  R    RI   RL2  LB   BI   G    GI
		fxch	st(6)							; G    R    RI   RL2  LB   BI   LG2  GI
		fadd	qword ptr[Magic]				; Gk   R    RI   RL2  LB   BI   LG2  GI
		fxch	st(1)							; R    Gk   RI   RL2  LB   BI   LG2  GI
		fadd	qword ptr[Magic]				; Rk   Gk   RI   RL2  LB   BI   LG2  GI
		fxch	st(1)							; Gk   Rk   RI   RL2  LB   BI   LG2  GI
		fstp	qword ptr[Bucket]				; Rk   RI   RL2  LB   BI   LG2  GI
		fstp	qword ptr[Bucket2]				; RI   RL2  LB   BI   LG2  GI

		mov		edx,dword ptr[Bucket]
		mov		eax,dword ptr[Bucket2]

		fild	dword ptr[Blue]					; b    RI   RL2  LB   BI   LG2  GI
		fmul	st,st(3)						; B    RI   RL2  LB   BI   LG2  GI
		fxch	st(3)							; LB   RI   RL2  B    BI   LG2  GI
		fadd	st,st(4)						; LB2  RI   RL2  B    BI   LG2  GI
		fxch	st(3)							; B    RI   RL2  LB2  BI   LG2  GI
		fadd	qword ptr[Magic]				; Bk   RI   RL2  LB2  BI   LG2  GI

		and		edx,GREENMASK2
		and		eax,REDMASK2

		fstp	qword ptr[Bucket]				; RI   RL2  LB2  BI   LG2  GI
		fstp	qword ptr[Bucket2]				; RL2  LB2  BI   LG2  GI

		mov		edi,edx
		mov		ebx,dword ptr[Bucket]
		or		edi,eax
		mov		ebp,TDest
		and		ebx,BLUEMASK2

		fxch	st(3)							; LG2  LB2  BI   RL2  GI
		or		edi,ebx

		fld		qword ptr[Bucket2]				; RI   LG2  LB2  BI   RL2  GI

		rol		edi,16
		add		TDest,4

		mov		[ebp],edi
		dec		[widTemp]

		jnz		PixieLoop

		pop		ebp

		fstp	[u16]
		fstp	[v16]
		fstp	[u16]
		fstp	[v16]
		fstp	[u16]
		fstp	[v16]

GouraudReturnNoZ:
		pop		edi
		pop		esi
		pop		ecx
		pop		ebx
	}
}

void	DrawScanLineGouraudNoZTrans_Asm555X86FPU(EdgeAsmFPU *pLeft, EdgeAsmFPU *pRight)
{
	TDest	=Dest;
	Red		=Green	=0;
	_asm
	{
		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		ebx,pLeft
		mov     ecx,pRight
		mov     eax,[ebx]EdgeAsmFPU.X
		mov     edx,[ecx]EdgeAsmFPU.X
		sub     edx,eax
		jle		GouraudReturnNoZ

		mov		esi,eax
		inc		edx
		shl		eax,1
		add		TDest,eax
		test	esi,1		//dword align left side
		jz		NoSinglePixie

		//odd sized spans write one pixel to dword align
		fld     dword ptr [ebx]EdgeAsmFPU.u		; UL
		fmul    [Real65536]						; UL16
		fld     dword ptr [ebx]EdgeAsmFPU.v		; VL   UL16
		fmul	[Real65536]						; VL16 UL16
		fild    dword ptr [ebx]EdgeAsmFPU.R		; RL   VL16 UL16
		fild    dword ptr [ebx]EdgeAsmFPU.G		; GL   RL   VL16 UL16
		fild    dword ptr [ebx]EdgeAsmFPU.B		; BL   GL   RL   VL16 UL16
		fxch	st(4)							; UL16 GL   RL   VL16 BL
		fistp	[u16]							; GL   RL   VL16 BL
		fxch	st(2)							; VL16 RL   GL   BL
		fistp	[v16]							; RL   GL   BL

		mov     [widTemp],edx

		mov		ecx,[VShift]
		mov		ebx,[GHMaskShifted]

		mov		esi,[GWMask]
		mov		edi,[v16]

		mov		edx,dword ptr[u16]

		shr		edi,cl

		shr		edx,16
		xor		eax,eax

		and		edi,ebx
		and		edx,esi

		add		edi,edx

		add		edi,GBitPtrHalf
		mov		ax,word ptr[edi*2]

		mov		word ptr[TempPix],ax

		mov		ebx,eax
		and		eax,REDMASK2

		mov		ecx,ebx
		and		ebx,GREENMASK2

		mov		dword ptr[Red],eax
		and		ecx,BLUEMASK2

		mov		dword ptr[Green],ebx
		mov		dword ptr[Blue],ecx

		fimul	dword ptr[Red]					; R    GL   BL
		fxch	st(1)							; GL   R    BL
		fimul	dword ptr[Green]				; G    R    BL
		fxch	st(1)							; R    G    BL
		fadd	qword ptr[Magic]				; Rk   G    BL
		fxch	st(2)							; BL   G    Rk
		fimul	[Blue]							; B    G    Rk
		fxch	st(1)							; G    B    Rk
		fadd	qword ptr[Magic]				; Gk   B    Rk
		fxch	st(2)							; Rk   B    Gk
		fstp	qword ptr[Bucket]				; B    Gk
		fadd	qword ptr[Magic]				; Bk   Gk
		fxch	st(1)							; Gk   Bk
		fstp	qword ptr[Bucket2]				; Bk

		mov		edx,dword ptr[Bucket]
		mov		eax,dword ptr[Bucket2]

		and		edx,REDMASK2
		and		eax,GREENMASK2

		fstp	qword ptr[Bucket]

		mov		ecx,TDest
		mov		ebx,dword ptr[Bucket]

		and		ebx,BLUEMASK2
		or		edx,eax

		add		TDest,2
		or		edx,ebx

		mov		ebx,pLeft
		cmp		word ptr[TempPix],01h
		je		SkipSinglePixie

		mov		word ptr[ecx],dx

SkipSinglePixie:
		mov     edx,[widTemp]
		mov     ecx,pRight
		dec		edx
		jz		GouraudReturnNoZ
		mov		esi,edx
		and		esi,1
		sub		edx,esi
		jz		GouraudReturnNoZ

NoSinglePixie:
		mov     [widTemp],edx                 ; just for a temp
		shr		edx,1

		; try to keep fmul fxch pairs seperated to avoid stalling
		; calc this scanlines steps           ; FPU Stack
		                                      ; st0  st1  st2  st3  st4  st5  st6  st7
		fild    dword ptr [widTemp]           ; WID

		mov     [widTemp],edx                 ; Color interps doubled

		fld     dword ptr [ecx]EdgeAsmFPU.u      ; UR   WID
		fld     dword ptr [ebx]EdgeAsmFPU.u      ; UL   UR   WID
		fsub    st(1), st                     ; UL   UD   WID
		fld     dword ptr [ecx]EdgeAsmFPU.v      ; VR   UL   UD   WID
		fxch    st(1)                         ; UL   VR   UD   WID
		fmul    [Real65536]                   ; UL16 VR   UD   WID
		fld     dword ptr [ebx]EdgeAsmFPU.v      ; VL   UL16 VR   UD   WID
		fsub    st(2), st                     ; VL   UL16 VD   UD   WID
		fild    dword ptr [ecx]EdgeAsmFPU.R      ; RR   VL   UL16 VD   UD   WID
		fxch    st(3)                         ; VD   VL   UL16 RR   UD   WID
		fmul    [Real65536]                   ; VD16 VL   UL16 RR   UD   WID
		fild    dword ptr [ebx]EdgeAsmFPU.R      ; RL   VD16 VL   UL16 RR   UD   WID
		fsub    st(4),st                      ; RL   VD16 VL   UL16 RD   UD   WID
		fxch    st(5)                         ; UD   VD16 VL   UL16 RD   RL   WID
		fmul    [Real65536]                   ; UD16 VD16 VL   UL16 RD   RL   WID
		fxch    st(2)						  ; VL   VD16 UD16 UL16 RD   RL   WID
		fmul    [Real65536]                   ; VL16 VD16 UD16 UL16 RD   RL   WID
		fxch	st(3)							; UL16 VD16 UD16 VL16 RD   RL   WID
		fistp	[u16]							; VD16 UD16 VL16 RD   RL   WID
		fxch	st(2)							; VL16 UD16 VD16 RD   RL   WID
		fistp	[v16]							; UD16 VD16 RD   RL   WID
		fld1									; 1    UD16 VD16 RD   RL   WID
		fdivrp	st(5),st						; UD16 VD16 RD   RL   DWID

		//let that cook

		fmul	st,st(4)						; USTP VD16 RD   RL   DWID
		fild	[ecx]EdgeAsmFPU.G					; RG   USTP VD16 RD   RL   DWID
		fild	[ebx]EdgeAsmFPU.G					; LG   RG   USTP VD16 RD   RL   DWID
		fsub	st(1),st						; LG   GD   USTP VD16 RD   RL   DWID
		fxch	st(2)							; USTP GD   LG   VD16 RD   RL   DWID
		fistp	[UStep]							; GD   LG   VD16 RD   RL   DWID
		fild	[ecx]EdgeAsmFPU.B					; RB   GD   LG   VD16 RD   RL   DWID
		fild	[ebx]EdgeAsmFPU.B					; LB   RB   GD   LG   VD16 RD   RL   DWID
		fsub	st(1),st						; LB   BD   GD   LG   VD16 RD   RL   DWID
		fxch	st(4)							; VD16 BD   GD   LG   LB   RD   RL   DWID
		fmul	st,st(7)						; VSTP BD   GD   LG   LB   RD   RL   DWID
		fxch	st(7)							; DWID BD   GD   LG   LB   RD   RL   VSTP
		fmul	dword ptr[Two]					; DW2  BD   GD   LG   LB   RD   RL   VSTP
		fxch	st(7)							; VSTP BD   GD   LG   LB   RD   RL   DW2
		fxch	st(5)							; RD   BD   GD   LG   LB   VSTP RL   DW2
		fmul	st,st(7)						; RI   BD   GD   LG   LB   VSTP RL   DW2
		frndint
		fxch	st(2)							; GD   BD   RI   LG   LB   VSTP RL   DW2
		fmul	st,st(7)						; GI   BD   RI   LG   LB   VSTP RL   DW2
		frndint
		fxch	st(7)							; DW2  BD   RI   LG   LB   VSTP RL   GI
		fmulp	st(1),st						; BI   RI   LG   LB   VSTP RL   GI
		frndint
		fxch	st(4)							; VSTP RI   LG   LB   BI   RL   GI
		fistp	[VStep]							; RI   LG   LB   BI   RL   GI

		push	ebp

PixieLoop:
		mov		ecx,[VShift]
		mov		ebx,[GHMaskShifted]

		mov		ebp,[GWMask]
		mov		edi,[v16]

		mov		edx,dword ptr[u16]
		mov		esi,edi

		mov		eax,edx

		shr		edi,cl
		add		esi,[VStep]

		shr		edx,16
		add		eax,[UStep]

		and		edi,ebx
		and		edx,ebp

		add		edi,edx
		mov		edx,eax

		add		edi,GBitPtrHalf
		add		eax,[UStep]

		mov		[u16],eax
		mov		ax,word ptr[edi*2]

		mov		edi,esi
		add		esi,[VStep]

		shr		edx,16
		mov		[v16],esi

		shr		edi,cl
		and		edx,ebp

		mov		esi,GBitPtrHalf
		and		edi,ebx

		shl		eax,16
		add		esi,edx

		add		esi,edi

		mov		ax,word ptr[esi*2]

		mov		[TempPix],eax

		mov		ebx,eax
		and		eax,REDMASK2

		mov		ecx,ebx
		and		ebx,GREENMASK2

		mov		dword ptr[Red],eax
		and		ecx,BLUEMASK2

		mov		dword ptr[Green],ebx
		mov		dword ptr[Blue],ecx

		fild	qword ptr[Red]					; r    RI   LG   LB   BI   RL   GI
		fmul	st,st(5)						; R    RI   LG   LB   BI   RL   GI
		fild	qword ptr[Green]				; g    R    RI   LG   LB   BI   RL   GI
		fmul	st,st(3)						; G    R    RI   LG   LB   BI   RL   GI
		fxch	st(6)							; RL   R    RI   LG   LB   BI   G    GI
		fadd	st,st(2)						; RL2  R    RI   LG   LB   BI   G    GI
		fxch	st(3)							; LG   R    RI   RL2  LB   BI   G    GI
		fadd	st,st(7)						; LG2  R    RI   RL2  LB   BI   G    GI
		fxch	st(6)							; G    R    RI   RL2  LB   BI   LG2  GI
		fadd	qword ptr[Magic]				; Gk   R    RI   RL2  LB   BI   LG2  GI
		fxch	st(1)							; R    Gk   RI   RL2  LB   BI   LG2  GI
		fadd	qword ptr[Magic]				; Rk   Gk   RI   RL2  LB   BI   LG2  GI
		fxch	st(1)							; Gk   Rk   RI   RL2  LB   BI   LG2  GI
		fstp	qword ptr[Bucket]				; Rk   RI   RL2  LB   BI   LG2  GI
		fstp	qword ptr[Bucket2]				; RI   RL2  LB   BI   LG2  GI

		mov		edx,dword ptr[Bucket]
		mov		eax,dword ptr[Bucket2]

		fild	dword ptr[Blue]					; b    RI   RL2  LB   BI   LG2  GI
		fmul	st,st(3)						; B    RI   RL2  LB   BI   LG2  GI
		fxch	st(3)							; LB   RI   RL2  B    BI   LG2  GI
		fadd	st,st(4)						; LB2  RI   RL2  B    BI   LG2  GI
		fxch	st(3)							; B    RI   RL2  LB2  BI   LG2  GI
		fadd	qword ptr[Magic]				; Bk   RI   RL2  LB2  BI   LG2  GI

		and		edx,GREENMASK2
		and		eax,REDMASK2

		fstp	qword ptr[Bucket]				; RI   RL2  LB2  BI   LG2  GI
		fstp	qword ptr[Bucket2]				; RL2  LB2  BI   LG2  GI

		mov		edi,edx
		mov		ebx,dword ptr[Bucket]
		or		edi,eax
		mov		ebp,TDest
		and		ebx,BLUEMASK2

		fxch	st(3)							; LG2  LB2  BI   RL2  GI
		fld		qword ptr[Bucket2]				; RI   LG2  LB2  BI   RL2  GI

		or		edi,ebx
		add		TDest,4

		cmp		[TempPix],010001h
		je		SkipPixie

		mov		[ebp],edi

SkipPixie:
		dec		[widTemp]

		jnz		PixieLoop

		pop		ebp

		fstp	[u16]
		fstp	[v16]
		fstp	[u16]
		fstp	[v16]
		fstp	[u16]
		fstp	[v16]

GouraudReturnNoZ:
		pop		edi
		pop		esi
		pop		ecx
		pop		ebx
	}
}

void	DrawScanLineGouraudZBuffer_Asm555X86FPU(EdgeAsmFPU *pLeft, EdgeAsmFPU *pRight)
{
	TDest	=Dest;
	Red		=Green	=0;
	_asm
	{
		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		ebx,pLeft
		mov     ecx,pRight
		mov     eax,[ebx]EdgeAsmFPU.X
		mov     edx,[ecx]EdgeAsmFPU.X
		sub     edx,eax
		jle		GouraudReturnNoZ

		mov		esi,eax
		inc		edx
		shl		eax,1
		add		TDest,eax
		add		pZBufferPtr,eax
		test	esi,1		//dword align left
		jz		NoSinglePixie

		//odd sized spans write one pixel to dword align
		fld     dword ptr [ebx]EdgeAsmFPU.u		; UL
		fmul    [Real65536]						; UL16
		fld     dword ptr [ebx]EdgeAsmFPU.v		; VL   UL16
		fmul	[Real65536]						; VL16 UL16
		fild    dword ptr [ebx]EdgeAsmFPU.R		; RL   VL16 UL16
		fild    dword ptr [ebx]EdgeAsmFPU.G		; GL   RL   VL16 UL16
		fild    dword ptr [ebx]EdgeAsmFPU.B		; BL   GL   RL   VL16 UL16
		fxch	st(4)							; UL16 GL   RL   VL16 BL
		fistp	[u16]							; GL   RL   VL16 BL
		fxch	st(2)							; VL16 RL   GL   BL
		fistp	[v16]							; RL   GL   BL
		fld		dword ptr[ebx]EdgeAsmFPU.z		; z    RL   GL   BL
		fistp	[z16]							; RL   GL   BL

		mov     [widTemp],edx

		mov		ecx,[VShift]
		mov		ebx,[GHMaskShifted]

		mov		esi,[GWMask]
		mov		edi,[v16]

		mov		edx,dword ptr[u16]

		shr		edi,cl

		shr		edx,16
		xor		eax,eax

		and		edi,ebx
		and		edx,esi

		add		edi,edx

		add		edi,GBitPtrHalf
		mov		ax,word ptr[edi*2]

		mov		ebx,eax
		and		eax,REDMASK2

		mov		ecx,ebx
		and		ebx,GREENMASK2

		mov		dword ptr[Red],eax
		and		ecx,BLUEMASK2

		mov		dword ptr[Green],ebx
		mov		dword ptr[Blue],ecx

		fimul	dword ptr[Red]					; R    GL   BL
		fxch	st(1)							; GL   R    BL
		fimul	dword ptr[Green]				; G    R    BL
		fxch	st(1)							; R    G    BL
		fadd	qword ptr[Magic]				; Rk   G    BL
		fxch	st(2)							; BL   G    Rk
		fimul	[Blue]							; B    G    Rk
		fxch	st(1)							; G    B    Rk
		fadd	qword ptr[Magic]				; Gk   B    Rk
		fxch	st(2)							; Rk   B    Gk
		fstp	qword ptr[Bucket]				; B    Gk
		fadd	qword ptr[Magic]				; Bk   Gk
		fxch	st(1)							; Gk   Bk
		fstp	qword ptr[Bucket2]				; Bk

		mov		edx,dword ptr[Bucket]
		mov		eax,dword ptr[Bucket2]

		and		edx,REDMASK2
		and		eax,GREENMASK2

		fstp	qword ptr[Bucket]

		mov		ecx,TDest
		mov		ebx,dword ptr[Bucket]

		mov		esi,[z16]
		and		ebx,BLUEMASK2

		shr		esi,16
		or		edx,eax

		mov		eax,pZBufferPtr
		add		TDest,2

		cmp		word ptr[eax],si
		jg		SkipSinglePixie

		or		edx,ebx
		mov		word ptr[eax],si

		mov		word ptr[ecx],dx

SkipSinglePixie:
		mov		ebx,pLeft
		mov     edx,[widTemp]
		add		pZBufferPtr,2
		mov     ecx,pRight
		dec		edx
		jz		GouraudReturnNoZ
		mov		esi,edx
		and		esi,1
		sub		edx,esi
		jz		GouraudReturnNoZ

NoSinglePixie:
		mov     [widTemp],edx                 ; just for a temp
		shr		edx,1

		; try to keep fmul fxch pairs seperated to avoid stalling
		; calc this scanlines steps           ; FPU Stack
		                                      ; st0  st1  st2  st3  st4  st5  st6  st7
		fild    dword ptr [widTemp]           ; WID

		mov     [widTemp],edx                 ; Color interps doubled

		fld     dword ptr [ecx]EdgeAsmFPU.u ; UR   WID
		fld     dword ptr [ebx]EdgeAsmFPU.u ; UL   UR   WID
		fsub    st(1), st                   ; UL   UD   WID
		fld     dword ptr [ecx]EdgeAsmFPU.v ; VR   UL   UD   WID
		fxch    st(1)                       ; UL   VR   UD   WID
		fmul    [Real65536]                 ; UL16 VR   UD   WID
		fld     dword ptr [ebx]EdgeAsmFPU.v ; VL   UL16 VR   UD   WID
		fsub    st(2), st                   ; VL   UL16 VD   UD   WID
		fild    dword ptr [ecx]EdgeAsmFPU.R ; RR   VL   UL16 VD   UD   WID
		fxch    st(3)                       ; VD   VL   UL16 RR   UD   WID
		fmul    [Real65536]                 ; VD16 VL   UL16 RR   UD   WID
		fild    dword ptr [ebx]EdgeAsmFPU.R ; RL   VD16 VL   UL16 RR   UD   WID
		fsub    st(4),st                    ; RL   VD16 VL   UL16 RD   UD   WID
		fxch    st(5)                       ; UD   VD16 VL   UL16 RD   RL   WID
		fmul    [Real65536]                 ; UD16 VD16 VL   UL16 RD   RL   WID
		fxch    st(2)						; VL   VD16 UD16 UL16 RD   RL   WID
		fmul    [Real65536]                 ; VL16 VD16 UD16 UL16 RD   RL   WID
		fxch	st(3)						; UL16 VD16 UD16 VL16 RD   RL   WID
		fistp	[u16]						; VD16 UD16 VL16 RD   RL   WID
		fxch	st(2)						; VL16 UD16 VD16 RD   RL   WID
		fistp	[v16]						; UD16 VD16 RD   RL   WID
		fld1								; 1    UD16 VD16 RD   RL   WID
		fdivrp	st(5),st					; UD16 VD16 RD   RL   DWID

		//let that cook

		fmul	st,st(4)						; USTP VD16 RD   RL   DWID
		fild	[ecx]EdgeAsmFPU.G				; RG   USTP VD16 RD   RL   DWID
		fild	[ebx]EdgeAsmFPU.G				; LG   RG   USTP VD16 RD   RL   DWID
		fsub	st(1),st						; LG   GD   USTP VD16 RD   RL   DWID
		fxch	st(2)							; USTP GD   LG   VD16 RD   RL   DWID
		fistp	[UStep]							; GD   LG   VD16 RD   RL   DWID
		fild	[ecx]EdgeAsmFPU.B				; RB   GD   LG   VD16 RD   RL   DWID
		fild	[ebx]EdgeAsmFPU.B				; LB   RB   GD   LG   VD16 RD   RL   DWID
		fsub	st(1),st						; LB   BD   GD   LG   VD16 RD   RL   DWID
		fxch	st(4)							; VD16 BD   GD   LG   LB   RD   RL   DWID
		fmul	st,st(7)						; VSTP BD   GD   LG   LB   RD   RL   DWID
		fxch	st(7)							; DWID BD   GD   LG   LB   RD   RL   VSTP
		fmul	dword ptr[Two]					; DW2  BD   GD   LG   LB   RD   RL   VSTP
		fxch	st(7)							; VSTP BD   GD   LG   LB   RD   RL   DW2
		fxch	st(5)							; RD   BD   GD   LG   LB   VSTP RL   DW2
		fmul	st,st(7)						; RI   BD   GD   LG   LB   VSTP RL   DW2
		frndint
		fxch	st(2)							; GD   BD   RI   LG   LB   VSTP RL   DW2
		fmul	st,st(7)						; GI   BD   RI   LG   LB   VSTP RL   DW2
		frndint
		fxch	st(1)							; BD   GI   RI   LG   LB   VSTP RL   DW2
		fmul	st,st(7)						; BD   GI   RI   LG   LB   VSTP RL   DW2
		frndint
		fxch	st(5)							; VSTP GI   RI   LG   LB   BD   RL   DW2
		fistp	[VStep]							; GI   RI   LG   LB   BD   RL   DW2
		fld		[ecx]EdgeAsmFPU.z				; rz   GI   RI   LG   LB   BD   RL   DW2
		fsub	[ebx]EdgeAsmFPU.z				; zd   GI   RI   LG   LB   BD   RL   DW2
		fxch	st(7)							; DW2  GI   RI   LG   LB   BD   RL   zd
		fmulp	st(7),st						; GI   RI   LG   LB   BD   RL   zd
		fld		[ebx]EdgeAsmFPU.z				; lz   GI   RI   LG   LB   BD   RL   zd
		fxch	st(7)							; zd   GI   RI   LG   LB   BD   RL   lz
		fistp	[ZStep]							; GI   RI   LG   LB   BD   RL   lz
		fxch	st(6)							; lz   RI   LG   LB   BD   RL   GI
		fistp	[z16]							; RI   LG   LB   BD   RL   GI

		push	ebp

PixieLoop:
		mov		ebx,[ZStep]

		add		[z16],ebx
		mov		ecx,[VShift]

		mov		edi,[z16]

		shr		edi,16
		mov		ebx,[GHMaskShifted]

		mov		ebp,[GWMask]
		mov		edx,dword ptr[u16]

		mov		word ptr[Z32],di
		mov		eax,edx

		shr		edx,16
		mov		edi,[v16]

		add		eax,[UStep]
		mov		esi,edi

		shr		edi,cl
		add		esi,[VStep]

		and		edi,ebx
		and		edx,ebp

		add		edi,edx
		mov		edx,eax

		add		edi,GBitPtrHalf
		add		eax,[UStep]

		mov		[u16],eax
		mov		ax,word ptr[edi*2]

		mov		edi,esi
		add		esi,[VStep]

		shr		edx,16
		mov		[v16],esi

		shr		edi,cl
		and		edx,ebp

		mov		esi,GBitPtrHalf
		and		edi,ebx

		shl		eax,16
		add		esi,edx

		add		esi,edi

		mov		ax,word ptr[esi*2]

		mov		ebx,eax
		and		eax,REDMASK2

		mov		ecx,ebx
		and		ebx,GREENMASK2

		mov		dword ptr[Red],eax
		and		ecx,BLUEMASK2

		mov		dword ptr[Green],ebx
		mov		dword ptr[Blue],ecx

		fild	qword ptr[Red]					; r    RI   LG   LB   BI   RL   GI
		fmul	st,st(5)						; R    RI   LG   LB   BI   RL   GI
		fild	qword ptr[Green]				; g    R    RI   LG   LB   BI   RL   GI
		fmul	st,st(3)						; G    R    RI   LG   LB   BI   RL   GI
		fxch	st(6)							; RL   R    RI   LG   LB   BI   G    GI
		fadd	st,st(2)						; RL2  R    RI   LG   LB   BI   G    GI
		fxch	st(3)							; LG   R    RI   RL2  LB   BI   G    GI
		fadd	st,st(7)						; LG2  R    RI   RL2  LB   BI   G    GI
		fxch	st(6)							; G    R    RI   RL2  LB   BI   LG2  GI
		fadd	qword ptr[Magic]				; Gk   R    RI   RL2  LB   BI   LG2  GI
		fxch	st(1)							; R    Gk   RI   RL2  LB   BI   LG2  GI
		fadd	qword ptr[Magic]				; Rk   Gk   RI   RL2  LB   BI   LG2  GI
		fxch	st(1)							; Gk   Rk   RI   RL2  LB   BI   LG2  GI
		fstp	qword ptr[Bucket]				; Rk   RI   RL2  LB   BI   LG2  GI
		fstp	qword ptr[Bucket2]				; RI   RL2  LB   BI   LG2  GI

		mov		edx,dword ptr[Bucket]
		mov		eax,dword ptr[Bucket2]

		fild	dword ptr[Blue]					; b    RI   RL2  LB   BI   LG2  GI
		fmul	st,st(3)						; B    RI   RL2  LB   BI   LG2  GI
		fxch	st(3)							; LB   RI   RL2  B    BI   LG2  GI
		fadd	st,st(4)						; LB2  RI   RL2  B    BI   LG2  GI
		fxch	st(3)							; B    RI   RL2  LB2  BI   LG2  GI
		fadd	qword ptr[Magic]				; Bk   RI   RL2  LB2  BI   LG2  GI

		and		edx,GREENMASK2
		and		eax,REDMASK2

		fstp	qword ptr[Bucket]				; RI   RL2  LB2  BI   LG2  GI
		fstp	qword ptr[Bucket2]				; RL2  LB2  BI   LG2  GI

		mov		edi,edx
		mov		ebx,dword ptr[Bucket]
		or		edi,eax
		mov		ebp,TDest
		and		ebx,BLUEMASK2

		fxch	st(3)							; LG2  LB2  BI   RL2  GI
		fld		qword ptr[Bucket2]				; RI   LG2  LB2  BI   RL2  GI

		mov		si,word ptr[Z32]
		add		TDest,4

		mov		ecx,pZBufferPtr
		or		edi,ebx

		cmp		word ptr[ecx],si
		jg		SkipPixie

		rol		edi,16

		mov		[ecx],si
		mov		dword ptr[ebp],edi
		mov		[ecx+2],si
SkipPixie:
		add		pZBufferPtr,4
		dec		[widTemp]

		jnz		PixieLoop
//		mov		dword ptr[ebp],0ffffffffh

		pop		ebp

		fstp	[u16]
		fstp	[v16]
		fstp	[u16]
		fstp	[v16]
		fstp	[u16]
		fstp	[v16]

GouraudReturnNoZ:
		pop		edi
		pop		esi
		pop		ecx
		pop		ebx
	}
}

void	DrawScanLineGouraudZBufferNoZWrite_Asm555X86FPU(EdgeAsmFPU *pLeft, EdgeAsmFPU *pRight)
{
	TDest	=Dest;
	Red		=Green	=0;
	_asm
	{
		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		ebx,pLeft
		mov     ecx,pRight
		mov     eax,[ebx]EdgeAsmFPU.X
		mov     edx,[ecx]EdgeAsmFPU.X
		sub     edx,eax
		jle		GouraudReturnNoZ

		mov		esi,eax
		inc		edx
		shl		eax,1
		add		TDest,eax
		add		pZBufferPtr,eax
		test	esi,1		//dword align left
		jz		NoSinglePixie

		//odd sized spans write one pixel to dword align
		fld     dword ptr [ebx]EdgeAsmFPU.u		; UL
		fmul    [Real65536]						; UL16
		fld     dword ptr [ebx]EdgeAsmFPU.v		; VL   UL16
		fmul	[Real65536]						; VL16 UL16
		fild    dword ptr [ebx]EdgeAsmFPU.R		; RL   VL16 UL16
		fild    dword ptr [ebx]EdgeAsmFPU.G		; GL   RL   VL16 UL16
		fild    dword ptr [ebx]EdgeAsmFPU.B		; BL   GL   RL   VL16 UL16
		fxch	st(4)							; UL16 GL   RL   VL16 BL
		fistp	[u16]							; GL   RL   VL16 BL
		fxch	st(2)							; VL16 RL   GL   BL
		fistp	[v16]							; RL   GL   BL
		fld		dword ptr[ebx]EdgeAsmFPU.z		; z    RL   GL   BL
		fistp	[z16]							; RL   GL   BL

		mov     [widTemp],edx

		mov		ecx,[VShift]
		mov		ebx,[GHMaskShifted]

		mov		esi,[GWMask]
		mov		edi,[v16]

		mov		edx,dword ptr[u16]

		shr		edi,cl

		shr		edx,16
		xor		eax,eax

		and		edi,ebx
		and		edx,esi

		add		edi,edx

		add		edi,GBitPtrHalf
		mov		ax,word ptr[edi*2]

		mov		ebx,eax
		and		eax,REDMASK2

		mov		ecx,ebx
		and		ebx,GREENMASK2

		mov		dword ptr[Red],eax
		and		ecx,BLUEMASK2

		mov		dword ptr[Green],ebx
		mov		dword ptr[Blue],ecx

		fimul	dword ptr[Red]					; R    GL   BL
		fxch	st(1)							; GL   R    BL
		fimul	dword ptr[Green]				; G    R    BL
		fxch	st(1)							; R    G    BL
		fadd	qword ptr[Magic]				; Rk   G    BL
		fxch	st(2)							; BL   G    Rk
		fimul	[Blue]							; B    G    Rk
		fxch	st(1)							; G    B    Rk
		fadd	qword ptr[Magic]				; Gk   B    Rk
		fxch	st(2)							; Rk   B    Gk
		fstp	qword ptr[Bucket]				; B    Gk
		fadd	qword ptr[Magic]				; Bk   Gk
		fxch	st(1)							; Gk   Bk
		fstp	qword ptr[Bucket2]				; Bk

		mov		edx,dword ptr[Bucket]
		mov		eax,dword ptr[Bucket2]

		and		edx,REDMASK2
		and		eax,GREENMASK2

		fstp	qword ptr[Bucket]

		mov		ecx,TDest
		mov		ebx,dword ptr[Bucket]

		mov		esi,[z16]
		and		ebx,BLUEMASK2

		shr		esi,16
		or		edx,eax

		mov		eax,pZBufferPtr
		add		TDest,2

		cmp		word ptr[eax],si
		jg		SkipSinglePixie

		or		edx,ebx

		mov		word ptr[ecx],dx

SkipSinglePixie:
		mov		ebx,pLeft
		mov     edx,[widTemp]
		add		pZBufferPtr,2
		mov     ecx,pRight
		dec		edx
		jz		GouraudReturnNoZ
		mov		esi,edx
		and		esi,1
		sub		edx,esi
		jz		GouraudReturnNoZ

NoSinglePixie:
		mov     [widTemp],edx                 ; just for a temp
		shr		edx,1

		; try to keep fmul fxch pairs seperated to avoid stalling
		; calc this scanlines steps           ; FPU Stack
		                                      ; st0  st1  st2  st3  st4  st5  st6  st7
		fild    dword ptr [widTemp]           ; WID

		mov     [widTemp],edx                 ; Color interps doubled

		fld     dword ptr [ecx]EdgeAsmFPU.u ; UR   WID
		fld     dword ptr [ebx]EdgeAsmFPU.u ; UL   UR   WID
		fsub    st(1), st                   ; UL   UD   WID
		fld     dword ptr [ecx]EdgeAsmFPU.v ; VR   UL   UD   WID
		fxch    st(1)                       ; UL   VR   UD   WID
		fmul    [Real65536]                 ; UL16 VR   UD   WID
		fld     dword ptr [ebx]EdgeAsmFPU.v ; VL   UL16 VR   UD   WID
		fsub    st(2), st                   ; VL   UL16 VD   UD   WID
		fild    dword ptr [ecx]EdgeAsmFPU.R ; RR   VL   UL16 VD   UD   WID
		fxch    st(3)                       ; VD   VL   UL16 RR   UD   WID
		fmul    [Real65536]                 ; VD16 VL   UL16 RR   UD   WID
		fild    dword ptr [ebx]EdgeAsmFPU.R ; RL   VD16 VL   UL16 RR   UD   WID
		fsub    st(4),st                    ; RL   VD16 VL   UL16 RD   UD   WID
		fxch    st(5)                       ; UD   VD16 VL   UL16 RD   RL   WID
		fmul    [Real65536]                 ; UD16 VD16 VL   UL16 RD   RL   WID
		fxch    st(2)						; VL   VD16 UD16 UL16 RD   RL   WID
		fmul    [Real65536]                 ; VL16 VD16 UD16 UL16 RD   RL   WID
		fxch	st(3)						; UL16 VD16 UD16 VL16 RD   RL   WID
		fistp	[u16]						; VD16 UD16 VL16 RD   RL   WID
		fxch	st(2)						; VL16 UD16 VD16 RD   RL   WID
		fistp	[v16]						; UD16 VD16 RD   RL   WID
		fld1								; 1    UD16 VD16 RD   RL   WID
		fdivrp	st(5),st					; UD16 VD16 RD   RL   DWID

		//let that cook

		fmul	st,st(4)						; USTP VD16 RD   RL   DWID
		fild	[ecx]EdgeAsmFPU.G				; RG   USTP VD16 RD   RL   DWID
		fild	[ebx]EdgeAsmFPU.G				; LG   RG   USTP VD16 RD   RL   DWID
		fsub	st(1),st						; LG   GD   USTP VD16 RD   RL   DWID
		fxch	st(2)							; USTP GD   LG   VD16 RD   RL   DWID
		fistp	[UStep]							; GD   LG   VD16 RD   RL   DWID
		fild	[ecx]EdgeAsmFPU.B				; RB   GD   LG   VD16 RD   RL   DWID
		fild	[ebx]EdgeAsmFPU.B				; LB   RB   GD   LG   VD16 RD   RL   DWID
		fsub	st(1),st						; LB   BD   GD   LG   VD16 RD   RL   DWID
		fxch	st(4)							; VD16 BD   GD   LG   LB   RD   RL   DWID
		fmul	st,st(7)						; VSTP BD   GD   LG   LB   RD   RL   DWID
		fxch	st(7)							; DWID BD   GD   LG   LB   RD   RL   VSTP
		fmul	dword ptr[Two]					; DW2  BD   GD   LG   LB   RD   RL   VSTP
		fxch	st(7)							; VSTP BD   GD   LG   LB   RD   RL   DW2
		fxch	st(5)							; RD   BD   GD   LG   LB   VSTP RL   DW2
		fmul	st,st(7)						; RI   BD   GD   LG   LB   VSTP RL   DW2
		frndint
		fxch	st(2)							; GD   BD   RI   LG   LB   VSTP RL   DW2
		fmul	st,st(7)						; GI   BD   RI   LG   LB   VSTP RL   DW2
		frndint
		fxch	st(1)							; BD   GI   RI   LG   LB   VSTP RL   DW2
		fmul	st,st(7)						; BD   GI   RI   LG   LB   VSTP RL   DW2
		frndint
		fxch	st(5)							; VSTP GI   RI   LG   LB   BD   RL   DW2
		fistp	[VStep]							; GI   RI   LG   LB   BD   RL   DW2
		fld		[ecx]EdgeAsmFPU.z				; rz   GI   RI   LG   LB   BD   RL   DW2
		fsub	[ebx]EdgeAsmFPU.z				; zd   GI   RI   LG   LB   BD   RL   DW2
		fxch	st(7)							; DW2  GI   RI   LG   LB   BD   RL   zd
		fmulp	st(7),st						; GI   RI   LG   LB   BD   RL   zd
		fld		[ebx]EdgeAsmFPU.z				; lz   GI   RI   LG   LB   BD   RL   zd
		fxch	st(7)							; zd   GI   RI   LG   LB   BD   RL   lz
		fistp	[ZStep]							; GI   RI   LG   LB   BD   RL   lz
		fxch	st(6)							; lz   RI   LG   LB   BD   RL   GI
		fistp	[z16]							; RI   LG   LB   BD   RL   GI

		push	ebp

PixieLoop:
		mov		ebx,[ZStep]

		add		[z16],ebx
		mov		ecx,[VShift]

		mov		edi,[z16]

		shr		edi,16
		mov		ebx,[GHMaskShifted]

		mov		ebp,[GWMask]
		mov		edx,dword ptr[u16]

		mov		word ptr[Z32],di
		mov		eax,edx

		shr		edx,16
		mov		edi,[v16]

		add		eax,[UStep]
		mov		esi,edi

		shr		edi,cl
		add		esi,[VStep]

		and		edi,ebx
		and		edx,ebp

		add		edi,edx
		mov		edx,eax

		add		edi,GBitPtrHalf
		add		eax,[UStep]

		mov		[u16],eax
		mov		ax,word ptr[edi*2]

		mov		edi,esi
		add		esi,[VStep]

		shr		edx,16
		mov		[v16],esi

		shr		edi,cl
		and		edx,ebp

		mov		esi,GBitPtrHalf
		and		edi,ebx

		shl		eax,16
		add		esi,edx

		add		esi,edi

		mov		ax,word ptr[esi*2]

		mov		ebx,eax
		and		eax,REDMASK2

		mov		ecx,ebx
		and		ebx,GREENMASK2

		mov		dword ptr[Red],eax
		and		ecx,BLUEMASK2

		mov		dword ptr[Green],ebx
		mov		dword ptr[Blue],ecx

		fild	qword ptr[Red]					; r    RI   LG   LB   BI   RL   GI
		fmul	st,st(5)						; R    RI   LG   LB   BI   RL   GI
		fild	qword ptr[Green]				; g    R    RI   LG   LB   BI   RL   GI
		fmul	st,st(3)						; G    R    RI   LG   LB   BI   RL   GI
		fxch	st(6)							; RL   R    RI   LG   LB   BI   G    GI
		fadd	st,st(2)						; RL2  R    RI   LG   LB   BI   G    GI
		fxch	st(3)							; LG   R    RI   RL2  LB   BI   G    GI
		fadd	st,st(7)						; LG2  R    RI   RL2  LB   BI   G    GI
		fxch	st(6)							; G    R    RI   RL2  LB   BI   LG2  GI
		fadd	qword ptr[Magic]				; Gk   R    RI   RL2  LB   BI   LG2  GI
		fxch	st(1)							; R    Gk   RI   RL2  LB   BI   LG2  GI
		fadd	qword ptr[Magic]				; Rk   Gk   RI   RL2  LB   BI   LG2  GI
		fxch	st(1)							; Gk   Rk   RI   RL2  LB   BI   LG2  GI
		fstp	qword ptr[Bucket]				; Rk   RI   RL2  LB   BI   LG2  GI
		fstp	qword ptr[Bucket2]				; RI   RL2  LB   BI   LG2  GI

		mov		edx,dword ptr[Bucket]
		mov		eax,dword ptr[Bucket2]

		fild	dword ptr[Blue]					; b    RI   RL2  LB   BI   LG2  GI
		fmul	st,st(3)						; B    RI   RL2  LB   BI   LG2  GI
		fxch	st(3)							; LB   RI   RL2  B    BI   LG2  GI
		fadd	st,st(4)						; LB2  RI   RL2  B    BI   LG2  GI
		fxch	st(3)							; B    RI   RL2  LB2  BI   LG2  GI
		fadd	qword ptr[Magic]				; Bk   RI   RL2  LB2  BI   LG2  GI

		and		edx,GREENMASK2
		and		eax,REDMASK2

		fstp	qword ptr[Bucket]				; RI   RL2  LB2  BI   LG2  GI
		fstp	qword ptr[Bucket2]				; RL2  LB2  BI   LG2  GI

		mov		edi,edx
		mov		ebx,dword ptr[Bucket]
		or		edi,eax
		mov		ebp,TDest
		and		ebx,BLUEMASK2

		fxch	st(3)							; LG2  LB2  BI   RL2  GI
		fld		qword ptr[Bucket2]				; RI   LG2  LB2  BI   RL2  GI

		mov		si,word ptr[Z32]
		add		TDest,4

		mov		ecx,pZBufferPtr
		or		edi,ebx

		cmp		word ptr[ecx],si
		jg		SkipPixie

		rol		edi,16

		mov		dword ptr[ebp],edi
SkipPixie:
		add		pZBufferPtr,4
		dec		[widTemp]

		jnz		PixieLoop

		pop		ebp

		fstp	[u16]
		fstp	[v16]
		fstp	[u16]
		fstp	[v16]
		fstp	[u16]
		fstp	[v16]

GouraudReturnNoZ:
		pop		edi
		pop		esi
		pop		ecx
		pop		ebx
	}
}

void	DrawScanLineGouraudNoZBufferZWrite_Asm555X86FPU(EdgeAsmFPU *pLeft, EdgeAsmFPU *pRight)
{
	TDest	=Dest;
	Red		=Green	=0;
	_asm
	{
		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		ebx,pLeft
		mov     ecx,pRight
		mov     eax,[ebx]EdgeAsmFPU.X
		mov     edx,[ecx]EdgeAsmFPU.X
		sub     edx,eax
		jle		GouraudReturnNoZ

		mov		esi,eax
		inc		edx
		shl		eax,1
		add		TDest,eax
		add		pZBufferPtr,eax
		test	esi,1		//dword align left
		jz		NoSinglePixie

		//odd sized spans write one pixel to dword align
		fld     dword ptr [ebx]EdgeAsmFPU.u		; UL
		fmul    [Real65536]						; UL16
		fld     dword ptr [ebx]EdgeAsmFPU.v		; VL   UL16
		fmul	[Real65536]						; VL16 UL16
		fild    dword ptr [ebx]EdgeAsmFPU.R		; RL   VL16 UL16
		fild    dword ptr [ebx]EdgeAsmFPU.G		; GL   RL   VL16 UL16
		fild    dword ptr [ebx]EdgeAsmFPU.B		; BL   GL   RL   VL16 UL16
		fxch	st(4)							; UL16 GL   RL   VL16 BL
		fistp	[u16]							; GL   RL   VL16 BL
		fxch	st(2)							; VL16 RL   GL   BL
		fistp	[v16]							; RL   GL   BL
		fld		dword ptr[ebx]EdgeAsmFPU.z		; z    RL   GL   BL
		fistp	[z16]							; RL   GL   BL

		mov     [widTemp],edx

		mov		ecx,[VShift]
		mov		ebx,[GHMaskShifted]

		mov		esi,[GWMask]
		mov		edi,[v16]

		mov		edx,dword ptr[u16]

		shr		edi,cl

		shr		edx,16
		xor		eax,eax

		and		edi,ebx
		and		edx,esi

		add		edi,edx

		add		edi,GBitPtrHalf
		mov		ax,word ptr[edi*2]

		mov		ebx,eax
		and		eax,REDMASK2

		mov		ecx,ebx
		and		ebx,GREENMASK2

		mov		dword ptr[Red],eax
		and		ecx,BLUEMASK2

		mov		dword ptr[Green],ebx
		mov		dword ptr[Blue],ecx

		fimul	dword ptr[Red]					; R    GL   BL
		fxch	st(1)							; GL   R    BL
		fimul	dword ptr[Green]				; G    R    BL
		fxch	st(1)							; R    G    BL
		fadd	qword ptr[Magic]				; Rk   G    BL
		fxch	st(2)							; BL   G    Rk
		fimul	[Blue]							; B    G    Rk
		fxch	st(1)							; G    B    Rk
		fadd	qword ptr[Magic]				; Gk   B    Rk
		fxch	st(2)							; Rk   B    Gk
		fstp	qword ptr[Bucket]				; B    Gk
		fadd	qword ptr[Magic]				; Bk   Gk
		fxch	st(1)							; Gk   Bk
		fstp	qword ptr[Bucket2]				; Bk

		mov		edx,dword ptr[Bucket]
		mov		eax,dword ptr[Bucket2]

		and		edx,REDMASK2
		and		eax,GREENMASK2

		fstp	qword ptr[Bucket]

		mov		ecx,TDest
		mov		ebx,dword ptr[Bucket]

		mov		esi,[z16]
		and		ebx,BLUEMASK2

		shr		esi,16
		or		edx,eax

		mov		eax,pZBufferPtr
		add		TDest,2

		or		edx,ebx
		mov		word ptr[eax],si

		mov		word ptr[ecx],dx

		mov		ebx,pLeft
		mov     edx,[widTemp]
		add		pZBufferPtr,2
		mov     ecx,pRight
		dec		edx
		jz		GouraudReturnNoZ
		mov		esi,edx
		and		esi,1
		sub		edx,esi
		jz		GouraudReturnNoZ

NoSinglePixie:
		mov     [widTemp],edx                 ; just for a temp
		shr		edx,1

		; try to keep fmul fxch pairs seperated to avoid stalling
		; calc this scanlines steps           ; FPU Stack
		                                      ; st0  st1  st2  st3  st4  st5  st6  st7
		fild    dword ptr [widTemp]           ; WID

		mov     [widTemp],edx                 ; Color interps doubled

		fld     dword ptr [ecx]EdgeAsmFPU.u ; UR   WID
		fld     dword ptr [ebx]EdgeAsmFPU.u ; UL   UR   WID
		fsub    st(1), st                   ; UL   UD   WID
		fld     dword ptr [ecx]EdgeAsmFPU.v ; VR   UL   UD   WID
		fxch    st(1)                       ; UL   VR   UD   WID
		fmul    [Real65536]                 ; UL16 VR   UD   WID
		fld     dword ptr [ebx]EdgeAsmFPU.v ; VL   UL16 VR   UD   WID
		fsub    st(2), st                   ; VL   UL16 VD   UD   WID
		fild    dword ptr [ecx]EdgeAsmFPU.R ; RR   VL   UL16 VD   UD   WID
		fxch    st(3)                       ; VD   VL   UL16 RR   UD   WID
		fmul    [Real65536]                 ; VD16 VL   UL16 RR   UD   WID
		fild    dword ptr [ebx]EdgeAsmFPU.R ; RL   VD16 VL   UL16 RR   UD   WID
		fsub    st(4),st                    ; RL   VD16 VL   UL16 RD   UD   WID
		fxch    st(5)                       ; UD   VD16 VL   UL16 RD   RL   WID
		fmul    [Real65536]                 ; UD16 VD16 VL   UL16 RD   RL   WID
		fxch    st(2)						; VL   VD16 UD16 UL16 RD   RL   WID
		fmul    [Real65536]                 ; VL16 VD16 UD16 UL16 RD   RL   WID
		fxch	st(3)						; UL16 VD16 UD16 VL16 RD   RL   WID
		fistp	[u16]						; VD16 UD16 VL16 RD   RL   WID
		fxch	st(2)						; VL16 UD16 VD16 RD   RL   WID
		fistp	[v16]						; UD16 VD16 RD   RL   WID
		fld1								; 1    UD16 VD16 RD   RL   WID
		fdivrp	st(5),st					; UD16 VD16 RD   RL   DWID

		//let that cook

		fmul	st,st(4)						; USTP VD16 RD   RL   DWID
		fild	[ecx]EdgeAsmFPU.G				; RG   USTP VD16 RD   RL   DWID
		fild	[ebx]EdgeAsmFPU.G				; LG   RG   USTP VD16 RD   RL   DWID
		fsub	st(1),st						; LG   GD   USTP VD16 RD   RL   DWID
		fxch	st(2)							; USTP GD   LG   VD16 RD   RL   DWID
		fistp	[UStep]							; GD   LG   VD16 RD   RL   DWID
		fild	[ecx]EdgeAsmFPU.B				; RB   GD   LG   VD16 RD   RL   DWID
		fild	[ebx]EdgeAsmFPU.B				; LB   RB   GD   LG   VD16 RD   RL   DWID
		fsub	st(1),st						; LB   BD   GD   LG   VD16 RD   RL   DWID
		fxch	st(4)							; VD16 BD   GD   LG   LB   RD   RL   DWID
		fmul	st,st(7)						; VSTP BD   GD   LG   LB   RD   RL   DWID
		fxch	st(7)							; DWID BD   GD   LG   LB   RD   RL   VSTP
		fmul	dword ptr[Two]					; DW2  BD   GD   LG   LB   RD   RL   VSTP
		fxch	st(7)							; VSTP BD   GD   LG   LB   RD   RL   DW2
		fxch	st(5)							; RD   BD   GD   LG   LB   VSTP RL   DW2
		fmul	st,st(7)						; RI   BD   GD   LG   LB   VSTP RL   DW2
		frndint
		fxch	st(2)							; GD   BD   RI   LG   LB   VSTP RL   DW2
		fmul	st,st(7)						; GI   BD   RI   LG   LB   VSTP RL   DW2
		frndint
		fxch	st(1)							; BD   GI   RI   LG   LB   VSTP RL   DW2
		fmul	st,st(7)						; BD   GI   RI   LG   LB   VSTP RL   DW2
		frndint
		fxch	st(5)							; VSTP GI   RI   LG   LB   BD   RL   DW2
		fistp	[VStep]							; GI   RI   LG   LB   BD   RL   DW2
		fld		[ecx]EdgeAsmFPU.z				; rz   GI   RI   LG   LB   BD   RL   DW2
		fsub	[ebx]EdgeAsmFPU.z				; zd   GI   RI   LG   LB   BD   RL   DW2
		fxch	st(7)							; DW2  GI   RI   LG   LB   BD   RL   zd
		fmulp	st(7),st						; GI   RI   LG   LB   BD   RL   zd
		fld		[ebx]EdgeAsmFPU.z				; lz   GI   RI   LG   LB   BD   RL   zd
		fxch	st(7)							; zd   GI   RI   LG   LB   BD   RL   lz
		fistp	[ZStep]							; GI   RI   LG   LB   BD   RL   lz
		fxch	st(6)							; lz   RI   LG   LB   BD   RL   GI
		fistp	[z16]							; RI   LG   LB   BD   RL   GI

		push	ebp

PixieLoop:
		mov		ebx,[ZStep]

		add		[z16],ebx
		mov		ecx,[VShift]

		mov		edi,[z16]

		shr		edi,16
		mov		ebx,[GHMaskShifted]

		mov		ebp,[GWMask]
		mov		edx,dword ptr[u16]

		mov		word ptr[Z32],di
		mov		eax,edx

		shr		edx,16
		mov		edi,[v16]

		add		eax,[UStep]
		mov		esi,edi

		shr		edi,cl
		add		esi,[VStep]

		and		edi,ebx
		and		edx,ebp

		add		edi,edx
		mov		edx,eax

		add		edi,GBitPtrHalf
		add		eax,[UStep]

		mov		[u16],eax
		mov		ax,word ptr[edi*2]

		mov		edi,esi
		add		esi,[VStep]

		shr		edx,16
		mov		[v16],esi

		shr		edi,cl
		and		edx,ebp

		mov		esi,GBitPtrHalf
		and		edi,ebx

		shl		eax,16
		add		esi,edx

		add		esi,edi

		mov		ax,word ptr[esi*2]

		mov		ebx,eax
		and		eax,REDMASK2

		mov		ecx,ebx
		and		ebx,GREENMASK2

		mov		dword ptr[Red],eax
		and		ecx,BLUEMASK2

		mov		dword ptr[Green],ebx
		mov		dword ptr[Blue],ecx

		fild	qword ptr[Red]					; r    RI   LG   LB   BI   RL   GI
		fmul	st,st(5)						; R    RI   LG   LB   BI   RL   GI
		fild	qword ptr[Green]				; g    R    RI   LG   LB   BI   RL   GI
		fmul	st,st(3)						; G    R    RI   LG   LB   BI   RL   GI
		fxch	st(6)							; RL   R    RI   LG   LB   BI   G    GI
		fadd	st,st(2)						; RL2  R    RI   LG   LB   BI   G    GI
		fxch	st(3)							; LG   R    RI   RL2  LB   BI   G    GI
		fadd	st,st(7)						; LG2  R    RI   RL2  LB   BI   G    GI
		fxch	st(6)							; G    R    RI   RL2  LB   BI   LG2  GI
		fadd	qword ptr[Magic]				; Gk   R    RI   RL2  LB   BI   LG2  GI
		fxch	st(1)							; R    Gk   RI   RL2  LB   BI   LG2  GI
		fadd	qword ptr[Magic]				; Rk   Gk   RI   RL2  LB   BI   LG2  GI
		fxch	st(1)							; Gk   Rk   RI   RL2  LB   BI   LG2  GI
		fstp	qword ptr[Bucket]				; Rk   RI   RL2  LB   BI   LG2  GI
		fstp	qword ptr[Bucket2]				; RI   RL2  LB   BI   LG2  GI

		mov		edx,dword ptr[Bucket]
		mov		eax,dword ptr[Bucket2]

		fild	dword ptr[Blue]					; b    RI   RL2  LB   BI   LG2  GI
		fmul	st,st(3)						; B    RI   RL2  LB   BI   LG2  GI
		fxch	st(3)							; LB   RI   RL2  B    BI   LG2  GI
		fadd	st,st(4)						; LB2  RI   RL2  B    BI   LG2  GI
		fxch	st(3)							; B    RI   RL2  LB2  BI   LG2  GI
		fadd	qword ptr[Magic]				; Bk   RI   RL2  LB2  BI   LG2  GI

		and		edx,GREENMASK2
		and		eax,REDMASK2

		fstp	qword ptr[Bucket]				; RI   RL2  LB2  BI   LG2  GI
		fstp	qword ptr[Bucket2]				; RL2  LB2  BI   LG2  GI

		mov		edi,edx
		mov		ebx,dword ptr[Bucket]
		or		edi,eax
		mov		ebp,TDest
		and		ebx,BLUEMASK2

		fxch	st(3)							; LG2  LB2  BI   RL2  GI
		fld		qword ptr[Bucket2]				; RI   LG2  LB2  BI   RL2  GI

		mov		si,word ptr[Z32]
		add		TDest,4

		mov		ecx,pZBufferPtr
		or		edi,ebx

		mov		[ecx],si
		rol		edi,16

		mov		dword ptr[ebp],edi
		mov		[ecx+2],si

		add		pZBufferPtr,4
		dec		[widTemp]

		jnz		PixieLoop

		pop		ebp

		fstp	[u16]
		fstp	[v16]
		fstp	[u16]
		fstp	[v16]
		fstp	[u16]
		fstp	[v16]

GouraudReturnNoZ:
		pop		edi
		pop		esi
		pop		ecx
		pop		ebx
	}
}

void	DrawScanLineGouraudZBufferTrans_Asm555X86FPU(EdgeAsmFPU *pLeft, EdgeAsmFPU *pRight)
{
	TDest	=Dest;
	Red		=Green	=0;
	_asm
	{
		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		ebx,pLeft
		mov     ecx,pRight
		mov     eax,[ebx]EdgeAsmFPU.X
		mov     edx,[ecx]EdgeAsmFPU.X
		sub     edx,eax
		jle		GouraudReturnNoZ

		mov		esi,eax
		inc		edx
		shl		eax,1
		add		TDest,eax
		add		pZBufferPtr,eax
		test	esi,1		//dword align left
		jz		NoSinglePixie

		//odd sized spans write one pixel to dword align
		fld     dword ptr [ebx]EdgeAsmFPU.u		; UL
		fmul    [Real65536]						; UL16
		fld     dword ptr [ebx]EdgeAsmFPU.v		; VL   UL16
		fmul	[Real65536]						; VL16 UL16
		fild    dword ptr [ebx]EdgeAsmFPU.R		; RL   VL16 UL16
		fild    dword ptr [ebx]EdgeAsmFPU.G		; GL   RL   VL16 UL16
		fild    dword ptr [ebx]EdgeAsmFPU.B		; BL   GL   RL   VL16 UL16
		fxch	st(4)							; UL16 GL   RL   VL16 BL
		fistp	[u16]							; GL   RL   VL16 BL
		fxch	st(2)							; VL16 RL   GL   BL
		fistp	[v16]							; RL   GL   BL
		fld		dword ptr[ebx]EdgeAsmFPU.z		; z    RL   GL   BL
		fistp	[z16]							; RL   GL   BL

		mov     [widTemp],edx

		mov		ecx,[VShift]
		mov		ebx,[GHMaskShifted]

		mov		esi,[GWMask]
		mov		edi,[v16]

		mov		edx,dword ptr[u16]

		shr		edi,cl

		shr		edx,16
		xor		eax,eax

		and		edi,ebx
		and		edx,esi

		add		edi,edx

		add		edi,GBitPtrHalf
		mov		ax,word ptr[edi*2]

		mov		word ptr[TempPix],ax

		mov		ebx,eax
		and		eax,REDMASK2

		mov		ecx,ebx
		and		ebx,GREENMASK2

		mov		dword ptr[Red],eax
		and		ecx,BLUEMASK2

		mov		dword ptr[Green],ebx
		mov		dword ptr[Blue],ecx

		fimul	dword ptr[Red]					; R    GL   BL
		fxch	st(1)							; GL   R    BL
		fimul	dword ptr[Green]				; G    R    BL
		fxch	st(1)							; R    G    BL
		fadd	qword ptr[Magic]				; Rk   G    BL
		fxch	st(2)							; BL   G    Rk
		fimul	[Blue]							; B    G    Rk
		fxch	st(1)							; G    B    Rk
		fadd	qword ptr[Magic]				; Gk   B    Rk
		fxch	st(2)							; Rk   B    Gk
		fstp	qword ptr[Bucket]				; B    Gk
		fadd	qword ptr[Magic]				; Bk   Gk
		fxch	st(1)							; Gk   Bk
		fstp	qword ptr[Bucket2]				; Bk

		mov		edx,dword ptr[Bucket]
		mov		eax,dword ptr[Bucket2]

		and		edx,REDMASK2
		and		eax,GREENMASK2

		fstp	qword ptr[Bucket]

		mov		ecx,TDest
		mov		ebx,dword ptr[Bucket]

		mov		esi,[z16]
		and		ebx,BLUEMASK2

		shr		esi,16
		or		edx,eax

		mov		eax,pZBufferPtr
		add		TDest,2

		cmp		word ptr[TempPix],01h
		je		SkipSinglePixie

		cmp		word ptr[eax],si
		jg		SkipSinglePixie

		or		edx,ebx
		mov		word ptr[eax],si

		mov		word ptr[ecx],dx

SkipSinglePixie:
		mov		ebx,pLeft
		mov     edx,[widTemp]
		add		pZBufferPtr,2
		mov     ecx,pRight
		dec		edx
		jz		GouraudReturnNoZ
		mov		esi,edx
		and		esi,1
		sub		edx,esi
		jz		GouraudReturnNoZ

NoSinglePixie:
		mov     [widTemp],edx                 ; just for a temp
		shr		edx,1

		; try to keep fmul fxch pairs seperated to avoid stalling
		; calc this scanlines steps           ; FPU Stack
		                                      ; st0  st1  st2  st3  st4  st5  st6  st7
		fild    dword ptr [widTemp]           ; WID

		mov     [widTemp],edx                 ; Color interps doubled

		fld     dword ptr [ecx]EdgeAsmFPU.u ; UR   WID
		fld     dword ptr [ebx]EdgeAsmFPU.u ; UL   UR   WID
		fsub    st(1), st                   ; UL   UD   WID
		fld     dword ptr [ecx]EdgeAsmFPU.v ; VR   UL   UD   WID
		fxch    st(1)                       ; UL   VR   UD   WID
		fmul    [Real65536]                 ; UL16 VR   UD   WID
		fld     dword ptr [ebx]EdgeAsmFPU.v ; VL   UL16 VR   UD   WID
		fsub    st(2), st                   ; VL   UL16 VD   UD   WID
		fild    dword ptr [ecx]EdgeAsmFPU.R ; RR   VL   UL16 VD   UD   WID
		fxch    st(3)                       ; VD   VL   UL16 RR   UD   WID
		fmul    [Real65536]                 ; VD16 VL   UL16 RR   UD   WID
		fild    dword ptr [ebx]EdgeAsmFPU.R ; RL   VD16 VL   UL16 RR   UD   WID
		fsub    st(4),st                    ; RL   VD16 VL   UL16 RD   UD   WID
		fxch    st(5)                       ; UD   VD16 VL   UL16 RD   RL   WID
		fmul    [Real65536]                 ; UD16 VD16 VL   UL16 RD   RL   WID
		fxch    st(2)						; VL   VD16 UD16 UL16 RD   RL   WID
		fmul    [Real65536]                 ; VL16 VD16 UD16 UL16 RD   RL   WID
		fxch	st(3)						; UL16 VD16 UD16 VL16 RD   RL   WID
		fistp	[u16]						; VD16 UD16 VL16 RD   RL   WID
		fxch	st(2)						; VL16 UD16 VD16 RD   RL   WID
		fistp	[v16]						; UD16 VD16 RD   RL   WID
		fld1								; 1    UD16 VD16 RD   RL   WID
		fdivrp	st(5),st					; UD16 VD16 RD   RL   DWID

		//let that cook

		fmul	st,st(4)						; USTP VD16 RD   RL   DWID
		fild	[ecx]EdgeAsmFPU.G				; RG   USTP VD16 RD   RL   DWID
		fild	[ebx]EdgeAsmFPU.G				; LG   RG   USTP VD16 RD   RL   DWID
		fsub	st(1),st						; LG   GD   USTP VD16 RD   RL   DWID
		fxch	st(2)							; USTP GD   LG   VD16 RD   RL   DWID
		fistp	[UStep]							; GD   LG   VD16 RD   RL   DWID
		fild	[ecx]EdgeAsmFPU.B				; RB   GD   LG   VD16 RD   RL   DWID
		fild	[ebx]EdgeAsmFPU.B				; LB   RB   GD   LG   VD16 RD   RL   DWID
		fsub	st(1),st						; LB   BD   GD   LG   VD16 RD   RL   DWID
		fxch	st(4)							; VD16 BD   GD   LG   LB   RD   RL   DWID
		fmul	st,st(7)						; VSTP BD   GD   LG   LB   RD   RL   DWID
		fxch	st(7)							; DWID BD   GD   LG   LB   RD   RL   VSTP
		fmul	dword ptr[Two]					; DW2  BD   GD   LG   LB   RD   RL   VSTP
		fxch	st(7)							; VSTP BD   GD   LG   LB   RD   RL   DW2
		fxch	st(5)							; RD   BD   GD   LG   LB   VSTP RL   DW2
		fmul	st,st(7)						; RI   BD   GD   LG   LB   VSTP RL   DW2
		frndint
		fxch	st(2)							; GD   BD   RI   LG   LB   VSTP RL   DW2
		fmul	st,st(7)						; GI   BD   RI   LG   LB   VSTP RL   DW2
		frndint
		fxch	st(1)							; BD   GI   RI   LG   LB   VSTP RL   DW2
		fmul	st,st(7)						; BD   GI   RI   LG   LB   VSTP RL   DW2
		frndint
		fxch	st(5)							; VSTP GI   RI   LG   LB   BD   RL   DW2
		fistp	[VStep]							; GI   RI   LG   LB   BD   RL   DW2
		fld		[ecx]EdgeAsmFPU.z				; rz   GI   RI   LG   LB   BD   RL   DW2
		fsub	[ebx]EdgeAsmFPU.z				; zd   GI   RI   LG   LB   BD   RL   DW2
		fxch	st(7)							; DW2  GI   RI   LG   LB   BD   RL   zd
		fmulp	st(7),st						; GI   RI   LG   LB   BD   RL   zd
		fld		[ebx]EdgeAsmFPU.z				; lz   GI   RI   LG   LB   BD   RL   zd
		fxch	st(7)							; zd   GI   RI   LG   LB   BD   RL   lz
		fistp	[ZStep]							; GI   RI   LG   LB   BD   RL   lz
		fxch	st(6)							; lz   RI   LG   LB   BD   RL   GI
		fistp	[z16]							; RI   LG   LB   BD   RL   GI

		push	ebp

PixieLoop:
		mov		ebx,[ZStep]

		add		[z16],ebx
		mov		ecx,[VShift]

		mov		edi,[z16]

		shr		edi,16
		mov		ebx,[GHMaskShifted]

		mov		ebp,[GWMask]
		mov		edx,dword ptr[u16]

		mov		word ptr[Z32],di
		mov		eax,edx

		shr		edx,16
		mov		edi,[v16]

		add		eax,[UStep]
		mov		esi,edi

		shr		edi,cl
		add		esi,[VStep]

		and		edi,ebx
		and		edx,ebp

		add		edi,edx
		mov		edx,eax

		add		edi,GBitPtrHalf
		add		eax,[UStep]

		mov		[u16],eax
		mov		ax,word ptr[edi*2]

		mov		edi,esi
		add		esi,[VStep]

		shr		edx,16
		mov		[v16],esi

		shr		edi,cl
		and		edx,ebp

		mov		esi,GBitPtrHalf
		and		edi,ebx

		shl		eax,16
		add		esi,edx

		add		esi,edi

		mov		ax,word ptr[esi*2]

		mov		[TempPix],eax

		mov		ebx,eax
		and		eax,REDMASK2

		mov		ecx,ebx
		and		ebx,GREENMASK2

		mov		dword ptr[Red],eax
		and		ecx,BLUEMASK2

		mov		dword ptr[Green],ebx
		mov		dword ptr[Blue],ecx

		fild	qword ptr[Red]					; r    RI   LG   LB   BI   RL   GI
		fmul	st,st(5)						; R    RI   LG   LB   BI   RL   GI
		fild	qword ptr[Green]				; g    R    RI   LG   LB   BI   RL   GI
		fmul	st,st(3)						; G    R    RI   LG   LB   BI   RL   GI
		fxch	st(6)							; RL   R    RI   LG   LB   BI   G    GI
		fadd	st,st(2)						; RL2  R    RI   LG   LB   BI   G    GI
		fxch	st(3)							; LG   R    RI   RL2  LB   BI   G    GI
		fadd	st,st(7)						; LG2  R    RI   RL2  LB   BI   G    GI
		fxch	st(6)							; G    R    RI   RL2  LB   BI   LG2  GI
		fadd	qword ptr[Magic]				; Gk   R    RI   RL2  LB   BI   LG2  GI
		fxch	st(1)							; R    Gk   RI   RL2  LB   BI   LG2  GI
		fadd	qword ptr[Magic]				; Rk   Gk   RI   RL2  LB   BI   LG2  GI
		fxch	st(1)							; Gk   Rk   RI   RL2  LB   BI   LG2  GI
		fstp	qword ptr[Bucket]				; Rk   RI   RL2  LB   BI   LG2  GI
		fstp	qword ptr[Bucket2]				; RI   RL2  LB   BI   LG2  GI

		mov		edx,dword ptr[Bucket]
		mov		eax,dword ptr[Bucket2]

		fild	dword ptr[Blue]					; b    RI   RL2  LB   BI   LG2  GI
		fmul	st,st(3)						; B    RI   RL2  LB   BI   LG2  GI
		fxch	st(3)							; LB   RI   RL2  B    BI   LG2  GI
		fadd	st,st(4)						; LB2  RI   RL2  B    BI   LG2  GI
		fxch	st(3)							; B    RI   RL2  LB2  BI   LG2  GI
		fadd	qword ptr[Magic]				; Bk   RI   RL2  LB2  BI   LG2  GI

		and		edx,GREENMASK2
		and		eax,REDMASK2

		fstp	qword ptr[Bucket]				; RI   RL2  LB2  BI   LG2  GI
		fstp	qword ptr[Bucket2]				; RL2  LB2  BI   LG2  GI

		mov		edi,edx
		mov		ebx,dword ptr[Bucket]
		or		edi,eax
		mov		ebp,TDest
		and		ebx,BLUEMASK2

		fxch	st(3)							; LG2  LB2  BI   RL2  GI
		fld		qword ptr[Bucket2]				; RI   LG2  LB2  BI   RL2  GI

		mov		si,word ptr[Z32]
		add		TDest,4

		mov		ecx,pZBufferPtr
		or		edi,ebx

		cmp		[TempPix],010001h
		je		SkipPixie

		rol		edi,16

		cmp		word ptr[ecx],si
		jg		SkipPixie

		mov		[ecx],si
		mov		dword ptr[ebp],edi
		mov		[ecx+2],si
SkipPixie:
		add		pZBufferPtr,4
		dec		[widTemp]

		jnz		PixieLoop
//		mov		dword ptr[ebp],0ffffffffh

		pop		ebp

		fstp	[u16]
		fstp	[v16]
		fstp	[u16]
		fstp	[v16]
		fstp	[u16]
		fstp	[v16]

GouraudReturnNoZ:
		pop		edi
		pop		esi
		pop		ecx
		pop		ebx
	}
}

void	DrawScanLineGouraudZBufferNoZWriteTrans_Asm555X86FPU(EdgeAsmFPU *pLeft, EdgeAsmFPU *pRight)
{
	TDest	=Dest;
	Red		=Green	=0;
	_asm
	{
		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		ebx,pLeft
		mov     ecx,pRight
		mov     eax,[ebx]EdgeAsmFPU.X
		mov     edx,[ecx]EdgeAsmFPU.X
		sub     edx,eax
		jle		GouraudReturnNoZ

		mov		esi,eax
		inc		edx
		shl		eax,1
		add		TDest,eax
		add		pZBufferPtr,eax
		test	esi,1		//dword align left
		jz		NoSinglePixie

		//odd sized spans write one pixel to dword align
		fld     dword ptr [ebx]EdgeAsmFPU.u		; UL
		fmul    [Real65536]						; UL16
		fld     dword ptr [ebx]EdgeAsmFPU.v		; VL   UL16
		fmul	[Real65536]						; VL16 UL16
		fild    dword ptr [ebx]EdgeAsmFPU.R		; RL   VL16 UL16
		fild    dword ptr [ebx]EdgeAsmFPU.G		; GL   RL   VL16 UL16
		fild    dword ptr [ebx]EdgeAsmFPU.B		; BL   GL   RL   VL16 UL16
		fxch	st(4)							; UL16 GL   RL   VL16 BL
		fistp	[u16]							; GL   RL   VL16 BL
		fxch	st(2)							; VL16 RL   GL   BL
		fistp	[v16]							; RL   GL   BL
		fld		dword ptr[ebx]EdgeAsmFPU.z		; z    RL   GL   BL
		fistp	[z16]							; RL   GL   BL

		mov     [widTemp],edx

		mov		ecx,[VShift]
		mov		ebx,[GHMaskShifted]

		mov		esi,[GWMask]
		mov		edi,[v16]

		mov		edx,dword ptr[u16]

		shr		edi,cl

		shr		edx,16
		xor		eax,eax

		and		edi,ebx
		and		edx,esi

		add		edi,edx

		add		edi,GBitPtrHalf
		mov		ax,word ptr[edi*2]

		mov		word ptr[TempPix],ax

		mov		ebx,eax
		and		eax,REDMASK2

		mov		ecx,ebx
		and		ebx,GREENMASK2

		mov		dword ptr[Red],eax
		and		ecx,BLUEMASK2

		mov		dword ptr[Green],ebx
		mov		dword ptr[Blue],ecx

		fimul	dword ptr[Red]					; R    GL   BL
		fxch	st(1)							; GL   R    BL
		fimul	dword ptr[Green]				; G    R    BL
		fxch	st(1)							; R    G    BL
		fadd	qword ptr[Magic]				; Rk   G    BL
		fxch	st(2)							; BL   G    Rk
		fimul	[Blue]							; B    G    Rk
		fxch	st(1)							; G    B    Rk
		fadd	qword ptr[Magic]				; Gk   B    Rk
		fxch	st(2)							; Rk   B    Gk
		fstp	qword ptr[Bucket]				; B    Gk
		fadd	qword ptr[Magic]				; Bk   Gk
		fxch	st(1)							; Gk   Bk
		fstp	qword ptr[Bucket2]				; Bk

		mov		edx,dword ptr[Bucket]
		mov		eax,dword ptr[Bucket2]

		and		edx,REDMASK2
		and		eax,GREENMASK2

		fstp	qword ptr[Bucket]

		mov		ecx,TDest
		mov		ebx,dword ptr[Bucket]

		mov		esi,[z16]
		and		ebx,BLUEMASK2

		shr		esi,16
		or		edx,eax

		mov		eax,pZBufferPtr
		add		TDest,2

		cmp		word ptr[TempPix],01h
		je		SkipSinglePixie

		cmp		word ptr[eax],si
		jg		SkipSinglePixie

		or		edx,ebx

		mov		word ptr[ecx],dx

SkipSinglePixie:
		mov		ebx,pLeft
		mov     edx,[widTemp]
		add		pZBufferPtr,2
		mov     ecx,pRight
		dec		edx
		jz		GouraudReturnNoZ
		mov		esi,edx
		and		esi,1
		sub		edx,esi
		jz		GouraudReturnNoZ

NoSinglePixie:
		mov     [widTemp],edx                 ; just for a temp
		shr		edx,1

		; try to keep fmul fxch pairs seperated to avoid stalling
		; calc this scanlines steps           ; FPU Stack
		                                      ; st0  st1  st2  st3  st4  st5  st6  st7
		fild    dword ptr [widTemp]           ; WID

		mov     [widTemp],edx                 ; Color interps doubled

		fld     dword ptr [ecx]EdgeAsmFPU.u ; UR   WID
		fld     dword ptr [ebx]EdgeAsmFPU.u ; UL   UR   WID
		fsub    st(1), st                   ; UL   UD   WID
		fld     dword ptr [ecx]EdgeAsmFPU.v ; VR   UL   UD   WID
		fxch    st(1)                       ; UL   VR   UD   WID
		fmul    [Real65536]                 ; UL16 VR   UD   WID
		fld     dword ptr [ebx]EdgeAsmFPU.v ; VL   UL16 VR   UD   WID
		fsub    st(2), st                   ; VL   UL16 VD   UD   WID
		fild    dword ptr [ecx]EdgeAsmFPU.R ; RR   VL   UL16 VD   UD   WID
		fxch    st(3)                       ; VD   VL   UL16 RR   UD   WID
		fmul    [Real65536]                 ; VD16 VL   UL16 RR   UD   WID
		fild    dword ptr [ebx]EdgeAsmFPU.R ; RL   VD16 VL   UL16 RR   UD   WID
		fsub    st(4),st                    ; RL   VD16 VL   UL16 RD   UD   WID
		fxch    st(5)                       ; UD   VD16 VL   UL16 RD   RL   WID
		fmul    [Real65536]                 ; UD16 VD16 VL   UL16 RD   RL   WID
		fxch    st(2)						; VL   VD16 UD16 UL16 RD   RL   WID
		fmul    [Real65536]                 ; VL16 VD16 UD16 UL16 RD   RL   WID
		fxch	st(3)						; UL16 VD16 UD16 VL16 RD   RL   WID
		fistp	[u16]						; VD16 UD16 VL16 RD   RL   WID
		fxch	st(2)						; VL16 UD16 VD16 RD   RL   WID
		fistp	[v16]						; UD16 VD16 RD   RL   WID
		fld1								; 1    UD16 VD16 RD   RL   WID
		fdivrp	st(5),st					; UD16 VD16 RD   RL   DWID

		//let that cook

		fmul	st,st(4)						; USTP VD16 RD   RL   DWID
		fild	[ecx]EdgeAsmFPU.G				; RG   USTP VD16 RD   RL   DWID
		fild	[ebx]EdgeAsmFPU.G				; LG   RG   USTP VD16 RD   RL   DWID
		fsub	st(1),st						; LG   GD   USTP VD16 RD   RL   DWID
		fxch	st(2)							; USTP GD   LG   VD16 RD   RL   DWID
		fistp	[UStep]							; GD   LG   VD16 RD   RL   DWID
		fild	[ecx]EdgeAsmFPU.B				; RB   GD   LG   VD16 RD   RL   DWID
		fild	[ebx]EdgeAsmFPU.B				; LB   RB   GD   LG   VD16 RD   RL   DWID
		fsub	st(1),st						; LB   BD   GD   LG   VD16 RD   RL   DWID
		fxch	st(4)							; VD16 BD   GD   LG   LB   RD   RL   DWID
		fmul	st,st(7)						; VSTP BD   GD   LG   LB   RD   RL   DWID
		fxch	st(7)							; DWID BD   GD   LG   LB   RD   RL   VSTP
		fmul	dword ptr[Two]					; DW2  BD   GD   LG   LB   RD   RL   VSTP
		fxch	st(7)							; VSTP BD   GD   LG   LB   RD   RL   DW2
		fxch	st(5)							; RD   BD   GD   LG   LB   VSTP RL   DW2
		fmul	st,st(7)						; RI   BD   GD   LG   LB   VSTP RL   DW2
		frndint
		fxch	st(2)							; GD   BD   RI   LG   LB   VSTP RL   DW2
		fmul	st,st(7)						; GI   BD   RI   LG   LB   VSTP RL   DW2
		frndint
		fxch	st(1)							; BD   GI   RI   LG   LB   VSTP RL   DW2
		fmul	st,st(7)						; BD   GI   RI   LG   LB   VSTP RL   DW2
		frndint
		fxch	st(5)							; VSTP GI   RI   LG   LB   BD   RL   DW2
		fistp	[VStep]							; GI   RI   LG   LB   BD   RL   DW2
		fld		[ecx]EdgeAsmFPU.z				; rz   GI   RI   LG   LB   BD   RL   DW2
		fsub	[ebx]EdgeAsmFPU.z				; zd   GI   RI   LG   LB   BD   RL   DW2
		fxch	st(7)							; DW2  GI   RI   LG   LB   BD   RL   zd
		fmulp	st(7),st						; GI   RI   LG   LB   BD   RL   zd
		fld		[ebx]EdgeAsmFPU.z				; lz   GI   RI   LG   LB   BD   RL   zd
		fxch	st(7)							; zd   GI   RI   LG   LB   BD   RL   lz
		fistp	[ZStep]							; GI   RI   LG   LB   BD   RL   lz
		fxch	st(6)							; lz   RI   LG   LB   BD   RL   GI
		fistp	[z16]							; RI   LG   LB   BD   RL   GI

		push	ebp

PixieLoop:
		mov		ebx,[ZStep]

		add		[z16],ebx
		mov		ecx,[VShift]

		mov		edi,[z16]

		shr		edi,16
		mov		ebx,[GHMaskShifted]

		mov		ebp,[GWMask]
		mov		edx,dword ptr[u16]

		mov		word ptr[Z32],di
		mov		eax,edx

		shr		edx,16
		mov		edi,[v16]

		add		eax,[UStep]
		mov		esi,edi

		shr		edi,cl
		add		esi,[VStep]

		and		edi,ebx
		and		edx,ebp

		add		edi,edx
		mov		edx,eax

		add		edi,GBitPtrHalf
		add		eax,[UStep]

		mov		[u16],eax
		mov		ax,word ptr[edi*2]

		mov		edi,esi
		add		esi,[VStep]

		shr		edx,16
		mov		[v16],esi

		shr		edi,cl
		and		edx,ebp

		mov		esi,GBitPtrHalf
		and		edi,ebx

		shl		eax,16
		add		esi,edx

		add		esi,edi

		mov		ax,word ptr[esi*2]

		mov		[TempPix],eax

		mov		ebx,eax
		and		eax,REDMASK2

		mov		ecx,ebx
		and		ebx,GREENMASK2

		mov		dword ptr[Red],eax
		and		ecx,BLUEMASK2

		mov		dword ptr[Green],ebx
		mov		dword ptr[Blue],ecx

		fild	qword ptr[Red]					; r    RI   LG   LB   BI   RL   GI
		fmul	st,st(5)						; R    RI   LG   LB   BI   RL   GI
		fild	qword ptr[Green]				; g    R    RI   LG   LB   BI   RL   GI
		fmul	st,st(3)						; G    R    RI   LG   LB   BI   RL   GI
		fxch	st(6)							; RL   R    RI   LG   LB   BI   G    GI
		fadd	st,st(2)						; RL2  R    RI   LG   LB   BI   G    GI
		fxch	st(3)							; LG   R    RI   RL2  LB   BI   G    GI
		fadd	st,st(7)						; LG2  R    RI   RL2  LB   BI   G    GI
		fxch	st(6)							; G    R    RI   RL2  LB   BI   LG2  GI
		fadd	qword ptr[Magic]				; Gk   R    RI   RL2  LB   BI   LG2  GI
		fxch	st(1)							; R    Gk   RI   RL2  LB   BI   LG2  GI
		fadd	qword ptr[Magic]				; Rk   Gk   RI   RL2  LB   BI   LG2  GI
		fxch	st(1)							; Gk   Rk   RI   RL2  LB   BI   LG2  GI
		fstp	qword ptr[Bucket]				; Rk   RI   RL2  LB   BI   LG2  GI
		fstp	qword ptr[Bucket2]				; RI   RL2  LB   BI   LG2  GI

		mov		edx,dword ptr[Bucket]
		mov		eax,dword ptr[Bucket2]

		fild	dword ptr[Blue]					; b    RI   RL2  LB   BI   LG2  GI
		fmul	st,st(3)						; B    RI   RL2  LB   BI   LG2  GI
		fxch	st(3)							; LB   RI   RL2  B    BI   LG2  GI
		fadd	st,st(4)						; LB2  RI   RL2  B    BI   LG2  GI
		fxch	st(3)							; B    RI   RL2  LB2  BI   LG2  GI
		fadd	qword ptr[Magic]				; Bk   RI   RL2  LB2  BI   LG2  GI

		and		edx,GREENMASK2
		and		eax,REDMASK2

		fstp	qword ptr[Bucket]				; RI   RL2  LB2  BI   LG2  GI
		fstp	qword ptr[Bucket2]				; RL2  LB2  BI   LG2  GI

		mov		edi,edx
		mov		ebx,dword ptr[Bucket]
		or		edi,eax
		mov		ebp,TDest
		and		ebx,BLUEMASK2

		fxch	st(3)							; LG2  LB2  BI   RL2  GI
		fld		qword ptr[Bucket2]				; RI   LG2  LB2  BI   RL2  GI

		mov		si,word ptr[Z32]
		add		TDest,4

		mov		ecx,pZBufferPtr
		or		edi,ebx

		cmp		[TempPix],010001h
		je		SkipPixie

		rol		edi,16

		cmp		word ptr[ecx],si
		jg		SkipPixie

		mov		dword ptr[ebp],edi
SkipPixie:
		add		pZBufferPtr,4
		dec		[widTemp]

		jnz		PixieLoop

		pop		ebp

		fstp	[u16]
		fstp	[v16]
		fstp	[u16]
		fstp	[v16]
		fstp	[u16]
		fstp	[v16]

GouraudReturnNoZ:
		pop		edi
		pop		esi
		pop		ecx
		pop		ebx
	}
}

void	DrawScanLineGouraudNoZBufferZWriteTrans_Asm555X86FPU(EdgeAsmFPU *pLeft, EdgeAsmFPU *pRight)
{
	TDest	=Dest;
	Red		=Green	=0;
	_asm
	{
		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		ebx,pLeft
		mov     ecx,pRight
		mov     eax,[ebx]EdgeAsmFPU.X
		mov     edx,[ecx]EdgeAsmFPU.X
		sub     edx,eax
		jle		GouraudReturnNoZ

		mov		esi,eax
		inc		edx
		shl		eax,1
		add		TDest,eax
		add		pZBufferPtr,eax
		test	esi,1		//dword align left
		jz		NoSinglePixie

		//odd sized spans write one pixel to dword align
		fld     dword ptr [ebx]EdgeAsmFPU.u		; UL
		fmul    [Real65536]						; UL16
		fld     dword ptr [ebx]EdgeAsmFPU.v		; VL   UL16
		fmul	[Real65536]						; VL16 UL16
		fild    dword ptr [ebx]EdgeAsmFPU.R		; RL   VL16 UL16
		fild    dword ptr [ebx]EdgeAsmFPU.G		; GL   RL   VL16 UL16
		fild    dword ptr [ebx]EdgeAsmFPU.B		; BL   GL   RL   VL16 UL16
		fxch	st(4)							; UL16 GL   RL   VL16 BL
		fistp	[u16]							; GL   RL   VL16 BL
		fxch	st(2)							; VL16 RL   GL   BL
		fistp	[v16]							; RL   GL   BL
		fld		dword ptr[ebx]EdgeAsmFPU.z		; z    RL   GL   BL
		fistp	[z16]							; RL   GL   BL

		mov     [widTemp],edx

		mov		ecx,[VShift]
		mov		ebx,[GHMaskShifted]

		mov		esi,[GWMask]
		mov		edi,[v16]

		mov		edx,dword ptr[u16]

		shr		edi,cl

		shr		edx,16
		xor		eax,eax

		and		edi,ebx
		and		edx,esi

		add		edi,edx

		add		edi,GBitPtrHalf
		mov		ax,word ptr[edi*2]

		mov		word ptr[TempPix],ax

		mov		ebx,eax
		and		eax,REDMASK2

		mov		ecx,ebx
		and		ebx,GREENMASK2

		mov		dword ptr[Red],eax
		and		ecx,BLUEMASK2

		mov		dword ptr[Green],ebx
		mov		dword ptr[Blue],ecx

		fimul	dword ptr[Red]					; R    GL   BL
		fxch	st(1)							; GL   R    BL
		fimul	dword ptr[Green]				; G    R    BL
		fxch	st(1)							; R    G    BL
		fadd	qword ptr[Magic]				; Rk   G    BL
		fxch	st(2)							; BL   G    Rk
		fimul	[Blue]							; B    G    Rk
		fxch	st(1)							; G    B    Rk
		fadd	qword ptr[Magic]				; Gk   B    Rk
		fxch	st(2)							; Rk   B    Gk
		fstp	qword ptr[Bucket]				; B    Gk
		fadd	qword ptr[Magic]				; Bk   Gk
		fxch	st(1)							; Gk   Bk
		fstp	qword ptr[Bucket2]				; Bk

		mov		edx,dword ptr[Bucket]
		mov		eax,dword ptr[Bucket2]

		and		edx,REDMASK2
		and		eax,GREENMASK2

		fstp	qword ptr[Bucket]

		mov		ecx,TDest
		mov		ebx,dword ptr[Bucket]

		mov		esi,[z16]
		and		ebx,BLUEMASK2

		shr		esi,16
		or		edx,eax

		mov		eax,pZBufferPtr
		add		TDest,2

		cmp		word ptr[TempPix],01h
		je		SkipSinglePixie

		or		edx,ebx
		mov		word ptr[eax],si

		mov		word ptr[ecx],dx

SkipSinglePixie:
		mov		ebx,pLeft
		mov     edx,[widTemp]
		add		pZBufferPtr,2
		mov     ecx,pRight
		dec		edx
		jz		GouraudReturnNoZ
		mov		esi,edx
		and		esi,1
		sub		edx,esi
		jz		GouraudReturnNoZ

NoSinglePixie:
		mov     [widTemp],edx                 ; just for a temp
		shr		edx,1

		; try to keep fmul fxch pairs seperated to avoid stalling
		; calc this scanlines steps           ; FPU Stack
		                                      ; st0  st1  st2  st3  st4  st5  st6  st7
		fild    dword ptr [widTemp]           ; WID

		mov     [widTemp],edx                 ; Color interps doubled

		fld     dword ptr [ecx]EdgeAsmFPU.u ; UR   WID
		fld     dword ptr [ebx]EdgeAsmFPU.u ; UL   UR   WID
		fsub    st(1), st                   ; UL   UD   WID
		fld     dword ptr [ecx]EdgeAsmFPU.v ; VR   UL   UD   WID
		fxch    st(1)                       ; UL   VR   UD   WID
		fmul    [Real65536]                 ; UL16 VR   UD   WID
		fld     dword ptr [ebx]EdgeAsmFPU.v ; VL   UL16 VR   UD   WID
		fsub    st(2), st                   ; VL   UL16 VD   UD   WID
		fild    dword ptr [ecx]EdgeAsmFPU.R ; RR   VL   UL16 VD   UD   WID
		fxch    st(3)                       ; VD   VL   UL16 RR   UD   WID
		fmul    [Real65536]                 ; VD16 VL   UL16 RR   UD   WID
		fild    dword ptr [ebx]EdgeAsmFPU.R ; RL   VD16 VL   UL16 RR   UD   WID
		fsub    st(4),st                    ; RL   VD16 VL   UL16 RD   UD   WID
		fxch    st(5)                       ; UD   VD16 VL   UL16 RD   RL   WID
		fmul    [Real65536]                 ; UD16 VD16 VL   UL16 RD   RL   WID
		fxch    st(2)						; VL   VD16 UD16 UL16 RD   RL   WID
		fmul    [Real65536]                 ; VL16 VD16 UD16 UL16 RD   RL   WID
		fxch	st(3)						; UL16 VD16 UD16 VL16 RD   RL   WID
		fistp	[u16]						; VD16 UD16 VL16 RD   RL   WID
		fxch	st(2)						; VL16 UD16 VD16 RD   RL   WID
		fistp	[v16]						; UD16 VD16 RD   RL   WID
		fld1								; 1    UD16 VD16 RD   RL   WID
		fdivrp	st(5),st					; UD16 VD16 RD   RL   DWID

		//let that cook

		fmul	st,st(4)						; USTP VD16 RD   RL   DWID
		fild	[ecx]EdgeAsmFPU.G				; RG   USTP VD16 RD   RL   DWID
		fild	[ebx]EdgeAsmFPU.G				; LG   RG   USTP VD16 RD   RL   DWID
		fsub	st(1),st						; LG   GD   USTP VD16 RD   RL   DWID
		fxch	st(2)							; USTP GD   LG   VD16 RD   RL   DWID
		fistp	[UStep]							; GD   LG   VD16 RD   RL   DWID
		fild	[ecx]EdgeAsmFPU.B				; RB   GD   LG   VD16 RD   RL   DWID
		fild	[ebx]EdgeAsmFPU.B				; LB   RB   GD   LG   VD16 RD   RL   DWID
		fsub	st(1),st						; LB   BD   GD   LG   VD16 RD   RL   DWID
		fxch	st(4)							; VD16 BD   GD   LG   LB   RD   RL   DWID
		fmul	st,st(7)						; VSTP BD   GD   LG   LB   RD   RL   DWID
		fxch	st(7)							; DWID BD   GD   LG   LB   RD   RL   VSTP
		fmul	dword ptr[Two]					; DW2  BD   GD   LG   LB   RD   RL   VSTP
		fxch	st(7)							; VSTP BD   GD   LG   LB   RD   RL   DW2
		fxch	st(5)							; RD   BD   GD   LG   LB   VSTP RL   DW2
		fmul	st,st(7)						; RI   BD   GD   LG   LB   VSTP RL   DW2
		frndint
		fxch	st(2)							; GD   BD   RI   LG   LB   VSTP RL   DW2
		fmul	st,st(7)						; GI   BD   RI   LG   LB   VSTP RL   DW2
		frndint
		fxch	st(1)							; BD   GI   RI   LG   LB   VSTP RL   DW2
		fmul	st,st(7)						; BD   GI   RI   LG   LB   VSTP RL   DW2
		frndint
		fxch	st(5)							; VSTP GI   RI   LG   LB   BD   RL   DW2
		fistp	[VStep]							; GI   RI   LG   LB   BD   RL   DW2
		fld		[ecx]EdgeAsmFPU.z				; rz   GI   RI   LG   LB   BD   RL   DW2
		fsub	[ebx]EdgeAsmFPU.z				; zd   GI   RI   LG   LB   BD   RL   DW2
		fxch	st(7)							; DW2  GI   RI   LG   LB   BD   RL   zd
		fmulp	st(7),st						; GI   RI   LG   LB   BD   RL   zd
		fld		[ebx]EdgeAsmFPU.z				; lz   GI   RI   LG   LB   BD   RL   zd
		fxch	st(7)							; zd   GI   RI   LG   LB   BD   RL   lz
		fistp	[ZStep]							; GI   RI   LG   LB   BD   RL   lz
		fxch	st(6)							; lz   RI   LG   LB   BD   RL   GI
		fistp	[z16]							; RI   LG   LB   BD   RL   GI

		push	ebp

PixieLoop:
		mov		ebx,[ZStep]

		add		[z16],ebx
		mov		ecx,[VShift]

		mov		edi,[z16]

		shr		edi,16
		mov		ebx,[GHMaskShifted]

		mov		ebp,[GWMask]
		mov		edx,dword ptr[u16]

		mov		word ptr[Z32],di
		mov		eax,edx

		shr		edx,16
		mov		edi,[v16]

		add		eax,[UStep]
		mov		esi,edi

		shr		edi,cl
		add		esi,[VStep]

		and		edi,ebx
		and		edx,ebp

		add		edi,edx
		mov		edx,eax

		add		edi,GBitPtrHalf
		add		eax,[UStep]

		mov		[u16],eax
		mov		ax,word ptr[edi*2]

		mov		edi,esi
		add		esi,[VStep]

		shr		edx,16
		mov		[v16],esi

		shr		edi,cl
		and		edx,ebp

		mov		esi,GBitPtrHalf
		and		edi,ebx

		shl		eax,16
		add		esi,edx

		add		esi,edi

		mov		ax,word ptr[esi*2]

		mov		[TempPix],eax

		mov		ebx,eax
		and		eax,REDMASK2

		mov		ecx,ebx
		and		ebx,GREENMASK2

		mov		dword ptr[Red],eax
		and		ecx,BLUEMASK2

		mov		dword ptr[Green],ebx
		mov		dword ptr[Blue],ecx

		fild	qword ptr[Red]					; r    RI   LG   LB   BI   RL   GI
		fmul	st,st(5)						; R    RI   LG   LB   BI   RL   GI
		fild	qword ptr[Green]				; g    R    RI   LG   LB   BI   RL   GI
		fmul	st,st(3)						; G    R    RI   LG   LB   BI   RL   GI
		fxch	st(6)							; RL   R    RI   LG   LB   BI   G    GI
		fadd	st,st(2)						; RL2  R    RI   LG   LB   BI   G    GI
		fxch	st(3)							; LG   R    RI   RL2  LB   BI   G    GI
		fadd	st,st(7)						; LG2  R    RI   RL2  LB   BI   G    GI
		fxch	st(6)							; G    R    RI   RL2  LB   BI   LG2  GI
		fadd	qword ptr[Magic]				; Gk   R    RI   RL2  LB   BI   LG2  GI
		fxch	st(1)							; R    Gk   RI   RL2  LB   BI   LG2  GI
		fadd	qword ptr[Magic]				; Rk   Gk   RI   RL2  LB   BI   LG2  GI
		fxch	st(1)							; Gk   Rk   RI   RL2  LB   BI   LG2  GI
		fstp	qword ptr[Bucket]				; Rk   RI   RL2  LB   BI   LG2  GI
		fstp	qword ptr[Bucket2]				; RI   RL2  LB   BI   LG2  GI

		mov		edx,dword ptr[Bucket]
		mov		eax,dword ptr[Bucket2]

		fild	dword ptr[Blue]					; b    RI   RL2  LB   BI   LG2  GI
		fmul	st,st(3)						; B    RI   RL2  LB   BI   LG2  GI
		fxch	st(3)							; LB   RI   RL2  B    BI   LG2  GI
		fadd	st,st(4)						; LB2  RI   RL2  B    BI   LG2  GI
		fxch	st(3)							; B    RI   RL2  LB2  BI   LG2  GI
		fadd	qword ptr[Magic]				; Bk   RI   RL2  LB2  BI   LG2  GI

		and		edx,GREENMASK2
		and		eax,REDMASK2

		fstp	qword ptr[Bucket]				; RI   RL2  LB2  BI   LG2  GI
		fstp	qword ptr[Bucket2]				; RL2  LB2  BI   LG2  GI

		mov		edi,edx
		mov		ebx,dword ptr[Bucket]
		or		edi,eax
		mov		ebp,TDest
		and		ebx,BLUEMASK2

		fxch	st(3)							; LG2  LB2  BI   RL2  GI
		fld		qword ptr[Bucket2]				; RI   LG2  LB2  BI   RL2  GI

		mov		si,word ptr[Z32]
		add		TDest,4

		mov		ecx,pZBufferPtr
		or		edi,ebx

		cmp		[TempPix],010001h
		je		SkipPixie

		mov		[ecx],si
		rol		edi,16

		mov		dword ptr[ebp],edi
		mov		[ecx+2],si
SkipPixie:
		add		pZBufferPtr,4
		dec		[widTemp]

		jnz		PixieLoop

		pop		ebp

		fstp	[u16]
		fstp	[v16]
		fstp	[u16]
		fstp	[v16]
		fstp	[u16]
		fstp	[v16]

GouraudReturnNoZ:
		pop		edi
		pop		esi
		pop		ecx
		pop		ebx
	}
}

void	DrawScanLineGouraudNoZSolid_Asm555X86FPU(EdgeAsmFPU *pLeft, EdgeAsmFPU *pRight)
{
	TDest	=Dest;
	Red		=Green	=0;
	_asm
	{
		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		ebx,pLeft
		mov     ecx,pRight
		mov     eax,[ebx]EdgeAsmFPU.X
		mov     edx,[ecx]EdgeAsmFPU.X
		sub     edx,eax
		jle		GouraudReturnNoZ

		mov		esi,eax
		inc		edx
		shl		eax,1
		add		TDest,eax
		test	esi,1					//dword align left side
		jz		NoSinglePixie

		//odd sized spans write one pixel to dword align
		fild	dword ptr [ebx]EdgeAsmFPU.B		; BL
		fmul	dword ptr[BlueMask]
		fadd	qword ptr[Magic]				; Bk
		fild	dword ptr [ebx]EdgeAsmFPU.G		; GL   Bk
		fmul	dword ptr[GreenMask2]
		fadd	qword ptr[Magic]				; Gk   Bk
		fild	dword ptr [ebx]EdgeAsmFPU.R		; RL   Gk   Bk
		fmul	dword ptr[MiniRedMask2]
		fadd	qword ptr[Magic]				; Rk   Gk   Bk
		fxch	st(2)							; Bk   Gk   Rk
		fstp	qword ptr[Bucket]				; Gk   Rk
		fstp	qword ptr[Bucket2]				; Rk

		mov		esi,dword ptr[Bucket]
		mov		eax,dword ptr[Bucket2]

		and		esi,BLUEMASK2
		and		eax,GREENMASK2

		fstp	qword ptr[Bucket]

		mov		ecx,TDest
		mov		ebx,dword ptr[Bucket]

		and		ebx,REDMASK2
		or		esi,eax

		add		TDest,2
		or		esi,ebx

		mov		ebx,pLeft
		mov		word ptr[ecx],si

		mov		ecx,pRight
		dec		edx

		jz		GouraudReturnNoZ
		mov		esi,edx
		and		esi,1
		sub		edx,esi
		jz		GouraudReturnNoZ

NoSinglePixie:
		shr		edx,1
		fld1
		mov     [widTemp],edx                 ; just for a temp
		

		; try to keep fmul fxch pairs seperated to avoid stalling
		; calc this scanlines steps				; FPU Stack
												; st0  st1  st2  st3  st4  st5  st6  st7
		fidiv   dword ptr [widTemp]				; WID
		fild    dword ptr [ecx]EdgeAsmFPU.R		; RR   WID
		fild    dword ptr [ebx]EdgeAsmFPU.R		; RL   RR   WID
		fsub	st(1),st						; RL   RD   WID
		fild	[ecx]EdgeAsmFPU.G				; GR   RL   RD   WID
		fild	[ebx]EdgeAsmFPU.G				; GL   GR   RL   RD   WID
		fsub	st(1),st						; GL   GD   RL   RD   WID
		fild	[ecx]EdgeAsmFPU.B				; BR   GL   GD   RL   RD   WID
		fild	[ebx]EdgeAsmFPU.B				; BL   BR   GL   GD   RL   RD   WID
		fsub	st(1),st						; BL   BD   GL   GD   RL   RD   WID
		fxch	st(5)							; RD   BD   GL   GD   RL   BL   WID
		fmul	st,st(6)						; RI   BD   GL   GD   RL   BL   WID
		frndint
		fxch	st(3)							; GD   BD   GL   RI   RL   BL   WID
		fmul	st,st(6)						; GI   BD   GL   RI   RL   BL   WID
		frndint
		fxch	st(6)							; WID  BD   GL   RI   RL   BL   GI
		fmulp	st(1),st						; BI   GL   RI   RL   BL   GI
		frndint
		fld		qword ptr[RedMask2]				; rm   BI   GL   RI   RL   BL   GI
		fmul	st(3),st						; rm   BI   GL   RI   RL   BL   GI
		fmulp	st(4),st						; BI   GL   RI   RL   BL   GI
		fld		dword ptr[GreenMask2]			; gm   BI   GL   RI   RL   BL   GI
		fmul	st(2),st						; gm   BI   GL   RI   RL   BL   GI
		fmulp	st(6),st						; BI   GL   RI   RL   BL   GI
		fld		dword ptr[BlueMask]				; bm   BI   GL   RI   RL   BL   GI
		fmul	st(1),st						; bm   BI   GL   RI   RL   BL   GI
		fmulp	st(5),st						; BI   GL   RI   RL   BL   GI

PixieLoop:

		fld		st(3)							; r    BI   GL   RI   RL   BL   GI
		fadd	qword ptr[Magic]				; rk   BI   GL   RI   RL   BL   GI
		fld		st(2)							; g    rk   BI   GL   RI   RL   BL   GI
		fadd	qword ptr[Magic]				; gk   rk   BI   GL   RI   RL   BL   GI
		fxch	st(1)							; rk   gk   BI   GL   RI   RL   BL   GI
		fstp	qword ptr[Bucket]				; gk   BI   GL   RI   RL   BL   GI
		fstp	qword ptr[Bucket2]				; BI   GL   RI   RL   BL   GI
		fld		st(4)							; b    BI   GL   RI   RL   BL   GI
		fadd	qword ptr[Magic]				; bk   BI   GL   RI   RL   BL   GI

		mov		ecx,dword ptr[Bucket]
		mov		eax,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]				; BI   GL   RI   RL   BL   GI
		fadd	st(4),st						; BI   GL   RI   RL   BL2  GI

		and		ecx,REDMASK2
		and		eax,GREENMASK2

		mov		ebx,dword ptr[Bucket]
		or		ecx,eax

		fstp	qword ptr[Bucket]				; GL   RI   RL   BL2  GI
		fadd	st,st(4)						; GL2  RI   RL   BL2  GI
		fstp	qword ptr[Bucket2]				; RI   RL   BL2  GI
		fadd	st(1),st						; RI   RL2  BL2  GI
		fld		qword ptr[Bucket2]				; GL2  RI   RL2  BL2  GI
		fld		qword ptr[Bucket]				; BI   GL2  RI   RL2  BL2  GI

		and		ebx,BLUEMASK2
		mov		edi,TDest

		or		ecx,ebx
		add		TDest,4

		rol		ecx,16

		mov		[edi],ecx
		dec		edx

		jnz		PixieLoop


		fstp	[u16]
		fstp	[v16]
		fstp	[u16]
		fstp	[v16]
		fstp	[u16]
		fstp	[v16]

GouraudReturnNoZ:
		pop		edi
		pop		esi
		pop		ecx
		pop		ebx
	}
}

void	DrawScanLineGouraudNoZBufferZWriteSolid_Asm555X86FPU(EdgeAsmFPU *pLeft, EdgeAsmFPU *pRight)
{
	TDest	=Dest;
	Red		=Green	=0;
	_asm
	{
		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		ebx,pLeft
		mov     ecx,pRight
		mov     eax,[ebx]EdgeAsmFPU.X
		mov     edx,[ecx]EdgeAsmFPU.X
		sub     edx,eax
		jle		GouraudReturnNoZ

		mov		esi,eax
		inc		edx
		shl		eax,1
		add		TDest,eax
		add		pZBufferPtr,eax
		test	esi,1					//dword align left side
		jz		NoSinglePixie

		//odd sized spans write one pixel to dword align
		fild	dword ptr [ebx]EdgeAsmFPU.B		; BL
		fmul	dword ptr[BlueMask]
		fadd	qword ptr[Magic]				; Bk
		fild	dword ptr [ebx]EdgeAsmFPU.G		; GL   Bk
		fmul	dword ptr[GreenMask2]
		fadd	qword ptr[Magic]				; Gk   Bk
		fild	dword ptr [ebx]EdgeAsmFPU.R		; RL   Gk   Bk
		fmul	dword ptr[MiniRedMask2]
		fadd	qword ptr[Magic]				; Rk   Gk   Bk
		fxch	st(2)							; Bk   Gk   Rk
		fstp	qword ptr[Bucket]				; Gk   Rk
		fstp	qword ptr[Bucket2]				; Rk
		fld		dword ptr[ebx]EdgeAsmFPU.z		; z
		fistp	[z16]							;

		mov		esi,dword ptr[Bucket]
		mov		eax,dword ptr[Bucket2]

		and		esi,BLUEMASK2
		and		eax,GREENMASK2

		fstp	qword ptr[Bucket]

		mov		ecx,TDest
		mov		ebx,dword ptr[Bucket]

		and		ebx,REDMASK2
		or		esi,eax

		mov		edi,[z16]
		mov		eax,pZBufferPtr

		add		TDest,2
		or		esi,ebx

		shr		edi,16
		mov		ebx,pLeft

		mov		word ptr[eax],di
		mov		word ptr[ecx],si

		mov		ecx,pRight
		dec		edx

		jz		GouraudReturnNoZ
		mov		esi,edx
		add		pZBufferPtr,2
		and		esi,1
		sub		edx,esi
		jz		GouraudReturnNoZ

NoSinglePixie:
		shr		edx,1
		fld1
		mov     [widTemp],edx                 ; just for a temp
		

		; try to keep fmul fxch pairs seperated to avoid stalling
		; calc this scanlines steps				; FPU Stack
												; st0  st1  st2  st3  st4  st5  st6  st7
		fidiv   dword ptr [widTemp]				; WID
		fild    dword ptr [ecx]EdgeAsmFPU.R		; RR   WID
		fild    dword ptr [ebx]EdgeAsmFPU.R		; RL   RR   WID
		fsub	st(1),st						; RL   RD   WID
		fild	[ecx]EdgeAsmFPU.G				; GR   RL   RD   WID
		fild	[ebx]EdgeAsmFPU.G				; GL   GR   RL   RD   WID
		fsub	st(1),st						; GL   GD   RL   RD   WID
		fild	[ecx]EdgeAsmFPU.B				; BR   GL   GD   RL   RD   WID
		fild	[ebx]EdgeAsmFPU.B				; BL   BR   GL   GD   RL   RD   WID
		fsub	st(1),st						; BL   BD   GL   GD   RL   RD   WID
		fxch	st(5)							; RD   BD   GL   GD   RL   BL   WID
		fmul	st,st(6)						; RI   BD   GL   GD   RL   BL   WID
		frndint
		fxch	st(3)							; GD   BD   GL   RI   RL   BL   WID
		fmul	st,st(6)						; GI   BD   GL   RI   RL   BL   WID
		frndint
		fxch	st(6)							; WID  BD   GL   RI   RL   BL   GI
		fmulp	st(1),st						; BI   GL   RI   RL   BL   GI
		frndint
		fld		qword ptr[RedMask2]				; rm   BI   GL   RI   RL   BL   GI
		fmul	st(3),st						; rm   BI   GL   RI   RL   BL   GI
		fmulp	st(4),st						; BI   GL   RI   RL   BL   GI
		fld		dword ptr[GreenMask2]			; gm   BI   GL   RI   RL   BL   GI
		fmul	st(2),st						; gm   BI   GL   RI   RL   BL   GI
		fmulp	st(6),st						; BI   GL   RI   RL   BL   GI
		fld		dword ptr[BlueMask]				; bm   BI   GL   RI   RL   BL   GI
		fmul	st(1),st						; bm   BI   GL   RI   RL   BL   GI
		fmulp	st(5),st						; BI   GL   RI   RL   BL   GI
		fld		[ecx]EdgeAsmFPU.z				; rz   BI   GL   RI   RL   BL   GI
		fsub	[ebx]EdgeAsmFPU.z				; zd   BI   GL   RI   RL   BL   GI
		fld		[ebx]EdgeAsmFPU.z				; lz   zd   BI   GL   RI   RL   BL   GI
		fistp	[z16]							; zd   BI   GL   RI   RL   BL   GI
		fistp	[ZStep]							; BI   GL   RI   RL   BL   GI

PixieLoop:

		fld		st(3)							; r    BI   GL   RI   RL   BL   GI
		fadd	qword ptr[Magic]				; rk   BI   GL   RI   RL   BL   GI
		fld		st(2)							; g    rk   BI   GL   RI   RL   BL   GI
		fadd	qword ptr[Magic]				; gk   rk   BI   GL   RI   RL   BL   GI
		fxch	st(1)							; rk   gk   BI   GL   RI   RL   BL   GI
		fstp	qword ptr[Bucket]				; gk   BI   GL   RI   RL   BL   GI

		mov		edi,[z16]
		mov		esi,pZBufferPtr

		shr		edi,16
		mov		ebx,[ZStep]

		mov		word ptr[esi],di
		add		[z16],ebx

		fstp	qword ptr[Bucket2]				; BI   GL   RI   RL   BL   GI
		fld		st(4)							; b    BI   GL   RI   RL   BL   GI
		fadd	qword ptr[Magic]				; bk   BI   GL   RI   RL   BL   GI

		mov		word ptr[esi+2],di

		mov		ecx,dword ptr[Bucket]
		mov		eax,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]				; BI   GL   RI   RL   BL   GI
		fadd	st(4),st						; BI   GL   RI   RL   BL2  GI

		and		ecx,REDMASK2
		and		eax,GREENMASK2

		mov		ebx,dword ptr[Bucket]
		or		ecx,eax

		fstp	qword ptr[Bucket]				; GL   RI   RL   BL2  GI
		fadd	st,st(4)						; GL2  RI   RL   BL2  GI
		fstp	qword ptr[Bucket2]				; RI   RL   BL2  GI
		fadd	st(1),st						; RI   RL2  BL2  GI
		fld		qword ptr[Bucket2]				; GL2  RI   RL2  BL2  GI
		fld		qword ptr[Bucket]				; BI   GL2  RI   RL2  BL2  GI

		and		ebx,BLUEMASK2
		mov		edi,TDest

		or		ecx,ebx
		add		TDest,4

		rol		ecx,16

		mov		[edi],ecx

		add		pZBufferPtr,4

		dec		edx

		jnz		PixieLoop


		fstp	[u16]
		fstp	[v16]
		fstp	[u16]
		fstp	[v16]
		fstp	[u16]
		fstp	[v16]

GouraudReturnNoZ:
		pop		edi
		pop		esi
		pop		ecx
		pop		ebx
	}
}

void	DrawScanLineGouraudZBufferSolid_Asm555X86FPU(EdgeAsmFPU *pLeft, EdgeAsmFPU *pRight)
{
	TDest	=Dest;
	Red		=Green	=0;
	_asm
	{
		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		ebx,pLeft
		mov     ecx,pRight
		mov     eax,[ebx]EdgeAsmFPU.X
		mov     edx,[ecx]EdgeAsmFPU.X
		sub     edx,eax
		jle		GouraudReturnNoZ

		mov		esi,eax
		inc		edx
		shl		eax,1
		add		TDest,eax
		add		pZBufferPtr,eax
		test	esi,1					//dword align left side
		jz		NoSinglePixie

		//odd sized spans write one pixel to dword align
		fild	dword ptr [ebx]EdgeAsmFPU.B		; BL
		fmul	dword ptr[BlueMask]
		fadd	qword ptr[Magic]				; Bk
		fild	dword ptr [ebx]EdgeAsmFPU.G		; GL   Bk
		fmul	dword ptr[GreenMask2]
		fadd	qword ptr[Magic]				; Gk   Bk
		fild	dword ptr [ebx]EdgeAsmFPU.R		; RL   Gk   Bk
		fmul	dword ptr[MiniRedMask2]
		fadd	qword ptr[Magic]				; Rk   Gk   Bk
		fxch	st(2)							; Bk   Gk   Rk
		fstp	qword ptr[Bucket]				; Gk   Rk
		fstp	qword ptr[Bucket2]				; Rk
		fld		dword ptr[ebx]EdgeAsmFPU.z		; z
		fistp	[z16]							;

		mov		esi,dword ptr[Bucket]
		mov		eax,dword ptr[Bucket2]

		and		esi,BLUEMASK2
		and		eax,GREENMASK2

		fstp	qword ptr[Bucket]

		mov		ecx,TDest
		mov		ebx,dword ptr[Bucket]

		and		ebx,REDMASK2
		or		esi,eax

		mov		edi,[z16]
		mov		eax,pZBufferPtr

		add		TDest,2
		or		esi,ebx

		shr		edi,16
		mov		ebx,pLeft

		cmp		word ptr[eax],di
		jg		SkipSinglePixie

		mov		word ptr[eax],di
		mov		word ptr[ecx],si

SkipSinglePixie:
		mov		ecx,pRight
		add		pZBufferPtr,2
		dec		edx

		jz		GouraudReturnNoZ
		mov		esi,edx
		and		esi,1
		sub		edx,esi
		jz		GouraudReturnNoZ

NoSinglePixie:
		shr		edx,1
		fld1
		mov     [widTemp],edx                 ; just for a temp
		

		; try to keep fmul fxch pairs seperated to avoid stalling
		; calc this scanlines steps				; FPU Stack
												; st0  st1  st2  st3  st4  st5  st6  st7
		fidiv   dword ptr [widTemp]				; WID
		fild    dword ptr [ecx]EdgeAsmFPU.R		; RR   WID
		fild    dword ptr [ebx]EdgeAsmFPU.R		; RL   RR   WID
		fsub	st(1),st						; RL   RD   WID
		fild	[ecx]EdgeAsmFPU.G				; GR   RL   RD   WID
		fild	[ebx]EdgeAsmFPU.G				; GL   GR   RL   RD   WID
		fsub	st(1),st						; GL   GD   RL   RD   WID
		fild	[ecx]EdgeAsmFPU.B				; BR   GL   GD   RL   RD   WID
		fild	[ebx]EdgeAsmFPU.B				; BL   BR   GL   GD   RL   RD   WID
		fsub	st(1),st						; BL   BD   GL   GD   RL   RD   WID
		fxch	st(5)							; RD   BD   GL   GD   RL   BL   WID
		fmul	st,st(6)						; RI   BD   GL   GD   RL   BL   WID
		frndint
		fxch	st(3)							; GD   BD   GL   RI   RL   BL   WID
		fmul	st,st(6)						; GI   BD   GL   RI   RL   BL   WID
		frndint
		fxch	st(6)							; WID  BD   GL   RI   RL   BL   GI
		fmulp	st(1),st						; BI   GL   RI   RL   BL   GI
		frndint
		fld		qword ptr[RedMask2]				; rm   BI   GL   RI   RL   BL   GI
		fmul	st(3),st						; rm   BI   GL   RI   RL   BL   GI
		fmulp	st(4),st						; BI   GL   RI   RL   BL   GI
		fld		dword ptr[GreenMask2]			; gm   BI   GL   RI   RL   BL   GI
		fmul	st(2),st						; gm   BI   GL   RI   RL   BL   GI
		fmulp	st(6),st						; BI   GL   RI   RL   BL   GI
		fld		dword ptr[BlueMask]				; bm   BI   GL   RI   RL   BL   GI
		fmul	st(1),st						; bm   BI   GL   RI   RL   BL   GI
		fmulp	st(5),st						; BI   GL   RI   RL   BL   GI
		fld		[ecx]EdgeAsmFPU.z				; rz   BI   GL   RI   RL   BL   GI
		fsub	[ebx]EdgeAsmFPU.z				; zd   BI   GL   RI   RL   BL   GI
		fld		[ebx]EdgeAsmFPU.z				; lz   zd   BI   GL   RI   RL   BL   GI
		fistp	[z16]							; zd   BI   GL   RI   RL   BL   GI
		fistp	[ZStep]							; BI   GL   RI   RL   BL   GI

PixieLoop:

		fld		st(3)							; r    BI   GL   RI   RL   BL   GI
		fadd	qword ptr[Magic]				; rk   BI   GL   RI   RL   BL   GI
		fld		st(2)							; g    rk   BI   GL   RI   RL   BL   GI
		fadd	qword ptr[Magic]				; gk   rk   BI   GL   RI   RL   BL   GI
		fxch	st(1)							; rk   gk   BI   GL   RI   RL   BL   GI
		fstp	qword ptr[Bucket]				; gk   BI   GL   RI   RL   BL   GI
		fstp	qword ptr[Bucket2]				; BI   GL   RI   RL   BL   GI
		fld		st(4)							; b    BI   GL   RI   RL   BL   GI
		fadd	qword ptr[Magic]				; bk   BI   GL   RI   RL   BL   GI

		mov		ecx,dword ptr[Bucket]
		mov		eax,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]				; BI   GL   RI   RL   BL   GI
		fadd	st(4),st						; BI   GL   RI   RL   BL2  GI

		and		ecx,REDMASK2
		and		eax,GREENMASK2

		mov		ebx,dword ptr[Bucket]
		or		ecx,eax

		fstp	qword ptr[Bucket]				; GL   RI   RL   BL2  GI
		fadd	st,st(4)						; GL2  RI   RL   BL2  GI
		fstp	qword ptr[Bucket2]				; RI   RL   BL2  GI
		fadd	st(1),st						; RI   RL2  BL2  GI
		fld		qword ptr[Bucket2]				; GL2  RI   RL2  BL2  GI
		fld		qword ptr[Bucket]				; BI   GL2  RI   RL2  BL2  GI

		and		ebx,BLUEMASK2
		mov		edi,TDest

		or		ecx,ebx
		add		TDest,4

		mov		eax,[z16]
		mov		esi,pZBufferPtr

		shr		eax,16
		mov		ebx,[ZStep]

		cmp		word ptr[esi],ax
		jg		SkipPixie

		mov		word ptr[esi],ax
		rol		ecx,16

		add		[z16],ebx
		mov		word ptr[esi+2],ax

		mov		[edi],ecx
SkipPixie:
		add		pZBufferPtr,4
		dec		edx

		jnz		PixieLoop


		fstp	[u16]
		fstp	[v16]
		fstp	[u16]
		fstp	[v16]
		fstp	[u16]
		fstp	[v16]

GouraudReturnNoZ:
		pop		edi
		pop		esi
		pop		ecx
		pop		ebx
	}
}

void	DrawScanLineGouraudZBufferNoZWriteSolid_Asm555X86FPU(EdgeAsmFPU *pLeft, EdgeAsmFPU *pRight)
{
	TDest	=Dest;
	Red		=Green	=0;
	_asm
	{
		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		ebx,pLeft
		mov     ecx,pRight
		mov     eax,[ebx]EdgeAsmFPU.X
		mov     edx,[ecx]EdgeAsmFPU.X
		sub     edx,eax
		jle		GouraudReturnNoZ

		mov		esi,eax
		inc		edx
		shl		eax,1
		add		TDest,eax
		add		pZBufferPtr,eax
		test	esi,1					//dword align left side
		jz		NoSinglePixie

		//odd sized spans write one pixel to dword align
		fild	dword ptr [ebx]EdgeAsmFPU.B		; BL
		fmul	dword ptr[BlueMask]
		fadd	qword ptr[Magic]				; Bk
		fild	dword ptr [ebx]EdgeAsmFPU.G		; GL   Bk
		fmul	dword ptr[GreenMask2]
		fadd	qword ptr[Magic]				; Gk   Bk
		fild	dword ptr [ebx]EdgeAsmFPU.R		; RL   Gk   Bk
		fmul	dword ptr[MiniRedMask2]
		fadd	qword ptr[Magic]				; Rk   Gk   Bk
		fxch	st(2)							; Bk   Gk   Rk
		fstp	qword ptr[Bucket]				; Gk   Rk
		fstp	qword ptr[Bucket2]				; Rk
		fld		dword ptr[ebx]EdgeAsmFPU.z		; z
		fistp	[z16]							;

		mov		esi,dword ptr[Bucket]
		mov		eax,dword ptr[Bucket2]

		and		esi,BLUEMASK2
		and		eax,GREENMASK2

		fstp	qword ptr[Bucket]

		mov		ecx,TDest
		mov		ebx,dword ptr[Bucket]

		and		ebx,REDMASK2
		or		esi,eax

		mov		edi,[z16]
		mov		eax,pZBufferPtr

		add		TDest,2
		or		esi,ebx

		shr		edi,16
		mov		ebx,pLeft

		cmp		word ptr[eax],di
		jg		SkipSinglePixie

		mov		word ptr[ecx],si

SkipSinglePixie:
		mov		ecx,pRight
		add		pZBufferPtr,2
		dec		edx

		jz		GouraudReturnNoZ
		mov		esi,edx
		and		esi,1
		sub		edx,esi
		jz		GouraudReturnNoZ

NoSinglePixie:
		shr		edx,1
		fld1
		mov     [widTemp],edx                 ; just for a temp
		

		; try to keep fmul fxch pairs seperated to avoid stalling
		; calc this scanlines steps				; FPU Stack
												; st0  st1  st2  st3  st4  st5  st6  st7
		fidiv   dword ptr [widTemp]				; WID
		fild    dword ptr [ecx]EdgeAsmFPU.R		; RR   WID
		fild    dword ptr [ebx]EdgeAsmFPU.R		; RL   RR   WID
		fsub	st(1),st						; RL   RD   WID
		fild	[ecx]EdgeAsmFPU.G				; GR   RL   RD   WID
		fild	[ebx]EdgeAsmFPU.G				; GL   GR   RL   RD   WID
		fsub	st(1),st						; GL   GD   RL   RD   WID
		fild	[ecx]EdgeAsmFPU.B				; BR   GL   GD   RL   RD   WID
		fild	[ebx]EdgeAsmFPU.B				; BL   BR   GL   GD   RL   RD   WID
		fsub	st(1),st						; BL   BD   GL   GD   RL   RD   WID
		fxch	st(5)							; RD   BD   GL   GD   RL   BL   WID
		fmul	st,st(6)						; RI   BD   GL   GD   RL   BL   WID
		frndint
		fxch	st(3)							; GD   BD   GL   RI   RL   BL   WID
		fmul	st,st(6)						; GI   BD   GL   RI   RL   BL   WID
		frndint
		fxch	st(6)							; WID  BD   GL   RI   RL   BL   GI
		fmulp	st(1),st						; BI   GL   RI   RL   BL   GI
		frndint
		fld		qword ptr[RedMask2]				; rm   BI   GL   RI   RL   BL   GI
		fmul	st(3),st						; rm   BI   GL   RI   RL   BL   GI
		fmulp	st(4),st						; BI   GL   RI   RL   BL   GI
		fld		dword ptr[GreenMask2]			; gm   BI   GL   RI   RL   BL   GI
		fmul	st(2),st						; gm   BI   GL   RI   RL   BL   GI
		fmulp	st(6),st						; BI   GL   RI   RL   BL   GI
		fld		dword ptr[BlueMask]				; bm   BI   GL   RI   RL   BL   GI
		fmul	st(1),st						; bm   BI   GL   RI   RL   BL   GI
		fmulp	st(5),st						; BI   GL   RI   RL   BL   GI
		fld		[ecx]EdgeAsmFPU.z				; rz   BI   GL   RI   RL   BL   GI
		fsub	[ebx]EdgeAsmFPU.z				; zd   BI   GL   RI   RL   BL   GI
		fld		[ebx]EdgeAsmFPU.z				; lz   zd   BI   GL   RI   RL   BL   GI
		fistp	[z16]							; zd   BI   GL   RI   RL   BL   GI
		fistp	[ZStep]							; BI   GL   RI   RL   BL   GI

PixieLoop:

		fld		st(3)							; r    BI   GL   RI   RL   BL   GI
		fadd	qword ptr[Magic]				; rk   BI   GL   RI   RL   BL   GI
		fld		st(2)							; g    rk   BI   GL   RI   RL   BL   GI
		fadd	qword ptr[Magic]				; gk   rk   BI   GL   RI   RL   BL   GI
		fxch	st(1)							; rk   gk   BI   GL   RI   RL   BL   GI
		fstp	qword ptr[Bucket]				; gk   BI   GL   RI   RL   BL   GI
		fstp	qword ptr[Bucket2]				; BI   GL   RI   RL   BL   GI
		fld		st(4)							; b    BI   GL   RI   RL   BL   GI
		fadd	qword ptr[Magic]				; bk   BI   GL   RI   RL   BL   GI

		mov		ecx,dword ptr[Bucket]
		mov		eax,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]				; BI   GL   RI   RL   BL   GI
		fadd	st(4),st						; BI   GL   RI   RL   BL2  GI

		and		ecx,REDMASK2
		and		eax,GREENMASK2

		mov		ebx,dword ptr[Bucket]
		or		ecx,eax

		fstp	qword ptr[Bucket]				; GL   RI   RL   BL2  GI
		fadd	st,st(4)						; GL2  RI   RL   BL2  GI
		fstp	qword ptr[Bucket2]				; RI   RL   BL2  GI
		fadd	st(1),st						; RI   RL2  BL2  GI
		fld		qword ptr[Bucket2]				; GL2  RI   RL2  BL2  GI
		fld		qword ptr[Bucket]				; BI   GL2  RI   RL2  BL2  GI

		and		ebx,BLUEMASK2
		mov		edi,TDest

		or		ecx,ebx
		add		TDest,4

		mov		eax,[z16]
		mov		esi,pZBufferPtr

		shr		eax,16
		mov		ebx,[ZStep]

		cmp		word ptr[esi],ax
		jg		SkipPixie

		rol		ecx,16
		add		[z16],ebx

		mov		[edi],ecx
SkipPixie:
		add		pZBufferPtr,4
		dec		edx

		jnz		PixieLoop


		fstp	[u16]
		fstp	[v16]
		fstp	[u16]
		fstp	[v16]
		fstp	[u16]
		fstp	[v16]

GouraudReturnNoZ:
		pop		edi
		pop		esi
		pop		ecx
		pop		ebx
	}
}

void DrawSpan16_AsmLitZBuffer555X86FPU(int32 x1, int32 x2, int32 y)
{
	TDest	=Dest;
	_asm
	{
		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		eax,x1
		mov		ecx,x2
		sub		ecx,eax
		jle		Return16

		mov		edi,[GBitPtr16]
		mov		pTex,edi

		fild	[y]						; y

		mov		esi,x1
		mov		edi,[TDest]

		shl		esi,1
		mov		eax,ecx

		add		edi,esi
		add		pZBufferPtr,esi

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

		//zbuffer step action
		fld		[ZiStepX]
		fmul	dword ptr[ZBufferPrec]
		fmul	dword ptr[Two]
		fistp	dword ptr[ZDelta]

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

		//zbuffer action
		fld		st
		fmul	dword ptr[ZBufferPrec]
		fistp	dword ptr[ZVal]

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
		fld1							; 1    UZR  ZRi  VZR  UL   VL
		fdiv	st,st(2)				; ZR   UZR  ZRi  VZR  UL   VL

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


		mov esi,dword ptr[UFixed]
		mov eax,dword ptr[VFixed]


		mov ecx, GMipLevel4_8
		sar esi, cl
		sar eax, cl
		and esi, 0ffh
		and eax, 0ffh
		mov UDist, esi
		mov VDist, eax

		mov esi,dword ptr[UFixed]
		mov eax,dword ptr[VFixed]
		mov ecx, GMipLevel20
		shr esi, cl
		shr eax, cl

		imul eax, GLightWidth
		add esi, eax

		mov edx, esi
		shl esi, 1
		add edx, esi

		add edx, GLightData

		// Interpolate accross top
		xor ecx, ecx
		mov cl, [edx+3]
		mov eax, ecx
		mov cl, [edx+0]
		sub eax, ecx
		imul eax, UDist
		shl ecx, 8
		add eax, ecx
		mov [R1], eax

		xor ecx, ecx
		mov cl, [edx+4]
		mov eax, ecx
		mov cl, [edx+1]
		sub eax, ecx
		imul eax, UDist
		shl ecx, 8
		add eax, ecx
		mov [G1], eax

		xor ecx, ecx
		mov cl, [edx+5]
		mov eax, ecx
		mov cl, [edx+2]
		sub eax, ecx
		imul eax, UDist
		shl ecx, 8
		add eax, ecx
		mov [B1], eax

		add edx, GLightWidth
		add edx, GLightWidth
		add edx, GLightWidth

		// Interpolate accross bottom
		xor ecx, ecx
		mov cl, [edx+3]
		mov eax, ecx
		mov cl, [edx+0]
		sub eax, ecx
		imul eax, UDist
		shl ecx, 8
		add eax, ecx
		mov [R2], eax

		xor ecx, ecx
		mov cl, [edx+4]
		mov eax, ecx
		mov cl, [edx+1]
		sub eax, ecx
		imul eax, UDist
		shl ecx, 8
		add eax, ecx
		mov [G2], eax

		xor ecx, ecx
		mov cl, [edx+5]
		mov eax, ecx
		mov cl, [edx+2]
		sub eax, ecx
		imul eax, UDist
		shl ecx, 8
		add eax, ecx
		mov [B2], eax

		// Interpolate down
		mov eax, [R2]
		sub eax, [R1]
		imul eax, VDist
		sar eax, 8
		add eax, [R1]
		shr eax, 8
		and	eax,0feh

		mov [RR1], eax

		mov eax, [G2]
		sub eax, [G1]
		imul eax, VDist
		sar eax, 8
		add eax, [G1]
		shr eax, 8
		and	eax,0feh

		mov [GG1], eax

		mov eax, [B2]
		sub eax, [B1]
		imul eax, VDist
		sar eax, 8
		add eax, [B1]
		shr eax, 8
		and	eax,0feh

		mov [BB1], eax

		fstp	[FTemp0]				; UZR  ZRi  VZR  UL   VL
		fstp	[FTemp1]				; ZRi  VZR  UL   VL
		fstp	[FTemp2]				; VZR  UL   VL
		fstp	[FTemp3]				; UL   VL
		fstp	[FTemp4]				; VL
		fstp	[FTemp5]				; 

		fild	[RR1]					; LR
		fild	[GG1]					; LG   LR
		fild	[BB1]					; LB   LG   LR
		
		
		mov		ebx,dword ptr[U1]
		mov		eax,dword ptr[V1]

		add		ebx,dword ptr[UAdjust2]
		add		eax,dword ptr[VAdjust2]

		mov		ecx,[VShift]
		mov		dword ptr[Bucket],ebx

		shl		eax,cl

		push	ebp

		mov		dword ptr[Bucket2],eax
		mov		ebp,dword ptr[DeltaV]

		and		eax,[GHMaskShifted16]
		and		ebx,[GWMaskShifted]

		shl		ebp,cl
		add		eax,ebx

		mov		edx,dword ptr[Bucket2]
		mov		esi,pTex

		shr		eax,16
		mov		dword ptr[DeltaV],ebp

		mov		ebx,dword ptr[Bucket]

		mov		ax,word ptr[2*eax+esi]
		add		edx,dword ptr[DeltaV]

		xor		eax,0
		mov		ecx,edx

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		rol		eax,16
		and		ebx,[GWMaskShifted]

		xor		eax,0
		add		ecx,ebx

		add		edx,dword ptr[DeltaV]
		mov		esi,pTex

		shr		ecx,16
		add		ebx,dword ptr[DeltaU]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,edx

		xor		eax,0
		and		ecx,[GHMaskShifted16]

		mov		esi,eax
		mov		ebp,eax

		and		esi,REDMASK2
		and		ebp,GREENMASK2

		mov		dword ptr[Red],esi
		mov		dword ptr[Green],ebp

		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,[ZVal]

		mov		esi,[ZVal]
		add		ecx,[ZDelta]

		shr		esi,16
		mov		[ZVal],ecx

		mov		ecx,pZBufferPtr

		cmp		si,word ptr[ecx+0]
		jle		Skip0

		mov		[edi+0],ebp
		mov		word ptr[ecx+0],si
		mov		word ptr[ecx+2],si

Skip0:
		mov		ecx,edx

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		and		ebx,[GWMaskShifted]
		xor		eax,0

		add		ecx,ebx
		mov		esi,pTex

		rol		eax,16
		add		edx,dword ptr[DeltaV]

		shr		ecx,16
		add		ebx,dword ptr[DeltaU]

		mov		ax,word ptr[2*ecx+esi]

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
/////////////////////////


		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,[ZVal]

		mov		esi,[ZVal]
		add		ecx,[ZDelta]

		shr		esi,16
		mov		[ZVal],ecx

		mov		ecx,pZBufferPtr

		cmp		si,word ptr[ecx+4]
		jle		Skip1

		mov		[edi+4],ebp
		mov		word ptr[ecx+4],si
		mov		word ptr[ecx+6],si

Skip1:
		mov		ecx,edx

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		and		ebx,[GWMaskShifted]
		xor		eax,0

		add		ecx,ebx
		mov		esi,pTex

		rol		eax,16
		add		edx,dword ptr[DeltaV]

		shr		ecx,16
		add		ebx,dword ptr[DeltaU]

		mov		ax,word ptr[2*ecx+esi]

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
/////////////////////////


		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,[ZVal]

		mov		esi,[ZVal]
		add		ecx,[ZDelta]

		shr		esi,16
		mov		[ZVal],ecx

		mov		ecx,pZBufferPtr

		cmp		si,word ptr[ecx+8]
		jle		Skip2

		mov		[edi+8],ebp
		mov		word ptr[ecx+8],si
		mov		word ptr[ecx+10],si

Skip2:
		mov		ecx,edx

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		and		ebx,[GWMaskShifted]
		xor		eax,0

		add		ecx,ebx
		mov		esi,pTex

		shr		ecx,16
		add		edx,dword ptr[DeltaV]

		rol		eax,16
		mov		ax,word ptr[2*ecx+esi]

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
		add		ebx,dword ptr[DeltaU]
/////////////////////////


		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,[ZVal]

		mov		esi,[ZVal]
		add		ecx,[ZDelta]

		shr		esi,16
		mov		[ZVal],ecx

		mov		ecx,pZBufferPtr

		cmp		si,word ptr[ecx+12]
		jle		Skip3

		mov		[edi+12],ebp
		mov		word ptr[ecx+12],si
		mov		word ptr[ecx+14],si

Skip3:
		mov		ecx,edx

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		and		ebx,[GWMaskShifted]
		xor		eax,0

		add		ecx,ebx
		mov		esi,pTex

		shr		ecx,16
		add		edx,dword ptr[DeltaV]

		rol		eax,16
		mov		ax,word ptr[2*ecx+esi]

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
		add		ebx,dword ptr[DeltaU]
/////////////////////////


		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,[ZVal]

		mov		esi,[ZVal]
		add		ecx,[ZDelta]

		shr		esi,16
		mov		[ZVal],ecx

		mov		ecx,pZBufferPtr

		cmp		si,word ptr[ecx+16]
		jle		Skip4

		mov		[edi+16],ebp
		mov		word ptr[ecx+16],si
		mov		word ptr[ecx+18],si

Skip4:
		mov		ecx,edx

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		and		ebx,[GWMaskShifted]
		xor		eax,0

		add		ecx,ebx
		mov		esi,pTex

		shr		ecx,16
		add		edx,dword ptr[DeltaV]

		rol		eax,16
		mov		ax,word ptr[2*ecx+esi]

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
		add		ebx,dword ptr[DeltaU]
/////////////////////////


		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,[ZVal]

		mov		esi,[ZVal]
		add		ecx,[ZDelta]

		shr		esi,16
		mov		[ZVal],ecx

		mov		ecx,pZBufferPtr

		cmp		si,word ptr[ecx+20]
		jle		Skip5

		mov		[edi+20],ebp
		mov		word ptr[ecx+20],si
		mov		word ptr[ecx+22],si

Skip5:
		mov		ecx,edx

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		and		ebx,[GWMaskShifted]
		xor		eax,0

		add		ecx,ebx
		mov		esi,pTex

		shr		ecx,16
		add		edx,dword ptr[DeltaV]

		rol		eax,16
		mov		ax,word ptr[2*ecx+esi]

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
		add		ebx,dword ptr[DeltaU]
/////////////////////////


		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,[ZVal]

		mov		esi,[ZVal]
		add		ecx,[ZDelta]

		shr		esi,16
		mov		[ZVal],ecx

		mov		ecx,pZBufferPtr

		cmp		si,word ptr[ecx+24]
		jle		Skip6

		mov		[edi+24],ebp
		mov		word ptr[ecx+24],si
		mov		word ptr[ecx+26],si

Skip6:
		mov		ecx,edx

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		and		ebx,[GWMaskShifted]
		xor		eax,0

		add		ecx,ebx
		mov		esi,pTex

		shr		ecx,16
		add		edx,dword ptr[DeltaV]

		rol		eax,16
		mov		ax,word ptr[2*ecx+esi]

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
		add		ebx,dword ptr[DeltaU]
/////////////////////////
		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		fstp	dword ptr[Bucket]
		fstp	dword ptr[Bucket2]
		fstp	dword ptr[Bucket]

		and		esi,GREENMASK2

		fld		dword ptr[FTemp5]
		fld		dword ptr[FTemp4]

		or		ebp,esi

		fld		dword ptr[FTemp3]
		fld		dword ptr[FTemp2]

		rol		ebp,16

		fld		dword ptr[FTemp1]
		fld		dword ptr[FTemp0]
		mov		ecx,[ZVal]

		mov		esi,[ZVal]
		add		ecx,[ZDelta]

		shr		esi,16
		mov		[ZVal],ecx

		mov		ecx,pZBufferPtr

		cmp		si,word ptr[ecx+28]
		jle		Skip7

		mov		[edi+28],ebp
		mov		word ptr[ecx+28],si
		mov		word ptr[ecx+30],si

Skip7:
		pop		ebp


		; get corrected right side deltas ; st0  st1  st2  st3  st4  st5  st6  st7
										; ZR   UZR  ZRi  VZR  UL   VL
		fld		st						; ZR   ZR   UZR  ZRi  VZR  UL   VL
		fmul	st,st(4)				; VR   ZR   UZR  ZRi  VZR  UL   VL
		fxch	st(1)					; ZR   VR   UZR  ZRi  VZR  UL   VL
		fmul	st,st(2)				; UR   VR   UZR  ZRi  VZR  UL   VL

		add		edi,32					; move screen pointer to start of next aspan
		add		[pZBufferPtr],32
		dec		[NumASpans]			; dec num affine spans
		jnz		SpanLoop16

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

OnePixelSpan16:
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
		mov		esi,dword ptr[UFixed]
		mov		eax,dword ptr[VFixed]

		mov ecx, GMipLevel4_8
		sar esi, cl
		sar eax, cl
		and esi, 0ffh
		and eax, 0ffh
		mov UDist, esi
		mov VDist, eax

		mov esi,dword ptr[UFixed]
		mov eax,dword ptr[VFixed]
		mov ecx, GMipLevel20
		shr esi, cl
		shr eax, cl

		imul eax, GLightWidth
		add esi, eax

		mov edx, esi
		shl esi, 1
		add edx, esi

		add edx, GLightData

		// Interpolate accross top
		xor ecx, ecx
		mov cl, [edx+3]
		mov eax, ecx
		mov cl, [edx+0]
		sub eax, ecx
		imul eax, UDist
		shl ecx, 8
		add eax, ecx
		mov [R1], eax

		xor ecx, ecx
		mov cl, [edx+4]
		mov eax, ecx
		mov cl, [edx+1]
		sub eax, ecx
		imul eax, UDist
		shl ecx, 8
		add eax, ecx
		mov [G1], eax

		xor ecx, ecx
		mov cl, [edx+5]
		mov eax, ecx
		mov cl, [edx+2]
		sub eax, ecx
		imul eax, UDist
		shl ecx, 8
		add eax, ecx
		mov [B1], eax

		add edx, GLightWidth
		add edx, GLightWidth
		add edx, GLightWidth

		// Interpolate accross bottom
		xor ecx, ecx
		mov cl, [edx+3]
		mov eax, ecx
		mov cl, [edx+0]
		sub eax, ecx
		imul eax, UDist
		shl ecx, 8
		add eax, ecx
		mov [R2], eax

		xor ecx, ecx
		mov cl, [edx+4]
		mov eax, ecx
		mov cl, [edx+1]
		sub eax, ecx
		imul eax, UDist
		shl ecx, 8
		add eax, ecx
		mov [G2], eax

		xor ecx, ecx
		mov cl, [edx+5]
		mov eax, ecx
		mov cl, [edx+2]
		sub eax, ecx
		imul eax, UDist
		shl ecx, 8
		add eax, ecx
		mov [B2], eax

		// Interpolate down
		mov eax, [R2]
		sub eax, [R1]
		imul eax, VDist
		sar eax, 8
		add eax, [R1]
		shr eax, 8
		and	eax,0feh

		mov [RR1], eax

		mov eax, [G2]
		sub eax, [G1]
		imul eax, VDist
		sar eax, 8
		add eax, [G1]
		shr eax, 8
		and	eax,0feh

		mov [GG1], eax

		mov eax, [B2]
		sub eax, [B1]
		imul eax, VDist
		sar eax, 8
		add eax, [B1]
		shr eax, 8
		and	eax,0feh

		mov [BB1], eax

		fstp	[FTemp0]
		fstp	[FTemp1]
		fstp	[FTemp2]
		fstp	[FTemp3]
		fstp	[FTemp4]
		fstp	[FTemp5]

		mov		ebx,dword ptr[U1]
		mov		edx,dword ptr[V1]

		fild	[RR1]					; LR
		fild	[GG1]					; LG   LR
		fild	[BB1]					; LB   LG   LR

		mov		ecx,[VShift]
		add		edx,dword ptr[VAdjust2]

		add		ebx,dword ptr[UAdjust2]
		mov		eax,dword ptr[DeltaV]

		shl		eax,cl
		mov		esi,pTex

		shl		edx,cl
		mov		dword ptr[DeltaV],eax

		mov		eax,[ZDelta]
		mov		ecx,[ZVal]

		sar		eax,1
		push	ebp

		mov		[ZDelta],eax

LeftoverLoop16:
		mov		eax,edx
		and		ebx,[GWMaskShifted]

		and		eax,[GHMaskShifted16]

		add		eax,ebx
		add		ebx,dword ptr[DeltaU]

		shr		eax,16
		add		edi,2

		mov		ax,word ptr[2*eax+esi]
		add		edx,dword ptr[DeltaV]

		xor		eax,0

		mov		esi,eax
		mov		ebp,eax

		and		esi,REDMASK2
		and		ebp,GREENMASK2

		mov		dword ptr[Red],esi
		mov		dword ptr[Green],ebp

		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		and		esi,GREENMASK2
		mov		eax,ecx

		or		ebp,esi

		shr		eax,16
		mov		esi,pZBufferPtr

		cmp		ax,word ptr[esi]
		jle		SkipLeftOver

		mov		word ptr[edi-2],bp
		mov		word ptr[esi],ax

SkipLeftOver:
		add		ecx,[ZDelta]
		mov		esi,pTex

		add		pZBufferPtr,2

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
		pop		edi
		pop		esi
		pop		ecx
		pop		ebx
	}
}

void DrawSpan16_AsmLitZWrite555X86FPU(int32 x1, int32 x2, int32 y)
{
	TDest	=Dest;
	_asm
	{
		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		eax,x1
		mov		ecx,x2
		sub		ecx,eax
		jle		Return16

		mov		edi,[GBitPtr16]
		mov		pTex,edi

		fild	[y]						; y

		mov		esi,x1
		mov		edi,[TDest]

		shl		esi,1
		mov		eax,ecx

		add		edi,esi
		add		pZBufferPtr,esi

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

		//zbuffer step action
		fld		[ZiStepX]
		fmul	dword ptr[ZBufferPrec]
		fmul	dword ptr[Two]
		fistp	dword ptr[ZDelta]

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

		//zbuffer action
		fld		st
		fmul	dword ptr[ZBufferPrec]
		fistp	dword ptr[ZVal]

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
		fld1							; 1    UZR  ZRi  VZR  UL   VL
		fdiv	st,st(2)				; ZR   UZR  ZRi  VZR  UL   VL

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


		mov esi,dword ptr[UFixed]
		mov eax,dword ptr[VFixed]


		mov ecx, GMipLevel4_8
		sar esi, cl
		sar eax, cl
		and esi, 0ffh
		and eax, 0ffh
		mov UDist, esi
		mov VDist, eax

		mov esi,dword ptr[UFixed]
		mov eax,dword ptr[VFixed]
		mov ecx, GMipLevel20
		shr esi, cl
		shr eax, cl

		imul eax, GLightWidth
		add esi, eax

		mov edx, esi
		shl esi, 1
		add edx, esi

		add edx, GLightData

		// Interpolate accross top
		xor ecx, ecx
		mov cl, [edx+3]
		mov eax, ecx
		mov cl, [edx+0]
		sub eax, ecx
		imul eax, UDist
		shl ecx, 8
		add eax, ecx
		mov [R1], eax

		xor ecx, ecx
		mov cl, [edx+4]
		mov eax, ecx
		mov cl, [edx+1]
		sub eax, ecx
		imul eax, UDist
		shl ecx, 8
		add eax, ecx
		mov [G1], eax

		xor ecx, ecx
		mov cl, [edx+5]
		mov eax, ecx
		mov cl, [edx+2]
		sub eax, ecx
		imul eax, UDist
		shl ecx, 8
		add eax, ecx
		mov [B1], eax

		add edx, GLightWidth
		add edx, GLightWidth
		add edx, GLightWidth

		// Interpolate accross bottom
		xor ecx, ecx
		mov cl, [edx+3]
		mov eax, ecx
		mov cl, [edx+0]
		sub eax, ecx
		imul eax, UDist
		shl ecx, 8
		add eax, ecx
		mov [R2], eax

		xor ecx, ecx
		mov cl, [edx+4]
		mov eax, ecx
		mov cl, [edx+1]
		sub eax, ecx
		imul eax, UDist
		shl ecx, 8
		add eax, ecx
		mov [G2], eax

		xor ecx, ecx
		mov cl, [edx+5]
		mov eax, ecx
		mov cl, [edx+2]
		sub eax, ecx
		imul eax, UDist
		shl ecx, 8
		add eax, ecx
		mov [B2], eax

		// Interpolate down
		mov eax, [R2]
		sub eax, [R1]
		imul eax, VDist
		sar eax, 8
		add eax, [R1]
		shr eax, 8
		and	eax,0feh

		mov [RR1], eax

		mov eax, [G2]
		sub eax, [G1]
		imul eax, VDist
		sar eax, 8
		add eax, [G1]
		shr eax, 8
		and	eax,0feh

		mov [GG1], eax

		mov eax, [B2]
		sub eax, [B1]
		imul eax, VDist
		sar eax, 8
		add eax, [B1]
		shr eax, 8
		and	eax,0feh

		mov [BB1], eax

		fstp	[FTemp0]				; UZR  ZRi  VZR  UL   VL
		fstp	[FTemp1]				; ZRi  VZR  UL   VL
		fstp	[FTemp2]				; VZR  UL   VL
		fstp	[FTemp3]				; UL   VL
		fstp	[FTemp4]				; VL
		fstp	[FTemp5]				; 

		fild	[RR1]					; LR
		fild	[GG1]					; LG   LR
		fild	[BB1]					; LB   LG   LR
		
		
		mov		ebx,dword ptr[U1]
		mov		eax,dword ptr[V1]

		add		ebx,dword ptr[UAdjust2]
		add		eax,dword ptr[VAdjust2]

		mov		ecx,[VShift]
		mov		dword ptr[Bucket],ebx

		shl		eax,cl

		push	ebp

		mov		dword ptr[Bucket2],eax
		mov		ebp,dword ptr[DeltaV]

		and		eax,[GHMaskShifted16]
		and		ebx,[GWMaskShifted]

		shl		ebp,cl
		add		eax,ebx

		mov		edx,dword ptr[Bucket2]
		mov		esi,pTex

		shr		eax,16
		mov		dword ptr[DeltaV],ebp

		mov		ebx,dword ptr[Bucket]

		mov		ax,word ptr[2*eax+esi]
		add		edx,dword ptr[DeltaV]

		xor		eax,0
		mov		ecx,edx

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		rol		eax,16
		and		ebx,[GWMaskShifted]

		xor		eax,0
		add		ecx,ebx

		add		edx,dword ptr[DeltaV]
		mov		esi,pTex

		shr		ecx,16
		add		ebx,dword ptr[DeltaU]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,edx

		xor		eax,0
		and		ecx,[GHMaskShifted16]

		mov		esi,eax
		mov		ebp,eax

		and		esi,REDMASK2
		and		ebp,GREENMASK2

		mov		dword ptr[Red],esi
		mov		dword ptr[Green],ebp

		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,[ZVal]

		mov		esi,[ZVal]
		add		ecx,[ZDelta]

		shr		esi,16
		mov		[ZVal],ecx

		mov		ecx,pZBufferPtr
		mov		[edi+0],ebp

		mov		word ptr[ecx+0],si
		mov		word ptr[ecx+2],si
		mov		ecx,edx

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		and		ebx,[GWMaskShifted]
		xor		eax,0

		add		ecx,ebx
		mov		esi,pTex

		rol		eax,16
		add		edx,dword ptr[DeltaV]

		shr		ecx,16
		add		ebx,dword ptr[DeltaU]

		mov		ax,word ptr[2*ecx+esi]

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
/////////////////////////


		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,[ZVal]

		mov		esi,[ZVal]
		add		ecx,[ZDelta]

		shr		esi,16
		mov		[ZVal],ecx

		mov		ecx,pZBufferPtr
		mov		[edi+4],ebp

		mov		word ptr[ecx+4],si
		mov		word ptr[ecx+6],si
		mov		ecx,edx

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		and		ebx,[GWMaskShifted]
		xor		eax,0

		add		ecx,ebx
		mov		esi,pTex

		rol		eax,16
		add		edx,dword ptr[DeltaV]

		shr		ecx,16
		add		ebx,dword ptr[DeltaU]

		mov		ax,word ptr[2*ecx+esi]

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
/////////////////////////


		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,[ZVal]

		mov		esi,[ZVal]
		add		ecx,[ZDelta]

		shr		esi,16
		mov		[ZVal],ecx

		mov		ecx,pZBufferPtr
		mov		[edi+8],ebp

		mov		word ptr[ecx+8],si
		mov		word ptr[ecx+10],si
		mov		ecx,edx

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		and		ebx,[GWMaskShifted]
		xor		eax,0

		add		ecx,ebx
		mov		esi,pTex

		shr		ecx,16
		add		edx,dword ptr[DeltaV]

		rol		eax,16
		mov		ax,word ptr[2*ecx+esi]

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
		add		ebx,dword ptr[DeltaU]
/////////////////////////


		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,[ZVal]

		mov		esi,[ZVal]
		add		ecx,[ZDelta]

		shr		esi,16
		mov		[ZVal],ecx

		mov		ecx,pZBufferPtr
		mov		[edi+12],ebp

		mov		word ptr[ecx+12],si
		mov		word ptr[ecx+14],si
		mov		ecx,edx

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		and		ebx,[GWMaskShifted]
		xor		eax,0

		add		ecx,ebx
		mov		esi,pTex

		shr		ecx,16
		add		edx,dword ptr[DeltaV]

		rol		eax,16
		mov		ax,word ptr[2*ecx+esi]

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
		add		ebx,dword ptr[DeltaU]
/////////////////////////


		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,[ZVal]

		mov		esi,[ZVal]
		add		ecx,[ZDelta]

		shr		esi,16
		mov		[ZVal],ecx

		mov		ecx,pZBufferPtr
		mov		[edi+16],ebp

		mov		word ptr[ecx+16],si
		mov		word ptr[ecx+18],si
		mov		ecx,edx

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		and		ebx,[GWMaskShifted]
		xor		eax,0

		add		ecx,ebx
		mov		esi,pTex

		shr		ecx,16
		add		edx,dword ptr[DeltaV]

		rol		eax,16
		mov		ax,word ptr[2*ecx+esi]

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
		add		ebx,dword ptr[DeltaU]
/////////////////////////


		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,[ZVal]

		mov		esi,[ZVal]
		add		ecx,[ZDelta]

		shr		esi,16
		mov		[ZVal],ecx

		mov		ecx,pZBufferPtr
		mov		[edi+20],ebp

		mov		word ptr[ecx+20],si
		mov		word ptr[ecx+22],si
		mov		ecx,edx

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		and		ebx,[GWMaskShifted]
		xor		eax,0

		add		ecx,ebx
		mov		esi,pTex

		shr		ecx,16
		add		edx,dword ptr[DeltaV]

		rol		eax,16
		mov		ax,word ptr[2*ecx+esi]

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
		add		ebx,dword ptr[DeltaU]
/////////////////////////


		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,[ZVal]

		mov		esi,[ZVal]
		add		ecx,[ZDelta]

		shr		esi,16
		mov		[ZVal],ecx

		mov		ecx,pZBufferPtr
		mov		[edi+24],ebp

		mov		word ptr[ecx+24],si
		mov		word ptr[ecx+26],si
		mov		ecx,edx

		add		ebx,dword ptr[DeltaU]
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,pTex

		add		ecx,ebx

		shr		ecx,16
		add		edx,dword ptr[DeltaV]

		rol		eax,16
		mov		ax,word ptr[2*ecx+esi]

//		mov		ecx,edx
		xor		eax,0

		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
//		add		ebx,dword ptr[DeltaU]
/////////////////////////
		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
//		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		fstp	dword ptr[Bucket]
		fstp	dword ptr[Bucket2]
		fstp	dword ptr[Bucket]

		and		esi,GREENMASK2

		fld		dword ptr[FTemp5]
		fld		dword ptr[FTemp4]

		or		ebp,esi

		fld		dword ptr[FTemp3]
		fld		dword ptr[FTemp2]

		rol		ebp,16

		fld		dword ptr[FTemp1]
		fld		dword ptr[FTemp0]
		mov		ecx,[ZVal]

		mov		esi,[ZVal]
		add		ecx,[ZDelta]

		shr		esi,16
		mov		[ZVal],ecx

		mov		ecx,pZBufferPtr
		mov		[edi+28],ebp

		mov		word ptr[ecx+28],si
		mov		word ptr[ecx+30],si
		pop		ebp


		; get corrected right side deltas ; st0  st1  st2  st3  st4  st5  st6  st7
										; ZR   UZR  ZRi  VZR  UL   VL
		fld		st						; ZR   ZR   UZR  ZRi  VZR  UL   VL
		fmul	st,st(4)				; VR   ZR   UZR  ZRi  VZR  UL   VL
		fxch	st(1)					; ZR   VR   UZR  ZRi  VZR  UL   VL
		fmul	st,st(2)				; UR   VR   UZR  ZRi  VZR  UL   VL

		add		edi,32					; move screen pointer to start of next aspan
		add		pZBufferPtr,32
		dec		[NumASpans]			; dec num affine spans
		jnz		SpanLoop16

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

OnePixelSpan16:
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
		mov		esi,dword ptr[UFixed]
		mov		eax,dword ptr[VFixed]

		mov ecx, GMipLevel4_8
		sar esi, cl
		sar eax, cl
		and esi, 0ffh
		and eax, 0ffh
		mov UDist, esi
		mov VDist, eax

		mov esi,dword ptr[UFixed]
		mov eax,dword ptr[VFixed]
		mov ecx, GMipLevel20
		shr esi, cl
		shr eax, cl

		imul eax, GLightWidth
		add esi, eax

		mov edx, esi
		shl esi, 1
		add edx, esi

		add edx, GLightData

		// Interpolate accross top
		xor ecx, ecx
		mov cl, [edx+3]
		mov eax, ecx
		mov cl, [edx+0]
		sub eax, ecx
		imul eax, UDist
		shl ecx, 8
		add eax, ecx
		mov [R1], eax

		xor ecx, ecx
		mov cl, [edx+4]
		mov eax, ecx
		mov cl, [edx+1]
		sub eax, ecx
		imul eax, UDist
		shl ecx, 8
		add eax, ecx
		mov [G1], eax

		xor ecx, ecx
		mov cl, [edx+5]
		mov eax, ecx
		mov cl, [edx+2]
		sub eax, ecx
		imul eax, UDist
		shl ecx, 8
		add eax, ecx
		mov [B1], eax

		add edx, GLightWidth
		add edx, GLightWidth
		add edx, GLightWidth

		// Interpolate accross bottom
		xor ecx, ecx
		mov cl, [edx+3]
		mov eax, ecx
		mov cl, [edx+0]
		sub eax, ecx
		imul eax, UDist
		shl ecx, 8
		add eax, ecx
		mov [R2], eax

		xor ecx, ecx
		mov cl, [edx+4]
		mov eax, ecx
		mov cl, [edx+1]
		sub eax, ecx
		imul eax, UDist
		shl ecx, 8
		add eax, ecx
		mov [G2], eax

		xor ecx, ecx
		mov cl, [edx+5]
		mov eax, ecx
		mov cl, [edx+2]
		sub eax, ecx
		imul eax, UDist
		shl ecx, 8
		add eax, ecx
		mov [B2], eax

		// Interpolate down
		mov eax, [R2]
		sub eax, [R1]
		imul eax, VDist
		sar eax, 8
		add eax, [R1]
		shr eax, 8
		and	eax,0feh

		mov [RR1], eax

		mov eax, [G2]
		sub eax, [G1]
		imul eax, VDist
		sar eax, 8
		add eax, [G1]
		shr eax, 8
		and	eax,0feh

		mov [GG1], eax

		mov eax, [B2]
		sub eax, [B1]
		imul eax, VDist
		sar eax, 8
		add eax, [B1]
		shr eax, 8
		and	eax,0feh

		mov [BB1], eax

		fstp	[FTemp0]
		fstp	[FTemp1]
		fstp	[FTemp2]
		fstp	[FTemp3]
		fstp	[FTemp4]
		fstp	[FTemp5]

		mov		ebx,dword ptr[U1]
		mov		edx,dword ptr[V1]

		fild	[RR1]					; LR
		fild	[GG1]					; LG   LR
		fild	[BB1]					; LB   LG   LR

		mov		ecx,[VShift]
		add		edx,dword ptr[VAdjust2]

		add		ebx,dword ptr[UAdjust2]
		mov		eax,dword ptr[DeltaV]

		shl		eax,cl
		mov		esi,pTex

		shl		edx,cl
		mov		dword ptr[DeltaV],eax

		mov		eax,[ZDelta]
		mov		ecx,[ZVal]

		sar		eax,1
		push	ebp

		mov		[ZDelta],eax

LeftoverLoop16:
		mov		eax,edx
		and		ebx,[GWMaskShifted]

		and		eax,[GHMaskShifted16]

		add		eax,ebx
		add		ebx,dword ptr[DeltaU]

		shr		eax,16
		add		edi,2

		mov		ax,word ptr[2*eax+esi]
		add		edx,dword ptr[DeltaV]

		xor		eax,0

		mov		esi,eax
		mov		ebp,eax

		and		esi,REDMASK2
		and		ebp,GREENMASK2

		mov		dword ptr[Red],esi
		mov		dword ptr[Green],ebp

		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		and		esi,GREENMASK2
		mov		eax,ecx

		or		ebp,esi

		shr		eax,16
		mov		esi,pZBufferPtr

		mov		word ptr[edi-2],bp
		mov		word ptr[esi],ax

		add		ecx,[ZDelta]
		mov		esi,pTex

		add		pZBufferPtr,2

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
		pop		edi
		pop		esi
		pop		ecx
		pop		ebx
	}
}

void DrawSpan16_AsmGouraudZBuffer555X86FPU(int32 x1, int32 x2, int32 y, int32 r1, int32 g1, int32 b1, int32 r2, int32 g2, int32 b2)
{
	RR1	=r1;
	GG1	=g1;
	BB1	=b1;
	TDest	=Dest;
	_asm
	{
		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		eax,x1
		mov		ecx,x2
		sub		ecx,eax
		jle		Return16

		mov		edi,[GBitPtr16]
		mov		pTex,edi

		fild	[y]						; y
		mov		[widTemp],ecx

		fild	[x1]						; x    y
		fild	[widTemp]

		fld1
		fdivrp	st(1),st

		mov		esi,x1
		mov		edi,[TDest]

		shl		esi,1
		mov		eax,ecx

		add		edi,esi
		add		pZBufferPtr,esi

		mov		eax,ecx
		shr		ecx,4
		and		eax,15
		_emit 75h
		_emit 06h
		dec		ecx
		mov		eax,16

		mov		[NumASpans],ecx
		mov		[RemainingCount],eax

		fild	[r2]
		fisub	[r1]
		fild	[g2]
		fisub	[g1]
		fild	[b2]
		fisub	[b1]
		fxch	st(2)
		fmul	st,st(3)
		fxch	st(1)
		fmul	st,st(3)
		fxch	st(2)
		fmul	st,st(3)
		frndint
		fstp	qword ptr[BlueDelta]
		frndint
		fstp	qword ptr[RedDelta]
		frndint
		fstp	qword ptr[GreenDelta]
		fstp	qword ptr[FTemp0]


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

		//zbuffer step action
		fld		[ZiStepX]
		fmul	dword ptr[ZBufferPrec]
		fmul	dword ptr[Two]
		fistp	dword ptr[ZDelta]

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

		//zbuffer action
		fld		st
		fmul	dword ptr[ZBufferPrec]
		fistp	dword ptr[ZVal]

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
		fld1							; 1    UZR  ZRi  VZR  UL   VL
		fdiv	st,st(2)				; ZR   UZR  ZRi  VZR  UL   VL

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

		fstp	[FTemp0]				; UZR  ZRi  VZR  UL   VL
		fstp	[FTemp1]				; ZRi  VZR  UL   VL
		fstp	[FTemp2]				; VZR  UL   VL
		fstp	[FTemp3]				; UL   VL
		fstp	[FTemp4]				; VL
		fstp	[FTemp5]				; 


		fild	[RR1]					; LR
		fild	[GG1]					; LG   LR
		fild	[BB1]					; LB   LG   LR
		
		
		mov		ebx,dword ptr[U1]
		mov		eax,dword ptr[V1]

		add		ebx,dword ptr[UAdjust2]
		add		eax,dword ptr[VAdjust2]

		mov		ecx,[VShift]
		mov		dword ptr[Bucket],ebx

		shl		eax,cl

		push	ebp

		mov		dword ptr[Bucket2],eax
		mov		ebp,dword ptr[DeltaV]

		and		eax,[GHMaskShifted16]
		and		ebx,[GWMaskShifted]

		shl		ebp,cl
		add		eax,ebx

		mov		edx,dword ptr[Bucket2]
		mov		esi,pTex

		shr		eax,16
		mov		dword ptr[DeltaV],ebp

		mov		ebx,dword ptr[Bucket]

		mov		ax,word ptr[2*eax+esi]
		add		edx,dword ptr[DeltaV]

		xor		eax,0
		mov		ecx,edx

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		rol		eax,16
		and		ebx,[GWMaskShifted]

		xor		eax,0
		add		ecx,ebx

		add		edx,dword ptr[DeltaV]
		mov		esi,pTex

		shr		ecx,16
		add		ebx,dword ptr[DeltaU]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,edx

		xor		eax,0
		and		ecx,[GHMaskShifted16]

		mov		esi,eax
		mov		ebp,eax

		and		esi,REDMASK2
		and		ebp,GREENMASK2

		mov		dword ptr[Red],esi
		mov		dword ptr[Green],ebp

		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR
		fxch	st(1)
		fadd	qword ptr[GreenDelta]
		fxch	st(1)
		fadd	qword ptr[BlueDelta]
		fxch	st(2)
		fadd	qword ptr[RedDelta]
		fxch	st(2)

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,[ZVal]

		mov		esi,[ZVal]
		add		ecx,[ZDelta]

		shr		esi,16
		mov		[ZVal],ecx

		mov		ecx,pZBufferPtr

		cmp		si,word ptr[ecx+0]
		jle		Skip0

		mov		[edi+0],ebp
		mov		word ptr[ecx+0],si
		mov		word ptr[ecx+2],si

Skip0:
		mov		ecx,edx
		mov		esi,pTex

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		and		ebx,[GWMaskShifted]
		xor		eax,0

		add		ecx,ebx

		rol		eax,16
		add		edx,dword ptr[DeltaV]

		shr		ecx,16
		add		ebx,dword ptr[DeltaU]

		mov		ax,word ptr[2*ecx+esi]

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
/////////////////////////


		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR
		fxch	st(1)
		fadd	qword ptr[GreenDelta]
		fxch	st(1)
		fadd	qword ptr[BlueDelta]
		fxch	st(2)
		fadd	qword ptr[RedDelta]
		fxch	st(2)

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,[ZVal]

		mov		esi,[ZVal]
		add		ecx,[ZDelta]

		shr		esi,16
		mov		[ZVal],ecx

		mov		ecx,pZBufferPtr

		cmp		si,word ptr[ecx+4]
		jle		Skip1

		mov		[edi+4],ebp
		mov		word ptr[ecx+4],si
		mov		word ptr[ecx+6],si

Skip1:
		mov		ecx,edx
		mov		esi,pTex

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		and		ebx,[GWMaskShifted]
		xor		eax,0

		add		ecx,ebx

		rol		eax,16
		add		edx,dword ptr[DeltaV]

		shr		ecx,16
		add		ebx,dword ptr[DeltaU]

		mov		ax,word ptr[2*ecx+esi]

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
/////////////////////////


		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR
		fxch	st(1)
		fadd	qword ptr[GreenDelta]
		fxch	st(1)
		fadd	qword ptr[BlueDelta]
		fxch	st(2)
		fadd	qword ptr[RedDelta]
		fxch	st(2)

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,[ZVal]

		mov		esi,[ZVal]
		add		ecx,[ZDelta]

		shr		esi,16
		mov		[ZVal],ecx

		mov		ecx,pZBufferPtr

		cmp		si,word ptr[ecx+8]
		jle		Skip2

		mov		[edi+8],ebp
		mov		word ptr[ecx+8],si
		mov		word ptr[ecx+10],si

Skip2:
		mov		ecx,edx
		mov		esi,pTex

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		and		ebx,[GWMaskShifted]
		xor		eax,0

		add		ecx,ebx

		shr		ecx,16
		add		edx,dword ptr[DeltaV]

		rol		eax,16
		mov		ax,word ptr[2*ecx+esi]

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
		add		ebx,dword ptr[DeltaU]
/////////////////////////


		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR
		fxch	st(1)
		fadd	qword ptr[GreenDelta]
		fxch	st(1)
		fadd	qword ptr[BlueDelta]
		fxch	st(2)
		fadd	qword ptr[RedDelta]
		fxch	st(2)

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,[ZVal]

		mov		esi,[ZVal]
		add		ecx,[ZDelta]

		shr		esi,16
		mov		[ZVal],ecx

		mov		ecx,pZBufferPtr

		cmp		si,word ptr[ecx+12]
		jle		Skip3

		mov		[edi+12],ebp
		mov		word ptr[ecx+12],si
		mov		word ptr[ecx+14],si

Skip3:
		mov		ecx,edx
		mov		esi,pTex

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		and		ebx,[GWMaskShifted]
		xor		eax,0

		add		ecx,ebx

		shr		ecx,16
		add		edx,dword ptr[DeltaV]

		rol		eax,16
		mov		ax,word ptr[2*ecx+esi]

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
		add		ebx,dword ptr[DeltaU]
/////////////////////////


		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR
		fxch	st(1)
		fadd	qword ptr[GreenDelta]
		fxch	st(1)
		fadd	qword ptr[BlueDelta]
		fxch	st(2)
		fadd	qword ptr[RedDelta]
		fxch	st(2)

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,[ZVal]

		mov		esi,[ZVal]
		add		ecx,[ZDelta]

		shr		esi,16
		mov		[ZVal],ecx

		mov		ecx,pZBufferPtr

		cmp		si,word ptr[ecx+16]
		jle		Skip4

		mov		[edi+16],ebp
		mov		word ptr[ecx+16],si
		mov		word ptr[ecx+18],si

Skip4:
		mov		ecx,edx
		mov		esi,pTex

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		and		ebx,[GWMaskShifted]
		xor		eax,0

		add		ecx,ebx

		shr		ecx,16
		add		edx,dword ptr[DeltaV]

		rol		eax,16
		mov		ax,word ptr[2*ecx+esi]

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
		add		ebx,dword ptr[DeltaU]
/////////////////////////


		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR
		fxch	st(1)
		fadd	qword ptr[GreenDelta]
		fxch	st(1)
		fadd	qword ptr[BlueDelta]
		fxch	st(2)
		fadd	qword ptr[RedDelta]
		fxch	st(2)

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,[ZVal]

		mov		esi,[ZVal]
		add		ecx,[ZDelta]

		shr		esi,16
		mov		[ZVal],ecx

		mov		ecx,pZBufferPtr

		cmp		si,word ptr[ecx+20]
		jle		Skip5

		mov		[edi+20],ebp
		mov		word ptr[ecx+20],si
		mov		word ptr[ecx+22],si

Skip5:
		mov		ecx,edx
		mov		esi,pTex

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		and		ebx,[GWMaskShifted]
		xor		eax,0

		add		ecx,ebx

		shr		ecx,16
		add		edx,dword ptr[DeltaV]

		rol		eax,16
		mov		ax,word ptr[2*ecx+esi]

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
		add		ebx,dword ptr[DeltaU]
/////////////////////////


		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR
		fxch	st(1)
		fadd	qword ptr[GreenDelta]
		fxch	st(1)
		fadd	qword ptr[BlueDelta]
		fxch	st(2)
		fadd	qword ptr[RedDelta]
		fxch	st(2)

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,[ZVal]

		mov		esi,[ZVal]
		add		ecx,[ZDelta]

		shr		esi,16
		mov		[ZVal],ecx

		mov		ecx,pZBufferPtr

		cmp		si,word ptr[ecx+24]
		jle		Skip6

		mov		[edi+24],ebp
		mov		word ptr[ecx+24],si
		mov		word ptr[ecx+26],si

Skip6:
		mov		ecx,edx
		mov		esi,pTex

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		and		ebx,[GWMaskShifted]
		xor		eax,0

		add		ecx,ebx

		shr		ecx,16
		add		edx,dword ptr[DeltaV]

		rol		eax,16
		mov		ax,word ptr[2*ecx+esi]

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
		add		ebx,dword ptr[DeltaU]
/////////////////////////
		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR
		fadd	qword ptr[BlueDelta]
		fistp	dword ptr[BB1]
		fadd	qword ptr[GreenDelta]
		fistp	dword ptr[GG1]
		fadd	qword ptr[RedDelta]
		fistp	dword ptr[RR1]

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		fstp	dword ptr[Bucket]
		fstp	dword ptr[Bucket2]
		fstp	dword ptr[Bucket]

		and		esi,GREENMASK2

		fld		dword ptr[FTemp5]
		fld		dword ptr[FTemp4]

		or		ebp,esi

		fld		dword ptr[FTemp3]
		fld		dword ptr[FTemp2]

		rol		ebp,16

		fld		dword ptr[FTemp1]
		fld		dword ptr[FTemp0]

		mov		ecx,[ZVal]

		mov		esi,[ZVal]
		add		ecx,[ZDelta]

		shr		esi,16
		mov		[ZVal],ecx

		mov		ecx,pZBufferPtr

		cmp		si,word ptr[ecx+28]
		jle		Skip7

		mov		[edi+28],ebp
		mov		word ptr[ecx+28],si
		mov		word ptr[ecx+30],si

Skip7:
		pop		ebp


		; get corrected right side deltas ; st0  st1  st2  st3  st4  st5  st6  st7
										; ZR   UZR  ZRi  VZR  UL   VL
		fld		st						; ZR   ZR   UZR  ZRi  VZR  UL   VL
		fmul	st,st(4)				; VR   ZR   UZR  ZRi  VZR  UL   VL
		fxch	st(1)					; ZR   VR   UZR  ZRi  VZR  UL   VL
		fmul	st,st(2)				; UR   VR   UZR  ZRi  VZR  UL   VL

		add		edi,32					; move screen pointer to start of next aspan
		add		[pZBufferPtr],32
		dec		[NumASpans]			; dec num affine spans
		jnz		SpanLoop16

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

OnePixelSpan16:
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

		fstp	[FTemp0]
		fstp	[FTemp1]
		fstp	[FTemp2]
		fstp	[FTemp3]
		fstp	[FTemp4]
		fstp	[FTemp5]

		mov		ebx,dword ptr[U1]
		mov		edx,dword ptr[V1]

		fild	[RR1]					; LR
		fild	[GG1]					; LG   LR
		fild	[BB1]					; LB   LG   LR

		mov		ecx,[VShift]
		add		edx,dword ptr[VAdjust2]

		add		ebx,dword ptr[UAdjust2]
		mov		eax,dword ptr[DeltaV]

		shl		eax,cl
		mov		esi,pTex

		shl		edx,cl
		mov		dword ptr[DeltaV],eax

		mov		eax,[ZDelta]
		mov		ecx,[ZVal]

		sar		eax,1
		push	ebp

		mov		[ZDelta],eax

LeftoverLoop16:
		mov		eax,edx
		and		ebx,[GWMaskShifted]

		and		eax,[GHMaskShifted16]

		add		eax,ebx
		add		ebx,dword ptr[DeltaU]

		shr		eax,16
		add		edi,2

		mov		ax,word ptr[2*eax+esi]
		add		edx,dword ptr[DeltaV]

		xor		eax,0

		mov		esi,eax
		mov		ebp,eax

		and		esi,REDMASK2
		and		ebp,GREENMASK2

		mov		dword ptr[Red],esi
		mov		dword ptr[Green],ebp

		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		and		esi,GREENMASK2
		mov		eax,ecx

		or		ebp,esi

		shr		eax,16
		mov		esi,pZBufferPtr

		cmp		ax,word ptr[esi]
		jle		SkipLeftOver

		mov		word ptr[edi-2],bp
		mov		word ptr[esi],ax

SkipLeftOver:
		add		ecx,[ZDelta]
		mov		esi,pTex

		add		pZBufferPtr,2

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
		pop		edi
		pop		esi
		pop		ecx
		pop		ebx
	}
}


void DrawSpan16_AsmGouraudZWrite555X86FPU(int32 x1, int32 x2, int32 y, int32 r1, int32 g1, int32 b1, int32 r2, int32 g2, int32 b2)
{
	RR1	=r1;
	GG1	=g1;
	BB1	=b1;
	TDest	=Dest;
	_asm
	{
		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		eax,x1
		mov		ecx,x2
		sub		ecx,eax
		jle		Return16

		mov		edi,[GBitPtr16]
		mov		pTex,edi

		fild	[y]						; y
		mov		[widTemp],ecx

		fild	[x1]						; x    y
		fild	[widTemp]

		fld1
		fdivrp	st(1),st

		mov		esi,x1
		mov		edi,[TDest]

		shl		esi,1
		mov		eax,ecx

		add		edi,esi
		add		pZBufferPtr,esi

		mov		eax,ecx
		shr		ecx,4
		and		eax,15
		_emit 75h
		_emit 06h
		dec		ecx
		mov		eax,16

		mov		[NumASpans],ecx
		mov		[RemainingCount],eax

		fild	[r2]
		fisub	[r1]
		fild	[g2]
		fisub	[g1]
		fild	[b2]
		fisub	[b1]
		fxch	st(2)
		fmul	st,st(3)
		fxch	st(1)
		fmul	st,st(3)
		fxch	st(2)
		fmul	st,st(3)
		frndint
		fstp	qword ptr[BlueDelta]
		frndint
		fstp	qword ptr[RedDelta]
		frndint
		fstp	qword ptr[GreenDelta]
		fstp	qword ptr[FTemp0]


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

		//zbuffer step action
		fld		[ZiStepX]
		fmul	dword ptr[ZBufferPrec]
		fmul	dword ptr[Two]
		fistp	dword ptr[ZDelta]

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

		//zbuffer action
		fld		st
		fmul	dword ptr[ZBufferPrec]
		fistp	dword ptr[ZVal]

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
		fld1							; 1    UZR  ZRi  VZR  UL   VL
		fdiv	st,st(2)				; ZR   UZR  ZRi  VZR  UL   VL

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

		fstp	[FTemp0]				; UZR  ZRi  VZR  UL   VL
		fstp	[FTemp1]				; ZRi  VZR  UL   VL
		fstp	[FTemp2]				; VZR  UL   VL
		fstp	[FTemp3]				; UL   VL
		fstp	[FTemp4]				; VL
		fstp	[FTemp5]				; 


		fild	dword ptr[RR1]			; LR
		fild	dword ptr[GG1]			; LG   LR
		fild	dword ptr[BB1]			; LB   LG   LR
		
		
		mov		ebx,dword ptr[U1]
		mov		eax,dword ptr[V1]

		add		ebx,dword ptr[UAdjust2]
		add		eax,dword ptr[VAdjust2]

		mov		ecx,[VShift]
		mov		dword ptr[Bucket],ebx

		shl		eax,cl

		push	ebp

		mov		dword ptr[Bucket2],eax
		mov		ebp,dword ptr[DeltaV]

		and		eax,[GHMaskShifted16]
		and		ebx,[GWMaskShifted]

		shl		ebp,cl
		add		eax,ebx

		mov		edx,dword ptr[Bucket2]
		mov		esi,pTex

		shr		eax,16
		mov		dword ptr[DeltaV],ebp

		mov		ebx,dword ptr[Bucket]

		mov		ax,word ptr[2*eax+esi]
		add		edx,dword ptr[DeltaV]

		xor		eax,0
		mov		ecx,edx

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		rol		eax,16
		and		ebx,[GWMaskShifted]

		xor		eax,0
		add		ecx,ebx

		add		edx,dword ptr[DeltaV]
		mov		esi,pTex

		shr		ecx,16
		add		ebx,dword ptr[DeltaU]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,edx

		xor		eax,0
		and		ecx,[GHMaskShifted16]

		mov		esi,eax
		mov		ebp,eax

		and		esi,REDMASK2
		and		ebp,GREENMASK2

		mov		dword ptr[Red],esi
		mov		dword ptr[Green],ebp

		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR
		fxch	st(1)
		fadd	qword ptr[GreenDelta]
		fxch	st(1)
		fadd	qword ptr[BlueDelta]
		fxch	st(2)
		fadd	qword ptr[RedDelta]
		fxch	st(2)

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,[ZVal]

		mov		esi,[ZVal]
		add		ecx,[ZDelta]

		shr		esi,16
		mov		[ZVal],ecx

		mov		ecx,pZBufferPtr

		mov		[edi+0],ebp
		mov		word ptr[ecx+0],si
		mov		word ptr[ecx+2],si

		mov		ecx,edx
		mov		esi,pTex

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		and		ebx,[GWMaskShifted]
		xor		eax,0

		add		ecx,ebx

		rol		eax,16
		add		edx,dword ptr[DeltaV]

		shr		ecx,16
		add		ebx,dword ptr[DeltaU]

		mov		ax,word ptr[2*ecx+esi]

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
/////////////////////////


		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR
		fxch	st(1)
		fadd	qword ptr[GreenDelta]
		fxch	st(1)
		fadd	qword ptr[BlueDelta]
		fxch	st(2)
		fadd	qword ptr[RedDelta]
		fxch	st(2)

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,[ZVal]

		mov		esi,[ZVal]
		add		ecx,[ZDelta]

		shr		esi,16
		mov		[ZVal],ecx

		mov		ecx,pZBufferPtr


		mov		[edi+4],ebp
		mov		word ptr[ecx+4],si
		mov		word ptr[ecx+6],si

		mov		ecx,edx
		mov		esi,pTex

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		and		ebx,[GWMaskShifted]
		xor		eax,0

		add		ecx,ebx

		rol		eax,16
		add		edx,dword ptr[DeltaV]

		shr		ecx,16
		add		ebx,dword ptr[DeltaU]

		mov		ax,word ptr[2*ecx+esi]

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
/////////////////////////


		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR
		fxch	st(1)
		fadd	qword ptr[GreenDelta]
		fxch	st(1)
		fadd	qword ptr[BlueDelta]
		fxch	st(2)
		fadd	qword ptr[RedDelta]
		fxch	st(2)

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,[ZVal]

		mov		esi,[ZVal]
		add		ecx,[ZDelta]

		shr		esi,16
		mov		[ZVal],ecx

		mov		ecx,pZBufferPtr

		mov		[edi+8],ebp
		mov		word ptr[ecx+8],si
		mov		word ptr[ecx+10],si

		mov		ecx,edx
		mov		esi,pTex

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		and		ebx,[GWMaskShifted]
		xor		eax,0

		add		ecx,ebx

		shr		ecx,16
		add		edx,dword ptr[DeltaV]

		rol		eax,16
		mov		ax,word ptr[2*ecx+esi]

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
		add		ebx,dword ptr[DeltaU]
/////////////////////////


		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR
		fxch	st(1)
		fadd	qword ptr[GreenDelta]
		fxch	st(1)
		fadd	qword ptr[BlueDelta]
		fxch	st(2)
		fadd	qword ptr[RedDelta]
		fxch	st(2)

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,[ZVal]

		mov		esi,[ZVal]
		add		ecx,[ZDelta]

		shr		esi,16
		mov		[ZVal],ecx

		mov		ecx,pZBufferPtr

		mov		[edi+12],ebp
		mov		word ptr[ecx+12],si
		mov		word ptr[ecx+14],si

		mov		ecx,edx
		mov		esi,pTex

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		and		ebx,[GWMaskShifted]
		xor		eax,0

		add		ecx,ebx

		shr		ecx,16
		add		edx,dword ptr[DeltaV]

		rol		eax,16
		mov		ax,word ptr[2*ecx+esi]

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
		add		ebx,dword ptr[DeltaU]
/////////////////////////


		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR
		fxch	st(1)
		fadd	qword ptr[GreenDelta]
		fxch	st(1)
		fadd	qword ptr[BlueDelta]
		fxch	st(2)
		fadd	qword ptr[RedDelta]
		fxch	st(2)

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,[ZVal]

		mov		esi,[ZVal]
		add		ecx,[ZDelta]

		shr		esi,16
		mov		[ZVal],ecx

		mov		ecx,pZBufferPtr

		mov		[edi+16],ebp
		mov		word ptr[ecx+16],si
		mov		word ptr[ecx+18],si

		mov		ecx,edx
		mov		esi,pTex

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		and		ebx,[GWMaskShifted]
		xor		eax,0

		add		ecx,ebx

		shr		ecx,16
		add		edx,dword ptr[DeltaV]

		rol		eax,16
		mov		ax,word ptr[2*ecx+esi]

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
		add		ebx,dword ptr[DeltaU]
/////////////////////////


		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR
		fxch	st(1)
		fadd	qword ptr[GreenDelta]
		fxch	st(1)
		fadd	qword ptr[BlueDelta]
		fxch	st(2)
		fadd	qword ptr[RedDelta]
		fxch	st(2)

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,[ZVal]

		mov		esi,[ZVal]
		add		ecx,[ZDelta]

		shr		esi,16
		mov		[ZVal],ecx

		mov		ecx,pZBufferPtr

		mov		[edi+20],ebp
		mov		word ptr[ecx+20],si
		mov		word ptr[ecx+22],si

		mov		ecx,edx
		mov		esi,pTex

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		and		ebx,[GWMaskShifted]
		xor		eax,0

		add		ecx,ebx

		shr		ecx,16
		add		edx,dword ptr[DeltaV]

		rol		eax,16
		mov		ax,word ptr[2*ecx+esi]

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
		add		ebx,dword ptr[DeltaU]
/////////////////////////


		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR
		fxch	st(1)
		fadd	qword ptr[GreenDelta]
		fxch	st(1)
		fadd	qword ptr[BlueDelta]
		fxch	st(2)
		fadd	qword ptr[RedDelta]
		fxch	st(2)

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,[ZVal]

		mov		esi,[ZVal]
		add		ecx,[ZDelta]

		shr		esi,16
		mov		[ZVal],ecx

		mov		ecx,pZBufferPtr

		mov		[edi+24],ebp
		mov		word ptr[ecx+24],si
		mov		word ptr[ecx+26],si

		mov		ecx,edx
		mov		esi,pTex

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		and		ebx,[GWMaskShifted]
		xor		eax,0

		add		ecx,ebx

		shr		ecx,16
		add		edx,dword ptr[DeltaV]

		rol		eax,16
		mov		ax,word ptr[2*ecx+esi]

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
		add		ebx,dword ptr[DeltaU]
/////////////////////////
		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR
		fadd	qword ptr[BlueDelta]
		fistp	dword ptr[BB1]
		fadd	qword ptr[GreenDelta]
		fistp	dword ptr[GG1]
		fadd	qword ptr[RedDelta]
		fistp	dword ptr[RR1]

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		fstp	dword ptr[Bucket]
		fstp	dword ptr[Bucket2]
		fstp	dword ptr[Bucket]

		and		esi,GREENMASK2

		fld		dword ptr[FTemp5]
		fld		dword ptr[FTemp4]

		or		ebp,esi

		fld		dword ptr[FTemp3]
		fld		dword ptr[FTemp2]

		rol		ebp,16

		fld		dword ptr[FTemp1]
		fld		dword ptr[FTemp0]

		mov		ecx,[ZVal]

		mov		esi,[ZVal]
		add		ecx,[ZDelta]

		shr		esi,16
		mov		[ZVal],ecx

		mov		ecx,pZBufferPtr

		mov		[edi+28],ebp
		mov		word ptr[ecx+28],si
		mov		word ptr[ecx+30],si

		pop		ebp


		; get corrected right side deltas ; st0  st1  st2  st3  st4  st5  st6  st7
										; ZR   UZR  ZRi  VZR  UL   VL
		fld		st						; ZR   ZR   UZR  ZRi  VZR  UL   VL
		fmul	st,st(4)				; VR   ZR   UZR  ZRi  VZR  UL   VL
		fxch	st(1)					; ZR   VR   UZR  ZRi  VZR  UL   VL
		fmul	st,st(2)				; UR   VR   UZR  ZRi  VZR  UL   VL

		add		edi,32					; move screen pointer to start of next aspan
		add		[pZBufferPtr],32
		dec		[NumASpans]			; dec num affine spans
		jnz		SpanLoop16

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

OnePixelSpan16:
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

		fstp	[FTemp0]
		fstp	[FTemp1]
		fstp	[FTemp2]
		fstp	[FTemp3]
		fstp	[FTemp4]
		fstp	[FTemp5]

		mov		ebx,dword ptr[U1]
		mov		edx,dword ptr[V1]

		fild	[RR1]					; LR
		fild	[GG1]					; LG   LR
		fild	[BB1]					; LB   LG   LR

		mov		ecx,[VShift]
		add		edx,dword ptr[VAdjust2]

		add		ebx,dword ptr[UAdjust2]
		mov		eax,dword ptr[DeltaV]

		shl		eax,cl
		mov		esi,pTex

		shl		edx,cl
		mov		dword ptr[DeltaV],eax

		mov		eax,[ZDelta]
		mov		ecx,[ZVal]

		sar		eax,1
		push	ebp

		mov		[ZDelta],eax

LeftoverLoop16:
		mov		eax,edx
		and		ebx,[GWMaskShifted]

		and		eax,[GHMaskShifted16]

		add		eax,ebx
		add		ebx,dword ptr[DeltaU]

		shr		eax,16
		add		edi,2

		mov		ax,word ptr[2*eax+esi]
		add		edx,dword ptr[DeltaV]

		xor		eax,0

		mov		esi,eax
		mov		ebp,eax

		and		esi,REDMASK2
		and		ebp,GREENMASK2

		mov		dword ptr[Red],esi
		mov		dword ptr[Green],ebp

		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		and		esi,GREENMASK2
		mov		eax,ecx

		or		ebp,esi

		shr		eax,16
		mov		esi,pZBufferPtr

		mov		word ptr[edi-2],bp
		mov		word ptr[esi],ax

		add		ecx,[ZDelta]
		mov		esi,pTex

		add		pZBufferPtr,2

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
		pop		edi
		pop		esi
		pop		ecx
		pop		ebx
	}
}


void DrawSpan16_AsmLit555X86FPU(int32 x1, int32 x2, int32 y)
{
	_asm
	{
		mov		eax,x1
		mov		ecx,x2
		sub		ecx,eax
		jle		Return16

		mov		edi,[GBitPtr16]
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
		fld1							; 1    UZR  ZRi  VZR  UL   VL
		fdiv	st,st(2)				; ZR   UZR  ZRi  VZR  UL   VL

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


		mov esi,dword ptr[UFixed]
		mov eax,dword ptr[VFixed]


		mov ecx, GMipLevel4_8
		sar esi, cl
		sar eax, cl
		and esi, 0ffh
		and eax, 0ffh
		mov UDist, esi
		mov VDist, eax

		mov esi,dword ptr[UFixed]
		mov eax,dword ptr[VFixed]
		mov ecx, GMipLevel20
		shr esi, cl
		shr eax, cl

		imul eax, GLightWidth
		add esi, eax

		mov edx, esi
		shl esi, 1
		add edx, esi

		add edx, GLightData

		// Interpolate accross top
		xor ecx, ecx
		mov cl, [edx+3]
		mov eax, ecx
		mov cl, [edx+0]
		sub eax, ecx
		imul eax, UDist
		shl ecx, 8
		add eax, ecx
		mov [R1], eax

		xor ecx, ecx
		mov cl, [edx+4]
		mov eax, ecx
		mov cl, [edx+1]
		sub eax, ecx
		imul eax, UDist
		shl ecx, 8
		add eax, ecx
		mov [G1], eax

		xor ecx, ecx
		mov cl, [edx+5]
		mov eax, ecx
		mov cl, [edx+2]
		sub eax, ecx
		imul eax, UDist
		shl ecx, 8
		add eax, ecx
		mov [B1], eax

		add edx, GLightWidth
		add edx, GLightWidth
		add edx, GLightWidth

		// Interpolate accross bottom
		xor ecx, ecx
		mov cl, [edx+3]
		mov eax, ecx
		mov cl, [edx+0]
		sub eax, ecx
		imul eax, UDist
		shl ecx, 8
		add eax, ecx
		mov [R2], eax

		xor ecx, ecx
		mov cl, [edx+4]
		mov eax, ecx
		mov cl, [edx+1]
		sub eax, ecx
		imul eax, UDist
		shl ecx, 8
		add eax, ecx
		mov [G2], eax

		xor ecx, ecx
		mov cl, [edx+5]
		mov eax, ecx
		mov cl, [edx+2]
		sub eax, ecx
		imul eax, UDist
		shl ecx, 8
		add eax, ecx
		mov [B2], eax

		// Interpolate down
		mov eax, [R2]
		sub eax, [R1]
		imul eax, VDist
		sar eax, 8
		add eax, [R1]
		shr eax, 8
		and	eax,0feh

		mov [RR1], eax

		mov eax, [G2]
		sub eax, [G1]
		imul eax, VDist
		sar eax, 8
		add eax, [G1]
		shr eax, 8
		and	eax,0feh

		mov [GG1], eax

		mov eax, [B2]
		sub eax, [B1]
		imul eax, VDist
		sar eax, 8
		add eax, [B1]
		shr eax, 8
		and	eax,0feh

		mov [BB1], eax

		fstp	[FTemp0]				; UZR  ZRi  VZR  UL   VL
		fstp	[FTemp1]				; ZRi  VZR  UL   VL
		fstp	[FTemp2]				; VZR  UL   VL
		fstp	[FTemp3]				; UL   VL
		fstp	[FTemp4]				; VL
		fstp	[FTemp5]				; 

		fild	[RR1]					; LR
		fild	[GG1]					; LG   LR
		fild	[BB1]					; LB   LG   LR
		
		
		mov		ebx,dword ptr[U1]
		mov		eax,dword ptr[V1]

		add		ebx,dword ptr[UAdjust2]
		add		eax,dword ptr[VAdjust2]

		mov		ecx,[VShift]
		mov		dword ptr[Bucket],ebx

		shl		eax,cl

		push	ebp

		mov		dword ptr[Bucket2],eax
		mov		ebp,dword ptr[DeltaV]

		and		eax,[GHMaskShifted16]
		and		ebx,[GWMaskShifted]

		shl		ebp,cl
		add		eax,ebx

		mov		edx,dword ptr[Bucket2]
		mov		esi,pTex

		shr		eax,16
		mov		dword ptr[DeltaV],ebp

		mov		ebx,dword ptr[Bucket]

		mov		ax,word ptr[2*eax+esi]
		add		edx,dword ptr[DeltaV]

		xor		eax,0
		mov		ecx,edx

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		rol		eax,16
		and		ebx,[GWMaskShifted]

		xor		eax,0
		add		ecx,ebx

		add		edx,dword ptr[DeltaV]
		mov		esi,pTex

		shr		ecx,16
		add		ebx,dword ptr[DeltaU]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,edx

		xor		eax,0
		and		ecx,[GHMaskShifted16]

		mov		esi,eax
		mov		ebp,eax

		and		esi,REDMASK2
		and		ebp,GREENMASK2

		mov		dword ptr[Red],esi
		mov		dword ptr[Green],ebp

		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,edx

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		and		ebx,[GWMaskShifted]
		xor		eax,0

		add		ecx,ebx
		mov		[edi],ebp

		rol		eax,16
		add		edx,dword ptr[DeltaV]

		shr		ecx,16
		add		ebx,dword ptr[DeltaU]

		mov		ax,word ptr[2*ecx+esi]

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
/////////////////////////


		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,edx

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		and		ebx,[GWMaskShifted]
		xor		eax,0

		add		ecx,ebx
		mov		[edi+4],ebp

		rol		eax,16
		add		edx,dword ptr[DeltaV]

		shr		ecx,16
		add		ebx,dword ptr[DeltaU]

		mov		ax,word ptr[2*ecx+esi]

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
/////////////////////////


		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,edx

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		and		ebx,[GWMaskShifted]
		xor		eax,0

		add		ecx,ebx
		mov		[edi+8],ebp

		shr		ecx,16
		add		edx,dword ptr[DeltaV]

		rol		eax,16
		mov		ax,word ptr[2*ecx+esi]

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
		add		ebx,dword ptr[DeltaU]
/////////////////////////


		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,edx

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		and		ebx,[GWMaskShifted]
		xor		eax,0

		add		ecx,ebx
		mov		[edi+12],ebp

		shr		ecx,16
		add		edx,dword ptr[DeltaV]

		rol		eax,16
		mov		ax,word ptr[2*ecx+esi]

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
		add		ebx,dword ptr[DeltaU]
/////////////////////////


		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,edx

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		and		ebx,[GWMaskShifted]
		xor		eax,0

		add		ecx,ebx
		mov		[edi+16],ebp

		shr		ecx,16
		add		edx,dword ptr[DeltaV]

		rol		eax,16
		mov		ax,word ptr[2*ecx+esi]

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
		add		ebx,dword ptr[DeltaU]
/////////////////////////


		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,edx

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		and		ebx,[GWMaskShifted]
		xor		eax,0

		add		ecx,ebx
		mov		[edi+20],ebp

		shr		ecx,16
		add		edx,dword ptr[DeltaV]

		rol		eax,16
		mov		ax,word ptr[2*ecx+esi]

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
		add		ebx,dword ptr[DeltaU]
/////////////////////////


		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,edx

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		and		ebx,[GWMaskShifted]
		xor		eax,0

		add		ecx,ebx
		mov		[edi+24],ebp

		shr		ecx,16
		add		edx,dword ptr[DeltaV]

		rol		eax,16
		mov		ax,word ptr[2*ecx+esi]

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
		add		ebx,dword ptr[DeltaU]
/////////////////////////
		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		fstp	dword ptr[Bucket]
		fstp	dword ptr[Bucket2]
		fstp	dword ptr[Bucket]

		and		esi,GREENMASK2

		fld		dword ptr[FTemp5]
		fld		dword ptr[FTemp4]

		or		ebp,esi

		fld		dword ptr[FTemp3]
		fld		dword ptr[FTemp2]

		rol		ebp,16

		fld		dword ptr[FTemp1]
		fld		dword ptr[FTemp0]

		mov		[edi+28],ebp				; store pixel 0
		pop		ebp


		; get corrected right side deltas ; st0  st1  st2  st3  st4  st5  st6  st7
										; ZR   UZR  ZRi  VZR  UL   VL
		fld		st						; ZR   ZR   UZR  ZRi  VZR  UL   VL
		fmul	st,st(4)				; VR   ZR   UZR  ZRi  VZR  UL   VL
		fxch	st(1)					; ZR   VR   UZR  ZRi  VZR  UL   VL
		fmul	st,st(2)				; UR   VR   UZR  ZRi  VZR  UL   VL

		add		edi,32					; move screen pointer to start of next aspan
		dec		[NumASpans]			; dec num affine spans
		jnz		SpanLoop16

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

OnePixelSpan16:
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
		mov		esi,dword ptr[UFixed]
		mov		eax,dword ptr[VFixed]

		mov ecx, GMipLevel4_8
		sar esi, cl
		sar eax, cl
		and esi, 0ffh
		and eax, 0ffh
		mov UDist, esi
		mov VDist, eax

		mov esi,dword ptr[UFixed]
		mov eax,dword ptr[VFixed]
		mov ecx, GMipLevel20
		shr esi, cl
		shr eax, cl

		imul eax, GLightWidth
		add esi, eax

		mov edx, esi
		shl esi, 1
		add edx, esi

		add edx, GLightData

		// Interpolate accross top
		xor ecx, ecx
		mov cl, [edx+3]
		mov eax, ecx
		mov cl, [edx+0]
		sub eax, ecx
		imul eax, UDist
		shl ecx, 8
		add eax, ecx
		mov [R1], eax

		xor ecx, ecx
		mov cl, [edx+4]
		mov eax, ecx
		mov cl, [edx+1]
		sub eax, ecx
		imul eax, UDist
		shl ecx, 8
		add eax, ecx
		mov [G1], eax

		xor ecx, ecx
		mov cl, [edx+5]
		mov eax, ecx
		mov cl, [edx+2]
		sub eax, ecx
		imul eax, UDist
		shl ecx, 8
		add eax, ecx
		mov [B1], eax

		add edx, GLightWidth
		add edx, GLightWidth
		add edx, GLightWidth

		// Interpolate accross bottom
		xor ecx, ecx
		mov cl, [edx+3]
		mov eax, ecx
		mov cl, [edx+0]
		sub eax, ecx
		imul eax, UDist
		shl ecx, 8
		add eax, ecx
		mov [R2], eax

		xor ecx, ecx
		mov cl, [edx+4]
		mov eax, ecx
		mov cl, [edx+1]
		sub eax, ecx
		imul eax, UDist
		shl ecx, 8
		add eax, ecx
		mov [G2], eax

		xor ecx, ecx
		mov cl, [edx+5]
		mov eax, ecx
		mov cl, [edx+2]
		sub eax, ecx
		imul eax, UDist
		shl ecx, 8
		add eax, ecx
		mov [B2], eax

		// Interpolate down
		mov eax, [R2]
		sub eax, [R1]
		imul eax, VDist
		sar eax, 8
		add eax, [R1]
		shr eax, 8
		and	eax,0feh

		mov [RR1], eax

		mov eax, [G2]
		sub eax, [G1]
		imul eax, VDist
		sar eax, 8
		add eax, [G1]
		shr eax, 8
		and	eax,0feh

		mov [GG1], eax

		mov eax, [B2]
		sub eax, [B1]
		imul eax, VDist
		sar eax, 8
		add eax, [B1]
		shr eax, 8
		and	eax,0feh

		mov [BB1], eax

		fstp	[FTemp0]
		fstp	[FTemp1]
		fstp	[FTemp2]
		fstp	[FTemp3]
		fstp	[FTemp4]
		fstp	[FTemp5]

		mov		ebx,dword ptr[U1]
		mov		edx,dword ptr[V1]

		fild	[RR1]					; LR
		fild	[GG1]					; LG   LR
		fild	[BB1]					; LB   LG   LR

		mov		ecx,[VShift]
		add		edx,dword ptr[VAdjust2]

		add		ebx,dword ptr[UAdjust2]
		mov		eax,dword ptr[DeltaV]

		shl		eax,cl
		mov		esi,pTex

		shl		edx,cl
		mov		dword ptr[DeltaV],eax


		push	ebp

LeftoverLoop16:
		mov		eax,edx
		and		ebx,[GWMaskShifted]

		and		eax,[GHMaskShifted16]

		add		eax,ebx
		add		ebx,dword ptr[DeltaU]

		shr		eax,16
		add		edi,2

		mov		ax,word ptr[2*eax+esi]
		add		edx,dword ptr[DeltaV]

		xor		eax,0

		mov		esi,eax
		mov		ebp,eax

		and		esi,REDMASK2
		and		ebp,GREENMASK2

		mov		dword ptr[Red],esi
		mov		dword ptr[Green],ebp

		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		and		esi,GREENMASK2

		or		ebp,esi

		mov		esi,pTex

		mov		word ptr[edi-2],bp

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

void DrawSpan16_AsmGouraudZBufferTrans555X86FPU(int32 x1, int32 x2, int32 y, int32 r1, int32 g1, int32 b1, int32 r2, int32 g2, int32 b2)
{
	RR1	=r1;
	GG1	=g1;
	BB1	=b1;
	TDest	=Dest;
	_asm
	{
		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		eax,x1
		mov		ecx,x2
		sub		ecx,eax
		jle		Return16

		mov		edi,[GBitPtr16]
		mov		pTex,edi

		fild	[y]						; y
		mov		[widTemp],ecx

		fild	[x1]						; x    y
		fild	[widTemp]

		fld1
		fdivrp	st(1),st

		mov		esi,x1
		mov		edi,[TDest]

		shl		esi,1
		mov		eax,ecx

		add		edi,esi
		add		pZBufferPtr,esi

		mov		eax,ecx
		shr		ecx,4
		and		eax,15
		_emit 75h
		_emit 06h
		dec		ecx
		mov		eax,16

		mov		[NumASpans],ecx
		mov		[RemainingCount],eax

		fild	[r2]
		fisub	[r1]
		fild	[g2]
		fisub	[g1]
		fild	[b2]
		fisub	[b1]
		fxch	st(2)
		fmul	st,st(3)
		fxch	st(1)
		fmul	st,st(3)
		fxch	st(2)
		fmul	st,st(3)
		frndint
		fstp	qword ptr[BlueDelta]
		frndint
		fstp	qword ptr[RedDelta]
		frndint
		fstp	qword ptr[GreenDelta]
		fstp	qword ptr[FTemp0]


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

		//zbuffer step action
		fld		[ZiStepX]
		fmul	dword ptr[ZBufferPrec]
		fmul	dword ptr[Two]
		fistp	dword ptr[ZDelta]

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

		//zbuffer action
		fld		st
		fmul	dword ptr[ZBufferPrec]
		fistp	dword ptr[ZVal]

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
		fld1							; 1    UZR  ZRi  VZR  UL   VL
		fdiv	st,st(2)				; ZR   UZR  ZRi  VZR  UL   VL

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

		fstp	[FTemp0]				; UZR  ZRi  VZR  UL   VL
		fstp	[FTemp1]				; ZRi  VZR  UL   VL
		fstp	[FTemp2]				; VZR  UL   VL
		fstp	[FTemp3]				; UL   VL
		fstp	[FTemp4]				; VL
		fstp	[FTemp5]				; 


		fild	[RR1]					; LR
		fild	[GG1]					; LG   LR
		fild	[BB1]					; LB   LG   LR
		
		
		mov		ebx,dword ptr[U1]
		mov		eax,dword ptr[V1]

		add		ebx,dword ptr[UAdjust2]
		add		eax,dword ptr[VAdjust2]

		mov		ecx,[VShift]
		mov		dword ptr[Bucket],ebx

		shl		eax,cl

		push	ebp

		mov		dword ptr[Bucket2],eax
		mov		ebp,dword ptr[DeltaV]

		and		eax,[GHMaskShifted16]
		and		ebx,[GWMaskShifted]

		shl		ebp,cl
		add		eax,ebx

		mov		edx,dword ptr[Bucket2]
		mov		esi,pTex

		shr		eax,16
		mov		dword ptr[DeltaV],ebp

		mov		ebx,dword ptr[Bucket]

		mov		ax,word ptr[2*eax+esi]
		add		edx,dword ptr[DeltaV]

		xor		eax,0
		mov		ecx,edx

		mov		ebp,eax
		add		ebx,dword ptr[DeltaU]

		shr		ebp,1
		and		ecx,[GHMaskShifted16]

		rol		eax,16
		mov		[CKeyTest],ebp

		and		ebx,[GWMaskShifted]

		xor		eax,0
		add		ecx,ebx

		add		edx,dword ptr[DeltaV]
		mov		esi,pTex

		shr		ecx,16
		add		ebx,dword ptr[DeltaU]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,edx

		xor		eax,0
		and		ecx,[GHMaskShifted16]

		mov		esi,eax
		mov		ebp,eax

		and		esi,REDMASK2
		and		ebp,GREENMASK2

		mov		dword ptr[Red],esi
		mov		dword ptr[Green],ebp

		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax
		shr		eax,1

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		[CKeyTest],eax

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR
		fxch	st(1)
		fadd	qword ptr[GreenDelta]
		fxch	st(1)
		fadd	qword ptr[BlueDelta]
		fxch	st(2)
		fadd	qword ptr[RedDelta]
		fxch	st(2)

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,[ZVal]

		mov		esi,[ZVal]
		add		ecx,[ZDelta]

		shr		esi,16
		mov		[ZVal],ecx

		dec		[CKeyTest]
		jl		Skip0

		mov		ecx,pZBufferPtr

		cmp		si,word ptr[ecx+0]
		jle		Skip0

		mov		[edi+0],ebp
		mov		word ptr[ecx+0],si
		mov		word ptr[ecx+2],si

Skip0:
		mov		ecx,edx

		mov		esi,pTex
		xor		eax,0

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		mov		ebp,eax
		and		ebx,[GWMaskShifted]

		shr		ebp,1
		add		ecx,ebx

		rol		eax,16
		add		edx,dword ptr[DeltaV]

		shr		ecx,16
		add		ebx,dword ptr[DeltaU]

		mov		ax,word ptr[2*ecx+esi]
		mov		[CKeyTest],ebp

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
/////////////////////////


		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax
		shr		eax,1

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		[CKeyTest],eax

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR
		fxch	st(1)
		fadd	qword ptr[GreenDelta]
		fxch	st(1)
		fadd	qword ptr[BlueDelta]
		fxch	st(2)
		fadd	qword ptr[RedDelta]
		fxch	st(2)

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,[ZVal]

		mov		esi,[ZVal]
		add		ecx,[ZDelta]

		shr		esi,16
		mov		[ZVal],ecx

		dec		[CKeyTest]
		jl		Skip1

		mov		ecx,pZBufferPtr

		cmp		si,word ptr[ecx+4]
		jle		Skip1

		mov		[edi+4],ebp
		mov		word ptr[ecx+4],si
		mov		word ptr[ecx+6],si

Skip1:
		mov		ecx,edx

		mov		esi,pTex
		xor		eax,0

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		mov		ebp,eax
		and		ebx,[GWMaskShifted]

		shr		ebp,1
		add		ecx,ebx

		rol		eax,16
		add		edx,dword ptr[DeltaV]

		shr		ecx,16
		add		ebx,dword ptr[DeltaU]

		mov		ax,word ptr[2*ecx+esi]
		mov		[CKeyTest],ebp

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
/////////////////////////


		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax
		shr		eax,1

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		[CKeyTest],eax

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR
		fxch	st(1)
		fadd	qword ptr[GreenDelta]
		fxch	st(1)
		fadd	qword ptr[BlueDelta]
		fxch	st(2)
		fadd	qword ptr[RedDelta]
		fxch	st(2)

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,[ZVal]

		mov		esi,[ZVal]
		add		ecx,[ZDelta]

		shr		esi,16
		mov		[ZVal],ecx

		dec		[CKeyTest]
		jl		Skip2

		mov		ecx,pZBufferPtr

		cmp		si,word ptr[ecx+8]
		jle		Skip2

		mov		[edi+8],ebp
		mov		word ptr[ecx+8],si
		mov		word ptr[ecx+10],si

Skip2:
		mov		ecx,edx

		mov		esi,pTex
		xor		eax,0

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		mov		ebp,eax
		and		ebx,[GWMaskShifted]

		shr		ebp,1
		add		ecx,ebx

		rol		eax,16
		add		edx,dword ptr[DeltaV]

		shr		ecx,16
		add		ebx,dword ptr[DeltaU]

		mov		ax,word ptr[2*ecx+esi]
		mov		[CKeyTest],ebp

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
		add		ebx,dword ptr[DeltaU]
/////////////////////////


		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax
		shr		eax,1

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		[CKeyTest],eax

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR
		fxch	st(1)
		fadd	qword ptr[GreenDelta]
		fxch	st(1)
		fadd	qword ptr[BlueDelta]
		fxch	st(2)
		fadd	qword ptr[RedDelta]
		fxch	st(2)

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,[ZVal]

		mov		esi,[ZVal]
		add		ecx,[ZDelta]

		shr		esi,16
		mov		[ZVal],ecx

		dec		[CKeyTest]
		jl		Skip3

		mov		ecx,pZBufferPtr

		cmp		si,word ptr[ecx+12]
		jle		Skip3

		mov		[edi+12],ebp
		mov		word ptr[ecx+12],si
		mov		word ptr[ecx+14],si

Skip3:
		mov		ecx,edx

		mov		esi,pTex
		xor		eax,0

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		mov		ebp,eax
		and		ebx,[GWMaskShifted]

		shr		ebp,1
		add		ecx,ebx

		rol		eax,16
		add		edx,dword ptr[DeltaV]

		shr		ecx,16
		add		ebx,dword ptr[DeltaU]

		mov		ax,word ptr[2*ecx+esi]
		mov		[CKeyTest],ebp

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
		add		ebx,dword ptr[DeltaU]
/////////////////////////


		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax
		shr		eax,1

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		[CKeyTest],eax

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR
		fxch	st(1)
		fadd	qword ptr[GreenDelta]
		fxch	st(1)
		fadd	qword ptr[BlueDelta]
		fxch	st(2)
		fadd	qword ptr[RedDelta]
		fxch	st(2)

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,[ZVal]

		mov		esi,[ZVal]
		add		ecx,[ZDelta]

		shr		esi,16
		mov		[ZVal],ecx

		dec		[CKeyTest]
		jl		Skip4

		mov		ecx,pZBufferPtr

		cmp		si,word ptr[ecx+16]
		jle		Skip4

		mov		[edi+16],ebp
		mov		word ptr[ecx+16],si
		mov		word ptr[ecx+18],si

Skip4:
		mov		ecx,edx

		mov		esi,pTex
		xor		eax,0

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		mov		ebp,eax
		and		ebx,[GWMaskShifted]

		shr		ebp,1
		add		ecx,ebx

		rol		eax,16
		add		edx,dword ptr[DeltaV]

		shr		ecx,16
		add		ebx,dword ptr[DeltaU]

		mov		ax,word ptr[2*ecx+esi]
		mov		[CKeyTest],ebp

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
		add		ebx,dword ptr[DeltaU]
/////////////////////////


		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax
		shr		eax,1

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		[CKeyTest],eax

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR
		fxch	st(1)
		fadd	qword ptr[GreenDelta]
		fxch	st(1)
		fadd	qword ptr[BlueDelta]
		fxch	st(2)
		fadd	qword ptr[RedDelta]
		fxch	st(2)

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,[ZVal]

		mov		esi,[ZVal]
		add		ecx,[ZDelta]

		shr		esi,16
		mov		[ZVal],ecx

		dec		[CKeyTest]
		jl		Skip5

		mov		ecx,pZBufferPtr

		cmp		si,word ptr[ecx+20]
		jle		Skip5

		mov		[edi+20],ebp
		mov		word ptr[ecx+20],si
		mov		word ptr[ecx+22],si

Skip5:
		mov		ecx,edx

		mov		esi,pTex
		xor		eax,0

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		mov		ebp,eax
		and		ebx,[GWMaskShifted]

		shr		ebp,1
		add		ecx,ebx

		rol		eax,16
		add		edx,dword ptr[DeltaV]

		shr		ecx,16
		add		ebx,dword ptr[DeltaU]

		mov		ax,word ptr[2*ecx+esi]
		mov		[CKeyTest],ebp

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
		add		ebx,dword ptr[DeltaU]
/////////////////////////


		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax
		shr		eax,1

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		[CKeyTest],eax

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR
		fxch	st(1)
		fadd	qword ptr[GreenDelta]
		fxch	st(1)
		fadd	qword ptr[BlueDelta]
		fxch	st(2)
		fadd	qword ptr[RedDelta]
		fxch	st(2)

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		add		ecx,ebx
		and		esi,GREENMASK2

		shr		ecx,16
		or		ebp,esi

		xor		eax,0
		mov		esi,pTex

		rol		ebp,16
		add		edx,dword ptr[DeltaV]

		mov		ax,word ptr[2*ecx+esi]
		mov		ecx,[ZVal]

		mov		esi,[ZVal]
		add		ecx,[ZDelta]

		shr		esi,16
		mov		[ZVal],ecx

		dec		[CKeyTest]
		jl		Skip6

		mov		ecx,pZBufferPtr

		cmp		si,word ptr[ecx+24]
		jle		Skip6

		mov		[edi+24],ebp
		mov		word ptr[ecx+24],si
		mov		word ptr[ecx+26],si

Skip6:
		mov		ecx,edx

		mov		esi,pTex
		xor		eax,0

		add		ebx,dword ptr[DeltaU]
		and		ecx,[GHMaskShifted16]

		mov		ebp,eax
		and		ebx,[GWMaskShifted]

		shr		ebp,1
		add		ecx,ebx

		rol		eax,16
		add		edx,dword ptr[DeltaV]

		shr		ecx,16
		add		ebx,dword ptr[DeltaU]

		mov		ax,word ptr[2*ecx+esi]
		mov		[CKeyTest],ebp

		mov		ecx,edx
		xor		eax,0

		and		ecx,[GHMaskShifted16]
		mov		esi,eax

		mov		ebp,eax
		and		esi,REDMASK2

		and		ebp,GREENMASK2
		mov		dword ptr[Red],esi

		mov		dword ptr[Green],ebp
		add		ebx,dword ptr[DeltaU]
/////////////////////////
		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax
		shr		eax,1

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		[CKeyTest],eax

		and		ebp,BLUEMASK
		and		ebx,[GWMaskShifted]

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR
		fadd	qword ptr[BlueDelta]
		fistp	dword ptr[BB1]
		fadd	qword ptr[GreenDelta]
		fistp	dword ptr[GG1]
		fadd	qword ptr[RedDelta]
		fistp	dword ptr[RR1]

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		fstp	dword ptr[Bucket]
		fstp	dword ptr[Bucket2]
		fstp	dword ptr[Bucket]

		and		esi,GREENMASK2

		fld		dword ptr[FTemp5]
		fld		dword ptr[FTemp4]

		or		ebp,esi

		fld		dword ptr[FTemp3]
		fld		dword ptr[FTemp2]

		rol		ebp,16

		fld		dword ptr[FTemp1]
		fld		dword ptr[FTemp0]

		mov		eax,[CKeyTest]
		mov		ecx,[ZVal]

		mov		esi,[ZVal]
		add		ecx,[ZDelta]

		shr		esi,16
		mov		[ZVal],ecx

		dec		eax
		jl		Skip7

		mov		ecx,pZBufferPtr

		cmp		si,word ptr[ecx+28]
		jle		Skip7

		mov		[edi+28],ebp
		mov		word ptr[ecx+28],si
		mov		word ptr[ecx+30],si

Skip7:
		pop		ebp


		; get corrected right side deltas ; st0  st1  st2  st3  st4  st5  st6  st7
										; ZR   UZR  ZRi  VZR  UL   VL
		fld		st						; ZR   ZR   UZR  ZRi  VZR  UL   VL
		fmul	st,st(4)				; VR   ZR   UZR  ZRi  VZR  UL   VL
		fxch	st(1)					; ZR   VR   UZR  ZRi  VZR  UL   VL
		fmul	st,st(2)				; UR   VR   UZR  ZRi  VZR  UL   VL

		add		edi,32					; move screen pointer to start of next aspan
		add		[pZBufferPtr],32
		dec		[NumASpans]			; dec num affine spans
		jnz		SpanLoop16

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

OnePixelSpan16:
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

		fstp	[FTemp0]
		fstp	[FTemp1]
		fstp	[FTemp2]
		fstp	[FTemp3]
		fstp	[FTemp4]
		fstp	[FTemp5]

		mov		ebx,dword ptr[U1]
		mov		edx,dword ptr[V1]

		fild	[RR1]					; LR
		fild	[GG1]					; LG   LR
		fild	[BB1]					; LB   LG   LR

		mov		ecx,[VShift]
		add		edx,dword ptr[VAdjust2]

		add		ebx,dword ptr[UAdjust2]
		mov		eax,dword ptr[DeltaV]

		shl		eax,cl
		mov		esi,pTex

		shl		edx,cl
		mov		dword ptr[DeltaV],eax

		mov		eax,[ZDelta]
		mov		ecx,[ZVal]

		sar		eax,1
		push	ebp

		mov		[ZDelta],eax

LeftoverLoop16:
		mov		eax,edx
		and		ebx,[GWMaskShifted]

		and		eax,[GHMaskShifted16]

		add		eax,ebx
		add		ebx,dword ptr[DeltaU]

		shr		eax,16
		add		edi,2

		mov		ax,word ptr[2*eax+esi]
		add		edx,dword ptr[DeltaV]

		xor		eax,0

		mov		esi,eax
		mov		ebp,eax

		and		esi,REDMASK2
		and		ebp,GREENMASK2

		mov		dword ptr[Red],esi
		mov		dword ptr[Green],ebp

		fild	qword ptr[Red]			; r    LB   LG   LR

		mov		ebp,eax
		shr		eax,1

		fmul	st,st(3)				; R    LB   LG   LR
		fild	qword ptr[Green]		; g    R    LB   LG   LR

		and		ebp,BLUEMASK
		mov		[CKeyTest],eax

		mov		dword ptr[Blue],ebp

		fmul	st,st(3)				; G    R    LB   LG   LR
		fild	[Blue]					; b    G    R    LB   LG   LR
		fmul	st,st(3)				; B    G    R    LB   LG   LR
		fxch	st(2)					; R    G    B    LB   LG   LR
		fadd	qword ptr[Magic]		; Rk   G    B    LB   LG   LR
		fxch	st(1)					; G    Rk   B    LB   LG   LR
		fadd	qword ptr[Magic]		; Gk   Rk   B    LB   LG   LR
		fxch	st(2)					; B    Rk   Gk   LB   LG   LR
		fadd	qword ptr[Magic]		; Bk   Rk   Gk   LB   LG   LR
		fxch	st(1)					; Rk   Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket]		; Bk   Gk   LB   LG   LR
		fstp	qword ptr[Bucket2]		; Gk   LB   LG   LR

		mov		eax,dword ptr[Bucket]
		mov		ebp,dword ptr[Bucket2]

		fstp	qword ptr[Bucket]		; LB   LG   LR

		and		eax,REDMASK2
		and		ebp,BLUEMASK
		
		mov		esi,dword ptr[Bucket]
		or		ebp,eax

		and		esi,GREENMASK2
		mov		eax,ecx

		or		ebp,esi

		shr		eax,16
		mov		esi,pZBufferPtr

		dec		[CKeyTest]
		jl		SkipLeftOver

		cmp		ax,word ptr[esi]
		jle		SkipLeftOver

		mov		word ptr[edi-2],bp
		mov		word ptr[esi],ax

SkipLeftOver:
		add		ecx,[ZDelta]
		mov		esi,pTex

		add		pZBufferPtr,2

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
		pop		edi
		pop		esi
		pop		ecx
		pop		ebx
	}
}

