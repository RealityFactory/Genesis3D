/****************************************************************************************/
/*  SpanBuffer.C                                                                        */
/*                                                                                      */
/*  Author: Mike Sandige                                                                */
/*  Description:  This is a raster-line based span buffer (like a z buffer but it works */
/*                with groups of horizontal pixels, rather than single pixels)          */
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
#include <Assert.h>

#include "SpanBuffer.h"
#include "Ram.h"

#ifdef GENESIS_VERSION_2
#include "errorlog.h"
#else
#define geErrorLog_AddString(Error,xx,yy) 
#endif

//#define SPAN_MAX_SPARES 25000 
//#define SPAN_MAX_LINES 768
//#define RESOLUTION_X 640
//#define RESOLUTION_Y 640

typedef struct SpanBuffer_Span SpanBuffer_Span;

typedef struct SpanBuffer_Span
{
	int	Min, Max;
    SpanBuffer_Span *Next;
} SpanBuffer_Span;

static SpanBuffer_Span	**SpanBuffer_Lines;			// list of spans for each scanline
static SpanBuffer_Span *SpanBuffer_Spares;			// unused spans since last Clear
static int				SpanBuffer_FirstSpare;		// index of next spare span: SpanBuffer_Spares[SpanBuffer_FirstSpare]
static int				SpanBuffer_LineCount;		// number of lines in span buffer table.
static int				SpanBuffer_MaxSpares;
SpanBuffer_ClipSegment  *SpanBuffer_Segments;		// epxorted list of clipped spans (segments)	


void SpanBuffer_Destroy(void)
{
	if ( SpanBuffer_Lines != NULL )
		geRam_Free(SpanBuffer_Lines);
	SpanBuffer_Lines = NULL;

	if ( SpanBuffer_Spares != NULL )
		geRam_Free(SpanBuffer_Spares);
	SpanBuffer_Spares = NULL;

	if ( SpanBuffer_Segments != NULL )
		geRam_Free(SpanBuffer_Segments);
	SpanBuffer_Segments = NULL;
}

void SpanBuffer_Clear(void)
{ 
	int i;
	assert(SpanBuffer_Lines!=NULL);

	for (i=0; i<SpanBuffer_LineCount; i++) 
		{
			SpanBuffer_Lines[i] = NULL;
		}	
	SpanBuffer_FirstSpare = 0;
}

geBoolean SpanBuffer_Create(int Width, int Height, int MaxSpans)
{
	assert( Width > 0 );
	assert( Height > 0 );
	assert( MaxSpans > 0 );

	SpanBuffer_Destroy();

	SpanBuffer_MaxSpares = MaxSpans;
	SpanBuffer_LineCount = Height;

	SpanBuffer_Lines = geRam_Allocate(sizeof(SpanBuffer_Span *) * Height);
	if (SpanBuffer_Lines == NULL)
		{
			geErrorLog_AddString( GE_ERR_MEMORY_RESOURCE,"Unable to create span buffer table",NULL);
			return GE_FALSE;
		}

	SpanBuffer_Spares = geRam_Allocate(sizeof(SpanBuffer_Span) * MaxSpans);
	if (SpanBuffer_Lines == NULL)
		{
			geErrorLog_AddString(GE_ERR_MEMORY_RESOURCE ,"Unable to create spare span list",NULL);
			geRam_Free(SpanBuffer_Lines);
			SpanBuffer_Lines = NULL;
			return GE_FALSE;
		}

	SpanBuffer_Segments = geRam_Allocate(sizeof(SpanBuffer_ClipSegment) * (Width/2) );
	if (SpanBuffer_Segments == NULL )
		{
			geErrorLog_AddString( GE_ERR_MEMORY_RESOURCE,"Unable to create span buffer clip segment list",NULL);
			geRam_Free(SpanBuffer_Lines); 
			SpanBuffer_Lines = NULL;
			geRam_Free(SpanBuffer_Spares);
			SpanBuffer_Spares = NULL;
			return GE_FALSE;
		}
	SpanBuffer_Clear();
	return GE_TRUE;
}

#if 0 // not used (yet)
geBoolean SpanBuffer_Visible(int Line, int Left, int Right)
{
	SpanBuffer_Span *S;
	assert( Line>=0 );
	assert( Line< SpanBuffer_LineCount );
	assert( Right>=Left );
	// assumes that adjacent spans are always merged.

	S = SpanBuffer_Lines[Line];
	while (S)
		{
			if (Left<S->Min)
				return GE_TRUE;
			if (Right<=S->Max)
				return GE_FALSE;
			S=S->Next;
		}
	return GE_TRUE;
}
#endif

