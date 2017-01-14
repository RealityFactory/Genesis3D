/****************************************************************************************/
/*  DIBDisplay.C                                                                        */
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
/*Genesis3D Version 1.1 released November 15, 1999                            */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/

#define WIN32_LEAN_AND_MEAN
#pragma warning(disable : 4201 4214 4115)
#include <windows.h>
#pragma warning(default : 4201 4214 4115; disable : 4514)

#include "DIBDisplay.h"
#include "CPUInfo.h"
#include <assert.h>
#include <stdio.h>			// vsprintf()
#include <malloc.h>			// malloc(),free()


#ifdef GENESIS_VERSION_2
#include "errorlog.h"
#else
#define geErrorLog_AddString(Error,xx,yy) 
#define geErrorLog_Add(Error,xx) 
#endif


#define DIBDISPLAY_DESCRIPTION_STRING "Software (Window)"

typedef struct
{
   uint32 r;
   uint32 g;
   uint32 b;
   int32 alpha;
}
DIBDisplay_RGB32;


	// data needed for DIB section (and some cached info about bitmasks
typedef struct
{
	uint32		DIB_R_bitmask;			// DIB high-color pixel format
	uint32		DIB_G_bitmask;
	uint32		DIB_B_bitmask;

	BITMAPINFO  *pbmi;					// BITMAPINFO structure
	HBITMAP		hDIB;					// Handle returned from CreateDIBSection

	uint8		*lpDIBBuffer;			// DIB image buffer
} DIBDisplay_DIBInfo;	



typedef struct DIBDisplay 
{
	HWND				hWnd;					// Handle to application window
	geBoolean			Locked;					// is display 'locked'
	int32				BitsPerPixel;			// Mode info set by last call to 
	int32				Pitch;					// (BitsPerPixel/8) * Size_X
	int32				Size_X;					// size of window rounded up to the nearest multiple of 4
	int32				Size_Y;
	uint32				Flags;					// display flags (currently unused)
	DIBDisplay_DIBInfo	DIBInfo;
} DIBDisplay;


void DIBDisplay_GetDisplayFormat(	const DIBDisplay *D,
									int32   *Width, 
									int32   *Height,
									int32   *BitsPerPixel,
									uint32  *Flags)
{
	assert( D            != NULL );
	assert( Width        != NULL );
	assert( Height       != NULL );
	assert( BitsPerPixel != NULL );
	assert( Flags        != NULL );

	*Width        = D->Size_X;
	*Height       = D->Size_Y;
	*BitsPerPixel = D->BitsPerPixel;
	*Flags        = D->Flags;
}	


geBoolean DIBDisplay_GetDisplayInfo(	char			*DescriptionString, 
										unsigned int	 DescriptionStringMaxLength,
										DisplayModeInfo *Info)
{
	int BitsPerPixel;

	assert( Info != NULL );
	assert( DescriptionString != NULL );
	assert( DescriptionStringMaxLength > 0 );
	if (strlen(DIBDISPLAY_DESCRIPTION_STRING) >= DescriptionStringMaxLength)
		{
			geErrorLog_AddString(GE_ERR_BAD_PARAMETER,"DIBDisplay_GetDescriptionString: description string too short",NULL);
			return GE_FALSE;
		}

	strcpy(DescriptionString,DIBDISPLAY_DESCRIPTION_STRING);

//	if ( CPUInfo_TestFor3DNow()==GE_TRUE)
//		BitsPerPixel = 32;
//	else
		BitsPerPixel = 16;

	if (DisplayModeInfo_AddEntry(Info,-1,-1,BitsPerPixel,0)==GE_FALSE)
		{
			geErrorLog_AddString(GE_ERR_SUBSYSTEM_FAILURE,"DIBDisplay_GetDescriptionString: unable to add mode entry",NULL);
			return GE_FALSE;
		}
	return GE_TRUE;
}


//------------------------------------------------
static void DIBDisplay_DebugPrintf(char *fmt, ...)
{
	static char work_string[4096];

	if ((fmt == NULL) || (strlen(fmt) > sizeof(work_string)))
		{
			strcpy(work_string, "(String missing or too large)");
		}
	else
		{
			va_list ap;

			va_start(ap,fmt);

			vsprintf(work_string, fmt, ap);

			va_end  (ap);
		}

	#if 0  // log to file
	{
		FILE       *log;
		log = fopen("DEBUG.LOG","a+t");

		if (log != NULL)
			{
				fprintf(log,"%s\n",work_string);
				fclose(log);
			}
	}
	#endif

	OutputDebugString(work_string);
}


