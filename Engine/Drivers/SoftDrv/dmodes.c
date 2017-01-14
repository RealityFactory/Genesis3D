/****************************************************************************************/
/*  dmodes.c                                                                            */
/*                                                                                      */
/*  Author:       Ken Baird                                                             */
/*  Description:  Directdraw related stuff                                              */
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
#define INITGUID
#include <windows.h>
#include "softdrv.h"
#include "ddraw.h"
#include <stdio.h>

LPDIRECTDRAW			lpDD;			// DirectDraw object
LPDIRECTDRAW4			lpDD4;
LPDIRECTDRAWSURFACE4	lpDDSPrimary;	// DirectDraw primary surface
LPDIRECTDRAWSURFACE4	lpDDSBack;		// DirectDraw back surface
BOOL					bActive		=TRUE;
BOOL					bInitDone	=FALSE;
BOOL					bWindowed	=FALSE;
HWND					mhWnd		=NULL;
static BOOL				bDMA		=FALSE;
static BOOL				bHardBlt	=FALSE;
static BOOL				bDMAPageLock=FALSE;
BOOL					bBackLocked	=FALSE;
#ifdef STRICT
WNDPROC					pOldWndProc;
#else
FARPROC					pOldWndProc;
#endif
HINSTANCE				ddrawinst;

typedef HRESULT (WINAPI *LPDIRECTDRAWCREATE)( GUID FAR *lpGUID, LPDIRECTDRAW FAR *lplpDD, IUnknown FAR *pUnkOuter );

extern	int	NumDevices;
typedef struct DecalDelayTag
{
	LPDIRECTDRAWSURFACE4	surf;
	RECT					srect;
	S32						x;
	S32						y;
}	DecalDelay;

static	DecalDelay	DecalQ[8192];
static	int			NumDecalsQd;
char* MyErrorToString(HRESULT error);

//these statics must be eliminated soon
static	int CurrentModeWidth, CurrentModeHeight, CurrentModeDepth;


BOOL	WINAPI	DDrawDriverEnumCallbackEx(GUID *pGUID, LPSTR pDescription, LPSTR pName, LPVOID pContext, HMONITOR hm)
{
	LPDIRECTDRAWCREATE	lpDDCreate;
	LPDIRECTDRAW		pDD;
	LPDIRECTDRAW4		pDD4	=NULL;
	HRESULT				hRet;
	VidEnumInfo			*vinfo	=&((VidEnumInfo *)pContext)[NumDevices];
	
	ddrawinst	=LoadLibrary("ddraw.dll");

	if(!ddrawinst)
	{
		return	DDENUMRET_CANCEL;
	}

	lpDDCreate	=(LPDIRECTDRAWCREATE)GetProcAddress(ddrawinst, "DirectDrawCreate");
	if(lpDDCreate)
	{
		hRet	=lpDDCreate(NULL, &pDD, NULL);
	}
	else
	{
		return	DDENUMRET_CANCEL;
	}
	vinfo->DeviceGuid	=pGUID;
	
	if(hRet != DD_OK)
	{
		ErrorPrintf("DirectDrawCreate FAILED : DDrawDriverEnumCallbackEx\n");
		return	DDENUMRET_CANCEL;
	}

	hRet	=pDD->lpVtbl->QueryInterface(pDD, &IID_IDirectDraw4, (LPVOID *)&pDD4);
	if(hRet != DD_OK)
	{
		ErrorPrintf("QueryInterface FAILED : DDrawDriverEnumCallbackEx\n");
		return	DDENUMRET_CANCEL;
	}
	
	hRet	=pDD4->lpVtbl->GetDeviceIdentifier(pDD4, &vinfo->DeviceInfo,0);
	hRet	=pDD4->lpVtbl->GetDeviceIdentifier(pDD4, &vinfo->DeviceInfoHost, DDGDI_GETHOSTIDENTIFIER);
	
	if(pDD4)
	{
		pDD4->lpVtbl->Release(pDD4);
	}

	FreeLibrary(ddrawinst);
	
	if(NumDevices < 16)
	{
		NumDevices++;
	}
	else
	{
		return	DDENUMRET_CANCEL;
	}
	return	DDENUMRET_OK;
}

BOOL	WINAPI	OldDDrawDriverEnumCallback(GUID *pGUID, LPSTR pDescription, LPSTR pName, LPVOID context)
{
	return	(DDrawDriverEnumCallbackEx(pGUID, pDescription, pName, context, NULL));
}

HRESULT WINAPI ModeCallback(LPDDSURFACEDESC2 pdds, LPVOID lParam)
{
	VidEnumInfo	*vinfo	=(VidEnumInfo *)lParam;

	if(pdds->ddpfPixelFormat.dwRGBBitCount==vinfo->bpp)
	{
		vinfo->VidModes[vinfo->NumVidModes].width	=pdds->dwWidth;
		vinfo->VidModes[vinfo->NumVidModes].flags	=0;
		vinfo->VidModes[vinfo->NumVidModes].height	=pdds->dwHeight;
		vinfo->VidModes[vinfo->NumVidModes++].bpp	=pdds->ddpfPixelFormat.dwRGBBitCount;
	}
	
	return	S_FALSE;
}

void	FreeDDraw(VidEnumInfo *vinfo)
{
	int	x	=0;

	if(lpDD4)
	{
		if(vinfo->VidModes[vinfo->CurrentVidMode].current & FLIP)
		{
			lpDDSBack	=NULL;
		}
		else
		{
			if(lpDDSBack)
			{
				lpDDSBack->lpVtbl->Release(lpDDSBack);
				lpDDSBack	=NULL;
			}
		}
		if(lpDDSPrimary)
		{
			lpDDSPrimary->lpVtbl->Release(lpDDSPrimary);
			lpDDSPrimary	=NULL;
		}
		lpDD4->lpVtbl->Release(lpDD4);
		lpDD4	=NULL;
	}
	FreeLibrary(ddrawinst);
}

