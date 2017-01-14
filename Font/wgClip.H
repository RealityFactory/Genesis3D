/****************************************************************************************/
/*  WGCLIP.H                                                                            */
/*                                                                                      */
/*  Author: Thom Robertson                                                              */
/*  Description: 2D rectangular clip testing support                                    */
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
#ifndef GE_CLIP_H
#define GE_CLIP_H


#ifdef __cplusplus
extern "C" {
#endif

#define GE_CLIP_CENTER 1
#define GE_CLIP_CORNER 0
#include	"basetype.h"
#include	"getypes.h"

//***************************************************************
// returns true if you need to draw at all.
GENESISAPI geBoolean GENESISCC CalculateClipping(
                           GE_Rect *artRect, int32 *resultX, int32 *resultY, 
                           int32 x, int32 y,
                           const GE_Rect bounds, int32 type);

#ifdef __cplusplus
}
#endif

#endif