//------------------------------------------------
#ifndef NDEBUG
static geBoolean DIBDisplay_IsValid(const DIBDisplay *D)
{
	if (D == NULL)
		return GE_FALSE;

	if (D->DIBInfo.pbmi == NULL)
		return GE_FALSE;
	if (D->DIBInfo.hDIB == NULL)
		return GE_FALSE;
	if (D->DIBInfo.lpDIBBuffer == NULL)
		return GE_FALSE;

	return GE_TRUE;
}
#endif


//------------------------------------------------
static geBoolean DIBDisplay_FindDesktopRGBMasks(	HDC		hdc,
								int32	desktop_bpp,
								uint32  *desktop_R_bitmask,
								uint32	*desktop_G_bitmask,
								uint32	*desktop_B_bitmask)
{
	assert( hdc != NULL );
	assert( desktop_R_bitmask != NULL );
	assert( desktop_G_bitmask != NULL );
	assert( desktop_B_bitmask != NULL );

	if(desktop_bpp > 8)
		{
			COLORREF		color,save;
			HBITMAP			TempBmp;
			OSVERSIONINFO	WinVer;

			memset(&WinVer, 0, sizeof(OSVERSIONINFO));
			WinVer.dwOSVersionInfoSize	=sizeof(OSVERSIONINFO);

			GetVersionEx(&WinVer);
			if(WinVer.dwPlatformId == VER_PLATFORM_WIN32_NT)
				{
					TempBmp	=CreateCompatibleBitmap(hdc, 8, 8);
					if(TempBmp)
						{
							BITMAPINFO		*TempInfo;
							int HeaderSize =  sizeof(BITMAPINFO) + sizeof(RGBQUAD)*2;
							TempInfo = malloc(HeaderSize);
							if (TempInfo == NULL)
								{
									geErrorLog_Add(DIBDisplay_FindDesktopRGBMasks,"Failed to allocate header for NT mask test");
									DeleteObject(TempBmp);
									return GE_FALSE;
								}
							memset(TempInfo, 0, HeaderSize);
							TempInfo->bmiHeader.biSize		=sizeof(BITMAPINFO);
							TempInfo->bmiHeader.biBitCount	=(uint16)desktop_bpp;
							TempInfo->bmiHeader.biCompression	=BI_BITFIELDS;
							if(GetDIBits(hdc, TempBmp, 0, 0, NULL, TempInfo, DIB_RGB_COLORS))
								{
									*desktop_R_bitmask	=*((uint32 *)&TempInfo->bmiColors[0]);
									*desktop_G_bitmask	=*((uint32 *)&TempInfo->bmiColors[1]);
									*desktop_B_bitmask	=*((uint32 *)&TempInfo->bmiColors[2]);
									DIBDisplay_DebugPrintf("(%x-%x-%x)\n", *desktop_R_bitmask, *desktop_G_bitmask, *desktop_B_bitmask);
								}
							else
								{
									free(TempInfo);
									DeleteObject(TempBmp);
									geErrorLog_AddString(GE_ERR_WINDOWS_API_FAILURE,"DIBDisplay_FindDesktopRGBMasks:  Unable to get bit format from desktop compatible bitmap",NULL);
									return GE_FALSE;
								}
							free(TempInfo);
							DeleteObject(TempBmp);
						}
					else
						{
							geErrorLog_AddString(GE_ERR_WINDOWS_API_FAILURE,"DIBDisplay_FindDesktopRGBMasks:  Unable to create desktop compatible bitmap",NULL);
							return GE_FALSE;
						}
					// detect failure (Win2000 only?)
					if (((*desktop_R_bitmask | *desktop_G_bitmask | *desktop_B_bitmask)&0x7FFF) != 0x7FFF)
						{
							SetLastError(0);
							geErrorLog_AddString(GE_ERR_WINDOWS_API_FAILURE,"DIBDisplay_FindDesktopRGBMasks:  GetDIBits didn't return proper masking information.",NULL);
							return GE_FALSE;
						}

				}
			else
				{
					HDC hdc2;
					
					hdc2 = CreateDC("Display", NULL, NULL,NULL);
					if (hdc2==NULL)
						{
							geErrorLog_AddString(GE_ERR_WINDOWS_API_FAILURE,"DIBDisplay_FindDesktopRGBMasks: Unable to create compatible hdc",NULL);
							return GE_FALSE;
						}
						
					save = GetPixel(hdc2,0,0);
					if ( save == CLR_INVALID )
						{
							geErrorLog_AddString(GE_ERR_WINDOWS_API_FAILURE,"DIBDisplay_FindDesktopRGBMasks:  Unable to get pixel from desktop",NULL);
							DeleteDC(hdc2);
							return GE_FALSE;
						}
							

					if (SetPixel(hdc2,0,0,RGB(0x08,0x08,0x08))==-1)
						{
							geErrorLog_AddString(GE_ERR_WINDOWS_API_FAILURE,"DIBDisplay_FindDesktopRGBMasks: Unable to select bitmap",NULL);
							DeleteDC(hdc2);
							return GE_FALSE;
						}
					
					color = GetPixel(hdc2,0,0) & 0xffffff;
					if ( color == CLR_INVALID )
						{
							geErrorLog_AddString(GE_ERR_WINDOWS_API_FAILURE,"DIBDisplay_FindDesktopRGBMasks:  Unable to get test pixel from desktop",NULL);
							DeleteDC(hdc2);
							return GE_FALSE;
						}

					SetPixel(hdc,0,0,save);		// ignore an error here.
					DeleteDC(hdc2);

					DIBDisplay_DebugPrintf("DIBDisplay: Desktop pixel format = 0x%X ",color);
					

					switch (color)
						{
							case 0x000000:						// 0x000000 = 5-5-5
								DIBDisplay_DebugPrintf("(5-5-5)\n");

								*desktop_R_bitmask = 0x007c00;
								*desktop_G_bitmask = 0x0003e0;
								*desktop_B_bitmask = 0x00001f;
								break;

							case 0x000800:						// 0x000800 = 5-6-5
    							DIBDisplay_DebugPrintf("(5-6-5)\n");

								*desktop_R_bitmask = 0x00f800;
								*desktop_G_bitmask = 0x0007e0;
								*desktop_B_bitmask = 0x00001f;
								break;
				
							case 0x080808:						// 0x080808 = 8-8-8
								DIBDisplay_DebugPrintf("(8-8-8)\n");

								*desktop_R_bitmask = 0xff0000;
								*desktop_G_bitmask = 0x00ff00;
								*desktop_B_bitmask = 0x0000ff;
								break;

							default:
								DIBDisplay_DebugPrintf("(Unsupported, using 5-6-5)\n");

								if ((desktop_bpp == 15) || (desktop_bpp == 16))
								   {
									   *desktop_R_bitmask = 0x00f800;
									   *desktop_G_bitmask = 0x0007e0;
									   *desktop_B_bitmask = 0x00001f;
									}
								else
									{
										geErrorLog_AddString(GE_ERR_DISPLAY_RESOURCE,"DIBDisplay_FindDesktopRGBMasks:  Unsupported desktop format.",NULL);
										return GE_FALSE;
									}
								break;
							}

				}
		}
	return GE_TRUE;
}




