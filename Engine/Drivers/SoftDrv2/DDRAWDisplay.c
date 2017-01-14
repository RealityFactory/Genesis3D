/****************************************************************************************/
/*  DDRAWDisplay.C                                                                      */
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
/*Genesis3D Version 1.1 released November 15, 1999                            */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/

#pragma warning(disable : 4201 4214 4115)
#include <windows.h>
#include <assert.h>
#include "ddraw.h"
#include <stdio.h>
#pragma warning(default : 4201 4214 4115; disable : 4514)

#include "DDRAWDisplay.h" 

#ifdef GENESIS_VERSION_2
#include "errorlog.h"
#else
#define geErrorLog_AddString(Error,xx,yy) 
#endif


#define DDRAWDISPLAY_DESCRIPTION_STRING "Software (Full Screen)  "

typedef struct 
{
	LPDIRECTDRAW4			lpDD4;
	HANDLE					ddrawinst;
} DDRAWDisplay_DLLHooks;

typedef struct DDRAWDisplay 
{
	DDRAWDisplay_DLLHooks  DLL;

	LPDIRECTDRAWSURFACE4	lpDDSPrimary;	// DirectDraw primary surface
	LPDIRECTDRAWSURFACE4	lpDDSBack;		// DirectDraw back surface
	BOOL					bActive;

	int				Width;
	int				Height;
	int				BitsPerPixel;
	int				ModeFlags;

	HWND			hWnd;
	geBoolean		Locked;
	uint8			*Buffer;
	int32			Pitch;
} DDRAWDisplay;

typedef HRESULT (WINAPI *LPDIRECTDRAWCREATE)( GUID FAR *lpGUID, LPDIRECTDRAW FAR *lplpDD, IUnknown FAR *pUnkOuter );


static geBoolean DDRAWDisplay_IsValid(DDRAWDisplay *D)
{
	if ( D == NULL )
		return GE_FALSE;

	if (D->DLL.ddrawinst == NULL )
		return GE_FALSE;

	if (D->DLL.lpDD4 == NULL)
		return GE_FALSE;

	if (D->hWnd == NULL )
		return GE_FALSE;

	return GE_TRUE;
}

static void DDRAWDisplay_UnloadDLL( DDRAWDisplay_DLLHooks *H )
{
	assert( H != NULL );
	if (H->lpDD4)
		{
			H->lpDD4->lpVtbl->Release(H->lpDD4);
			H->lpDD4 = NULL;
		}
	if (H->ddrawinst)
		{
			FreeLibrary(H->ddrawinst);
			H->ddrawinst = NULL;
		}
}

static geBoolean DDRAWDisplay_LoadDLL( DDRAWDisplay_DLLHooks *H )
{
	LPDIRECTDRAW			lpDD       = NULL;			// DirectDraw object
	LPDIRECTDRAWCREATE		lpDDCreate = NULL;
	HRESULT	ddrval;
	assert( H != NULL );
	
	H->ddrawinst = NULL;
	H->lpDD4	 = NULL;


	H->ddrawinst	=LoadLibrary("ddraw.dll");

	if(!H->ddrawinst)
	{
		geErrorLog_AddString(-1,"failed to load","ddraw.dll");
		goto LoadDLL_ERROR;
	}
	
	lpDDCreate	=(LPDIRECTDRAWCREATE)GetProcAddress(H->ddrawinst, "DirectDrawCreate");
	if(lpDDCreate)
		{
			ddrval	=lpDDCreate(NULL, &(lpDD), NULL);
			if((ddrval != DD_OK) || ((lpDD)==NULL))
				{
					geErrorLog_AddString(-1,"ddraw lpDDCreate failed",  geErrorLog_IntToString(ddrval));
					goto LoadDLL_ERROR;
				}
		}
	else
		{
			geErrorLog_AddString(-1 ,"Unable to find DirectDrawCreate entry into ddraw.dll",NULL);
			goto LoadDLL_ERROR;
		}
	
	ddrval = lpDD->lpVtbl->QueryInterface(lpDD, &IID_IDirectDraw4, (LPVOID *)&(H->lpDD4));
	if(ddrval!=DD_OK)
	{
		geErrorLog_AddString(-1,"QueryInterface failed", geErrorLog_IntToString(ddrval));
		goto LoadDLL_ERROR;
	}
	
	lpDD->lpVtbl->Release(lpDD);
	lpDD	=NULL;


	return GE_TRUE;


	LoadDLL_ERROR:
		if (lpDD != NULL )
			{
				lpDD->lpVtbl->Release(lpDD);
				lpDD	=NULL;
			}
		DDRAWDisplay_UnloadDLL(H);
	return GE_FALSE;
}
	
