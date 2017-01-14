/****************************************************************************************/
/*  span.h                                                                              */
/*                                                                                      */
/*  Author:       John Pollard, Ken Baird                                               */
/*  Description:  Header for span.c and important defines                               */
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
#ifndef SPAN_H
#define SPAN_H

#include "basetype.h"

#define MAXSPANS 25000
#define MAXSCANLINES 768

typedef struct SList 
{
    uint8		Used;
	int32		Min, Max;
	float	z1, z2;		// So we can spit z spans out for other spans that need to be occluded
    uint32		Flags;
    struct SList	*Last;
    struct SList	*Next;
} SList;

typedef struct
{
	SList	*First;
	SList	*Current;
} SpanMinMax;

extern	int32	NumCachedAlphaPolys;
extern	int32	NumCachedAlphaSpans;

SList	*NewSList(void);
void	FreeSList(SList *s);
void	ResetSList(void);
void	ResetSpans(uint32 Rows);

extern SpanMinMax SMinMax[MAXSCANLINES];
extern SList ScanHash[MAXSPANS];  // hash table for SList
extern int32 CurrentSList;


#endif
