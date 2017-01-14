/****************************************************************************************/
/*  Span.H                                                                              */
/*                                                                                      */
/*  Author: Mike Sandige                                                                */
/*  Description:  Span abstracts and contains all the various ROP functions.            */
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
#include "rop.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (GENESISCC *Span_DrawFunction)(void);

typedef enum 
{
	GE_SPAN_DESTINATION_FORMAT_555,
	GE_SPAN_DESTINATION_FORMAT_565,

	GE_SPAN_DESTINATION_FORMATS
} geSpan_DestinationFormat;

typedef enum
{
	GE_SPAN_HARDWARE_INTEL,
	GE_SPAN_HARDWARE_MMX,
	GE_SPAN_HARDWARE_AMD,

	GE_SPAN_HARDWARE_VERSIONS
} geSpan_CPU;

geBoolean GENESISCC Span_SetOutputMode( geSpan_DestinationFormat DestFormat, geSpan_CPU CPU);

Span_DrawFunction GENESISCC Span_GetDrawFunction(geROP ROP);


#ifdef __cplusplus
}
#endif


#endif
