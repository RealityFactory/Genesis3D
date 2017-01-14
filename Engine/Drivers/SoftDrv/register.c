/****************************************************************************************/
/*  register.c                                                                          */
/*                                                                                      */
/*  Author:       John Pollard, Ken Baird                                               */
/*  Description:  Texture Handle related code                                           */
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
#include <Windows.h>
#include <stdio.h>
#include <Assert.h>

#include "SoftDrv.h"
#include "DCommon.h"
#include "Register.h"
#include "Sal.h"
#include "ddraw.h"

geRDriver_THandle	TextureHandles[MAX_TEXTURE_HANDLES];

S32 SnapToPower2(S32 Width);
S32 Log2(S32 P2);

extern CPUInfo		ProcessorInfo;

//========================================================================================
//	FindTextureHandle
//========================================================================================
geRDriver_THandle *FindTextureHandle()
{
	int32				i;
	geRDriver_THandle	*THandle;

	THandle = TextureHandles;

	for (i=0; i< MAX_TEXTURE_HANDLES; i++, THandle++)
	{
		if (!THandle->Active)
		{
			memset(THandle, 0, sizeof(geRDriver_THandle));

			THandle->Active = GE_TRUE;

			return THandle;
		}
	}

	return NULL;
}

//========================================================================================
//	FreeTextureHandle
//========================================================================================
geBoolean FreeTextureHandle(geRDriver_THandle *THandle)
{
	int	k;

	assert(THandle);
//<>	assert(THandle->Active == GE_TRUE);

	if( ! THandle->Active )
	{
		return	GE_FALSE;
	}

	if(THandle->PalHandle)
	{
		FreeTextureHandle(THandle->PalHandle);
	}

	if(THandle->AlphaHandle)
	{
		FreeTextureHandle(THandle->AlphaHandle);
	}

	for(k=0;k < THandle->MipLevels;k++)
	{
		if(!ProcessorInfo.Has3DNow)
		{
			if(THandle->BitPtr[k])
			{
				free(THandle->BitPtr[k]);
			}
		}			
		THandle->BitPtr[k]	=NULL;
	}

	memset(THandle, 0, sizeof(geRDriver_THandle));	

	return	GE_TRUE;
}

//==================================================================================
//	FreeAllTextureHandles
//==================================================================================
geBoolean FreeAllTextureHandles(void)
{
	int32				i;
	geRDriver_THandle	*pTHandle;

	pTHandle = TextureHandles;

#ifdef _DEBUG
	OutputDebugString("FreeAllTextureHandles\n");
#endif

	for (i=0; i< MAX_TEXTURE_HANDLES; i++, pTHandle++)
	{
		if (!pTHandle->Active)
		{
			continue;
		}
		
		FreeTextureHandle(pTHandle);
	}

	return GE_TRUE;
}

geRDriver_THandle *DRIVERCC CreateTexture(int32 Width, int32 Height, int32 NumMipLevels, const geRDriver_PixelFormat *PixelFormat)
{
	int32				i, SWidth, SHeight;			// Snapped to power of 2 width/height
	geRDriver_THandle	*THandle;

	assert(PixelFormat);
	
	THandle = FindTextureHandle();

	if (!THandle)
	{
		SetLastDrvError(DRV_ERROR_GENERIC, "SOFT_CreateTexture: No more handles left.");
		goto ExitWithError;
	}

	if(PixelFormat->Flags & RDRIVER_PF_3D)
	{
		if (Width > 256)
		{
			SetLastDrvError(DRV_ERROR_GENERIC, "SOFT_CreateTexture: Width > 256.");
			goto ExitWithError;
		}

		if (Height > 256)
		{
			SetLastDrvError(DRV_ERROR_GENERIC, "SOFT_CreateTexture: Height > 256.");
			goto ExitWithError;
		}
		SWidth = SnapToPower2(Width);
		SHeight = SnapToPower2(Height);

		if (Width != SWidth)
		{
			SetLastDrvError(DRV_ERROR_GENERIC, "SOFT_CreateTexture: Not a power of 2.");
			goto ExitWithError;
		}

		if (Height != SHeight)
		{
			SetLastDrvError(DRV_ERROR_GENERIC, "SOFT_CreateTexture: Not a power of 2.");
			goto ExitWithError;
		}
	}
	
	THandle->MipLevels		=NumMipLevels;
	THandle->Width			=Width;
	THandle->Height			=Height;
	THandle->PixelFormat	=*PixelFormat;
	THandle->Flags			=0;

	for(i=0;i < NumMipLevels;i++)
	{
		if(PixelFormat->Flags & RDRIVER_PF_PALETTE)
		{
			if( PixelFormat->PixelFormat == GE_PIXELFORMAT_32BIT_ARGB ||
				PixelFormat->PixelFormat == GE_PIXELFORMAT_32BIT_XRGB )	// <> CB added XRGB
			{
				THandle->BitPtr[i]	=(U16 *)malloc(sizeof(U32) * Width * Height);
			}
			else
			{
				SetLastDrvError(DRV_ERROR_GENERIC, "SOFT_CreateTexture: Bad Pal format.");
				goto ExitWithError;
			}
		}
		else
		{
			switch (PixelFormat->PixelFormat)
			{
				case GE_PIXELFORMAT_16BIT_565_RGB:
				{
					THandle->BitPtr[i]	=(U16 *)malloc(sizeof(U16) * Width * Height);
					break;
				}
				
				case GE_PIXELFORMAT_16BIT_555_RGB:
				{
					THandle->BitPtr[i]	=(U16 *)malloc(sizeof(U16) * Width * Height);
					break;
				}

				case GE_PIXELFORMAT_8BIT:
				{
					THandle->BitPtr[i]	=(U16 *)malloc(sizeof(U8) * Width * Height);
					break;
				}

				case GE_PIXELFORMAT_24BIT_RGB:
				{
					THandle->BitPtr[i]	=(U16 *)malloc((sizeof(U8)*3) * Width * Height);
					break;
				}

				default:
				{
					SetLastDrvError(DRV_ERROR_GENERIC, "SOFT_Create3DTexture: Invalid pixel format.");
					goto ExitWithError;
//					THandle->BitPtr[i]	=(U16 *)malloc(sizeof(U16) * Width * Height);
//					break;
				}
			}

			if ( PixelFormat->Flags & RDRIVER_PF_CAN_DO_COLORKEY )
			{
				THandle->Flags |= THANDLE_TRANS;
			}
		}
	}

	return THandle;
		
	ExitWithError:
	{
		return NULL;
	}
}

