/****************************************************************************************/
/*  GSpan.h                                                                             */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Front to back span code                                                */
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
#ifndef GSPAN
#define GSPAN

#include <Windows.h>

#define MAX_SPAN_LINES			1024
#define MAX_SPANS				35000

typedef struct 
{
	int32	x1;								// Starting x on screen
	int32	x2;								// Ending x on screen
} SPAN;

typedef struct _SList
{
    int32	Min, Max;
    uint8	Used;
    uint32	Flags;
    _SList	*Last;
    _SList	*Next;
} SLIST;

typedef struct
{
	SLIST *First;
	SLIST *Current;
} SPAN_MINMAX;

extern	SPAN	SpanLines[MAX_SPAN_LINES];

extern	SPAN_MINMAX	SMinMax[MAX_SPAN_LINES];			// Linked list of spans for each scanline...
extern	SLIST	ScanHash[MAX_SPANS];					// hash table for SList

extern	int32		NumWorldPixels;
extern	int32		NumSpans;
extern	int32		NumSpanPixels[MAX_SPAN_LINES];
extern	int32		PolysRendered;

void	DRIVERCC EdgeOutNoUV (int32 x1, int32 y1, int32 x2, int32 y2);
void	DRIVERCC AddSpanNoUV(int32 x1, int32 x2, int32 y);

void	ResetSList(void);
SLIST	*NewSList(void);
void	ResetSpans(int32 Rows);

#endif