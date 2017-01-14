/****************************************************************************************/
/*  span.c                                                                              */
/*                                                                                      */
/*  Author:       John Pollard, Ken Baird                                               */
/*  Description:  Span reject list type stuff                                           */
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
#include <Assert.h>
#include <Stdio.h>
#include <Dos.h>

#include "System.h"
#include "Span.h"
#include "Render.h"
#include "SoftDrv.h"
#include "Scene.h"
#include "3dnowspan.h"
#include "x86span565.h"
#include "x86span555.h"


SpanMinMax		SMinMax[MAXSCANLINES];			// Linked list of spans for each scanline...
SList			ScanHash[MAXSPANS];				// hash table for SList
int32				CurrentSList;					// Current SUB in SList we are at...
int32				NumSpanPixels[MAXSCANLINES];
extern 		CPUInfo			ProcessorInfo;
extern 		BOOL			bBackLocked;

extern  __int64	WrapMask, QFixedScale16;
extern  __int64	GLMapMulUV, UVL16;
extern  __int64	UVAdjust, UVAdjust2;
extern  __int64	UVDivZ16StepX, ARL, GBL;
extern  __int64	UVDivZOrigin, UVR;
extern  __int64	QFixedScale, ZIR;
extern  __int64	UVDivZStepX, UVDivZStepY;
extern  uint8 *TexPal;

extern  BOOL	LockDDrawBackBuffer(DRV_Window *cwnd, RECT *wrect);

//***************************************************************************
//	Clears the index into the ScanHash array.  This array is where I do all
//	my SList allocating...
//***************************************************************************
void ResetSList(void)
{
	CurrentSList = 0;
}

//***************************************************************************
//	Makes a new valid SList in ScanHash, then updates the CurrentSList indexer.
//***************************************************************************
SList *NewSList(void)
{

   CurrentSList++;                          

   assert(CurrentSList < MAXSPANS);

   return &ScanHash[CurrentSList-1];

   return NULL;
}

// ***************************************************************************
// Just clears some variables so we know this SList is no longer used...
// ***************************************************************************
void FreeSList(SList *s)
{
    assert(s != NULL);
    s->Used  = 0;
}

//================================================================================
// Just clears the global span buckets...
//================================================================================
void ResetSpans(uint32 Rows)
{ 
	uint32 i;

	for (i=0; i<Rows; i++) 
	{
		SMinMax[i].First = NULL;
		NumSpanPixels[i] = 0;
	}	

	ResetSList();
}