//------------------------------------------------
static void DIBDisplay_DestroyDIB(DIBDisplay_DIBInfo *DI)
{
	assert( DI != NULL );

	if (DI->pbmi != NULL)
		{
			free(DI->pbmi);
			DI->pbmi = NULL;
		}

	if (DI->hDIB != NULL)
		{
			DeleteObject(DI->hDIB);
			DI->hDIB = NULL;
		}
}

//------------------------------------------------
static geBoolean DIBDisplay_CreateDIB( 
				int32   display_size_X,
                int32   display_size_Y,
                int32   display_bpp,
				HWND    hWnd,
				DIBDisplay_DIBInfo *DI)
{
	HDC      hdc;
	BITMAPINFOHEADER *pbmih;              // Pointer to pbmi header
	uint32		desktop_R_bitmask;        // Desktop high-color pixel format
	uint32		desktop_G_bitmask;        
	uint32		desktop_B_bitmask;
	int32		desktop_bpp;              // Windows video mode BPP
	
	assert( display_size_X > 0 );
	assert( display_size_Y > 0 );
	assert( (display_bpp == 16) || (display_bpp == 24));
	assert( DI != NULL );

	DI->hDIB        = NULL;
	DI->lpDIBBuffer = NULL;

	DI->pbmi = (BITMAPINFO *) 
				malloc(sizeof (BITMAPINFOHEADER) + (sizeof (RGBQUAD) * 256));

	if (DI->pbmi == NULL)
		{
			// geErrorLog_AddString("failed to allocate bitmap header",NULL);
			return GE_FALSE;
		}

	memset(DI->pbmi, 0, sizeof (BITMAPINFOHEADER) + (sizeof (RGBQUAD) * 256));

	pbmih = &(DI->pbmi->bmiHeader);

	pbmih->biSize          =  sizeof(BITMAPINFOHEADER);
	pbmih->biWidth         =  display_size_X;
	pbmih->biHeight        = -(display_size_Y);
	pbmih->biPlanes        =  1;
	pbmih->biBitCount      =  (uint16) display_bpp;
	pbmih->biSizeImage     =  0;
	pbmih->biXPelsPerMeter =  0;
	pbmih->biYPelsPerMeter =  0;
	pbmih->biClrUsed       =  0;
	pbmih->biClrImportant  =  0;

	//
	// Get Windows desktop display format (and masks, in high-color modes)
	//

	hdc = GetDC(hWnd);
	if ( hdc == NULL )
		{
			// geErrorLog_AddString("unable to get desktop HDC");
			return GE_FALSE;
		}

	desktop_bpp = GetDeviceCaps(hdc, BITSPIXEL) * 
				  GetDeviceCaps(hdc, PLANES);

	DIBDisplay_DebugPrintf("DIBDisplay: Desktop %d BPP\n", desktop_bpp);

	if (DIBDisplay_FindDesktopRGBMasks( hdc,
										desktop_bpp,
										&desktop_R_bitmask,
										&desktop_G_bitmask,
										&desktop_B_bitmask)==GE_FALSE)
		{
			//geErrorLog_AddString("unable to determine desktop bit format");
			//ReleaseDC(hWnd, hdc);
			//return GE_FALSE;
			desktop_R_bitmask = 0x00f800;
			desktop_G_bitmask = 0x0007e0;
			desktop_B_bitmask = 0x00001f;
		}
			

	//
	// If DIB and desktop are both in 15/16-BPP mode, set DIB to desktop
	// pixel format for maximum throughput
	// 
	// Otherwise, set DIB to 5-6-5 mode if 16BPP DIB requested, or 8-8-8 mode
	// if 24BPP DIB requested
	//
	// Finally, if 8BPP DIB requested, create GDI palette object based on 
	// current logical palette
	//

	switch (display_bpp)
	{
		case 16:

			pbmih->biCompression = BI_BITFIELDS;

			if ((desktop_bpp == 15) || (desktop_bpp == 16))
				{
					DI->DIB_R_bitmask = desktop_R_bitmask;
					DI->DIB_G_bitmask = desktop_G_bitmask;
					DI->DIB_B_bitmask = desktop_B_bitmask;
				}
			else
				{
					DI->DIB_R_bitmask = 0x00f800;
					DI->DIB_G_bitmask = 0x0007e0;
					DI->DIB_B_bitmask = 0x00001f;
				}
			break;

		case 24:

			pbmih->biCompression = BI_BITFIELDS;
			DI->DIB_R_bitmask = 0xff0000;
			DI->DIB_G_bitmask = 0x00ff00;
			DI->DIB_B_bitmask = 0x0000ff;
			break;
	}

	*(uint32 *) (&(DI->pbmi->bmiColors[0])) = DI->DIB_R_bitmask;
	*(uint32 *) (&(DI->pbmi->bmiColors[1])) = DI->DIB_G_bitmask;
	*(uint32 *) (&(DI->pbmi->bmiColors[2])) = DI->DIB_B_bitmask;


	//
	// Allocate the DIB section ("back buffer")
	//

	DI->hDIB = CreateDIBSection(hdc,							// Device context
								DI->pbmi,						// BITMAPINFO structure
								DIB_RGB_COLORS,					// Color data type
								(void **) &(DI->lpDIBBuffer),	// Address of image map pointer
								NULL,							// File
								0);								// Bitmap file offset

	ReleaseDC(hWnd, hdc);

	if (DI->hDIB == NULL)
		{
			//geErrorLog_AddString("CreateDIBSection failed");
			return GE_FALSE;
		}

	return GE_TRUE;
	
}

