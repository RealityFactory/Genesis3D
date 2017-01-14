/****************************************************************************************/
/*  dmodes.h                                                                            */
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
/*  Genesis3D Version 1.1 released November 15, 1999                                 */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#include "basetype.h"

extern	void					GetDDrawPixelFormat(DRV_Window *cwnd);
extern	LPDIRECTDRAWSURFACE4	DDrawLoadSurface(U32 dwWidth, U32 dwHeight, const void *pixels, const char *pal);
extern	geBoolean				LockDDrawBackBuffer(DRV_Window *cwnd, RECT *wrect);
extern	geBoolean				UnlockDDrawBackBuffer(DRV_Window *cwnd, RECT *wrect);
extern	geBoolean				RefreshDDraw(DRV_Window *cwnd, VidModeList *cmode, RECT *src, RECT *dst);
extern	void					ClearBackBuffer(DRV_Window *cwnd);
extern  geBoolean				DDrawBlitDecal(LPDIRECTDRAWSURFACE4 lpDDSDecal, RECT *SRect, S32 x, S32 y);
extern  geBoolean				DDrawBlitDecalDelayed(LPDIRECTDRAWSURFACE4 lpDDSDecal, RECT *SRect, S32 x, S32 y);
extern  geBoolean				DDrawBlitDecalToFront(LPDIRECTDRAWSURFACE4 lpDDSDecal, RECT *SRect, S32 x, S32 y);
extern	geBoolean	DRIVERCC	DrvSetActive(geBoolean wParam);
extern	void					SetDDrawWindow(HWND hwnd);
extern	geBoolean				SetDDrawMode(U32 top, VidEnumInfo *vinfo);
extern	geBoolean				DoDDrawInit(HWND hwnd, VidEnumInfo *vinfo);
extern	geBoolean				DoModeEnumeration(VidEnumInfo *vinfo);
extern	geBoolean				DoEnumeration(VidEnumInfo *vinfo);
extern	void					GetDDrawPixelFormat(DRV_Window *cwnd);
extern	void					FreeDDraw(VidEnumInfo *vinfo);

extern	geBoolean				bWindowed, bActive, bBackLocked;