void DDRAWDisplay_GetDisplayFormat(	const DDRAWDisplay *D,
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

	*Width        = D->Width;
	*Height       = D->Height;
	*BitsPerPixel = D->BitsPerPixel;
	*Flags        = D->ModeFlags;
}	

void	DDRAWDisplay_Destroy(DDRAWDisplay **pD)
{
	DDRAWDisplay *D;

	assert( pD );
	assert( DDRAWDisplay_IsValid(*pD)!=GE_FALSE );
	D = *pD;

	if (D->Locked == GE_TRUE)
		DDRAWDisplay_Unlock(D);

	if (D->ModeFlags & FLIP)
	{
		D->lpDDSBack	=NULL;
	}
	else
	{
		if(D->lpDDSBack)
		{
			D->lpDDSBack->lpVtbl->Release(D->lpDDSBack);
			D->lpDDSBack	=NULL;
		}
	}
	if(D->lpDDSPrimary)
	{
		D->lpDDSPrimary->lpVtbl->Release(D->lpDDSPrimary);
		D->lpDDSPrimary	=NULL;
	}

	DDRAWDisplay_UnloadDLL(&(D->DLL));
	
	free(D);
	*pD = NULL;
}

static geBoolean DDRAWDisplay_RestoreAll(DDRAWDisplay *D)
{
	HRESULT	ddrval;
	assert( DDRAWDisplay_IsValid(D)!=GE_FALSE );

	ddrval	=D->DLL.lpDD4->lpVtbl->SetCooperativeLevel(D->DLL.lpDD4, D->hWnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
	if(ddrval!=DD_OK)
	{
		geErrorLog_AddString(-1,"DDRAWDisplay_RestoreAll(): ddraw SetCooperativeLevel", geErrorLog_IntToString(ddrval));
		return	GE_FALSE;
	}

	ddrval	=D->DLL.lpDD4->lpVtbl->SetDisplayMode(D->DLL.lpDD4, D->Width, D->Height, D->BitsPerPixel, 0, 0);
	if(ddrval!=DD_OK)
	{
		geErrorLog_AddString(-1,"DDRAWDisplay_RestoreAll: ddraw SetDisplayMode", geErrorLog_IntToString(ddrval));
		return	GE_FALSE;
	}
	ddrval	=D->DLL.lpDD4->lpVtbl->RestoreAllSurfaces(D->DLL.lpDD4);
	if(ddrval!=DD_OK)
	{
		geErrorLog_AddString(-1,"DDRAWDisplay_RestoreAll: ddraw RestoreAllSurfaces", geErrorLog_IntToString(ddrval));
		return	GE_FALSE;
	}
	return	GE_TRUE;
}


geBoolean	DDRAWDisplay_Lock(DDRAWDisplay *D, uint8 **Buffer, int32 *Pitch)
{
	HRESULT			ddrval;
	DDSURFACEDESC2	ddsd;
	RECT			wrect;
	int Forever = 1;
	assert( DDRAWDisplay_IsValid(D)!=GE_FALSE );

	if(!D->bActive)
		return GE_TRUE;
		// <> should probably be GE_FALSE, but this stays compatible with the existing convention

	assert( Buffer != NULL );
	assert( Pitch  != NULL );

	if (D->Locked != GE_FALSE)
		{
			geErrorLog_AddString(-1,"DDRAWDisplay_Lock: already locked",NULL );
			return GE_FALSE;
		}
	
	memset(&ddsd, 0, sizeof(DDSCAPS2));
	ddsd.dwSize	=sizeof(ddsd);
	ddsd.dwFlags=DDSD_HEIGHT | DDSD_WIDTH;

	D->lpDDSBack->lpVtbl->GetSurfaceDesc(D->lpDDSBack, &ddsd);
	wrect.left   = 0;
	wrect.top    = 0;
	wrect.right  = ddsd.dwWidth-1;
	wrect.bottom = ddsd.dwHeight-1;

	while(Forever)
	{
		ddrval	=D->lpDDSBack->lpVtbl->Lock(D->lpDDSBack, &wrect, &ddsd, DDLOCK_SURFACEMEMORYPTR, NULL);
		if(ddrval==DD_OK)
		{
			break;
		}

		if(ddrval==DDERR_SURFACELOST)
		{
			if (DDRAWDisplay_RestoreAll(D) == GE_FALSE) 
			{
				geErrorLog_AddString(-1,"DDRAWDisplay_Lock: lost ddraw surface", NULL);
				return	GE_FALSE;
			}
		}
		else if(ddrval!=DDERR_WASSTILLDRAWING)
		{
			geErrorLog_AddString(-1,"DDRAWDisplay_Lock: was still drawing",geErrorLog_IntToString(ddrval));
			return	GE_FALSE;
		}
	}
	*Buffer	= (uint8 *)ddsd.lpSurface;
	*Pitch  = ddsd.lPitch;
	D->Locked = GE_TRUE;
	D->Buffer = *Buffer;
	D->Pitch  = *Pitch;
	return GE_TRUE;
}

geBoolean	DDRAWDisplay_Unlock(DDRAWDisplay *D)
{
	int Forever=1;
	HRESULT			ddrval	=DD_OK;
	assert( DDRAWDisplay_IsValid(D)!=GE_FALSE );

	if(!D->bActive)
		return GE_TRUE;
		// <> should probably be GE_FALSE, but this stays compatible with the existing convention

	if (D->Locked != GE_TRUE)
		{
			geErrorLog_AddString(-1,"DDRAWDisplay_Unlock: surface not locked",NULL);
			return GE_FALSE;
		}
	
	D->Locked = GE_FALSE;
	
	while(Forever)
	{
		ddrval	=D->lpDDSBack->lpVtbl->Unlock(D->lpDDSBack, NULL);
		if(ddrval==DD_OK)
			break;

		#pragma message (" can you loose a locked surface?")
		if(ddrval==DDERR_SURFACELOST)		
		{
			if (DDRAWDisplay_RestoreAll(D) == GE_FALSE)
			{
				geErrorLog_AddString(-1,"DDRAWDisplay_Unlock: surface lost\n",NULL);
				return	GE_FALSE;
			}
		}
		else if(ddrval!=DDERR_WASSTILLDRAWING)
		{
			geErrorLog_AddString(-1,"DDRAWDisplay_Unlock: was still drawing.",geErrorLog_IntToString(ddrval));
			return	GE_FALSE;
		}
	}

	D->Locked = GE_FALSE;
	return GE_TRUE;
}

geBoolean	DDRAWDisplay_Blit(DDRAWDisplay *D)
{
	HRESULT			ddrval;
	DDSURFACEDESC2	ddsd;
	RECT			rDest, rSrc;
	int				Forever=1;
	assert( DDRAWDisplay_IsValid(D)!=GE_FALSE );

	if(!D->bActive)
	{
		return	GE_TRUE;
	}

	memset(&ddsd, 0, sizeof(DDSCAPS2));
	ddsd.dwSize	=sizeof(ddsd);
	ddsd.dwFlags=DDSD_HEIGHT | DDSD_WIDTH; 

	D->lpDDSBack->lpVtbl->GetSurfaceDesc(D->lpDDSBack, &ddsd);
	rDest.left	=rDest.top	=0;
	rDest.right	=ddsd.dwWidth-1;
	rDest.bottom=ddsd.dwHeight-1;

	memset(&ddsd, 0, sizeof(DDSCAPS2));
	ddsd.dwSize	=sizeof(ddsd);
	ddsd.dwFlags=DDSD_HEIGHT | DDSD_WIDTH;

	D->lpDDSPrimary->lpVtbl->GetSurfaceDesc(D->lpDDSPrimary, &ddsd);
	rSrc.left	=rSrc.top	=0;
	rSrc.right	=ddsd.dwWidth-1;
	rSrc.bottom	=ddsd.dwHeight-1;

	ddrval	=DD_OK;
	if((D->ModeFlags & VIDEO) && (D->ModeFlags & FASTBLT))
	{
		while(Forever)
		{
			ddrval	=D->lpDDSPrimary->lpVtbl->BltFast(D->lpDDSPrimary, 0, 0, D->lpDDSBack, NULL, 0);
			if(ddrval==DD_OK)
			{
				break;
			}

			if(ddrval==DDERR_SURFACELOST)
			{
				if (DDRAWDisplay_RestoreAll(D) == GE_FALSE)
				{
					geErrorLog_AddString(-1,"DDRAWDisplay_Blit: lost surface",NULL);
					return	GE_FALSE;
				}
			}
			else if(ddrval!=DDERR_WASSTILLDRAWING)
			{
				geErrorLog_AddString(-1,"DDRAWDisplay_Blit: was still drawing",NULL);
				return	GE_FALSE;
			}
		}
	}
	else if(D->ModeFlags & FLIP)
	{
		while(Forever)
		{
			ddrval	=D->lpDDSPrimary->lpVtbl->Flip(D->lpDDSPrimary, D->lpDDSBack, DDFLIP_NOVSYNC);
			if(ddrval==DD_OK)
			{
				break;
			}

			if(ddrval==DDERR_SURFACELOST)
			{
				if (DDRAWDisplay_RestoreAll(D)==GE_FALSE)
				{
					geErrorLog_AddString(-1,"DDRAWDisplay_Blit: lost surface",NULL);
					return	GE_FALSE;
				}
			}
			else if(ddrval!=DDERR_WASSTILLDRAWING)
			{
				geErrorLog_AddString(-1,"DDRAWDisplay_Blit: was still drawing",NULL);
				return	GE_FALSE;
			}
		}
	}
	else	//safe blt
	{
		while(Forever)
		{
			ddrval	=D->lpDDSPrimary->lpVtbl->Blt(D->lpDDSPrimary, NULL, D->lpDDSBack, NULL, DDBLT_WAIT, NULL);
			if(ddrval==DD_OK)
				break;

			if(ddrval==DDERR_SURFACELOST)
			{
				if (DDRAWDisplay_RestoreAll(D)==GE_FALSE)
				{
					geErrorLog_AddString(-1,"DDRAWDisplay_Blit: lost surface",NULL);
					return	GE_FALSE;
				}
			}
			else if(ddrval!=DDERR_WASSTILLDRAWING)
			{
				geErrorLog_AddString(-1,"DDRAWDisplay_Blit: was still drawing",NULL);
				return	GE_FALSE;
			}
		}
	}
	return GE_TRUE;
}

//my best guess for best performance is fast, blt, flip
//dma is off for now because it ran horribly on the machine
//I tested it on.  If people get 500mhz agp bus action in a few
//years it might be useful perhaps
static geBoolean DDRAWDisplay_SetMode(DDRAWDisplay *D, int width, int height, int bpp, uint32 flags)
{
	HRESULT			ddrval;
	DDSURFACEDESC2	ddsd;
	DDSCAPS2		ddscaps;

	assert( DDRAWDisplay_IsValid(D)!=GE_FALSE );


	D->ModeFlags = 0;

	ddrval	=D->DLL.lpDD4->lpVtbl->SetDisplayMode(D->DLL.lpDD4, width, height, bpp, 0, 0);
	
	D->Height	=height;
	D->Width	=width;
	D->BitsPerPixel	=bpp;

	if(!(flags & MODEXMODE))
	{

		memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
		ddsd.dwSize			=sizeof(ddsd);
		ddsd.dwFlags		=DDSD_CAPS;
		ddsd.ddsCaps.dwCaps	=DDSCAPS_PRIMARYSURFACE | DDSCAPS_VIDEOMEMORY;

		ddrval	=D->DLL.lpDD4->lpVtbl->CreateSurface(D->DLL.lpDD4, &ddsd, &D->lpDDSPrimary, NULL);
		if(ddrval==DD_OK)
		{

			memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
			ddsd.dwSize		=sizeof(ddsd);
			ddsd.dwFlags	=DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
			ddsd.ddsCaps.dwCaps	=DDSCAPS_SYSTEMMEMORY;
			//ddsd.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY;
			ddsd.dwHeight	=height;
			ddsd.dwWidth	=width;

			ddrval	=D->DLL.lpDD4->lpVtbl->CreateSurface(D->DLL.lpDD4, &ddsd, &D->lpDDSBack, NULL);
			if(ddrval==DD_OK)
			{
				
				//both were created, make sure they are in vidram
				memset(&ddscaps, 0, sizeof(DDSCAPS2));
				D->lpDDSPrimary->lpVtbl->GetCaps(D->lpDDSPrimary, &ddscaps);
				if(ddscaps.dwCaps & DDSCAPS_VIDEOMEMORY)
				{
					memset(&ddscaps, 0, sizeof(DDSCAPS2));
					D->lpDDSBack->lpVtbl->GetCaps(D->lpDDSBack, &ddscaps);
					if(ddscaps.dwCaps & DDSCAPS_VIDEOMEMORY)
					{

						//both are good to go
						D->ModeFlags   |=VIDEO;

						//mark fastblt unless it's stretching
						if(!(flags & STRETCHMODE))
						{
							D->ModeFlags	|=FASTBLT;
						}
					}
				}
			}
		}
			#pragma message ("this cant be set can it?")
		else if(!(D->ModeFlags & VIDEO)
			&& !(D->ModeFlags & STRETCHMODE))
		{
			if(D->lpDDSBack)		D->lpDDSBack->lpVtbl->Release(D->lpDDSBack);
			if(D->lpDDSPrimary)	D->lpDDSPrimary->lpVtbl->Release(D->lpDDSPrimary);

			memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
			ddsd.dwSize			=sizeof(ddsd);
			ddsd.dwFlags		=DDSD_CAPS;
			ddsd.ddsCaps.dwCaps	=DDSCAPS_PRIMARYSURFACE;

			ddrval	=D->DLL.lpDD4->lpVtbl->CreateSurface(D->DLL.lpDD4, &ddsd, &D->lpDDSPrimary, NULL);
			if(ddrval==DD_OK)
			{
				memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
				ddsd.dwSize		=sizeof(ddsd);
				ddsd.dwFlags	=DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
				ddsd.dwHeight	=height;
				ddsd.dwWidth	=width;

				ddrval	=D->DLL.lpDD4->lpVtbl->CreateSurface(D->DLL.lpDD4, &ddsd, &D->lpDDSBack, NULL);
				if(ddrval==DD_OK)
				{
					//both were created good enough
					D->ModeFlags	|=SYSTEM|SAFEBLT;
				}
			}
		}
	}	//flip 
	if((flags & MODEXMODE)	|| D->lpDDSBack==NULL)
	{
		if(D->lpDDSBack)		D->lpDDSBack->lpVtbl->Release(D->lpDDSBack);
		if(D->lpDDSPrimary)	D->lpDDSPrimary->lpVtbl->Release(D->lpDDSPrimary);

		//try flip
		memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
		ddsd.dwSize				=sizeof(ddsd);
		ddsd.dwFlags			=DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
		ddsd.ddsCaps.dwCaps		=DDSCAPS_PRIMARYSURFACE |
									DDSCAPS_COMPLEX | DDSCAPS_FLIP;
		ddsd.dwBackBufferCount	=1;

		ddrval	=D->DLL.lpDD4->lpVtbl->CreateSurface(D->DLL.lpDD4, &ddsd, &D->lpDDSPrimary, NULL);
		if(ddrval==DD_OK)
		{
			memset(&ddscaps, 0, sizeof(DDSCAPS2));
			ddscaps.dwCaps	=DDSCAPS_BACKBUFFER;
			ddrval	=D->lpDDSPrimary->lpVtbl->GetAttachedSurface(D->lpDDSPrimary, &ddscaps, &D->lpDDSBack);
			if(ddrval==DD_OK)
			{
				D->ModeFlags	|=SYSTEM|FLIP;
			}
			else
			{
				geErrorLog_AddString(-1,"Unable to create primary buffer",NULL);
				return GE_FALSE;
			}
		}		
	}
	return GE_TRUE;
}


	 

DDRAWDisplay *DDRAWDisplay_Create(HWND hwnd, int Width, int Height, int BPP, uint32 Flags)
{
	DDRAWDisplay *D = NULL;
	
	HRESULT				ddrval     = 0;

	D = malloc(sizeof(DDRAWDisplay));
	if (D == NULL)
		{
			geErrorLog_AddString(-1,"Failed to create DDRAWDisplay object",NULL);
			return NULL;
		}

	memset(D,0,sizeof(DDRAWDisplay));


	if (DDRAWDisplay_LoadDLL(&(D->DLL))==GE_FALSE)
		{
			geErrorLog_AddString(-1,"failed to load ddraw dll",NULL);
			goto Create_ERROR;
		}
	D->hWnd = hwnd;
	D->bActive = GE_FALSE;
		
	ddrval	=D->DLL.lpDD4->lpVtbl->SetCooperativeLevel(D->DLL.lpDD4, hwnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
	if(ddrval!=DD_OK)
	{
		geErrorLog_AddString(-1,"DDRAWDisplay_Create: failed to set cooperative level", geErrorLog_IntToString(ddrval));
		goto Create_ERROR;
	}
	
	if (DDRAWDisplay_SetMode(D, Width, Height, BPP, Flags)==GE_FALSE)
		{
			geErrorLog_AddString(-1,"Failed to create buffers",NULL);
			goto Create_ERROR;
		}
	
	assert( DDRAWDisplay_IsValid(D)!=GE_FALSE );
	return D;	

	Create_ERROR:
	if (D)
		{
			DDRAWDisplay_UnloadDLL(&(D->DLL));
			free(D);
		}
	return NULL;
}




geBoolean	DDRAWDisplay_GetPixelFormat(	DDRAWDisplay *D,
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
	DDPIXELFORMAT	ddpf;
	uint32			i, j;

	assert( DDRAWDisplay_IsValid(D)!=GE_FALSE );
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

	ddpf.dwSize	=sizeof(ddpf);
	D->lpDDSPrimary->lpVtbl->GetPixelFormat(D->lpDDSPrimary, &ddpf);

	if(!(ddpf.dwFlags & DDPF_RGB))
	{
		return GE_FALSE;
	}
	*bytes_per_pixel	=ddpf.dwRGBBitCount / 8;
	*R_mask		=ddpf.dwRBitMask;
	*G_mask		=ddpf.dwGBitMask;
	*B_mask		=ddpf.dwBBitMask;

	for(j=0,i=ddpf.dwRBitMask;!(i & 1);i>>=1,j++);
	*R_shift	=j;

	for(j=0,i=ddpf.dwGBitMask;!(i & 1);i>>=1,j++);
	*G_shift	=j;

	for(j=0,i=ddpf.dwBBitMask;!(i & 1);i>>=1,j++);
	*B_shift	=j;

	for(i=(ddpf.dwRBitMask>>*R_shift),*R_width=0;i;i >>= 1, (*R_width)++);
	for(i=(ddpf.dwGBitMask>>*G_shift),*G_width=0;i;i >>= 1, (*G_width)++);
	for(i=(ddpf.dwBBitMask>>*B_shift),*B_width=0;i;i >>= 1, (*B_width)++);
	return GE_TRUE;
}


geBoolean	DDRAWDisplay_Wipe(DDRAWDisplay *D,uint32 color)
{
	DDSURFACEDESC2	ddsd;
	int				Width, Height;

	assert( DDRAWDisplay_IsValid(D)!=GE_FALSE );

	if(!D->bActive)
	{
		return GE_FALSE;
	}

	if (!D->Locked)
		{
			geErrorLog_AddString(-1,"DDRAWDisplay_Wipe: surface not locked",NULL );
			return GE_FALSE;
		}
	memset(&ddsd, 0, sizeof(DDSCAPS2));
	ddsd.dwSize	=sizeof(ddsd);
	ddsd.dwFlags=DDSD_HEIGHT | DDSD_WIDTH;
	#pragma message ("this may not be necessary")
	D->lpDDSBack->lpVtbl->GetSurfaceDesc(D->lpDDSBack, &ddsd);

	Width	=ddsd.dwWidth;
	Height	=ddsd.dwHeight;

	memset(D->Buffer, color, (Height*D->Pitch));
	return GE_TRUE;
}

geBoolean DDRAWDisplay_SetActive(DDRAWDisplay *D, geBoolean wParam)
{
	assert( D != NULL );

	D->bActive	=wParam;

	if(D->bActive)
	{	
		if(D->lpDDSPrimary->lpVtbl->IsLost(D->lpDDSPrimary)==DDERR_SURFACELOST)
		{
			if(DDRAWDisplay_RestoreAll(D)!=GE_FALSE)
			{
				ShowWindow(D->hWnd, SW_SHOWNORMAL);	//dx doesn't restore it
			}
			else
			{
				geErrorLog_AddString(-1,"DDRAWDisplay_SetActive: Couldn't restore surfaces",NULL);
				return GE_FALSE;
			}
		}
	}
	return	GE_TRUE;
}

// -----------------------------------------------------------


static HRESULT WINAPI DDRAWDisplay_ModeCallback(LPDDSURFACEDESC2 pdds, LPVOID lParam)
{
	DisplayModeInfo *Info =(DisplayModeInfo *)lParam;

	#pragma message ("only 16 bit display is supported")
	if(pdds->ddpfPixelFormat.dwRGBBitCount==16)		
	{
		DisplayModeInfo_AddEntry(Info,
				pdds->dwWidth,
				pdds->dwHeight,
				pdds->ddpfPixelFormat.dwRGBBitCount,
				pdds->ddpfPixelFormat.dwRGBBitCount);
	}
	return	S_FALSE;
}

geBoolean DDRAWDisplay_GetDescriptionString(char *DescriptionString, unsigned int DescriptionStringMaxLength)
{
	DDRAWDisplay_DLLHooks DLL={NULL,0};

	DDDEVICEIDENTIFIER	DDDeviceIdentifier;
	HRESULT				hRet;

	assert( DescriptionString != NULL );
	
	if (DDRAWDisplay_LoadDLL(&DLL)==GE_FALSE)
		{	
			geErrorLog_AddString(-1,"unable to load ddraw dll",NULL);
			goto GetDescriptionString_ERROR;
		}
	
	memset(&DDDeviceIdentifier, 0, sizeof(DDDeviceIdentifier));

	hRet	=DLL.lpDD4->lpVtbl->GetDeviceIdentifier(DLL.lpDD4, &DDDeviceIdentifier,0);
	
	if(hRet != DD_OK)
	{
		geErrorLog_AddString(-1,"DDRAWDisplay_GetDescriptionString: ddraw GetDeviceIdentifier() failed", NULL);
		goto GetDescriptionString_ERROR;
	}

	DDRAWDisplay_UnloadDLL(&DLL);		
	if (strlen(DDDeviceIdentifier.szDescription) + strlen(DDRAWDISPLAY_DESCRIPTION_STRING) >= DescriptionStringMaxLength)
		{
			geErrorLog_AddString(-1,"DDRAWDisplay_GetDescriptionString: description string too short",NULL);
			goto GetDescriptionString_ERROR;
		}

	strcpy(DescriptionString,DDRAWDISPLAY_DESCRIPTION_STRING);
	//strcat(DescriptionString,DDDeviceIdentifier.szDescription);

	return	GE_TRUE;

	//------
	GetDescriptionString_ERROR:
		DDRAWDisplay_UnloadDLL(&DLL);
		return GE_FALSE;
}


geBoolean DDRAWDisplay_GetDisplayInfo(	char			*DescriptionString, 
										unsigned int	 DescriptionStringMaxLength,
										DisplayModeInfo *Info)
{
	DDDEVICEIDENTIFIER	DDDeviceIdentifier;
	DDRAWDisplay_DLLHooks DLL={NULL,0};
	HRESULT				ddrval = 0;
	
	assert( Info != NULL );

	if (DDRAWDisplay_LoadDLL( &DLL )==GE_FALSE)
		{
			geErrorLog_AddString(-1,"failed to load DDRAW dll",NULL);
			return GE_FALSE;
		}
	
	#if 0
		{
			//test for general dma support
			DDCAPS				ddcaps;
			memset(&ddcaps, 0, sizeof(DDCAPS));
			ddcaps.dwSize	=sizeof(ddcaps);
			DLL.lpDD4->lpVtbl->GetCaps(DLL.lpDD4, &ddcaps, NULL);

			if(ddcaps.dwCaps & DDCAPS_CANBLTSYSMEM)
			{
				//System to video blits supported
				if(ddcaps.dwSVBCaps & DDCAPS_BLTQUEUE)
				{
					D->bDMA	=TRUE;	//	DMA Asynch System to Video blits supported
				}
			}
		}
	#endif


	#if 0		// need this?
		ddrval	=DLL.lpDD4->lpVtbl->SetCooperativeLevel(DLL.lpDD4, ActiveWnd,
			DDSCL_ALLOWMODEX | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
		if(ddrval != DD_OK)
			{
				geErrorLog_Add(-1,"Failed to set Cooperative Level",NULL);
				goto GetDisplayInfo_ERROR;
			}
	#endif
	
	memset(&DDDeviceIdentifier, 0, sizeof(DDDeviceIdentifier));

	ddrval	=DLL.lpDD4->lpVtbl->GetDeviceIdentifier(DLL.lpDD4, &DDDeviceIdentifier,0);
	
	if(ddrval != DD_OK)
	{
		geErrorLog_AddString(-1,"ddraw GetDeviceIdentifier() failed", NULL);
		goto GetDisplayInfo_ERROR;
	}

	if (strlen(DDDeviceIdentifier.szDescription) + strlen(DDRAWDISPLAY_DESCRIPTION_STRING) >= DescriptionStringMaxLength)
		{
			geErrorLog_AddString(-1,"description string too short",NULL);
			goto GetDisplayInfo_ERROR;
		}

	strcpy(DescriptionString,DDRAWDISPLAY_DESCRIPTION_STRING);
//	strcat(DescriptionString,DDDeviceIdentifier.szDescription);


	DLL.lpDD4->lpVtbl->EnumDisplayModes(DLL.lpDD4, 0, NULL, (LPVOID)Info, DDRAWDisplay_ModeCallback);

	DDRAWDisplay_UnloadDLL(&DLL);
	return		GE_TRUE;	

	GetDisplayInfo_ERROR:
		DDRAWDisplay_UnloadDLL(&DLL);
		return GE_FALSE;

}