//------------------------------------------------
geBoolean DIBDisplay_Blit(DIBDisplay *D)
{
	HDC hdc;
	assert( DIBDisplay_IsValid(D) != GE_FALSE );

	hdc = GetDC(D->hWnd);
	if (hdc == NULL)
		{
			//geErrorLog_AddString("Unable to get HDC for blit");
			return GE_FALSE;
		}

	#if 0
		if (desktop_bpp == 8)
			{
			//
			// Select palette if desktop running in 8-bit mode
			//
			// If palette has changed, realize it
			//

				//probably want to set up a halftone palette, and dither down into the 8 bbp desktop.

				SelectPalette(	hdc,  hPalette, 0);

				if (palette_change_request != GE_FALSE)
					{
						palette_change_request = GE_FALSE;
						RealizePalette(hdc);
					}
			}
	#endif


	//
	// Disable Boolean operations during stretching
	//

	if (SetStretchBltMode(hdc, COLORONCOLOR)==0)
		{
			//geErrorLog_AddString("unable to set stretch blit mode");
			return GE_FALSE;
		}

	StretchDIBits(hdc,            // Destination DC
				  0,              // Destination X
				  0,              // Destination Y
				  D->Size_X,     // Destination (client area) width
				  D->Size_Y,    // Destination (client area) height 
				  0,              // Source X
				  0,              // Source Y
				  D->Size_X, // Source (back buffer) width
				  D->Size_Y, // Source (back buffer) height
				  D->DIBInfo.lpDIBBuffer,    // Pointer to source (back buffer)
				  D->DIBInfo.pbmi,           // Bitmap info for back buffer
				  DIB_RGB_COLORS, // Bitmap contains index values
				  SRCCOPY);       // Do normal copy with stretching

	ReleaseDC(D->hWnd, hdc);
	return GE_TRUE;
}


