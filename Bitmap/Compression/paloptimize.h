#ifndef GE_PALOPTIMIZE_H
#define GE_PALOPTIMIZE_H

/****************************************************************************************/
/*  PalOptimize                                                                         */
/*                                                                                      */
/*  Author: Charles Bloom                                                               */
/*  Description:  Palette Perfecting code                                               */
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

#include "basetype.h"
#include "bitmap.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void paletteOptimize(const geBitmap_Info * Info,const void * Bits,
						uint8 *palette,int palEntries,int maxSamples);

	// use maxIterations == 0 or -1 for infinity

#ifdef __cplusplus
}
#endif

#endif