int SpanBuffer_ClipAndAdd(int Line, int LeftStart, int Width)
{
	SpanBuffer_Span *LastSpan;
	SpanBuffer_Span *NewSpan;
	SpanBuffer_Span *Span;
	SpanBuffer_ClipSegment *Segment;
	int Left = LeftStart;
	int Right = LeftStart + Width -1;  
	int SegmentCount = 0;
	#pragma message("fix this off by one problem here and in the engine!")
	if (Line>=SpanBuffer_LineCount)
		return 0;

	assert( Line >= 0 );
	assert( Line< SpanBuffer_LineCount );
	assert( Width >= 0 ); 
	assert( LeftStart >=0 );

	Segment  = &(SpanBuffer_Segments[0]);
	LastSpan = NULL;
	Span     = SpanBuffer_Lines[Line];

	while (Span)
		{
			if (Left < Span->Min)
				{   // new span starts to left of this span
					if (Right<Span->Min)  
						{	// new span ends to left of this span
							break;
						}
					else
						{	// new span ends in or to the right of this span
							Segment->LeftOffset = Left - LeftStart;
							Segment->Width = Span->Min - Left;
							SegmentCount++;
							Segment++;
							// Stretch Span to Left. 
							Span->Min = Left;
							if (LastSpan)
								{
									if (LastSpan->Max+1 == Span->Min)
										{	// collapse LastSpan with Span
											LastSpan->Max = Span->Max;
											LastSpan->Next = Span->Next;
											Span = LastSpan;
											// *Span is lost for this frame
										}
								}
						}
				}
			if (Left <= Span->Max)
				{
					Left = Span->Max + 1;
					if (Left > Right)
						return SegmentCount;
				}
			LastSpan = Span;
			Span = Span->Next;
		}

	// add span
	Segment->LeftOffset = Left - LeftStart;
	Segment->Width = Right - Left + 1;
	SegmentCount++;


	if (LastSpan)
		{
			if (LastSpan->Max+1 == Left)
				{	// stretch LastSpan 
					LastSpan->Max = Right;
				}
			else
				{
					if (SpanBuffer_FirstSpare<SpanBuffer_MaxSpares)
						NewSpan = &(SpanBuffer_Spares[SpanBuffer_FirstSpare++]);
					else
						return SegmentCount;
					NewSpan->Min   = Left;
					NewSpan->Max   = Right;
					NewSpan->Next  = Span;
					LastSpan->Next = NewSpan;
				}	
		}
	else
		{
			if (SpanBuffer_FirstSpare<SpanBuffer_MaxSpares)
				NewSpan = &(SpanBuffer_Spares[SpanBuffer_FirstSpare++]);
			else
				return SegmentCount;
			NewSpan->Min   = Left;
			NewSpan->Max   = Right;
			NewSpan->Next  = Span;
			SpanBuffer_Lines[Line] = NewSpan;
		}


	return SegmentCount;
}	


#if 0
void SpanBuffer_Test()
{
	int Visible;

	SpanBufferReset(1);
	Visible = SpanBuffer_Visible(0, 5,9 );

	SpanBufferClipAndAdd(0,30,10);
	Visible = SpanBuffer_Visible(0, 5,9 );
	Visible = SpanBuffer_Visible(0, 30,40);
	Visible = SpanBuffer_Visible(0, 30,41);
	Visible = SpanBuffer_Visible(0, 29,40);
	
	SpanBufferClipAndAdd(0,10,10);
	Visible = SpanBuffer_Visible(0, 10,50);
	Visible = SpanBuffer_Visible(0, 30,41);
	Visible = SpanBuffer_Visible(0, 50,60);
	Visible = SpanBuffer_Visible(0, 1,2);
	Visible = SpanBuffer_Visible(0, 15,35);
	Visible = SpanBuffer_Visible(0, 25,26);
	
	Visible = SpanBuffer_Visible(0, 10,20);
	Visible = SpanBuffer_Visible(0, 30,40);
	
		
	SpanBufferClipAndAdd(0,5,50);
	Visible = SpanBuffer_Visible(0, 5,10);
	Visible = SpanBuffer_Visible(0, 60,70);
	Visible = SpanBuffer_Visible(0, 1,2);


}
#endif