HRESULT	RestoreAll(void)
{
	HRESULT	ddrval;

	ddrval	=lpDD4->lpVtbl->SetCooperativeLevel(lpDD4, mhWnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
	if(ddrval!=DD_OK)
	{
		ErrorPrintf("Error During SetCooperativeLevel() in RestoreAll()\n%s\n", MyErrorToString(ddrval));
		return	FALSE;
	}

	ddrval	=lpDD4->lpVtbl->SetDisplayMode(lpDD4, CurrentModeWidth, CurrentModeHeight, CurrentModeDepth, 0, 0);
	if(ddrval!=DD_OK)
	{
		ErrorPrintf("Error During SetDisplayMode() in RestoreAll()\n%s\n", MyErrorToString(ddrval));
		return	FALSE;
	}
	ddrval	=lpDD4->lpVtbl->RestoreAllSurfaces(lpDD4);
	if(ddrval!=DD_OK)
	{
		ErrorPrintf("Error During RestoreAllSurfaces() in RestoreAll()\n%s\n", MyErrorToString(ddrval));
		return	FALSE;
	}
	return	ddrval;
}

void	SetDDrawWindow(HWND hwnd)
{
	mhWnd		=hwnd;
}

BOOL	LockDDrawBackBuffer(DRV_Window *cwnd, RECT *wrect)
{
	HRESULT			ddrval;
	DDSURFACEDESC2	ddsd;

	if(!bActive)
		return TRUE;

	memset(&ddsd, 0, sizeof(DDSCAPS2));
	ddsd.dwSize	=sizeof(ddsd);
	ddsd.dwFlags=DDSD_HEIGHT | DDSD_WIDTH;

	lpDDSBack->lpVtbl->GetSurfaceDesc(lpDDSBack, &ddsd);
	if(((int)ddsd.dwWidth < wrect->right) || ((int)ddsd.dwHeight < wrect->bottom))
	{
		ErrorPrintf("ERROR:  World render rect passed by the engine is larger than the drawing surface.\n");
		ErrorPrintf("WorldRect: %d, %d, %d, %d\n", wrect->left, wrect->top, wrect->right, wrect->bottom);
		ErrorPrintf("Surface:  %d, %d\n", ddsd.dwWidth, ddsd.dwHeight);
		return	FALSE;
	}

	while(1)
	{
		ddrval	=lpDDSBack->lpVtbl->Lock(lpDDSBack, wrect, &ddsd, DDLOCK_SURFACEMEMORYPTR, NULL);
		if(ddrval==DD_OK)
		{
			break;
		}

		if(ddrval==DDERR_SURFACELOST)
		{
			ddrval	=RestoreAll();
			if(ddrval!=DD_OK )
			{
				ErrorPrintf("Error During Lock() in LockDDrawBackBuffer()...\n");
				ErrorPrintf("DirectDraw Surfaces were lost and could not be restored : %s\n", MyErrorToString(ddrval));
				return	FALSE;
			}
		}
		else if(ddrval!=DDERR_WASSTILLDRAWING)
		{
			ErrorPrintf("Error During Lock() in LockDDrawBackBuffer() : %s\n", MyErrorToString(ddrval));
			return	FALSE;
		}
		SleepEx(5, FALSE);
	}
	cwnd->Buffer	=(U8 *)ddsd.lpSurface;
	cwnd->PixelPitch=ddsd.lPitch;
	bBackLocked		=TRUE;
	return TRUE;
}

BOOL	UnlockDDrawBackBuffer(DRV_Window *cwnd, RECT *wrect)
{
	HRESULT			ddrval	=DD_OK;

	if(!bActive)
		return TRUE;

	while(1)
	{
		ddrval	=lpDDSBack->lpVtbl->Unlock(lpDDSBack, NULL);
		if(ddrval==DD_OK)
			break;

		if(ddrval==DDERR_SURFACELOST)
		{
			ddrval	=RestoreAll();
			if(ddrval!=DD_OK )
			{
				ErrorPrintf("Error During Unlock() in UnlockDDrawBackBuffer()...\n");
				ErrorPrintf("DirectDraw Surfaces were lost and could not be restored : %s\n", MyErrorToString(ddrval));
				return	FALSE;
			}
		}
		else if(ddrval!=DDERR_WASSTILLDRAWING)
		{
			ErrorPrintf("Error During Unlock() in UnlockDDrawBackBuffer() : %s\n", MyErrorToString(ddrval));
			return	FALSE;
		}
		SleepEx(5, FALSE);
	}
	bBackLocked	=FALSE;
	return TRUE;
}

BOOL	RefreshDDraw(DRV_Window *cwnd, VidModeList *cmode, RECT *src, RECT *dst)
{
	HRESULT			ddrval;
	DDSURFACEDESC2	ddsd;
//	int				i;
	RECT			rDest, rSrc;

	if(!bActive)
	{
		return	TRUE;
	}

	memset(&ddsd, 0, sizeof(DDSCAPS2));
	ddsd.dwSize	=sizeof(ddsd);
	ddsd.dwFlags=DDSD_HEIGHT | DDSD_WIDTH;

	lpDDSBack->lpVtbl->GetSurfaceDesc(lpDDSBack, &ddsd);
	rDest.left	=rDest.top	=0;
	rDest.right	=ddsd.dwWidth-1;
	rDest.bottom=ddsd.dwHeight-1;

	memset(&ddsd, 0, sizeof(DDSCAPS2));
	ddsd.dwSize	=sizeof(ddsd);
	ddsd.dwFlags=DDSD_HEIGHT | DDSD_WIDTH;

	lpDDSPrimary->lpVtbl->GetSurfaceDesc(lpDDSPrimary, &ddsd);
	rSrc.left	=rSrc.top	=0;
	rSrc.right	=ddsd.dwWidth-1;
	rSrc.bottom	=ddsd.dwHeight-1;

	ddrval	=DD_OK;
	if((cmode->current & VIDEO) && (cmode->current & FASTBLT))
	{
		while(1)
		{
			ddrval	=lpDDSPrimary->lpVtbl->BltFast(lpDDSPrimary, 0, 0, lpDDSBack, NULL, 0);
			if(ddrval==DD_OK)
			{
				break;
			}

			if(ddrval==DDERR_SURFACELOST)
			{
				ddrval	=RestoreAll();
				if(ddrval!=DD_OK)
				{
					ErrorPrintf("Error During BltFast() in RefreshDDraw()...\n");
					ErrorPrintf("DirectDraw Surfaces were lost and could not be restored : %s\n", MyErrorToString(ddrval));
					return	FALSE;
				}
			}
			else if(ddrval!=DDERR_WASSTILLDRAWING)
			{
				ErrorPrintf("Error During BltFast() in RefreshDDraw() : %s\n", MyErrorToString(ddrval));
				return	FALSE;
			}
			SleepEx(5, FALSE);
		}
	}
	else if(cmode->current & FLIP)
	{
		while(1)
		{
			ddrval	=lpDDSPrimary->lpVtbl->Flip(lpDDSPrimary, lpDDSBack, DDFLIP_NOVSYNC);
			if(ddrval==DD_OK)
			{
				break;
			}

			if(ddrval==DDERR_SURFACELOST)
			{
				ddrval	=RestoreAll();
				if(ddrval!=DD_OK)
				{
					ErrorPrintf("Error During Flip() in RefreshDDraw()...\n");
					ErrorPrintf("DirectDraw Surfaces were lost and could not be restored : %s\n", MyErrorToString(ddrval));
					return	FALSE;
				}
			}
			else if(ddrval!=DDERR_WASSTILLDRAWING)
			{
				ErrorPrintf("Error During Flip() in RefreshDDraw() : %s\n", MyErrorToString(ddrval));
				return	FALSE;
			}
			SleepEx(5, FALSE);
		}
	}
	else	//safe blt
	{
		if(cmode->current & STRETCHMODE)
		{
			while(1)
			{
				ddrval	=lpDDSPrimary->lpVtbl->Blt(lpDDSPrimary, NULL, lpDDSBack, NULL, 0, NULL);
				if(ddrval==DD_OK)
					break;

				if(ddrval==DDERR_SURFACELOST)
				{
					ddrval	=RestoreAll();
					if(ddrval!=DD_OK)
					{
						ErrorPrintf("Error During StretchMode Blt() in RefreshDDraw()...\n");
						ErrorPrintf("DirectDraw Surfaces were lost and could not be restored : %s\n", MyErrorToString(ddrval));
						return	FALSE;
					}
				}
				else if(ddrval!=DDERR_WASSTILLDRAWING)
				{
					ErrorPrintf("Error During StretchMode Blt() in RefreshDDraw() : %s\n", MyErrorToString(ddrval));
					return	FALSE;
				}
				SleepEx(5, FALSE);
			}
		}
		else
		{
			while(1)
			{
				ddrval	=lpDDSPrimary->lpVtbl->Blt(lpDDSPrimary, NULL, lpDDSBack, NULL, 0, NULL);
				if(ddrval==DD_OK)
					break;

				if(ddrval==DDERR_SURFACELOST)
				{
					ddrval	=RestoreAll();
					if(ddrval!=DD_OK)
					{
						ErrorPrintf("Error During Blt() in RefreshDDraw()...\n");
						ErrorPrintf("DirectDraw Surfaces were lost and could not be restored : %s\n", MyErrorToString(ddrval));
						return	FALSE;
					}
				}
				else if(ddrval!=DDERR_WASSTILLDRAWING)
				{
					ErrorPrintf("Error During Blt() in RefreshDDraw() : %s\n", MyErrorToString(ddrval));
					return	FALSE;
				}
				SleepEx(5, FALSE);
			}
		}
	}
/*	for(i=0;i < NumDecalsQd;i++)
	{
		if(DecalQ[i].srect.left == -1)
		{
			DDrawBlitDecalToFront(DecalQ[i].surf, NULL, DecalQ[i].x, DecalQ[i].y); 
		}
		else
		{
			DDrawBlitDecalToFront(DecalQ[i].surf, &DecalQ[i].srect, DecalQ[i].x, DecalQ[i].y); 
		}
	}
	NumDecalsQd	=0;*/
	return TRUE;
}

//my best guess for best performance is fast, blt, flip
//dma is off for now because it ran horribly on the machine
//I tested it on.  If people get 500mhz agp bus action in a few
//years it might be useful perhaps
geBoolean	SetDDrawMode(U32 top, VidEnumInfo *vinfo)
{
	HRESULT			ddrval;
	DDSURFACEDESC2	ddsd;
	DDSCAPS2		ddscaps;

	if(vinfo->VidModes[top].flags & STRETCHMODE)
	{
		if((vinfo->VidModes[top].width > 640)
			||((vinfo->VidModes[top].width == 640)
			&&(vinfo->VidModes[top].height > 480)))
		{
			ddrval	=lpDD4->lpVtbl->SetDisplayMode(lpDD4, vinfo->VidModes[top].width, vinfo->VidModes[top].height, vinfo->VidModes[top].bpp, 0, 0);
		}
		else
		{
			ddrval	=lpDD4->lpVtbl->SetDisplayMode(lpDD4, 640, 480, vinfo->VidModes[top].bpp, 0, 0);
		}
		vinfo->VidModes[top].current	|=STRETCHMODE;
	}
	else
	{
		ddrval	=lpDD4->lpVtbl->SetDisplayMode(lpDD4, vinfo->VidModes[top].width, vinfo->VidModes[top].height, vinfo->VidModes[top].bpp, 0, 0);
	}
	if(ddrval != DD_OK)
	{
		return	GE_FALSE;
	}
	CurrentModeHeight	=vinfo->VidModes[top].height;
	CurrentModeWidth	=vinfo->VidModes[top].width;
	CurrentModeDepth	=vinfo->VidModes[top].bpp;

	if(!(vinfo->VidModes[top].flags & MODEXMODE))
	{
		memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
		ddsd.dwSize			=sizeof(ddsd);
		ddsd.dwFlags		=DDSD_CAPS;
		ddsd.ddsCaps.dwCaps	=DDSCAPS_PRIMARYSURFACE | DDSCAPS_VIDEOMEMORY;

		ddrval	=lpDD4->lpVtbl->CreateSurface(lpDD4, &ddsd, &lpDDSPrimary, NULL);
		if(ddrval==DD_OK)
		{
			memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
			ddsd.dwSize		=sizeof(ddsd);
			ddsd.dwFlags	=DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
			ddsd.ddsCaps.dwCaps	=DDSCAPS_SYSTEMMEMORY;
			if(vinfo->VidModes[top].flags & STRETCHMODE)
			{
				if((vinfo->VidModes[top].width > 640)
					||((vinfo->VidModes[top].width == 640)
					&&(vinfo->VidModes[top].height > 480)))
				{
					ddsd.dwHeight	=640;
					ddsd.dwWidth	=480;
				}
				else
				{
					ddsd.dwHeight	=vinfo->VidModes[top].height;
					ddsd.dwWidth	=vinfo->VidModes[top].width;
				}
			}
			else
			{
				ddsd.dwHeight	=vinfo->VidModes[top].height;
				ddsd.dwWidth	=vinfo->VidModes[top].width;
			}

			ddrval	=lpDD4->lpVtbl->CreateSurface(lpDD4, &ddsd, &lpDDSBack, NULL);
			if(ddrval==DD_OK)
			{
				//need to grab pitch here since it can change
				//depending on where the surface is created
				memset(&ddsd, 0, sizeof(DDSCAPS2));
				ddsd.dwSize	=sizeof(ddsd);
				ddsd.dwFlags=DDSD_HEIGHT | DDSD_WIDTH;
				lpDDSBack->lpVtbl->GetSurfaceDesc(lpDDSBack, &ddsd);
				vinfo->VidModes[top].pitch	=ddsd.lPitch;

				//both were created, make sure they are in vidram
				memset(&ddscaps, 0, sizeof(DDSCAPS2));
				lpDDSPrimary->lpVtbl->GetCaps(lpDDSPrimary, &ddscaps);
				if(ddscaps.dwCaps & DDSCAPS_VIDEOMEMORY)
				{
					memset(&ddscaps, 0, sizeof(DDSCAPS2));
					lpDDSBack->lpVtbl->GetCaps(lpDDSBack, &ddscaps);
					if(ddscaps.dwCaps & DDSCAPS_VIDEOMEMORY)
					{
						//both are good to go
						vinfo->VidModes[top].current	|=VIDEO;

						//mark fastblt unless it's stretching
						if(!(vinfo->VidModes[top].flags & STRETCHMODE))
						{
							vinfo->VidModes[top].current	|=FASTBLT;
						}
					}
				}
			}
		}
		else if(!(vinfo->VidModes[top].current & VIDEO)
			&& !(vinfo->VidModes[top].current & STRETCHMODE))
		{
			if(lpDDSBack)		lpDDSBack->lpVtbl->Release(lpDDSBack);
			if(lpDDSPrimary)	lpDDSPrimary->lpVtbl->Release(lpDDSPrimary);

			memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
			ddsd.dwSize			=sizeof(ddsd);
			ddsd.dwFlags		=DDSD_CAPS;
			ddsd.ddsCaps.dwCaps	=DDSCAPS_PRIMARYSURFACE;

			ddrval	=lpDD4->lpVtbl->CreateSurface(lpDD4, &ddsd, &lpDDSPrimary, NULL);
			if(ddrval==DD_OK)
			{
				memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
				ddsd.dwSize		=sizeof(ddsd);
				ddsd.dwFlags	=DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
				ddsd.dwHeight	=vinfo->VidModes[top].height;
				ddsd.dwWidth	=vinfo->VidModes[top].width;

				ddrval	=lpDD4->lpVtbl->CreateSurface(lpDD4, &ddsd, &lpDDSBack, NULL);
				if(ddrval==DD_OK)
				{
					//need to grab pitch here since it can change
					//depending on where the surface is created
					memset(&ddsd, 0, sizeof(DDSCAPS2));
					ddsd.dwSize	=sizeof(ddsd);
					ddsd.dwFlags=DDSD_HEIGHT | DDSD_WIDTH;
					lpDDSBack->lpVtbl->GetSurfaceDesc(lpDDSBack, &ddsd);
					vinfo->VidModes[top].pitch	=ddsd.lPitch;

					//both were created good enough
					vinfo->VidModes[top].current	|=SYSTEM|SAFEBLT;
				}
			}
		}
	}	//flip 
	if((vinfo->VidModes[top].flags & MODEXMODE)	|| lpDDSBack==NULL)
	{
		if(lpDDSBack)		lpDDSBack->lpVtbl->Release(lpDDSBack);
		if(lpDDSPrimary)	lpDDSPrimary->lpVtbl->Release(lpDDSPrimary);

		//try flip
		memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
		ddsd.dwSize				=sizeof(ddsd);
		ddsd.dwFlags			=DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
		ddsd.ddsCaps.dwCaps		=DDSCAPS_PRIMARYSURFACE |
									DDSCAPS_COMPLEX | DDSCAPS_FLIP;
		ddsd.dwBackBufferCount	=1;

		ddrval	=lpDD4->lpVtbl->CreateSurface(lpDD4, &ddsd, &lpDDSPrimary, NULL);
		if(ddrval==DD_OK)
		{
			memset(&ddscaps, 0, sizeof(DDSCAPS2));
			ddscaps.dwCaps	=DDSCAPS_BACKBUFFER;
			ddrval	=lpDDSPrimary->lpVtbl->GetAttachedSurface(lpDDSPrimary, &ddscaps, &lpDDSBack);
			if(ddrval==DD_OK)
			{
				//need to grab pitch here since it can change
				//depending on where the surface is created
				memset(&ddsd, 0, sizeof(DDSCAPS2));
				ddsd.dwSize	=sizeof(ddsd);
				ddsd.dwFlags=DDSD_HEIGHT | DDSD_WIDTH;
				lpDDSBack->lpVtbl->GetSurfaceDesc(lpDDSBack, &ddsd);
				vinfo->VidModes[top].pitch	=ddsd.lPitch;

				vinfo->VidModes[top].current	|=SYSTEM|FLIP;
			}
		}
	}

	return	GE_TRUE;
}

//sort the passed in list of video modes, returning a sorted
//reliable list of resolutions and capabilities
//capabilities are tested for each mode and surface type
//to make a more reliable list than getcaps() can provide
void SortDDrawVideoModeList(VidEnumInfo *vinfo)
{
	int				top, search;
	VidModeList		temp;
	DDSURFACEDESC2	ddsd;
	DDSCAPS2		ddscaps;
	HRESULT			ddrval;

	for(top=0;top < vinfo->NumVidModes;top++)
	{
		if(vinfo->VidModes[top].flags & MODEXMODE)
		{
			//modex can only flip I guess
			vinfo->VidModes[top].flags	|=(SYSTEM | FLIP);
		}
		else
		{
			vinfo->VidModes[top].flags	|=SAFEBLT;	//everyone has this but X
			ddrval	=lpDD4->lpVtbl->SetDisplayMode(lpDD4, vinfo->VidModes[top].width, vinfo->VidModes[top].height, vinfo->VidModes[top].bpp, 0, 0);

			//check fastblt, front and back
			memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
			ddsd.dwSize			=sizeof(ddsd);
			ddsd.dwFlags		=DDSD_CAPS;
			ddsd.ddsCaps.dwCaps	=DDSCAPS_PRIMARYSURFACE | DDSCAPS_VIDEOMEMORY;

			ddrval	=lpDD4->lpVtbl->CreateSurface(lpDD4, &ddsd, &lpDDSPrimary, NULL);
			if(ddrval==DD_OK)
			{
				memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
				ddsd.dwSize		=sizeof(ddsd);
				ddsd.dwFlags	=DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
				ddsd.dwHeight	=vinfo->VidModes[top].height;
				ddsd.dwWidth	=vinfo->VidModes[top].width;

				ddrval	=lpDD4->lpVtbl->CreateSurface(lpDD4, &ddsd, &lpDDSBack, NULL);
				if(ddrval==DD_OK)
				{
					//both were created, make sure they are
					//in vidram
					memset(&ddscaps, 0, sizeof(DDSCAPS2));
					lpDDSPrimary->lpVtbl->GetCaps(lpDDSPrimary, &ddscaps);
					if(ddscaps.dwCaps & DDSCAPS_VIDEOMEMORY)
					{
						memset(&ddscaps, 0, sizeof(DDSCAPS2));
						lpDDSBack->lpVtbl->GetCaps(lpDDSBack, &ddscaps);
						if(ddscaps.dwCaps & DDSCAPS_VIDEOMEMORY)
						{
							//both are good to go
							vinfo->VidModes[top].flags	|=VIDEO;
						}
					}
					lpDDSBack->lpVtbl->Release(lpDDSBack);
				}
				lpDDSPrimary->lpVtbl->Release(lpDDSPrimary);
			}

			if(bDMA)
			{
				//check dma capabilities
				memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
				ddsd.dwSize			=sizeof(ddsd);
				ddsd.dwFlags		=DDSD_CAPS;
				ddsd.ddsCaps.dwCaps	=DDSCAPS_PRIMARYSURFACE | DDSCAPS_SYSTEMMEMORY;

				ddrval	=lpDD4->lpVtbl->CreateSurface(lpDD4, &ddsd, &lpDDSPrimary, NULL);
				if(ddrval==DD_OK)
				{
					memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
					ddsd.dwSize			=sizeof(ddsd);
					ddsd.dwFlags		=DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
					ddsd.ddsCaps.dwCaps	=DDSCAPS_SYSTEMMEMORY;
					ddsd.dwHeight		=vinfo->VidModes[top].height;
					ddsd.dwWidth		=vinfo->VidModes[top].width;

					ddrval	=lpDD4->lpVtbl->CreateSurface(lpDD4, &ddsd, &lpDDSBack, NULL);
					if(ddrval==DD_OK)
					{
						memset(&ddscaps, 0, sizeof(DDSCAPS2));
						lpDDSPrimary->lpVtbl->GetCaps(lpDDSPrimary, &ddscaps);
						if(ddscaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
						{
							memset(&ddscaps, 0, sizeof(DDSCAPS2));
							lpDDSBack->lpVtbl->GetCaps(lpDDSBack, &ddscaps);
							if(ddscaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
							{
								//both are good to go
								vinfo->VidModes[top].flags	|=(SYSTEM|DMABLT);
							}
						}
						lpDDSBack->lpVtbl->Release(lpDDSBack);
					}
					lpDDSPrimary->lpVtbl->Release(lpDDSPrimary);
				}
			}

			//check flip
			memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
			ddsd.dwSize				=sizeof(ddsd);
			ddsd.dwFlags			=DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
			ddsd.ddsCaps.dwCaps		=DDSCAPS_PRIMARYSURFACE |
									DDSCAPS_COMPLEX | DDSCAPS_FLIP;
			ddsd.dwBackBufferCount	=1;

			ddrval	=lpDD4->lpVtbl->CreateSurface(lpDD4, &ddsd, &lpDDSPrimary, NULL);
			if(ddrval==DD_OK)	//make sure the backbuffer is really there
			{
				memset(&ddscaps, 0, sizeof(DDSCAPS2));
				ddscaps.dwCaps	=DDSCAPS_BACKBUFFER;
				ddrval	=lpDDSPrimary->lpVtbl->GetAttachedSurface(lpDDSPrimary, &ddscaps, &lpDDSBack);
				if(ddrval==DD_OK)
				{
					//need to grab pitch here since it can change
					//depending on where the surface is created
					memset(&ddsd, 0, sizeof(DDSCAPS2));
					ddsd.dwSize	=sizeof(ddsd);
					ddsd.dwFlags=DDSD_HEIGHT | DDSD_WIDTH;
					lpDDSBack->lpVtbl->GetSurfaceDesc(lpDDSBack, &ddsd);
					vinfo->VidModes[top].pitch	=ddsd.lPitch;

					vinfo->VidModes[top].flags	|=(SYSTEM|FLIP);
					lpDDSPrimary->lpVtbl->Release(lpDDSPrimary);
				}
			}

			if(bHardBlt)
			{
				vinfo->VidModes[top].flags	|=HARDWARE;
			}
		}
	}

	//drop in extra modes for stretching onto the end
	//disable stretched modes for now
/*
	search	=vinfo->NumVidModes;
	for(top=0;top < search;top++)
	{
		//that's a pretty big if 
		//DISABLE > 640 FOR NOW
		if(((vinfo->VidModes[top].width < 640)
			&&(vinfo->VidModes[top].height < 480))
			&& !(vinfo->VidModes[top].flags & MODEXMODE))
		{
			vinfo->VidModes[vinfo->NumVidModes]	=vinfo->VidModes[top];
			vinfo->VidModes[vinfo->NumVidModes++].flags	|=STRETCHMODE;
		}
	}
*/
	//sort
	for(top=0;top < vinfo->NumVidModes-1;top++)
	{
		for(search=top+1;search < vinfo->NumVidModes;search++)
		{
			if(vinfo->VidModes[search].width < vinfo->VidModes[top].width)
			{
				temp	=vinfo->VidModes[search];

				vinfo->VidModes[search]=vinfo->VidModes[top];
				vinfo->VidModes[top]	=temp;
			}
		}
	}
}

BOOL	DoDDrawInit(HWND hwnd, VidEnumInfo *vinfo)
{
	LPDIRECTDRAWCREATE	lpDDCreate;
	HRESULT				ddrval;

	bInitDone	=FALSE;
	ddrawinst	=LoadLibrary("ddraw.dll");

	if(!ddrawinst)
	{
		return	FALSE;
	}
	SetDDrawWindow(hwnd);

	lpDDCreate	=(LPDIRECTDRAWCREATE)GetProcAddress(ddrawinst, "DirectDrawCreate");
	if(lpDDCreate)
	{
		ddrval	=lpDDCreate(NULL, &lpDD, NULL);
	}
	else
	{
		return	DDENUMRET_CANCEL;
	}

	if(ddrval!=DD_OK)
	{
		ErrorPrintf("Non Fatal Error During DirectDrawCreate() in DoDDrawInit()\n%s\n", MyErrorToString(ddrval));
		return	FALSE;
	}

	ddrval = lpDD->lpVtbl->QueryInterface(lpDD, &IID_IDirectDraw4, (LPVOID *)&lpDD4);
	if(ddrval!=DD_OK)
	{
		ErrorPrintf("Non Fatal Error During QueryInterface() in DoDDrawInit()\n%s\n", MyErrorToString(ddrval));
		return	FALSE;
	}
	
	lpDD->lpVtbl->Release(lpDD);
	lpDD	=NULL;

	ddrval	=lpDD4->lpVtbl->SetCooperativeLevel(lpDD4, mhWnd,
			DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
	if(ddrval!=DD_OK)
	{
		ErrorPrintf("Non Fatal Error During SetCooperativeLevel() in DoDDrawInit()\n%s\n", MyErrorToString(ddrval));
		return	FALSE;
	}
	FreeLibrary(ddrawinst);

	bInitDone	=TRUE;
	NumDecalsQd	=0;
	return		TRUE;	
}


BOOL	DoEnumeration(VidEnumInfo *vinfo)
{
	LPDIRECTDRAWCREATE	lpDDCreate;
	LPDIRECTDRAW		pDD;
	LPDIRECTDRAW4		pDD4	=NULL;
	HRESULT				hRet;
	bInitDone			=FALSE;


	ddrawinst	=LoadLibrary("ddraw.dll");

	if(!ddrawinst)
	{
		return	FALSE;
	}

	lpDDCreate	=(LPDIRECTDRAWCREATE)GetProcAddress(ddrawinst, "DirectDrawCreate");
	if(lpDDCreate)
	{
		hRet	=lpDDCreate(NULL, &pDD, NULL);
	}
	else
	{
		return	DDENUMRET_CANCEL;
	}
	
	vinfo->DeviceGuid	=NULL;
	
	if(hRet != DD_OK)
	{
		ErrorPrintf("Non Fatal Error During DirectDrawCreate() in DoEnumeration()\n%s\n", MyErrorToString(hRet));
		return	DDENUMRET_CANCEL;
	}
	memset(&vinfo->DeviceInfo, 0, sizeof(vinfo->DeviceInfo));
	memset(&vinfo->DeviceInfoHost, 0, sizeof(vinfo->DeviceInfo));

	hRet	=pDD->lpVtbl->QueryInterface(pDD, &IID_IDirectDraw4, (LPVOID *)&pDD4);
	if(hRet != DD_OK)
	{
		ErrorPrintf("Non Fatal Error During QueryInterface() in DoEnumeration()\n%s\n", MyErrorToString(hRet));
		return	DDENUMRET_CANCEL;
	}
	
	hRet	=pDD4->lpVtbl->GetDeviceIdentifier(pDD4, &vinfo->DeviceInfo,0);
	hRet	=pDD4->lpVtbl->GetDeviceIdentifier(pDD4, &vinfo->DeviceInfoHost, DDGDI_GETHOSTIDENTIFIER);
	
	if(pDD4)
	{
		pDD4->lpVtbl->Release(pDD4);
	}

	FreeLibrary(ddrawinst);
	
	if(NumDevices < 16)
	{
		NumDevices++;
	}
	else
	{
		return	DDENUMRET_CANCEL;
	}
	return	DDENUMRET_OK;

	return	TRUE;	
}

BOOL	DoModeEnumeration(VidEnumInfo *vinfo)
{
	LPDIRECTDRAWCREATE	lpDDCreate;
	HRESULT				ddrval;
	DDCAPS				ddcaps;

	bInitDone		=FALSE;

	ddrawinst	=LoadLibrary("ddraw.dll");

	if(!ddrawinst)
	{
		return	FALSE;
	}

	lpDDCreate	=(LPDIRECTDRAWCREATE)GetProcAddress(ddrawinst, "DirectDrawCreate");
	if(lpDDCreate)
	{
		ddrval	=lpDDCreate(NULL, &lpDD, NULL);
	}
	else
	{
		return	DDENUMRET_CANCEL;
	}

	if(ddrval != DD_OK)
	{
		ErrorPrintf("DirectDrawCreate FAILED : DoModeEnumeration\n%s\n", MyErrorToString(ddrval));
		return 0;
	}

	ddrval = lpDD->lpVtbl->QueryInterface(lpDD, &IID_IDirectDraw4, (LPVOID *)&lpDD4);
	if(ddrval != DD_OK)
	{
		ErrorPrintf("QueryInterface FAILED : DoModeEnumeration\n%s\n", MyErrorToString(ddrval));
		return 0;
	}
	
	lpDD->lpVtbl->Release(lpDD);
	lpDD	=NULL;

	//test for general dma support
	memset(&ddcaps, 0, sizeof(DDCAPS));
	ddcaps.dwSize	=sizeof(ddcaps);
	lpDD4->lpVtbl->GetCaps(lpDD4, &ddcaps, NULL);

	if(ddcaps.dwCaps & DDCAPS_CANBLTSYSMEM)
	{
		ErrorPrintf("System to video blits supported\n");
		if(ddcaps.dwVSBCaps & DDCAPS_BLTQUEUE)
		{
			ErrorPrintf("DMA Asynch Video to System blits supported\n");
		}
		if(ddcaps.dwSSBCaps & DDCAPS_BLTQUEUE)
		{
			ErrorPrintf("DMA Asynch System to System blits supported\n");
		}
		if(ddcaps.dwSVBCaps & DDCAPS_BLTQUEUE)
		{
			bDMA	=TRUE;

			ErrorPrintf("DMA Asynch System to Video blits supported\n");
			if(ddcaps.dwCaps & DDCAPS2_NOPAGELOCKREQUIRED)
			{
				ErrorPrintf("DMA Asynch page lock not required\n");
			}
			else
			{
				bDMAPageLock	=TRUE;
				ErrorPrintf("DMA Asynch page lock required\n");
			}
		}
	}
	if(ddcaps.dwCaps2 & DDCAPS_BLT)
	{
		ErrorPrintf("Hardware Blt supported\n");
		bHardBlt	=TRUE;
	}

	//commented code below used for heavy mode testing
//	ddrval	=lpDD4->lpVtbl->SetCooperativeLevel(lpDD4, ActiveWnd,
//		DDSCL_ALLOWMODEX | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
//	if(ddrval != DD_OK)
//	{
//		return 0;
//	}
	lpDD4->lpVtbl->EnumDisplayModes(lpDD4, 0, NULL, (LPVOID)vinfo, ModeCallback);

//	SortDDrawVideoModeList(vinfo);

	lpDD4->lpVtbl->Release(lpDD4);
	lpDD4		=NULL;

	FreeLibrary(ddrawinst);

	return		TRUE;	
}

void	GetDDrawPixelFormat(DRV_Window *cwnd)
{
	DDPIXELFORMAT	ddpf;
	U32				i, j;

	ddpf.dwSize	=sizeof(ddpf);
	lpDDSPrimary->lpVtbl->GetPixelFormat(lpDDSPrimary, &ddpf);

	if(!(ddpf.dwFlags & DDPF_RGB))
	{
		return;
	}
	cwnd->BytesPerPixel	=ddpf.dwRGBBitCount / 8;
	cwnd->R_mask		=ddpf.dwRBitMask;
	cwnd->G_mask		=ddpf.dwGBitMask;
	cwnd->B_mask		=ddpf.dwBBitMask;

	for(j=0,i=ddpf.dwRBitMask;!(i & 1);i>>=1,j++);
	cwnd->R_shift	=j;

	for(j=0,i=ddpf.dwGBitMask;!(i & 1);i>>=1,j++);
	cwnd->G_shift	=j;

	for(j=0,i=ddpf.dwBBitMask;!(i & 1);i>>=1,j++);
	cwnd->B_shift	=j;

	for(i=(ddpf.dwRBitMask>>cwnd->R_shift),cwnd->R_width=0;i;i >>= 1, cwnd->R_width++);
	for(i=(ddpf.dwGBitMask>>cwnd->G_shift),cwnd->G_width=0;i;i >>= 1, cwnd->G_width++);
	for(i=(ddpf.dwBBitMask>>cwnd->B_shift),cwnd->B_width=0;i;i >>= 1, cwnd->B_width++);
}

LPDIRECTDRAWSURFACE4	DDrawLoadSurface(U32 dwWidth, U32 dwHeight, const void *pixels, const char *pal)
{
	LPDIRECTDRAWSURFACE4	lpDDS;
	DDSURFACEDESC2			ddsd;
	HRESULT					ddrval;
	int						i;
	
	memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
	ddsd.dwSize			=sizeof(DDSURFACEDESC2);
	ddsd.dwFlags		=DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_CKSRCBLT;
	ddsd.ddsCaps.dwCaps	=DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
	ddsd.dwHeight		=dwHeight;
	ddsd.dwWidth		=dwWidth;

	ddsd.ddckCKSrcBlt.dwColorSpaceLowValue	=((((U8 *)pal)[765])<<16)
											|((((U8 *)pal)[766])<<8)
											|(((U8 *)pal)[767]);
	ddsd.ddckCKSrcBlt.dwColorSpaceHighValue	=((((U8 *)pal)[765])<<16)
											|((((U8 *)pal)[766])<<8)
											|(((U8 *)pal)[767]);

	
	ddrval	=lpDD4->lpVtbl->CreateSurface(lpDD4, &ddsd, &lpDDS, NULL);

	if(ddrval != DD_OK)
	{
		return	NULL;
	}
	
	memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
	ddsd.dwSize	=sizeof(DDSURFACEDESC2);
	ddrval		=lpDDS->lpVtbl->Lock(lpDDS, NULL, &ddsd, DDLOCK_WAIT, NULL);
	
	if(ddrval != DD_OK)
	{
		lpDDS->lpVtbl->Release(lpDDS);
		return	NULL;
	}

	for(i=0;i < (S32)(dwHeight * dwWidth);i++)
	{
		((U32 *)ddsd.lpSurface)[i]	=(((U8 *)pal)[(((U8 *)pixels)[i]) * 3])<<16
									|((((U8 *)pal)[((((U8 *)pixels)[i]) * 3)+1])<<8)
									|((((U8 *)pal)[((((U8 *)pixels)[i]) * 3)+2])<<0);
	}

	lpDDS->lpVtbl->Unlock(lpDDS, NULL);
	
	return	lpDDS;
}

char* MyErrorToString(HRESULT error)
{
    switch(error) {
        case DD_OK:
            /* Also includes D3D_OK and D3DRM_OK */
            return "No error.\0";
        case DDERR_ALREADYINITIALIZED:
            return "This object is already initialized.\0";
        case DDERR_BLTFASTCANTCLIP:
            return "Return if a clipper object is attached to the source surface passed into a BltFast call.\0";
        case DDERR_CANNOTATTACHSURFACE:
            return "This surface can not be attached to the requested surface.\0";
        case DDERR_CANNOTDETACHSURFACE:
            return "This surface can not be detached from the requested surface.\0";
        case DDERR_CANTCREATEDC:
            return "Windows can not create any more DCs.\0";
        case DDERR_CANTDUPLICATE:
            return "Can't duplicate primary & 3D surfaces, or surfaces that are implicitly created.\0";
        case DDERR_CLIPPERISUSINGHWND:
            return "An attempt was made to set a cliplist for a clipper object that is already monitoring an hwnd.\0";
        case DDERR_COLORKEYNOTSET:
            return "No src color key specified for this operation.\0";
        case DDERR_CURRENTLYNOTAVAIL:
            return "Support is currently not available.\0";
        case DDERR_DIRECTDRAWALREADYCREATED:
            return "A DirectDraw object representing this driver has already been created for this process.\0";
        case DDERR_EXCEPTION:
            return "An exception was encountered while performing the requested operation.\0";
        case DDERR_EXCLUSIVEMODEALREADYSET:
            return "An attempt was made to set the cooperative level when it was already set to exclusive.\0";
        case DDERR_GENERIC:
            return "Generic failure.\0";
        case DDERR_HEIGHTALIGN:
            return "Height of rectangle provided is not a multiple of reqd alignment.\0";
        case DDERR_HWNDALREADYSET:
            return "The CooperativeLevel HWND has already been set. It can not be reset while the process has surfaces or palettes created.\0";
        case DDERR_HWNDSUBCLASSED:
            return "HWND used by DirectDraw CooperativeLevel has been subclassed, this prevents DirectDraw from restoring state.\0";
        case DDERR_IMPLICITLYCREATED:
            return "This surface can not be restored because it is an implicitly created surface.\0";
        case DDERR_INCOMPATIBLEPRIMARY:
            return "Unable to match primary surface creation request with existing primary surface.\0";
        case DDERR_INVALIDCAPS:
            return "One or more of the caps bits passed to the callback are incorrect.\0";
        case DDERR_INVALIDCLIPLIST:
            return "DirectDraw does not support the provided cliplist.\0";
        case DDERR_INVALIDDIRECTDRAWGUID:
            return "The GUID passed to DirectDrawCreate is not a valid DirectDraw driver identifier.\0";
        case DDERR_INVALIDMODE:
            return "DirectDraw does not support the requested mode.\0";
        case DDERR_INVALIDOBJECT:
            return "DirectDraw received a pointer that was an invalid DIRECTDRAW object.\0";
        case DDERR_INVALIDPARAMS:
            return "One or more of the parameters passed to the function are incorrect.\0";
        case DDERR_INVALIDPIXELFORMAT:
            return "The pixel format was invalid as specified.\0";
        case DDERR_INVALIDPOSITION:
            return "Returned when the position of the overlay on the destination is no longer legal for that destination.\0";
        case DDERR_INVALIDRECT:
            return "Rectangle provided was invalid.\0";
        case DDERR_LOCKEDSURFACES:
            return "Operation could not be carried out because one or more surfaces are locked.\0";
        case DDERR_NO3D:
            return "There is no 3D present.\0";
        case DDERR_NOALPHAHW:
            return "Operation could not be carried out because there is no alpha accleration hardware present or available.\0";
        case DDERR_NOBLTHW:
            return "No blitter hardware present.\0";
        case DDERR_NOCLIPLIST:
            return "No cliplist available.\0";
        case DDERR_NOCLIPPERATTACHED:
            return "No clipper object attached to surface object.\0";
        case DDERR_NOCOLORCONVHW:
            return "Operation could not be carried out because there is no color conversion hardware present or available.\0";
        case DDERR_NOCOLORKEY:
            return "Surface doesn't currently have a color key\0";
        case DDERR_NOCOLORKEYHW:
            return "Operation could not be carried out because there is no hardware support of the destination color key.\0";
        case DDERR_NOCOOPERATIVELEVELSET:
            return "Create function called without DirectDraw object method SetCooperativeLevel being called.\0";
        case DDERR_NODC:
            return "No DC was ever created for this surface.\0";
        case DDERR_NODDROPSHW:
            return "No DirectDraw ROP hardware.\0";
        case DDERR_NODIRECTDRAWHW:
            return "A hardware-only DirectDraw object creation was attempted but the driver did not support any hardware.\0";
        case DDERR_NOEMULATION:
            return "Software emulation not available.\0";
        case DDERR_NOEXCLUSIVEMODE:
            return "Operation requires the application to have exclusive mode but the application does not have exclusive mode.\0";
        case DDERR_NOFLIPHW:
            return "Flipping visible surfaces is not supported.\0";
        case DDERR_NOGDI:
            return "There is no GDI present.\0";
        case DDERR_NOHWND:
            return "Clipper notification requires an HWND or no HWND has previously been set as the CooperativeLevel HWND.\0";
        case DDERR_NOMIRRORHW:
            return "Operation could not be carried out because there is no hardware present or available.\0";
        case DDERR_NOOVERLAYDEST:
            return "Returned when GetOverlayPosition is called on an overlay that UpdateOverlay has never been called on to establish a destination.\0";
        case DDERR_NOOVERLAYHW:
            return "Operation could not be carried out because there is no overlay hardware present or available.\0";
        case DDERR_NOPALETTEATTACHED:
            return "No palette object attached to this surface.\0";
        case DDERR_NOPALETTEHW:
            return "No hardware support for 16 or 256 color palettes.\0";
        case DDERR_NORASTEROPHW:
            return "Operation could not be carried out because there is no appropriate raster op hardware present or available.\0";
        case DDERR_NOROTATIONHW:
            return "Operation could not be carried out because there is no rotation hardware present or available.\0";
        case DDERR_NOSTRETCHHW:
            return "Operation could not be carried out because there is no hardware support for stretching.\0";
        case DDERR_NOT4BITCOLOR:
            return "DirectDrawSurface is not in 4 bit color palette and the requested operation requires 4 bit color palette.\0";
        case DDERR_NOT4BITCOLORINDEX:
            return "DirectDrawSurface is not in 4 bit color index palette and the requested operation requires 4 bit color index palette.\0";
        case DDERR_NOT8BITCOLOR:
            return "DirectDrawSurface is not in 8 bit color mode and the requested operation requires 8 bit color.\0";
        case DDERR_NOTAOVERLAYSURFACE:
            return "Returned when an overlay member is called for a non-overlay surface.\0";
        case DDERR_NOTEXTUREHW:
            return "Operation could not be carried out because there is no texture mapping hardware present or available.\0";
        case DDERR_NOTFLIPPABLE:
            return "An attempt has been made to flip a surface that is not flippable.\0";
        case DDERR_NOTFOUND:
            return "Requested item was not found.\0";
        case DDERR_NOTLOCKED:
            return "Surface was not locked.  An attempt to unlock a surface that was not locked at all, or by this process, has been attempted.\0";
        case DDERR_NOTPALETTIZED:
            return "The surface being used is not a palette-based surface.\0";
        case DDERR_NOVSYNCHW:
            return "Operation could not be carried out because there is no hardware support for vertical blank synchronized operations.\0";
        case DDERR_NOZBUFFERHW:
            return "Operation could not be carried out because there is no hardware support for zbuffer blitting.\0";
        case DDERR_NOZOVERLAYHW:
            return "Overlay surfaces could not be z layered based on their BltOrder because the hardware does not support z layering of overlays.\0";
        case DDERR_OUTOFCAPS:
            return "The hardware needed for the requested operation has already been allocated.\0";
        case DDERR_OUTOFMEMORY:
            return "DirectDraw does not have enough memory to perform the operation.\0";
        case DDERR_OUTOFVIDEOMEMORY:
            return "DirectDraw does not have enough memory to perform the operation.\0";
        case DDERR_OVERLAYCANTCLIP:
            return "The hardware does not support clipped overlays.\0";
        case DDERR_OVERLAYCOLORKEYONLYONEACTIVE:
            return "Can only have ony color key active at one time for overlays.\0";
        case DDERR_OVERLAYNOTVISIBLE:
            return "Returned when GetOverlayPosition is called on a hidden overlay.\0";
        case DDERR_PALETTEBUSY:
            return "Access to this palette is being refused because the palette is already locked by another thread.\0";
        case DDERR_PRIMARYSURFACEALREADYEXISTS:
            return "This process already has created a primary surface.\0";
        case DDERR_REGIONTOOSMALL:
            return "Region passed to Clipper::GetClipList is too small.\0";
        case DDERR_SURFACEALREADYATTACHED:
            return "This surface is already attached to the surface it is being attached to.\0";
        case DDERR_SURFACEALREADYDEPENDENT:
            return "This surface is already a dependency of the surface it is being made a dependency of.\0";
        case DDERR_SURFACEBUSY:
            return "Access to this surface is being refused because the surface is already locked by another thread.\0";
        case DDERR_SURFACEISOBSCURED:
            return "Access to surface refused because the surface is obscured.\0";
        case DDERR_SURFACELOST:
            return "Access to this surface is being refused because the surface memory is gone. The DirectDrawSurface object representing this surface should have Restore called on it.\0";
        case DDERR_SURFACENOTATTACHED:
            return "The requested surface is not attached.\0";
        case DDERR_TOOBIGHEIGHT:
            return "Height requested by DirectDraw is too large.\0";
        case DDERR_TOOBIGSIZE:
            return "Size requested by DirectDraw is too large, but the individual height and width are OK.\0";
        case DDERR_TOOBIGWIDTH:
            return "Width requested by DirectDraw is too large.\0";
        case DDERR_UNSUPPORTED:
            return "Action not supported.\0";
        case DDERR_UNSUPPORTEDFORMAT:
            return "FOURCC format requested is unsupported by DirectDraw.\0";
        case DDERR_UNSUPPORTEDMASK:
            return "Bitmask in the pixel format requested is unsupported by DirectDraw.\0";
        case DDERR_VERTICALBLANKINPROGRESS:
            return "Vertical blank is in progress.\0";
        case DDERR_WASSTILLDRAWING:
            return "Informs DirectDraw that the previous Blt which is transfering information to or from this Surface is incomplete.\0";
        case DDERR_WRONGMODE:
            return "This surface can not be restored because it was created in a different mode.\0";
        case DDERR_XALIGN:
            return "Rectangle provided was not horizontally aligned on required boundary.\0";
        default:
            return "Unrecognized error value.\0";
    }
}

BOOL	DDrawBlitDecal(LPDIRECTDRAWSURFACE4 lpDDSDecal, RECT *SRect, S32 x, S32 y)
{
	HRESULT			ddrval;
	DDSURFACEDESC2	ddsd;
	RECT			rDest;
	char			szTemp[256];

	//make sure the back buffer is unlocked (for stretch)
/*	if(bBackLocked)
	{
		ddrval	=lpDDSBack->lpVtbl->Unlock(lpDDSBack, NULL);
		if(ddrval!=DD_OK && ddrval!=DDERR_NOTLOCKED)
		{
			ErrorPrintf(MyErrorToString(ddrval));
			return	TRUE;
		}
	}
	bBackLocked	=FALSE;
*/

	if(SRect)
	{
		rDest.left	=x;
		rDest.top	=y;
		rDest.right	=x + (SRect->right - SRect->left);
		rDest.bottom=y + (SRect->bottom- SRect->top);
	}
	else
	{
		memset(&ddsd, 0, sizeof(DDSCAPS2));
		ddsd.dwSize	=sizeof(ddsd);
		ddsd.dwFlags=DDSD_HEIGHT | DDSD_WIDTH;
		lpDDSDecal->lpVtbl->GetSurfaceDesc(lpDDSDecal, &ddsd);

		rDest.left	=x;
		rDest.top	=y;
		rDest.right	=x + ddsd.dwWidth;
		rDest.bottom=y + ddsd.dwHeight;
	}

	ddrval	=DD_OK;
	while(1)
	{
		if(SRect)
		{
			ddrval	=lpDDSBack->lpVtbl->Blt(lpDDSBack, &rDest, lpDDSDecal, SRect, DDBLT_WAIT | DDBLT_KEYSRC, NULL);
		}
		else
		{
			ddrval	=lpDDSBack->lpVtbl->Blt(lpDDSBack, &rDest, lpDDSDecal, NULL, DDBLT_WAIT | DDBLT_KEYSRC, NULL);
		}
		if(ddrval==DD_OK)
			break;

		if(ddrval==DDERR_SURFACELOST)
		{
			ddrval	=RestoreAll();
			if(ddrval!=DD_OK)
			{
				ErrorPrintf("Error During Blt() in DDrawBltDecal()...\n");
				ErrorPrintf("DirectDraw Surfaces were lost and could not be restored : %s\n", MyErrorToString(ddrval));
				return	FALSE;
			}
		}
		else if(ddrval!=DDERR_WASSTILLDRAWING)
		{
			ErrorPrintf("Error During Blt() in DDrawBltDecal() : %s\n", MyErrorToString(ddrval));
			if(SRect)
			{
				sprintf(szTemp, "SLeft:%d, STop:%d, SRight:%d, SBottom:%d\n", SRect->left, SRect->top, SRect->right, SRect->bottom);
				ErrorPrintf(szTemp);
			}
			sprintf(szTemp, "DLeft:%d, DTop:%d, DRight:%d, DBottom:%d\n%x", rDest.left, rDest.top, rDest.right, rDest.bottom, (U32)lpDDSDecal);
			ErrorPrintf(szTemp);
			return	FALSE;
		}
	}
	return	TRUE;
}

BOOL	DDrawBlitDecalToFront(LPDIRECTDRAWSURFACE4 lpDDSDecal, RECT *SRect, S32 x, S32 y)
{
	HRESULT			ddrval;
	DDSURFACEDESC2	ddsd;
	RECT			rDest;
	char			szTemp[256];

	if(SRect)
	{
		rDest.left	=x;
		rDest.top	=y;
		rDest.right	=x + (SRect->right - SRect->left);
		rDest.bottom=y + (SRect->bottom- SRect->top);
	}
	else
	{
		memset(&ddsd, 0, sizeof(DDSCAPS2));
		ddsd.dwSize	=sizeof(ddsd);
		ddsd.dwFlags=DDSD_HEIGHT | DDSD_WIDTH;
		lpDDSDecal->lpVtbl->GetSurfaceDesc(lpDDSDecal, &ddsd);

		rDest.left	=x;
		rDest.top	=y;
		rDest.right	=x + ddsd.dwWidth;
		rDest.bottom=y + ddsd.dwHeight;
	}

	ddrval	=DD_OK;
	while(1)
	{
		if(SRect)
		{
			ddrval	=lpDDSPrimary->lpVtbl->Blt(lpDDSPrimary, &rDest, lpDDSDecal, SRect, DDBLT_WAIT | DDBLT_KEYSRC, NULL);
		}
		else
		{
			ddrval	=lpDDSPrimary->lpVtbl->Blt(lpDDSPrimary, &rDest, lpDDSDecal, NULL, DDBLT_WAIT | DDBLT_KEYSRC, NULL);
		}
		if(ddrval==DD_OK)
			break;

		if(ddrval==DDERR_SURFACELOST)
		{
			ddrval	=RestoreAll();
			if(ddrval!=DD_OK)
			{
				ErrorPrintf("Error During Blt() in DDrawBltDecalToFront()...\n");
				ErrorPrintf("DirectDraw Surfaces were lost and could not be restored : %s\n", MyErrorToString(ddrval));
				return	FALSE;
			}
		}
		else if(ddrval!=DDERR_WASSTILLDRAWING)
		{
			ErrorPrintf("Error During Blt() in DDrawBltDecalToFront() : %s\n", MyErrorToString(ddrval));
			if(SRect)
			{
				sprintf(szTemp, "SLeft:%d, STop:%d, SRight:%d, SBottom:%d\n", SRect->left, SRect->top, SRect->right, SRect->bottom);
				ErrorPrintf(szTemp);
			}
			sprintf(szTemp, "DLeft:%d, DTop:%d, DRight:%d, DBottom:%d\n%x", rDest.left, rDest.top, rDest.right, rDest.bottom, (U32)lpDDSDecal);
			ErrorPrintf(szTemp);
			return	FALSE;
		}
	}
	return	TRUE;
}

BOOL	DDrawBlitDecalDelayed(LPDIRECTDRAWSURFACE4 lpDDSDecal, RECT *SRect, S32 x, S32 y)
{
	DecalQ[NumDecalsQd].surf	=lpDDSDecal;
	if(SRect)
	{
		DecalQ[NumDecalsQd].srect	=*SRect;
	}
	else
	{
		DecalQ[NumDecalsQd].srect.left	=-1;
	}
	DecalQ[NumDecalsQd].x		=x;
	DecalQ[NumDecalsQd].y		=y;

	if(NumDecalsQd < 8191)
	{
		NumDecalsQd++;
	}
	return	TRUE;
}

void	ClearBackBuffer(DRV_Window *cwnd)
{
	DDBLTFX			bfx;

	if(!bActive)
	{
		return;
	}

	memset(&bfx, 0, sizeof(bfx));
	bfx.dwSize		=sizeof(bfx);
	bfx.dwFillColor	=0;

	lpDDSBack->lpVtbl->Blt(lpDDSBack, NULL, NULL, NULL, DDBLT_COLORFILL, &bfx);
}

geBoolean	DRIVERCC DrvSetActive(geBoolean wParam)
{
	bActive	=wParam;

	if(bInitDone && bActive)
	{	//regaining focus
		ErrorPrintf("Regaining Focus\n");
		OutputDebugString("Regaining Focus\n");
		if(lpDDSPrimary->lpVtbl->IsLost(lpDDSPrimary)==DDERR_SURFACELOST)
		{
			if(RestoreAll()==DD_OK)
			{
				ErrorPrintf("Regained focus and restored all DirectDraw surfaces\n");
				OutputDebugString("Regained focus and restored all DirectDraw surfaces\n");
				ShowWindow(mhWnd, SW_SHOWNORMAL);	//dx doesn't restore it
			}
			else
			{
				ErrorPrintf("Couldn't restore surfaces!\n");
				OutputDebugString("Couldn't restore surfaces!\n");
			}
		}
		else
		{
			ErrorPrintf("Regained focus, no surfaces lost\n");
			OutputDebugString("Regained focus, no surfaces lost\n");
		}
	}
	else
	{
		ErrorPrintf("Lost Focus\n");
		OutputDebugString("Lost Focus\n");
	}
	return	GE_TRUE;
}

