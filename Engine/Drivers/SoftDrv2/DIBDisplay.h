/****************************************************************************************/
/*  DIBDisplay.H                                                                        */
/*                                                                                      */
/*  Author:  Mike Sandige                                                               */
/*  Description:  display surface manager for windows with a DIB as the frame buffer    */
/*                Code fragments contributed by John Miles                              */
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


#ifndef DIBDisplay_H
#define DIBDisplay_H

#include "basetype.h"
#include "DisplayModeInfo.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DIBDisplay DIBDisplay;

geBoolean DIBDisplay_GetDisplayInfo(	char			*DescriptionString, 
										unsigned int	 DescriptionStringMaxLength,
										DisplayModeInfo *Info);

void DIBDisplay_GetDisplayFormat(		const DIBDisplay *D,
										int32   *Width, 
										int32   *Height,
										int32   *BitsPerPixel,
										uint32  *Flags);

geBoolean DIBDisplay_GetPixelFormat  (	const DIBDisplay *D,
										//int32       *pixel_pitch,
										int32       *bytes_per_pixel,
										int32       *R_shift,
										uint32      *R_mask,
										int32       *R_width,
										int32       *G_shift,
										uint32      *G_mask,
										int32       *G_width,
										int32       *B_shift,
										uint32      *B_mask,
										int32       *B_width);


geBoolean DIBDisplay_Blit		(	DIBDisplay *D);

geBoolean DIBDisplay_Wipe		(	DIBDisplay *D,	
									uint32        color);

geBoolean DIBDisplay_Lock		(	DIBDisplay *D,
									uint8       **ptr,
									int32       *pitch);

geBoolean DIBDisplay_Unlock		(	DIBDisplay *D);

void DIBDisplay_Destroy			(	DIBDisplay **pDIBDisplay);

                         
						 
#ifdef _INC_WINDOWS						                             
DIBDisplay *DIBDisplay_Create	(	HWND hWindow,
									int  Width,
									int  Height,
									int  display_bpp,
									uint32 Flags);
#endif


#ifdef __cplusplus
}
#endif

#endif
