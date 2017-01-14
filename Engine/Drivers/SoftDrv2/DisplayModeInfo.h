/****************************************************************************************/
/*  DisplayModeInfo.H                                                                   */
/*                                                                                      */
/*  Author:  Mike Sandige                                                               */
/*  Description:  This is a simple container to hold information about available display*/
/*                modes for the software driver.                                        */
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

#ifndef DisplayModeInfo_H
#define DisplayModeInfo_H

#include "basetype.h"

#ifdef __cplusplus
extern "C" {
#endif


enum	DisplayModeInfo_Flags
{
	SYSTEM				=1,	//store buffer in system
	VIDEO				=2,	//or video ram
	HARDWARE			=4,	//refresh choices
	DMABLT				=8,
	FASTBLT				=16,
	SAFEBLT				=32,
	FLIP				=64,
	DMAPAGELOCKREQUIRED	=128,//pagelock for dma req
	DMAASYNCH			=256,//can do asynch dma
	STRETCHMODE			=512,//stretch to fit
	MODEXMODE			=1024
};

typedef struct DisplayModeInfo DisplayModeInfo;

DisplayModeInfo *	DisplayModeInfo_Create(			void );

void				DisplayModeInfo_Destroy(		DisplayModeInfo **Info );

int					DisplayModeInfo_GetModeCount(	DisplayModeInfo *Info);

geBoolean			DisplayModeInfo_AddEntry(		DisplayModeInfo *Info, 
													int Width,
													int Height,
													int BitsPerPixel,
													uint32 Flags );

geBoolean			DisplayModeInfo_GetNth(			DisplayModeInfo *Info, 
													int Nth,
													int *Width, 
													int *Height,
													int *BitsPerPixel,
													uint32 *Flags);

#ifdef __cplusplus
}
#endif


#endif

