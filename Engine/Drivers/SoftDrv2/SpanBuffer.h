/****************************************************************************************/
/*  SpanBuffer.H                                                                        */
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
/*  Genesis3D Version 1.1 released November 15, 1999                                 */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#ifndef SPANBUFFER_H
#define SPANBUFFER_H

#include "basetype.h" 
 
#ifdef __cplusplus
extern "C" {
#endif

typedef struct 
{									// This is a clipped segment of a span.
	int LeftOffset;					// offset from starting pixel from original span (LeftStart)
	int Width;						// width of this segment
}  SpanBuffer_ClipSegment;		

// this array holds the resulting clipped spans (segments) that result from calling _ClipAndAdd()
extern SpanBuffer_ClipSegment *SpanBuffer_Segments;


	// initializes the span buffer
geBoolean SpanBuffer_Create(int Width, int Height, int MaxSpans);

	// destroys the span buffer
void SpanBuffer_Destroy(void);

	// empties the span buffer
void	SpanBuffer_Clear(void);

	// adds a new span.  The span is specified by a starting pixel and a width(number of pixels)
	//  The return value is the number of clipped spans (segments) to draw.  (0 if none)
	//	The clipped spans are put into the exported array (SpanBuffer_Segments[0..return value+1])
int		SpanBuffer_ClipAndAdd(int Line, int LeftStart, int Width);


#ifdef __cplusplus
}
#endif


#endif
