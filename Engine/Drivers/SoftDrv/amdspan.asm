;****************************************************************************************/
;*  amdspan.asm                                                                         */
;*                                                                                      */
;*  Author:       Ken Baird                                                             */
;*  Description:  3dnow render assembly                                                 */
;*                                                                                      */
;*  The contents of this file are subject to the Genesis3D Public License               */
;*  Version 1.01 (the "License"); you may not use this file except in                   */
;*  compliance with the License. You may obtain a copy of the License at                */
;*  http://www.genesis3d.com                                                            */
;*                                                                                      */
;*  Software distributed under the License is distributed on an "AS IS"                 */
;*  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See                */
;*  the License for the specific language governing rights and limitations              */
;*  under the License.                                                                  */
;*                                                                                      */
;*  The Original Code is Genesis3D, released March 25, 1999.                            */
;*  Copyright (C) 1999 WildTangent Inc All Rights Reserved								*/
;*                                                                                      */
;****************************************************************************************/
TITLE   amdspan.asm
.586
.K3D
.model FLAT,C
option language:c

assume ds:FLAT,es:FLAT,ss:FLAT
assume fs:nothing,gs:nothing

OPTION OLDSTRUCTS


EXTERNDEF GBitPtr:DWORD
EXTERNDEF ABitPtr:DWORD
EXTERNDEF SolidColor:DWORD
EXTERNDEF pTex:DWORD
EXTERNDEF ClientWindow:DWORD
EXTERNDEF Dest:DWORD
EXTERNDEF NumASpans:DWORD
EXTERNDEF RemainingCount:DWORD
EXTERNDEF UDivZStepX:DWORD
EXTERNDEF VDivZStepX:DWORD
EXTERNDEF ZiStepX:DWORD
EXTERNDEF UDivZStepY:DWORD
EXTERNDEF VDivZStepY:DWORD
EXTERNDEF ZiStepY:DWORD
EXTERNDEF UDivZ16StepX:DWORD
EXTERNDEF VDivZ16StepX:DWORD
EXTERNDEF Zi16StepX:DWORD
EXTERNDEF ZiOrigin:DWORD
EXTERNDEF FloatTemp:DWORD
EXTERNDEF GLMapMulU:DWORD
EXTERNDEF UAdjust:DWORD
EXTERNDEF UAdjustL:DWORD
EXTERNDEF TexPal:DWORD
EXTERNDEF ATexPal:DWORD
EXTERNDEF U1:DWORD
EXTERNDEF UFixed:DWORD
EXTERNDEF MaxU:DWORD
EXTERNDEF MaxV:DWORD
EXTERNDEF QFixedScaleLUT:DWORD
EXTERNDEF GMipLevel4_8:DWORD
EXTERNDEF GMipLevel20:DWORD
EXTERNDEF GLMapAdd:DWORD
EXTERNDEF GLightWidth:DWORD
EXTERNDEF GLightData:DWORD
EXTERNDEF ZBuffer:DWORD
EXTERNDEF Zero:QWORD
EXTERNDEF UV16:QWORD
EXTERNDEF UVLeft:QWORD
EXTERNDEF UVLeft2:QWORD
EXTERNDEF UVLeftW:QWORD
EXTERNDEF UVDivZ16StepX:QWORD
EXTERNDEF UVDivZStepX:QWORD
EXTERNDEF UVDivZStepY:QWORD
EXTERNDEF ARL:QWORD
EXTERNDEF GBL:QWORD
EXTERNDEF Q128:QWORD
EXTERNDEF WrapMask:QWORD
EXTERNDEF QFixedScale16:QWORD
EXTERNDEF QFixedScale:QWORD
EXTERNDEF UVDivZOrigin:QWORD
EXTERNDEF UVR:QWORD
EXTERNDEF ZIR:QWORD
EXTERNDEF UVZ:QWORD
EXTERNDEF GLMapMulUV:QWORD
EXTERNDEF UVL16:QWORD
EXTERNDEF UV162:QWORD
EXTERNDEF UV16V:QWORD
EXTERNDEF QShiftV:QWORD
EXTERNDEF LMapMask8:QWORD
EXTERNDEF UVAdjustL:QWORD
EXTERNDEF UVAdjust:QWORD
EXTERNDEF UVAdjust2:QWORD
EXTERNDEF QGMip20:QWORD
EXTERNDEF QGMip4_8:QWORD
EXTERNDEF QDibCan:QWORD
EXTERNDEF QZCan:QWORD
EXTERNDEF QDibOrCan:QWORD
EXTERNDEF QZOrCan:QWORD
EXTERNDEF QZVal:QWORD
EXTERNDEF QZDelta:QWORD
EXTERNDEF QZOut:QWORD
EXTERNDEF QDibOut:QWORD
EXTERNDEF SCan:QWORD
EXTERNDEF QZVal32_0:QWORD
EXTERNDEF QZVal32_1:QWORD
EXTERNDEF QZBufferPrec:QWORD
EXTERNDEF pZBufferPtr:DWORD
EXTERNDEF QNegAlpha:QWORD
EXTERNDEF RGBADelta:QWORD
EXTERNDEF VertAlpha:QWORD

DRV_Window struc
	hWnd dd ?
	Buffer dd ?
	BWidth dd ?
	Height dd ?
	PixelPitch dd ?
	BytesPerPixel dd ?
	R_shift dd ?
	G_shift dd ?
	B_shift dd ?
	R_mask dd ?
	G_mask dd ?
	B_mask dd ?
	R_width dd ?
	G_width dd ?
	B_width dd ?
DRV_Window ends

EdgeAsm struc
	X		dd ?
	yf		dd ?
	Height	dd ?
	xf		dd ?
	uf		dd ?
	vf		dd ?
	zf		dd ?
	rf		dd ?
	gf		dd ?
	bf		dd ?
	xstep	dd ?
	ustep	dd ?
	vstep	dd ?
	zstep	dd ?
	rstep	dd ?
	gstep	dd ?
	bstep	dd ?
EdgeAsm ends

;include listing.inc
;include amd3d.inc
include stdcall.inc



_TEXT$01   SEGMENT PARA USE32 PUBLIC 'CODE'
           ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING



;here's the standard 32 bit pal based combine with lightmap polydraw
;it's your basic subdivided affine mapper with an mmx combine
;no z operations in this loop
;prefetching is useless
;you might think doing pmulhw is a better idea rather than the
;additional shifts it takes to use pmullw...
;trouble is the lack of unsigned word ops

cProc DrawSpan32_AsmLit3DNow, 12,<x1 : dword, x2 : dword, y : dword>

		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		eax,x1
		mov		ecx,x2
		sub		ecx,eax
		jle		ReturnLit


		mov		eax, y

		mov		edx,x1
		mov		ebx,offset ClientWindow

		shl		edx, 2
		imul	eax, [ebx].PixelPitch

		mov		edi,[ebx].Buffer

		add		edx,eax
		mov		eax,ecx

		add		edi, edx

		shr		ecx,4
		and		eax,15
		jnz		@f
		dec		ecx
		mov		eax,16
@@:

		mov		[NumASpans],ecx		
		mov		[RemainingCount],eax

;		prefetch	[GBitPtr]
										;   mm0      mm1      mm2      mm3      mm4      mm5      mm6      mm7
		movd		mm0,x1				;    |x       |        |        |        |        |        |        |
		movq		mm2,[UVDivZStepX]	;    |x       |        |UZdX    |        |        |        |        |

		movd		mm1,y				;    |x       |y       |UZdX    |        |        |        |        |
		movq		mm3,[UVDivZStepY]	;    |x       |y       |UZdX    |UZdY    |        |        |        |

		punpckldq	mm0,mm0				;   x|x       |y       |UZdX    |UZdY    |        |        |        |
		punpckldq	mm1,mm1				;   x|x      y|y   UZdX|VZdX    |UZdY    |        |        |        |

;		prefetch	[GBitPtr+32]

		pi2fd		mm0,mm0
		pi2fd		mm1,mm1

		pfmul		mm2,mm0				;   x|x      y|y    UZX|VZX UZdY|VZdY    |        |        |        |
		movd		mm4,[ZiStepX]		;   x|x      y|y    UZX|VZX UZdY|VZdY    |ZdX     |        |        |

		pfmul		mm3,mm1				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZdX     |        |        |
		movq		mm6,[UVDivZOrigin]	;   x|x      y|y    UZX|VZX  UZY|VZY     |ZdX     |        |UZO     |

;		prefetch	[GBitPtr+64]

		movd		mm5,[ZiStepY]		;   x|x      y|y    UZX|VZX  UZY|VZY     |ZdX     |ZdY     |UZO     |
		pfmul		mm4,mm0				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX      |ZdY     |UZO     |

		pfmul		mm5,mm1				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX      |ZY   UZO|VZO     |
		movd		mm7,[ZiOrigin]

		pfadd		mm6,mm2				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX      |ZY  UZXS|VZXS    |
		pfadd		mm4,mm7				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZXS     |ZY  UZXS|VZXS    |

;		prefetch	[GBitPtr+96]

		pfadd		mm6,mm3				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZXS     |ZY    UZ|VZ      |
		pfadd		mm4,mm5				;   x|x      y|y    UZX|VZX  UZY|VZY     |Zi      |ZY    UZ|VZ      |

		movd		mm7,[Zi16StepX]

		pfrcp		mm0,mm4				;  ZL|ZL     y|y    UZX|VZX  UZY|VZY     |Zi      |ZY    UZ|VZ      |
		movq		mm1,mm6				;  ZL|ZL    UZ|VZ   UZX|VZX  UZY|VZY     |Zi      |ZY    UZ|VZ      |

		pfadd		mm4,mm7				;  ZL|ZL    UZ|VZ   UZX|VZX  UZY|VZY     |ZRi     |ZY    UZ|VZ      |
		pfmul		mm1,mm0				;  ZL|ZL    UL|VL   UZX|VZX  UZY|VZY     |ZRi     |ZY    UZ|VZ      |

;		prefetch	[GBitPtr+128]

		pfrcp		mm2,mm4				;  ZL|ZL    UL|VL    ZR|ZR   UZY|VZY     |ZRi     |ZY    UZ|VZ      |
		pfadd		mm6,[UVDivZ16StepX] ;  ZL|ZL    UL|VL    ZR|ZR   UZY|VZY     |ZRi     |ZY   UZR|VZR     |

		pfmul		mm2,mm6				;  ZL|ZL    UL|VL    UR|VR   UZY|VZY     |ZRi     |ZY   UZR|VZR     |
		movq		mm0,mm1				;  UL|VL    UL|VL    UR|VR   UZY|VZY     |ZRi     |ZY   UZR|VZR     |

		movq		mm3,mm2				; ULi|VLi	UL|VL    UR|UR    UR|UR      |ZRi     |ZY   UZR|VZR     |
		pfmul		mm0,[QFixedScale]	; UL6|VL6	UL|VL    UR|UR    UR|UR      |ZRi     |ZY   UZR|VZR     |

		pfmul		mm3,[QFixedScale]	; UL6|VL6	UL|VL   UR6|UR6  UR6|UR6     |ZRi     |ZY   UZR|VZR     |
		pf2id		mm0,mm0				;UL6i|VL6i  UL|VL   UR6|VR6  UR6|VR6     |ZRi     |ZY   UZR|VZR     |

		pf2id		mm3,mm3				;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR     |
;		prefetch	[GBitPtr+160]

		test	ecx,ecx
		jz		HandleLeftoverPixelsLit

		movq		mm7,[GLMapMulUV]	;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR   LU|LV
		pfmul		mm7,mm1				;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR  ULL|VLL
		pfmul		mm7,[QFixedScale]
		pf2id		mm7,mm7				; ULw|VLw   DU|DV    UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |
		paddd		mm7,[UVAdjustL]
		movq		[UVL16],mm7

		; Clamp U/V
		mov		ebx,dword ptr[UVL16+4]

;		prefetch	[GBitPtr+192]

		cmp		ebx,MaxU
		jle		TryClampU0Litp
		mov		ecx,MaxU
		mov		dword ptr[UVL16+4],ecx
		jmp		NoClampU0Litp

TryClampU0Litp:
		cmp		ebx,0
		jge		NoClampU0Litp
		mov		dword ptr[UVL16+4],0
NoClampU0Litp:
		mov		eax,dword ptr[UVL16]
		cmp		eax,MaxV
		jle		TryClampV0Litp
		mov		ecx,MaxV
		mov		dword ptr[UVL16],ecx
		jmp		NoClampV0Litp

TryClampV0Litp:
		cmp		eax,0
		jge		NoClampV0Litp
		mov		dword ptr[UVL16],0

NoClampV0Litp:

;		prefetch	[GBitPtr+224]

		movq		mm7,[UVL16]
		movq		mm5,[UVL16]

		movq		[UVLeftW],mm3
		psrad		mm7,[QGMip4_8]

		psrld		mm5,[QGMip20]
		pand		mm7,[LMapMask8]

		movq		[UVL16],mm5
		movq		mm3,mm7

		punpckhwd	mm7,mm7
		mov			eax,dword ptr[UVL16]

;		prefetch	[GLightData]
		punpckldq	mm7,mm7

		movq		mm5,[Zero]
		imul		eax,[GLightWidth]

		movq		[UVZ],mm6
		punpcklwd	mm3,mm3

		add			eax,dword ptr[UVL16+4]
		movq		mm6,[Zero]

		lea			eax,[2*eax+eax]

		punpckldq	mm3,mm3
		add			eax,[GLightData]

		;bilininterpolate to get good color
		punpcklbw	mm6,[eax+3]
		mov			ecx,[GLightWidth]

		punpcklbw	mm5,[eax]
		psrlw		mm6,8

		add			eax,ecx
		psrlw		mm5,8

		movq		[UVLeft],mm2
		psubw		mm6,mm5

		movq		mm2,[Zero]
		psllw		mm5,8

		add			eax,ecx
		pmullw		mm6,mm7

		add			eax,ecx
		paddw		mm6,mm5

		movq		mm5,[Zero]
		punpcklbw	mm2,[eax+3]

		punpcklbw	mm5,[eax]
		psrlw		mm2,8
;		prefetch	[GBitPtr+256]

		psrlw		mm5,8
		psubw		mm2,mm5

		psllw		mm5,8

		pmullw		mm2,mm7
		movq		mm7,mm6

		paddw		mm2,mm5
		psrlw		mm6,8

		psrlw		mm2,8

		pfrcp		mm5,mm4

		psubw		mm2,mm6

		pmullw		mm2,mm3

		movq		mm3,[UVLeftW]

		paddw		mm7,mm2

		psrlw		mm7,8

		movq		mm2,mm7
		movq		mm6,[UVZ]

		psllq		mm2,16

		punpckhwd	mm7,mm2

		psrlq		mm2,16

		punpckldq	mm7,mm2

		psllq		mm7,8
		movq		mm2,[UVLeft]

		movq		[ARL],mm7


SpanLoopLit:
		;use float uv for lightmap uv
		movq		mm7,[GLMapMulUV]	;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR   LU|LV
		movq		mm5,mm1

		pfmul		mm7,mm2				;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR  ULL|VLL
		pfmul		mm5,[QFixedScale]

		pfmul		mm7,[QFixedScale]
		pf2id		mm5,mm5

		pf2id		mm7,mm7				; ULw|VLw   DU|DV    UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |
		paddd		mm5,[UVAdjust]

		pfsubr		mm1,mm2				; ULw|VLw   DU|DV    UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |
		paddd		mm5,[UVAdjust2]

		movq		[UVL16],mm7
		movq		[UVLeft],mm5

		movd		mm7,[Zi16StepX]
		movq		mm0,mm5

		pfmul		mm1,[QFixedScale16]	; ULw|VLw DU16|DV16  UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |
		;right side stuff becomes left	; ULw|VLw DU16|DV16  UL|VL   URw|VRw     |ZLi     |ZY   UZL|VZL     |
		pfadd		mm4,mm7				;ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZL|VZL     |

		pand		mm5,[WrapMask]
		pf2id		mm1,mm1				; ULw|VLw DU16|DV16  UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |

		movq		mm7,[UVAdjustL]
		movq		[UV16],mm5

		pfadd		mm6,[UVDivZ16StepX]	; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |
		paddd		mm7,[UVL16]

		psrlq		mm5,[QShiftV]
		movq		[UVL16],mm7

		movq		[UV16V],mm5


		; Clamp U/V
		mov		ebx,dword ptr[UVL16+4]
		cmp		ebx,MaxU
		jle		TryClampU0Lit
		mov		ecx,MaxU
		mov		dword ptr[UVL16+4],ecx
		jmp		NoClampU0Lit

TryClampU0Lit:
		cmp		ebx,0
		jge		NoClampU0Lit
		mov		dword ptr[UVL16+4],0
NoClampU0Lit:
		mov		eax,dword ptr[UVL16]
		cmp		eax,MaxV
		jle		TryClampV0Lit
		mov		ecx,MaxV
		mov		dword ptr[UVL16],ecx
		jmp		NoClampV0Lit

TryClampV0Lit:
		cmp		eax,0
		jge		NoClampV0Lit
		mov		dword ptr[UVL16],0

NoClampV0Lit:
		movq	mm7,[UVL16]
		movq	mm5,[UVL16]

		movq	[UVLeftW],mm3
		psrad	mm7,[QGMip4_8]

		psrld	mm5,[QGMip20]
		pand	mm7,[LMapMask8]

		movq	[UVL16],mm5
		movq	mm3,mm7

		punpckhwd	mm7,mm7
		mov		eax,dword ptr[UVL16]

		movq	mm5,[Zero]
		imul	eax,[GLightWidth]

		movq	[UVZ],mm6
		punpcklwd	mm3,mm3

		add		eax,dword ptr[UVL16+4]
		movq	mm6,[Zero]

		punpckldq	mm7,mm7
		lea		eax,[2*eax+eax]

		punpckldq	mm3,mm3
		add		eax,[GLightData]

		mov			ecx,[GLightWidth]

		;bilininterpolate to get good color
		punpcklbw	mm6,[eax+3]
		punpcklbw	mm5,[eax]

		psrlw		mm6,8
		add			eax,ecx

		psrlw		mm5,8

		movq		[UVLeft],mm2
		psubw		mm6,mm5

		movq		mm2,[Zero]
		psllw		mm5,8

		add			eax,ecx
		pmullw		mm6,mm7

		add			eax,ecx
		paddw		mm6,mm5

		movq		mm5,[Zero]
		punpcklbw	mm2,[eax+3]

		punpcklbw	mm5,[eax]
		psrlw		mm2,8

		psrlw		mm5,8
		mov			eax,dword ptr[UV16V]

		psubw		mm2,mm5

		mov			ebx,dword ptr[UV16+4]
		psllw		mm5,8

		mov			esi,GBitPtr
		pmullw		mm2,mm7

		paddw		mm2,mm5
		movq		mm7,mm6

		psrlw		mm2,8
		psrlw		mm6,8

		add			esi,eax
		shr			ebx,16

		psubw		mm2,mm6
		add			esi,ebx

		movq		[ZIR],mm4
		pmullw		mm2,mm3

		xor			eax,eax
		mov			ecx,TexPal

		paddw		mm7,mm2

		pfrcp		mm5,mm4

		psrlw		mm7,8

		movq		mm2,mm7

		psllq		mm2,16

		punpckhwd	mm7,mm2

		psrlq		mm2,16

		punpckldq	mm7,mm2

		movq		mm4,[ARL]
		psllq		mm7,8

		movq		mm6,[ARL]
		psrlw		mm4,4

		movq		[ARL],mm7
		movq		mm3,mm7

		psrlw		mm3,4

		psubw		mm3,mm4

;		movq		mm3,[Zero]


		paddd		mm0,mm1				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |

		mov			al,byte ptr[esi]
		movq		mm4,mm0				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw   UL|VL      |ZY   UZR|VZR     |

		mov			esi,GBitPtr

		pand		mm4,[WrapMask]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movq		[UVR],mm5

		movq		[UV16],mm4			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |
		psrlq		mm4,[QShiftV]

		mov			ebx,dword ptr[UV16+4]
		movq		[UV16V],mm4

		mov			edx,dword ptr[UV16V]
		movq		mm2,mm6

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |ARGB
		psrlw		mm2,8

		shr			ebx,16
		add			esi,ebx

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		paddd		mm0,mm1

		add			esi,edx
		pmullw		mm7,mm2

		xor			eax,eax
		movq		mm4,mm0

		psrlw		mm7,8

		mov			al,byte ptr[esi]
		paddw		mm6,mm3

		pand		mm4,[WrapMask]
		movq		mm2,mm6

		movd		mm5,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		psrlw		mm2,8

		movq		[UV16],mm4			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |
		punpcklbw	mm5,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB

		psrlq		mm4,[QShiftV]
		mov			ebx,dword ptr[UV16+4]

		movq		[UV16V],mm4
		paddw		mm6,mm3

		mov			edx,dword ptr[UV16V]
		paddd		mm0,mm1

		pmullw		mm5,mm2
		mov			esi,GBitPtr

		movq		mm2,mm6
		psrlw		mm5,8


		shr			ebx,16
		psrlw		mm2,8

		add			esi,edx
		movq		mm4,mm0

		packuswb	mm7,mm5				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw ARGB|ARGB UZR|VZR   AR|GB
		add			esi,ebx

		movq		[edi],mm7
		xor			eax,eax

		pand		mm4,[WrapMask]

		mov			al,byte ptr[esi]
		movq		[UV16],mm4

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB
		mov			esi,GBitPtr

		mov			ebx,dword ptr[UV16+4]
		psrlq		mm4,[QShiftV]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		paddd		mm0,mm1

		movq		[UV16V],mm4
		paddw		mm6,mm3

		pmullw		mm7,mm2
		xor			eax,eax

		mov			edx,dword ptr[UV16V]
		movq		mm2,mm6

		psrlw		mm7,8
		shr			ebx,16

		movq		mm4,mm0
		add			esi,edx

		pand		mm4,[WrapMask]
		add			esi,ebx

		mov			al,byte ptr[esi]
		psrlw		mm2,8

		movq		[UV16],mm4			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movd		mm5,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		mov			esi,GBitPtr

		punpcklbw	mm5,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		paddd		mm0,mm1

		psrlq		mm4,[QShiftV]
		paddw		mm6,mm3

		movq		[UV16V],mm4
		pmullw		mm5,mm2

		mov			ebx,dword ptr[UV16+4]
		movq		mm2,mm6

		psrlw		mm5,8

		mov			edx,dword ptr[UV16V]
		movq		mm4,mm0

		shr			ebx,16
		pand		mm4,[WrapMask]

		add			esi,edx
		packuswb	mm7,mm5

		movq		[UV16],mm4
		add			esi,ebx

		movq		[edi+8],mm7
		xor			eax,eax

		psrlq		mm4,[QShiftV]
		psrlw		mm2,8

		mov			al,byte ptr[esi]
		mov			esi,GBitPtr

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB

		mov			ebx,dword ptr[UV16+4]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		paddd		mm0,mm1

		movq		[UV16V],mm4
		paddw		mm6,mm3

		pmullw		mm7,mm2
		xor			eax,eax

		mov			edx,dword ptr[UV16V]
		movq		mm2,mm6

		psrlw		mm7,8

		shr			ebx,16
		movq		mm4,mm0

		add			esi,edx
		psrlw		mm2,8

		pand		mm4,[WrapMask]
		add			esi,ebx

		mov			al,byte ptr[esi]
		movq		[UV16],mm4			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movd		mm5,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		mov			esi,GBitPtr

		punpcklbw	mm5,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		paddd		mm0,mm1

		psrlq		mm4,[QShiftV]
		paddw		mm6,mm3

		movq		[UV16V],mm4
		pmullw		mm5,mm2

		mov			ebx,dword ptr[UV16+4]
		movq		mm2,mm6

		psrlw		mm5,8

		mov			edx,dword ptr[UV16V]
		movq		mm4,mm0

		shr			ebx,16
		pand		mm4,[WrapMask]

		add			esi,edx
		packuswb	mm7,mm5

		movq		[UV16],mm4
		add			esi,ebx

		movq		[edi+16],mm7
		xor			eax,eax

		psrlq		mm4,[QShiftV]
		psrlw		mm2,8

		mov			al,byte ptr[esi]
		mov			esi,GBitPtr

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB

		mov			ebx,dword ptr[UV16+4]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		paddd		mm0,mm1

		movq		[UV16V],mm4
		paddw		mm6,mm3

		pmullw		mm7,mm2
		xor			eax,eax

		mov			edx,dword ptr[UV16V]
		movq		mm2,mm6

		psrlw		mm7,8

		shr			ebx,16
		movq		mm4,mm0

		add			esi,edx
		psrlw		mm2,8

		pand		mm4,[WrapMask]
		add			esi,ebx

		mov			al,byte ptr[esi]
		movq		[UV16],mm4			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movd		mm5,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		mov			esi,GBitPtr

		punpcklbw	mm5,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		paddd		mm0,mm1

		psrlq		mm4,[QShiftV]
		paddw		mm6,mm3

		movq		[UV16V],mm4
		pmullw		mm5,mm2

		mov			ebx,dword ptr[UV16+4]
		movq		mm2,mm6

		psrlw		mm5,8

		mov			edx,dword ptr[UV16V]
		movq		mm4,mm0

		shr			ebx,16
		pand		mm4,[WrapMask]

		add			esi,edx
		packuswb	mm7,mm5

		movq		[UV16],mm4
		add			esi,ebx

		movq		[edi+24],mm7
		xor			eax,eax

		psrlq		mm4,[QShiftV]
		psrlw		mm2,8

		mov			al,byte ptr[esi]
		mov			esi,GBitPtr

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB

		mov			ebx,dword ptr[UV16+4]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		paddd		mm0,mm1

		movq		[UV16V],mm4
		paddw		mm6,mm3

		pmullw		mm7,mm2
		xor			eax,eax

		mov			edx,dword ptr[UV16V]
		movq		mm2,mm6

		psrlw		mm7,8

		shr			ebx,16
		movq		mm4,mm0

		add			esi,edx
		psrlw		mm2,8

		pand		mm4,[WrapMask]
		add			esi,ebx

		mov			al,byte ptr[esi]
		movq		[UV16],mm4			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movd		mm5,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		mov			esi,GBitPtr

		punpcklbw	mm5,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		paddd		mm0,mm1

		psrlq		mm4,[QShiftV]
		paddw		mm6,mm3

		movq		[UV16V],mm4
		pmullw		mm5,mm2

		mov			ebx,dword ptr[UV16+4]
		movq		mm2,mm6

		psrlw		mm5,8

		mov			edx,dword ptr[UV16V]
		movq		mm4,mm0

		shr			ebx,16
		pand		mm4,[WrapMask]

		add			esi,edx
		packuswb	mm7,mm5

		movq		[UV16],mm4
		add			esi,ebx

		movq		[edi+32],mm7
		xor			eax,eax

		psrlq		mm4,[QShiftV]
		psrlw		mm2,8

		mov			al,byte ptr[esi]
		mov			esi,GBitPtr

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB

		mov			ebx,dword ptr[UV16+4]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		paddd		mm0,mm1

		movq		[UV16V],mm4
		paddw		mm6,mm3

		pmullw		mm7,mm2
		xor			eax,eax

		mov			edx,dword ptr[UV16V]
		movq		mm2,mm6

		psrlw		mm7,8

		shr			ebx,16
		movq		mm4,mm0

		add			esi,edx

		pand		mm4,[WrapMask]
		add			esi,ebx

		mov			al,byte ptr[esi]
		movq		[UV16],mm4			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movd		mm5,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		mov			esi,GBitPtr

		psrlw		mm2,8

		punpcklbw	mm5,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		paddd		mm0,mm1

		psrlq		mm4,[QShiftV]
		paddw		mm6,mm3

		movq		[UV16V],mm4
		pmullw		mm5,mm2

		mov			ebx,dword ptr[UV16+4]
		movq		mm2,mm6

		psrlw		mm5,8

		mov			edx,dword ptr[UV16V]
		movq		mm4,mm0

		shr			ebx,16
		pand		mm4,[WrapMask]

		add			esi,edx
		packuswb	mm7,mm5

		movq		[UV16],mm4
		add			esi,ebx
		psrlw		mm2,8

		movq		[edi+40],mm7
		xor			eax,eax

		psrlq		mm4,[QShiftV]

		mov			al,byte ptr[esi]
		mov			esi,GBitPtr

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB
		paddw		mm6,mm3

		mov			ebx,dword ptr[UV16+4]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		paddd		mm0,mm1

		movq		[UV16V],mm4

		pmullw		mm7,mm2
		xor			eax,eax

		mov			edx,dword ptr[UV16V]
		movq		mm2,mm6

		psrlw		mm7,8

		shr			ebx,16
		movq		mm4,mm0

		add			esi,edx
		psrlw		mm2,8

		pand		mm4,[WrapMask]
		add			esi,ebx

		mov			al,byte ptr[esi]
		movq		[UV16],mm4			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movd		mm5,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		mov			esi,GBitPtr

		punpcklbw	mm5,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		paddd		mm0,mm1

		psrlq		mm4,[QShiftV]
		paddw		mm6,mm3

		movq		[UV16V],mm4
		pmullw		mm5,mm2

		mov			ebx,dword ptr[UV16+4]
		movq		mm2,mm6

		psrlw		mm5,8

		mov			edx,dword ptr[UV16V]
		movq		mm4,mm0

		shr			ebx,16
		psrlw		mm2,8

		pand		mm4,[WrapMask]

		add			esi,edx
		packuswb	mm7,mm5

		movq		[UV16],mm4
		add			esi,ebx

		movq		[edi+48],mm7
		xor			eax,eax

		psrlq		mm4,[QShiftV]

		mov			al,byte ptr[esi]
		mov			esi,GBitPtr

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB

		mov			ebx,dword ptr[UV16+4]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB

		movq		[UV16V],mm4
		paddw		mm6,mm3

		pmullw		mm7,mm2
		xor			eax,eax

		mov			edx,dword ptr[UV16V]
		movq		mm2,mm6

		psrlw		mm7,8

		shr			ebx,16

		add			esi,edx
		psrlw		mm2,8

		add			esi,ebx

		mov			al,byte ptr[esi]

		movd		mm5,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB

		punpcklbw	mm5,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB

		pmullw		mm5,mm2
		psrlw		mm5,8

		packuswb	mm7,mm5

		movq		[edi+56],mm7

		; get corrected right side deltas ; ULw|VLw DU16|DV16 argb|ARGBagAG|rbRB    |ZRi   aA|rR   UZR|VZR aArR|gGbB

		movq		mm4,[ZIR]
		movq		mm6,[UVZ]

		movq		mm2,[UVR]
		movq		mm0,[UVLeftW]		; ULw|VLw DU16|DV16 argb|ARGBagAG|rbRB    |ZRi   aA|rR   UZR|VZR aArR|gGbB

		movq		mm1,[UVLeft]		; ULw|VLw   UL|VL   argb|ARGBagAG|rbRB ZRi|ZRi   aA|rR   UZR|VZR aArR|gGbB
		punpckldq	mm2,mm2				; ULw|VLw   UL|VL    ZRi|ZRi agAG|rbRB ZRi|ZRi   aA|rR   UZR|VZR aArR|gGbB

		pfmul		mm2,mm6				; ULw|VLw   UL|VL     UR|VR  agAG|rbRB ZRi|ZRi   aA|rR   UZR|VZR aArR|gGbB

		add		edi,64					; move screen pointer to start of next aspan

		movq		mm3,mm2
		pfmul		mm3,[QFixedScale]

		pf2id		mm3,mm3

		dec		[NumASpans]			; dec num affine spans
		jnz		SpanLoopLit

HandleLeftoverPixelsLit:


		mov		esi,GBitPtr

		cmp			[RemainingCount],0
		jz			ReturnLit

		mov			eax,[RemainingCount]
		mov			dword ptr[ZIR],eax
		mov			dword ptr[ZIR+4],eax
		
		movq		mm7,[GLMapMulUV]	;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR   LU|LV
		movq		mm5,mm1

		pfsub		mm6,[UVDivZ16StepX]
		pfmul		mm7,mm1				;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR  ULL|VLL

		pfmul		mm5,[QFixedScale]
		pi2fd		mm3,[ZIR]

		pfmul		mm7,[QFixedScale]
		pfmul		mm3,[UVDivZStepX]

		pf2id		mm5,mm5
		pfadd		mm3,mm6

		pi2fd		mm2,[ZIR]
		movd		mm6,[Zi16StepX]

		pf2id		mm7,mm7				; ULw|VLw   DU|DV    UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |
		pfsub		mm4,mm6

		movd		mm6,[ZiStepX]
		mov			ebx,[RemainingCount]

		pfmul		mm2,mm6

		shl			ebx,3

		pfadd		mm2,mm4
		paddd		mm5,[UVAdjust]

		movq		[UVL16],mm7
		pfrcp		mm2,mm2

		paddd		mm5,[UVAdjust2]
		pfmul		mm2,mm3

		movq		[UVLeft],mm5
		pfsubr		mm1,mm2				; ULw|VLw   DU|DV    UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |

		pfmul		mm1,qword ptr[QFixedScaleLUT+ebx]	; ULw|VLw DU16|DV16  UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |

		movq		mm0,mm5
		pand		mm5,[WrapMask]
		pf2id		mm1,mm1				; ULw|VLw DU16|DV16  UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |

		movq		[UV16],mm5
		movq		mm7,[UVAdjustL]

		psrlq		mm5,[QShiftV]
		paddd		mm7,[UVL16]

		movq		[UV16V],mm5
		movq		[UVL16],mm7


OnePixelSpanLit:
		; Clamp U/V
		mov		ebx,dword ptr[UVL16+4]
		cmp		ebx,MaxU
		jle		TryClampU1Lit
		mov		ecx,MaxU
		mov		dword ptr[UVL16+4],ecx
		jmp		NoClampU1Lit

TryClampU1Lit:
		cmp		ebx,0
		jge		NoClampU1Lit
		mov		dword ptr[UVL16+4],0
NoClampU1Lit:
		mov		eax,dword ptr[UVL16]
		cmp		eax,MaxV
		jle		TryClampV1Lit
		mov		ecx,MaxV
		mov		dword ptr[UVL16],ecx
		jmp		NoClampV1Lit

TryClampV1Lit:
		cmp		eax,0
		jge		NoClampV1Lit
		mov		dword ptr[UVL16],0

NoClampV1Lit:
		movq	mm7,[UVL16]
		movq	mm5,[UVL16]

		psrad	mm7,[QGMip4_8]
		psrld	mm5,[QGMip20]

		pand	mm7,[LMapMask8]
		movq	[UVL16],mm5

		movq	mm3,mm7
		punpckhwd	mm7,mm7

		mov		eax,dword ptr[UVL16]
		movq	mm5,[Zero]

		imul	eax,[GLightWidth]
		punpcklwd	mm3,mm3

		add		eax,dword ptr[UVL16+4]
		movq	mm6,[Zero]

		punpckldq	mm7,mm7
		lea		eax,[2*eax+eax]

		punpckldq	mm3,mm3
		add		eax,[GLightData]

		;bilininterpolate to get good color
		punpcklbw	mm6,[eax+3]
		mov			ecx,[GLightWidth]

		psrlw		mm6,8
		punpcklbw	mm5,[eax]

		add			eax,ecx
		psrlw		mm5,8

		movq		mm2,[Zero]
		psubw		mm6,mm5

		add			eax,ecx
		psllw		mm5,8

		add			eax,ecx
		pmullw		mm6,mm7		; B|B

		punpcklbw	mm2,[eax+3]
		paddw		mm6,mm5

		psrlw		mm2,8
		movq		mm5,[Zero]

		mov			ebx,dword ptr[UV16+4]
		punpcklbw	mm5,[eax]

		mov			eax,dword ptr[UV16V]
		psrlw		mm5,8

		mov			esi,GBitPtr
		psubw		mm2,mm5

		shr			ebx,16
		psllw		mm5,8

		add			esi,eax
		pmullw		mm2,mm7

		movq		mm7,mm6
		paddw		mm2,mm5

		psrlw		mm6,8
		psrlw		mm2,8

		mov			ecx,TexPal
		psubw		mm2,mm6

		add			esi,ebx
		pmullw		mm2,mm3

		xor			eax,eax
		paddw		mm7,mm2

		psrlw		mm7,8

		movq		mm2,mm7		;make ABGR ARGB

		psllq		mm2,16

		punpckhwd	mm7,mm2			;BAGB

		psrlq		mm2,16

		punpckldq	mm7,mm2

LeftoverLoopLit:
		paddd		mm0,mm1				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |

		mov			al,byte ptr[esi]
		movq		mm4,mm0				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw   UL|VL      |ZY   UZR|VZR     |

		mov			esi,GBitPtr

		pand		mm4,[WrapMask]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movd		mm6,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |ARGB

		movq		[UV16],mm4			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		punpcklbw	mm6,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB

		mov			ebx,dword ptr[UV16+4]

		psrlq		mm4,[QShiftV]
		pmullw		mm6,mm7

		shr			ebx,16

		movq		[UV16V],mm4
		add			esi,ebx

		psrlw		mm6,8

		mov			edx,dword ptr[UV16V]
		add			esi,edx
		packuswb	mm6,mm6

		xor			eax,eax
		movd		[edi],mm6

		add			edi,4

		dec		[RemainingCount]
		jge		LeftoverLoopLit

ReturnLit:

	pop		edi
	pop		esi
	pop		ecx
	pop		ebx

	cRet DrawSpan32_AsmLit3DNow
endProc DrawSpan32_AsmLit3DNow



;32 bit gouraud perspective mapper
;this needs serious cleaning... the clamping is useless
;this big stack hurts

cProc DrawSpan32_AsmGouraud3DNow, 36,<x1 : dword, x2 : dword, y : dword, r1 : dword, g1 : dword, b1 : dword, r2 : dword, g2 : dword, b2 : dword>

		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		eax,x1
		mov		ecx,x2
		sub		ecx,eax
		jle		ReturnLit

;		femms

;		inc		ecx
		mov		eax, y
		mov		edx,x1
		mov		ebx,offset ClientWindow
		shl		edx, 2
		imul	eax, [ebx].PixelPitch
		mov		edi,[ebx].Buffer
		add		edx,eax
		mov		eax,ecx
		add		edi, edx
		mov		edx,ecx

		shr		ecx,4
		and		eax,15
		jnz		@f
		dec		ecx
		mov		eax,16
@@:

		mov		[NumASpans],ecx
		mov		[RemainingCount],eax

		;grab the left side lights
		movd		mm5,r1
		movd		mm4,b1

;		punpckldq	mm5,qword ptr[Zero]
		movd		mm6,g1

		pf2id		mm5,mm5
		punpckldq	mm4,mm6

		pf2id		mm4,mm4
		packssdw	mm4,mm5

		psllw		mm4,7

		movq		[ARL],mm4

										;   mm0      mm1      mm2      mm3      mm4      mm5      mm6      mm7
		movd		mm0,x1				;    |x       |        |        |        |        |        |        |
		movq		mm2,[UVDivZStepX]	;    |x       |    UZdX|VZdX    |        |        |        |        |

		movd		mm1,y				;    |x       |y   UZdX|VZdX    |        |        |        |        |
		movq		mm3,[UVDivZStepY]	;    |x       |y   UZdX|VZdXUZdY|VZdY    |        |        |        |

		punpckldq	mm0,mm0				;   x|x       |y   UZdX|VZdXUZdY|VZdY    |        |        |        |
		punpckldq	mm1,mm1				;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |        |        |        |

		movd		mm7,edx				;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |        |        |        |wid
		movd		mm5,b2				;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |        |b       |        |wid

		pi2fd		mm0,mm0				;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |        |b       |        |wid
		movd		mm6,b1				;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |        |b       |b       |wid

		pi2fd		mm7,mm7				;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |        |b       |b       |wid
		punpckldq	mm5,qword ptr[g2]	;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |       g|b       |b       |wid

		pi2fd		mm1,mm1				;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |       g|b       |b       |wid
		punpckldq	mm6,qword ptr[g1]	;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |       g|b      g|b       |wid

		pfmul		mm2,mm0				;   x|x      y|y    UZX|VZX UZdY|VZdY    |       g|b      g|b       |wid
		pfrcp		mm7,mm7				;   x|x      y|y    UZX|VZX UZdY|VZdY    |       g|b      g|b     dw|dw

		pfsub		mm5,mm6				;   x|x      y|y    UZX|VZX UZdY|VZdY    |      gd|bd     g|b     dw|dw
		movd		mm4,[r1]			;   x|x      y|y    UZX|VZX UZdY|VZdY    |r     gd|bd     g|b     dw|dw

		movd		mm6,[r2]			;   x|x      y|y    UZX|VZX UZdY|VZdY    |r     gd|bd     g|r     dw|dw
		pfmul		mm3,mm1				;   x|x      y|y    UZX|VZX  UZY|VZY     |r     gd|bd     g|r     dw|dw

		pfsub		mm6,mm4				;   x|x      y|y    UZX|VZX  UZY|VZY     |r     gd|bd     x|rd    dw|dw
		pfmul		mm7,[Q128]			;   x|x      y|y    UZX|VZX  UZY|VZY     |r     gd|bd     x|rd    DW|DW

		movd		mm4,[ZiStepX]		;   x|x      y|y    UZX|VZX  UZY|VZY     |ZdX   gd|bd     x|rd    DW|DW
		pfmul		mm5,mm7				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZdX   GD|BD     x|rd    DW|DW

		pfmul		mm4,mm0				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX    GD|BD     x|rd    DW|DW
		pfmul		mm6,mm7				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX    GD|BD     x|RD    DW|DW

		pf2id		mm5,mm5
		pf2id		mm6,mm6

		packssdw	mm5,mm6
		movq		[RGBADelta],mm5

		movd		mm5,[ZiStepY]		;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX      |ZdY    x|RD    DW|DW
		movq		mm6,[UVDivZOrigin]	;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX      |ZdY  UZO|VZO   DW|DW

		pfmul		mm5,mm1				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX      |ZY   UZO|VZO   DW|DW
		movd		mm7,[ZiOrigin]		;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX      |ZY   UZO|VZO     |ZO

		pfadd		mm6,mm2				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX      |ZY  UZXS|VZXS    |ZO
		pfadd		mm4,mm7				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZXS     |ZY  UZXS|VZXS    |ZO

		pfadd		mm6,mm3				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZXS     |ZY    UZ|VZ      |ZO
		pfadd		mm4,mm5				;   x|x      y|y    UZX|VZX  UZY|VZY     |Zi      |ZY    UZ|VZ      |ZO

		movd		mm7,[Zi16StepX]		;   x|x      y|y    UZX|VZX  UZY|VZY     |Zi      |ZY    UZ|VZ      |ZdX16
		pfrcp		mm0,mm4				;  ZL|ZL     y|y    UZX|VZX  UZY|VZY     |Zi      |ZY    UZ|VZ      |ZdX16

		movq		mm1,mm6				;  ZL|ZL    UZ|VZ   UZX|VZX  UZY|VZY     |Zi      |ZY    UZ|VZ      |ZdX16
		pfadd		mm4,mm7				;  ZL|ZL    UZ|VZ   UZX|VZX  UZY|VZY     |ZRi     |ZY    UZ|VZ      |ZdX16

		pfmul		mm1,mm0				;  ZL|ZL    UL|VL   UZX|VZX  UZY|VZY     |ZRi     |ZY    UZ|VZ      |ZdX16
		pfrcp		mm2,mm4				;  ZL|ZL    UL|VL    ZR|ZR   UZY|VZY     |ZRi     |ZY    UZ|VZ      |ZdX16

		pfadd		mm6,[UVDivZ16StepX] ;  ZL|ZL    UL|VL    ZR|ZR   UZY|VZY     |ZRi     |ZY   UZR|VZR     |ZdX16
		pfmul		mm2,mm6				;  ZL|ZL    UL|VL    UR|VR   UZY|VZY     |ZRi     |ZY   UZR|VZR     |ZdX16

		movq		mm0,mm1				;  UL|VL    UL|VL    UR|VR   UZY|VZY     |ZRi     |ZY   UZR|VZR     |ZdX16
		movq		mm3,mm2				; ULi|VLi	UL|VL    UR|UR    UR|UR      |ZRi     |ZY   UZR|VZR     |ZdX16

		pfmul		mm0,[QFixedScale]	; UL6|VL6	UL|VL    UR|UR    UR|UR      |ZRi     |ZY   UZR|VZR     |ZdX16
		pfmul		mm3,[QFixedScale]	; UL6|VL6	UL|VL   UR6|UR6  UR6|UR6     |ZRi     |ZY   UZR|VZR     |ZdX16

		pf2id		mm0,mm0				;UL6i|VL6i  UL|VL   UR6|VR6  UR6|VR6     |ZRi     |ZY   UZR|VZR     |ZdX16
		pf2id		mm3,mm3				;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR     |ZdX16


		test	ecx,ecx
		jz		HandleLeftoverPixelsLit

		movq		mm7,[GLMapMulUV]	;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR   LU|LV
		pfmul		mm7,mm1				;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR  ULL|VLL
		pfmul		mm7,[QFixedScale]
		pf2id		mm7,mm7				; ULw|VLw   DU|DV    UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |
		paddd		mm7,[UVAdjustL]
		movq		[UVL16],mm7

		; Clamp U/V
		mov		ebx,dword ptr[UVL16+4]

		cmp		ebx,MaxU
		jle		TryClampU0Litp
		mov		ecx,MaxU
		mov		dword ptr[UVL16+4],ecx
		jmp		NoClampU0Litp

TryClampU0Litp:
		cmp		ebx,0
		jge		NoClampU0Litp
		mov		dword ptr[UVL16+4],0
NoClampU0Litp:
		mov		eax,dword ptr[UVL16]
		cmp		eax,MaxV
		jle		TryClampV0Litp
		mov		ecx,MaxV
		mov		dword ptr[UVL16],ecx
		jmp		NoClampV0Litp

TryClampV0Litp:
		cmp		eax,0
		jge		NoClampV0Litp
		mov		dword ptr[UVL16],0

NoClampV0Litp:

		pfrcp		mm5,mm4

SpanLoopLit:
		;use float uv for lightmap uv
		movq		mm7,[GLMapMulUV]	;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR   LU|LV
		movq		mm5,mm1

		pfmul		mm7,mm2				;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR  ULL|VLL
		pfmul		mm5,[QFixedScale]

		pfmul		mm7,[QFixedScale]
		pf2id		mm5,mm5

		pf2id		mm7,mm7				; ULw|VLw   DU|DV    UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |
		paddd		mm5,[UVAdjust]

		pfsubr		mm1,mm2				; ULw|VLw   DU|DV    UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |
		paddd		mm5,[UVAdjust2]

		movq		[UVL16],mm7
		movq		[UVLeft],mm5

		movd		mm7,[Zi16StepX]
		movq		mm0,mm5

		pfmul		mm1,[QFixedScale16]	; ULw|VLw DU16|DV16  UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |
		;right side stuff becomes left	; ULw|VLw DU16|DV16  UL|VL   URw|VRw     |ZLi     |ZY   UZL|VZL     |
		pfadd		mm4,mm7				;ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZL|VZL     |

		pand		mm5,[WrapMask]
		pf2id		mm1,mm1				; ULw|VLw DU16|DV16  UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |

		movq		mm7,[UVAdjustL]
		movq		[UV16],mm5

		pfadd		mm6,[UVDivZ16StepX]	; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |
		paddd		mm7,[UVL16]

		psrlq		mm5,[QShiftV]
		movq		[UVL16],mm7

		movq		[UV16V],mm5


		; Clamp U/V
		mov		ebx,dword ptr[UVL16+4]
		cmp		ebx,MaxU
		jle		TryClampU0Lit
		mov		ecx,MaxU
		mov		dword ptr[UVL16+4],ecx
		jmp		NoClampU0Lit

TryClampU0Lit:
		cmp		ebx,0
		jge		NoClampU0Lit
		mov		dword ptr[UVL16+4],0
NoClampU0Lit:
		mov		eax,dword ptr[UVL16]
		cmp		eax,MaxV
		jle		TryClampV0Lit
		mov		ecx,MaxV
		mov		dword ptr[UVL16],ecx
		jmp		NoClampV0Lit

TryClampV0Lit:
		cmp		eax,0
		jge		NoClampV0Lit
		mov		dword ptr[UVL16],0

NoClampV0Lit:
		movq		[UVLeftW],mm3
		mov			eax,dword ptr[UV16V]
		mov			ebx,dword ptr[UV16+4]
		movq		[ZIR],mm4
		mov			esi,GBitPtr
		shr			ebx,16
		movq		[UVZ],mm6
		add			esi,eax
		add			esi,ebx
		movq		[UVLeft],mm2

		mov			ecx,TexPal
		pfrcp		mm5,mm4
		movq		mm3,[RGBADelta]
		xor			eax,eax
		movq		mm6,[ARL]



		paddd		mm0,mm1				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |

		mov			al,byte ptr[esi]
		movq		mm4,mm0				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw   UL|VL      |ZY   UZR|VZR     |

		mov			esi,GBitPtr

		pand		mm4,[WrapMask]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movq		[UVR],mm5

		movq		[UV16],mm4			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |
		psrlq		mm4,[QShiftV]

		mov			ebx,dword ptr[UV16+4]
		movq		[UV16V],mm4

		mov			edx,dword ptr[UV16V]

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |ARGB

		shr			ebx,16
		add			esi,ebx

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		paddd		mm0,mm1

		psllw		mm7,1
		add			esi,edx

		pmulhw		mm7,mm6
		paddw		mm6,mm3

		xor			eax,eax
		movq		mm4,mm0

		mov			al,byte ptr[esi]

		pand		mm4,[WrapMask]

		movd		mm5,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB

		movq		[UV16],mm4			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |
		punpcklbw	mm5,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB

		psrlq		mm4,[QShiftV]
		mov			ebx,dword ptr[UV16+4]

		movq		[UV16V],mm4
		psllw		mm5,1

		mov			edx,dword ptr[UV16V]
		paddd		mm0,mm1

		pmulhw		mm5,mm6
		paddw		mm6,mm3

		shr			ebx,16
		mov			esi,GBitPtr

		add			esi,edx
		movq		mm4,mm0

		packuswb	mm7,mm5				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw ARGB|ARGB UZR|VZR   AR|GB
		add			esi,ebx

		movq		[edi],mm7
		xor			eax,eax

		pand		mm4,[WrapMask]

		mov			al,byte ptr[esi]
		movq		[UV16],mm4

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB
		mov			esi,GBitPtr

		mov			ebx,dword ptr[UV16+4]
		psrlq		mm4,[QShiftV]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		paddd		mm0,mm1

		movq		[UV16V],mm4
		psllw		mm7,1

		pmulhw		mm7,mm6
		xor			eax,eax

		mov			edx,dword ptr[UV16V]
		paddw		mm6,mm3

		shr			ebx,16
		movq		mm4,mm0

		add			esi,edx

		pand		mm4,[WrapMask]
		add			esi,ebx

		mov			al,byte ptr[esi]
		movq		[UV16],mm4			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movd		mm5,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		mov			esi,GBitPtr

		punpcklbw	mm5,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		paddd		mm0,mm1

		psrlq		mm4,[QShiftV]
		psllw		mm5,1

		movq		[UV16V],mm4
		pmulhw		mm5,mm6

		mov			ebx,dword ptr[UV16+4]
		paddw		mm6,mm3

		mov			edx,dword ptr[UV16V]
		movq		mm4,mm0

		shr			ebx,16
		pand		mm4,[WrapMask]

		add			esi,edx
		packuswb	mm7,mm5

		movq		[UV16],mm4
		add			esi,ebx

		movq		[edi+8],mm7
		xor			eax,eax

		psrlq		mm4,[QShiftV]

		mov			al,byte ptr[esi]
		mov			esi,GBitPtr

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB

		mov			ebx,dword ptr[UV16+4]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		paddd		mm0,mm1

		movq		[UV16V],mm4
		psllw		mm7,1

		pmulhw		mm7,mm6
		xor			eax,eax

		mov			edx,dword ptr[UV16V]
		paddw		mm6,mm3

		shr			ebx,16
		movq		mm4,mm0

		add			esi,edx


		pand		mm4,[WrapMask]
		add			esi,ebx

		mov			al,byte ptr[esi]
		movq		[UV16],mm4			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movd		mm5,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		mov			esi,GBitPtr

		punpcklbw	mm5,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		paddd		mm0,mm1

		psrlq		mm4,[QShiftV]
		psllw		mm5,1

		movq		[UV16V],mm4
		pmulhw		mm5,mm6

		mov			ebx,dword ptr[UV16+4]
		paddw		mm6,mm3

		mov			edx,dword ptr[UV16V]
		movq		mm4,mm0

		shr			ebx,16
		pand		mm4,[WrapMask]

		add			esi,edx
		packuswb	mm7,mm5

		movq		[UV16],mm4
		add			esi,ebx

		movq		[edi+16],mm7
		xor			eax,eax

		psrlq		mm4,[QShiftV]

		mov			al,byte ptr[esi]
		mov			esi,GBitPtr

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB

		mov			ebx,dword ptr[UV16+4]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		paddd		mm0,mm1

		movq		[UV16V],mm4
		psllw		mm7,1

		pmulhw		mm7,mm6
		xor			eax,eax

		mov			edx,dword ptr[UV16V]
		paddw		mm6,mm3

		shr			ebx,16
		movq		mm4,mm0

		add			esi,edx

		pand		mm4,[WrapMask]
		add			esi,ebx

		mov			al,byte ptr[esi]
		movq		[UV16],mm4			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movd		mm5,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		mov			esi,GBitPtr

		punpcklbw	mm5,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		paddd		mm0,mm1

		psrlq		mm4,[QShiftV]
		psllw		mm5,1

		movq		[UV16V],mm4
		pmulhw		mm5,mm6

		mov			ebx,dword ptr[UV16+4]
		paddw		mm6,mm3

		mov			edx,dword ptr[UV16V]
		movq		mm4,mm0

		shr			ebx,16
		pand		mm4,[WrapMask]

		add			esi,edx
		packuswb	mm7,mm5

		movq		[UV16],mm4
		add			esi,ebx

		movq		[edi+24],mm7
		xor			eax,eax

		psrlq		mm4,[QShiftV]

		mov			al,byte ptr[esi]
		mov			esi,GBitPtr

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB

		mov			ebx,dword ptr[UV16+4]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		paddd		mm0,mm1

		movq		[UV16V],mm4
		psllw		mm7,1

		pmulhw		mm7,mm6
		xor			eax,eax

		mov			edx,dword ptr[UV16V]
		paddw		mm6,mm3

		shr			ebx,16
		movq		mm4,mm0

		add			esi,edx

		pand		mm4,[WrapMask]
		add			esi,ebx

		mov			al,byte ptr[esi]
		movq		[UV16],mm4			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movd		mm5,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		mov			esi,GBitPtr

		punpcklbw	mm5,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		paddd		mm0,mm1

		psrlq		mm4,[QShiftV]
		psllw		mm5,1

		movq		[UV16V],mm4
		pmulhw		mm5,mm6

		mov			ebx,dword ptr[UV16+4]
		paddw		mm6,mm3

		mov			edx,dword ptr[UV16V]
		movq		mm4,mm0

		shr			ebx,16
		pand		mm4,[WrapMask]

		add			esi,edx
		packuswb	mm7,mm5

		movq		[UV16],mm4
		add			esi,ebx

		movq		[edi+32],mm7
		xor			eax,eax

		psrlq		mm4,[QShiftV]

		mov			al,byte ptr[esi]
		mov			esi,GBitPtr

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB

		mov			ebx,dword ptr[UV16+4]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		paddd		mm0,mm1

		movq		[UV16V],mm4
		psllw		mm7,1

		pmulhw		mm7,mm6
		xor			eax,eax

		mov			edx,dword ptr[UV16V]
		paddw		mm6,mm3

		shr			ebx,16
		movq		mm4,mm0

		add			esi,edx

		pand		mm4,[WrapMask]
		add			esi,ebx

		mov			al,byte ptr[esi]
		movq		[UV16],mm4			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movd		mm5,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		mov			esi,GBitPtr

		punpcklbw	mm5,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		paddd		mm0,mm1

		psrlq		mm4,[QShiftV]
		psllw		mm5,1

		movq		[UV16V],mm4
		pmulhw		mm5,mm6

		mov			ebx,dword ptr[UV16+4]
		paddw		mm6,mm3

		mov			edx,dword ptr[UV16V]
		movq		mm4,mm0

		shr			ebx,16
		pand		mm4,[WrapMask]

		add			esi,edx
		packuswb	mm7,mm5

		movq		[UV16],mm4
		add			esi,ebx

		movq		[edi+40],mm7
		xor			eax,eax

		psrlq		mm4,[QShiftV]

		mov			al,byte ptr[esi]
		mov			esi,GBitPtr

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB

		mov			ebx,dword ptr[UV16+4]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		paddd		mm0,mm1

		movq		[UV16V],mm4
		psllw		mm7,1

		pmulhw		mm7,mm6
		xor			eax,eax

		mov			edx,dword ptr[UV16V]
		paddw		mm6,mm3

		shr			ebx,16
		movq		mm4,mm0

		add			esi,edx

		pand		mm4,[WrapMask]
		add			esi,ebx

		mov			al,byte ptr[esi]
		movq		[UV16],mm4			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movd		mm5,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		mov			esi,GBitPtr

		punpcklbw	mm5,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		paddd		mm0,mm1

		psrlq		mm4,[QShiftV]
		psllw		mm5,1

		movq		[UV16V],mm4
		pmulhw		mm5,mm6

		mov			ebx,dword ptr[UV16+4]
		paddw		mm6,mm3

		mov			edx,dword ptr[UV16V]
		movq		mm4,mm0

		shr			ebx,16
		pand		mm4,[WrapMask]

		add			esi,edx
		packuswb	mm7,mm5

		movq		[UV16],mm4
		add			esi,ebx

		movq		[edi+48],mm7
		xor			eax,eax

		psrlq		mm4,[QShiftV]

		mov			al,byte ptr[esi]
		mov			esi,GBitPtr

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB

		mov			ebx,dword ptr[UV16+4]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB

		movq		[UV16V],mm4
		psllw		mm7,1

		pmulhw		mm7,mm6
		xor			eax,eax

		mov			edx,dword ptr[UV16V]
		paddw		mm6,mm3

		shr			ebx,16

		add			esi,edx

		add			esi,ebx

		mov			al,byte ptr[esi]

		movd		mm5,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB

		punpcklbw	mm5,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB

		psllw		mm5,1

		pmulhw		mm5,mm6
		paddw		mm6,mm3

		packuswb	mm7,mm5
		movq		[ARL],mm6

		movq		[edi+56],mm7

		; get corrected right side deltas ; ULw|VLw DU16|DV16 argb|ARGBagAG|rbRB    |ZRi   aA|rR   UZR|VZR aArR|gGbB

		movq		mm4,[ZIR]
		movq		mm6,[UVZ]

		movq		mm2,[UVR]
		movq		mm0,[UVLeftW]		; ULw|VLw DU16|DV16 argb|ARGBagAG|rbRB    |ZRi   aA|rR   UZR|VZR aArR|gGbB

		movq		mm1,[UVLeft]		; ULw|VLw   UL|VL   argb|ARGBagAG|rbRB ZRi|ZRi   aA|rR   UZR|VZR aArR|gGbB
		punpckldq	mm2,mm2				; ULw|VLw   UL|VL    ZRi|ZRi agAG|rbRB ZRi|ZRi   aA|rR   UZR|VZR aArR|gGbB

		pfmul		mm2,mm6				; ULw|VLw   UL|VL     UR|VR  agAG|rbRB ZRi|ZRi   aA|rR   UZR|VZR aArR|gGbB

		add		edi,64					; move screen pointer to start of next aspan

		movq		mm3,mm2
		pfmul		mm3,[QFixedScale]

		pf2id		mm3,mm3

		dec		[NumASpans]			; dec num affine spans
		jnz		SpanLoopLit

HandleLeftoverPixelsLit:


		mov		esi,GBitPtr


		cmp			[RemainingCount],0
		jz			ReturnLit

		mov			eax,[RemainingCount]
		mov			dword ptr[ZIR],eax
		mov			dword ptr[ZIR+4],eax
		
		movq		mm7,[GLMapMulUV]	;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR   LU|LV
		movq		mm5,mm1

		pfsub		mm6,[UVDivZ16StepX]
		pfmul		mm7,mm1				;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR  ULL|VLL

		pfmul		mm5,[QFixedScale]
		pi2fd		mm3,[ZIR]

		pfmul		mm7,[QFixedScale]
		pfmul		mm3,[UVDivZStepX]

		pf2id		mm5,mm5
		pfadd		mm3,mm6

		pi2fd		mm2,[ZIR]
		movd		mm6,[Zi16StepX]

		pf2id		mm7,mm7				; ULw|VLw   DU|DV    UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |
		pfsub		mm4,mm6

		movd		mm6,[ZiStepX]
		mov			ebx,[RemainingCount]

		pfmul		mm2,mm6

		shl			ebx,3

		pfadd		mm2,mm4
		paddd		mm5,[UVAdjust]

		movq		[UVL16],mm7
		pfrcp		mm2,mm2

		paddd		mm5,[UVAdjust2]
		pfmul		mm2,mm3

		movq		[UVLeft],mm5
		pfsubr		mm1,mm2				; ULw|VLw   DU|DV    UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |

		pfmul		mm1,qword ptr[QFixedScaleLUT+ebx]	; ULw|VLw DU16|DV16  UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |

		movq		mm0,mm5
		pand		mm5,[WrapMask]
		pf2id		mm1,mm1				; ULw|VLw DU16|DV16  UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |

		movq		[UV16],mm5
		movq		mm7,[UVAdjustL]

		psrlq		mm5,[QShiftV]
		paddd		mm7,[UVL16]

		movq		[UV16V],mm5
		movq		[UVL16],mm7


OnePixelSpanLit:
		; Clamp U/V
		mov		ebx,dword ptr[UVL16+4]
		cmp		ebx,MaxU
		jle		TryClampU1Lit
		mov		ecx,MaxU
		mov		dword ptr[UVL16+4],ecx
		jmp		NoClampU1Lit

TryClampU1Lit:
		cmp		ebx,0
		jge		NoClampU1Lit
		mov		dword ptr[UVL16+4],0
NoClampU1Lit:
		mov		eax,dword ptr[UVL16]
		cmp		eax,MaxV
		jle		TryClampV1Lit
		mov		ecx,MaxV
		mov		dword ptr[UVL16],ecx
		jmp		NoClampV1Lit

TryClampV1Lit:
		cmp		eax,0
		jge		NoClampV1Lit
		mov		dword ptr[UVL16],0

NoClampV1Lit:
		mov			ebx,dword ptr[UV16+4]
		mov			eax,dword ptr[UV16V]

		mov			esi,GBitPtr
		shr			ebx,16
		add			esi,eax
		add			esi,ebx
		mov			ecx,TexPal
		xor			eax,eax
		movq		mm6,[ARL]

LeftoverLoopLit:
		paddd		mm0,mm1				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |

		mov			al,byte ptr[esi]
		movq		mm4,mm0				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw   UL|VL      |ZY   UZR|VZR     |

		mov			esi,GBitPtr

		pand		mm4,[WrapMask]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |ARGB

		movq		[UV16],mm4			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB

		mov			ebx,dword ptr[UV16+4]

		psrlq		mm4,[QShiftV]

		shr			ebx,16
		psllw		mm7,1

		movq		[UV16V],mm4
		add			esi,ebx

		mov			edx,dword ptr[UV16V]
		pmulhw		mm7,mm6

		add			esi,edx
		packuswb	mm7,mm7

		xor			eax,eax
		movd		[edi],mm7

		add			edi,4

		dec		[RemainingCount]
		jge		LeftoverLoopLit

ReturnLit:

	pop		edi
	pop		esi
	pop		ecx
	pop		ebx

	cRet DrawSpan32_AsmGouraud3DNow
endProc DrawSpan32_AsmGouraud3DNow


;affine gouraud mapper... will probably be phased out

cProc DrawScanLineGouraudNoZ_Asm3DNow, 8,<pLeft : dword, pRight : dword>

		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		ebx,pLeft
		mov     eax,[ebx].X
		mov     ecx,pRight
		mov     edx,[ecx].X
		sub     edx,eax
		jle		GouraudReturnNoZ

		mov		edi,[ebx].X
;		inc		edx

		femms

;		prefetch	[GBitPtr]

		movd		mm1,[ecx].vf
		movd		mm0,edx

		punpckldq	mm1,qword ptr[ecx].uf
		pi2fd		mm0,mm0

		movq		mm7,[GBL]
		movd		mm2,[ebx].vf

		pfrcp		mm0,mm0
		pfmul		mm1,mm7

		punpckldq	mm2,qword ptr[ebx].uf
		movd		mm3,[ecx].bf

		pfmul		mm2,mm7
		movq		mm7,mm0

		pfmul		mm0,[QFixedScale]
		pfsub		mm1,mm2

		punpckldq	mm3,qword ptr[ecx].gf
		pfmul		mm1,mm0

		movd		mm4,[ecx].rf

		pfmul		mm2,[QFixedScale]
		pf2id		mm1,mm1

		movd		mm5,[ebx].bf
		pf2id		mm2,mm2

		punpckldq	mm5,qword ptr[ebx].gf
		pfmul		mm7,[Q128]

		movd		mm6,[ebx].rf

		pfsub		mm3,mm5
		pfsub		mm4,mm6

		pfmul		mm3,mm7
		pf2id		mm5,mm5

		pfmul		mm4,mm7
		pf2id		mm6,mm6

		movq		mm0,mm2
		pf2id		mm4,mm4

		pf2id		mm3,mm3
		shl			edi,2

		pand		mm0,[WrapMask]
		packssdw	mm3,mm4

		movq		[UV16],mm0
		packssdw	mm5,mm6

		mov			ebx,dword ptr[UV16+4]
		add			edi,[Dest]

		psrlq		mm0,[QShiftV]
		psllw		mm5,7

		shr			ebx,16
		mov			esi,GBitPtr

		movq		[UV16V],mm0
		add			esi,ebx

		mov			ecx,TexPal
		mov			eax,dword ptr[UV16V]

		add			esi,eax
		xor			eax,eax

		push		ebp

GouraudLoopNoZ:
		mov			al,byte ptr[esi]
		movq		mm0,mm2				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw   UL|VL      |ZY   UZR|VZR     |

		mov			esi,GBitPtr
		paddd		mm2,mm1				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |

		pand		mm0,[WrapMask]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |ARGB

		movq		[UV16],mm0			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB

		mov			ebx,dword ptr[UV16+4]

		psrlq		mm0,[QShiftV]

		shr			ebx,16
		psllw		mm7,1

		movq		[UV16V],mm0
		add			esi,ebx

		mov			ebp,dword ptr[UV16V]
		pmulhw		mm7,mm5

		add			esi,ebp
		packuswb	mm7,mm7

		xor			eax,eax
		movd		[edi],mm7

		paddw		mm5,mm3
		add			edi,4

		dec			edx
		jge			GouraudLoopNoZ

		pop			ebp

		femms

GouraudReturnNoZ:

	pop		edi
	pop		esi
	pop		ecx
	pop		ebx

	cRet DrawScanLineGouraudNoZ_Asm3DNow
endProc DrawScanLineGouraudNoZ_Asm3DNow



;does true 32 bit alpha blending... not your greyscale
;junk like hardware... I mean true 32 bit alpha
;looks bad without filtering though

cProc DrawScanLineGouraudNoZAlphaTex_Asm3DNow, 8,<pLeft : dword, pRight : dword>

		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		ebx,pLeft
		mov     eax,[ebx].X
		mov     ecx,pRight
		mov     edx,[ecx].X
		sub     edx,eax
		jle		GouraudReturnNoZAlphaTex

		mov		edi,[ebx].X
;		inc		edx

		femms

;		prefetch	[GBitPtr]

		movd		mm1,[ecx].vf
		movd		mm0,edx

		punpckldq	mm1,qword ptr[ecx].uf
		pi2fd		mm0,mm0

		movq		mm7,[GBL]
		movd		mm2,[ebx].vf

		pfrcp		mm0,mm0
		pfmul		mm1,mm7

		punpckldq	mm2,qword ptr[ebx].uf
		movd		mm3,[ecx].bf

		pfmul		mm2,mm7
		movq		mm7,mm0

		pfmul		mm0,[QFixedScale]
		pfsub		mm1,mm2

		punpckldq	mm3,qword ptr[ecx].gf
		pfmul		mm1,mm0

		movd		mm4,[ecx].rf

		pfmul		mm2,[QFixedScale]
		pf2id		mm1,mm1

		movd		mm5,[ebx].bf
		pf2id		mm2,mm2

		punpckldq	mm5,qword ptr[ebx].gf
		pfmul		mm7,[Q128]

		movd		mm6,[ebx].rf

		pfsub		mm3,mm5
		pfsub		mm4,mm6

		pfmul		mm3,mm7
		pf2id		mm5,mm5

		pfmul		mm4,mm7
		pf2id		mm6,mm6

		movq		mm0,mm2
		pf2id		mm4,mm4

		pf2id		mm3,mm3
		shl			edi,2

		pand		mm0,[WrapMask]
		packssdw	mm3,mm4

		movq		[UV16],mm0
		packssdw	mm5,mm6

		mov			ebx,dword ptr[UV16+4]
		add			edi,[Dest]

		psrlq		mm0,[QShiftV]
		psllw		mm5,7

		shr			ebx,16
		mov			esi,GBitPtr

		movq		[UV16V],mm0
		add			esi,ebx

		mov			ecx,TexPal
		mov			eax,dword ptr[UV16V]

		add			esi,eax
		xor			eax,eax

		push		ebp

GouraudLoopNoZAlphaTex:
		mov			al,byte ptr[esi]
		movq		mm0,mm2				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw   UL|VL      |ZY   UZR|VZR     |

		sub			esi,GBitPtr
		paddd		mm2,mm1				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |

		pand		mm0,[WrapMask]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |ARGB

		movq		[UV16],mm0			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB

		mov			ebx,dword ptr[UV16+4]

		psrlq		mm0,[QShiftV]

		add			esi,ABitPtr

		shr			ebx,16

		mov			al,byte ptr[esi]
		psllw		mm7,1

		movq		[UV16V],mm0
		mov			esi,ebx

		movd		mm0,[edi]

		mov			ebx,ATexPal

		mov			ebp,dword ptr[UV16V]
		pmulhw		mm7,mm5

		movd		mm4,[ebx+eax*4]
		add			esi,ebp

		punpcklbw	mm0,[Zero]

		movq		mm6,[QNegAlpha]

		punpcklbw	mm4,[Zero]

		add			esi,GBitPtr

		pmullw		mm7,mm4
		psubw		mm6,mm4

		paddw		mm5,mm3
		pmullw		mm0,mm6

		add			edi,4
		paddw		mm0,mm7

		psrlw		mm0,8

		packuswb	mm0,mm0
		
		movd		[edi-4],mm0

		dec			edx
		jge			GouraudLoopNoZAlphaTex

		pop			ebp

		femms

GouraudReturnNoZAlphaTex:

	pop		edi
	pop		esi
	pop		ecx
	pop		ebx

	cRet DrawScanLineGouraudNoZAlphaTex_Asm3DNow
endProc DrawScanLineGouraudNoZAlphaTex_Asm3DNow


;same with z

cProc DrawScanLineGouraudZBufferAlphaTex_Asm3DNow, 8,<pLeft : dword, pRight : dword>

		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		ebx,pLeft
		mov     eax,[ebx].X
		mov     ecx,pRight
		mov     edx,[ecx].X
		sub     edx,eax
		jle		GouraudReturnZBufferAlphaTex

		mov		edi,[ebx].X
;		inc		edx

		femms

		movd		mm1,[ecx].vf
		movd		mm0,edx

		punpckldq	mm1,qword ptr[ecx].uf
		pi2fd		mm0,mm0

		movq		mm7,[GBL]
		movd		mm2,[ebx].vf

		pfrcp		mm0,mm0
		pfmul		mm1,mm7

		punpckldq	mm2,qword ptr[ebx].uf
		movd		mm3,[ecx].bf

		pfmul		mm2,mm7
		movq		mm7,mm0

		pfmul		mm0,[QFixedScale]
		pfsub		mm1,mm2

		punpckldq	mm3,qword ptr[ecx].gf
		pfmul		mm1,mm0

		movd		mm4,[ecx].rf

		pfmul		mm2,[QFixedScale]
		pf2id		mm1,mm1

		movd		mm5,[ebx].bf
		pf2id		mm2,mm2

		movq		mm0,mm7

		punpckldq	mm5,qword ptr[ebx].gf
		pfmul		mm7,[Q128]

		movd		mm6,[ebx].rf

		pfsub		mm3,mm5
		pfsub		mm4,mm6

		pfmul		mm3,mm7
		pf2id		mm5,mm5

		pfmul		mm4,mm7
		movq		mm7,mm0

		pf2id		mm6,mm6

		movq		mm0,mm2
		pf2id		mm4,mm4

		pf2id		mm3,mm3
		shl			edi,1

		pand		mm0,[WrapMask]
		packssdw	mm3,mm4

		add			[pZBufferPtr],edi
		packssdw	mm5,mm6

		movq		[UV16],mm0
		shl			edi,1

		movd		mm4,[ecx].zf
		psllw		mm5,7

		movd		mm6,[ebx].zf
		mov			ebx,dword ptr[UV16+4]

		pfsub		mm4,mm6
		add			edi,[Dest]

		psrlq		mm0,[QShiftV]
		pfmul		mm4,mm7

		mov			esi,GBitPtr
		pf2id		mm6,mm6

		movq		[UV16V],mm0
		shr			ebx,16

		mov			eax,dword ptr[UV16V]
		add			esi,ebx

		mov			ecx,TexPal
		pf2id		mm4,mm4

		movq		[ARL],mm1
		add			esi,eax

		movq		[UVR],mm3

		xor			eax,eax
		push		ebp

GouraudLoopZBufferAlphaTex:
		mov			al,byte ptr[esi]
		movq		mm0,mm2				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw   UL|VL      |ZY   UZR|VZR     |

		sub			esi,GBitPtr
		paddd		mm2,[ARL]			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |

		pand		mm0,[WrapMask]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |ARGB

		movq		[UV16],mm0			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB

		mov			ebx,dword ptr[UV16+4]

		psrlq		mm0,[QShiftV]

		add			esi,ABitPtr

		shr			ebx,16

		mov			al,byte ptr[esi]
		psllw		mm7,1

		movq		[UV16V],mm0
		mov			esi,ebx

		mov			ebx,ATexPal
		mov			ebp,dword ptr[UV16V]

		movd		mm1,[ebx+eax*4]
		pmulhw		mm7,mm5

		movq		mm3,[QNegAlpha]
		add			esi,ebp

		punpcklbw	mm1,[Zero]
		movd		eax,mm6

		add			esi,GBitPtr
		shr			eax,16

		mov			ebx,pZBufferPtr
		paddd		mm6,mm4

		paddw		mm5,[UVR]
		add			edi,4

		cmp			word ptr[ebx],ax

		jg			SkipPixelZBufferAlphaTex

		movd		mm0,[edi-4]
		pmullw		mm7,mm1

		punpcklbw	mm0,[Zero]
		psubw		mm3,mm1

		pmullw		mm0,mm3

		mov			word ptr[ebx],ax
		paddw		mm0,mm7

		psrlw		mm0,8

		packuswb	mm0,mm0
		
		movd		[edi-4],mm0

SkipPixelZBufferAlphaTex:
	
		add			[pZBufferPtr],2
		xor			eax,eax

		dec			edx
		jge			GouraudLoopZBufferAlphaTex

		pop			ebp

		femms

GouraudReturnZBufferAlphaTex:

	pop		edi
	pop		esi
	pop		ecx
	pop		ebx

	cRet DrawScanLineGouraudZBufferAlphaTex_Asm3DNow
endProc DrawScanLineGouraudZBufferAlphaTex_Asm3DNow



;same with zwrite

cProc DrawScanLineGouraudNoZBufferZWriteAlphaTex_Asm3DNow, 8,<pLeft : dword, pRight : dword>

		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		ebx,pLeft
		mov     eax,[ebx].X
		mov     ecx,pRight
		mov     edx,[ecx].X
		sub     edx,eax
		jle		GouraudReturnNoZBufferZWriteAlphaTex

		mov		edi,[ebx].X
;		inc		edx

		femms

		movd		mm1,[ecx].vf
		movd		mm0,edx

		punpckldq	mm1,qword ptr[ecx].uf
		pi2fd		mm0,mm0

		movq		mm7,[GBL]
		movd		mm2,[ebx].vf

		pfrcp		mm0,mm0
		pfmul		mm1,mm7

		punpckldq	mm2,qword ptr[ebx].uf
		movd		mm3,[ecx].bf

		pfmul		mm2,mm7
		movq		mm7,mm0

		pfmul		mm0,[QFixedScale]
		pfsub		mm1,mm2

		punpckldq	mm3,qword ptr[ecx].gf
		pfmul		mm1,mm0

		movd		mm4,[ecx].rf

		pfmul		mm2,[QFixedScale]
		pf2id		mm1,mm1

		movd		mm5,[ebx].bf
		pf2id		mm2,mm2

		movq		mm0,mm7

		punpckldq	mm5,qword ptr[ebx].gf
		pfmul		mm7,[Q128]

		movd		mm6,[ebx].rf

		pfsub		mm3,mm5
		pfsub		mm4,mm6

		pfmul		mm3,mm7
		pf2id		mm5,mm5

		pfmul		mm4,mm7
		movq		mm7,mm0

		pf2id		mm6,mm6

		movq		mm0,mm2
		pf2id		mm4,mm4

		pf2id		mm3,mm3
		shl			edi,1

		pand		mm0,[WrapMask]
		packssdw	mm3,mm4

		add			[pZBufferPtr],edi
		packssdw	mm5,mm6

		movq		[UV16],mm0
		shl			edi,1

		movd		mm4,[ecx].zf
		psllw		mm5,7

		movd		mm6,[ebx].zf
		mov			ebx,dword ptr[UV16+4]

		pfsub		mm4,mm6
		add			edi,[Dest]

		psrlq		mm0,[QShiftV]
		pfmul		mm4,mm7

		mov			esi,GBitPtr
		pf2id		mm6,mm6

		movq		[UV16V],mm0
		shr			ebx,16

		mov			eax,dword ptr[UV16V]
		add			esi,ebx

		mov			ecx,TexPal
		pf2id		mm4,mm4

		movq		[ARL],mm1
		add			esi,eax

		movq		[UVR],mm3

		xor			eax,eax
		push		ebp

GouraudLoopNoZBufferZWriteAlphaTex:
		mov			al,byte ptr[esi]
		movq		mm0,mm2				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw   UL|VL      |ZY   UZR|VZR     |

		sub			esi,GBitPtr
		paddd		mm2,[ARL]			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |

		pand		mm0,[WrapMask]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |ARGB

		movq		[UV16],mm0			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB

		mov			ebx,dword ptr[UV16+4]

		psrlq		mm0,[QShiftV]

		add			esi,ABitPtr

		shr			ebx,16

		mov			al,byte ptr[esi]
		psllw		mm7,1

		movq		[UV16V],mm0
		mov			esi,ebx

		mov			ebx,ATexPal
		mov			ebp,dword ptr[UV16V]

		movd		mm1,[ebx+eax*4]
		pmulhw		mm7,mm5

		movq		mm3,[QNegAlpha]
		add			esi,ebp

		punpcklbw	mm1,[Zero]
		movd		eax,mm6

		add			esi,GBitPtr
		shr			eax,16

		mov			ebx,pZBufferPtr
		paddd		mm6,mm4

		paddw		mm5,[UVR]
		add			edi,4

		movd		mm0,[edi-4]
		pmullw		mm7,mm1

		punpcklbw	mm0,[Zero]
		psubw		mm3,mm1

		pmullw		mm0,mm3

		mov			word ptr[ebx],ax
		paddw		mm0,mm7

		psrlw		mm0,8

		packuswb	mm0,mm0
		
		movd		[edi-4],mm0

		add			[pZBufferPtr],2
		xor			eax,eax

		dec			edx
		jge			GouraudLoopNoZBufferZWriteAlphaTex

		pop			ebp

		femms

GouraudReturnNoZBufferZWriteAlphaTex:

	pop		edi
	pop		esi
	pop		ecx
	pop		ebx

	cRet DrawScanLineGouraudNoZBufferZWriteAlphaTex_Asm3DNow
endProc DrawScanLineGouraudNoZBufferZWriteAlphaTex_Asm3DNow


;zmask but no zwrite

cProc DrawScanLineGouraudZBufferNoZWriteAlphaTex_Asm3DNow, 8,<pLeft : dword, pRight : dword>

		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		ebx,pLeft
		mov     eax,[ebx].X
		mov     ecx,pRight
		mov     edx,[ecx].X
		sub     edx,eax
		jle		GouraudReturnZBufferNoZWriteAlphaTex

		mov		edi,[ebx].X
;		inc		edx

		femms

		movd		mm1,[ecx].vf
		movd		mm0,edx

		punpckldq	mm1,qword ptr[ecx].uf
		pi2fd		mm0,mm0

		movq		mm7,[GBL]
		movd		mm2,[ebx].vf

		pfrcp		mm0,mm0
		pfmul		mm1,mm7

		punpckldq	mm2,qword ptr[ebx].uf
		movd		mm3,[ecx].bf

		pfmul		mm2,mm7
		movq		mm7,mm0

		pfmul		mm0,[QFixedScale]
		pfsub		mm1,mm2

		punpckldq	mm3,qword ptr[ecx].gf
		pfmul		mm1,mm0

		movd		mm4,[ecx].rf

		pfmul		mm2,[QFixedScale]
		pf2id		mm1,mm1

		movd		mm5,[ebx].bf
		pf2id		mm2,mm2

		movq		mm0,mm7

		punpckldq	mm5,qword ptr[ebx].gf
		pfmul		mm7,[Q128]

		movd		mm6,[ebx].rf

		pfsub		mm3,mm5
		pfsub		mm4,mm6

		pfmul		mm3,mm7
		pf2id		mm5,mm5

		pfmul		mm4,mm7
		movq		mm7,mm0

		pf2id		mm6,mm6

		movq		mm0,mm2
		pf2id		mm4,mm4

		pf2id		mm3,mm3
		shl			edi,1

		pand		mm0,[WrapMask]
		packssdw	mm3,mm4

		add			[pZBufferPtr],edi
		packssdw	mm5,mm6

		movq		[UV16],mm0
		shl			edi,1

		movd		mm4,[ecx].zf
		psllw		mm5,7

		movd		mm6,[ebx].zf
		mov			ebx,dword ptr[UV16+4]

		pfsub		mm4,mm6
		add			edi,[Dest]

		psrlq		mm0,[QShiftV]
		pfmul		mm4,mm7

		mov			esi,GBitPtr
		pf2id		mm6,mm6

		movq		[UV16V],mm0
		shr			ebx,16

		mov			eax,dword ptr[UV16V]
		add			esi,ebx

		mov			ecx,TexPal
		pf2id		mm4,mm4

		movq		[ARL],mm1
		add			esi,eax

		movq		[UVR],mm3

		xor			eax,eax
		push		ebp

GouraudLoopZBufferNoZWriteAlphaTex:
		mov			al,byte ptr[esi]
		movq		mm0,mm2				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw   UL|VL      |ZY   UZR|VZR     |

		sub			esi,GBitPtr
		paddd		mm2,[ARL]			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |

		pand		mm0,[WrapMask]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |ARGB

		movq		[UV16],mm0			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB

		mov			ebx,dword ptr[UV16+4]

		psrlq		mm0,[QShiftV]

		add			esi,ABitPtr

		shr			ebx,16

		mov			al,byte ptr[esi]
		psllw		mm7,1

		movq		[UV16V],mm0
		mov			esi,ebx

		mov			ebx,ATexPal
		mov			ebp,dword ptr[UV16V]

		movd		mm1,[ebx+eax*4]
		pmulhw		mm7,mm5

		movq		mm3,[QNegAlpha]
		add			esi,ebp

		punpcklbw	mm1,[Zero]
		movd		eax,mm6

		add			esi,GBitPtr
		shr			eax,16

		mov			ebx,pZBufferPtr
		paddd		mm6,mm4

		paddw		mm5,[UVR]
		add			edi,4

		cmp			word ptr[ebx],ax

		jg			SkipPixelZBufferNoZWriteAlphaTex

		movd		mm0,[edi-4]
		pmullw		mm7,mm1

		punpcklbw	mm0,[Zero]
		psubw		mm3,mm1

		pmullw		mm0,mm3

		paddw		mm0,mm7

		psrlw		mm0,8

		packuswb	mm0,mm0
		
		movd		[edi-4],mm0

SkipPixelZBufferNoZWriteAlphaTex:
	
		add			[pZBufferPtr],2
		xor			eax,eax

		dec			edx
		jge			GouraudLoopZBufferNoZWriteAlphaTex

		pop			ebp

		femms

GouraudReturnZBufferNoZWriteAlphaTex:

	pop		edi
	pop		esi
	pop		ecx
	pop		ebx

	cRet DrawScanLineGouraudZBufferNoZWriteAlphaTex_Asm3DNow
endProc DrawScanLineGouraudZBufferNoZWriteAlphaTex_Asm3DNow



;solid color gouraud (no texture)

cProc DrawScanLineGouraudNoZSolid_Asm3DNow, 8,<pLeft : dword, pRight : dword>

		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		ebx,pLeft
		mov     eax,[ebx].X
		mov     ecx,pRight
		mov     edx,[ecx].X
		sub     edx,eax
		jle		GouraudReturnSolidNoZ

		mov		edi,[ebx].X
;		inc		edx

		femms

		movd		mm1,SolidColor
		movd		mm0,edx

		punpcklbw	mm1,[Zero]
		pi2fd		mm0,mm0

		movd		mm3,[ecx].bf
		pfrcp		mm0,mm0

		punpckldq	mm3,qword ptr[ecx].gf
		movq		mm7,mm0

		movd		mm4,[ecx].rf
		movd		mm5,[ebx].bf

		pfmul		mm7,[Q128]
		punpckldq	mm5,qword ptr[ebx].gf

		movd		mm6,[ebx].rf

		pfsub		mm3,mm5
		pfsub		mm4,mm6

		pfmul		mm3,mm7
		pf2id		mm5,mm5

		pfmul		mm4,mm7
		pf2id		mm6,mm6

		pf2id		mm4,mm4
		pf2id		mm3,mm3

		shl			edi,2
		packssdw	mm3,mm4

		packssdw	mm5,mm6

		add			edi,[Dest]

		psllw		mm5,7

GouraudLoopSolidNoZ:
		movq		mm7,mm1
		paddw		mm5,mm3

		psllw		mm7,1

		pmulhw		mm7,mm5
		add			edi,4

		packuswb	mm7,mm7
		movd		[edi-4],mm7

		dec			edx
		jge			GouraudLoopSolidNoZ

		femms

GouraudReturnSolidNoZ:

	pop		edi
	pop		esi
	pop		ecx
	pop		ebx

	cRet DrawScanLineGouraudNoZSolid_Asm3DNow
endProc DrawScanLineGouraudNoZSolid_Asm3DNow


;same with z

cProc DrawScanLineGouraudZBufferSolid_Asm3DNow, 8,<pLeft : dword, pRight : dword>

		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		ebx,pLeft
		mov     eax,[ebx].X
		mov     ecx,pRight
		mov     edx,[ecx].X
		sub     edx,eax
		jle		GouraudReturnSolidZBuffer

		mov		edi,[ebx].X
;		inc		edx

		femms

		movd		mm1,SolidColor
		movd		mm0,edx

		punpcklbw	mm1,[Zero]
		pi2fd		mm0,mm0

		movd		mm3,[ecx].bf
		pfrcp		mm0,mm0

		punpckldq	mm3,qword ptr[ecx].gf
		movq		mm7,mm0

		movd		mm4,[ecx].rf
		movd		mm5,[ebx].bf

		pfmul		mm7,[Q128]
		punpckldq	mm5,qword ptr[ebx].gf

		movd		mm6,[ebx].rf
		pfsub		mm3,mm5

		movd		mm2,[ecx].zf
		pfsub		mm4,mm6

		pfmul		mm3,mm7
		pf2id		mm5,mm5

		pfmul		mm4,mm7
		movd		mm7,[ebx].zf

		pf2id		mm6,mm6
		pfsub		mm2,mm7

		pf2id		mm4,mm4
		pfmul		mm2,mm0

		pf2id		mm3,mm3
		shl			edi,1

		packssdw	mm3,mm4
		add			[pZBufferPtr],edi

		packssdw	mm5,mm6
		shl			edi,1

		add			edi,[Dest]
		mov			ebx,pZBufferPtr

		pf2id		mm6,mm6
		psllw		mm5,7

		pf2id		mm7,mm7

GouraudLoopSolidZBuffer:
		movq		mm4,mm1
		paddw		mm5,mm3

		psllw		mm4,1
		movd		eax,mm7

		pmulhw		mm4,mm5

		shr			eax,16
		add			edi,4

		cmp			word ptr[ebx],ax
		jg			SkipPixelGouraudSolidZBuffer

		packuswb	mm4,mm4
		movd		[edi-4],mm4

		mov			word ptr[ebx],ax

SkipPixelGouraudSolidZBuffer:
		add			ebx,2
		paddd		mm7,mm6
		dec			edx
		jge			GouraudLoopSolidZBuffer

		femms

GouraudReturnSolidZBuffer:

	pop		edi
	pop		esi
	pop		ecx
	pop		ebx

	cRet DrawScanLineGouraudZBufferSolid_Asm3DNow
endProc DrawScanLineGouraudZBufferSolid_Asm3DNow


; same with zmask no zwrite

cProc DrawScanLineGouraudZBufferNoZWriteSolid_Asm3DNow, 8,<pLeft : dword, pRight : dword>

		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		ebx,pLeft
		mov     eax,[ebx].X
		mov     ecx,pRight
		mov     edx,[ecx].X
		sub     edx,eax
		jle		GouraudReturnSolidZBufferNoZWrite

		mov		edi,[ebx].X
;		inc		edx

		femms

		movd		mm1,SolidColor
		movd		mm0,edx

		punpcklbw	mm1,[Zero]
		pi2fd		mm0,mm0

		movd		mm3,[ecx].bf
		pfrcp		mm0,mm0

		punpckldq	mm3,qword ptr[ecx].gf
		movq		mm7,mm0

		movd		mm4,[ecx].rf
		movd		mm5,[ebx].bf

		pfmul		mm7,[Q128]
		punpckldq	mm5,qword ptr[ebx].gf

		movd		mm6,[ebx].rf
		pfsub		mm3,mm5

		movd		mm2,[ecx].zf
		pfsub		mm4,mm6

		pfmul		mm3,mm7
		pf2id		mm5,mm5

		pfmul		mm4,mm7
		movd		mm7,[ebx].zf

		pf2id		mm6,mm6
		pfsub		mm2,mm7

		pf2id		mm4,mm4
		pfmul		mm2,mm0

		pf2id		mm3,mm3
		shl			edi,1

		packssdw	mm3,mm4
		add			[pZBufferPtr],edi

		packssdw	mm5,mm6
		shl			edi,1

		add			edi,[Dest]
		mov			ebx,pZBufferPtr

		pf2id		mm6,mm6
		psllw		mm5,7

		pf2id		mm7,mm7

GouraudLoopSolidZBufferNoZWrite:
		movq		mm4,mm1
		paddw		mm5,mm3

		psllw		mm4,1
		movd		eax,mm7

		pmulhw		mm4,mm5

		shr			eax,16
		add			edi,4

		cmp			word ptr[ebx],ax
		jg			SkipPixelGouraudSolidZBufferNoZWrite

		packuswb	mm4,mm4
		movd		[edi-4],mm4

SkipPixelGouraudSolidZBufferNoZWrite:
		add			ebx,2
		paddd		mm7,mm6
		dec			edx
		jge			GouraudLoopSolidZBufferNoZWrite

		femms

GouraudReturnSolidZBufferNoZWrite:

	pop		edi
	pop		esi
	pop		ecx
	pop		ebx

	cRet DrawScanLineGouraudZBufferNoZWriteSolid_Asm3DNow
endProc DrawScanLineGouraudZBufferNoZWriteSolid_Asm3DNow


;same with zwrite only

cProc DrawScanLineGouraudNoZBufferZWriteSolid_Asm3DNow, 8,<pLeft : dword, pRight : dword>

		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		ebx,pLeft
		mov     eax,[ebx].X
		mov     ecx,pRight
		mov     edx,[ecx].X
		sub     edx,eax
		jle		GouraudReturnSolidNoZBufferZWrite

		mov		edi,[ebx].X
;		inc		edx

		femms

		movd		mm1,SolidColor
		movd		mm0,edx

		punpcklbw	mm1,[Zero]
		pi2fd		mm0,mm0

		movd		mm3,[ecx].bf
		pfrcp		mm0,mm0

		punpckldq	mm3,qword ptr[ecx].gf
		movq		mm7,mm0

		movd		mm4,[ecx].rf
		movd		mm5,[ebx].bf

		pfmul		mm7,[Q128]
		punpckldq	mm5,qword ptr[ebx].gf

		movd		mm6,[ebx].rf
		pfsub		mm3,mm5

		movd		mm2,[ecx].zf
		pfsub		mm4,mm6

		pfmul		mm3,mm7
		pf2id		mm5,mm5

		pfmul		mm4,mm7
		movd		mm7,[ebx].zf

		pf2id		mm6,mm6
		pfsub		mm2,mm7

		pf2id		mm4,mm4
		pfmul		mm2,mm0

		pf2id		mm3,mm3
		shl			edi,1

		packssdw	mm3,mm4
		add			[pZBufferPtr],edi

		packssdw	mm5,mm6
		shl			edi,1

		add			edi,[Dest]
		mov			ebx,pZBufferPtr

		pf2id		mm6,mm6
		psllw		mm5,7

		pf2id		mm7,mm7

GouraudLoopSolidNoZBufferZWrite:
		movq		mm4,mm1
		paddw		mm5,mm3

		psllw		mm4,1
		movd		eax,mm7

		pmulhw		mm4,mm5

		shr			eax,16
		add			edi,4

		cmp			word ptr[ebx],ax
		jg			SkipPixelGouraudSolidNoZBufferZWrite

		packuswb	mm4,mm4
		movd		[edi-4],mm4

		mov			word ptr[ebx],ax

SkipPixelGouraudSolidNoZBufferZWrite:
		add			ebx,2
		paddd		mm7,mm6
		dec			edx
		jge			GouraudLoopSolidNoZBufferZWrite

		femms

GouraudReturnSolidNoZBufferZWrite:

	pop		edi
	pop		esi
	pop		ecx
	pop		ebx

	cRet DrawScanLineGouraudNoZBufferZWriteSolid_Asm3DNow
endProc DrawScanLineGouraudNoZBufferZWriteSolid_Asm3DNow


;affine color keyed

cProc DrawScanLineGouraudNoZTrans_Asm3DNow, 8,<pLeft : dword, pRight : dword>

		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		ebx,pLeft
		mov     eax,[ebx].X
		mov     ecx,pRight
		mov     edx,[ecx].X
		sub     edx,eax
		jle		GouraudReturnNoZTrans

		mov		edi,[ebx].X
;		inc		edx

		femms

;		prefetch	[GBitPtr]

		movd		mm1,[ecx].vf
		movd		mm0,edx

		punpckldq	mm1,qword ptr[ecx].uf
		pi2fd		mm0,mm0

		movq		mm7,[GBL]
		movd		mm2,[ebx].vf

		pfrcp		mm0,mm0
		pfmul		mm1,mm7

		punpckldq	mm2,qword ptr[ebx].uf
		movd		mm3,[ecx].bf

		pfmul		mm2,mm7
		movq		mm7,mm0

		pfmul		mm0,[QFixedScale]
		pfsub		mm1,mm2

		punpckldq	mm3,qword ptr[ecx].gf
		pfmul		mm1,mm0

		movd		mm4,[ecx].rf

		pfmul		mm2,[QFixedScale]
		pf2id		mm1,mm1

		movd		mm5,[ebx].bf
		pf2id		mm2,mm2

		punpckldq	mm5,qword ptr[ebx].gf
		pfmul		mm7,[Q128]

		movd		mm6,[ebx].rf

		pfsub		mm3,mm5
		pfsub		mm4,mm6

		pfmul		mm3,mm7
		pf2id		mm5,mm5

		pfmul		mm4,mm7
		pf2id		mm6,mm6

		movq		mm0,mm2
		pf2id		mm4,mm4

		pf2id		mm3,mm3
		shl			edi,2

		pand		mm0,[WrapMask]
		packssdw	mm3,mm4

		movq		[UV16],mm0
		packssdw	mm5,mm6

		mov			ebx,dword ptr[UV16+4]
		add			edi,[Dest]

		psrlq		mm0,[QShiftV]
		psllw		mm5,7

		shr			ebx,16
		mov			esi,GBitPtr

		movq		[UV16V],mm0
		add			esi,ebx

		mov			ecx,TexPal
		mov			eax,dword ptr[UV16V]

		add			esi,eax
		xor			eax,eax

		push		ebp

GouraudLoopNoZTrans:
		movq		mm0,mm2				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw   UL|VL      |ZY   UZR|VZR     |
		mov			al,byte ptr[esi]

		pand		mm0,[WrapMask]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |
		mov			esi,GBitPtr

		movq		[UV16],mm0			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		mov			ebx,dword ptr[UV16+4]
		psrlq		mm0,[QShiftV]

		shr			ebx,16
		movq		[UV16V],mm0

		add			esi,ebx
		mov			ebp,dword ptr[UV16V]

		add			esi,ebp

		cmp			al,0ffh
		je			SkipPixelGouraudNoZTrans

		;ouch register contention from hell
		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |ARGB
		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		psllw		mm7,1
		pmulhw		mm7,mm5
		packuswb	mm7,mm7
		movd		[edi],mm7

SkipPixelGouraudNoZTrans:

		add			edi,4
		xor			eax,eax

		paddd		mm2,mm1				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |
		paddd		mm6,mm4
		paddw		mm5,mm3

		dec			edx
		jge			GouraudLoopNoZTrans

		pop			ebp

		femms

GouraudReturnNoZTrans:

	pop		edi
	pop		esi
	pop		ecx
	pop		ebx

	cRet DrawScanLineGouraudNoZTrans_Asm3DNow
endProc DrawScanLineGouraudNoZTrans_Asm3DNow


;affine textured with zbuffering and gouraud

cProc DrawScanLineGouraudZBuffer_Asm3DNow, 8,<pLeft : dword, pRight : dword>

		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		ebx,pLeft
		mov     eax,[ebx].X
		mov     ecx,pRight
		mov     edx,[ecx].X
		sub     edx,eax
		jle		GouraudReturnZBuffer

		mov		edi,[ebx].X
;		inc		edx

		femms

		movd		mm1,[ecx].vf
		movd		mm0,edx

		punpckldq	mm1,qword ptr[ecx].uf
		pi2fd		mm0,mm0

		movq		mm7,[GBL]
		movd		mm2,[ebx].vf

		pfrcp		mm0,mm0
		pfmul		mm1,mm7

		punpckldq	mm2,qword ptr[ebx].uf
		movd		mm3,[ecx].bf

		pfmul		mm2,mm7
		movq		mm7,mm0

		pfmul		mm0,[QFixedScale]
		pfsub		mm1,mm2

		punpckldq	mm3,qword ptr[ecx].gf
		pfmul		mm1,mm0

		movd		mm4,[ecx].rf

		pfmul		mm2,[QFixedScale]
		pf2id		mm1,mm1

		movd		mm5,[ebx].bf
		pf2id		mm2,mm2

		movq		mm0,mm7

		punpckldq	mm5,qword ptr[ebx].gf
		pfmul		mm7,[Q128]

		movd		mm6,[ebx].rf

		pfsub		mm3,mm5
		pfsub		mm4,mm6

		pfmul		mm3,mm7
		pf2id		mm5,mm5

		pfmul		mm4,mm7
		movq		mm7,mm0

		pf2id		mm6,mm6

		movq		mm0,mm2
		pf2id		mm4,mm4

		pf2id		mm3,mm3
		shl			edi,1

		pand		mm0,[WrapMask]
		packssdw	mm3,mm4

		add			[pZBufferPtr],edi
		packssdw	mm5,mm6

		movq		[UV16],mm0
		shl			edi,1

		movd		mm4,[ecx].zf
		psllw		mm5,7

		movd		mm6,[ebx].zf
		mov			ebx,dword ptr[UV16+4]

		pfsub		mm4,mm6
		add			edi,[Dest]

		psrlq		mm0,[QShiftV]
		pfmul		mm4,mm7

		mov			esi,GBitPtr
		pf2id		mm6,mm6

		movq		[UV16V],mm0
		shr			ebx,16

		mov			eax,dword ptr[UV16V]
		add			esi,ebx

		mov			ecx,TexPal
		pf2id		mm4,mm4

		add			esi,eax
		xor			eax,eax

		push		ebp

GouraudLoopZBuffer:
		mov			al,byte ptr[esi]
		movq		mm0,mm2				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw   UL|VL      |ZY   UZR|VZR     |

		mov			esi,GBitPtr
		paddd		mm2,mm1				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |

		pand		mm0,[WrapMask]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |ARGB
		movd		eax,mm6

		movq		[UV16],mm0			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |
		shr			eax,16

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		paddd		mm6,mm4

		mov			ebx,dword ptr[UV16+4]

		psrlq		mm0,[QShiftV]

		shr			ebx,16
		psllw		mm7,1

		movq		[UV16V],mm0
		add			esi,ebx

		mov			ebp,dword ptr[UV16V]
		pmulhw		mm7,mm5

		mov			ebx,pZBufferPtr
		add			esi,ebp

		packuswb	mm7,mm7
		paddw		mm5,mm3

		cmp			word ptr[ebx],ax
		jg			SkipPixelGouraudZBuffer

		movd		[edi],mm7
		mov			word ptr[ebx],ax

SkipPixelGouraudZBuffer:

		add			edi,4
		xor			eax,eax

		add			[pZBufferPtr],2

		dec			edx
		jge			GouraudLoopZBuffer

		pop			ebp

		femms

GouraudReturnZBuffer:

	pop		edi
	pop		esi
	pop		ecx
	pop		ebx

	cRet DrawScanLineGouraudZBuffer_Asm3DNow
endProc DrawScanLineGouraudZBuffer_Asm3DNow


;same with no zwrite

cProc DrawScanLineGouraudZBufferNoZWrite_Asm3DNow, 8,<pLeft : dword, pRight : dword>

		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		ebx,pLeft
		mov     eax,[ebx].X
		mov     ecx,pRight
		mov     edx,[ecx].X
		sub     edx,eax
		jle		GouraudReturnZBufferNoZWrite

		mov		edi,[ebx].X
;		inc		edx

		femms

		movd		mm1,[ecx].vf
		movd		mm0,edx

		punpckldq	mm1,qword ptr[ecx].uf
		pi2fd		mm0,mm0

		movq		mm7,[GBL]
		movd		mm2,[ebx].vf

		pfrcp		mm0,mm0
		pfmul		mm1,mm7

		punpckldq	mm2,qword ptr[ebx].uf
		movd		mm3,[ecx].bf

		pfmul		mm2,mm7
		movq		mm7,mm0

		pfmul		mm0,[QFixedScale]
		pfsub		mm1,mm2

		punpckldq	mm3,qword ptr[ecx].gf
		pfmul		mm1,mm0

		movd		mm4,[ecx].rf

		pfmul		mm2,[QFixedScale]
		pf2id		mm1,mm1

		movd		mm5,[ebx].bf
		pf2id		mm2,mm2

		movq		mm0,mm7

		punpckldq	mm5,qword ptr[ebx].gf
		pfmul		mm7,[Q128]

		movd		mm6,[ebx].rf

		pfsub		mm3,mm5
		pfsub		mm4,mm6

		pfmul		mm3,mm7
		pf2id		mm5,mm5

		pfmul		mm4,mm7
		movq		mm7,mm0

		pf2id		mm6,mm6

		movq		mm0,mm2
		pf2id		mm4,mm4

		pf2id		mm3,mm3
		shl			edi,1

		pand		mm0,[WrapMask]
		packssdw	mm3,mm4

		add			[pZBufferPtr],edi
		packssdw	mm5,mm6

		movq		[UV16],mm0
		shl			edi,1

		movd		mm4,[ecx].zf
		psllw		mm5,7

		movd		mm6,[ebx].zf
		mov			ebx,dword ptr[UV16+4]

		pfsub		mm4,mm6
		add			edi,[Dest]

		psrlq		mm0,[QShiftV]
		pfmul		mm4,mm7

		mov			esi,GBitPtr
		pf2id		mm6,mm6

		movq		[UV16V],mm0
		shr			ebx,16

		mov			eax,dword ptr[UV16V]
		add			esi,ebx

		mov			ecx,TexPal
		pf2id		mm4,mm4

		add			esi,eax
		xor			eax,eax

		push		ebp

GouraudLoopZBufferNoZWrite:
		movq		mm0,mm2				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw   UL|VL      |ZY   UZR|VZR     |

		mov			al,byte ptr[esi]
		paddd		mm2,mm1				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |

		mov			esi,GBitPtr

		pand		mm0,[WrapMask]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |ARGB
		movd		eax,mm6

		movq		[UV16],mm0			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |
		shr			eax,16

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		paddd		mm6,mm4

		mov			ebx,dword ptr[UV16+4]

		psrlq		mm0,[QShiftV]

		shr			ebx,16
		psllw		mm7,1

		movq		[UV16V],mm0
		add			esi,ebx

		mov			ebp,dword ptr[UV16V]
		pmulhw		mm7,mm5

		mov			ebx,pZBufferPtr
		add			esi,ebp

		packuswb	mm7,mm7
		paddw		mm5,mm3

		cmp			word ptr[ebx],ax
		jg			SkipPixelGouraudZBufferNoZWrite

		movd		[edi],mm7

SkipPixelGouraudZBufferNoZWrite:

		add			edi,4
		xor			eax,eax

		add			[pZBufferPtr],2

		dec			edx
		jge			GouraudLoopZBufferNoZWrite

		pop			ebp

		femms

GouraudReturnZBufferNoZWrite:

	pop		edi
	pop		esi
	pop		ecx
	pop		ebx

	cRet DrawScanLineGouraudZBufferNoZWrite_Asm3DNow
endProc DrawScanLineGouraudZBufferNoZWrite_Asm3DNow


;same with z write only (no compare)

cProc DrawScanLineGouraudNoZBufferZWrite_Asm3DNow, 8,<pLeft : dword, pRight : dword>

		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		ebx,pLeft
		mov     eax,[ebx].X
		mov     ecx,pRight
		mov     edx,[ecx].X
		sub     edx,eax
		jle		GouraudReturnNoZBufferZWrite

		mov		edi,[ebx].X
;		inc		edx

		femms

		movd		mm1,[ecx].vf
		movd		mm0,edx

		punpckldq	mm1,qword ptr[ecx].uf
		pi2fd		mm0,mm0

		movq		mm7,[GBL]
		movd		mm2,[ebx].vf

		pfrcp		mm0,mm0
		pfmul		mm1,mm7

		punpckldq	mm2,qword ptr[ebx].uf
		movd		mm3,[ecx].bf

		pfmul		mm2,mm7
		movq		mm7,mm0

		pfmul		mm0,[QFixedScale]
		pfsub		mm1,mm2

		punpckldq	mm3,qword ptr[ecx].gf
		pfmul		mm1,mm0

		movd		mm4,[ecx].rf

		pfmul		mm2,[QFixedScale]
		pf2id		mm1,mm1

		movd		mm5,[ebx].bf
		pf2id		mm2,mm2

		movq		mm0,mm7

		punpckldq	mm5,qword ptr[ebx].gf
		pfmul		mm7,[Q128]

		movd		mm6,[ebx].rf

		pfsub		mm3,mm5
		pfsub		mm4,mm6

		pfmul		mm3,mm7
		pf2id		mm5,mm5

		pfmul		mm4,mm7
		movq		mm7,mm0

		pf2id		mm6,mm6

		movq		mm0,mm2
		pf2id		mm4,mm4

		pf2id		mm3,mm3
		shl			edi,1

		pand		mm0,[WrapMask]
		packssdw	mm3,mm4

		add			[pZBufferPtr],edi
		packssdw	mm5,mm6

		movq		[UV16],mm0
		shl			edi,1

		movd		mm4,[ecx].zf
		psllw		mm5,7

		movd		mm6,[ebx].zf
		mov			ebx,dword ptr[UV16+4]

		pfsub		mm4,mm6
		add			edi,[Dest]

		psrlq		mm0,[QShiftV]
		pfmul		mm4,mm7

		mov			esi,GBitPtr
		pf2id		mm6,mm6

		movq		[UV16V],mm0
		shr			ebx,16

		mov			eax,dword ptr[UV16V]
		add			esi,ebx

		mov			ecx,TexPal
		pf2id		mm4,mm4

		add			esi,eax
		xor			eax,eax

		push		ebp

GouraudLoopNoZBufferZWrite:
		movq		mm0,mm2				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw   UL|VL      |ZY   UZR|VZR     |

		mov			al,byte ptr[esi]
		paddd		mm2,mm1				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |

		mov			esi,GBitPtr

		pand		mm0,[WrapMask]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |ARGB
		movd		eax,mm6

		movq		[UV16],mm0			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |
		shr			eax,16

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		paddd		mm6,mm4

		mov			ebx,dword ptr[UV16+4]

		psrlq		mm0,[QShiftV]

		shr			ebx,16
		psllw		mm7,1

		movq		[UV16V],mm0
		add			esi,ebx

		mov			ebp,dword ptr[UV16V]
		pmulhw		mm7,mm5

		mov			ebx,pZBufferPtr
		add			esi,ebp

		packuswb	mm7,mm7
		paddw		mm5,mm3

		movd		[edi],mm7
		mov			word ptr[ebx],ax

		add			edi,4
		xor			eax,eax

		add			[pZBufferPtr],2

		dec			edx
		jge			GouraudLoopNoZBufferZWrite

		pop			ebp

		femms

GouraudReturnNoZBufferZWrite:

	pop		edi
	pop		esi
	pop		ecx
	pop		ebx

	cRet DrawScanLineGouraudNoZBufferZWrite_Asm3DNow
endProc DrawScanLineGouraudNoZBufferZWrite_Asm3DNow


;affine textured, gouraud, colorkeyed, zbuffered

cProc DrawScanLineGouraudZBufferTrans_Asm3DNow, 8,<pLeft : dword, pRight : dword>

		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		ebx,pLeft
		mov     eax,[ebx].X
		mov     ecx,pRight
		mov     edx,[ecx].X
		sub     edx,eax
		jle		GouraudReturnZBufferTrans

		mov		edi,[ebx].X
;		inc		edx

		femms

		movd		mm1,[ecx].vf
		movd		mm0,edx

		punpckldq	mm1,qword ptr[ecx].uf
		pi2fd		mm0,mm0

		movq		mm7,[GBL]
		movd		mm2,[ebx].vf

		pfrcp		mm0,mm0
		pfmul		mm1,mm7

		punpckldq	mm2,qword ptr[ebx].uf
		movd		mm3,[ecx].bf

		pfmul		mm2,mm7
		movq		mm7,mm0

		pfmul		mm0,[QFixedScale]
		pfsub		mm1,mm2

		punpckldq	mm3,qword ptr[ecx].gf
		pfmul		mm1,mm0

		movd		mm4,[ecx].rf

		pfmul		mm2,[QFixedScale]
		pf2id		mm1,mm1

		movd		mm5,[ebx].bf
		pf2id		mm2,mm2

		movq		mm0,mm7

		punpckldq	mm5,qword ptr[ebx].gf
		pfmul		mm7,[Q128]

		movd		mm6,[ebx].rf

		pfsub		mm3,mm5
		pfsub		mm4,mm6

		pfmul		mm3,mm7
		pf2id		mm5,mm5

		pfmul		mm4,mm7
		movq		mm7,mm0

		pf2id		mm6,mm6

		movq		mm0,mm2
		pf2id		mm4,mm4

		pf2id		mm3,mm3
		shl			edi,1

		pand		mm0,[WrapMask]
		packssdw	mm3,mm4

		add			[pZBufferPtr],edi
		packssdw	mm5,mm6

		movq		[UV16],mm0
		shl			edi,1

		movd		mm4,[ecx].zf
		psllw		mm5,7

		movd		mm6,[ebx].zf
		mov			ebx,dword ptr[UV16+4]

		pfsub		mm4,mm6
		add			edi,[Dest]

		psrlq		mm0,[QShiftV]
		pfmul		mm4,mm7

		mov			esi,GBitPtr
		pf2id		mm6,mm6

		movq		[UV16V],mm0
		shr			ebx,16

		mov			eax,dword ptr[UV16V]
		add			esi,ebx

		mov			ecx,TexPal
		pf2id		mm4,mm4

		add			esi,eax
		xor			eax,eax

		push		ebp

GouraudLoopZBufferTrans:
		movq		mm0,mm2				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw   UL|VL      |ZY   UZR|VZR     |
		mov			al,byte ptr[esi]

		pand		mm0,[WrapMask]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |
		mov			esi,GBitPtr

		movq		[UV16],mm0			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		mov			ebx,dword ptr[UV16+4]
		psrlq		mm0,[QShiftV]

		shr			ebx,16
		movq		[UV16V],mm0

		add			esi,ebx
		mov			ebp,dword ptr[UV16V]

		add			esi,ebp

		cmp			al,0ffh
		je			SkipPixelGouraudZBufferTrans

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |ARGB
		movd		eax,mm6

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		shr			eax,16

		mov			ebx,pZBufferPtr
		psllw		mm7,1

		pmulhw		mm7,mm5

		packuswb	mm7,mm7

		cmp			word ptr[ebx],ax
		jg			SkipPixelGouraudZBufferTrans

		movd		[edi],mm7
		mov			word ptr[ebx],ax

SkipPixelGouraudZBufferTrans:

		add			edi,4
		xor			eax,eax

		add			[pZBufferPtr],2
		paddd		mm2,mm1				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |
		paddd		mm6,mm4
		paddw		mm5,mm3

		dec			edx
		jge			GouraudLoopZBufferTrans

		pop			ebp

		femms

GouraudReturnZBufferTrans:

	pop		edi
	pop		esi
	pop		ecx
	pop		ebx

	cRet DrawScanLineGouraudZBufferTrans_Asm3DNow
endProc DrawScanLineGouraudZBufferTrans_Asm3DNow


;same with zwrite only

cProc DrawScanLineGouraudNoZBufferZWriteTrans_Asm3DNow, 8,<pLeft : dword, pRight : dword>

		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		ebx,pLeft
		mov     eax,[ebx].X
		mov     ecx,pRight
		mov     edx,[ecx].X
		sub     edx,eax
		jle		GouraudReturnTransNoZBufferZWrite

		mov		edi,[ebx].X
;		inc		edx

		femms

		movd		mm1,[ecx].vf
		movd		mm0,edx

		punpckldq	mm1,qword ptr[ecx].uf
		pi2fd		mm0,mm0

		movq		mm7,[GBL]
		movd		mm2,[ebx].vf

		pfrcp		mm0,mm0
		pfmul		mm1,mm7

		punpckldq	mm2,qword ptr[ebx].uf
		movd		mm3,[ecx].bf

		pfmul		mm2,mm7
		movq		mm7,mm0

		pfmul		mm0,[QFixedScale]
		pfsub		mm1,mm2

		punpckldq	mm3,qword ptr[ecx].gf
		pfmul		mm1,mm0

		movd		mm4,[ecx].rf

		pfmul		mm2,[QFixedScale]
		pf2id		mm1,mm1

		movd		mm5,[ebx].bf
		pf2id		mm2,mm2

		movq		mm0,mm7

		punpckldq	mm5,qword ptr[ebx].gf
		pfmul		mm7,[Q128]

		movd		mm6,[ebx].rf

		pfsub		mm3,mm5
		pfsub		mm4,mm6

		pfmul		mm3,mm7
		pf2id		mm5,mm5

		pfmul		mm4,mm7
		movq		mm7,mm0

		pf2id		mm6,mm6

		movq		mm0,mm2
		pf2id		mm4,mm4

		pf2id		mm3,mm3
		shl			edi,1

		pand		mm0,[WrapMask]
		packssdw	mm3,mm4

		add			[pZBufferPtr],edi
		packssdw	mm5,mm6

		movq		[UV16],mm0
		shl			edi,1

		movd		mm4,[ecx].zf
		psllw		mm5,7

		movd		mm6,[ebx].zf
		mov			ebx,dword ptr[UV16+4]

		pfsub		mm4,mm6
		add			edi,[Dest]

		psrlq		mm0,[QShiftV]
		pfmul		mm4,mm7

		mov			esi,GBitPtr
		pf2id		mm6,mm6

		movq		[UV16V],mm0
		shr			ebx,16

		mov			eax,dword ptr[UV16V]
		add			esi,ebx

		mov			ecx,TexPal
		pf2id		mm4,mm4

		add			esi,eax
		xor			eax,eax

		push		ebp

GouraudLoopTransNoZBufferZWrite:
		movq		mm0,mm2				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw   UL|VL      |ZY   UZR|VZR     |
		mov			al,byte ptr[esi]

		pand		mm0,[WrapMask]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |
		mov			esi,GBitPtr

		movq		[UV16],mm0			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		mov			ebx,dword ptr[UV16+4]
		psrlq		mm0,[QShiftV]

		shr			ebx,16
		movq		[UV16V],mm0

		add			esi,ebx
		mov			ebp,dword ptr[UV16V]

		add			esi,ebp

		cmp			al,0ffh
		je			SkipPixelGouraudTransNoZBufferZWrite

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |ARGB
		movd		eax,mm6

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		shr			eax,16

		mov			ebx,pZBufferPtr
		psllw		mm7,1

		pmulhw		mm7,mm5

		packuswb	mm7,mm7

		cmp			word ptr[ebx],ax
		jg			SkipPixelGouraudTransNoZBufferZWrite

		movd		[edi],mm7
		mov			word ptr[ebx],ax

SkipPixelGouraudTransNoZBufferZWrite:

		add			edi,4
		xor			eax,eax

		add			[pZBufferPtr],2
		paddd		mm2,mm1				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |
		paddd		mm6,mm4
		paddw		mm5,mm3

		dec			edx
		jge			GouraudLoopTransNoZBufferZWrite

		pop			ebp

		femms

GouraudReturnTransNoZBufferZWrite:

	pop		edi
	pop		esi
	pop		ecx
	pop		ebx

	cRet DrawScanLineGouraudNoZBufferZWriteTrans_Asm3DNow
endProc DrawScanLineGouraudNoZBufferZWriteTrans_Asm3DNow


;zmask no z write

cProc DrawScanLineGouraudZBufferNoZWriteTrans_Asm3DNow, 8,<pLeft : dword, pRight : dword>

		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		ebx,pLeft
		mov     eax,[ebx].X
		mov     ecx,pRight
		mov     edx,[ecx].X
		sub     edx,eax
		jle		GouraudReturnTransZBufferNoZWrite

		mov		edi,[ebx].X
;		inc		edx

		femms

		movd		mm1,[ecx].vf
		movd		mm0,edx

		punpckldq	mm1,qword ptr[ecx].uf
		pi2fd		mm0,mm0

		movq		mm7,[GBL]
		movd		mm2,[ebx].vf

		pfrcp		mm0,mm0
		pfmul		mm1,mm7

		punpckldq	mm2,qword ptr[ebx].uf
		movd		mm3,[ecx].bf

		pfmul		mm2,mm7
		movq		mm7,mm0

		pfmul		mm0,[QFixedScale]
		pfsub		mm1,mm2

		punpckldq	mm3,qword ptr[ecx].gf
		pfmul		mm1,mm0

		movd		mm4,[ecx].rf

		pfmul		mm2,[QFixedScale]
		pf2id		mm1,mm1

		movd		mm5,[ebx].bf
		pf2id		mm2,mm2

		movq		mm0,mm7

		punpckldq	mm5,qword ptr[ebx].gf
		pfmul		mm7,[Q128]

		movd		mm6,[ebx].rf

		pfsub		mm3,mm5
		pfsub		mm4,mm6

		pfmul		mm3,mm7
		pf2id		mm5,mm5

		pfmul		mm4,mm7
		movq		mm7,mm0

		pf2id		mm6,mm6

		movq		mm0,mm2
		pf2id		mm4,mm4

		pf2id		mm3,mm3
		shl			edi,1

		pand		mm0,[WrapMask]
		packssdw	mm3,mm4

		add			[pZBufferPtr],edi
		packssdw	mm5,mm6

		movq		[UV16],mm0
		shl			edi,1

		movd		mm4,[ecx].zf
		psllw		mm5,7

		movd		mm6,[ebx].zf
		mov			ebx,dword ptr[UV16+4]

		pfsub		mm4,mm6
		add			edi,[Dest]

		psrlq		mm0,[QShiftV]
		pfmul		mm4,mm7

		mov			esi,GBitPtr
		pf2id		mm6,mm6

		movq		[UV16V],mm0
		shr			ebx,16

		mov			eax,dword ptr[UV16V]
		add			esi,ebx

		mov			ecx,TexPal
		pf2id		mm4,mm4

		add			esi,eax
		xor			eax,eax

		push		ebp

GouraudLoopTransZBufferNoZWrite:
		movq		mm0,mm2				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw   UL|VL      |ZY   UZR|VZR     |
		mov			al,byte ptr[esi]

		pand		mm0,[WrapMask]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |
		mov			esi,GBitPtr

		movq		[UV16],mm0			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		mov			ebx,dword ptr[UV16+4]
		psrlq		mm0,[QShiftV]

		shr			ebx,16
		movq		[UV16V],mm0

		add			esi,ebx
		mov			ebp,dword ptr[UV16V]

		add			esi,ebp

		cmp			al,0ffh
		je			SkipPixelGouraudTransZBufferNoZWrite

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |ARGB
		movd		eax,mm6

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		shr			eax,16

		mov			ebx,pZBufferPtr
		psllw		mm7,1

		pmulhw		mm7,mm5

		packuswb	mm7,mm7

		cmp			word ptr[ebx],ax
		jg			SkipPixelGouraudTransZBufferNoZWrite

		movd		[edi],mm7

SkipPixelGouraudTransZBufferNoZWrite:

		add			edi,4
		xor			eax,eax

		add			[pZBufferPtr],2
		paddd		mm2,mm1				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |
		paddd		mm6,mm4
		paddw		mm5,mm3

		dec			edx
		jge			GouraudLoopTransZBufferNoZWrite

		pop			ebp

		femms

GouraudReturnTransZBufferNoZWrite:

	pop		edi
	pop		esi
	pop		ecx
	pop		ebx

	cRet DrawScanLineGouraudZBufferNoZWriteTrans_Asm3DNow
endProc DrawScanLineGouraudZBufferNoZWriteTrans_Asm3DNow


;zbuffered lightmap combine routine
;the zbuffering in the inner loop uses a method i came up
;with that I call the trashcan method.  It always does a 
;write, but uses flags to look up a pointer, either to junk
;or to a real zbuffer.  Same for the screen write
;it's very bizzare, but it's quick.  no jumps
;all the zbuffering for perspective correct stuff suffers
;from inaccuracy when the z delta is negative
;this is probably more signed unsigned mmx problems messing
;with me.  I haven't had time to fix it yet

cProc DrawSpan32_AsmLitZBuffer3DNow, 12,<x1 : dword, x2 : dword, y : dword>

		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		eax,x1
		mov		ecx,x2
		sub		ecx,eax
		jle		Return128z

;		femms

;		inc		ecx
		mov		ebx,offset ClientWindow
		mov		eax, y
		mov		edi,[ebx].Buffer
		mov		esi,x1
		imul	eax, [ebx].PixelPitch
		shl		esi,2
		mov		edx,ZBuffer
		add		eax,esi
		add		edi, eax
		shr		eax,1
;		inc		eax
;		inc		eax
		add		edx,eax
		mov		[Dest],edx
		mov		eax,offset QZCan
		mov		ebx,offset SCan
		mov		[eax],ebx
		mov		[eax+4],edx
		mov		eax,offset QDibCan
		mov		[eax],ebx
		mov		[eax+4],edi
		mov		ebx,offset QDibCan
		mov		eax,offset QDibOrCan
		mov		[eax],ebx
		mov		[eax+4],ebx
		mov		ebx,offset QZCan
		mov		eax,offset QZOrCan
		mov		[eax],ebx
		mov		[eax+4],ebx


		mov		eax,ecx
		shr		ecx,4
		and		eax,15
		jnz		@f
		dec		ecx
		mov		eax,16
@@:

		mov		[NumASpans],ecx
		mov		[RemainingCount],eax
										;   mm0      mm1      mm2      mm3      mm4      mm5      mm6      mm7
		movd		mm0,x1				;    |x       |        |        |        |        |        |        |
		movq		mm2,[UVDivZStepX]	;    |x       |        |UZdX    |        |        |        |        |

		movd		mm1,y				;    |x       |y       |UZdX    |        |        |        |        |
		movq		mm3,[UVDivZStepY]	;    |x       |y       |UZdX    |UZdY    |        |        |        |

		punpckldq	mm0,mm0				;   x|x       |y       |UZdX    |UZdY    |        |        |        |
		punpckldq	mm1,mm1				;   x|x      y|y   UZdX|VZdX    |UZdY    |        |        |        |

		pi2fd		mm0,mm0
		pi2fd		mm1,mm1

		pfmul		mm2,mm0				;   x|x      y|y    UZX|VZX UZdY|VZdY    |        |        |        |
		movd		mm4,[ZiStepX]		;   x|x      y|y    UZX|VZX UZdY|VZdY    |ZdX     |        |        |

		punpckldq	mm4,mm4
		pfmul		mm4,[QZBufferPrec]
		pf2id		mm4,mm4
		movq		[QZDelta],mm4

		movd		mm4,[ZiStepX]		;   x|x      y|y    UZX|VZX UZdY|VZdY    |ZdX     |        |        |
		pfmul		mm3,mm1				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZdX     |        |        |
		movq		mm6,[UVDivZOrigin]	;   x|x      y|y    UZX|VZX  UZY|VZY     |ZdX     |        |UZO     |

		movd		mm5,[ZiStepY]		;   x|x      y|y    UZX|VZX  UZY|VZY     |ZdX     |ZdY     |UZO     |
		pfmul		mm4,mm0				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX      |ZdY     |UZO     |

		pfmul		mm5,mm1				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX      |ZY   UZO|VZO     |
		movd		mm7,[ZiOrigin]

		pfadd		mm6,mm2				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX      |ZY  UZXS|VZXS    |
		pfadd		mm4,mm7				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZXS     |ZY  UZXS|VZXS    |

		pfadd		mm6,mm3				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZXS     |ZY    UZ|VZ      |
		pfadd		mm4,mm5				;   x|x      y|y    UZX|VZX  UZY|VZY     |Zi      |ZY    UZ|VZ      |

		movq		mm7,mm4
		movq		mm1,[QZDelta]
		punpckldq	mm7,mm7
		pfmul		mm7,[QZBufferPrec]
		pf2id		mm7,mm7
		movq		mm0,mm7
		paddd		mm7,mm1
		punpckldq	mm0,mm7
		paddd		mm7,mm1
		movq		[QZVal32_0],mm0
		movq		mm0,mm7
		paddd		mm7,mm1
		punpckldq	mm0,mm7
		movq		[QZVal32_1],mm0
		pslld		mm1,2
		movd		mm7,[Zi16StepX]
		movq		[QZDelta],mm1
		
		pfrcp		mm0,mm4				;  ZL|ZL     y|y    UZX|VZX  UZY|VZY     |Zi      |ZY    UZ|VZ      |
		movq		mm1,mm6				;  ZL|ZL    UZ|VZ   UZX|VZX  UZY|VZY     |Zi      |ZY    UZ|VZ      |

		pfadd		mm4,mm7				;  ZL|ZL    UZ|VZ   UZX|VZX  UZY|VZY     |ZRi     |ZY    UZ|VZ      |
		pfmul		mm1,mm0				;  ZL|ZL    UL|VL   UZX|VZX  UZY|VZY     |ZRi     |ZY    UZ|VZ      |

		pfrcp		mm2,mm4				;  ZL|ZL    UL|VL    ZR|ZR   UZY|VZY     |ZRi     |ZY    UZ|VZ      |
		pfadd		mm6,[UVDivZ16StepX] ;  ZL|ZL    UL|VL    ZR|ZR   UZY|VZY     |ZRi     |ZY   UZR|VZR     |

		pfmul		mm2,mm6				;  ZL|ZL    UL|VL    UR|VR   UZY|VZY     |ZRi     |ZY   UZR|VZR     |
		movq		mm0,mm1				;  UL|VL    UL|VL    UR|VR   UZY|VZY     |ZRi     |ZY   UZR|VZR     |

		movq		mm3,mm2				; ULi|VLi	UL|VL    UR|UR    UR|UR      |ZRi     |ZY   UZR|VZR     |
		pfmul		mm0,[QFixedScale]	; UL6|VL6	UL|VL    UR|UR    UR|UR      |ZRi     |ZY   UZR|VZR     |

		pfmul		mm3,[QFixedScale]	; UL6|VL6	UL|VL   UR6|UR6  UR6|UR6     |ZRi     |ZY   UZR|VZR     |
		pf2id		mm0,mm0				;UL6i|VL6i  UL|VL   UR6|VR6  UR6|VR6     |ZRi     |ZY   UZR|VZR     |

		pf2id		mm3,mm3				;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR     |

		test	ecx,ecx
		jz		HandleLeftoverPixels128z

SpanLoop128z:
		;use float uv for lightmap uv
		movq		mm7,[GLMapMulUV]	;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR   LU|LV
		movq		mm5,mm1

		pfmul		mm7,mm1				;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR  ULL|VLL
		pfmul		mm5,[QFixedScale]

		pfmul		mm7,[QFixedScale]
		pf2id		mm5,mm5

		pf2id		mm7,mm7				; ULw|VLw   DU|DV    UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |
		paddd		mm5,[UVAdjust]

		pfsubr		mm1,mm2				; ULw|VLw   DU|DV    UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |
		paddd		mm5,[UVAdjust2]

		movq		[UVL16],mm7
		movq		[UVLeft],mm5

		movd		mm7,[Zi16StepX]
		movq		mm0,mm5

		pfmul		mm1,[QFixedScale16]	; ULw|VLw DU16|DV16  UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |
		;right side stuff becomes left	; ULw|VLw DU16|DV16  UL|VL   URw|VRw     |ZLi     |ZY   UZL|VZL     |
		pfadd		mm4,mm7				;ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZL|VZL     |

		pand		mm5,[WrapMask]
		pf2id		mm1,mm1				; ULw|VLw DU16|DV16  UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |

		movq		mm7,[UVAdjustL]
		movq		[UV16],mm5

		pfadd		mm6,[UVDivZ16StepX]	; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |
		paddd		mm7,[UVL16]

		psrlq		mm5,[QShiftV]
		movq		[UVL16],mm7

		movq		[UV16V],mm5

		; Clamp U/V
		mov		ebx,dword ptr[UVL16+4]
		cmp		ebx,MaxU
		jle		TryClampU0128z
		mov		ecx,MaxU
		mov		dword ptr[UVL16+4],ecx
		jmp		NoClampU0128z

TryClampU0128z:
		cmp		ebx,0
		jge		NoClampU0128z
		mov		dword ptr[UVL16+4],0
NoClampU0128z:
		mov		eax,dword ptr[UVL16]
		cmp		eax,MaxV
		jle		TryClampV0128z
		mov		ecx,MaxV
		mov		dword ptr[UVL16],ecx
		jmp		NoClampV0128z

TryClampV0128z:
		cmp		eax,0
		jge		NoClampV0128z
		mov		dword ptr[UVL16],0

NoClampV0128z:
		movq	mm7,[UVL16]
		movq	mm5,[UVL16]

		movq	[UVLeftW],mm3
		psrad	mm7,[QGMip4_8]

		psrld	mm5,[QGMip20]
		pand	mm7,[LMapMask8]

		movq	[UVL16],mm5
		movq	mm3,mm7

		punpckhwd	mm7,mm7
		mov		eax,dword ptr[UVL16]

		movq	mm5,[Zero]
		imul	eax,[GLightWidth]

		movq	[UVZ],mm6
		punpcklwd	mm3,mm3

		add		eax,dword ptr[UVL16+4]
		movq	mm6,[Zero]

		punpckldq	mm7,mm7
		lea		eax,[2*eax+eax]

		punpckldq	mm3,mm3
		add		eax,[GLightData]

		;bilininterpolate to get good color
		punpcklbw	mm6,[eax+3]
		mov			ecx,[GLightWidth]

		punpcklbw	mm5,[eax]
		psrlw		mm6,8

		psrlw		mm5,8
		add			eax,ecx

		psubw		mm6,mm5
		add			eax,ecx

		movq		[UVLeft],mm2
		psllw		mm5,8

		add			eax,ecx

		movq		mm2,[Zero]
		pmullw		mm6,mm7		; B|B

		punpcklbw	mm2,[eax+3]
		paddw		mm6,mm5

		psrlw		mm2,8
		movq		mm5,[Zero]

		mov			ebx,dword ptr[UV16+4]
		punpcklbw	mm5,[eax]

		mov			eax,dword ptr[UV16V]
		psrlw		mm5,8

		psubw		mm2,mm5
		mov			esi,GBitPtr

		psllw		mm5,8
		pmullw		mm2,mm7

		movq		mm7,mm6
		paddw		mm2,mm5

		psrlw		mm6,8
		psrlw		mm2,8

		add			esi,eax
		psubw		mm2,mm6

		shr			ebx,16
		pmullw		mm2,mm3

		pfrcp		mm5,mm4
		paddw		mm7,mm2

		movq		[ZIR],mm4
		psrlw		mm7,8

		xor			eax,eax
		movq		mm2,mm7		;make ABGR ARGB

		mov			ecx,TexPal
		psllq		mm2,16

		push		ebp
		punpckhwd	mm7,mm2			;BAGB

		mov			ebp,[Dest]
		psrlq		mm2,16

		add			esi,ebx
		punpckldq	mm7,mm2


		;grab zbuffer values
		movq		mm2,[QZVal32_0]
		movq		mm3,[QZVal32_1]

		movq		mm6,mm7
		psrld		mm2,17

		psrld		mm3,17
;		psllq		mm6,8
;		psrlw		mm6,8

		packssdw	mm2,mm3

		pslld		mm2,1
		movq		[QZVal],mm2


		movq		mm2,[QZVal]
		paddd		mm0,mm1				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |

		mov			al,byte ptr[esi]
		movq		mm4,mm0				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw   UL|VL      |ZY   UZR|VZR     |

		mov			esi,GBitPtr
		pcmpgtw		mm2,[ebp]

		pand		mm4,[WrapMask]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |
		psrlw		mm2,15

		movq		[UV16],mm4			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |
		psllw		mm2,2

		psrlq		mm4,[QShiftV]
		movq		[GBL],mm2

		movq		[UV16V],mm4
		movq		mm3,mm2

		movq		[UVR],mm5
		punpcklwd	mm3,[Zero]

		mov			edx,dword ptr[UV16V]
		paddd		mm3,[QZOrCan]

		mov			ebx,dword ptr[UV16+4]
		movq		[QZOut],mm3

		movq		mm3,mm2

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |ARGB
		punpcklwd	mm3,[Zero]

		mov			edi,dword ptr[QZOut]
		paddd		mm3,[QDibOrCan]

		shr			ebx,16
		movq		[QDibOut],mm3

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		mov			edi,dword ptr[edi]

		xor			eax,eax
		add			esi,edx

		pmullw		mm7,mm6
		mov			ax,word ptr[QZVal]

		paddd		mm0,mm1
		mov			word ptr[edi],ax

		mov			edi,dword ptr[QDibOut]
		add			esi,ebx

		mov			edi,dword ptr[edi]

		psrlw		mm7,8
		xor			eax,eax

		packuswb	mm7,mm7
		mov			al,byte ptr[esi]

		movd		[edi],mm7
		movq		mm4,mm0

		pand		mm4,[WrapMask]
		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB

		movq		[UV16],mm4
		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB

		psrlq		mm4,[QShiftV]
		mov			ebx,dword ptr[UV16+4]

		movq		[UV16V],mm4
		pmullw		mm7,mm6

		mov			edx,dword ptr[UV16V]

		shr			ebx,16
		psrlw		mm7,8

		mov			esi,GBitPtr
		mov			edi,dword ptr[QZOut+4]

		add			esi,edx
		xor			eax,eax

		mov			edi,dword ptr[edi]
		paddd		mm0,mm1

		packuswb	mm7,mm7				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw ARGB|ARGB UZR|VZR   AR|GB
		mov			ax,word ptr[QZVal+2]

		add			esi,ebx
		mov			word ptr[edi+2],ax

		xor			eax,eax
		mov			edi,dword ptr[QDibOut+4]

		mov			al,byte ptr[esi]
		movq		mm4,mm0

		mov			edi,dword ptr[edi]
		pand		mm4,[WrapMask]

		mov			esi,GBitPtr
		movd		[edi+4],mm7

		movq		mm3,mm2
		movq		[UV16],mm4

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB
		psrlq		mm4,[QShiftV]

		punpckhwd	mm3,[Zero]
		movq		[UV16V],mm4

		mov			ebx,dword ptr[UV16+4]
		paddd		mm3,[QZorCan]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		movq		[QZOut],mm3

		movq		mm3,mm2

		pmullw		mm7,mm6
		mov			edi,dword ptr[QZOut]

		xor			eax,eax
		punpckhwd	mm3,[Zero]

		mov			edi,dword ptr[edi]
		psrlw		mm7,8

		mov			ax,word ptr[QZVal+4]
		paddd		mm3,[QDibOrCan]

		mov			word ptr[edi+4],ax
		movq		[QDibOut],mm3

		packuswb	mm7,mm7
		mov			edi,dword ptr[QDibOut]

		paddd		mm0,mm1
		mov			edi,dword ptr[edi]
		mov			edx,dword ptr[UV16V]

		movd		[edi+8],mm7
		movq		mm4,mm0

		shr			ebx,16
		xor			eax,eax

		pand		mm4,[WrapMask]
		add			esi,edx

		paddd		mm0,mm1
		add			esi,ebx

		movq		[UV16],mm4
		mov			al,byte ptr[esi]

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		psrlq		mm4,[QShiftV]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		movq		[UV16V],mm4

		mov			ebx,dword ptr[UV16+4]
		mov			edx,dword ptr[UV16V]

		pmullw		mm7,mm6

		shr			ebx,16
		psrlw		mm7,8

		mov			esi,GBitPtr
		mov			edi,dword ptr[QZOut+4]

		add			esi,edx
		xor			eax,eax

		mov			edi,dword ptr[edi]

		packuswb	mm7,mm7				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw ARGB|ARGB UZR|VZR   AR|GB
		mov			ax,word ptr[QZVal+6]

		add			esi,ebx
		mov			word ptr[edi+6],ax

		;regrab z
		movq		mm2,[QZVal32_0]
		movq		mm3,[QZVal32_1]

		paddd		mm2,[QZDelta]
		paddd		mm3,[QZDelta]

		movq		[QZVal32_0],mm2
		movq		[QZVal32_1],mm3

		psrld		mm2,17
		psrld		mm3,17

		packssdw	mm2,mm3
		xor			eax,eax

		pslld		mm2,1

		mov			edi,dword ptr[QDibOut+4]

		movq		[QZVal],mm2

		mov			al,byte ptr[esi]
		movq		mm4,mm0

		mov			edi,dword ptr[edi]
		pand		mm4,[WrapMask]

		pcmpgtw		mm2,[ebp+8]
		movd		[edi+12],mm7

		movq		[UV16],mm4

		psrlw		mm2,15
		mov			esi,GBitPtr

		psrlq		mm4,[QShiftV]

		psllw		mm2,2
		movq		mm3,mm2

		movq		[UV16V],mm4

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB
		punpcklwd	mm3,[Zero]

		mov			ebx,dword ptr[UV16+4]
		paddd		mm3,[QZorCan]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		movq		[QZOut],mm3

		movq		mm3,mm2

		pmullw		mm7,mm6
		mov			edi,dword ptr[QZOut]

		xor			eax,eax
		punpcklwd	mm3,[Zero]

		mov			edi,dword ptr[edi]
		psrlw		mm7,8

		mov			ax,word ptr[QZVal]
		paddd		mm3,[QDibOrCan]

		mov			word ptr[edi+8],ax
		movq		[QDibOut],mm3

		packuswb	mm7,mm7
		mov			edi,dword ptr[QDibOut]

		paddd		mm0,mm1
		mov			edx,dword ptr[UV16V]

		mov			edi,dword ptr[edi]

		movd		[edi+16],mm7
		movq		mm4,mm0

		shr			ebx,16
		xor			eax,eax

		pand		mm4,[WrapMask]
		add			esi,edx

		paddd		mm0,mm1
		add			esi,ebx

		mov			al,byte ptr[esi]
		movq		[UV16],mm4

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		psrlq		mm4,[QShiftV]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		movq		[UV16V],mm4

		mov			ebx,dword ptr[UV16+4]
		mov			edx,dword ptr[UV16V]

		pmullw		mm7,mm6

		shr			ebx,16
		psrlw		mm7,8

		mov			esi,GBitPtr
		mov			edi,dword ptr[QZOut+4]

		add			esi,edx
		xor			eax,eax

		mov			edi,dword ptr[edi]

		packuswb	mm7,mm7				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw ARGB|ARGB UZR|VZR   AR|GB
		mov			ax,word ptr[QZVal+2]

		add			esi,ebx
		mov			word ptr[edi+10],ax

		xor			eax,eax
		mov			edi,dword ptr[QDibOut+4]

		mov			al,byte ptr[esi]
		movq		mm4,mm0

		mov			edi,dword ptr[edi]
		pand		mm4,[WrapMask]

		mov			esi,GBitPtr
		movd		[edi+20],mm7

		movq		mm3,mm2
		movq		[UV16],mm4

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB
		psrlq		mm4,[QShiftV]

		punpckhwd	mm3,[Zero]
		movq		[UV16V],mm4

		mov			ebx,dword ptr[UV16+4]
		paddd		mm3,[QZorCan]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		movq		[QZOut],mm3

		movq		mm3,mm2

		pmullw		mm7,mm6
		mov			edi,dword ptr[QZOut]

		xor			eax,eax
		punpckhwd	mm3,[Zero]

		mov			edi,dword ptr[edi]
		psrlw		mm7,8

		mov			ax,word ptr[QZVal+4]
		paddd		mm3,[QDibOrCan]

		mov			word ptr[edi+12],ax
		movq		[QDibOut],mm3

		packuswb	mm7,mm7
		mov			edi,dword ptr[QDibOut]

		paddd		mm0,mm1
		mov			edi,dword ptr[edi]

		mov			edx,dword ptr[UV16V]

		movd		[edi+24],mm7
		movq		mm4,mm0

		shr			ebx,16
		xor			eax,eax

		pand		mm4,[WrapMask]
		add			esi,edx

		paddd		mm0,mm1
		add			esi,ebx

		mov			al,byte ptr[esi]
		movq		[UV16],mm4

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		psrlq		mm4,[QShiftV]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		movq		[UV16V],mm4

		mov			ebx,dword ptr[UV16+4]
		mov			edx,dword ptr[UV16V]

		pmullw		mm7,mm6

		shr			ebx,16
		psrlw		mm7,8

		mov			esi,GBitPtr
		mov			edi,dword ptr[QZOut+4]

		add			esi,edx
		xor			eax,eax

		mov			edi,dword ptr[edi]

		packuswb	mm7,mm7				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw ARGB|ARGB UZR|VZR   AR|GB
		mov			ax,word ptr[QZVal+6]

		add			esi,ebx
		mov			word ptr[edi+14],ax

		;regrab z
		movq		mm2,[QZVal32_0]
		movq		mm3,[QZVal32_1]

		paddd		mm2,[QZDelta]
		paddd		mm3,[QZDelta]

		movq		[QZVal32_0],mm2
		movq		[QZVal32_1],mm3

		psrld		mm2,17
		psrld		mm3,17

		packssdw	mm2,mm3
		xor			eax,eax

		pslld		mm2,1

		mov			edi,dword ptr[QDibOut+4]

		movq		[QZVal],mm2
		mov			edi,dword ptr[edi]

		mov			al,byte ptr[esi]
		movq		mm4,mm0

		pand		mm4,[WrapMask]
		pcmpgtw		mm2,[ebp+16]

		movd		[edi+28],mm7
		movq		[UV16],mm4

		psrlw		mm2,15
		mov			esi,GBitPtr

		psrlq		mm4,[QShiftV]

		psllw		mm2,2
		movq		mm3,mm2

		movq		[UV16V],mm4

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB
		punpcklwd	mm3,[Zero]

		mov			ebx,dword ptr[UV16+4]
		paddd		mm3,[QZorCan]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		movq		[QZOut],mm3

		movq		mm3,mm2

		pmullw		mm7,mm6
		mov			edi,dword ptr[QZOut]

		xor			eax,eax
		punpcklwd	mm3,[Zero]
		mov			edi,dword ptr[edi]

		mov			ax,word ptr[QZVal]
		paddd		mm3,[QDibOrCan]
		psrlw		mm7,8

		mov			word ptr[edi+16],ax
		movq		[QDibOut],mm3

		packuswb	mm7,mm7
		mov			edi,dword ptr[QDibOut]

		paddd		mm0,mm1
		mov			edi,dword ptr[edi]
		mov			edx,dword ptr[UV16V]

		movd		[edi+32],mm7
		movq		mm4,mm0

		shr			ebx,16
		xor			eax,eax

		pand		mm4,[WrapMask]
		add			esi,edx

		paddd		mm0,mm1
		add			esi,ebx

		mov			al,byte ptr[esi]
		movq		[UV16],mm4

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		psrlq		mm4,[QShiftV]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		movq		[UV16V],mm4

		mov			ebx,dword ptr[UV16+4]
		mov			edx,dword ptr[UV16V]

		pmullw		mm7,mm6

		shr			ebx,16
		psrlw		mm7,8

		mov			esi,GBitPtr
		mov			edi,dword ptr[QZOut+4]

		add			esi,edx
		xor			eax,eax
		mov			edi,dword ptr[edi]

		packuswb	mm7,mm7				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw ARGB|ARGB UZR|VZR   AR|GB
		mov			ax,word ptr[QZVal+2]

		add			esi,ebx
		mov			word ptr[edi+18],ax

		xor			eax,eax
		mov			edi,dword ptr[QDibOut+4]

		mov			al,byte ptr[esi]
		movq		mm4,mm0
		mov			edi,dword ptr[edi]

		pand		mm4,[WrapMask]

		mov			esi,GBitPtr
		movd		[edi+36],mm7

		movq		mm3,mm2
		movq		[UV16],mm4

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB
		psrlq		mm4,[QShiftV]

		punpckhwd	mm3,[Zero]
		movq		[UV16V],mm4

		mov			ebx,dword ptr[UV16+4]
		paddd		mm3,[QZorCan]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		movq		[QZOut],mm3

		movq		mm3,mm2

		pmullw		mm7,mm6
		mov			edi,dword ptr[QZOut]

		xor			eax,eax
		punpckhwd	mm3,[Zero]

		mov			edi,dword ptr[edi]
		psrlw		mm7,8

		mov			ax,word ptr[QZVal+4]
		paddd		mm3,[QDibOrCan]

		mov			word ptr[edi+20],ax
		movq		[QDibOut],mm3

		packuswb	mm7,mm7
		mov			edi,dword ptr[QDibOut]

		paddd		mm0,mm1
		mov			edi,dword ptr[edi]
		mov			edx,dword ptr[UV16V]

		movd		[edi+40],mm7
		movq		mm4,mm0

		shr			ebx,16
		xor			eax,eax

		pand		mm4,[WrapMask]
		add			esi,edx

		paddd		mm0,mm1
		add			esi,ebx

		mov			al,byte ptr[esi]
		movq		[UV16],mm4

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		psrlq		mm4,[QShiftV]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		movq		[UV16V],mm4

		mov			ebx,dword ptr[UV16+4]
		mov			edx,dword ptr[UV16V]

		pmullw		mm7,mm6

		shr			ebx,16
		psrlw		mm7,8

		mov			esi,GBitPtr
		mov			edi,dword ptr[QZOut+4]

		add			esi,edx
		xor			eax,eax
		mov			edi,dword ptr[edi]

		packuswb	mm7,mm7				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw ARGB|ARGB UZR|VZR   AR|GB
		mov			ax,word ptr[QZVal+6]

		add			esi,ebx
		mov			word ptr[edi+22],ax

		;regrab z
		movq		mm2,[QZVal32_0]
		movq		mm3,[QZVal32_1]

		paddd		mm2,[QZDelta]
		paddd		mm3,[QZDelta]

		movq		[QZVal32_0],mm2
		movq		[QZVal32_1],mm3

		psrld		mm2,17
		psrld		mm3,17

		packssdw	mm2,mm3
		xor			eax,eax

		pslld		mm2,1

		mov			edi,dword ptr[QDibOut+4]

		movq		[QZVal],mm2
		mov			edi,dword ptr[edi]

		mov			al,byte ptr[esi]
		movq		mm4,mm0

		pand		mm4,[WrapMask]
		pcmpgtw		mm2,[ebp+24]

		movd		[edi+44],mm7
		movq		[UV16],mm4

		psrlw		mm2,15
		mov			esi,GBitPtr

		psrlq		mm4,[QShiftV]

		psllw		mm2,2
		movq		mm3,mm2

		movq		[UV16V],mm4

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB
		punpcklwd	mm3,[Zero]

		mov			ebx,dword ptr[UV16+4]
		paddd		mm3,[QZorCan]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		movq		[QZOut],mm3

		movq		mm3,mm2

		pmullw		mm7,mm6
		mov			edi,dword ptr[QZOut]

		xor			eax,eax
		punpcklwd	mm3,[Zero]

		mov			edi,dword ptr[edi]
		psrlw		mm7,8

		mov			ax,word ptr[QZVal]
		paddd		mm3,[QDibOrCan]

		mov			word ptr[edi+24],ax
		movq		[QDibOut],mm3

		packuswb	mm7,mm7
		mov			edi,dword ptr[QDibOut]

		paddd		mm0,mm1
		mov			edi,dword ptr[edi]
		mov			edx,dword ptr[UV16V]

		movd		[edi+48],mm7
		movq		mm4,mm0

		shr			ebx,16
		xor			eax,eax

		pand		mm4,[WrapMask]
		add			esi,edx

		paddd		mm0,mm1
		add			esi,ebx

		mov			al,byte ptr[esi]
		movq		[UV16],mm4

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		psrlq		mm4,[QShiftV]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		movq		[UV16V],mm4

		mov			ebx,dword ptr[UV16+4]
		mov			edx,dword ptr[UV16V]

		pmullw		mm7,mm6

		shr			ebx,16
		psrlw		mm7,8

		mov			esi,GBitPtr
		mov			edi,dword ptr[QZOut+4]

		add			esi,edx
		xor			eax,eax
		mov			edi,dword ptr[edi]

		packuswb	mm7,mm7				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw ARGB|ARGB UZR|VZR   AR|GB
		mov			ax,word ptr[QZVal+2]

		add			esi,ebx
		mov			word ptr[edi+26],ax

		xor			eax,eax
		mov			edi,dword ptr[QDibOut+4]

		mov			al,byte ptr[esi]
		movq		mm4,mm0

		mov			edi,dword ptr[edi]
		pand		mm4,[WrapMask]

		mov			esi,GBitPtr
		movd		[edi+52],mm7

		movq		mm3,mm2
		movq		[UV16],mm4

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB
		psrlq		mm4,[QShiftV]

		punpckhwd	mm3,[Zero]
		movq		[UV16V],mm4

		mov			ebx,dword ptr[UV16+4]
		paddd		mm3,[QZorCan]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		movq		[QZOut],mm3

		movq		mm3,mm2

		pmullw		mm7,mm6
		mov			edi,dword ptr[QZOut]

		xor			eax,eax
		punpckhwd	mm3,[Zero]
		mov			edi,dword ptr[edi]

		mov			ax,word ptr[QZVal+4]
		paddd		mm3,[QDibOrCan]
		psrlw		mm7,8

		mov			word ptr[edi+28],ax
		movq		[QDibOut],mm3

		packuswb	mm7,mm7
		mov			edi,dword ptr[QDibOut]

		paddd		mm0,mm1
		mov			edi,dword ptr[edi]
		mov			edx,dword ptr[UV16V]

		movd		[edi+56],mm7
		movq		mm4,mm0

		shr			ebx,16
		xor			eax,eax

		pand		mm4,[WrapMask]
		add			esi,edx

		paddd		mm0,mm1
		add			esi,ebx

		mov			al,byte ptr[esi]

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB

		pmullw		mm7,mm6

		psrlw		mm7,8

		mov			edi,dword ptr[QZOut+4]

		xor			eax,eax
		mov			edi,dword ptr[edi]

		packuswb	mm7,mm7				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw ARGB|ARGB UZR|VZR   AR|GB
		mov			ax,word ptr[QZVal+6]

		mov			word ptr[edi+30],ax

		mov			edi,dword ptr[QDibOut+4]

		pop			ebp
		mov			edi,dword ptr[edi]

		;regrab z
		movq		mm2,[QZVal32_0]
		movq		mm3,[QZVal32_1]

		paddd		mm2,[QZDelta]
		paddd		mm3,[QZDelta]

		movq		[QZVal32_0],mm2
		movq		[QZVal32_1],mm3

		movd		[edi+60],mm7



		; get corrected right side deltas ; ULw|VLw DU16|DV16 argb|ARGBagAG|rbRB    |ZRi   aA|rR   UZR|VZR aArR|gGbB

		movq		mm4,[ZIR]
		movq		mm6,[UVZ]

		movq		mm2,[UVR]
		movq		mm0,[UVLeftW]		; ULw|VLw DU16|DV16 argb|ARGBagAG|rbRB    |ZRi   aA|rR   UZR|VZR aArR|gGbB

		movq		mm1,[UVLeft]		; ULw|VLw   UL|VL   argb|ARGBagAG|rbRB ZRi|ZRi   aA|rR   UZR|VZR aArR|gGbB
		punpckldq	mm2,mm2				; ULw|VLw   UL|VL    ZRi|ZRi agAG|rbRB ZRi|ZRi   aA|rR   UZR|VZR aArR|gGbB

		pfmul		mm2,mm6				; ULw|VLw   UL|VL     UR|VR  agAG|rbRB ZRi|ZRi   aA|rR   UZR|VZR aArR|gGbB
		mov			eax,offset QZCan
		add			dword ptr[Dest],32

		add			dword ptr[eax+4],32
		mov			eax,offset QDibCan

		add			dword ptr[eax+4],64


		movq		mm3,mm2
		pfmul		mm3,[QFixedScale]

		pf2id		mm3,mm3

		dec		[NumASpans]			; dec num affine spans
		jnz		SpanLoop128z

HandleLeftoverPixels128z:



		mov		esi,GBitPtr


		cmp		[RemainingCount],0
		jz		FPUReturn128z

		mov			eax,[RemainingCount]
		mov			dword ptr[ZIR],eax
		mov			dword ptr[ZIR+4],eax
		
		movq		mm7,[GLMapMulUV]	;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR   LU|LV
		movq		mm5,mm1

		pfsub		mm6,[UVDivZ16StepX]
		pfmul		mm7,mm1				;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR  ULL|VLL

		pfmul		mm5,[QFixedScale]
		pi2fd		mm3,[ZIR]

		pfmul		mm7,[QFixedScale]
		pfmul		mm3,[UVDivZStepX]

		pf2id		mm5,mm5
		pfadd		mm3,mm6

		pi2fd		mm2,[ZIR]
		movd		mm6,[Zi16StepX]

		pf2id		mm7,mm7				; ULw|VLw   DU|DV    UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |
		pfsub		mm4,mm6

		paddd		mm5,[UVAdjust]

		movd		mm6,[ZiStepX]
		mov			ebx,[RemainingCount]

		pfmul		mm2,mm6

		shl			ebx,3

		pfadd		mm2,mm4
		punpckldq	mm4,mm4

		movq		[UVL16],mm7
		pfrcp		mm2,mm2

		pfmul		mm4,[QZBufferPrec]

		paddd		mm5,[UVAdjust2]
		pfmul		mm2,mm3

		movq		[UVLeft],mm5
		pfsubr		mm1,mm2				; ULw|VLw   DU|DV    UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |

		pfmul		mm1,qword ptr[QFixedScaleLUT+ebx]	; ULw|VLw DU16|DV16  UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |

		movq		mm0,mm5
		pand		mm5,[WrapMask]
		pf2id		mm1,mm1				; ULw|VLw DU16|DV16  UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |

		movq		[UV16],mm5
		movq		mm7,[UVAdjustL]

		psrlq		mm5,[QShiftV]
		paddd		mm7,[UVL16]

		movq		[UV16V],mm5
		movq		[UVL16],mm7

OnePixelSpan128z:
		; Clamp U/V
		mov		ebx,dword ptr[UVL16+4]
		cmp		ebx,MaxU
		jle		TryClampU1128z
		mov		ecx,MaxU
		mov		dword ptr[UVL16+4],ecx
		jmp		NoClampU1128z

TryClampU1128z:
		cmp		ebx,0
		jge		NoClampU1128z
		mov		dword ptr[UVL16+4],0
NoClampU1128z:
		mov		eax,dword ptr[UVL16]
		cmp		eax,MaxV
		jle		TryClampV1128z
		mov		ecx,MaxV
		mov		dword ptr[UVL16],ecx
		jmp		NoClampV1128z

TryClampV1128z:
		cmp		eax,0
		jge		NoClampV1128z
		mov		dword ptr[UVL16],0

NoClampV1128z:
		movq	mm7,[UVL16]
		movq	mm5,[UVL16]

		psrad	mm7,[QGMip4_8]
		psrld	mm5,[QGMip20]

		pand	mm7,[LMapMask8]
		movq	[UVL16],mm5

		movq	mm3,mm7
		punpckhwd	mm7,mm7

		mov		eax,dword ptr[UVL16]
		movq	mm5,[Zero]

		imul	eax,[GLightWidth]
		punpcklwd	mm3,mm3

		add		eax,dword ptr[UVL16+4]
		movq	mm6,[Zero]

		punpckldq	mm7,mm7
		lea		eax,[2*eax+eax]

		punpckldq	mm3,mm3
		add		eax,[GLightData]

		;bilininterpolate to get good color
		punpcklbw	mm6,[eax+3]
		mov			ecx,[GLightWidth]

		punpcklbw	mm5,[eax]
		psrlw		mm6,8

		psrlw		mm5,8
		add			eax,ecx

		psubw		mm6,mm5
		add			eax,ecx

		psllw		mm5,8
		movq		mm2,[Zero]
		add			eax,ecx

		pmullw		mm6,mm7		; B|B
		punpcklbw	mm2,[eax+3]

		paddw		mm6,mm5
		movq		mm5,[Zero]

		mov			ebx,dword ptr[UV16+4]
		punpcklbw	mm5,[eax]

		psrlw		mm2,8
		psrlw		mm5,8

		mov			eax,dword ptr[UV16V]
		psubw		mm2,mm5

		psllw		mm5,8
		pmullw		mm2,mm7

		movq		mm7,mm6
		paddw		mm2,mm5

		psrlw		mm6,8
		psrlw		mm2,8

		mov			esi,GBitPtr
		psubw		mm2,mm6

		shr			ebx,16
		pmullw		mm2,mm3

		add			esi,eax
		paddw		mm7,mm2

		psrlw		mm7,8

		add			esi,ebx
		movq		mm2,mm7		;make ABGR ARGB

		mov			ecx,TexPal
		psllq		mm2,16

		mov			eax,offset QDibCan
		punpckhwd	mm7,mm2			;BAGB

		psrlq		mm2,16
		push		ebp

		mov			edi,dword ptr[eax+4]
		punpckldq	mm7,mm2

		mov			ebp,dword ptr[Dest]
		movd		mm3,dword ptr[ZiStepX]

		xor			eax,eax

LeftoverLoop128z:
		paddd		mm0,mm1				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |
		pf2id		mm2,mm4

		mov			al,byte ptr[esi]
		movq		mm5,mm0				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw   UL|VL      |ZY   UZR|VZR     |

		pfadd		mm4,mm3
		mov			esi,GBitPtr

		pand		mm5,[WrapMask]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |
		psrld		mm2,16

		movd		mm6,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |ARGB
		movd		eax,mm2

		movq		[UV16],mm5			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |
		xor			eax,0

		punpcklbw	mm6,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB

		mov			ebx,dword ptr[UV16+4]

		psrlq		mm5,[QShiftV]

		shr			ebx,16

		movq		[UV16V],mm5
		add			esi,ebx

		mov			edx,dword ptr[UV16V]
		pmullw		mm6,mm7

		add			esi,edx
		cmp			ax,word ptr[ebp]
		jl			SkipPixelLitZ

		psrlw		mm6,8


		mov			word ptr[ebp],ax
		packuswb	mm6,mm6

		movd		[edi],mm6

SkipPixelLitZ:

		add			edi,4
		xor			eax,eax
		add			ebp,2

		dec		[RemainingCount]
		jge		LeftoverLoop128z

		pop			ebp

FPUReturn128z:


;		femms
Return128z:

	pop		edi
	pop		esi
	pop		ecx
	pop		ebx

	cRet DrawSpan32_AsmLitZBuffer3DNow
endProc DrawSpan32_AsmLitZBuffer3DNow



;this is the most used routine likely.  
;lightmap combine, zwrite

cProc DrawSpan32_AsmLitZWrite3DNow, 12,<x1 : dword, x2 : dword, y : dword>

		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		eax,x1
		mov		ecx,x2
		sub		ecx,eax
		jle		ReturnLit

;		femms

;		inc		ecx
		mov		ebx,offset ClientWindow
		mov		eax, y
		mov		edi,[ebx].Buffer
		mov		esi,x1
		imul	eax, [ebx].PixelPitch
		shl		esi,2
		mov		edx,ZBuffer
		add		eax,esi
		add		edi, eax
		shr		eax,1
		add		edx,eax
		mov		[Dest],edx


		mov		eax,ecx
		shr		ecx,4
		and		eax,15
		jnz		@f
		dec		ecx
		mov		eax,16
@@:

		mov		[NumASpans],ecx
		mov		[RemainingCount],eax
										;   mm0      mm1      mm2      mm3      mm4      mm5      mm6      mm7
		movd		mm0,x1				;    |x       |        |        |        |        |        |        |
		movq		mm2,[UVDivZStepX]	;    |x       |        |UZdX    |        |        |        |        |

		movd		mm1,y				;    |x       |y       |UZdX    |        |        |        |        |
		movq		mm3,[UVDivZStepY]	;    |x       |y       |UZdX    |UZdY    |        |        |        |

		punpckldq	mm0,mm0				;   x|x       |y       |UZdX    |UZdY    |        |        |        |
		punpckldq	mm1,mm1				;   x|x      y|y   UZdX|VZdX    |UZdY    |        |        |        |

		pi2fd		mm0,mm0
		pi2fd		mm1,mm1

		pfmul		mm2,mm0				;   x|x      y|y    UZX|VZX UZdY|VZdY    |        |        |        |
		movd		mm4,[ZiStepX]		;   x|x      y|y    UZX|VZX UZdY|VZdY    |ZdX     |        |        |

		punpckldq	mm4,mm4
		pfmul		mm4,[QZBufferPrec]
		pf2id		mm4,mm4
		movq		[QZDelta],mm4

		movd		mm4,[ZiStepX]		;   x|x      y|y    UZX|VZX UZdY|VZdY    |ZdX     |        |        |
		pfmul		mm3,mm1				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZdX     |        |        |
		movq		mm6,[UVDivZOrigin]	;   x|x      y|y    UZX|VZX  UZY|VZY     |ZdX     |        |UZO     |

		movd		mm5,[ZiStepY]		;   x|x      y|y    UZX|VZX  UZY|VZY     |ZdX     |ZdY     |UZO     |
		pfmul		mm4,mm0				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX      |ZdY     |UZO     |

		pfmul		mm5,mm1				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX      |ZY   UZO|VZO     |
		movd		mm7,[ZiOrigin]

		pfadd		mm6,mm2				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX      |ZY  UZXS|VZXS    |
		pfadd		mm4,mm7				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZXS     |ZY  UZXS|VZXS    |

		pfadd		mm6,mm3				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZXS     |ZY    UZ|VZ      |
		pfadd		mm4,mm5				;   x|x      y|y    UZX|VZX  UZY|VZY     |Zi      |ZY    UZ|VZ      |

;		movq		mm7,mm4
;		movq		mm1,[QZDelta]
;		punpckldq	mm7,mm7
;		pfmul		mm7,[QZBufferPrec]
;		pf2id		mm7,mm7
;		movq		mm0,mm7
;		paddd		mm7,mm1
;		movq		[QZVal32_0],mm0
;		paddd		mm7,mm1
;		movq		[QZVal32_1],mm7
;		pslld		mm1,2
;		movd		mm7,[Zi16StepX]
;		movq		[QZDelta],mm1

		movq		mm7,mm4
		movq		mm1,[QZDelta]
		punpckldq	mm7,mm7
		pfmul		mm7,[QZBufferPrec]
		pf2id		mm7,mm7
		movq		mm0,mm7
		paddd		mm7,mm1
		punpckldq	mm0,mm7
		paddd		mm7,mm1
		movq		[QZVal32_0],mm0
		movq		mm0,mm7
		paddd		mm7,mm1
		punpckldq	mm0,mm7
		movq		[QZVal32_1],mm0
		pslld		mm1,2
		movd		mm7,[Zi16StepX]
		movq		[QZDelta],mm1
		
		pfrcp		mm0,mm4				;  ZL|ZL     y|y    UZX|VZX  UZY|VZY     |Zi      |ZY    UZ|VZ      |
		movq		mm1,mm6				;  ZL|ZL    UZ|VZ   UZX|VZX  UZY|VZY     |Zi      |ZY    UZ|VZ      |

		pfadd		mm4,mm7				;  ZL|ZL    UZ|VZ   UZX|VZX  UZY|VZY     |ZRi     |ZY    UZ|VZ      |
		pfmul		mm1,mm0				;  ZL|ZL    UL|VL   UZX|VZX  UZY|VZY     |ZRi     |ZY    UZ|VZ      |

		pfrcp		mm2,mm4				;  ZL|ZL    UL|VL    ZR|ZR   UZY|VZY     |ZRi     |ZY    UZ|VZ      |
		pfadd		mm6,[UVDivZ16StepX] ;  ZL|ZL    UL|VL    ZR|ZR   UZY|VZY     |ZRi     |ZY   UZR|VZR     |

		pfmul		mm2,mm6				;  ZL|ZL    UL|VL    UR|VR   UZY|VZY     |ZRi     |ZY   UZR|VZR     |
		movq		mm0,mm1				;  UL|VL    UL|VL    UR|VR   UZY|VZY     |ZRi     |ZY   UZR|VZR     |

		movq		mm3,mm2				; ULi|VLi	UL|VL    UR|UR    UR|UR      |ZRi     |ZY   UZR|VZR     |
		pfmul		mm0,[QFixedScale]	; UL6|VL6	UL|VL    UR|UR    UR|UR      |ZRi     |ZY   UZR|VZR     |

		pfmul		mm3,[QFixedScale]	; UL6|VL6	UL|VL   UR6|UR6  UR6|UR6     |ZRi     |ZY   UZR|VZR     |
		pf2id		mm0,mm0				;UL6i|VL6i  UL|VL   UR6|VR6  UR6|VR6     |ZRi     |ZY   UZR|VZR     |

		pf2id		mm3,mm3				;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR     |

		test	ecx,ecx
		jz		HandleLeftoverPixelsLit

		movq		mm7,[GLMapMulUV]	;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR   LU|LV
		pfmul		mm7,mm1				;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR  ULL|VLL
		pfmul		mm7,[QFixedScale]
		pf2id		mm7,mm7				; ULw|VLw   DU|DV    UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |
		paddd		mm7,[UVAdjustL]
		movq		[UVL16],mm7

		; Clamp U/V
		mov		ebx,dword ptr[UVL16+4]

;		prefetch	[Dest]

		cmp		ebx,MaxU
		jle		TryClampU0Litp
		mov		ecx,MaxU
		mov		dword ptr[UVL16+4],ecx
		jmp		NoClampU0Litp

TryClampU0Litp:
		cmp		ebx,0
		jge		NoClampU0Litp
		mov		dword ptr[UVL16+4],0
NoClampU0Litp:
		mov		eax,dword ptr[UVL16]
		cmp		eax,MaxV
		jle		TryClampV0Litp
		mov		ecx,MaxV
		mov		dword ptr[UVL16],ecx
		jmp		NoClampV0Litp

TryClampV0Litp:
		cmp		eax,0
		jge		NoClampV0Litp
		mov		dword ptr[UVL16],0

NoClampV0Litp:

		prefetch	[GLightData]

		movq		mm7,[UVL16]
		movq		mm5,[UVL16]

		movq		[UVLeftW],mm3
		psrad		mm7,[QGMip4_8]

		psrld		mm5,[QGMip20]
		pand		mm7,[LMapMask8]

		movq		[UVL16],mm5
		movq		mm3,mm7

		punpckhwd	mm7,mm7
		mov			eax,dword ptr[UVL16]

		movq		mm5,[Zero]
		imul		eax,[GLightWidth]

		movq		[UVZ],mm6
		punpcklwd	mm3,mm3

		add			eax,dword ptr[UVL16+4]
		movq		mm6,[Zero]

		punpckldq	mm7,mm7
		lea			eax,[2*eax+eax]

		punpckldq	mm3,mm3
		add			eax,[GLightData]

		;bilininterpolate to get good color
		punpcklbw	mm6,[eax+3]
		mov			ecx,[GLightWidth]

		psrlw		mm6,8
		punpcklbw	mm5,[eax]

		add			eax,ecx
		psrlw		mm5,8

		movq		[UVLeft],mm2
		psubw		mm6,mm5

		movq		mm2,[Zero]
		psllw		mm5,8

		add			eax,ecx
		pmullw		mm6,mm7

		add			eax,ecx
		paddw		mm6,mm5

		movq		mm5,[Zero]
		punpcklbw	mm2,[eax+3]

		punpcklbw	mm5,[eax]
		psrlw		mm2,8

		psrlw		mm5,8
;		prefetch	[GBitPtr+256]

		psubw		mm2,mm5

		psllw		mm5,8

		pmullw		mm2,mm7
		movq		mm7,mm6

		paddw		mm2,mm5
		psrlw		mm6,8

		psrlw		mm2,8
		pfrcp		mm5,mm4

		psubw		mm2,mm6

		pmullw		mm2,mm3

		movq		mm3,[UVLeftW]
		paddw		mm7,mm2
		
		psrlw		mm7,8

		movq		mm2,mm7

		psllq		mm2,16

		punpckhwd	mm7,mm2

		psrlq		mm2,16

		punpckldq	mm7,mm2

		psllq		mm7,8

		movq		mm2,[UVLeft]

		movq		[ARL],mm7

		movq		mm6,[UVZ]

SpanLoopLit:
		;use float uv for lightmap uv
		movq		mm7,[GLMapMulUV]	;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR   LU|LV
		movq		mm5,mm1

		pfmul		mm7,mm2				;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR  ULL|VLL
		pfmul		mm5,[QFixedScale]

		pfmul		mm7,[QFixedScale]
		pf2id		mm5,mm5

		pf2id		mm7,mm7				; ULw|VLw   DU|DV    UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |
		paddd		mm5,[UVAdjust]

		pfsubr		mm1,mm2				; ULw|VLw   DU|DV    UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |
		paddd		mm5,[UVAdjust2]

		movq		[UVL16],mm7
		movq		[UVLeft],mm5

		movd		mm7,[Zi16StepX]
		movq		mm0,mm5

		pfmul		mm1,[QFixedScale16]	; ULw|VLw DU16|DV16  UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |
		;right side stuff becomes left	; ULw|VLw DU16|DV16  UL|VL   URw|VRw     |ZLi     |ZY   UZL|VZL     |
		pfadd		mm4,mm7				;ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZL|VZL     |

		pand		mm5,[WrapMask]
		pf2id		mm1,mm1				; ULw|VLw DU16|DV16  UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |

		movq		mm7,[UVAdjustL]
		movq		[UV16],mm5

		pfadd		mm6,[UVDivZ16StepX]	; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |
		paddd		mm7,[UVL16]

		psrlq		mm5,[QShiftV]
		movq		[UVL16],mm7

		movq		[UV16V],mm5

		prefetch	[edi]


		; Clamp U/V
		mov		ebx,dword ptr[UVL16+4]
		cmp		ebx,MaxU
		jle		TryClampU0Lit
		mov		ecx,MaxU
		mov		dword ptr[UVL16+4],ecx
		jmp		NoClampU0Lit

TryClampU0Lit:
		cmp		ebx,0
		jge		NoClampU0Lit
		mov		dword ptr[UVL16+4],0
NoClampU0Lit:
		mov		eax,dword ptr[UVL16]
		cmp		eax,MaxV
		jle		TryClampV0Lit
		mov		ecx,MaxV
		mov		dword ptr[UVL16],ecx
		jmp		NoClampV0Lit

TryClampV0Lit:
		cmp		eax,0
		jge		NoClampV0Lit
		mov		dword ptr[UVL16],0

NoClampV0Lit:
		movq	mm7,[UVL16]
		movq	mm5,[UVL16]

		movq	[UVLeftW],mm3
		psrad	mm7,[QGMip4_8]

		psrld	mm5,[QGMip20]
		pand	mm7,[LMapMask8]

		movq	[UVL16],mm5
		movq	mm3,mm7

		punpckhwd	mm7,mm7
		mov		eax,dword ptr[UVL16]

		movq	mm5,[Zero]
		imul	eax,[GLightWidth]

		movq	[UVZ],mm6
		punpcklwd	mm3,mm3

		add		eax,dword ptr[UVL16+4]
		movq	mm6,[Zero]

		punpckldq	mm7,mm7
		lea		eax,[2*eax+eax]

		punpckldq	mm3,mm3
		add		eax,[GLightData]

		mov			ecx,[GLightWidth]

		;bilininterpolate to get good color
		punpcklbw	mm6,[eax+3]

		punpcklbw	mm5,[eax]
		psrlw		mm6,8

		psrlw		mm5,8
		add			eax,ecx

		movq		[UVLeft],mm2
		psubw		mm6,mm5

		movq		mm2,[Zero]
		psllw		mm5,8

		add			eax,ecx
		pmullw		mm6,mm7

		add			eax,ecx
		paddw		mm6,mm5

		movq		mm5,[Zero]
		punpcklbw	mm2,[eax+3]

		punpcklbw	mm5,[eax]
		psrlw		mm2,8

		psrlw		mm5,8

		mov			eax,dword ptr[UV16V]
		psubw		mm2,mm5

		psllw		mm5,8
		pmullw		mm2,mm7

		movq		mm7,mm6
		paddw		mm2,mm5

		psrlw		mm6,8
		psrlw		mm2,8

		mov			ebx,dword ptr[UV16+4]
		psubw		mm2,mm6

		mov			esi,GBitPtr
		pmullw		mm2,mm3

		shr			ebx,16
		paddw		mm7,mm2

		add			esi,eax
		psrlw		mm7,8

		pfrcp		mm5,mm4
		movq		mm2,mm7

		add			esi,ebx
		psllq		mm2,16

		movq		[ZIR],mm4
		punpckhwd	mm7,mm2

		psrlq		mm2,16
		xor			eax,eax

		punpckldq	mm7,mm2
		mov			ecx,TexPal

		movq		mm4,[ARL]
		psllq		mm7,8

		prefetch	[ecx]

		movq		mm6,[ARL]
		psrlw		mm4,4

		movq		[ARL],mm7
		movq		mm3,mm7

		psrlw		mm3,4
		psubw		mm3,mm4

		push		ebp
		mov			ebp,[Dest]

		prefetch	[ebp]

		;grab zbuffer values
		movq		mm2,[QZVal32_0]
		movq		mm7,[QZVal32_1]

		psrld		mm2,17
		psrld		mm7,17

		prefetch	[esi]

		packssdw	mm2,mm7

		pslld		mm2,1
		movq		[ebp],mm2


		paddd		mm0,mm1				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |

		mov			al,byte ptr[esi]
		movq		mm4,mm0				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw   UL|VL      |ZY   UZR|VZR     |

		mov			esi,GBitPtr

		pand		mm4,[WrapMask]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movq		[UVR],mm5

		movq		[UV16],mm4			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |
		psrlq		mm4,[QShiftV]

		prefetch	[ecx+32]

		mov			ebx,dword ptr[UV16+4]
		movq		[UV16V],mm4

		mov			edx,dword ptr[UV16V]
		movq		mm2,mm6

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |ARGB
		psrlw		mm2,8

		shr			ebx,16
		add			esi,ebx

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		paddd		mm0,mm1

		add			esi,edx
		pmullw		mm7,mm2
		prefetch	[esi]

		xor			eax,eax
		movq		mm4,mm0

		psrlw		mm7,8

		mov			al,byte ptr[esi]
		paddw		mm6,mm3

		pand		mm4,[WrapMask]
		movq		mm2,mm6

		prefetch	[edi+32]

		movd		mm5,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		psrlw		mm2,8

		movq		[UV16],mm4			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |
		punpcklbw	mm5,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB

		psrlq		mm4,[QShiftV]
		mov			ebx,dword ptr[UV16+4]

		movq		[UV16V],mm4
		paddw		mm6,mm3

		mov			edx,dword ptr[UV16V]
		paddd		mm0,mm1

		pmullw		mm5,mm2
		mov			esi,GBitPtr

		movq		mm2,mm6
		psrlw		mm5,8

		shr			ebx,16
		psrlw		mm2,8

		add			esi,edx
		movq		mm4,mm0

		packuswb	mm7,mm5				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw ARGB|ARGB UZR|VZR   AR|GB
		add			esi,ebx

		movq		[edi],mm7
		prefetch	[esi]
		xor			eax,eax

		pand		mm4,[WrapMask]

		mov			al,byte ptr[esi]
		movq		[UV16],mm4

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB
		mov			esi,GBitPtr

		mov			ebx,dword ptr[UV16+4]
		psrlq		mm4,[QShiftV]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		paddd		mm0,mm1

		movq		[UV16V],mm4
		paddw		mm6,mm3

		pmullw		mm7,mm2
		xor			eax,eax

		mov			edx,dword ptr[UV16V]
		shr			ebx,16

		movq		mm2,mm6
		add			esi,edx

		psrlw		mm7,8
		add			esi,ebx

		movq		mm4,mm0
		psrlw		mm2,8

		pand		mm4,[WrapMask]
		mov			al,byte ptr[esi]

		movq		[UV16],mm4			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movd		mm5,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		mov			esi,GBitPtr

		punpcklbw	mm5,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		paddd		mm0,mm1

		psrlq		mm4,[QShiftV]
		paddw		mm6,mm3

		movq		[UV16V],mm4
		pmullw		mm5,mm2

		mov			ebx,dword ptr[UV16+4]
		movq		mm2,mm6

		prefetch	[ebp+32]
		psrlw		mm5,8

		mov			edx,dword ptr[UV16V]
		movq		mm4,mm0

		shr			ebx,16
		pand		mm4,[WrapMask]

		add			esi,edx
		packuswb	mm7,mm5

		movq		[UV16],mm4
		add			esi,ebx

		movq		[edi+8],mm7
		prefetch	[esi]
		xor			eax,eax

		psrlq		mm4,[QShiftV]
		psrlw		mm2,8

		mov			al,byte ptr[esi]
		mov			esi,GBitPtr

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB

		mov			ebx,dword ptr[UV16+4]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		paddd		mm0,mm1

		movq		[UV16V],mm4
		paddw		mm6,mm3

		pmullw		mm7,mm2
		xor			eax,eax

		mov			edx,dword ptr[UV16V]
		shr			ebx,16

		movq		mm2,mm6
		add			esi,edx

		psrlw		mm7,8
		add			esi,ebx

		movq		mm4,mm0
		psrlw		mm2,8

		pand		mm4,[WrapMask]
		mov			al,byte ptr[esi]
		movq		[UV16],mm4			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movd		mm5,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		mov			esi,GBitPtr

		punpcklbw	mm5,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		paddd		mm0,mm1

		psrlq		mm4,[QShiftV]
		paddw		mm6,mm3

		movq		[UV16V],mm4
		pmullw		mm5,mm2

		mov			ebx,dword ptr[UV16+4]

		psrlw		mm5,8

		mov			edx,dword ptr[UV16V]
		movq		mm4,mm0

		shr			ebx,16
		pand		mm4,[WrapMask]

		add			esi,edx
		packuswb	mm7,mm5

		movq		[UV16],mm4
		add			esi,ebx

		movq		[edi+16],mm7
		xor			eax,eax

		prefetch	[esi]

		;regrab z
		movq		mm2,[QZVal32_0]
		movq		mm7,[QZVal32_1]

		paddd		mm2,[QZDelta]
		paddd		mm7,[QZDelta]

		movq		[QZVal32_0],mm2
		movq		[QZVal32_1],mm7

		psrld		mm2,17
		psrld		mm7,17

		packssdw	mm2,mm7
		pslld		mm2,1

		movq		[ebp+8],mm2
		movq		mm2,mm6

		psrlq		mm4,[QShiftV]
		psrlw		mm2,8

		mov			al,byte ptr[esi]
		mov			esi,GBitPtr

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB

		mov			ebx,dword ptr[UV16+4]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		paddd		mm0,mm1

		movq		[UV16V],mm4
		paddw		mm6,mm3

		pmullw		mm7,mm2
		xor			eax,eax

		mov			edx,dword ptr[UV16V]
		shr			ebx,16

		movq		mm2,mm6
		add			esi,edx

		psrlw		mm7,8
		add			esi,ebx

		movq		mm4,mm0
		psrlw		mm2,8

		pand		mm4,[WrapMask]
		mov			al,byte ptr[esi]
		movq		[UV16],mm4			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movd		mm5,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		mov			esi,GBitPtr

		punpcklbw	mm5,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		paddd		mm0,mm1

		psrlq		mm4,[QShiftV]
		paddw		mm6,mm3

		movq		[UV16V],mm4
		pmullw		mm5,mm2

		mov			ebx,dword ptr[UV16+4]
		movq		mm2,mm6

		psrlw		mm5,8

		mov			edx,dword ptr[UV16V]
		movq		mm4,mm0

		shr			ebx,16
		pand		mm4,[WrapMask]

		add			esi,edx
		packuswb	mm7,mm5

		movq		[UV16],mm4
		add			esi,ebx

		movq		[edi+24],mm7
		prefetch	[esi]
		xor			eax,eax

		psrlq		mm4,[QShiftV]
		psrlw		mm2,8

		mov			al,byte ptr[esi]
		mov			esi,GBitPtr

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB

		mov			ebx,dword ptr[UV16+4]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		paddd		mm0,mm1

		movq		[UV16V],mm4
		paddw		mm6,mm3

		pmullw		mm7,mm2
		xor			eax,eax

		mov			edx,dword ptr[UV16V]
		shr			ebx,16

		movq		mm2,mm6
		add			esi,edx

		psrlw		mm7,8
		add			esi,ebx

		movq		mm4,mm0
		psrlw		mm2,8

		pand		mm4,[WrapMask]
		mov			al,byte ptr[esi]
		movq		[UV16],mm4			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movd		mm5,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		mov			esi,GBitPtr

		punpcklbw	mm5,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		paddd		mm0,mm1

		psrlq		mm4,[QShiftV]
		paddw		mm6,mm3

		movq		[UV16V],mm4
		pmullw		mm5,mm2

		mov			ebx,dword ptr[UV16+4]

		psrlw		mm5,8

		mov			edx,dword ptr[UV16V]
		movq		mm4,mm0

		shr			ebx,16
		pand		mm4,[WrapMask]

		add			esi,edx
		packuswb	mm7,mm5

		movq		[UV16],mm4
		add			esi,ebx

		movq		[edi+32],mm7
		xor			eax,eax

		prefetch	[esi]

		;regrab z
		movq		mm2,[QZVal32_0]
		movq		mm7,[QZVal32_1]

		paddd		mm2,[QZDelta]
		paddd		mm7,[QZDelta]

		movq		[QZVal32_0],mm2
		movq		[QZVal32_1],mm7

		psrld		mm2,17
		psrld		mm7,17

		packssdw	mm2,mm7
		pslld		mm2,1

		movq		[ebp+16],mm2

		psrlq		mm4,[QShiftV]
		movq		mm2,mm6

		mov			al,byte ptr[esi]
		mov			esi,GBitPtr

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB
		psrlw		mm2,8

		mov			ebx,dword ptr[UV16+4]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		paddd		mm0,mm1

		movq		[UV16V],mm4
		paddw		mm6,mm3

		pmullw		mm7,mm2
		xor			eax,eax

		mov			edx,dword ptr[UV16V]
		shr			ebx,16

		movq		mm2,mm6
		add			esi,edx

		psrlw		mm7,8
		add			esi,ebx

		movq		mm4,mm0
		psrlw		mm2,8

		pand		mm4,[WrapMask]
		mov			al,byte ptr[esi]
		movq		[UV16],mm4			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movd		mm5,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		mov			esi,GBitPtr

		punpcklbw	mm5,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		paddd		mm0,mm1

		psrlq		mm4,[QShiftV]
		paddw		mm6,mm3

		movq		[UV16V],mm4
		pmullw		mm5,mm2

		mov			ebx,dword ptr[UV16+4]
		movq		mm2,mm6

		psrlw		mm5,8

		mov			edx,dword ptr[UV16V]
		movq		mm4,mm0

		shr			ebx,16
		pand		mm4,[WrapMask]

		add			esi,edx
		packuswb	mm7,mm5

		movq		[UV16],mm4
		add			esi,ebx
		psrlw		mm2,8

		movq		[edi+40],mm7
		prefetch	[esi]
		xor			eax,eax

		psrlq		mm4,[QShiftV]

		mov			al,byte ptr[esi]
		mov			esi,GBitPtr

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB
		paddw		mm6,mm3

		mov			ebx,dword ptr[UV16+4]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		paddd		mm0,mm1

		movq		[UV16V],mm4

		pmullw		mm7,mm2
		xor			eax,eax

		mov			edx,dword ptr[UV16V]
		shr			ebx,16

		movq		mm2,mm6
		add			esi,edx

		psrlw		mm7,8
		add			esi,ebx

		movq		mm4,mm0
		psrlw		mm2,8

		pand		mm4,[WrapMask]
		mov			al,byte ptr[esi]
		movq		[UV16],mm4			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movd		mm5,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		mov			esi,GBitPtr

		punpcklbw	mm5,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		paddd		mm0,mm1

		psrlq		mm4,[QShiftV]
		paddw		mm6,mm3

		movq		[UV16V],mm4
		pmullw		mm5,mm2

		mov			ebx,dword ptr[UV16+4]

		psrlw		mm5,8

		mov			edx,dword ptr[UV16V]
		movq		mm4,mm0

		shr			ebx,16

		pand		mm4,[WrapMask]

		add			esi,edx
		packuswb	mm7,mm5

		movq		[UV16],mm4
		add			esi,ebx

		movq		[edi+48],mm7
		xor			eax,eax
		prefetch	[esi]

		;regrab z
		movq		mm2,[QZVal32_0]
		movq		mm7,[QZVal32_1]

		paddd		mm2,[QZDelta]
		paddd		mm7,[QZDelta]

		movq		[QZVal32_0],mm2
		movq		[QZVal32_1],mm7

		psrld		mm2,17
		psrld		mm7,17

		packssdw	mm2,mm7
		pslld		mm2,1

		movq		[ebp+24],mm2


		psrlq		mm4,[QShiftV]
		movq		mm2,mm6

		mov			al,byte ptr[esi]
		psrlw		mm2,8

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB
		mov			esi,GBitPtr

		mov			ebx,dword ptr[UV16+4]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB

		movq		[UV16V],mm4
		paddw		mm6,mm3

		pmullw		mm7,mm2
		xor			eax,eax

		mov			edx,dword ptr[UV16V]
		movq		mm2,mm6

		shr			ebx,16

		add			esi,edx
		psrlw		mm2,8

		add			esi,ebx

		mov			al,byte ptr[esi]

		psrlw		mm7,8
		movd		mm5,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB

		punpcklbw	mm5,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB

		pmullw		mm5,mm2
		add			dword ptr[Dest],32

		psrlw		mm5,8

		packuswb	mm7,mm5
		pop			ebp

		movq		[edi+56],mm7

		;regrab z
		movq		mm2,[QZVal32_0]
		movq		mm7,[QZVal32_1]

		paddd		mm2,[QZDelta]
		paddd		mm7,[QZDelta]

		movq		[QZVal32_0],mm2
		movq		[QZVal32_1],mm7

		; get corrected right side deltas ; ULw|VLw DU16|DV16 argb|ARGBagAG|rbRB    |ZRi   aA|rR   UZR|VZR aArR|gGbB

		movq		mm4,[ZIR]
		movq		mm6,[UVZ]

		movq		mm2,[UVR]
		movq		mm0,[UVLeftW]		; ULw|VLw DU16|DV16 argb|ARGBagAG|rbRB    |ZRi   aA|rR   UZR|VZR aArR|gGbB

		movq		mm1,[UVLeft]		; ULw|VLw   UL|VL   argb|ARGBagAG|rbRB ZRi|ZRi   aA|rR   UZR|VZR aArR|gGbB
		punpckldq	mm2,mm2				; ULw|VLw   UL|VL    ZRi|ZRi agAG|rbRB ZRi|ZRi   aA|rR   UZR|VZR aArR|gGbB

		pfmul		mm2,mm6				; ULw|VLw   UL|VL     UR|VR  agAG|rbRB ZRi|ZRi   aA|rR   UZR|VZR aArR|gGbB

		add		edi,64					; move screen pointer to start of next aspan

		movq		mm3,mm2
		pfmul		mm3,[QFixedScale]

		pf2id		mm3,mm3

		dec		[NumASpans]			; dec num affine spans
		jnz		SpanLoopLit

HandleLeftoverPixelsLit:


		mov		esi,GBitPtr


		mov			eax,[RemainingCount]
		mov			dword ptr[ZIR],eax
		mov			dword ptr[ZIR+4],eax
		
		movq		mm7,[GLMapMulUV]	;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR   LU|LV
		movq		mm5,mm1

		pfsub		mm6,[UVDivZ16StepX]
		pfmul		mm7,mm1				;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR  ULL|VLL

		pfmul		mm5,[QFixedScale]
		pi2fd		mm3,[ZIR]

		pfmul		mm7,[QFixedScale]
		pfmul		mm3,[UVDivZStepX]

		pf2id		mm5,mm5
		pfadd		mm3,mm6

		pi2fd		mm2,[ZIR]
		movd		mm6,[Zi16StepX]

		pf2id		mm7,mm7				; ULw|VLw   DU|DV    UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |
		pfsub		mm4,mm6

		movd		mm6,[ZiStepX]
		mov			ebx,[RemainingCount]

		pfmul		mm2,mm6

		shl			ebx,3

		pfadd		mm2,mm4
		paddd		mm5,[UVAdjust]

		movq		[UVL16],mm7
		pfrcp		mm2,mm2

		paddd		mm5,[UVAdjust2]
		pfmul		mm2,mm3

		punpckldq	mm4,mm4

		movq		[UVLeft],mm5
		pfsubr		mm1,mm2				; ULw|VLw   DU|DV    UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |

		pfmul		mm4,[QZBufferPrec]
		pfmul		mm1,qword ptr[QFixedScaleLUT+ebx]	; ULw|VLw DU16|DV16  UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |

		movq		mm0,mm5
		pand		mm5,[WrapMask]
		pf2id		mm1,mm1				; ULw|VLw DU16|DV16  UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |

		movq		[UV16],mm5
		movq		mm7,[UVAdjustL]

		psrlq		mm5,[QShiftV]
		paddd		mm7,[UVL16]

		movq		[UV16V],mm5
		movq		[UVL16],mm7


OnePixelSpanLit:
		; Clamp U/V
		mov		ebx,dword ptr[UVL16+4]
		cmp		ebx,MaxU
		jle		TryClampU1Lit
		mov		ecx,MaxU
		mov		dword ptr[UVL16+4],ecx
		jmp		NoClampU1Lit

TryClampU1Lit:
		cmp		ebx,0
		jge		NoClampU1Lit
		mov		dword ptr[UVL16+4],0
NoClampU1Lit:
		mov		eax,dword ptr[UVL16]
		cmp		eax,MaxV
		jle		TryClampV1Lit
		mov		ecx,MaxV
		mov		dword ptr[UVL16],ecx
		jmp		NoClampV1Lit

TryClampV1Lit:
		cmp		eax,0
		jge		NoClampV1Lit
		mov		dword ptr[UVL16],0

NoClampV1Lit:
		movq	mm7,[UVL16]
		movq	mm5,[UVL16]

		psrad	mm7,[QGMip4_8]
		psrld	mm5,[QGMip20]

		pand	mm7,[LMapMask8]
		movq	[UVL16],mm5

		movq	mm3,mm7
		punpckhwd	mm7,mm7

		mov		eax,dword ptr[UVL16]
		movq	mm5,[Zero]

		imul	eax,[GLightWidth]
		punpcklwd	mm3,mm3

		add		eax,dword ptr[UVL16+4]
		movq	mm6,[Zero]

		punpckldq	mm7,mm7
		lea		eax,[2*eax+eax]

		punpckldq	mm3,mm3
		add		eax,[GLightData]

		;bilininterpolate to get good color
		punpcklbw	mm6,[eax+3]
		mov			ecx,[GLightWidth]

		punpcklbw	mm5,[eax]
		psrlw		mm6,8

		psrlw		mm5,8
		add			eax,ecx

		movq		mm2,[Zero]
		psubw		mm6,mm5

		add			eax,ecx
		psllw		mm5,8

		add			eax,ecx
		pmullw		mm6,mm7		; B|B

		punpcklbw	mm2,[eax+3]
		paddw		mm6,mm5

		psrlw		mm2,8
		movq		mm5,[Zero]

		mov			ebx,dword ptr[UV16+4]
		punpcklbw	mm5,[eax]

		mov			eax,dword ptr[UV16V]
		psrlw		mm5,8

		mov			esi,GBitPtr
		psubw		mm2,mm5

		shr			ebx,16
		psllw		mm5,8

		add			esi,eax
		pmullw		mm2,mm7

		movq		mm7,mm6
		paddw		mm2,mm5

		psrlw		mm6,8
		psrlw		mm2,8

		add			esi,ebx
		mov			ecx,TexPal

		psubw		mm2,mm6
		xor			eax,eax

		pmullw		mm2,mm3

		paddw		mm7,mm2

		psrlw		mm7,8

		movq		mm2,mm7		;make ABGR ARGB

		psllq		mm2,16

		punpckhwd	mm7,mm2			;BAGB

		psrlq		mm2,16
		push		ebp

		punpckldq	mm7,mm2

		mov			ebp,dword ptr[Dest]
		movd		mm3,dword ptr[ZiStepX]

		xor			eax,eax

LeftoverLoopLit:
		paddd		mm0,mm1				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |
		pf2id		mm2,mm4

		mov			al,byte ptr[esi]
		movq		mm5,mm0				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw   UL|VL      |ZY   UZR|VZR     |

		pfadd		mm4,mm3
		mov			esi,GBitPtr

		pand		mm5,[WrapMask]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |
		psrlq		mm2,16

		movd		mm6,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |ARGB
		movd		eax,mm2

		movq		[UV16],mm5			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |
		xor			eax,0

		punpcklbw	mm6,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		mov			word ptr[ebp],ax

		mov			ebx,dword ptr[UV16+4]
		add			ebp,2

		psrlq		mm5,[QShiftV]

		shr			ebx,16

		movq		[UV16V],mm5
		pmullw		mm6,mm7

		add			esi,ebx
		mov			edx,dword ptr[UV16V]
		psrlw		mm6,8

		add			esi,edx
		packuswb	mm6,mm6

		xor			eax,eax
		movd		[edi],mm6

		add			edi,4

		dec		[RemainingCount]
		jge		LeftoverLoopLit

		pop			ebp

ReturnLit:

	pop		edi
	pop		esi
	pop		ecx
	pop		ebx

	cRet DrawSpan32_AsmLitZWrite3DNow
endProc DrawSpan32_AsmLitZWrite3DNow


;perspective correct gouraud with zwrite

cProc DrawSpan32_AsmGouraudZWrite3DNow, 36,<x1 : dword, x2 : dword, y : dword, r1 : dword, g1 : dword, b1 : dword, r2 : dword, g2 : dword, b2 : dword>

		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		eax,x1
		mov		ecx,x2
		sub		ecx,eax
		jle		ReturnLit

;		femms

;		inc		ecx
		mov		eax,y
		mov		edx,x1
		mov		ebx,offset ClientWindow
		shl		edx,2
		imul	eax,[ebx].PixelPitch
		mov		edi,[ebx].Buffer
		mov		esi,ZBuffer
		add		edx,eax
		mov		eax,ecx
		add		edi,edx
		shr		edx,1
		add		esi,edx
		mov		[Dest],esi
		mov		edx,ecx

		shr		ecx,4
		and		eax,15
		jnz		@f
		dec		ecx
		mov		eax,16
@@:

		mov		[NumASpans],ecx
		mov		[RemainingCount],eax

		;grab the left side lights
		movd		mm5,r1
		movd		mm4,b1

;		punpckldq	mm5,qword ptr[Zero]
		movd		mm6,g1

		pf2id		mm5,mm5
		punpckldq	mm4,mm6

		pf2id		mm4,mm4
		packssdw	mm4,mm5

		psllw		mm4,7

		movq		[ARL],mm4

										;   mm0      mm1      mm2      mm3      mm4      mm5      mm6      mm7
		movd		mm0,x1				;    |x       |        |        |        |        |        |        |
		movq		mm2,[UVDivZStepX]	;    |x       |    UZdX|VZdX    |        |        |        |        |

		movd		mm1,y				;    |x       |y   UZdX|VZdX    |        |        |        |        |
		movq		mm3,[UVDivZStepY]	;    |x       |y   UZdX|VZdXUZdY|VZdY    |        |        |        |

		punpckldq	mm0,mm0				;   x|x       |y   UZdX|VZdXUZdY|VZdY    |        |        |        |
		punpckldq	mm1,mm1				;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |        |        |        |

		movd		mm7,edx				;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |        |        |        |wid
		movd		mm5,b2				;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |        |b       |        |wid

		pi2fd		mm0,mm0				;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |        |b       |        |wid
		movd		mm6,b1				;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |        |b       |b       |wid

		pi2fd		mm7,mm7				;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |        |b       |b       |wid
		punpckldq	mm5,qword ptr[g2]	;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |       g|b       |b       |wid

		pi2fd		mm1,mm1				;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |       g|b       |b       |wid
		punpckldq	mm6,qword ptr[g1]	;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |       g|b      g|b       |wid

		pfmul		mm2,mm0				;   x|x      y|y    UZX|VZX UZdY|VZdY    |       g|b      g|b       |wid
		pfrcp		mm7,mm7				;   x|x      y|y    UZX|VZX UZdY|VZdY    |       g|b      g|b     dw|dw

		pfsub		mm5,mm6				;   x|x      y|y    UZX|VZX UZdY|VZdY    |      gd|bd     g|b     dw|dw
		movd		mm4,[r1]			;   x|x      y|y    UZX|VZX UZdY|VZdY    |r     gd|bd     g|b     dw|dw

		movd		mm6,[r2]			;   x|x      y|y    UZX|VZX UZdY|VZdY    |r     gd|bd     g|r     dw|dw
		pfmul		mm3,mm1				;   x|x      y|y    UZX|VZX  UZY|VZY     |r     gd|bd     g|r     dw|dw

		pfsub		mm6,mm4				;   x|x      y|y    UZX|VZX  UZY|VZY     |r     gd|bd     x|rd    dw|dw
		pfmul		mm7,[Q128]			;   x|x      y|y    UZX|VZX  UZY|VZY     |r     gd|bd     x|rd    DW|DW

		movd		mm4,[ZiStepX]		;   x|x      y|y    UZX|VZX  UZY|VZY     |ZdX   gd|bd     x|rd    DW|DW

		punpckldq	mm4,mm4
		pfmul		mm4,[QZBufferPrec]
		pf2id		mm4,mm4
		movq		[QZDelta],mm4

		movd		mm4,[ZiStepX]		;   x|x      y|y    UZX|VZX  UZY|VZY     |ZdX   gd|bd     x|rd    DW|DW
		pfmul		mm5,mm7				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZdX   GD|BD     x|rd    DW|DW

		pfmul		mm4,mm0				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX    GD|BD     x|rd    DW|DW
		pfmul		mm6,mm7				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX    GD|BD     x|RD    DW|DW

		pf2id		mm5,mm5
		pf2id		mm6,mm6

		packssdw	mm5,mm6
		movq		[RGBADelta],mm5

		movd		mm5,[ZiStepY]		;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX      |ZdY    x|RD    DW|DW
		movq		mm6,[UVDivZOrigin]	;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX      |ZdY  UZO|VZO   DW|DW

		pfmul		mm5,mm1				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX      |ZY   UZO|VZO   DW|DW
		movd		mm7,[ZiOrigin]		;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX      |ZY   UZO|VZO     |ZO

		pfadd		mm6,mm2				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX      |ZY  UZXS|VZXS    |ZO
		pfadd		mm4,mm7				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZXS     |ZY  UZXS|VZXS    |ZO

		pfadd		mm6,mm3				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZXS     |ZY    UZ|VZ      |ZO
		pfadd		mm4,mm5				;   x|x      y|y    UZX|VZX  UZY|VZY     |Zi      |ZY    UZ|VZ      |ZO

		movq		mm7,mm4
		movq		mm1,[QZDelta]
		punpckldq	mm7,mm7
		pfmul		mm7,[QZBufferPrec]
		pf2id		mm7,mm7
		movq		mm0,mm7
		paddd		mm7,mm1
		punpckldq	mm0,mm7
		paddd		mm7,mm1
		movq		[QZVal32_0],mm0
		movq		mm0,mm7
		paddd		mm7,mm1
		punpckldq	mm0,mm7
		movq		[QZVal32_1],mm0
		pslld		mm1,2
		movd		mm7,[Zi16StepX]
		movq		[QZDelta],mm1

		pfrcp		mm0,mm4				;  ZL|ZL     y|y    UZX|VZX  UZY|VZY     |Zi      |ZY    UZ|VZ      |ZdX16

		movq		mm1,mm6				;  ZL|ZL    UZ|VZ   UZX|VZX  UZY|VZY     |Zi      |ZY    UZ|VZ      |ZdX16
		pfadd		mm4,mm7				;  ZL|ZL    UZ|VZ   UZX|VZX  UZY|VZY     |ZRi     |ZY    UZ|VZ      |ZdX16

		pfmul		mm1,mm0				;  ZL|ZL    UL|VL   UZX|VZX  UZY|VZY     |ZRi     |ZY    UZ|VZ      |ZdX16
		pfrcp		mm2,mm4				;  ZL|ZL    UL|VL    ZR|ZR   UZY|VZY     |ZRi     |ZY    UZ|VZ      |ZdX16

		pfadd		mm6,[UVDivZ16StepX] ;  ZL|ZL    UL|VL    ZR|ZR   UZY|VZY     |ZRi     |ZY   UZR|VZR     |ZdX16
		pfmul		mm2,mm6				;  ZL|ZL    UL|VL    UR|VR   UZY|VZY     |ZRi     |ZY   UZR|VZR     |ZdX16

		movq		mm0,mm1				;  UL|VL    UL|VL    UR|VR   UZY|VZY     |ZRi     |ZY   UZR|VZR     |ZdX16
		movq		mm3,mm2				; ULi|VLi	UL|VL    UR|UR    UR|UR      |ZRi     |ZY   UZR|VZR     |ZdX16

		pfmul		mm0,[QFixedScale]	; UL6|VL6	UL|VL    UR|UR    UR|UR      |ZRi     |ZY   UZR|VZR     |ZdX16
		pfmul		mm3,[QFixedScale]	; UL6|VL6	UL|VL   UR6|UR6  UR6|UR6     |ZRi     |ZY   UZR|VZR     |ZdX16

		pf2id		mm0,mm0				;UL6i|VL6i  UL|VL   UR6|VR6  UR6|VR6     |ZRi     |ZY   UZR|VZR     |ZdX16
		pf2id		mm3,mm3				;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR     |ZdX16


		test	ecx,ecx
		jz		HandleLeftoverPixelsLit

		movq		mm7,[GLMapMulUV]	;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR   LU|LV
		pfmul		mm7,mm1				;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR  ULL|VLL
		pfmul		mm7,[QFixedScale]
		pf2id		mm7,mm7				; ULw|VLw   DU|DV    UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |
		paddd		mm7,[UVAdjustL]
		movq		[UVL16],mm7

		; Clamp U/V
		mov		ebx,dword ptr[UVL16+4]

		cmp		ebx,MaxU
		jle		TryClampU0Litp
		mov		ecx,MaxU
		mov		dword ptr[UVL16+4],ecx
		jmp		NoClampU0Litp

TryClampU0Litp:
		cmp		ebx,0
		jge		NoClampU0Litp
		mov		dword ptr[UVL16+4],0
NoClampU0Litp:
		mov		eax,dword ptr[UVL16]
		cmp		eax,MaxV
		jle		TryClampV0Litp
		mov		ecx,MaxV
		mov		dword ptr[UVL16],ecx
		jmp		NoClampV0Litp

TryClampV0Litp:
		cmp		eax,0
		jge		NoClampV0Litp
		mov		dword ptr[UVL16],0

NoClampV0Litp:

		pfrcp		mm5,mm4

SpanLoopLit:
		;use float uv for lightmap uv
		movq		mm7,[GLMapMulUV]	;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR   LU|LV
		movq		mm5,mm1

		pfmul		mm7,mm2				;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR  ULL|VLL
		pfmul		mm5,[QFixedScale]

		pfmul		mm7,[QFixedScale]
		pf2id		mm5,mm5

		pf2id		mm7,mm7				; ULw|VLw   DU|DV    UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |
		paddd		mm5,[UVAdjust]

		pfsubr		mm1,mm2				; ULw|VLw   DU|DV    UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |
		paddd		mm5,[UVAdjust2]

		movq		[UVL16],mm7
		movq		[UVLeft],mm5

		movd		mm7,[Zi16StepX]
		movq		mm0,mm5

		pfmul		mm1,[QFixedScale16]	; ULw|VLw DU16|DV16  UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |
		;right side stuff becomes left	; ULw|VLw DU16|DV16  UL|VL   URw|VRw     |ZLi     |ZY   UZL|VZL     |
		pfadd		mm4,mm7				;ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZL|VZL     |

		pand		mm5,[WrapMask]
		pf2id		mm1,mm1				; ULw|VLw DU16|DV16  UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |

		movq		mm7,[UVAdjustL]
		movq		[UV16],mm5

		pfadd		mm6,[UVDivZ16StepX]	; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |
		paddd		mm7,[UVL16]

		psrlq		mm5,[QShiftV]
		movq		[UVL16],mm7

		movq		[UV16V],mm5


		; Clamp U/V
		mov		ebx,dword ptr[UVL16+4]
		cmp		ebx,MaxU
		jle		TryClampU0Lit
		mov		ecx,MaxU
		mov		dword ptr[UVL16+4],ecx
		jmp		NoClampU0Lit

TryClampU0Lit:
		cmp		ebx,0
		jge		NoClampU0Lit
		mov		dword ptr[UVL16+4],0
NoClampU0Lit:
		mov		eax,dword ptr[UVL16]
		cmp		eax,MaxV
		jle		TryClampV0Lit
		mov		ecx,MaxV
		mov		dword ptr[UVL16],ecx
		jmp		NoClampV0Lit

TryClampV0Lit:
		cmp		eax,0
		jge		NoClampV0Lit
		mov		dword ptr[UVL16],0

NoClampV0Lit:
		movq		[UVLeftW],mm3
		mov			eax,dword ptr[UV16V]
		mov			ebx,dword ptr[UV16+4]
		movq		[ZIR],mm4
		mov			esi,GBitPtr
		shr			ebx,16
		movq		[UVZ],mm6
		add			esi,eax
		add			esi,ebx
		movq		[UVLeft],mm2

		mov			ecx,TexPal
		pfrcp		mm5,mm4
		movq		mm3,[RGBADelta]
		xor			eax,eax
		movq		mm6,[ARL]

		push		ebp
		mov			ebp,[Dest]

		;grab zbuffer values
		movq		mm2,[QZVal32_0]
		movq		mm7,[QZVal32_1]

		psrld		mm2,17
		psrld		mm7,17

		packssdw	mm2,mm7

		pslld		mm2,1
		movq		[ebp],mm2


		paddd		mm0,mm1				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |

		mov			al,byte ptr[esi]
		movq		mm4,mm0				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw   UL|VL      |ZY   UZR|VZR     |

		mov			esi,GBitPtr

		pand		mm4,[WrapMask]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movq		[UVR],mm5

		movq		[UV16],mm4			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |
		psrlq		mm4,[QShiftV]

		mov			ebx,dword ptr[UV16+4]
		movq		[UV16V],mm4

		mov			edx,dword ptr[UV16V]

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |ARGB

		shr			ebx,16
		add			esi,ebx

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		paddd		mm0,mm1

		psllw		mm7,1
		add			esi,edx

		pmulhw		mm7,mm6
		paddw		mm6,mm3

		xor			eax,eax
		movq		mm4,mm0

		mov			al,byte ptr[esi]

		pand		mm4,[WrapMask]

		movd		mm5,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB

		movq		[UV16],mm4			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |
		punpcklbw	mm5,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB

		psrlq		mm4,[QShiftV]
		mov			ebx,dword ptr[UV16+4]

		movq		[UV16V],mm4
		psllw		mm5,1

		mov			edx,dword ptr[UV16V]
		paddd		mm0,mm1

		pmulhw		mm5,mm6
		paddw		mm6,mm3

		shr			ebx,16
		mov			esi,GBitPtr

		add			esi,edx
		movq		mm4,mm0

		packuswb	mm7,mm5				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw ARGB|ARGB UZR|VZR   AR|GB
		add			esi,ebx

		movq		[edi],mm7
		xor			eax,eax

		pand		mm4,[WrapMask]

		mov			al,byte ptr[esi]
		movq		[UV16],mm4

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB
		mov			esi,GBitPtr

		mov			ebx,dword ptr[UV16+4]
		psrlq		mm4,[QShiftV]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		paddd		mm0,mm1

		movq		[UV16V],mm4
		psllw		mm7,1

		pmulhw		mm7,mm6
		xor			eax,eax

		mov			edx,dword ptr[UV16V]
		paddw		mm6,mm3

		shr			ebx,16
		movq		mm4,mm0

		add			esi,edx

		pand		mm4,[WrapMask]
		add			esi,ebx

		mov			al,byte ptr[esi]
		movq		[UV16],mm4			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movd		mm5,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		mov			esi,GBitPtr

		punpcklbw	mm5,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		paddd		mm0,mm1

		psrlq		mm4,[QShiftV]
		psllw		mm5,1

		movq		[UV16V],mm4
		pmulhw		mm5,mm6

		mov			ebx,dword ptr[UV16+4]
		paddw		mm6,mm3

		mov			edx,dword ptr[UV16V]
		movq		mm4,mm0

		shr			ebx,16
		pand		mm4,[WrapMask]

		add			esi,edx
		packuswb	mm7,mm5

		movq		[UV16],mm4
		add			esi,ebx

		movq		[edi+8],mm7
		xor			eax,eax

		psrlq		mm4,[QShiftV]

		mov			al,byte ptr[esi]
		mov			esi,GBitPtr

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB

		mov			ebx,dword ptr[UV16+4]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		paddd		mm0,mm1

		movq		[UV16V],mm4
		psllw		mm7,1

		pmulhw		mm7,mm6
		xor			eax,eax

		mov			edx,dword ptr[UV16V]
		paddw		mm6,mm3

		shr			ebx,16
		movq		mm4,mm0

		add			esi,edx


		pand		mm4,[WrapMask]
		add			esi,ebx

		mov			al,byte ptr[esi]
		movq		[UV16],mm4			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movd		mm5,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		mov			esi,GBitPtr

		punpcklbw	mm5,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		paddd		mm0,mm1

		psrlq		mm4,[QShiftV]
		psllw		mm5,1

		movq		[UV16V],mm4
		pmulhw		mm5,mm6

		mov			ebx,dword ptr[UV16+4]
		paddw		mm6,mm3

		mov			edx,dword ptr[UV16V]
		movq		mm4,mm0

		shr			ebx,16
		pand		mm4,[WrapMask]

		add			esi,edx
		packuswb	mm7,mm5

		movq		[UV16],mm4
		add			esi,ebx

		movq		[edi+16],mm7
		xor			eax,eax

		;regrab z
		movq		mm2,[QZVal32_0]
		movq		mm7,[QZVal32_1]

		paddd		mm2,[QZDelta]
		paddd		mm7,[QZDelta]

		movq		[QZVal32_0],mm2
		movq		[QZVal32_1],mm7

		psrld		mm2,17
		psrld		mm7,17

		packssdw	mm2,mm7
		pslld		mm2,1

		movq		[ebp+8],mm2


		psrlq		mm4,[QShiftV]

		mov			al,byte ptr[esi]
		mov			esi,GBitPtr

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB

		mov			ebx,dword ptr[UV16+4]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		paddd		mm0,mm1

		movq		[UV16V],mm4
		psllw		mm7,1

		pmulhw		mm7,mm6
		xor			eax,eax

		mov			edx,dword ptr[UV16V]
		paddw		mm6,mm3

		shr			ebx,16
		movq		mm4,mm0

		add			esi,edx

		pand		mm4,[WrapMask]
		add			esi,ebx

		mov			al,byte ptr[esi]
		movq		[UV16],mm4			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movd		mm5,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		mov			esi,GBitPtr

		punpcklbw	mm5,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		paddd		mm0,mm1

		psrlq		mm4,[QShiftV]
		psllw		mm5,1

		movq		[UV16V],mm4
		pmulhw		mm5,mm6

		mov			ebx,dword ptr[UV16+4]
		paddw		mm6,mm3

		mov			edx,dword ptr[UV16V]
		movq		mm4,mm0

		shr			ebx,16
		pand		mm4,[WrapMask]

		add			esi,edx
		packuswb	mm7,mm5

		movq		[UV16],mm4
		add			esi,ebx

		movq		[edi+24],mm7
		xor			eax,eax

		psrlq		mm4,[QShiftV]

		mov			al,byte ptr[esi]
		mov			esi,GBitPtr

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB

		mov			ebx,dword ptr[UV16+4]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		paddd		mm0,mm1

		movq		[UV16V],mm4
		psllw		mm7,1

		pmulhw		mm7,mm6
		xor			eax,eax

		mov			edx,dword ptr[UV16V]
		paddw		mm6,mm3

		shr			ebx,16
		movq		mm4,mm0

		add			esi,edx

		pand		mm4,[WrapMask]
		add			esi,ebx

		mov			al,byte ptr[esi]
		movq		[UV16],mm4			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movd		mm5,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		mov			esi,GBitPtr

		punpcklbw	mm5,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		paddd		mm0,mm1

		psrlq		mm4,[QShiftV]
		psllw		mm5,1

		movq		[UV16V],mm4
		pmulhw		mm5,mm6

		mov			ebx,dword ptr[UV16+4]
		paddw		mm6,mm3

		mov			edx,dword ptr[UV16V]
		movq		mm4,mm0

		shr			ebx,16
		pand		mm4,[WrapMask]

		add			esi,edx
		packuswb	mm7,mm5

		movq		[UV16],mm4
		add			esi,ebx

		movq		[edi+32],mm7
		xor			eax,eax

		;regrab z
		movq		mm2,[QZVal32_0]
		movq		mm7,[QZVal32_1]

		paddd		mm2,[QZDelta]
		paddd		mm7,[QZDelta]

		movq		[QZVal32_0],mm2
		movq		[QZVal32_1],mm7

		psrld		mm2,17
		psrld		mm7,17

		packssdw	mm2,mm7
		pslld		mm2,1

		movq		[ebp+16],mm2

		psrlq		mm4,[QShiftV]

		mov			al,byte ptr[esi]
		mov			esi,GBitPtr

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB

		mov			ebx,dword ptr[UV16+4]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		paddd		mm0,mm1

		movq		[UV16V],mm4
		psllw		mm7,1

		pmulhw		mm7,mm6
		xor			eax,eax

		mov			edx,dword ptr[UV16V]
		paddw		mm6,mm3

		shr			ebx,16
		movq		mm4,mm0

		add			esi,edx

		pand		mm4,[WrapMask]
		add			esi,ebx

		mov			al,byte ptr[esi]
		movq		[UV16],mm4			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movd		mm5,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		mov			esi,GBitPtr

		punpcklbw	mm5,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		paddd		mm0,mm1

		psrlq		mm4,[QShiftV]
		psllw		mm5,1

		movq		[UV16V],mm4
		pmulhw		mm5,mm6

		mov			ebx,dword ptr[UV16+4]
		paddw		mm6,mm3

		mov			edx,dword ptr[UV16V]
		movq		mm4,mm0

		shr			ebx,16
		pand		mm4,[WrapMask]

		add			esi,edx
		packuswb	mm7,mm5

		movq		[UV16],mm4
		add			esi,ebx

		movq		[edi+40],mm7
		xor			eax,eax

		psrlq		mm4,[QShiftV]

		mov			al,byte ptr[esi]
		mov			esi,GBitPtr

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB

		mov			ebx,dword ptr[UV16+4]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		paddd		mm0,mm1

		movq		[UV16V],mm4
		psllw		mm7,1

		pmulhw		mm7,mm6
		xor			eax,eax

		mov			edx,dword ptr[UV16V]
		paddw		mm6,mm3

		shr			ebx,16
		movq		mm4,mm0

		add			esi,edx

		pand		mm4,[WrapMask]
		add			esi,ebx

		mov			al,byte ptr[esi]
		movq		[UV16],mm4			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movd		mm5,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		mov			esi,GBitPtr

		punpcklbw	mm5,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		paddd		mm0,mm1

		psrlq		mm4,[QShiftV]
		psllw		mm5,1

		movq		[UV16V],mm4
		pmulhw		mm5,mm6

		mov			ebx,dword ptr[UV16+4]
		paddw		mm6,mm3

		mov			edx,dword ptr[UV16V]
		movq		mm4,mm0

		shr			ebx,16
		pand		mm4,[WrapMask]

		add			esi,edx
		packuswb	mm7,mm5

		movq		[UV16],mm4
		add			esi,ebx

		movq		[edi+48],mm7
		xor			eax,eax

		;regrab z
		movq		mm2,[QZVal32_0]
		movq		mm7,[QZVal32_1]

		paddd		mm2,[QZDelta]
		paddd		mm7,[QZDelta]

		movq		[QZVal32_0],mm2
		movq		[QZVal32_1],mm7

		psrld		mm2,17
		psrld		mm7,17

		packssdw	mm2,mm7
		pslld		mm2,1

		movq		[ebp+24],mm2

		psrlq		mm4,[QShiftV]

		mov			al,byte ptr[esi]
		mov			esi,GBitPtr

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB

		mov			ebx,dword ptr[UV16+4]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB

		movq		[UV16V],mm4
		psllw		mm7,1

		pmulhw		mm7,mm6
		xor			eax,eax

		mov			edx,dword ptr[UV16V]
		paddw		mm6,mm3

		shr			ebx,16

		add			esi,edx

		add			esi,ebx

		mov			al,byte ptr[esi]

		movd		mm5,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB

		punpcklbw	mm5,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB

		psllw		mm5,1

		pmulhw		mm5,mm6
		pop			ebp
		paddw		mm6,mm3

		packuswb	mm7,mm5
		movq		[ARL],mm6

		movq		[edi+56],mm7
		;regrab z
		movq		mm2,[QZVal32_0]
		movq		mm7,[QZVal32_1]

		paddd		mm2,[QZDelta]
		paddd		mm7,[QZDelta]

		movq		[QZVal32_0],mm2
		movq		[QZVal32_1],mm7

		; get corrected right side deltas ; ULw|VLw DU16|DV16 argb|ARGBagAG|rbRB    |ZRi   aA|rR   UZR|VZR aArR|gGbB

		movq		mm4,[ZIR]
		movq		mm6,[UVZ]

		movq		mm2,[UVR]
		movq		mm0,[UVLeftW]		; ULw|VLw DU16|DV16 argb|ARGBagAG|rbRB    |ZRi   aA|rR   UZR|VZR aArR|gGbB

		movq		mm1,[UVLeft]		; ULw|VLw   UL|VL   argb|ARGBagAG|rbRB ZRi|ZRi   aA|rR   UZR|VZR aArR|gGbB
		punpckldq	mm2,mm2				; ULw|VLw   UL|VL    ZRi|ZRi agAG|rbRB ZRi|ZRi   aA|rR   UZR|VZR aArR|gGbB

		pfmul		mm2,mm6				; ULw|VLw   UL|VL     UR|VR  agAG|rbRB ZRi|ZRi   aA|rR   UZR|VZR aArR|gGbB

		add		edi,64					; move screen pointer to start of next aspan
		add		[Dest],32

		movq		mm3,mm2
		pfmul		mm3,[QFixedScale]

		pf2id		mm3,mm3

		dec		[NumASpans]			; dec num affine spans
		jnz		SpanLoopLit

HandleLeftoverPixelsLit:


		mov		esi,GBitPtr


		cmp			[RemainingCount],0
		jz			ReturnLit

		mov			eax,[RemainingCount]
		mov			dword ptr[ZIR],eax
		mov			dword ptr[ZIR+4],eax
		
		movq		mm7,[GLMapMulUV]	;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR   LU|LV
		movq		mm5,mm1

		pfsub		mm6,[UVDivZ16StepX]
		pfmul		mm7,mm1				;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR  ULL|VLL

		pfmul		mm5,[QFixedScale]
		pi2fd		mm3,[ZIR]

		pfmul		mm7,[QFixedScale]
		pfmul		mm3,[UVDivZStepX]

		pf2id		mm5,mm5
		pfadd		mm3,mm6

		pi2fd		mm2,[ZIR]
		movd		mm6,[Zi16StepX]

		pf2id		mm7,mm7				; ULw|VLw   DU|DV    UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |
		pfsub		mm4,mm6

		movd		mm6,[ZiStepX]
		mov			ebx,[RemainingCount]

		pfmul		mm2,mm6

		shl			ebx,3

		pfadd		mm2,mm4
		paddd		mm5,[UVAdjust]

		punpckldq	mm4,mm4

		movq		[UVL16],mm7
		pfrcp		mm2,mm2

		pfmul		mm4,[QZBufferPrec]

		paddd		mm5,[UVAdjust2]
		pfmul		mm2,mm3

		movq		[UVLeft],mm5
		pfsubr		mm1,mm2				; ULw|VLw   DU|DV    UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |

		pfmul		mm1,qword ptr[QFixedScaleLUT+ebx]	; ULw|VLw DU16|DV16  UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |

		movq		mm0,mm5
		pand		mm5,[WrapMask]
		pf2id		mm1,mm1				; ULw|VLw DU16|DV16  UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |

		movq		[UV16],mm5
		movq		mm7,[UVAdjustL]

		psrlq		mm5,[QShiftV]
		paddd		mm7,[UVL16]

		movq		[UV16V],mm5
		movq		[UVL16],mm7


OnePixelSpanLit:
		; Clamp U/V
		mov		ebx,dword ptr[UVL16+4]
		cmp		ebx,MaxU
		jle		TryClampU1Lit
		mov		ecx,MaxU
		mov		dword ptr[UVL16+4],ecx
		jmp		NoClampU1Lit

TryClampU1Lit:
		cmp		ebx,0
		jge		NoClampU1Lit
		mov		dword ptr[UVL16+4],0
NoClampU1Lit:
		mov		eax,dword ptr[UVL16]
		cmp		eax,MaxV
		jle		TryClampV1Lit
		mov		ecx,MaxV
		mov		dword ptr[UVL16],ecx
		jmp		NoClampV1Lit

TryClampV1Lit:
		cmp		eax,0
		jge		NoClampV1Lit
		mov		dword ptr[UVL16],0

NoClampV1Lit:
		mov			ebx,dword ptr[UV16+4]
		mov			eax,dword ptr[UV16V]

		mov			esi,GBitPtr
		shr			ebx,16
		add			esi,eax
		add			esi,ebx
		mov			ecx,TexPal
		xor			eax,eax
		movq		mm6,[ARL]
		push		ebp
		mov			ebp,dword ptr[Dest]
		movd		mm3,dword ptr[ZiStepX]

LeftoverLoopLit:
		paddd		mm0,mm1				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |
		pf2id		mm2,mm4

		mov			al,byte ptr[esi]
		movq		mm5,mm0				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw   UL|VL      |ZY   UZR|VZR     |

		pfadd		mm4,mm3
		mov			esi,GBitPtr

		pand		mm5,[WrapMask]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |
		psrlq		mm2,16

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |ARGB
		movd		eax,mm2

		movq		[UV16],mm5			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |
		xor			eax,0

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		mov			word ptr[ebp],ax

		mov			ebx,dword ptr[UV16+4]

		psrlq		mm5,[QShiftV]

		shr			ebx,16
		psllw		mm7,1

		movq		[UV16V],mm5
		add			esi,ebx

		mov			edx,dword ptr[UV16V]
		pmulhw		mm7,mm6

		add			esi,edx
		packuswb	mm7,mm7

		xor			eax,eax
		movd		[edi],mm7

		add			edi,4
		add			ebp,2

		dec			[RemainingCount]
		jge			LeftoverLoopLit

		pop			ebp

ReturnLit:

	pop		edi
	pop		esi
	pop		ecx
	pop		ebx

	cRet DrawSpan32_AsmGouraudZWrite3DNow
endProc DrawSpan32_AsmGouraudZWrite3DNow



;same with zbuffer

cProc DrawSpan32_AsmGouraudZBuffer3DNow, 36,<x1 : dword, x2 : dword, y : dword, r1 : dword, g1 : dword, b1 : dword, r2 : dword, g2 : dword, b2 : dword>

		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		eax,x1
		mov		ecx,x2
		sub		ecx,eax
		jle		ReturnLit

;		femms

;		inc		ecx
		mov		ebx,offset ClientWindow
		mov		eax, y
		mov		edi,[ebx].Buffer
		mov		esi,x1
		imul	eax, [ebx].PixelPitch
		shl		esi,2
		mov		edx,ZBuffer
		add		eax,esi
		add		edi, eax
		shr		eax,1
		add		edx,eax
		mov		[Dest],edx
		mov		eax,offset QZCan
		mov		ebx,offset SCan
		mov		[eax],ebx
		mov		[eax+4],edx
		mov		eax,offset QDibCan
		mov		[eax],ebx
		mov		[eax+4],edi
		mov		ebx,offset QDibCan
		mov		eax,offset QDibOrCan
		mov		[eax],ebx
		mov		[eax+4],ebx
		mov		ebx,offset QZCan
		mov		eax,offset QZOrCan
		mov		[eax],ebx
		mov		[eax+4],ebx
		mov		edx,ecx
		mov		eax,ecx

		shr		ecx,4
		and		eax,15
		jnz		@f
		dec		ecx
		mov		eax,16
@@:

		mov		[NumASpans],ecx
		mov		[RemainingCount],eax

		;grab the left side lights
		movd		mm5,r1
		movd		mm4,b1

;		punpckldq	mm5,qword ptr[Zero]
		movd		mm6,g1

		pf2id		mm5,mm5
		punpckldq	mm4,mm6

		pf2id		mm4,mm4
		packssdw	mm4,mm5

;		psllw		mm4,7

		movq		[ARL],mm4

										;   mm0      mm1      mm2      mm3      mm4      mm5      mm6      mm7
		movd		mm0,x1				;    |x       |        |        |        |        |        |        |
		movq		mm2,[UVDivZStepX]	;    |x       |    UZdX|VZdX    |        |        |        |        |

		movd		mm1,y				;    |x       |y   UZdX|VZdX    |        |        |        |        |
		movq		mm3,[UVDivZStepY]	;    |x       |y   UZdX|VZdXUZdY|VZdY    |        |        |        |

		punpckldq	mm0,mm0				;   x|x       |y   UZdX|VZdXUZdY|VZdY    |        |        |        |
		punpckldq	mm1,mm1				;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |        |        |        |

		movd		mm7,edx				;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |        |        |        |wid
		movd		mm5,b2				;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |        |b       |        |wid

		pi2fd		mm0,mm0				;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |        |b       |        |wid
		movd		mm6,b1				;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |        |b       |b       |wid

		pi2fd		mm7,mm7				;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |        |b       |b       |wid
		punpckldq	mm5,qword ptr[g2]	;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |       g|b       |b       |wid

		pi2fd		mm1,mm1				;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |       g|b       |b       |wid
		punpckldq	mm6,qword ptr[g1]	;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |       g|b      g|b       |wid

		pfmul		mm2,mm0				;   x|x      y|y    UZX|VZX UZdY|VZdY    |       g|b      g|b       |wid
		pfrcp		mm7,mm7				;   x|x      y|y    UZX|VZX UZdY|VZdY    |       g|b      g|b     dw|dw

		pfsub		mm5,mm6				;   x|x      y|y    UZX|VZX UZdY|VZdY    |      gd|bd     g|b     dw|dw
		movd		mm4,[r1]			;   x|x      y|y    UZX|VZX UZdY|VZdY    |r     gd|bd     g|b     dw|dw

		movd		mm6,[r2]			;   x|x      y|y    UZX|VZX UZdY|VZdY    |r     gd|bd     g|r     dw|dw
		pfmul		mm3,mm1				;   x|x      y|y    UZX|VZX  UZY|VZY     |r     gd|bd     g|r     dw|dw

		pfsub		mm6,mm4				;   x|x      y|y    UZX|VZX  UZY|VZY     |r     gd|bd     x|rd    dw|dw
		pfmul		mm7,[Q128]			;   x|x      y|y    UZX|VZX  UZY|VZY     |r     gd|bd     x|rd    DW|DW

		movd		mm4,[ZiStepX]		;   x|x      y|y    UZX|VZX  UZY|VZY     |ZdX   gd|bd     x|rd    DW|DW

		punpckldq	mm4,mm4
		pfmul		mm4,[QZBufferPrec]
		pf2id		mm4,mm4
		movq		[QZDelta],mm4

		movd		mm4,[ZiStepX]		;   x|x      y|y    UZX|VZX UZdY|VZdY    |ZdX     |        |        |
		pfmul		mm5,mm7				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZdX   GD|BD     x|rd    DW|DW

		pfmul		mm4,mm0				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX    GD|BD     x|rd    DW|DW
		pfmul		mm6,mm7				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX    GD|BD     x|RD    DW|DW

		pf2id		mm5,mm5
		pf2id		mm6,mm6

		packssdw	mm5,mm6
		movq		[RGBADelta],mm5

		movd		mm5,[ZiStepY]		;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX      |ZdY    x|RD    DW|DW
		movq		mm6,[UVDivZOrigin]	;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX      |ZdY  UZO|VZO   DW|DW

		pfmul		mm5,mm1				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX      |ZY   UZO|VZO   DW|DW
		movd		mm7,[ZiOrigin]		;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX      |ZY   UZO|VZO     |ZO

		pfadd		mm6,mm2				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX      |ZY  UZXS|VZXS    |ZO
		pfadd		mm4,mm7				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZXS     |ZY  UZXS|VZXS    |ZO

		pfadd		mm6,mm3				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZXS     |ZY    UZ|VZ      |ZO
		pfadd		mm4,mm5				;   x|x      y|y    UZX|VZX  UZY|VZY     |Zi      |ZY    UZ|VZ      |ZO

		movq		mm7,mm4
		movq		mm1,[QZDelta]
		punpckldq	mm7,mm7
		pfmul		mm7,[QZBufferPrec]
		pf2id		mm7,mm7
		movq		mm0,mm7
		paddd		mm7,mm1
		punpckldq	mm0,mm7
		paddd		mm7,mm1
		movq		[QZVal32_0],mm0
		movq		mm0,mm7
		paddd		mm7,mm1
		punpckldq	mm0,mm7
		movq		[QZVal32_1],mm0
		pslld		mm1,2
		movd		mm7,[Zi16StepX]
		movq		[QZDelta],mm1

		pfrcp		mm0,mm4				;  ZL|ZL     y|y    UZX|VZX  UZY|VZY     |Zi      |ZY    UZ|VZ      |ZdX16

		movq		mm1,mm6				;  ZL|ZL    UZ|VZ   UZX|VZX  UZY|VZY     |Zi      |ZY    UZ|VZ      |ZdX16
		pfadd		mm4,mm7				;  ZL|ZL    UZ|VZ   UZX|VZX  UZY|VZY     |ZRi     |ZY    UZ|VZ      |ZdX16

		pfmul		mm1,mm0				;  ZL|ZL    UL|VL   UZX|VZX  UZY|VZY     |ZRi     |ZY    UZ|VZ      |ZdX16
		pfrcp		mm2,mm4				;  ZL|ZL    UL|VL    ZR|ZR   UZY|VZY     |ZRi     |ZY    UZ|VZ      |ZdX16

		pfadd		mm6,[UVDivZ16StepX] ;  ZL|ZL    UL|VL    ZR|ZR   UZY|VZY     |ZRi     |ZY   UZR|VZR     |ZdX16
		pfmul		mm2,mm6				;  ZL|ZL    UL|VL    UR|VR   UZY|VZY     |ZRi     |ZY   UZR|VZR     |ZdX16

		movq		mm0,mm1				;  UL|VL    UL|VL    UR|VR   UZY|VZY     |ZRi     |ZY   UZR|VZR     |ZdX16
		movq		mm3,mm2				; ULi|VLi	UL|VL    UR|UR    UR|UR      |ZRi     |ZY   UZR|VZR     |ZdX16

		pfmul		mm0,[QFixedScale]	; UL6|VL6	UL|VL    UR|UR    UR|UR      |ZRi     |ZY   UZR|VZR     |ZdX16
		pfmul		mm3,[QFixedScale]	; UL6|VL6	UL|VL   UR6|UR6  UR6|UR6     |ZRi     |ZY   UZR|VZR     |ZdX16

		pf2id		mm0,mm0				;UL6i|VL6i  UL|VL   UR6|VR6  UR6|VR6     |ZRi     |ZY   UZR|VZR     |ZdX16
		pf2id		mm3,mm3				;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR     |ZdX16


		test	ecx,ecx
		jz		HandleLeftoverPixelsLit

		movq		mm7,[GLMapMulUV]	;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR   LU|LV
		pfmul		mm7,mm1				;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR  ULL|VLL
		pfmul		mm7,[QFixedScale]
		pf2id		mm7,mm7				; ULw|VLw   DU|DV    UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |
		paddd		mm7,[UVAdjustL]
		movq		[UVL16],mm7

		; Clamp U/V
		mov		ebx,dword ptr[UVL16+4]

		cmp		ebx,MaxU
		jle		TryClampU0Litp
		mov		ecx,MaxU
		mov		dword ptr[UVL16+4],ecx
		jmp		NoClampU0Litp

TryClampU0Litp:
		cmp		ebx,0
		jge		NoClampU0Litp
		mov		dword ptr[UVL16+4],0
NoClampU0Litp:
		mov		eax,dword ptr[UVL16]
		cmp		eax,MaxV
		jle		TryClampV0Litp
		mov		ecx,MaxV
		mov		dword ptr[UVL16],ecx
		jmp		NoClampV0Litp

TryClampV0Litp:
		cmp		eax,0
		jge		NoClampV0Litp
		mov		dword ptr[UVL16],0

NoClampV0Litp:

		pfrcp		mm5,mm4

SpanLoopLit:
		;use float uv for lightmap uv
		movq		mm7,[GLMapMulUV]	;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR   LU|LV
		movq		mm5,mm1

		pfmul		mm7,mm2				;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR  ULL|VLL
		pfmul		mm5,[QFixedScale]

		pfmul		mm7,[QFixedScale]
		pf2id		mm5,mm5

		pf2id		mm7,mm7				; ULw|VLw   DU|DV    UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |
		paddd		mm5,[UVAdjust]

		pfsubr		mm1,mm2				; ULw|VLw   DU|DV    UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |
		paddd		mm5,[UVAdjust2]

		movq		[UVL16],mm7
		movq		[UVLeft],mm5

		movd		mm7,[Zi16StepX]
		movq		mm0,mm5

		pfmul		mm1,[QFixedScale16]	; ULw|VLw DU16|DV16  UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |
		;right side stuff becomes left	; ULw|VLw DU16|DV16  UL|VL   URw|VRw     |ZLi     |ZY   UZL|VZL     |
		pfadd		mm4,mm7				;ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZL|VZL     |

		pand		mm5,[WrapMask]
		pf2id		mm1,mm1				; ULw|VLw DU16|DV16  UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |

		movq		mm7,[UVAdjustL]
		movq		[UV16],mm5

		pfadd		mm6,[UVDivZ16StepX]	; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |
		paddd		mm7,[UVL16]

		psrlq		mm5,[QShiftV]
		movq		[UVL16],mm7

		movq		[UV16V],mm5


		; Clamp U/V
		mov		ebx,dword ptr[UVL16+4]
		cmp		ebx,MaxU
		jle		TryClampU0Lit
		mov		ecx,MaxU
		mov		dword ptr[UVL16+4],ecx
		jmp		NoClampU0Lit

TryClampU0Lit:
		cmp		ebx,0
		jge		NoClampU0Lit
		mov		dword ptr[UVL16+4],0
NoClampU0Lit:
		mov		eax,dword ptr[UVL16]
		cmp		eax,MaxV
		jle		TryClampV0Lit
		mov		ecx,MaxV
		mov		dword ptr[UVL16],ecx
		jmp		NoClampV0Lit

TryClampV0Lit:
		cmp		eax,0
		jge		NoClampV0Lit
		mov		dword ptr[UVL16],0

NoClampV0Lit:
		movq		[UVLeftW],mm3
		mov			eax,dword ptr[UV16V]
		mov			ebx,dword ptr[UV16+4]
		movq		[ZIR],mm4
		mov			esi,GBitPtr
		shr			ebx,16
		movq		[UVZ],mm6
		add			esi,eax
		add			esi,ebx
		movq		[UVLeft],mm2

		mov			ecx,TexPal
		pfrcp		mm5,mm4
		movq		mm3,[RGBADelta]
		xor			eax,eax
		movq		mm6,[ARL]

		push		ebp
		mov			ebp,[Dest]

		;grab zbuffer values
		movq		mm2,[QZVal32_0]
		movq		mm7,[QZVal32_1]

		psrld		mm2,17
		psrld		mm7,17

		packssdw	mm2,mm7

		pslld		mm2,1
		movq		[QZVal],mm2


		movq		mm2,[QZVal]
		paddd		mm0,mm1				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |

		mov			al,byte ptr[esi]
		movq		mm4,mm0				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw   UL|VL      |ZY   UZR|VZR     |

		pcmpgtw		mm2,[ebp]
		mov			esi,GBitPtr

		psrlw		mm2,15
		pand		mm4,[WrapMask]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		psllw		mm2,2
		movq		[UV16],mm4			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movq		[GBL],mm2
		psrlq		mm4,[QShiftV]

		movq		mm7,mm2
		movq		[UV16V],mm4

		punpcklwd	mm7,[Zero]
		movq		[UVR],mm5

		paddd		mm7,[QZOrCan]
		mov			edx,dword ptr[UV16V]

		movq		[QZOut],mm7
		mov			ebx,dword ptr[UV16+4]

		movq		mm7,mm2
		mov			edi,dword ptr[QZOut]

		punpcklwd	mm7,[Zero]
		shr			ebx,16

		paddd		mm7,[QDibOrCan]
		mov			edi,dword ptr[edi]

		movq		[QDibOut],mm7
		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |ARGB

		add			esi,edx
		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB

		xor			eax,eax

		pmullw		mm7,mm6
		mov			ax,word ptr[QZVal]

		paddd		mm0,mm1
		mov			word ptr[edi],ax

		mov			edi,dword ptr[QDibOut]
		add			esi,ebx

		mov			edi,dword ptr[edi]
		paddw		mm6,mm3

		psrlw		mm7,8
		xor			eax,eax

		packuswb	mm7,mm7
		mov			al,byte ptr[esi]

		movd		[edi],mm7
		movq		mm4,mm0

		pand		mm4,[WrapMask]
		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB

		movq		[UV16],mm4
		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB

		psrlq		mm4,[QShiftV]
		mov			ebx,dword ptr[UV16+4]

		movq		[UV16V],mm4
		pmullw		mm7,mm6

		mov			edx,dword ptr[UV16V]
		paddw		mm6,mm3

		shr			ebx,16
		psrlw		mm7,8

		mov			esi,GBitPtr
		mov			edi,dword ptr[QZOut+4]

		add			esi,edx
		xor			eax,eax

		mov			edi,dword ptr[edi]
		paddd		mm0,mm1

		packuswb	mm7,mm7				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw ARGB|ARGB UZR|VZR   AR|GB
		mov			ax,word ptr[QZVal+2]

		add			esi,ebx
		mov			word ptr[edi+2],ax

		xor			eax,eax
		mov			edi,dword ptr[QDibOut+4]

		mov			al,byte ptr[esi]
		movq		mm4,mm0

		mov			edi,dword ptr[edi]
		pand		mm4,[WrapMask]

		mov			esi,GBitPtr
		movd		[edi+4],mm7

		movq		mm7,mm2
		movq		[UV16],mm4

		punpckhwd	mm7,[Zero]
		psrlq		mm4,[QShiftV]

		paddd		mm7,[QZorCan]
		movq		[UV16V],mm4

		movq		[QZOut],mm7
		mov			ebx,dword ptr[UV16+4]

		movq		mm7,mm2
		mov			edi,dword ptr[QZOut]

		punpckhwd	mm7,[Zero]
		mov			edi,dword ptr[edi]

		paddd		mm7,[QDibOrCan]
		paddd		mm0,mm1

		movq		[QDibOut],mm7
		mov			edx,dword ptr[UV16V]

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB
		xor			eax,0

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		mov			ax,word ptr[QZVal+4]

		pmullw		mm7,mm6
		mov			word ptr[edi+4],ax

		psrlw		mm7,8
		mov			edi,dword ptr[QDibOut]

		paddw		mm6,mm3

		packuswb	mm7,mm7
		mov			edi,dword ptr[edi]

		movq		mm4,mm0
		movd		[edi+8],mm7

		shr			ebx,16
		xor			eax,eax

		pand		mm4,[WrapMask]
		add			esi,edx

		paddd		mm0,mm1
		add			esi,ebx

		movq		[UV16],mm4
		mov			al,byte ptr[esi]

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		psrlq		mm4,[QShiftV]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		movq		[UV16V],mm4

		mov			ebx,dword ptr[UV16+4]
		mov			edx,dword ptr[UV16V]

		pmullw		mm7,mm6
		shr			ebx,16

		paddw		mm6,mm3
		psrlw		mm7,8

		mov			esi,GBitPtr
		mov			edi,dword ptr[QZOut+4]

		add			esi,edx
		xor			eax,eax

		mov			edi,dword ptr[edi]

		packuswb	mm7,mm7				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw ARGB|ARGB UZR|VZR   AR|GB
		mov			ax,word ptr[QZVal+6]

		add			esi,ebx
		mov			word ptr[edi+6],ax

		mov			edi,dword ptr[QDibOut+4]
		xor			eax,eax

		mov			edi,dword ptr[edi]
		mov			al,byte ptr[esi]

		movd		[edi+12],mm7
		movq		mm4,mm0

		;regrab z
		movq		mm2,[QZVal32_0]
		movq		mm7,[QZVal32_1]

		paddd		mm2,[QZDelta]
		paddd		mm7,[QZDelta]

		movq		[QZVal32_0],mm2
		movq		[QZVal32_1],mm7

		psrld		mm2,17
		psrld		mm7,17

		packssdw	mm2,mm7
		pand		mm4,[WrapMask]

		pslld		mm2,1
		movq		[UV16],mm4

		movq		[QZVal],mm2
		mov			esi,GBitPtr

		pcmpgtw		mm2,[ebp+8]
		psrlq		mm4,[QShiftV]


		psrlw		mm2,15
		movq		[UV16V],mm4


		psllw		mm2,2
		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB

		movq		mm4,mm2
		mov			ebx,dword ptr[UV16+4]

		punpcklwd	mm4,[Zero]
		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB

		paddd		mm4,[QZorCan]
		pmullw		mm7,mm6

		movq		[QZOut],mm4
		mov			edi,dword ptr[QZOut]

		movq		mm4,mm2
		xor			eax,eax

		paddw		mm6,mm3

		punpcklwd	mm4,[Zero]
		mov			edi,dword ptr[edi]

		psrlw		mm7,8

		mov			ax,word ptr[QZVal]
		paddd		mm4,[QDibOrCan]

		mov			word ptr[edi+8],ax
		movq		[QDibOut],mm4

		packuswb	mm7,mm7
		mov			edi,dword ptr[QDibOut]

		paddd		mm0,mm1
		mov			edx,dword ptr[UV16V]

		mov			edi,dword ptr[edi]

		movd		[edi+16],mm7
		movq		mm4,mm0

		shr			ebx,16
		xor			eax,eax

		pand		mm4,[WrapMask]
		add			esi,edx

		paddd		mm0,mm1
		add			esi,ebx

		mov			al,byte ptr[esi]
		movq		[UV16],mm4

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		psrlq		mm4,[QShiftV]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		movq		[UV16V],mm4

		mov			ebx,dword ptr[UV16+4]
		mov			edx,dword ptr[UV16V]

		pmullw		mm7,mm6

		shr			ebx,16
		psrlw		mm7,8

		mov			esi,GBitPtr
		mov			edi,dword ptr[QZOut+4]

		paddw		mm6,mm3

		add			esi,edx
		xor			eax,eax

		mov			edi,dword ptr[edi]
		movq		mm4,mm0

		packuswb	mm7,mm7				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw ARGB|ARGB UZR|VZR   AR|GB
		pand		mm4,[WrapMask]

		mov			ax,word ptr[QZVal+2]
		movq		[UV16],mm4

		add			esi,ebx
		psrlq		mm4,[QShiftV]

		mov			word ptr[edi+10],ax
		movq		[UV16V],mm4

		xor			eax,eax
		mov			edi,dword ptr[QDibOut+4]

		mov			al,byte ptr[esi]
		mov			edi,dword ptr[edi]

		mov			esi,GBitPtr
		movd		[edi+20],mm7

		movq		mm4,mm2
		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB

		punpckhwd	mm4,[Zero]

		mov			ebx,dword ptr[UV16+4]
		paddd		mm4,[QZorCan]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		movq		[QZOut],mm4

		movq		mm4,mm2

		pmullw		mm7,mm6
		mov			edi,dword ptr[QZOut]

		xor			eax,eax
		punpckhwd	mm4,[Zero]

		paddw		mm6,mm3

		mov			edi,dword ptr[edi]
		psrlw		mm7,8

		mov			ax,word ptr[QZVal+4]
		paddd		mm4,[QDibOrCan]

		mov			word ptr[edi+12],ax
		movq		[QDibOut],mm4

		packuswb	mm7,mm7
		mov			edi,dword ptr[QDibOut]

		paddd		mm0,mm1
		mov			edi,dword ptr[edi]

		mov			edx,dword ptr[UV16V]

		movd		[edi+24],mm7
		movq		mm4,mm0

		shr			ebx,16
		xor			eax,eax

		pand		mm4,[WrapMask]
		add			esi,edx

		paddd		mm0,mm1
		add			esi,ebx

		mov			al,byte ptr[esi]
		movq		[UV16],mm4

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		psrlq		mm4,[QShiftV]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		movq		[UV16V],mm4

		mov			ebx,dword ptr[UV16+4]
		mov			edx,dword ptr[UV16V]

		pmullw		mm7,mm6

		shr			ebx,16
		psrlw		mm7,8

		paddw		mm6,mm3

		mov			esi,GBitPtr
		mov			edi,dword ptr[QZOut+4]

		add			esi,edx
		xor			eax,eax

		mov			edi,dword ptr[edi]

		packuswb	mm7,mm7				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw ARGB|ARGB UZR|VZR   AR|GB
		mov			ax,word ptr[QZVal+6]

		add			esi,ebx
		mov			word ptr[edi+14],ax

		;regrab z
		movq		mm2,[QZVal32_0]
		movq		mm4,[QZVal32_1]

		paddd		mm2,[QZDelta]
		paddd		mm4,[QZDelta]

		movq		[QZVal32_0],mm2
		movq		[QZVal32_1],mm4

		psrld		mm2,17
		psrld		mm4,17

		packssdw	mm2,mm4
		xor			eax,eax

		pslld		mm2,1

		mov			edi,dword ptr[QDibOut+4]

		movq		[QZVal],mm2
		mov			edi,dword ptr[edi]

		mov			al,byte ptr[esi]
		movq		mm4,mm0

		pand		mm4,[WrapMask]
		pcmpgtw		mm2,[ebp+16]

		movd		[edi+28],mm7
		movq		[UV16],mm4

		psrlw		mm2,15
		mov			esi,GBitPtr

		psrlq		mm4,[QShiftV]

		psllw		mm2,2
		movq		[UV16V],mm4
		movq		mm4,mm2

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB
		punpcklwd	mm4,[Zero]

		mov			ebx,dword ptr[UV16+4]
		paddd		mm4,[QZorCan]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		movq		[QZOut],mm4

		movq		mm4,mm2

		pmullw		mm7,mm6
		mov			edi,dword ptr[QZOut]

		xor			eax,eax
		punpcklwd	mm4,[Zero]

		paddw		mm6,mm3
		mov			edi,dword ptr[edi]

		mov			ax,word ptr[QZVal]
		paddd		mm4,[QDibOrCan]
		psrlw		mm7,8

		mov			word ptr[edi+16],ax
		movq		[QDibOut],mm4

		packuswb	mm7,mm7
		mov			edi,dword ptr[QDibOut]

		paddd		mm0,mm1
		mov			edi,dword ptr[edi]
		mov			edx,dword ptr[UV16V]

		movd		[edi+32],mm7
		movq		mm4,mm0

		shr			ebx,16
		xor			eax,eax

		pand		mm4,[WrapMask]
		add			esi,edx

		paddd		mm0,mm1
		add			esi,ebx

		mov			al,byte ptr[esi]
		movq		[UV16],mm4

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		psrlq		mm4,[QShiftV]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		movq		[UV16V],mm4

		mov			ebx,dword ptr[UV16+4]
		mov			edx,dword ptr[UV16V]

		pmullw		mm7,mm6

		shr			ebx,16
		psrlw		mm7,8

		paddw		mm6,mm3

		mov			esi,GBitPtr
		mov			edi,dword ptr[QZOut+4]

		add			esi,edx
		xor			eax,eax

		movq		mm4,mm0
		mov			edi,dword ptr[edi]

		pand		mm4,[WrapMask]
		packuswb	mm7,mm7				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw ARGB|ARGB UZR|VZR   AR|GB

		movq		[UV16],mm4
		mov			ax,word ptr[QZVal+2]

		psrlq		mm4,[QShiftV]
		add			esi,ebx

		movq		[UV16V],mm4
		mov			word ptr[edi+18],ax

		xor			eax,eax
		mov			edi,dword ptr[QDibOut+4]

		mov			al,byte ptr[esi]
		mov			edi,dword ptr[edi]

		mov			esi,GBitPtr
		movd		[edi+36],mm7

		movq		mm4,mm2

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB

		punpckhwd	mm4,[Zero]

		mov			ebx,dword ptr[UV16+4]
		paddd		mm4,[QZorCan]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		movq		[QZOut],mm4

		movq		mm4,mm2

		pmullw		mm7,mm6
		mov			edi,dword ptr[QZOut]

		xor			eax,eax
		punpckhwd	mm4,[Zero]

		paddw		mm6,mm3

		mov			edi,dword ptr[edi]
		psrlw		mm7,8

		mov			ax,word ptr[QZVal+4]
		paddd		mm4,[QDibOrCan]

		mov			word ptr[edi+20],ax
		movq		[QDibOut],mm4

		packuswb	mm7,mm7
		mov			edi,dword ptr[QDibOut]

		paddd		mm0,mm1
		mov			edi,dword ptr[edi]
		mov			edx,dword ptr[UV16V]

		movd		[edi+40],mm7
		movq		mm4,mm0

		shr			ebx,16
		xor			eax,eax

		pand		mm4,[WrapMask]
		add			esi,edx

		paddd		mm0,mm1
		add			esi,ebx

		mov			al,byte ptr[esi]
		movq		[UV16],mm4

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		psrlq		mm4,[QShiftV]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		movq		[UV16V],mm4

		mov			ebx,dword ptr[UV16+4]
		mov			edx,dword ptr[UV16V]

		pmullw		mm7,mm6

		shr			ebx,16
		psrlw		mm7,8

		paddw		mm6,mm3

		mov			esi,GBitPtr
		mov			edi,dword ptr[QZOut+4]

		add			esi,edx
		xor			eax,eax
		mov			edi,dword ptr[edi]

		packuswb	mm7,mm7				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw ARGB|ARGB UZR|VZR   AR|GB
		mov			ax,word ptr[QZVal+6]

		add			esi,ebx
		mov			word ptr[edi+22],ax

		;regrab z
		movq		mm2,[QZVal32_0]
		movq		mm4,[QZVal32_1]

		paddd		mm2,[QZDelta]
		paddd		mm4,[QZDelta]

		movq		[QZVal32_0],mm2
		movq		[QZVal32_1],mm4

		psrld		mm2,17
		psrld		mm4,17

		packssdw	mm2,mm4
		xor			eax,eax

		pslld		mm2,1

		mov			edi,dword ptr[QDibOut+4]

		movq		[QZVal],mm2
		mov			edi,dword ptr[edi]

		mov			al,byte ptr[esi]
		movq		mm4,mm0

		pand		mm4,[WrapMask]
		pcmpgtw		mm2,[ebp+24]

		movd		[edi+44],mm7
		movq		[UV16],mm4

		psrlw		mm2,15
		mov			esi,GBitPtr

		psrlq		mm4,[QShiftV]

		psllw		mm2,2
		movq		[UV16V],mm4

		movq		mm4,mm2


		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB
		punpcklwd	mm4,[Zero]

		mov			ebx,dword ptr[UV16+4]
		paddd		mm4,[QZorCan]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		movq		[QZOut],mm4

		movq		mm4,mm2

		pmullw		mm7,mm6
		mov			edi,dword ptr[QZOut]

		xor			eax,eax
		punpcklwd	mm4,[Zero]

		paddw		mm6,mm3

		mov			edi,dword ptr[edi]
		psrlw		mm7,8

		mov			ax,word ptr[QZVal]
		paddd		mm4,[QDibOrCan]

		mov			word ptr[edi+24],ax
		movq		[QDibOut],mm4

		packuswb	mm7,mm7
		mov			edi,dword ptr[QDibOut]

		paddd		mm0,mm1
		mov			edi,dword ptr[edi]
		mov			edx,dword ptr[UV16V]

		movd		[edi+48],mm7
		movq		mm4,mm0

		shr			ebx,16
		xor			eax,eax

		pand		mm4,[WrapMask]
		add			esi,edx

		paddd		mm0,mm1
		add			esi,ebx

		mov			al,byte ptr[esi]
		movq		[UV16],mm4

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		psrlq		mm4,[QShiftV]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		movq		[UV16V],mm4

		mov			ebx,dword ptr[UV16+4]
		mov			edx,dword ptr[UV16V]

		pmullw		mm7,mm6

		shr			ebx,16
		psrlw		mm7,8

		paddw		mm6,mm3

		mov			esi,GBitPtr
		mov			edi,dword ptr[QZOut+4]

		add			esi,edx
		xor			eax,eax
		mov			edi,dword ptr[edi]

		packuswb	mm7,mm7				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw ARGB|ARGB UZR|VZR   AR|GB
		mov			ax,word ptr[QZVal+2]

		movq		mm4,mm0
		add			esi,ebx

		pand		mm4,[WrapMask]
		mov			word ptr[edi+26],ax

		movq		[UV16],mm4
		xor			eax,eax

		psrlq		mm4,[QShiftV]
		mov			edi,dword ptr[QDibOut+4]

		movq		[UV16V],mm4
		mov			al,byte ptr[esi]

		mov			edi,dword ptr[edi]

		mov			esi,GBitPtr
		movd		[edi+52],mm7

		movq		mm4,mm2

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB

		punpckhwd	mm4,[Zero]

		mov			ebx,dword ptr[UV16+4]
		paddd		mm4,[QZorCan]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		movq		[QZOut],mm4

		movq		mm4,mm2

		pmullw		mm7,mm6
		mov			edi,dword ptr[QZOut]

		xor			eax,eax
		punpckhwd	mm4,[Zero]
		mov			edi,dword ptr[edi]

		paddw		mm6,mm3

		mov			ax,word ptr[QZVal+4]
		paddd		mm4,[QDibOrCan]
		psrlw		mm7,8

		mov			word ptr[edi+28],ax
		movq		[QDibOut],mm4

		packuswb	mm7,mm7
		mov			edi,dword ptr[QDibOut]

		paddd		mm0,mm1
		mov			edi,dword ptr[edi]
		mov			edx,dword ptr[UV16V]

		movd		[edi+56],mm7
		movq		mm4,mm0

		shr			ebx,16
		xor			eax,eax

		pand		mm4,[WrapMask]
		add			esi,edx

		paddd		mm0,mm1
		add			esi,ebx

		mov			al,byte ptr[esi]

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB

		pmullw		mm7,mm6

		paddw		mm6,mm3
		psrlw		mm7,8

		mov			edi,dword ptr[QZOut+4]

		xor			eax,eax
		mov			edi,dword ptr[edi]

		packuswb	mm7,mm7				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw ARGB|ARGB UZR|VZR   AR|GB
		mov			ax,word ptr[QZVal+6]

		movq		[ARL],mm6
		mov			word ptr[edi+30],ax

		mov			edi,dword ptr[QDibOut+4]

		pop			ebp
		mov			edi,dword ptr[edi]

		;regrab z
		movq		mm2,[QZVal32_0]
		movq		mm3,[QZVal32_1]

		paddd		mm2,[QZDelta]
		paddd		mm3,[QZDelta]

		movq		[QZVal32_0],mm2
		movq		[QZVal32_1],mm3

		movd		[edi+60],mm7

		; get corrected right side deltas ; ULw|VLw DU16|DV16 argb|ARGBagAG|rbRB    |ZRi   aA|rR   UZR|VZR aArR|gGbB

		movq		mm4,[ZIR]
		movq		mm6,[UVZ]

		movq		mm2,[UVR]
		movq		mm0,[UVLeftW]		; ULw|VLw DU16|DV16 argb|ARGBagAG|rbRB    |ZRi   aA|rR   UZR|VZR aArR|gGbB

		movq		mm1,[UVLeft]		; ULw|VLw   UL|VL   argb|ARGBagAG|rbRB ZRi|ZRi   aA|rR   UZR|VZR aArR|gGbB
		punpckldq	mm2,mm2				; ULw|VLw   UL|VL    ZRi|ZRi agAG|rbRB ZRi|ZRi   aA|rR   UZR|VZR aArR|gGbB

		pfmul		mm2,mm6				; ULw|VLw   UL|VL     UR|VR  agAG|rbRB ZRi|ZRi   aA|rR   UZR|VZR aArR|gGbB
		mov			eax,offset QZCan
		add			dword ptr[Dest],32

		add			dword ptr[eax+4],32
		mov			eax,offset QDibCan

		add			dword ptr[eax+4],64

		movq		mm3,mm2
		pfmul		mm3,[QFixedScale]

		pf2id		mm3,mm3

		dec		[NumASpans]			; dec num affine spans
		jnz		SpanLoopLit

HandleLeftoverPixelsLit:


		mov		esi,GBitPtr


		cmp			[RemainingCount],0
		jz			ReturnLit

		mov			eax,[RemainingCount]
		mov			dword ptr[ZIR],eax
		mov			dword ptr[ZIR+4],eax
		
		movq		mm7,[GLMapMulUV]	;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR   LU|LV
		movq		mm5,mm1

		pfsub		mm6,[UVDivZ16StepX]
		pfmul		mm7,mm1				;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR  ULL|VLL

		pfmul		mm5,[QFixedScale]
		pi2fd		mm3,[ZIR]

		pfmul		mm7,[QFixedScale]
		pfmul		mm3,[UVDivZStepX]

		pf2id		mm5,mm5
		pfadd		mm3,mm6

		pi2fd		mm2,[ZIR]
		movd		mm6,[Zi16StepX]

		pf2id		mm7,mm7				; ULw|VLw   DU|DV    UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |
		pfsub		mm4,mm6

		movd		mm6,[ZiStepX]
		mov			ebx,[RemainingCount]

		pfmul		mm2,mm6

		shl			ebx,3

		pfadd		mm2,mm4
		paddd		mm5,[UVAdjust]

		punpckldq	mm4,mm4

		movq		[UVL16],mm7
		pfrcp		mm2,mm2

		pfmul		mm4,[QZBufferPrec]

		paddd		mm5,[UVAdjust2]
		pfmul		mm2,mm3

		movq		[UVLeft],mm5
		pfsubr		mm1,mm2				; ULw|VLw   DU|DV    UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |

		pfmul		mm1,qword ptr[QFixedScaleLUT+ebx]	; ULw|VLw DU16|DV16  UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |

		movq		mm0,mm5
		pand		mm5,[WrapMask]
		pf2id		mm1,mm1				; ULw|VLw DU16|DV16  UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |

		movq		[UV16],mm5
		movq		mm7,[UVAdjustL]

		psrlq		mm5,[QShiftV]
		paddd		mm7,[UVL16]

		movq		[UV16V],mm5
		movq		[UVL16],mm7


OnePixelSpanLit:
		; Clamp U/V
		mov		ebx,dword ptr[UVL16+4]
		cmp		ebx,MaxU
		jle		TryClampU1Lit
		mov		ecx,MaxU
		mov		dword ptr[UVL16+4],ecx
		jmp		NoClampU1Lit

TryClampU1Lit:
		cmp		ebx,0
		jge		NoClampU1Lit
		mov		dword ptr[UVL16+4],0
NoClampU1Lit:
		mov		eax,dword ptr[UVL16]
		cmp		eax,MaxV
		jle		TryClampV1Lit
		mov		ecx,MaxV
		mov		dword ptr[UVL16],ecx
		jmp		NoClampV1Lit

TryClampV1Lit:
		cmp		eax,0
		jge		NoClampV1Lit
		mov		dword ptr[UVL16],0

NoClampV1Lit:
		mov			ebx,dword ptr[UV16+4]
		mov			eax,dword ptr[UV16V]

		mov			esi,GBitPtr
		shr			ebx,16
		add			esi,eax
		add			esi,ebx
		mov			ecx,TexPal
		xor			eax,eax
		movq		mm6,[ARL]

		mov			eax,offset QDibCan
		push		ebp

		mov			edi,dword ptr[eax+4]
		mov			ebp,dword ptr[Dest]

		movd		mm3,dword ptr[ZiStepX]
		xor			eax,eax

LeftoverLoopLit:
		paddd		mm0,mm1				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |
		pf2id		mm2,mm4

		mov			al,byte ptr[esi]
		movq		mm5,mm0				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw   UL|VL      |ZY   UZR|VZR     |

		mov			esi,GBitPtr
		pfadd		mm4,mm3

		pand		mm5,[WrapMask]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |
		psrld		mm2,16

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |ARGB

		movq		[UV16],mm5			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |
		movd		eax,mm2

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		xor			eax,0

		mov			ebx,dword ptr[UV16+4]

		psrlq		mm5,[QShiftV]

		shr			ebx,16

		movq		[UV16V],mm5
		add			esi,ebx

		mov			edx,dword ptr[UV16V]
		pmullw		mm7,mm6

;		paddw		mm6,mm3	no lerp for this... too much goin on
		psrlw		mm7,8

		add			esi,edx
		cmp			ax,word ptr[ebp]
		jl			SkipPixelLitZ

		packuswb	mm7,mm7

		movd		[edi],mm7

SkipPixelLitZ:

		xor			eax,eax
		add			edi,4
		add			ebp,2

		dec		[RemainingCount]
		jge		LeftoverLoopLit

		pop			ebp

ReturnLit:

	pop		edi
	pop		esi
	pop		ecx
	pop		ebx

	cRet DrawSpan32_AsmGouraudZBuffer3DNow
endProc DrawSpan32_AsmGouraudZBuffer3DNow


;argb alpha (greyscale), affine, gouraud

cProc DrawScanLineGouraudNoZAlphaARGB_Asm3DNow, 8,<pLeft : dword, pRight : dword>

		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		ebx,pLeft
		mov     eax,[ebx].X
		mov     ecx,pRight
		mov     edx,[ecx].X
		sub     edx,eax
		jle		GouraudReturnNoZAlphaTex

		mov		edi,[ebx].X
;		inc		edx

		femms

;		prefetch	[GBitPtr]

		movd		mm1,[ecx].vf
		movd		mm0,edx

		punpckldq	mm1,qword ptr[ecx].uf
		pi2fd		mm0,mm0

		movq		mm7,[GBL]
		movd		mm2,[ebx].vf

		pfrcp		mm0,mm0
		pfmul		mm1,mm7

		punpckldq	mm2,qword ptr[ebx].uf
		movd		mm3,[ecx].bf

		pfmul		mm2,mm7
		movq		mm7,mm0

		pfmul		mm0,[QFixedScale]
		pfsub		mm1,mm2

		punpckldq	mm3,qword ptr[ecx].gf
		pfmul		mm1,mm0

		movd		mm4,[ecx].rf

		pfmul		mm2,[QFixedScale]
		pf2id		mm1,mm1

		movd		mm5,[ebx].bf
		pf2id		mm2,mm2

		punpckldq	mm5,qword ptr[ebx].gf
		pfmul		mm7,[Q128]

		movd		mm6,[ebx].rf

		pfsub		mm3,mm5
		pfsub		mm4,mm6

		pfmul		mm3,mm7
		pf2id		mm5,mm5

		pfmul		mm4,mm7
		pf2id		mm6,mm6

		movq		mm0,mm2
		pf2id		mm4,mm4

		pf2id		mm3,mm3
		shl			edi,2

		pand		mm0,[WrapMask]
		packssdw	mm3,mm4

		movq		[UV16],mm0
		packssdw	mm5,mm6

		mov			ebx,dword ptr[UV16+4]
		add			edi,[Dest]

		psrlq		mm0,[QShiftV]
		psllw		mm5,7

		shr			ebx,16
		mov			esi,GBitPtr

		movq		[UV16V],mm0
		add			esi,ebx

		mov			ecx,TexPal
		mov			eax,dword ptr[UV16V]

		add			esi,eax
		xor			eax,eax

		push		ebp

GouraudLoopNoZAlphaTex:
		mov			al,byte ptr[esi]
		movq		mm0,mm2				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw   UL|VL      |ZY   UZR|VZR     |

;		sub			esi,GBitPtr
		paddd		mm2,mm1				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |

		pand		mm0,[WrapMask]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |ARGB

		movq		[UV16],mm0			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB

		mov			ebx,dword ptr[UV16+4]
		movq		mm4,mm7

		psrlq		mm0,[QShiftV]
		psrlq		mm4,48

;		add			esi,ABitPtr
		movq		mm6,mm4

		shr			ebx,16
		psllq		mm6,16

;		mov			al,byte ptr[esi]
		psllw		mm7,1

		por			mm4,mm6
		movq		[UV16V],mm0

		mov			esi,ebx
		movd		mm0,[edi]

		punpckldq	mm4,mm4
;		mov			ebx,ATexPal

		mov			ebp,dword ptr[UV16V]
		pmulhw		mm7,mm5

;		movd		mm4,[ebx+eax*4]
		add			esi,ebp

		punpcklbw	mm0,[Zero]

		movq		mm6,[QNegAlpha]

		punpcklbw	mm4,[Zero]

		add			esi,GBitPtr

		pmullw		mm7,mm4
		psubw		mm6,mm4

		paddw		mm5,mm3
		pmullw		mm0,mm6

		add			edi,4
		paddw		mm0,mm7

		psrlw		mm0,8

		packuswb	mm0,mm0
		
		movd		[edi-4],mm0

		dec			edx
		jge			GouraudLoopNoZAlphaTex

		pop			ebp

		femms

GouraudReturnNoZAlphaTex:

	pop		edi
	pop		esi
	pop		ecx
	pop		ebx

	cRet DrawScanLineGouraudNoZAlphaARGB_Asm3DNow
endProc DrawScanLineGouraudNoZAlphaARGB_Asm3DNow



;perspective correct, zbuffered, argb alpha, gouraud
;mean nasty function

cProc DrawSpan32_AsmGouraudZBufferAlphaARGB3DNow, 36,<x1 : dword, x2 : dword, y : dword, r1 : dword, g1 : dword, b1 : dword, r2 : dword, g2 : dword, b2 : dword>

		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		eax,x1
		mov		ecx,x2
		dec		ecx
		sub		ecx,eax
		jle		ReturnLit

;		femms

;		inc		ecx
		mov		ebx,offset ClientWindow
		mov		eax, y
		mov		edi,[ebx].Buffer
		mov		esi,x1
		imul	eax, [ebx].PixelPitch
		shl		esi,2
		mov		edx,ZBuffer
		add		eax,esi
		add		edi, eax
		shr		eax,1
		add		edx,eax
		mov		[Dest],edx
		mov		eax,offset QZCan
		mov		ebx,offset SCan
		mov		[eax],ebx
		mov		[eax+4],edx
		mov		eax,offset QDibCan
		mov		[eax],ebx
		mov		[eax+4],edi
		mov		ebx,offset QDibCan
		mov		eax,offset QDibOrCan
		mov		[eax],ebx
		mov		[eax+4],ebx
		mov		ebx,offset QZCan
		mov		eax,offset QZOrCan
		mov		[eax],ebx
		mov		[eax+4],ebx
		mov		edx,ecx
		mov		eax,ecx

		shr		ecx,4
		and		eax,15
		jnz		@f
		dec		ecx
		mov		eax,16
@@:

		mov		[NumASpans],ecx
		mov		[RemainingCount],eax

		;grab the left side lights
		movd		mm5,r1
		movd		mm4,b1

;		punpckldq	mm5,qword ptr[Zero]
		movd		mm6,g1

		pf2id		mm5,mm5
		punpckldq	mm4,mm6

		pf2id		mm4,mm4
		packssdw	mm4,mm5

;		psllw		mm4,7

		movq		[ARL],mm4

										;   mm0      mm1      mm2      mm3      mm4      mm5      mm6      mm7
		movd		mm0,x1				;    |x       |        |        |        |        |        |        |
		movq		mm2,[UVDivZStepX]	;    |x       |    UZdX|VZdX    |        |        |        |        |

		movd		mm1,y				;    |x       |y   UZdX|VZdX    |        |        |        |        |
		movq		mm3,[UVDivZStepY]	;    |x       |y   UZdX|VZdXUZdY|VZdY    |        |        |        |

		punpckldq	mm0,mm0				;   x|x       |y   UZdX|VZdXUZdY|VZdY    |        |        |        |
		punpckldq	mm1,mm1				;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |        |        |        |

		movd		mm7,edx				;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |        |        |        |wid
		movd		mm5,b2				;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |        |b       |        |wid

		pi2fd		mm0,mm0				;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |        |b       |        |wid
		movd		mm6,b1				;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |        |b       |b       |wid

		pi2fd		mm7,mm7				;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |        |b       |b       |wid
		punpckldq	mm5,qword ptr[g2]	;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |       g|b       |b       |wid

		pi2fd		mm1,mm1				;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |       g|b       |b       |wid
		punpckldq	mm6,qword ptr[g1]	;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |       g|b      g|b       |wid

		pfmul		mm2,mm0				;   x|x      y|y    UZX|VZX UZdY|VZdY    |       g|b      g|b       |wid
		pfrcp		mm7,mm7				;   x|x      y|y    UZX|VZX UZdY|VZdY    |       g|b      g|b     dw|dw

		pfsub		mm5,mm6				;   x|x      y|y    UZX|VZX UZdY|VZdY    |      gd|bd     g|b     dw|dw
		movd		mm4,[r1]			;   x|x      y|y    UZX|VZX UZdY|VZdY    |r     gd|bd     g|b     dw|dw

		movd		mm6,[r2]			;   x|x      y|y    UZX|VZX UZdY|VZdY    |r     gd|bd     g|r     dw|dw
		pfmul		mm3,mm1				;   x|x      y|y    UZX|VZX  UZY|VZY     |r     gd|bd     g|r     dw|dw

		pfsub		mm6,mm4				;   x|x      y|y    UZX|VZX  UZY|VZY     |r     gd|bd     x|rd    dw|dw
		pfmul		mm7,[Q128]			;   x|x      y|y    UZX|VZX  UZY|VZY     |r     gd|bd     x|rd    DW|DW

		movd		mm4,[ZiStepX]		;   x|x      y|y    UZX|VZX  UZY|VZY     |ZdX   gd|bd     x|rd    DW|DW

		punpckldq	mm4,mm4
		pfmul		mm4,[QZBufferPrec]
		pf2id		mm4,mm4
		movq		[QZDelta],mm4

		movd		mm4,[ZiStepX]		;   x|x      y|y    UZX|VZX UZdY|VZdY    |ZdX     |        |        |
		pfmul		mm5,mm7				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZdX   GD|BD     x|rd    DW|DW

		pfmul		mm4,mm0				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX    GD|BD     x|rd    DW|DW
		pfmul		mm6,mm7				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX    GD|BD     x|RD    DW|DW

		pf2id		mm5,mm5
		pf2id		mm6,mm6

		packssdw	mm5,mm6
		movq		[RGBADelta],mm5

		movd		mm5,[ZiStepY]		;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX      |ZdY    x|RD    DW|DW
		movq		mm6,[UVDivZOrigin]	;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX      |ZdY  UZO|VZO   DW|DW

		pfmul		mm5,mm1				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX      |ZY   UZO|VZO   DW|DW
		movd		mm7,[ZiOrigin]		;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX      |ZY   UZO|VZO     |ZO

		pfadd		mm6,mm2				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX      |ZY  UZXS|VZXS    |ZO
		pfadd		mm4,mm7				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZXS     |ZY  UZXS|VZXS    |ZO

		pfadd		mm6,mm3				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZXS     |ZY    UZ|VZ      |ZO
		pfadd		mm4,mm5				;   x|x      y|y    UZX|VZX  UZY|VZY     |Zi      |ZY    UZ|VZ      |ZO

		movq		mm7,mm4
		movq		mm1,[QZDelta]
		punpckldq	mm7,mm7
		pfmul		mm7,[QZBufferPrec]
		pf2id		mm7,mm7
		movq		mm0,mm7
		paddd		mm7,mm1
		punpckldq	mm0,mm7
		paddd		mm7,mm1
		movq		[QZVal32_0],mm0
		movq		mm0,mm7
		paddd		mm7,mm1
		punpckldq	mm0,mm7
		movq		[QZVal32_1],mm0
		pslld		mm1,2
		movd		mm7,[Zi16StepX]
		movq		[QZDelta],mm1

		pfrcp		mm0,mm4				;  ZL|ZL     y|y    UZX|VZX  UZY|VZY     |Zi      |ZY    UZ|VZ      |ZdX16

		movq		mm1,mm6				;  ZL|ZL    UZ|VZ   UZX|VZX  UZY|VZY     |Zi      |ZY    UZ|VZ      |ZdX16
		pfadd		mm4,mm7				;  ZL|ZL    UZ|VZ   UZX|VZX  UZY|VZY     |ZRi     |ZY    UZ|VZ      |ZdX16

		pfmul		mm1,mm0				;  ZL|ZL    UL|VL   UZX|VZX  UZY|VZY     |ZRi     |ZY    UZ|VZ      |ZdX16
		pfrcp		mm2,mm4				;  ZL|ZL    UL|VL    ZR|ZR   UZY|VZY     |ZRi     |ZY    UZ|VZ      |ZdX16

		pfadd		mm6,[UVDivZ16StepX] ;  ZL|ZL    UL|VL    ZR|ZR   UZY|VZY     |ZRi     |ZY   UZR|VZR     |ZdX16
		pfmul		mm2,mm6				;  ZL|ZL    UL|VL    UR|VR   UZY|VZY     |ZRi     |ZY   UZR|VZR     |ZdX16

		movq		mm0,mm1				;  UL|VL    UL|VL    UR|VR   UZY|VZY     |ZRi     |ZY   UZR|VZR     |ZdX16
		movq		mm3,mm2				; ULi|VLi	UL|VL    UR|UR    UR|UR      |ZRi     |ZY   UZR|VZR     |ZdX16

		pfmul		mm0,[QFixedScale]	; UL6|VL6	UL|VL    UR|UR    UR|UR      |ZRi     |ZY   UZR|VZR     |ZdX16
		pfmul		mm3,[QFixedScale]	; UL6|VL6	UL|VL   UR6|UR6  UR6|UR6     |ZRi     |ZY   UZR|VZR     |ZdX16

		pf2id		mm0,mm0				;UL6i|VL6i  UL|VL   UR6|VR6  UR6|VR6     |ZRi     |ZY   UZR|VZR     |ZdX16
		pf2id		mm3,mm3				;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR     |ZdX16


		test	ecx,ecx
		jz		HandleLeftoverPixelsLit

		movq		mm7,[GLMapMulUV]	;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR   LU|LV
		pfmul		mm7,mm1				;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR  ULL|VLL
		pfmul		mm7,[QFixedScale]
		pf2id		mm7,mm7				; ULw|VLw   DU|DV    UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |
		paddd		mm7,[UVAdjustL]
		movq		[UVL16],mm7

		; Clamp U/V
		mov		ebx,dword ptr[UVL16+4]

		cmp		ebx,MaxU
		jle		TryClampU0Litp
		mov		ecx,MaxU
		mov		dword ptr[UVL16+4],ecx
		jmp		NoClampU0Litp

TryClampU0Litp:
		cmp		ebx,0
		jge		NoClampU0Litp
		mov		dword ptr[UVL16+4],0
NoClampU0Litp:
		mov		eax,dword ptr[UVL16]
		cmp		eax,MaxV
		jle		TryClampV0Litp
		mov		ecx,MaxV
		mov		dword ptr[UVL16],ecx
		jmp		NoClampV0Litp

TryClampV0Litp:
		cmp		eax,0
		jge		NoClampV0Litp
		mov		dword ptr[UVL16],0

NoClampV0Litp:

		pfrcp		mm5,mm4

SpanLoopLit:
		;use float uv for lightmap uv
		movq		mm7,[GLMapMulUV]	;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR   LU|LV
		movq		mm5,mm1

		pfmul		mm7,mm2				;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR  ULL|VLL
		pfmul		mm5,[QFixedScale]

		pfmul		mm7,[QFixedScale]
		pf2id		mm5,mm5

		pf2id		mm7,mm7				; ULw|VLw   DU|DV    UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |
		paddd		mm5,[UVAdjust]

		pfsubr		mm1,mm2				; ULw|VLw   DU|DV    UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |
		paddd		mm5,[UVAdjust2]

		movq		[UVL16],mm7
		movq		[UVLeft],mm5

		movd		mm7,[Zi16StepX]
		movq		mm0,mm5

		pfmul		mm1,[QFixedScale16]	; ULw|VLw DU16|DV16  UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |
		;right side stuff becomes left	; ULw|VLw DU16|DV16  UL|VL   URw|VRw     |ZLi     |ZY   UZL|VZL     |
		pfadd		mm4,mm7				;ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZL|VZL     |

		pand		mm5,[WrapMask]
		pf2id		mm1,mm1				; ULw|VLw DU16|DV16  UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |

		movq		mm7,[UVAdjustL]
		movq		[UV16],mm5

		pfadd		mm6,[UVDivZ16StepX]	; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |
		paddd		mm7,[UVL16]

		psrlq		mm5,[QShiftV]
		movq		[UVL16],mm7

		movq		[UV16V],mm5


		; Clamp U/V
		mov		ebx,dword ptr[UVL16+4]
		cmp		ebx,MaxU
		jle		TryClampU0Lit
		mov		ecx,MaxU
		mov		dword ptr[UVL16+4],ecx
		jmp		NoClampU0Lit

TryClampU0Lit:
		cmp		ebx,0
		jge		NoClampU0Lit
		mov		dword ptr[UVL16+4],0
NoClampU0Lit:
		mov		eax,dword ptr[UVL16]
		cmp		eax,MaxV
		jle		TryClampV0Lit
		mov		ecx,MaxV
		mov		dword ptr[UVL16],ecx
		jmp		NoClampV0Lit

TryClampV0Lit:
		cmp		eax,0
		jge		NoClampV0Lit
		mov		dword ptr[UVL16],0

NoClampV0Lit:
		movq		[UVLeftW],mm3
		mov			eax,dword ptr[UV16V]
		mov			ebx,dword ptr[UV16+4]
		movq		[ZIR],mm4
		mov			esi,GBitPtr
		shr			ebx,16
		movq		[UVZ],mm6
		add			esi,eax
		add			esi,ebx
		movq		[UVLeft],mm2

		mov			ecx,TexPal
		pfrcp		mm5,mm4
;		movq		mm3,[RGBADelta]
		xor			eax,eax
		movq		mm6,[ARL]

		push		ebp
		mov			ebp,[Dest]

		;grab zbuffer values
		movq		mm2,[QZVal32_0]
		movq		mm7,[QZVal32_1]

		psrld		mm2,17
		psrld		mm7,17

		packssdw	mm2,mm7

		pslld		mm2,1
		movq		[QZVal],mm2


		movq		mm2,[QZVal]
		paddd		mm0,mm1				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |

		mov			al,byte ptr[esi]
		movq		mm4,mm0				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw   UL|VL      |ZY   UZR|VZR     |

		pcmpgtw		mm2,[ebp]
		mov			esi,GBitPtr

		psrlw		mm2,15
		pand		mm4,[WrapMask]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		psllw		mm2,2
		movq		[UV16],mm4			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movq		[GBL],mm2
		psrlq		mm4,[QShiftV]

		movq		mm7,mm2
		movq		[UV16V],mm4

		punpcklwd	mm7,[Zero]
		movq		[UVR],mm5

		paddd		mm7,[QZOrCan]
		mov			edx,dword ptr[UV16V]

		movq		[QZOut],mm7
		mov			ebx,dword ptr[UV16+4]

		movq		mm7,mm2
		mov			edi,dword ptr[QZOut]

		punpcklwd	mm7,[Zero]
		shr			ebx,16

		paddd		mm7,[QDibOrCan]
		mov			edi,dword ptr[edi]

		movq		[QDibOut],mm7
		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |ARGB

		add			esi,edx
		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB

		xor			eax,eax
		movq		mm3,mm7

		pmullw		mm7,mm6
		mov			ax,word ptr[QZVal]

		psrlq		mm3,48
		paddd		mm0,mm1

		mov			word ptr[edi],ax
		movq		mm5,mm3

		mov			edi,dword ptr[QDibOut]
		psllq		mm5,16

		add			esi,ebx
		por			mm3,mm5

		mov			edi,dword ptr[edi]
		punpckldq	mm3,mm3
		paddw		mm6,[RGBADelta]

		psrlw		mm7,8

		xor			eax,eax
		movq		mm5,[QNegAlpha]

		pmullw		mm7,mm3
		psubw		mm5,mm3

		movq		mm3,[edi]
		mov			al,byte ptr[esi]

		punpcklbw	mm3,[Zero]
		movq		mm4,mm0

		pmullw		mm3,mm5
		pand		mm4,[WrapMask]

		paddw		mm3,mm7
		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB

		psrlw		mm3,8
		movq		[UV16],mm4

		packuswb	mm3,mm3
		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB

		movd		[edi],mm3
		movq		mm3,mm7

		psrlq		mm4,[QShiftV]
		psrlq		mm3,48

		mov			ebx,dword ptr[UV16+4]
		movq		mm5,mm3

		movq		[UV16V],mm4
		psllq		mm5,16

		pmullw		mm7,mm6
		por			mm3,mm5

		mov			edx,dword ptr[UV16V]
		punpckldq	mm3,mm3

		mov			edi,dword ptr[QZOut+4]
		psrlw		mm7,8

		xor			eax,eax
		mov			edi,dword ptr[edi]

		mov			ax,word ptr[QZVal+2]
		mov			esi,GBitPtr

		movq		mm5,[QNegAlpha]
		pmullw		mm7,mm3

		mov			word ptr[edi+2],ax
		xor			eax,eax

		mov			edi,dword ptr[QDibOut+4]
		shr			ebx,16

		mov			edi,dword ptr[edi]
		psubw		mm5,mm3

		paddw		mm6,[RGBADelta]
		add			esi,ebx

		movq		mm3,[edi+4]
		add			esi,edx

		punpcklbw	mm3,[Zero]
		mov			al,byte ptr[esi]

		pmullw		mm3,mm5
		paddd		mm0,mm1

		paddw		mm3,mm7
		packuswb	mm7,mm7				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw ARGB|ARGB UZR|VZR   AR|GB

		psrlw		mm3,8
		movq		mm4,mm0

		packuswb	mm3,mm3
		pand		mm4,[WrapMask]

		mov			esi,GBitPtr
		movd		[edi+4],mm3

		movq		mm7,mm2
		movq		[UV16],mm4

		punpckhwd	mm7,[Zero]
		psrlq		mm4,[QShiftV]

		paddd		mm7,[QZorCan]
		movq		[UV16V],mm4

		movq		[QZOut],mm7
		mov			ebx,dword ptr[UV16+4]

		movq		mm7,mm2
		mov			edi,dword ptr[QZOut]

		punpckhwd	mm7,[Zero]
		mov			edi,dword ptr[edi]

		paddd		mm7,[QDibOrCan]
		paddd		mm0,mm1

		movq		[QDibOut],mm7
		mov			edx,dword ptr[UV16V]

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB
		xor			eax,0

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		mov			ax,word ptr[QZVal+4]

		movq		mm3,mm7
		pmullw		mm7,mm6

		psrlq		mm3,48
		mov			word ptr[edi+4],ax

		movq		mm5,mm3
		psrlw		mm7,8

		psllq		mm5,16
		mov			edi,dword ptr[QDibOut]

		por			mm3,mm5
		paddw		mm6,[RGBADelta]

		punpckldq	mm3,mm3
		mov			edi,dword ptr[edi]

		movq		mm5,[QNegAlpha]

		pmullw		mm7,mm3
		psubw		mm5,mm3

		movq		mm3,[edi+8]
		movq		mm4,mm0

		punpcklbw	mm3,[Zero]
		shr			ebx,16

		pmullw		mm3,mm5
		xor			eax,eax

		paddw		mm3,mm7
		pand		mm4,[WrapMask]

		psrlw		mm3,8
		add			esi,edx

		packuswb	mm3,mm3
		paddd		mm0,mm1

		movd		[edi+8],mm3
		add			esi,ebx

		movq		[UV16],mm4
		mov			al,byte ptr[esi]

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		psrlq		mm4,[QShiftV]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		movq		[UV16V],mm4

		movq		mm3,mm7
		mov			ebx,dword ptr[UV16+4]

		psrlq		mm3,48
		mov			edx,dword ptr[UV16V]

		movq		mm5,mm3
		pmullw		mm7,mm6

		psllq		mm5,16
		shr			ebx,16

		por			mm3,mm5
		xor			eax,eax

		paddw		mm6,[RGBADelta]
		punpckldq	mm3,mm3

		mov			edi,dword ptr[QZOut+4]
		psrlw		mm7,8

		movq		mm5,[QNegAlpha]
		mov			esi,edx

		pmullw		mm7,mm3
		add			esi,GBitPtr

		mov			edi,dword ptr[edi]
		psubw		mm5,mm3

		mov			ax,word ptr[QZVal+6]
		add			esi,ebx

		mov			word ptr[edi+6],ax
		mov			edi,dword ptr[QDibOut+4]

		xor			eax,eax
		mov			edi,dword ptr[edi]

		mov			al,byte ptr[esi]
		movq		mm3,[edi+12]

		movq		mm4,mm0
		punpcklbw	mm3,[Zero]

		;regrab z
		movq		mm2,[QZVal32_0]
		pmullw		mm3,mm5

		paddd		mm2,[QZDelta]
		paddw		mm3,mm7

		movq		mm7,[QZVal32_1]
		psrlw		mm3,8

		paddd		mm7,[QZDelta]
		packuswb	mm3,mm3

		movq		[QZVal32_0],mm2
		movq		[QZVal32_1],mm7

		movd		[edi+12],mm3

		psrld		mm2,17
		psrld		mm7,17

		packssdw	mm2,mm7
		pand		mm4,[WrapMask]

		pslld		mm2,1
		movq		[UV16],mm4

		movq		[QZVal],mm2
		mov			esi,GBitPtr

		pcmpgtw		mm2,[ebp+8]
		psrlq		mm4,[QShiftV]


		psrlw		mm2,15
		movq		[UV16V],mm4


		psllw		mm2,2
		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB

		movq		mm4,mm2
		mov			ebx,dword ptr[UV16+4]

		punpcklwd	mm4,[Zero]
		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB


		paddd		mm4,[QZorCan]
		movq		mm3,mm7

		pmullw		mm7,mm6
		psrlq		mm3,48

		movq		[QZOut],mm4
		movq		mm5,mm3

		xor			eax,eax
		psllq		mm5,16

		mov			edi,dword ptr[QZOut]
		movq		mm4,mm2

		mov			edi,dword ptr[edi]
		por			mm3,mm5

		mov			ax,word ptr[QZVal]
		punpckldq	mm3,mm3

		mov			word ptr[edi+8],ax
		psrlw		mm7,8

		paddw		mm6,[RGBADelta]
		punpcklwd	mm4,[Zero]

		paddd		mm4,[QDibOrCan]
		pmullw		mm7,mm3

		movq		mm5,[QNegAlpha]
		paddd		mm0,mm1

		movq		[QDibOut],mm4
		psubw		mm5,mm3

		mov			edi,dword ptr[QDibOut]
		mov			edx,dword ptr[UV16V]

		mov			edi,dword ptr[edi]
		movq		mm4,mm0

		movq		mm3,[edi+16]
		shr			ebx,16

		punpcklbw	mm3,[Zero]
		xor			eax,eax

		pand		mm4,[WrapMask]
		pmullw		mm3,mm5

		add			esi,edx
		paddw		mm3,mm7

		paddd		mm0,mm1
		psrlw		mm3,8

		add			esi,ebx
		packuswb	mm3,mm3

		mov			al,byte ptr[esi]
		movq		[UV16],mm4

		movd		[edi+16],mm3

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		psrlq		mm4,[QShiftV]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		movq		[UV16V],mm4

		movq		mm3,mm7
		mov			ebx,dword ptr[UV16+4]

		psrlq		mm3,48
		mov			edx,dword ptr[UV16V]

		movq		mm5,mm3
		mov			edi,dword ptr[QZOut+4]

		pmullw		mm7,mm6
		mov			edi,dword ptr[edi]

		psllq		mm5,16
		mov			ax,word ptr[QZVal+2]

		shr			ebx,16
		mov			word ptr[edi+10],ax

		por			mm3,mm5
		xor			eax,eax

		mov			edi,dword ptr[QDibOut+4]
		psrlw		mm7,8

		punpckldq	mm3,mm3
		mov			esi,GBitPtr
		mov			edi,dword ptr[edi]

		movq		mm5,[QNegAlpha]
		pmullw		mm7,mm3

		psubw		mm5,mm3
		movq		mm3,[edi+20]

		punpcklbw	mm3,[Zero]
		paddw		mm6,[RGBADelta]

		pmullw		mm3,mm5
		add			esi,edx

		paddw		mm3,mm7
		xor			eax,eax

		psrlw		mm3,8
		movq		mm4,mm0

		packuswb	mm3,mm3

		pand		mm4,[WrapMask]
		movd		[edi+20],mm3

		movq		[UV16],mm4

		add			esi,ebx
		psrlq		mm4,[QShiftV]

		movq		[UV16V],mm4

		mov			al,byte ptr[esi]


		mov			esi,GBitPtr

		movq		mm4,mm2
		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB

		punpckhwd	mm4,[Zero]

		mov			ebx,dword ptr[UV16+4]
		paddd		mm4,[QZorCan]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		movq		[QZOut],mm4

		movq		mm3,mm7
		movq		mm4,mm2

		mov			edi,dword ptr[QZOut]
		punpckhwd	mm4,[Zero]

		mov			edi,dword ptr[edi]
		mov			ax,word ptr[QZVal+4]
		psrlq		mm3,48

		mov			word ptr[edi+12],ax
		pmullw		mm7,mm6

		movq		mm5,mm3
		xor			eax,eax

		psllq		mm5,16
		paddd		mm4,[QDibOrCan]

		por			mm3,mm5
		paddw		mm6,[RGBADelta]
		movq		[QDibOut],mm4

		punpckldq	mm3,mm3
		psrlw		mm7,8

		mov			edi,dword ptr[QDibOut]
		movq		mm5,[QNegAlpha]

		pmullw		mm7,mm3
		mov			edi,dword ptr[edi]

		psubw		mm5,mm3
		movq		mm3,[edi+24]

		paddd		mm0,mm1
		punpcklbw	mm3,[Zero]

		pmullw		mm3,mm5
		mov			edx,dword ptr[UV16V]

		paddw		mm3,mm7
		movq		mm4,mm0

		psrlw		mm3,8
		shr			ebx,16

		packuswb	mm3,mm3
		xor			eax,eax

		pand		mm4,[WrapMask]
		movd		[edi+24],mm3

		add			esi,edx
		mov			edi,dword ptr[QZOut+4]

		paddd		mm0,mm1
		mov			edi,dword ptr[edi]
		add			esi,ebx

		mov			al,byte ptr[esi]
		movq		[UV16],mm4

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		psrlq		mm4,[QShiftV]

		xor			eax,eax

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		movq		[UV16V],mm4

		mov			ax,word ptr[QZVal+6]
		movq		mm3,mm7

		mov			ebx,dword ptr[UV16+4]
		mov			word ptr[edi+14],ax

		psrlq		mm3,48
		mov			edx,dword ptr[UV16V]

		movq		mm5,mm3
		mov			edi,dword ptr[QDibOut+4]
		pmullw		mm7,mm6

		psllq		mm5,16
		shr			ebx,16

		por			mm3,mm5
		psrlw		mm7,8

		punpckldq	mm3,mm3
		paddw		mm6,[RGBADelta]

		movq		mm5,[QNegAlpha]
		pmullw		mm7,mm3

		mov			edi,dword ptr[edi]
		psubw		mm5,mm3



		movq		mm3,[edi+28]
		mov			esi,GBitPtr

		punpcklbw	mm3,[Zero]
		add			esi,edx

		pmullw		mm3,mm5
		xor			eax,eax

		paddw		mm3,mm7
		add			esi,ebx

		;regrab z
		movq		mm2,[QZVal32_0]
		psrlw		mm3,8

		movq		mm4,[QZVal32_1]
		packuswb	mm3,mm3

		paddd		mm2,[QZDelta]
		paddd		mm4,[QZDelta]

		movq		[QZVal32_0],mm2
		movq		[QZVal32_1],mm4

		psrld		mm2,17
		psrld		mm4,17

		packssdw	mm2,mm4
		xor			eax,eax

		pslld		mm2,1


		movq		[QZVal],mm2

		mov			al,byte ptr[esi]
		movq		mm4,mm0

		pand		mm4,[WrapMask]
		pcmpgtw		mm2,[ebp+16]

		movd		[edi+28],mm3
		movq		[UV16],mm4

		psrlw		mm2,15
		mov			esi,GBitPtr

		psrlq		mm4,[QShiftV]

		psllw		mm2,2
		movq		[UV16V],mm4
		movq		mm4,mm2

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB
		punpcklwd	mm4,[Zero]

		mov			ebx,dword ptr[UV16+4]
		paddd		mm4,[QZorCan]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		movq		[QZOut],mm4

		movq		mm3,mm7
		mov			edi,dword ptr[QZOut]

		movq		mm4,mm2
		mov			edi,dword ptr[edi]

		psrlq		mm3,48
		pmullw		mm7,mm6

		movq		mm5,mm3
		xor			eax,eax

		psllq		mm5,16
		mov			ax,word ptr[QZVal]

		por			mm3,mm5
		punpcklwd	mm4,[Zero]
		mov			word ptr[edi+16],ax

		punpckldq	mm3,mm3
		paddw		mm6,[RGBADelta]

		paddd		mm4,[QDibOrCan]
		psrlw		mm7,8

		movq		[QDibOut],mm4
		movq		mm5,[QNegAlpha]

		mov			edi,dword ptr[QDibOut]
		pmullw		mm7,mm3

		mov			edi,dword ptr[edi]
		psubw		mm5,mm3
		mov			edx,dword ptr[UV16V]

		movq		mm3,[edi+32]
		paddd		mm0,mm1

		punpcklbw	mm3,[Zero]
		shr			ebx,16

		pmullw		mm3,mm5
		xor			eax,eax

		paddw		mm3,mm7
		add			esi,edx

		psrlw		mm3,8
		add			esi,ebx

		movq		mm4,mm0
		packuswb	mm3,mm3

		pand		mm4,[WrapMask]
		movd		[edi+32],mm3

		paddd		mm0,mm1
		mov			edi,dword ptr[QZOut+4]

		mov			al,byte ptr[esi]
		movq		[UV16],mm4

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		psrlq		mm4,[QShiftV]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		movq		[UV16V],mm4

		movq		mm3,mm7
		mov			ebx,dword ptr[UV16+4]

		psrlq		mm3,48
		mov			edx,dword ptr[UV16V]

		movq		mm5,mm3
		mov			edi,dword ptr[edi]

		pmullw		mm7,mm6
		mov			ax,word ptr[QZVal+2]

		psllq		mm5,16
		mov			word ptr[edi+18],ax

		psrlw		mm7,8
		mov			edi,dword ptr[QDibOut+4]

		por			mm3,mm5
		paddw		mm6,[RGBADelta]

		punpckldq	mm3,mm3
		mov			edi,dword ptr[edi]

		shr			ebx,16
		movq		mm5,[QNegAlpha]

		pmullw		mm7,mm3
		psubw		mm5,mm3

		movq		mm3,[edi+36]
		mov			esi,GBitPtr

		punpcklbw	mm3,[Zero]
		add			esi,edx

		pmullw		mm3,mm5
		xor			eax,eax

		paddw		mm3,mm7
		movq		mm4,mm0

		psrlw		mm3,8
		pand		mm4,[WrapMask]

		packuswb	mm3,mm3
		movq		[UV16],mm4

		add			esi,ebx
		psrlq		mm4,[QShiftV]

		xor			eax,eax
		movq		[UV16V],mm4

		mov			al,byte ptr[esi]

		mov			esi,GBitPtr
		movd		[edi+36],mm3

		movq		mm4,mm2

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB

		punpckhwd	mm4,[Zero]

		mov			ebx,dword ptr[UV16+4]
		paddd		mm4,[QZorCan]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		movq		[QZOut],mm4

		movq		mm3,mm7
		movq		mm4,mm2

		mov			edi,dword ptr[QZOut]
		psrlq		mm3,48

		mov			ax,word ptr[QZVal+4]
		pmullw		mm7,mm6

		mov			edi,dword ptr[edi]
		movq		mm5,mm3

		punpckhwd	mm4,[Zero]
		mov			word ptr[edi+20],ax
		xor			eax,eax

		paddd		mm4,[QDibOrCan]
		psllq		mm5,16

		movq		[QDibOut],mm4
		por			mm3,mm5

		mov			edi,dword ptr[QDibOut]
		paddw		mm6,[RGBADelta]

		punpckldq	mm3,mm3

		mov			edi,dword ptr[edi]
		psrlw		mm7,8

		movq		mm5,[QNegAlpha]
		pmullw		mm7,mm3

		mov			edx,dword ptr[UV16V]
		psubw		mm5,mm3

		movq		mm3,[edi+40]
		paddd		mm0,mm1

		punpcklbw	mm3,[Zero]
		movq		mm4,mm0

		pmullw		mm3,mm5
		shr			ebx,16

		paddw		mm3,mm7
		add			esi,edx

		psrlw		mm3,8
		add			esi,ebx

		packuswb	mm3,mm3
		xor			eax,eax

		movd		[edi+40],mm3
		paddd		mm0,mm1

		pand		mm4,[WrapMask]
		mov			al,byte ptr[esi]

		movq		[UV16],mm4

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		psrlq		mm4,[QShiftV]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB

		movq		[UV16V],mm4
		movq		mm3,mm7

		mov			ebx,dword ptr[UV16+4]
		psrlq		mm3,48

		mov			edx,dword ptr[UV16V]
		movq		mm5,mm3

		mov			edi,dword ptr[QZOut+4]
		pmullw		mm7,mm6

		mov			edi,dword ptr[edi]
		psllq		mm5,16

		mov			ax,word ptr[QZVal+6]
		psrlw		mm7,8

		mov			word ptr[edi+22],ax
		por			mm3,mm5

		mov			edi,dword ptr[QDibOut+4]
		shr			ebx,16

		punpckldq	mm3,mm3
		paddw		mm6,[RGBADelta]

		movq		mm5,[QNegAlpha]
		mov			edi,dword ptr[edi]

		pmullw		mm7,mm3
		psubw		mm5,mm3

		movq		mm3,[edi+44]
		mov			esi,GBitPtr

		punpcklbw	mm3,[Zero]
		add			esi,ebx

		pmullw		mm3,mm5
		add			esi,edx

		paddw		mm3,mm7
		xor			eax,eax

		;regrab z
		movq		mm2,[QZVal32_0]
		psrlw		mm3,8

		movq		mm4,[QZVal32_1]
		packuswb	mm3,mm3

		paddd		mm2,[QZDelta]
		paddd		mm4,[QZDelta]

		movq		[QZVal32_0],mm2
		movq		[QZVal32_1],mm4

		psrld		mm2,17
		psrld		mm4,17

		packssdw	mm2,mm4
		xor			eax,eax

		pslld		mm2,1


		movq		[QZVal],mm2

		mov			al,byte ptr[esi]
		movq		mm4,mm0

		pand		mm4,[WrapMask]
		pcmpgtw		mm2,[ebp+24]

		movd		[edi+44],mm3
		movq		[UV16],mm4

		psrlw		mm2,15
		mov			esi,GBitPtr

		psrlq		mm4,[QShiftV]

		psllw		mm2,2
		movq		[UV16V],mm4

		movq		mm4,mm2


		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB
		punpcklwd	mm4,[Zero]

		mov			ebx,dword ptr[UV16+4]
		paddd		mm4,[QZorCan]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB

		movq		[QZOut],mm4
		movq		mm3,mm7

		mov			edi,dword ptr[QZOut]
		movq		mm4,mm2

		mov			ax,word ptr[QZVal]
		psrlq		mm3,48

		mov			edi,dword ptr[edi]
		pmullw		mm7,mm6

		mov			word ptr[edi+24],ax
		movq		mm5,mm3

		punpcklwd	mm4,[Zero]
		xor			eax,eax

		paddd		mm4,[QDibOrCan]
		psllq		mm5,16

		movq		[QDibOut],mm4
		por			mm3,mm5

		paddw		mm6,[RGBADelta]
		punpckldq	mm3,mm3

		mov			edi,dword ptr[QDibOut]
		psrlw		mm7,8

		movq		mm5,[QNegAlpha]
		pmullw		mm7,mm3

		mov			edi,dword ptr[edi]
		psubw		mm5,mm3

		movq		mm3,[edi+48]
		paddd		mm0,mm1

		punpcklbw	mm3,[Zero]
		movq		mm4,mm0

		pmullw		mm3,mm5
		shr			ebx,16

		mov			edx,dword ptr[UV16V]
		paddw		mm3,mm7

		pand		mm4,[WrapMask]
		psrlw		mm3,8

		movq		[UV16],mm4
		packuswb	mm3,mm3

		xor			eax,eax
		add			esi,edx

		movd		[edi+48],mm3
		paddd		mm0,mm1

		add			esi,ebx
		mov			al,byte ptr[esi]

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		psrlq		mm4,[QShiftV]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB

		movq		[UV16V],mm4
		movq		mm3,mm7

		mov			ebx,dword ptr[UV16+4]
		psrlq		mm3,48

		mov			edx,dword ptr[UV16V]
		movq		mm5,mm3

		mov			edi,dword ptr[QZOut+4]
		pmullw		mm7,mm6

		mov			ax,word ptr[QZVal+2]
		psllq		mm5,16

		mov			edi,dword ptr[edi]
		psrlw		mm7,8

		mov			word ptr[edi+26],ax
		por			mm3,mm5

		mov			edi,dword ptr[QDibOut+4]
		shr			ebx,16

		punpckldq	mm3,mm3
		paddw		mm6,[RGBADelta]

		movq		mm5,[QNegAlpha]
		pmullw		mm7,mm3

		mov			edi,dword ptr[edi]
		psubw		mm5,mm3

		movq		mm3,[edi+52]
		mov			esi,GBitPtr

		punpcklbw	mm3,[Zero]
		movq		mm4,mm0

		pmullw		mm3,mm5
		add			esi,edx

		paddw		mm3,mm7
		xor			eax,eax

		pand		mm4,[WrapMask]
		psrlw		mm3,8

		movq		[UV16],mm4
		packuswb	mm3,mm3

		psrlq		mm4,[QShiftV]
		add			esi,ebx

		movq		[UV16V],mm4
		xor			eax,eax


		movd		[edi+52],mm3
		mov			al,byte ptr[esi]

		mov			esi,GBitPtr
		movq		mm4,mm2

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB

		punpckhwd	mm4,[Zero]

		mov			ebx,dword ptr[UV16+4]
		paddd		mm4,[QZorCan]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		movq		[QZOut],mm4
		movq		mm3,mm7

		mov			edi,dword ptr[QZOut]
		movq		mm4,mm2

		mov			ax,word ptr[QZVal+4]
		psrlq		mm3,48

		mov			edi,dword ptr[edi]
		pmullw		mm7,mm6

		mov			word ptr[edi+28],ax
		movq		mm5,mm3

		xor			eax,eax
		psllq		mm5,16

		punpckhwd	mm4,[Zero]
		psrlw		mm7,8

		paddd		mm4,[QDibOrCan]
		por			mm3,mm5

		paddw		mm6,[RGBADelta]
		punpckldq	mm3,mm3

		movq		mm5,[QNegAlpha]
		pmullw		mm7,mm3

		movq		[QDibOut],mm4
		psubw		mm5,mm3

		mov			edi,dword ptr[QDibOut]
		paddd		mm0,mm1

		mov			edi,dword ptr[edi]
		movq		mm4,mm0

		movq		mm3,[edi+56]
		shr			ebx,16

		punpcklbw	mm3,[Zero]

		mov			edx,dword ptr[UV16V]
		pmullw		mm3,mm5

		pand		mm4,[WrapMask]
		paddw		mm3,mm7

		add			esi,edx
		psrlw		mm3,8

		add			esi,ebx
		packuswb	mm3,mm3

		paddd		mm0,mm1
		mov			al,byte ptr[esi]

		movd		[edi+56],mm3

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		xor			eax,eax

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		mov			edi,dword ptr[QZOut+4]

		mov			ax,word ptr[QZVal+6]
		movq		mm3,mm7

		mov			edi,dword ptr[edi]
		pmullw		mm7,mm6

		mov			word ptr[edi+30],ax
		psrlq		mm3,48

		mov			edi,dword ptr[QDibOut+4]
		movq		mm5,mm3

		paddw		mm6,[RGBADelta]
		psllq		mm5,16

		mov			edi,dword ptr[edi]
		psrlw		mm7,8

		movq		[ARL],mm6
		por			mm3,mm5

		pop			ebp
		punpckldq	mm3,mm3

		movq		mm5,[QNegAlpha]

		pmullw		mm7,mm3
		psubw		mm5,mm3

		;regrab z
		movq		mm2,[QZVal32_0]
		movq		mm3,[edi+60]

		movq		mm4,[QZVal32_1]
		punpcklbw	mm3,[Zero]

		paddd		mm2,[QZDelta]
		pmullw		mm3,mm5

		paddd		mm4,[QZDelta]
		paddw		mm3,mm7

		movq		[QZVal32_0],mm2
		psrlw		mm3,8

		movq		[QZVal32_1],mm4
		packuswb	mm3,mm3

		movd		[edi+60],mm3

		; get corrected right side deltas ; ULw|VLw DU16|DV16 argb|ARGBagAG|rbRB    |ZRi   aA|rR   UZR|VZR aArR|gGbB

		movq		mm4,[ZIR]
		movq		mm6,[UVZ]

		movq		mm2,[UVR]
		movq		mm0,[UVLeftW]		; ULw|VLw DU16|DV16 argb|ARGBagAG|rbRB    |ZRi   aA|rR   UZR|VZR aArR|gGbB

		movq		mm1,[UVLeft]		; ULw|VLw   UL|VL   argb|ARGBagAG|rbRB ZRi|ZRi   aA|rR   UZR|VZR aArR|gGbB
		punpckldq	mm2,mm2				; ULw|VLw   UL|VL    ZRi|ZRi agAG|rbRB ZRi|ZRi   aA|rR   UZR|VZR aArR|gGbB

		pfmul		mm2,mm6				; ULw|VLw   UL|VL     UR|VR  agAG|rbRB ZRi|ZRi   aA|rR   UZR|VZR aArR|gGbB
		mov			eax,offset QZCan
		add			dword ptr[Dest],32

		add			dword ptr[eax+4],32
		mov			eax,offset QDibCan

		add			dword ptr[eax+4],64

		movq		mm3,mm2
		pfmul		mm3,[QFixedScale]

		pf2id		mm3,mm3

		dec		[NumASpans]			; dec num affine spans
		jnz		SpanLoopLit

HandleLeftoverPixelsLit:


		mov		esi,GBitPtr


		cmp			[RemainingCount],0
		jz			ReturnLit

		mov			eax,[RemainingCount]
		mov			dword ptr[ZIR],eax
		mov			dword ptr[ZIR+4],eax
		
		movq		mm7,[GLMapMulUV]	;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR   LU|LV
		movq		mm5,mm1

		pfsub		mm6,[UVDivZ16StepX]
		pfmul		mm7,mm1				;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR  ULL|VLL

		pfmul		mm5,[QFixedScale]
		pi2fd		mm3,[ZIR]

		pfmul		mm7,[QFixedScale]
		pfmul		mm3,[UVDivZStepX]

		pf2id		mm5,mm5
		pfadd		mm3,mm6

		pi2fd		mm2,[ZIR]
		movd		mm6,[Zi16StepX]

		pf2id		mm7,mm7				; ULw|VLw   DU|DV    UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |
		pfsub		mm4,mm6

		movd		mm6,[ZiStepX]
		mov			ebx,[RemainingCount]

		pfmul		mm2,mm6

		shl			ebx,3

		pfadd		mm2,mm4
		paddd		mm5,[UVAdjust]

		punpckldq	mm4,mm4

		movq		[UVL16],mm7
		pfrcp		mm2,mm2

		pfmul		mm4,[QZBufferPrec]

		paddd		mm5,[UVAdjust2]
		pfmul		mm2,mm3

		movq		[UVLeft],mm5
		pfsubr		mm1,mm2				; ULw|VLw   DU|DV    UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |

		pfmul		mm1,qword ptr[QFixedScaleLUT+ebx]	; ULw|VLw DU16|DV16  UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |

		movq		mm0,mm5
		pand		mm5,[WrapMask]
		pf2id		mm1,mm1				; ULw|VLw DU16|DV16  UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |

		movq		[UV16],mm5
		movq		mm7,[UVAdjustL]

		psrlq		mm5,[QShiftV]
		paddd		mm7,[UVL16]

		movq		[UV16V],mm5
		movq		[UVL16],mm7


OnePixelSpanLit:
		; Clamp U/V
		mov		ebx,dword ptr[UVL16+4]
		cmp		ebx,MaxU
		jle		TryClampU1Lit
		mov		ecx,MaxU
		mov		dword ptr[UVL16+4],ecx
		jmp		NoClampU1Lit

TryClampU1Lit:
		cmp		ebx,0
		jge		NoClampU1Lit
		mov		dword ptr[UVL16+4],0
NoClampU1Lit:
		mov		eax,dword ptr[UVL16]
		cmp		eax,MaxV
		jle		TryClampV1Lit
		mov		ecx,MaxV
		mov		dword ptr[UVL16],ecx
		jmp		NoClampV1Lit

TryClampV1Lit:
		cmp		eax,0
		jge		NoClampV1Lit
		mov		dword ptr[UVL16],0

NoClampV1Lit:
		mov			ebx,dword ptr[UV16+4]
		mov			eax,dword ptr[UV16V]

		mov			esi,GBitPtr
		shr			ebx,16
		add			esi,eax
		add			esi,ebx
		mov			ecx,TexPal
		xor			eax,eax
		movq		mm6,[ARL]

		mov			eax,offset QDibCan
		push		ebp

		mov			edi,dword ptr[eax+4]
		mov			ebp,dword ptr[Dest]

		movq		[UVZ],mm1			;using this for a step temp

		movd		mm3,dword ptr[ZiStepX]
		xor			eax,eax

LeftoverLoopLit:
		paddd		mm0,[UVZ]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |
		pf2id		mm2,mm4

		mov			al,byte ptr[esi]
		movq		mm5,mm0				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw   UL|VL      |ZY   UZR|VZR     |

		mov			esi,GBitPtr
		pfadd		mm4,mm3

		pand		mm5,[WrapMask]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |
		psrld		mm2,16

		movq		[UV16],mm5			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |
		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |ARGB

		psrlq		mm5,[QShiftV]
		movd		eax,mm2

		movq		[UV16V],mm5
		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB

		xor			eax,0
		movq		mm5,mm7

		mov			ebx,dword ptr[UV16+4]
		psrlq		mm5,48

		shr			ebx,16
		movq		mm1,mm5

		add			esi,ebx
		psllq		mm1,16

		mov			edx,dword ptr[UV16V]
		por			mm5,mm1

		pmullw		mm7,mm6
		punpckldq	mm5,mm5

		psrlw		mm7,8
		movq		mm1,[QNegAlpha]

		pmullw		mm7,mm5
		psubw		mm1,mm5

		movq		mm5,[edi]
		paddw		mm6,[RGBADelta]

		punpcklbw	mm5,[Zero]
		add			esi,edx

		pmullw		mm5,mm1
		cmp			ax,word ptr[ebp]
		jl			SkipPixelLitZ

		paddw		mm5,mm7
		psrlw		mm5,8
		packuswb	mm5,mm5

		movd		[edi],mm5

SkipPixelLitZ:

		xor			eax,eax
		add			edi,4
		add			ebp,2

		dec		[RemainingCount]
		jge		LeftoverLoopLit

		pop			ebp

ReturnLit:

	pop		edi
	pop		esi
	pop		ecx
	pop		ebx

	cRet DrawSpan32_AsmGouraudZBufferAlphaARGB3DNow
endProc DrawSpan32_AsmGouraudZBufferAlphaARGB3DNow



; non interpolated alpha at the verts

cProc DrawSpan32_AsmGouraudZBufferVertexAlpha3DNow, 36,<x1 : dword, x2 : dword, y : dword, r1 : dword, g1 : dword, b1 : dword, r2 : dword, g2 : dword, b2 : dword>

		push	ebx
		push	ecx
		push	esi
		push	edi

		mov		eax,x1
		mov		ecx,x2
		dec		ecx
		sub		ecx,eax
		jle		ReturnLit

;		femms

;		inc		ecx
		mov		ebx,offset ClientWindow
		mov		eax, y
		mov		edi,[ebx].Buffer
		mov		esi,x1
		imul	eax, [ebx].PixelPitch
		shl		esi,2
		mov		edx,ZBuffer
		add		eax,esi
		add		edi, eax
		shr		eax,1
		add		edx,eax
		mov		[Dest],edx
		mov		eax,offset QZCan
		mov		ebx,offset SCan
		mov		[eax],ebx
		mov		[eax+4],edx
		mov		eax,offset QDibCan
		mov		[eax],ebx
		mov		[eax+4],edi
		mov		ebx,offset QDibCan
		mov		eax,offset QDibOrCan
		mov		[eax],ebx
		mov		[eax+4],ebx
		mov		ebx,offset QZCan
		mov		eax,offset QZOrCan
		mov		[eax],ebx
		mov		[eax+4],ebx
		mov		edx,ecx
		mov		eax,ecx

		shr		ecx,4
		and		eax,15
		jnz		@f
		dec		ecx
		mov		eax,16
@@:

		mov		[NumASpans],ecx
		mov		[RemainingCount],eax

		;grab the left side lights
		movd		mm5,r1
		movd		mm4,b1

;		punpckldq	mm5,qword ptr[Zero]
		movd		mm6,g1

		pf2id		mm5,mm5
		punpckldq	mm4,mm6

		pf2id		mm4,mm4
		packssdw	mm4,mm5

;		psllw		mm4,7

		movq		[ARL],mm4

										;   mm0      mm1      mm2      mm3      mm4      mm5      mm6      mm7
		movd		mm0,x1				;    |x       |        |        |        |        |        |        |
		movq		mm2,[UVDivZStepX]	;    |x       |    UZdX|VZdX    |        |        |        |        |

		movd		mm1,y				;    |x       |y   UZdX|VZdX    |        |        |        |        |
		movq		mm3,[UVDivZStepY]	;    |x       |y   UZdX|VZdXUZdY|VZdY    |        |        |        |

		punpckldq	mm0,mm0				;   x|x       |y   UZdX|VZdXUZdY|VZdY    |        |        |        |
		punpckldq	mm1,mm1				;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |        |        |        |

		movd		mm7,edx				;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |        |        |        |wid
		movd		mm5,b2				;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |        |b       |        |wid

		pi2fd		mm0,mm0				;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |        |b       |        |wid
		movd		mm6,b1				;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |        |b       |b       |wid

		pi2fd		mm7,mm7				;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |        |b       |b       |wid
		punpckldq	mm5,qword ptr[g2]	;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |       g|b       |b       |wid

		pi2fd		mm1,mm1				;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |       g|b       |b       |wid
		punpckldq	mm6,qword ptr[g1]	;   x|x      y|y   UZdX|VZdXUZdY|VZdY    |       g|b      g|b       |wid

		pfmul		mm2,mm0				;   x|x      y|y    UZX|VZX UZdY|VZdY    |       g|b      g|b       |wid
		pfrcp		mm7,mm7				;   x|x      y|y    UZX|VZX UZdY|VZdY    |       g|b      g|b     dw|dw

		pfsub		mm5,mm6				;   x|x      y|y    UZX|VZX UZdY|VZdY    |      gd|bd     g|b     dw|dw
		movd		mm4,[r1]			;   x|x      y|y    UZX|VZX UZdY|VZdY    |r     gd|bd     g|b     dw|dw

		movd		mm6,[r2]			;   x|x      y|y    UZX|VZX UZdY|VZdY    |r     gd|bd     g|r     dw|dw
		pfmul		mm3,mm1				;   x|x      y|y    UZX|VZX  UZY|VZY     |r     gd|bd     g|r     dw|dw

		pfsub		mm6,mm4				;   x|x      y|y    UZX|VZX  UZY|VZY     |r     gd|bd     x|rd    dw|dw
		pfmul		mm7,[Q128]			;   x|x      y|y    UZX|VZX  UZY|VZY     |r     gd|bd     x|rd    DW|DW

		movd		mm4,[ZiStepX]		;   x|x      y|y    UZX|VZX  UZY|VZY     |ZdX   gd|bd     x|rd    DW|DW

		punpckldq	mm4,mm4
		pfmul		mm4,[QZBufferPrec]
		pf2id		mm4,mm4
		movq		[QZDelta],mm4

		movd		mm4,[ZiStepX]		;   x|x      y|y    UZX|VZX UZdY|VZdY    |ZdX     |        |        |
		pfmul		mm5,mm7				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZdX   GD|BD     x|rd    DW|DW

		pfmul		mm4,mm0				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX    GD|BD     x|rd    DW|DW
		pfmul		mm6,mm7				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX    GD|BD     x|RD    DW|DW

		pf2id		mm5,mm5
		pf2id		mm6,mm6

		packssdw	mm5,mm6
		movq		[RGBADelta],mm5

		movd		mm5,[ZiStepY]		;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX      |ZdY    x|RD    DW|DW
		movq		mm6,[UVDivZOrigin]	;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX      |ZdY  UZO|VZO   DW|DW

		pfmul		mm5,mm1				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX      |ZY   UZO|VZO   DW|DW
		movd		mm7,[ZiOrigin]		;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX      |ZY   UZO|VZO     |ZO

		pfadd		mm6,mm2				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZX      |ZY  UZXS|VZXS    |ZO
		pfadd		mm4,mm7				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZXS     |ZY  UZXS|VZXS    |ZO

		pfadd		mm6,mm3				;   x|x      y|y    UZX|VZX  UZY|VZY     |ZXS     |ZY    UZ|VZ      |ZO
		pfadd		mm4,mm5				;   x|x      y|y    UZX|VZX  UZY|VZY     |Zi      |ZY    UZ|VZ      |ZO

		movq		mm7,mm4
		movq		mm1,[QZDelta]
		punpckldq	mm7,mm7
		pfmul		mm7,[QZBufferPrec]
		pf2id		mm7,mm7
		movq		mm0,mm7
		paddd		mm7,mm1
		punpckldq	mm0,mm7
		paddd		mm7,mm1
		movq		[QZVal32_0],mm0
		movq		mm0,mm7
		paddd		mm7,mm1
		punpckldq	mm0,mm7
		movq		[QZVal32_1],mm0
		pslld		mm1,2
		movd		mm7,[Zi16StepX]
		movq		[QZDelta],mm1

		pfrcp		mm0,mm4				;  ZL|ZL     y|y    UZX|VZX  UZY|VZY     |Zi      |ZY    UZ|VZ      |ZdX16

		movq		mm1,mm6				;  ZL|ZL    UZ|VZ   UZX|VZX  UZY|VZY     |Zi      |ZY    UZ|VZ      |ZdX16
		pfadd		mm4,mm7				;  ZL|ZL    UZ|VZ   UZX|VZX  UZY|VZY     |ZRi     |ZY    UZ|VZ      |ZdX16

		pfmul		mm1,mm0				;  ZL|ZL    UL|VL   UZX|VZX  UZY|VZY     |ZRi     |ZY    UZ|VZ      |ZdX16
		pfrcp		mm2,mm4				;  ZL|ZL    UL|VL    ZR|ZR   UZY|VZY     |ZRi     |ZY    UZ|VZ      |ZdX16

		pfadd		mm6,[UVDivZ16StepX] ;  ZL|ZL    UL|VL    ZR|ZR   UZY|VZY     |ZRi     |ZY   UZR|VZR     |ZdX16
		pfmul		mm2,mm6				;  ZL|ZL    UL|VL    UR|VR   UZY|VZY     |ZRi     |ZY   UZR|VZR     |ZdX16

		movq		mm0,mm1				;  UL|VL    UL|VL    UR|VR   UZY|VZY     |ZRi     |ZY   UZR|VZR     |ZdX16
		movq		mm3,mm2				; ULi|VLi	UL|VL    UR|UR    UR|UR      |ZRi     |ZY   UZR|VZR     |ZdX16

		pfmul		mm0,[QFixedScale]	; UL6|VL6	UL|VL    UR|UR    UR|UR      |ZRi     |ZY   UZR|VZR     |ZdX16
		pfmul		mm3,[QFixedScale]	; UL6|VL6	UL|VL   UR6|UR6  UR6|UR6     |ZRi     |ZY   UZR|VZR     |ZdX16

		pf2id		mm0,mm0				;UL6i|VL6i  UL|VL   UR6|VR6  UR6|VR6     |ZRi     |ZY   UZR|VZR     |ZdX16
		pf2id		mm3,mm3				;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR     |ZdX16


		test	ecx,ecx
		jz		HandleLeftoverPixelsLit

		movq		mm7,[GLMapMulUV]	;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR   LU|LV
		pfmul		mm7,mm1				;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR  ULL|VLL
		pfmul		mm7,[QFixedScale]
		pf2id		mm7,mm7				; ULw|VLw   DU|DV    UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |
		paddd		mm7,[UVAdjustL]
		movq		[UVL16],mm7

		; Clamp U/V
		mov		ebx,dword ptr[UVL16+4]

		cmp		ebx,MaxU
		jle		TryClampU0Litp
		mov		ecx,MaxU
		mov		dword ptr[UVL16+4],ecx
		jmp		NoClampU0Litp

TryClampU0Litp:
		cmp		ebx,0
		jge		NoClampU0Litp
		mov		dword ptr[UVL16+4],0
NoClampU0Litp:
		mov		eax,dword ptr[UVL16]
		cmp		eax,MaxV
		jle		TryClampV0Litp
		mov		ecx,MaxV
		mov		dword ptr[UVL16],ecx
		jmp		NoClampV0Litp

TryClampV0Litp:
		cmp		eax,0
		jge		NoClampV0Litp
		mov		dword ptr[UVL16],0

NoClampV0Litp:

		pfrcp		mm5,mm4

SpanLoopLit:
		;use float uv for lightmap uv
		movq		mm7,[GLMapMulUV]	;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR   LU|LV
		movq		mm5,mm1

		pfmul		mm7,mm2				;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR  ULL|VLL
		pfmul		mm5,[QFixedScale]

		pfmul		mm7,[QFixedScale]
		pf2id		mm5,mm5

		pf2id		mm7,mm7				; ULw|VLw   DU|DV    UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |
		paddd		mm5,[UVAdjust]

		pfsubr		mm1,mm2				; ULw|VLw   DU|DV    UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |
		paddd		mm5,[UVAdjust2]

		movq		[UVL16],mm7
		movq		[UVLeft],mm5

		movd		mm7,[Zi16StepX]
		movq		mm0,mm5

		pfmul		mm1,[QFixedScale16]	; ULw|VLw DU16|DV16  UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |
		;right side stuff becomes left	; ULw|VLw DU16|DV16  UL|VL   URw|VRw     |ZLi     |ZY   UZL|VZL     |
		pfadd		mm4,mm7				;ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZL|VZL     |

		pand		mm5,[WrapMask]
		pf2id		mm1,mm1				; ULw|VLw DU16|DV16  UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |

		movq		mm7,[UVAdjustL]
		movq		[UV16],mm5

		pfadd		mm6,[UVDivZ16StepX]	; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |
		paddd		mm7,[UVL16]

		psrlq		mm5,[QShiftV]
		movq		[UVL16],mm7

		movq		[UV16V],mm5


		; Clamp U/V
		mov		ebx,dword ptr[UVL16+4]
		cmp		ebx,MaxU
		jle		TryClampU0Lit
		mov		ecx,MaxU
		mov		dword ptr[UVL16+4],ecx
		jmp		NoClampU0Lit

TryClampU0Lit:
		cmp		ebx,0
		jge		NoClampU0Lit
		mov		dword ptr[UVL16+4],0
NoClampU0Lit:
		mov		eax,dword ptr[UVL16]
		cmp		eax,MaxV
		jle		TryClampV0Lit
		mov		ecx,MaxV
		mov		dword ptr[UVL16],ecx
		jmp		NoClampV0Lit

TryClampV0Lit:
		cmp		eax,0
		jge		NoClampV0Lit
		mov		dword ptr[UVL16],0

NoClampV0Lit:
		movq		[UVLeftW],mm3
		mov			eax,dword ptr[UV16V]
		mov			ebx,dword ptr[UV16+4]
		movq		[ZIR],mm4
		mov			esi,GBitPtr
		shr			ebx,16
		movq		[UVZ],mm6
		add			esi,eax
		add			esi,ebx
		movq		[UVLeft],mm2

		mov			ecx,TexPal
		pfrcp		mm5,mm4
;		movq		mm3,[RGBADelta]
		xor			eax,eax
		movq		mm6,[ARL]

		push		ebp
		mov			ebp,[Dest]

		;grab zbuffer values
		movq		mm2,[QZVal32_0]
		movq		mm7,[QZVal32_1]

		psrld		mm2,17
		psrld		mm7,17

		packssdw	mm2,mm7

		pslld		mm2,1
		movq		[QZVal],mm2


		movq		mm2,[QZVal]
		paddd		mm0,mm1				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |

		mov			al,byte ptr[esi]
		movq		mm4,mm0				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw   UL|VL      |ZY   UZR|VZR     |

		pcmpgtw		mm2,[ebp]
		mov			esi,GBitPtr

		psrlw		mm2,15
		pand		mm4,[WrapMask]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		psllw		mm2,2
		movq		[UV16],mm4			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |

		movq		[GBL],mm2
		psrlq		mm4,[QShiftV]

		movq		mm7,mm2
		movq		[UV16V],mm4

		punpcklwd	mm7,[Zero]
		movq		[UVR],mm5

		paddd		mm7,[QZOrCan]
		mov			edx,dword ptr[UV16V]

		movq		[QZOut],mm7
		mov			ebx,dword ptr[UV16+4]

		movq		mm7,mm2
		mov			edi,dword ptr[QZOut]

		punpcklwd	mm7,[Zero]
		shr			ebx,16

		paddd		mm7,[QDibOrCan]
		mov			edi,dword ptr[edi]

		movq		[QDibOut],mm7
		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |ARGB

		add			esi,edx
		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB

		xor			eax,eax

		pmullw		mm7,mm6
		mov			ax,word ptr[QZVal]

		paddd		mm0,mm1

		mov			word ptr[edi],ax

		mov			edi,dword ptr[QDibOut]

		add			esi,ebx

		mov			edi,dword ptr[edi]
		paddw		mm6,[RGBADelta]

		psrlw		mm7,8

		xor			eax,eax
		movq		mm5,[QNegAlpha]

		pmullw		mm7,qword ptr[VertAlpha]
		psubw		mm5,qword ptr[VertAlpha]

		movq		mm3,[edi]
		mov			al,byte ptr[esi]

		punpcklbw	mm3,[Zero]
		movq		mm4,mm0

		pmullw		mm3,mm5
		pand		mm4,[WrapMask]

		paddw		mm3,mm7
		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB

		psrlw		mm3,8
		movq		[UV16],mm4

		packuswb	mm3,mm3
		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB

		movd		[edi],mm3

		psrlq		mm4,[QShiftV]

		mov			ebx,dword ptr[UV16+4]

		movq		[UV16V],mm4

		pmullw		mm7,mm6

		mov			edx,dword ptr[UV16V]

		mov			edi,dword ptr[QZOut+4]
		psrlw		mm7,8

		xor			eax,eax
		mov			edi,dword ptr[edi]

		mov			ax,word ptr[QZVal+2]
		mov			esi,GBitPtr

		movq		mm5,[QNegAlpha]
		pmullw		mm7,qword ptr[VertAlpha]

		mov			word ptr[edi+2],ax
		xor			eax,eax

		mov			edi,dword ptr[QDibOut+4]
		shr			ebx,16

		mov			edi,dword ptr[edi]
		psubw		mm5,qword ptr[VertAlpha]

		paddw		mm6,[RGBADelta]
		add			esi,ebx

		movq		mm3,[edi+4]
		add			esi,edx

		punpcklbw	mm3,[Zero]
		mov			al,byte ptr[esi]

		pmullw		mm3,mm5
		paddd		mm0,mm1

		paddw		mm3,mm7
		packuswb	mm7,mm7				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw ARGB|ARGB UZR|VZR   AR|GB

		psrlw		mm3,8
		movq		mm4,mm0

		packuswb	mm3,mm3
		pand		mm4,[WrapMask]

		mov			esi,GBitPtr
		movd		[edi+4],mm3

		movq		mm7,mm2
		movq		[UV16],mm4

		punpckhwd	mm7,[Zero]
		psrlq		mm4,[QShiftV]

		paddd		mm7,[QZorCan]
		movq		[UV16V],mm4

		movq		[QZOut],mm7
		mov			ebx,dword ptr[UV16+4]

		movq		mm7,mm2
		mov			edi,dword ptr[QZOut]

		punpckhwd	mm7,[Zero]
		mov			edi,dword ptr[edi]

		paddd		mm7,[QDibOrCan]
		paddd		mm0,mm1

		movq		[QDibOut],mm7
		mov			edx,dword ptr[UV16V]

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB
		xor			eax,0

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		mov			ax,word ptr[QZVal+4]

		pmullw		mm7,mm6

		mov			word ptr[edi+4],ax

		psrlw		mm7,8

		mov			edi,dword ptr[QDibOut]

		paddw		mm6,[RGBADelta]

		mov			edi,dword ptr[edi]

		movq		mm5,[QNegAlpha]

		pmullw		mm7,qword ptr[VertAlpha]
		psubw		mm5,qword ptr[VertAlpha]

		movq		mm3,[edi+8]
		movq		mm4,mm0

		punpcklbw	mm3,[Zero]
		shr			ebx,16

		pmullw		mm3,mm5
		xor			eax,eax

		paddw		mm3,mm7
		pand		mm4,[WrapMask]

		psrlw		mm3,8
		add			esi,edx

		packuswb	mm3,mm3
		paddd		mm0,mm1

		movd		[edi+8],mm3
		add			esi,ebx

		movq		[UV16],mm4
		mov			al,byte ptr[esi]

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		psrlq		mm4,[QShiftV]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		movq		[UV16V],mm4

		mov			ebx,dword ptr[UV16+4]

		mov			edx,dword ptr[UV16V]

		pmullw		mm7,mm6

		shr			ebx,16

		xor			eax,eax

		paddw		mm6,[RGBADelta]

		mov			edi,dword ptr[QZOut+4]
		psrlw		mm7,8

		movq		mm5,[QNegAlpha]
		mov			esi,edx

		pmullw		mm7,qword ptr[VertAlpha]
		add			esi,GBitPtr

		mov			edi,dword ptr[edi]
		psubw		mm5,qword ptr[VertAlpha]

		mov			ax,word ptr[QZVal+6]
		add			esi,ebx

		mov			word ptr[edi+6],ax
		mov			edi,dword ptr[QDibOut+4]

		xor			eax,eax
		mov			edi,dword ptr[edi]

		mov			al,byte ptr[esi]
		movq		mm3,[edi+12]

		movq		mm4,mm0
		punpcklbw	mm3,[Zero]

		;regrab z
		movq		mm2,[QZVal32_0]
		pmullw		mm3,mm5

		paddd		mm2,[QZDelta]
		paddw		mm3,mm7

		movq		mm7,[QZVal32_1]
		psrlw		mm3,8

		paddd		mm7,[QZDelta]
		packuswb	mm3,mm3

		movq		[QZVal32_0],mm2
		movq		[QZVal32_1],mm7

		movd		[edi+12],mm3

		psrld		mm2,17
		psrld		mm7,17

		packssdw	mm2,mm7
		pand		mm4,[WrapMask]

		pslld		mm2,1
		movq		[UV16],mm4

		movq		[QZVal],mm2
		mov			esi,GBitPtr

		pcmpgtw		mm2,[ebp+8]
		psrlq		mm4,[QShiftV]


		psrlw		mm2,15
		movq		[UV16V],mm4


		psllw		mm2,2
		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB

		movq		mm4,mm2
		mov			ebx,dword ptr[UV16+4]

		punpcklwd	mm4,[Zero]
		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB


		paddd		mm4,[QZorCan]

		pmullw		mm7,mm6

		movq		[QZOut],mm4

		xor			eax,eax

		mov			edi,dword ptr[QZOut]
		movq		mm4,mm2

		mov			edi,dword ptr[edi]

		mov			ax,word ptr[QZVal]

		mov			word ptr[edi+8],ax
		psrlw		mm7,8

		paddw		mm6,[RGBADelta]
		punpcklwd	mm4,[Zero]

		paddd		mm4,[QDibOrCan]
		pmullw		mm7,qword ptr[VertAlpha]

		movq		mm5,[QNegAlpha]
		paddd		mm0,mm1

		movq		[QDibOut],mm4
		psubw		mm5,qword ptr[VertAlpha]

		mov			edi,dword ptr[QDibOut]
		mov			edx,dword ptr[UV16V]

		mov			edi,dword ptr[edi]
		movq		mm4,mm0

		movq		mm3,[edi+16]
		shr			ebx,16

		punpcklbw	mm3,[Zero]
		xor			eax,eax

		pand		mm4,[WrapMask]
		pmullw		mm3,mm5

		add			esi,edx
		paddw		mm3,mm7

		paddd		mm0,mm1
		psrlw		mm3,8

		add			esi,ebx
		packuswb	mm3,mm3

		mov			al,byte ptr[esi]
		movq		[UV16],mm4

		movd		[edi+16],mm3

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		psrlq		mm4,[QShiftV]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		movq		[UV16V],mm4

		mov			ebx,dword ptr[UV16+4]

		mov			edx,dword ptr[UV16V]

		mov			edi,dword ptr[QZOut+4]

		pmullw		mm7,mm6
		mov			edi,dword ptr[edi]

		mov			ax,word ptr[QZVal+2]

		shr			ebx,16
		mov			word ptr[edi+10],ax

		xor			eax,eax

		mov			edi,dword ptr[QDibOut+4]
		psrlw		mm7,8

		mov			esi,GBitPtr
		mov			edi,dword ptr[edi]

		movq		mm5,[QNegAlpha]
		pmullw		mm7,qword ptr[VertAlpha]

		psubw		mm5,qword ptr[VertAlpha]
		movq		mm3,[edi+20]

		punpcklbw	mm3,[Zero]
		paddw		mm6,[RGBADelta]

		pmullw		mm3,mm5
		add			esi,edx

		paddw		mm3,mm7
		xor			eax,eax

		psrlw		mm3,8
		movq		mm4,mm0

		packuswb	mm3,mm3

		pand		mm4,[WrapMask]
		movd		[edi+20],mm3

		movq		[UV16],mm4

		add			esi,ebx
		psrlq		mm4,[QShiftV]

		movq		[UV16V],mm4

		mov			al,byte ptr[esi]


		mov			esi,GBitPtr

		movq		mm4,mm2
		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB

		punpckhwd	mm4,[Zero]

		mov			ebx,dword ptr[UV16+4]
		paddd		mm4,[QZorCan]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		movq		[QZOut],mm4

		movq		mm4,mm2

		mov			edi,dword ptr[QZOut]
		punpckhwd	mm4,[Zero]

		mov			edi,dword ptr[edi]
		mov			ax,word ptr[QZVal+4]

		mov			word ptr[edi+12],ax
		pmullw		mm7,mm6

		xor			eax,eax

		paddd		mm4,[QDibOrCan]

		paddw		mm6,[RGBADelta]
		movq		[QDibOut],mm4

		psrlw		mm7,8

		mov			edi,dword ptr[QDibOut]
		movq		mm5,[QNegAlpha]

		pmullw		mm7,qword ptr[VertAlpha]
		mov			edi,dword ptr[edi]

		psubw		mm5,qword ptr[VertAlpha]
		movq		mm3,[edi+24]

		paddd		mm0,mm1
		punpcklbw	mm3,[Zero]

		pmullw		mm3,mm5
		mov			edx,dword ptr[UV16V]

		paddw		mm3,mm7
		movq		mm4,mm0

		psrlw		mm3,8
		shr			ebx,16

		packuswb	mm3,mm3
		xor			eax,eax

		pand		mm4,[WrapMask]
		movd		[edi+24],mm3

		add			esi,edx
		mov			edi,dword ptr[QZOut+4]

		paddd		mm0,mm1
		mov			edi,dword ptr[edi]
		add			esi,ebx

		mov			al,byte ptr[esi]
		movq		[UV16],mm4

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		psrlq		mm4,[QShiftV]

		xor			eax,eax

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		movq		[UV16V],mm4

		mov			ax,word ptr[QZVal+6]

		mov			ebx,dword ptr[UV16+4]
		mov			word ptr[edi+14],ax

		mov			edx,dword ptr[UV16V]

		mov			edi,dword ptr[QDibOut+4]
		pmullw		mm7,mm6

		shr			ebx,16

		psrlw		mm7,8

		paddw		mm6,[RGBADelta]

		movq		mm5,[QNegAlpha]
		pmullw		mm7,qword ptr[VertAlpha]

		mov			edi,dword ptr[edi]
		psubw		mm5,qword ptr[VertAlpha]



		movq		mm3,[edi+28]
		mov			esi,GBitPtr

		punpcklbw	mm3,[Zero]
		add			esi,edx

		pmullw		mm3,mm5
		xor			eax,eax

		paddw		mm3,mm7
		add			esi,ebx

		;regrab z
		movq		mm2,[QZVal32_0]
		psrlw		mm3,8

		movq		mm4,[QZVal32_1]
		packuswb	mm3,mm3

		paddd		mm2,[QZDelta]
		paddd		mm4,[QZDelta]

		movq		[QZVal32_0],mm2
		movq		[QZVal32_1],mm4

		psrld		mm2,17
		psrld		mm4,17

		packssdw	mm2,mm4
		xor			eax,eax

		pslld		mm2,1


		movq		[QZVal],mm2

		mov			al,byte ptr[esi]
		movq		mm4,mm0

		pand		mm4,[WrapMask]
		pcmpgtw		mm2,[ebp+16]

		movd		[edi+28],mm3
		movq		[UV16],mm4

		psrlw		mm2,15
		mov			esi,GBitPtr

		psrlq		mm4,[QShiftV]

		psllw		mm2,2
		movq		[UV16V],mm4
		movq		mm4,mm2

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB
		punpcklwd	mm4,[Zero]

		mov			ebx,dword ptr[UV16+4]
		paddd		mm4,[QZorCan]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		movq		[QZOut],mm4

		mov			edi,dword ptr[QZOut]

		movq		mm4,mm2
		mov			edi,dword ptr[edi]

		pmullw		mm7,mm6

		xor			eax,eax

		mov			ax,word ptr[QZVal]

		punpcklwd	mm4,[Zero]
		mov			word ptr[edi+16],ax

		paddw		mm6,[RGBADelta]

		paddd		mm4,[QDibOrCan]
		psrlw		mm7,8

		movq		[QDibOut],mm4
		movq		mm5,[QNegAlpha]

		mov			edi,dword ptr[QDibOut]
		pmullw		mm7,qword ptr[VertAlpha]

		mov			edi,dword ptr[edi]
		psubw		mm5,qword ptr[VertAlpha]
		mov			edx,dword ptr[UV16V]

		movq		mm3,[edi+32]
		paddd		mm0,mm1

		punpcklbw	mm3,[Zero]
		shr			ebx,16

		pmullw		mm3,mm5
		xor			eax,eax

		paddw		mm3,mm7
		add			esi,edx

		psrlw		mm3,8
		add			esi,ebx

		movq		mm4,mm0
		packuswb	mm3,mm3

		pand		mm4,[WrapMask]
		movd		[edi+32],mm3

		paddd		mm0,mm1
		mov			edi,dword ptr[QZOut+4]

		mov			al,byte ptr[esi]
		movq		[UV16],mm4

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		psrlq		mm4,[QShiftV]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		movq		[UV16V],mm4

		mov			ebx,dword ptr[UV16+4]

		mov			edx,dword ptr[UV16V]

		mov			edi,dword ptr[edi]

		pmullw		mm7,mm6
		mov			ax,word ptr[QZVal+2]

		mov			word ptr[edi+18],ax

		psrlw		mm7,8
		mov			edi,dword ptr[QDibOut+4]

		paddw		mm6,[RGBADelta]

		mov			edi,dword ptr[edi]

		shr			ebx,16
		movq		mm5,[QNegAlpha]

		pmullw		mm7,qword ptr[VertAlpha]
		psubw		mm5,qword ptr[VertAlpha]

		movq		mm3,[edi+36]
		mov			esi,GBitPtr

		punpcklbw	mm3,[Zero]
		add			esi,edx

		pmullw		mm3,mm5
		xor			eax,eax

		paddw		mm3,mm7
		movq		mm4,mm0

		psrlw		mm3,8
		pand		mm4,[WrapMask]

		packuswb	mm3,mm3
		movq		[UV16],mm4

		add			esi,ebx
		psrlq		mm4,[QShiftV]

		xor			eax,eax
		movq		[UV16V],mm4

		mov			al,byte ptr[esi]

		mov			esi,GBitPtr
		movd		[edi+36],mm3

		movq		mm4,mm2

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB

		punpckhwd	mm4,[Zero]

		mov			ebx,dword ptr[UV16+4]
		paddd		mm4,[QZorCan]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		movq		[QZOut],mm4

		movq		mm4,mm2

		mov			edi,dword ptr[QZOut]

		mov			ax,word ptr[QZVal+4]
		pmullw		mm7,mm6

		mov			edi,dword ptr[edi]

		punpckhwd	mm4,[Zero]
		mov			word ptr[edi+20],ax
		xor			eax,eax

		paddd		mm4,[QDibOrCan]

		movq		[QDibOut],mm4

		mov			edi,dword ptr[QDibOut]
		paddw		mm6,[RGBADelta]


		mov			edi,dword ptr[edi]
		psrlw		mm7,8

		movq		mm5,[QNegAlpha]
		pmullw		mm7,qword ptr[VertAlpha]

		mov			edx,dword ptr[UV16V]
		psubw		mm5,qword ptr[VertAlpha]

		movq		mm3,[edi+40]
		paddd		mm0,mm1

		punpcklbw	mm3,[Zero]
		movq		mm4,mm0

		pmullw		mm3,mm5
		shr			ebx,16

		paddw		mm3,mm7
		add			esi,edx

		psrlw		mm3,8
		add			esi,ebx

		packuswb	mm3,mm3
		xor			eax,eax

		movd		[edi+40],mm3
		paddd		mm0,mm1

		pand		mm4,[WrapMask]
		mov			al,byte ptr[esi]

		movq		[UV16],mm4

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		psrlq		mm4,[QShiftV]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB

		movq		[UV16V],mm4

		mov			ebx,dword ptr[UV16+4]

		mov			edx,dword ptr[UV16V]

		mov			edi,dword ptr[QZOut+4]
		pmullw		mm7,mm6

		mov			edi,dword ptr[edi]

		mov			ax,word ptr[QZVal+6]
		psrlw		mm7,8

		mov			word ptr[edi+22],ax

		mov			edi,dword ptr[QDibOut+4]
		shr			ebx,16

		paddw		mm6,[RGBADelta]

		movq		mm5,[QNegAlpha]
		mov			edi,dword ptr[edi]

		pmullw		mm7,qword ptr[VertAlpha]
		psubw		mm5,qword ptr[VertAlpha]

		movq		mm3,[edi+44]
		mov			esi,GBitPtr

		punpcklbw	mm3,[Zero]
		add			esi,ebx

		pmullw		mm3,mm5
		add			esi,edx

		paddw		mm3,mm7
		xor			eax,eax

		;regrab z
		movq		mm2,[QZVal32_0]
		psrlw		mm3,8

		movq		mm4,[QZVal32_1]
		packuswb	mm3,mm3

		paddd		mm2,[QZDelta]
		paddd		mm4,[QZDelta]

		movq		[QZVal32_0],mm2
		movq		[QZVal32_1],mm4

		psrld		mm2,17
		psrld		mm4,17

		packssdw	mm2,mm4
		xor			eax,eax

		pslld		mm2,1


		movq		[QZVal],mm2

		mov			al,byte ptr[esi]
		movq		mm4,mm0

		pand		mm4,[WrapMask]
		pcmpgtw		mm2,[ebp+24]

		movd		[edi+44],mm3
		movq		[UV16],mm4

		psrlw		mm2,15
		mov			esi,GBitPtr

		psrlq		mm4,[QShiftV]

		psllw		mm2,2
		movq		[UV16V],mm4

		movq		mm4,mm2


		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB
		punpcklwd	mm4,[Zero]

		mov			ebx,dword ptr[UV16+4]
		paddd		mm4,[QZorCan]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB

		movq		[QZOut],mm4

		mov			edi,dword ptr[QZOut]
		movq		mm4,mm2

		mov			ax,word ptr[QZVal]

		mov			edi,dword ptr[edi]
		pmullw		mm7,mm6

		mov			word ptr[edi+24],ax

		punpcklwd	mm4,[Zero]
		xor			eax,eax

		paddd		mm4,[QDibOrCan]

		movq		[QDibOut],mm4

		paddw		mm6,[RGBADelta]

		mov			edi,dword ptr[QDibOut]
		psrlw		mm7,8

		movq		mm5,[QNegAlpha]
		pmullw		mm7,qword ptr[VertAlpha]

		mov			edi,dword ptr[edi]
		psubw		mm5,qword ptr[VertAlpha]

		movq		mm3,[edi+48]
		paddd		mm0,mm1

		punpcklbw	mm3,[Zero]
		movq		mm4,mm0

		pmullw		mm3,mm5
		shr			ebx,16

		mov			edx,dword ptr[UV16V]
		paddw		mm3,mm7

		pand		mm4,[WrapMask]
		psrlw		mm3,8

		movq		[UV16],mm4
		packuswb	mm3,mm3

		xor			eax,eax
		add			esi,edx

		movd		[edi+48],mm3
		paddd		mm0,mm1

		add			esi,ebx
		mov			al,byte ptr[esi]

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		psrlq		mm4,[QShiftV]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB

		movq		[UV16V],mm4

		mov			ebx,dword ptr[UV16+4]

		mov			edx,dword ptr[UV16V]

		mov			edi,dword ptr[QZOut+4]
		pmullw		mm7,mm6

		mov			ax,word ptr[QZVal+2]

		mov			edi,dword ptr[edi]
		psrlw		mm7,8

		mov			word ptr[edi+26],ax

		mov			edi,dword ptr[QDibOut+4]
		shr			ebx,16

		paddw		mm6,[RGBADelta]

		movq		mm5,[QNegAlpha]
		pmullw		mm7,qword ptr[VertAlpha]

		mov			edi,dword ptr[edi]
		psubw		mm5,qword ptr[VertAlpha]

		movq		mm3,[edi+52]
		mov			esi,GBitPtr

		punpcklbw	mm3,[Zero]
		movq		mm4,mm0

		pmullw		mm3,mm5
		add			esi,edx

		paddw		mm3,mm7
		xor			eax,eax

		pand		mm4,[WrapMask]
		psrlw		mm3,8

		movq		[UV16],mm4
		packuswb	mm3,mm3

		psrlq		mm4,[QShiftV]
		add			esi,ebx

		movq		[UV16V],mm4
		xor			eax,eax


		movd		[edi+52],mm3
		mov			al,byte ptr[esi]

		mov			esi,GBitPtr
		movq		mm4,mm2

		movd		mm7,[ecx+eax*4]		; ULw|VLw DU16|DV16  UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |ARGB

		punpckhwd	mm4,[Zero]

		mov			ebx,dword ptr[UV16+4]
		paddd		mm4,[QZorCan]

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB
		movq		[QZOut],mm4

		mov			edi,dword ptr[QZOut]
		movq		mm4,mm2

		mov			ax,word ptr[QZVal+4]

		mov			edi,dword ptr[edi]
		pmullw		mm7,mm6

		mov			word ptr[edi+28],ax

		xor			eax,eax

		punpckhwd	mm4,[Zero]
		psrlw		mm7,8

		paddd		mm4,[QDibOrCan]

		paddw		mm6,[RGBADelta]

		movq		mm5,[QNegAlpha]
		pmullw		mm7,qword ptr[VertAlpha]

		movq		[QDibOut],mm4
		psubw		mm5,qword ptr[VertAlpha]

		mov			edi,dword ptr[QDibOut]
		paddd		mm0,mm1

		mov			edi,dword ptr[edi]
		movq		mm4,mm0

		movq		mm3,[edi+56]
		shr			ebx,16

		punpcklbw	mm3,[Zero]

		mov			edx,dword ptr[UV16V]
		pmullw		mm3,mm5

		pand		mm4,[WrapMask]
		paddw		mm3,mm7

		add			esi,edx
		psrlw		mm3,8

		add			esi,ebx
		packuswb	mm3,mm3

		paddd		mm0,mm1
		mov			al,byte ptr[esi]

		movd		[edi+56],mm3

		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ARGB UZR|VZR   AR|GB
		xor			eax,eax

		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw   AR|GB   UZR|VZR   AR|GB
		mov			edi,dword ptr[QZOut+4]

		mov			ax,word ptr[QZVal+6]

		mov			edi,dword ptr[edi]
		pmullw		mm7,mm6

		mov			word ptr[edi+30],ax

		mov			edi,dword ptr[QDibOut+4]

		paddw		mm6,[RGBADelta]

		mov			edi,dword ptr[edi]
		psrlw		mm7,8

		movq		[ARL],mm6

		pop			ebp

		movq		mm5,[QNegAlpha]

		pmullw		mm7,qword ptr[VertAlpha]
		psubw		mm5,qword ptr[VertAlpha]

		;regrab z
		movq		mm2,[QZVal32_0]
		movq		mm3,[edi+60]

		movq		mm4,[QZVal32_1]
		punpcklbw	mm3,[Zero]

		paddd		mm2,[QZDelta]
		pmullw		mm3,mm5

		paddd		mm4,[QZDelta]
		paddw		mm3,mm7

		movq		[QZVal32_0],mm2
		psrlw		mm3,8

		movq		[QZVal32_1],mm4
		packuswb	mm3,mm3

		movd		[edi+60],mm3

		; get corrected right side deltas ; ULw|VLw DU16|DV16 argb|ARGBagAG|rbRB    |ZRi   aA|rR   UZR|VZR aArR|gGbB

		movq		mm4,[ZIR]
		movq		mm6,[UVZ]

		movq		mm2,[UVR]
		movq		mm0,[UVLeftW]		; ULw|VLw DU16|DV16 argb|ARGBagAG|rbRB    |ZRi   aA|rR   UZR|VZR aArR|gGbB

		movq		mm1,[UVLeft]		; ULw|VLw   UL|VL   argb|ARGBagAG|rbRB ZRi|ZRi   aA|rR   UZR|VZR aArR|gGbB
		punpckldq	mm2,mm2				; ULw|VLw   UL|VL    ZRi|ZRi agAG|rbRB ZRi|ZRi   aA|rR   UZR|VZR aArR|gGbB

		pfmul		mm2,mm6				; ULw|VLw   UL|VL     UR|VR  agAG|rbRB ZRi|ZRi   aA|rR   UZR|VZR aArR|gGbB
		mov			eax,offset QZCan
		add			dword ptr[Dest],32

		add			dword ptr[eax+4],32
		mov			eax,offset QDibCan

		add			dword ptr[eax+4],64

		movq		mm3,mm2
		pfmul		mm3,[QFixedScale]

		pf2id		mm3,mm3

		dec		[NumASpans]			; dec num affine spans
		jnz		SpanLoopLit

HandleLeftoverPixelsLit:


		mov		esi,GBitPtr


		cmp			[RemainingCount],0
		jz			ReturnLit

		mov			eax,[RemainingCount]
		mov			dword ptr[ZIR],eax
		mov			dword ptr[ZIR+4],eax
		
		movq		mm7,[GLMapMulUV]	;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR   LU|LV
		movq		mm5,mm1

		pfsub		mm6,[UVDivZ16StepX]
		pfmul		mm7,mm1				;UL6i|VL6i  UL|VL  UR6i|VR6i URi|VRi     |ZRi     |ZY   UZR|VZR  ULL|VLL

		pfmul		mm5,[QFixedScale]
		pi2fd		mm3,[ZIR]

		pfmul		mm7,[QFixedScale]
		pfmul		mm3,[UVDivZStepX]

		pf2id		mm5,mm5
		pfadd		mm3,mm6

		pi2fd		mm2,[ZIR]
		movd		mm6,[Zi16StepX]

		pf2id		mm7,mm7				; ULw|VLw   DU|DV    UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |
		pfsub		mm4,mm6

		movd		mm6,[ZiStepX]
		mov			ebx,[RemainingCount]

		pfmul		mm2,mm6

		shl			ebx,3

		pfadd		mm2,mm4
		paddd		mm5,[UVAdjust]

		punpckldq	mm4,mm4

		movq		[UVL16],mm7
		pfrcp		mm2,mm2

		pfmul		mm4,[QZBufferPrec]

		paddd		mm5,[UVAdjust2]
		pfmul		mm2,mm3

		movq		[UVLeft],mm5
		pfsubr		mm1,mm2				; ULw|VLw   DU|DV    UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |

		pfmul		mm1,qword ptr[QFixedScaleLUT+ebx]	; ULw|VLw DU16|DV16  UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |

		movq		mm0,mm5
		pand		mm5,[WrapMask]
		pf2id		mm1,mm1				; ULw|VLw DU16|DV16  UR|VR   URw|VRw     |ZRi     |ZY   UZR|VZR     |

		movq		[UV16],mm5
		movq		mm7,[UVAdjustL]

		psrlq		mm5,[QShiftV]
		paddd		mm7,[UVL16]

		movq		[UV16V],mm5
		movq		[UVL16],mm7


OnePixelSpanLit:
		; Clamp U/V
		mov		ebx,dword ptr[UVL16+4]
		cmp		ebx,MaxU
		jle		TryClampU1Lit
		mov		ecx,MaxU
		mov		dword ptr[UVL16+4],ecx
		jmp		NoClampU1Lit

TryClampU1Lit:
		cmp		ebx,0
		jge		NoClampU1Lit
		mov		dword ptr[UVL16+4],0
NoClampU1Lit:
		mov		eax,dword ptr[UVL16]
		cmp		eax,MaxV
		jle		TryClampV1Lit
		mov		ecx,MaxV
		mov		dword ptr[UVL16],ecx
		jmp		NoClampV1Lit

TryClampV1Lit:
		cmp		eax,0
		jge		NoClampV1Lit
		mov		dword ptr[UVL16],0

NoClampV1Lit:
		mov			ebx,dword ptr[UV16+4]
		mov			eax,dword ptr[UV16V]

		mov			esi,GBitPtr
		shr			ebx,16
		add			esi,eax
		add			esi,ebx
		mov			ecx,TexPal
		xor			eax,eax
		movq		mm6,[ARL]

		mov			eax,offset QDibCan
		push		ebp

		mov			edi,dword ptr[eax+4]
		mov			ebp,dword ptr[Dest]

		movq		[UVZ],mm1			;using this for a step temp

		movd		mm3,dword ptr[ZiStepX]
		xor			eax,eax

LeftoverLoopLit:
		paddd		mm0,[UVZ]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw     |ZRi     |ZY   UZR|VZR     |
		pf2id		mm2,mm4

		mov			al,byte ptr[esi]
		movq		mm5,mm0				;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw   UL|VL      |ZY   UZR|VZR     |

		mov			esi,GBitPtr
		pfadd		mm4,mm3

		pand		mm5,[WrapMask]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |
		psrld		mm2,16

		movq		[UV16],mm5			;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |
		movd		mm7,[ecx+eax*4]		;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR     |ARGB

		psrlq		mm5,[QShiftV]
		movd		eax,mm2

		movq		[UV16V],mm5
		punpcklbw	mm7,qword ptr[Zero]	;  ULw|VLw  DU16|DV16   UL|VL   ULw|VLw  ULw|VLw     |ZY   UZR|VZR   AR|GB

		xor			eax,0
		mov			ebx,dword ptr[UV16+4]

		shr			ebx,16

		add			esi,ebx

		mov			edx,dword ptr[UV16V]

		pmullw		mm7,mm6

		psrlw		mm7,8
		movq		mm1,[QNegAlpha]

		pmullw		mm7,qword ptr[VertAlpha]
		psubw		mm1,qword ptr[VertAlpha]

		movq		mm5,[edi]
		paddw		mm6,[RGBADelta]

		punpcklbw	mm5,[Zero]
		add			esi,edx

		pmullw		mm5,mm1
		cmp			ax,word ptr[ebp]
		jl			SkipPixelLitZ

		paddw		mm5,mm7
		psrlw		mm5,8
		packuswb	mm5,mm5

		movd		[edi],mm5

SkipPixelLitZ:

		xor			eax,eax
		add			edi,4
		add			ebp,2

		dec		[RemainingCount]
		jge		LeftoverLoopLit

		pop			ebp

ReturnLit:

	pop		edi
	pop		esi
	pop		ecx
	pop		ebx

	cRet DrawSpan32_AsmGouraudZBufferVertexAlpha3DNow
endProc DrawSpan32_AsmGouraudZBufferVertexAlpha3DNow


;put the machine into 3dnow mode

cProc Femms3DNow, 0,<>

		femms

	cRet Femms3DNow
endProc Femms3DNow


;edge step for 3dnow

cProc StepWorld3DNow, 4,<edge : dword>


		mov     eax,edge

		movq	mm0,qword ptr[eax+12]
		movq	mm1,qword ptr[eax+20]

		pfadd	mm0,qword ptr[eax+28]
		pfadd	mm1,qword ptr[eax+36]

		movq	qword ptr[eax+12],mm0
		pf2id	mm0,mm0

		movq	qword ptr[eax+20],mm1
		pf2id	mm1,mm1

		mov		edx,dword ptr[eax+4]
		movq	qword ptr[eax+52],mm1

		inc		edx
		movd	dword ptr[eax],mm0

		mov		dword ptr[eax+4],edx
		psrlq	mm0,32

		mov		edx,dword ptr[eax+8]
		movd	dword ptr[eax+48],mm0

		dec		edx
		mov		dword ptr[eax+8],edx



	cRet StepWorld3DNow
endProc StepWorld3DNow



_TEXT$01   ends

end