geBoolean	DRIVERCC	THandle_GetInfo(geRDriver_THandle *THandle, int32 MipLevel, geRDriver_THandleInfo *Info)
{
	assert(THandle);
	assert(Info);
#ifdef _DEBUG
	if ( ! THandle->Active )
		return GE_FALSE;
#endif

	Info->Width		= THandle->Width >> MipLevel;
	Info->Height	= THandle->Height >> MipLevel;
	Info->Stride	= Info->Width;
	Info->Flags  	= 0;
	Info->PixelFormat	=THandle->PixelFormat;

	if ( ProcessorInfo.Has3DNow )
		Info->ColorKey = 255;
	else
		Info->ColorKey	= 1;

	if ( THandle->Flags & THANDLE_TRANS )
	{
		Info->Flags |= RDRIVER_THANDLE_HAS_COLORKEY;
	}

	return	GE_TRUE;
}

//can be used to null out the pal (cant assert on palhandle)
geBoolean DRIVERCC SetPalette(geRDriver_THandle *THandle, geRDriver_THandle *PalHandle)
{
	assert(THandle);

	THandle->PalHandle	=PalHandle;

	return	GE_TRUE;
}

//can be used to null out the pal (cant assert on palhandle)
geRDriver_THandle *DRIVERCC GetPalette(geRDriver_THandle *THandle)
{
	assert(THandle);
#ifdef _DEBUG
	if ( ! THandle->Active )
		return GE_FALSE;
#endif

	return	THandle->PalHandle;
}

geBoolean DRIVERCC	SetAlpha(geRDriver_THandle *THandle, geRDriver_THandle *AHandle)
{
	assert(THandle);
#ifdef _DEBUG
	if ( ! THandle->Active )
		return GE_FALSE;
#endif

	if(THandle->PixelFormat.Flags & RDRIVER_PF_HAS_ALPHA)
	{
		if(AHandle->PixelFormat.Flags & RDRIVER_PF_ALPHA)
		{
			THandle->AlphaHandle	=AHandle;
			return	GE_TRUE;
		}
	}
	return	GE_FALSE;
}

geRDriver_THandle *DRIVERCC	GetAlpha(geRDriver_THandle *THandle)
{
	assert(THandle);
#ifdef _DEBUG
	if ( ! THandle->Active )
		return GE_FALSE;
#endif

	return	THandle->AlphaHandle;
}

geBoolean DRIVERCC DestroyTexture(geRDriver_THandle *THandle)
{
	return FreeTextureHandle(THandle);
}

geBoolean DRIVERCC LockTextureHandle(geRDriver_THandle *THandle, int32 MipLevel, void **Data)
{
	assert(THandle);
	assert(MipLevel <= THandle->MipLevels && MipLevel >= 0);
#ifdef _DEBUG
	if ( ! THandle->Active )
		return GE_FALSE;
	if ( ! THandle->BitPtr[MipLevel] )
		return GE_FALSE;
#endif

	THandle->Flags	|=(THANDLE_LOCKED << MipLevel);
	*Data			=(uint16*)THandle->BitPtr[MipLevel];

	return	GE_TRUE;
}

geBoolean DRIVERCC UnLockTextureHandle(geRDriver_THandle *THandle, int32 MipLevel)
{
	assert(THandle);
	if (!(THandle->Flags & (THANDLE_LOCKED << MipLevel)))
		return GE_FALSE;

	THandle->Flags	&=~(THANDLE_LOCKED << MipLevel);	// This mip is now unlocked
	THandle->Flags	|=THANDLE_UPDATE;					// Mark it for updating...

	return GE_TRUE;
}

BOOL	DRIVERCC	DrvResetAll(void)
{
	return	FreeAllTextureHandles();
}


static	int	Brighten(int Color)
{
	Color	=(int)((float)Color * 1.5f);

	return((Color > 255)? 255 : Color);
}


S32 SnapToPower2(S32 Width)
{
	if (Width > 1 && Width <= 2) Width = 2;
	else if (Width > 2 && Width <= 4) Width = 4;
	else if (Width > 4 && Width <= 8) Width = 8;
	else if (Width > 8 && Width <= 16) Width =16;
	else if (Width > 16 && Width <= 32) Width = 32;
	else if (Width > 32 && Width <= 64) Width = 64;
	else if (Width > 64 && Width <= 128) Width = 128;
	else if (Width > 128 && Width <= 256) Width = 256;

	return Width;
}

S32 Log2(S32 P2)
{
	S32		p = 0;
	S32		i = 0;
	
	for (i = P2; (i&1) == 0; i>>=1)
		p++;

	return p;
}