//------------------------------------------------
geBoolean DIBDisplay_Wipe	(	DIBDisplay   *D,
								uint32        color)
{
	assert( DIBDisplay_IsValid(D) != GE_FALSE );
	if (!D->Locked)
		{
			//gegeErrorLog_AddString(Add(,"Display must be locked to clear",NULL);
			return GE_FALSE;
		}

	if (color==0)
		memset(D->DIBInfo.lpDIBBuffer, color, D->Size_Y * D->Pitch);
	else
		{
			int i;
			int16 *Ptr = (int16 *)D->DIBInfo.lpDIBBuffer;
			int16 C    = (int16)color;
			for (i=(D->Size_X * D->Size_Y); i>0; i--)
				{
					*(Ptr++) = C;
				}
		}
						
	return GE_TRUE;
}



//------------------------------------------------
DIBDisplay *DIBDisplay_Create	(	HWND hWindow,
									int Width,
									int Height,
									int   display_bpp,
									uint32  Flags )
{
	DIBDisplay *D;
	RECT	window_rect;
	BOOL	result;
	Flags;	Width;  Height; // avoid unused formal parameter warnings

	assert( display_bpp    > 0);

	D= malloc(sizeof(*D));
	if (D==NULL)
		{
			// errlogAdd("failed to allocate DIBDisplay object");
			return NULL;
		}


	result = GetClientRect(hWindow, &window_rect);

	if( !result )
		{
			//errlogAdd("failed to get client rect");
			return NULL;
		}
	assert ( window_rect.left  == 0 );
	assert ( window_rect.top == 0 );
	
	#pragma message ("getting size from window!")
	D->Size_X = ((window_rect.right  - window_rect.left)+3)&~3;
	D->Size_Y = (window_rect.bottom - window_rect.top);

	if ( D->Size_X <=0 )
		{
			//errlogAdd("bad window client width");
			return NULL;
		}
	if ( D->Size_Y <=0 )
		{
			//errlogAdd("bad widndow client height");
			return NULL;
		}

	D->Locked				= GE_FALSE;   
	D->BitsPerPixel			= display_bpp;
	D->Pitch				= (D->BitsPerPixel / 8 ) * D->Size_X;
	D->hWnd					= hWindow;

	D->DIBInfo.pbmi					= NULL;
	D->DIBInfo.hDIB					= NULL;
	D->DIBInfo.lpDIBBuffer			= NULL;
	
	if (DIBDisplay_CreateDIB(D->Size_X, D->Size_Y, display_bpp,
								hWindow,
								&(D->DIBInfo)) ==GE_FALSE)
		{
			// errlogAdd("failed to create Dib section")
			DIBDisplay_Destroy(&D);
			return NULL;
		}
	
	return D;
}


