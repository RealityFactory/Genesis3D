/****************************************************************************************/
/*  Display.H                                                                           */
/*                                                                                      */
/*  Author:  Mike Sandige                                                               */
/*  Description:  Abstracts all low-level display surfaces into a single API            */
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
// Display
//   display manager 
//
//   manages 
//     DIB format window displays
//     DDRAW format fullscreen displays


#ifndef Display_H
#define Display_H

#include "basetype.h"
#include "DisplayModeInfo.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { DISPLAY_DIB_WINDOW, DISPLAY_DDRAW_FULLSCREEN, DISPLAY_COUNT } Display_Type;

typedef struct Display Display;

geBoolean Display_GetDisplayInfo(		Display_Type	 DisplayType,
										char			*DescriptionString, 
										unsigned int	 DescriptionStringMaxLength,
										DisplayModeInfo *Info);

geBoolean Display_GetPixelFormat	(	const Display *D,
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

void	Display_GetDisplayFormat(		const Display *D,
										Display_Type *DisplayType,
										int32   *Width, 
										int32   *Height,
										int32   *BitsPerPixel,
										uint32  *Flags);

geBoolean Display_Blit		(	Display *D);

geBoolean Display_Wipe		(	Display *D,	
								uint32        color);

geBoolean Display_Lock		(	Display *D,
								uint8       **ptr,
								int32       *pitch);

geBoolean Display_Unlock	(	Display *D);

void Display_Destroy		(	Display **pDisplay);

geBoolean Display_SetActive	(	Display *D, geBoolean Active );

						 
#ifdef _INC_WINDOWS						                             
Display *Display_Create	(	HWND hWindow,
							Display_Type DisplayType,
							int32   RenderSizeAcross, 
							int32   RenderSizeDown,
							int32   Display_BitsPerPixel,
							uint32  Display_Flags);
#endif

#ifdef __cplusplus
}
#endif

#endif
