#ifndef GE_BRANDO_PALETTIZE_H
#define GE_BRANDO_PALETTIZE_H

/****************************************************************************************/
/*  Palettize                                                                           */
/*                                                                                      */
/*  Author: Charles Bloom                                                               */
/*  Description:  Palettize-ing code                                                    */
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

geBoolean palettizePlane(const	geBitmap_Info * SrcInfo,const	void * SrcBits,
								geBitmap_Info * DstInfo,		void * DstBits,
								int SizeX,int SizeY);

// you can create a palette with routines in "palcreate.h"

/******* if you want to do your own palettizing : ******/

typedef struct palInfo palInfo;

extern palInfo *	closestPalInit(uint8 * palette);
extern void			closestPalFree(palInfo *info);
extern int			closestPal(int R,int G,int B,palInfo *pi);

extern void Palettize_Start(void);
extern void Palettize_Stop(void);

#ifdef __cplusplus
}
#endif

#endif
