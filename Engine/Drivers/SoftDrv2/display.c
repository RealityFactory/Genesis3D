/****************************************************************************************/
/*  Display.C                                                                           */
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
/*Genesis3D Version 1.1 released November 15, 1999                            */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#pragma warning(disable : 4201 4214 4115)
#include <windows.h>
#pragma warning(default : 4201 4214 4115; disable : 4514)
#include <assert.h>
#include "basetype.h"
#include "display.h"
#include "DIBDisplay.h"
#include "DDRAWDisplay.h"

#ifdef GENESIS_VERSION_2
#include "errorlog.h"
#else
#define geErrorLog_AddString(Error,xx,yy) 
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Display 
{
	Display_Type DisplayType;
	DIBDisplay	 *pDIBDisplay;
	DDRAWDisplay *pDDRAWDisplay;
}Display;

#pragma message ("BitsPerPixel should be a Bitmap Format")

void Display_GetDisplayFormat(		const Display *D,
									Display_Type *DisplayType,
									int32   *Width, 
									int32   *Height,
									int32   *BitsPerPixel,
									uint32  *Flags)
{
	assert( D            != NULL );
	assert( DisplayType  != NULL );
	assert( Width        != NULL );
	assert( Height       != NULL );
	assert( BitsPerPixel != NULL );
	assert( Flags        != NULL );

	*DisplayType = D->DisplayType;
	if (D->DisplayType == DISPLAY_DIB_WINDOW)
		{
			DIBDisplay_GetDisplayFormat(	D->pDIBDisplay,
											Width, 
											Height,
											BitsPerPixel,
											Flags);
		}
	else
		{
			DDRAWDisplay_GetDisplayFormat(	D->pDDRAWDisplay,
											Width, 
											Height,
											BitsPerPixel,
											Flags);
		}
}	



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
										int32       *B_width)
{
	assert( D != NULL);
	if (D->DisplayType == DISPLAY_DIB_WINDOW)
		{
			return DIBDisplay_GetPixelFormat(	D->pDIBDisplay,
												//pixel_pitch,
												bytes_per_pixel,
												R_shift, R_mask, R_width,
												G_shift, G_mask, G_width,
												B_shift, B_mask, B_width);
		}
	else
		{
			return DDRAWDisplay_GetPixelFormat( D->pDDRAWDisplay,
												//pixel_pitch,
												bytes_per_pixel,
												R_shift, R_mask, R_width,
												G_shift, G_mask, G_width,
												B_shift, B_mask, B_width);
		}
}



geBoolean Display_Blit		(	Display *D )
{
	assert( D != NULL);
	if (D->DisplayType == DISPLAY_DIB_WINDOW)
		{
			return DIBDisplay_Blit(	D->pDIBDisplay );
		}
	else
		{
			return DDRAWDisplay_Blit( D->pDDRAWDisplay );
		}
}

geBoolean Display_Wipe		(	Display *D,	
								uint32        color)
{
	assert( D != NULL);
	if (D->DisplayType == DISPLAY_DIB_WINDOW)
		{
			return DIBDisplay_Wipe(	D->pDIBDisplay, color );
		}
	else
		{
			return DDRAWDisplay_Wipe(	D->pDDRAWDisplay, color );
		}
}

#pragma message ("should the ptr argument to Display_Lock be a uint8?")

geBoolean Display_Lock		(	Display *D,
								uint8       **ptr,
								int32       *pitch)
{
	assert( D != NULL);
	if (D->DisplayType == DISPLAY_DIB_WINDOW)
		{
			return DIBDisplay_Lock(	D->pDIBDisplay, ptr, pitch );
		}
	else
		{
			return DDRAWDisplay_Lock(D->pDDRAWDisplay,ptr,pitch);
		}
}

geBoolean Display_Unlock	(	Display *D  )
{
	assert( D != NULL);
	if (D->DisplayType == DISPLAY_DIB_WINDOW)
		{
			return DIBDisplay_Unlock(	D->pDIBDisplay );
		}
	else
		{
			return DDRAWDisplay_Unlock( D->pDDRAWDisplay);
		}
}

geBoolean Display_SetActive	(	Display *D, geBoolean Active )
{
	assert( D != NULL);
	if (D->DisplayType == DISPLAY_DIB_WINDOW)
		{
			return GE_TRUE;
		}
	else
		{
			return DDRAWDisplay_SetActive(D->pDDRAWDisplay,Active);
		}
}

void Display_Destroy		(	Display **pDisplay )
{
	Display *D;
	assert( pDisplay != NULL );
	D = *pDisplay;
	assert( D != NULL);
	if (D->DisplayType == DISPLAY_DIB_WINDOW)
		{
			DIBDisplay_Destroy(	&(D->pDIBDisplay) );
			D->pDIBDisplay = NULL;
		}
	else
		{
			DDRAWDisplay_Destroy(	&(D->pDDRAWDisplay) );
			D->pDDRAWDisplay = NULL;
		}
	free( D );
	*pDisplay = NULL;
}


Display *Display_Create	(	HWND hWindow,
							Display_Type DisplayType,
							int32   RenderSizeAcross, 
							int32   RenderSizeDown,
							int32   Display_BitsPerPixel,
							uint32   Flags)
{
	Display *D;
	D = malloc( sizeof( Display ) );
	assert( (DisplayType == DISPLAY_DIB_WINDOW) || (DisplayType == DISPLAY_DDRAW_FULLSCREEN));
	
	if (D == NULL)
		{
			geErrorLog_AddString(GE_ERR_MEMORY_RESOURCE,"unable to create Display object",NULL);
			return NULL;
		}

	D->DisplayType   = DisplayType;
	D->pDIBDisplay   = NULL;
	D->pDDRAWDisplay = NULL;
		
	if (D->DisplayType == DISPLAY_DIB_WINDOW)
		{
			D->pDIBDisplay =  DIBDisplay_Create( hWindow,RenderSizeAcross,RenderSizeDown,Display_BitsPerPixel,Flags );
			if (D->pDIBDisplay == NULL)
				{
					geErrorLog_AddString(GE_ERR_SUBSYSTEM_FAILURE,"Unable to create DIBDisplay object",NULL);
					free(D);
					return NULL;
				}
		}
	else
		{
			D->pDDRAWDisplay = DDRAWDisplay_Create( hWindow,RenderSizeAcross,RenderSizeDown,Display_BitsPerPixel,Flags );
			if (D->pDDRAWDisplay == NULL)
				{
					geErrorLog_AddString(GE_ERR_SUBSYSTEM_FAILURE,"Unable to create DDRAWDisplay object",NULL);
					free(D);
					return NULL;
				}
		}
	return D;
}


geBoolean Display_GetDisplayInfo(	Display_Type	 DisplayType,
									char			*DescriptionString, 
									unsigned int	 DescriptionStringMaxLength,
									DisplayModeInfo *Info)
{
	assert( (DisplayType == DISPLAY_DIB_WINDOW) || (DisplayType == DISPLAY_DDRAW_FULLSCREEN));
	if (DisplayType == DISPLAY_DDRAW_FULLSCREEN)
		{
			return DDRAWDisplay_GetDisplayInfo(	DescriptionString,  DescriptionStringMaxLength, Info);
		}
	else
		{
			return DIBDisplay_GetDisplayInfo(	DescriptionString,  DescriptionStringMaxLength, Info);
		}
}