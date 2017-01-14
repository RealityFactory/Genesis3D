/****************************************************************************************/
/*  drawspan.h                                                                          */
/*                                                                                      */
/*  Author:       Ken Baird                                                             */
/*  Description:  header for span draw code                                             */
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
#include "softdrv.h"
#include "basetype.h"

typedef struct EdgeAsmFPUTag EdgeAsmFPU;

extern	uint16	*TDest, *pZBufferPtr, *Dest;
extern	__int64	Red, Green;
extern	float	Real65536;
extern	double	Bucket, Bucket2, Bucket3, Magic, RedMask, RedMask2;
extern	uint32	UMask, VShift, VMask, TempPix, Blue;
static	uint32	VStep, UStep, ZStep, u16, v16, z16, Z32;

extern	float	const	Two, MiniRedMask, GreenMask;
extern	float	const	MiniRedMask2, GreenMask2, BlueMask;
extern	geFloat			FloatTemp, FTemp0, FTemp1, FTemp2;
extern	geFloat			FTemp3, FTemp4, FTemp5, FTemp6, FTemp7, FTemp8;

extern	int32	GHMaskShifted, GHMaskShifted16, GWMaskShifted, GWMask, widTemp;
extern	uint8	*GBitPtrHalf;
extern	double	MipMagic;
extern	double	MipMagic2;

extern uint32	NumASpans, RemainingCount;
extern double	DeltaU, DeltaV, DeltaW;
extern uint32	UFixed, VFixed, WLeft;
extern uint8	*pTex;

extern	int32	R1, B1, G1, R2, G2, B2;
extern	int32	RR1, RR2, GG1, GG2, BB1, BB2;
extern	int32	StepR, StepG, StepB;
extern	int32	UDist, VDist;
extern	int32	U1, V1, CKeyTest;
extern	float	GLMapMulU;	//lightscale
extern	float	GLMapMulV;	//lightscale
extern	int32	ZDelta, ZVal;
extern	float	ZBufferPrec;
extern	__int64	RedDelta, GreenDelta, BlueDelta;


//555
#define	REDMASK2	0x7c007c00;
#define	GREENMASK2	0x03e003e0;
#define	BLUEMASK2	0x001f001f;

//565
#define	REDMASK		0xf800f800;
#define	GREENMASK	0x07e007e0;
#define	BLUEMASK	0x001f001f;
