/****************************************************************************************/
/*  DDRAWDisplay.H                                                                      */
/*                                                                                      */
/*  Author:  Mike Sandige                                                               */
/*  Description:  display surface manager for full screen Direct Draw using a direct    */
/*                draw surface for the the frame buffer                                 */
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
#ifndef DDRAWDISPLAY_H
#define DDRAWDISPLAY_H


#include "basetype.h"
#include "DisplayModeInfo.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DDRAWDisplay DDRAWDisplay;

void	 DDRAWDisplay_GetDisplayFormat(		const DDRAWDisplay *D,
											int32   *Width, 
											int32   *Height,
											int32   *BitsPerPixel,
											uint32  *Flags);

geBoolean DDRAWDisplay_GetDisplayInfo(	char			*DescriptionString, 
										unsigned int	 DescriptionStringMaxLength,
										DisplayModeInfo *Info);

geBoolean DDRAWDisplay_GetPixelFormat(	DDRAWDisplay *D,
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

geBoolean	DDRAWDisplay_Lock(		DDRAWDisplay *D,uint8 **Buffer, int32 *Pitch);
geBoolean	DDRAWDisplay_Unlock(	DDRAWDisplay *D);
geBoolean	DDRAWDisplay_Blit(		DDRAWDisplay *D);
geBoolean	DDRAWDisplay_Wipe(		DDRAWDisplay *D,uint32 color);
geBoolean	DDRAWDisplay_SetActive(	DDRAWDisplay *D, geBoolean Active);
void		DDRAWDisplay_Destroy(	DDRAWDisplay **D);

#ifdef _INC_WINDOWS	
DDRAWDisplay *DDRAWDisplay_Create( HWND hwnd, int Width, int Height, int BBP, uint32 Flags);
#endif

#ifdef __cplusplus
}
#endif

#endif