//------------------------------------------------
void DIBDisplay_Destroy(DIBDisplay **pD)
{
	DIBDisplay *D;

	assert( *pD != NULL );
	assert( pD  != NULL );

	D = *pD;

	assert( DIBDisplay_IsValid(D) != GE_FALSE );
	
	DIBDisplay_DestroyDIB(&(D->DIBInfo));
	free( D );

	*pD = NULL;
}



//------------------------------------------------
geBoolean DIBDisplay_Lock      (DIBDisplay *D,
								uint8       **ptr,
								int32       *pitch)
{
	assert( DIBDisplay_IsValid(D) != GE_FALSE );
	assert( ptr    != NULL );
	assert( pitch  != NULL );
	assert( D->Locked == GE_FALSE );

	*ptr    = D->DIBInfo.lpDIBBuffer;
	*pitch  = D->Pitch;

	D->Locked = GE_TRUE;
	return GE_TRUE;
}


geBoolean DIBDisplay_Unlock        (DIBDisplay *D)
{
	assert( DIBDisplay_IsValid(D) != GE_FALSE );
	assert( D->Locked == GE_TRUE );
	D->Locked = GE_FALSE;
	return GE_TRUE;
}


//------------------------------------------------
geBoolean DIBDisplay_GetPixelFormat(	const DIBDisplay *D,
										//int32 *pixel_pitch, 
										int32 *bytes_per_pixel,
										int32 *R_shift,
										uint32 *R_mask,
										int32 *R_width,
										int32 *G_shift,
										uint32 *G_mask,
										int32 *G_width,
										int32 *B_shift,
										uint32 *B_mask,
										int32 *B_width)
{
	int32 red_shift=0;
	uint32 red_mask;
	int32 red_width=0;
	int32 grn_shift=0;
	uint32 grn_mask;
	int32 grn_width=0;
	int32 blu_shift=0;
	uint32 blu_mask;
	int32 blu_width=0;
	int32 i;

	assert( DIBDisplay_IsValid(D) != GE_FALSE );

	//assert( pixel_pitch     != NULL );
	assert( bytes_per_pixel != NULL );
	assert( R_shift         != NULL );
	assert( R_mask          != NULL );
	assert( R_width         != NULL );
	assert( G_shift         != NULL );
	assert( G_mask          != NULL );
	assert( G_width         != NULL );
	assert( B_shift         != NULL );
	assert( B_mask          != NULL );
	assert( B_width         != NULL );
	


	//*pixel_pitch     = (D->BitsPerPixel / 8);
	*bytes_per_pixel = (D->BitsPerPixel / 8);
		
	red_mask = D->DIBInfo.DIB_R_bitmask;
	grn_mask = D->DIBInfo.DIB_G_bitmask;
	blu_mask = D->DIBInfo.DIB_B_bitmask;
	
	//
	// Derive shift, width values from masks
	//

	for (i=31; i >= 0; i--)
		{
			if (red_mask & (1 << i))
				{
					red_shift = i;
				}

			if (grn_mask & (1 << i))
				{
					grn_shift = i;
				}

			if (blu_mask & (1 << i))
				{
					blu_shift = i;
				}
		}

	for (i=0; i <= 31; i++)
		{
			if (red_mask & (1 << i))
				{
					red_width = i - red_shift + 1;
				}

			if (grn_mask & (1 << i))
				{
					grn_width = i - grn_shift + 1;
				}

			if (blu_mask & (1 << i))
				{
					blu_width = i - blu_shift + 1;
				}
		}
	//
	// Pass all requested values back to the caller
	//

	*R_shift = red_shift;
	*G_shift = grn_shift;
	*B_shift = blu_shift;

	*R_mask  = red_mask;
	*G_mask  = grn_mask;
	*B_mask  = blu_mask;

	*R_width = red_width;
	*G_width = grn_width;
	*B_width = blu_width;
		
	return GE_TRUE;
}




