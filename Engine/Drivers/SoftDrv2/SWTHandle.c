/****************************************************************************************/
/*  SWTHandle.C                                                                         */
/*                                                                                      */
/*  Author: Mike Sandige, John Pollard                                                  */
/*  Description:  Manager for texture construction and available texture formats for    */
/*                the software driver                                                   */
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

#include <stdlib.h>					// malloc, free
#include <memory.h>					// memset
#include <Assert.h>

#include "SoftDrv.h"				// SD_Display, ClientWindow.  Could be cleaned up.
#include "SWTHandle.h"

#ifdef GENESIS_VERSION_2
#include "errorlog.h"
#else
#define geErrorLog_AddString(Error,xx,yy) 
#endif


#define	MAX_TEXTURE_HANDLES		15000

geRDriver_THandle	SWTHandle_TextureHandles[MAX_TEXTURE_HANDLES];

static int32 SWTHandle_SnapToPower2(int32 Width)
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


//========================================================================================
//	SWTHandle_FindTextureHandle
//========================================================================================
static geRDriver_THandle *SWTHandle_FindTextureHandle()
{
	int32				i;
	geRDriver_THandle	*THandle;

	THandle = SWTHandle_TextureHandles;

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
//	SWTHandle_FreeTextureHandle
//========================================================================================
static geBoolean SWTHandle_FreeTextureHandle(geRDriver_THandle *THandle)
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
		SWTHandle_FreeTextureHandle(THandle->PalHandle);
	}

	if(THandle->AlphaHandle)
	{
		SWTHandle_FreeTextureHandle(THandle->AlphaHandle);
	}

	for(k=0;k < THandle->MipLevels;k++)
	{
		if(THandle->BitPtr[k])
			{
				free(THandle->BitPtr[k]);
			}
		THandle->BitPtr[k]	=NULL;
	}

	memset(THandle, 0, sizeof(geRDriver_THandle));	

	return	GE_TRUE;
}

//==================================================================================
//	SWTHandle_FreeAllTextureHandles
//==================================================================================
geBoolean SWTHandle_FreeAllTextureHandles(void)
{
	int32				i;
	geRDriver_THandle	*pTHandle;

	pTHandle = SWTHandle_TextureHandles;

	for (i=0; i< MAX_TEXTURE_HANDLES; i++, pTHandle++)
	{
		if (!pTHandle->Active)
		{
			continue;
		}
		
		SWTHandle_FreeTextureHandle(pTHandle);
	}

	return GE_TRUE;
}

geRDriver_THandle *DRIVERCC SWTHandle_CreateTexture(int32							Width, 
													int32							Height, 
													int32							NumMipLevels, 
													const geRDriver_PixelFormat		*PixelFormat)
{
	int32				i, SWidth, SHeight;			// Snapped to power of 2 width/height
	geRDriver_THandle	*THandle;

	assert(PixelFormat);
	
	THandle = SWTHandle_FindTextureHandle();

	if (!THandle)
	{
		geErrorLog_AddString(GE_ERR_INTERNAL_RESOURCE, "SWTHandle_CreateTexture:  No more handles left.",NULL);
		goto ExitWithError;
	}

	if(PixelFormat->Flags & RDRIVER_PF_3D)
	{
		if (Width > 256)
		{
			geErrorLog_AddString(GE_ERR_BAD_PARAMETER, "SWTHandle_CreateTexture: Width > 256.",NULL);
			goto ExitWithError;
		}

		if (Height > 256)
		{
			geErrorLog_AddString(GE_ERR_BAD_PARAMETER, "SWTHandle_CreateTexture: Height > 256.",NULL);
			goto ExitWithError;
		}
		SWidth = SWTHandle_SnapToPower2(Width);
		SHeight = SWTHandle_SnapToPower2(Height);

		if (Width != SWidth)
		{
			geErrorLog_AddString(GE_ERR_BAD_PARAMETER, "SWTHandle_CreateTexture: Not a power of 2.",NULL);
			goto ExitWithError;
		}

		if (Height != SHeight)
		{
			geErrorLog_AddString(GE_ERR_BAD_PARAMETER, "SWTHandle_CreateTexture: Not a power of 2.",NULL);
			goto ExitWithError;
		}
	}
	
	THandle->MipLevels		=NumMipLevels;
	THandle->Width			=Width;
	THandle->Height			=Height;
	THandle->PixelFormat	=*PixelFormat;

	for(i=0;i < NumMipLevels;i++)
	{
		if(PixelFormat->Flags & RDRIVER_PF_PALETTE)
		{
		
			if( PixelFormat->PixelFormat == GE_PIXELFORMAT_32BIT_XRGB ||
				PixelFormat->PixelFormat == GE_PIXELFORMAT_32BIT_XBGR)	
			{
				THandle->BitPtr[i]	=(uint16 *)malloc(sizeof(U32) * Width * Height);
			}
			else					
			{
				geErrorLog_AddString(GE_ERR_BAD_PARAMETER, "SWTHandle_CreateTexture: Bad Pal format.",NULL);
				goto ExitWithError;
			}
		}
		else
		{
			switch (PixelFormat->PixelFormat)
			{
				case GE_PIXELFORMAT_16BIT_4444_ARGB:
				{
					THandle->BitPtr[i]	=(uint16 *)malloc(sizeof(uint16) * Width * Height);
					break;
				}
					
				case GE_PIXELFORMAT_16BIT_565_RGB:
				{
					THandle->BitPtr[i]	=(uint16 *)malloc(sizeof(uint16) * Width * Height);
					break;
				}
				
				case GE_PIXELFORMAT_16BIT_555_RGB:
				{
					THandle->BitPtr[i]	=(uint16 *)malloc(sizeof(uint16) * Width * Height);
					break;
				}

				case GE_PIXELFORMAT_8BIT:
				{
					THandle->BitPtr[i]	=(uint16 *)malloc(sizeof(uint8) * Width * Height);
					break;
				}

				case GE_PIXELFORMAT_24BIT_RGB:
				{
					THandle->BitPtr[i]	=(uint16 *)malloc((sizeof(uint8)*3) * Width * Height);
					break;
				}

				default:
				{
					geErrorLog_AddString(GE_ERR_BAD_PARAMETER, "SOFT_Create3DTexture: Invalid pixel format.",NULL);
					goto ExitWithError;
				}
			}
			
			if ( PixelFormat->Flags & RDRIVER_PF_CAN_DO_COLORKEY )
				{
					THandle->Flags |= THANDLE_TRANS;
				}

		}
		Width = Width / 2;
		if (Width < 1) Width = 1;
		Height = Height / 2;
		if (Height < 1) Height = 1;
	}

	return THandle;
		
	ExitWithError:
	{
		return NULL;
	}
}

geBoolean	DRIVERCC	SWTHandle_GetInfo(geRDriver_THandle *THandle, int32 MipLevel, geRDriver_THandleInfo *Info)
{
	assert(THandle);
	assert(Info);
	if ( ! THandle->Active )
		{
			geErrorLog_AddString(GE_ERR_BAD_PARAMETER, "SWTHandle_GetInfo:  Bad Texture Handle.",NULL);
			return GE_FALSE;
		}


	Info->Width			= THandle->Width  >> MipLevel;
	Info->Height		= THandle->Height >> MipLevel;
	Info->Stride		= THandle->Width  >> MipLevel;
	Info->ColorKey		= 1;
	Info->Flags			= GE_FALSE;
	#pragma message ("is this right?")
	Info->PixelFormat	= THandle->PixelFormat;
	if ( THandle->Flags & THANDLE_TRANS )
		{
	#pragma message ("remember this")
			Info->Flags	= RDRIVER_THANDLE_HAS_COLORKEY;
		}
	return	GE_TRUE;
}

//can be used to null out the pal (cant assert on palhandle)
geBoolean DRIVERCC SWTHandle_SetPalette(geRDriver_THandle *THandle, geRDriver_THandle *PalHandle)
{
	assert(THandle);

	THandle->PalHandle	=PalHandle;

	return	GE_TRUE;
}

//can be used to null out the pal (cant assert on palhandle)
geRDriver_THandle *DRIVERCC SWTHandle_GetPalette(geRDriver_THandle *THandle)
{
	assert(THandle);
	if ( ! THandle->Active )
		{
			geErrorLog_AddString(GE_ERR_BAD_PARAMETER, "SWTHandle_GetPalette: Bad Texture Handle.",NULL);
			return GE_FALSE;
		}

	return	THandle->PalHandle;
}

geBoolean DRIVERCC	SWTHandle_SetAlpha(geRDriver_THandle *THandle, geRDriver_THandle *AHandle)
{
	assert(THandle);
	if ( ! THandle->Active )
		{
			geErrorLog_AddString(GE_ERR_BAD_PARAMETER, "SWTHandle_SetAlpha: Bad Texture Handle.",NULL);
			return GE_FALSE;
		}

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

geRDriver_THandle *DRIVERCC	SWTHandle_GetAlpha(geRDriver_THandle *THandle)
{
	assert(THandle);
	if ( ! THandle->Active )
		{
			geErrorLog_AddString(GE_ERR_BAD_PARAMETER, "SWTHandle_GetAlpha: Bad Texture Handle.",NULL);
			return GE_FALSE;
		}

	return	THandle->AlphaHandle;
}

geBoolean DRIVERCC SWTHandle_DestroyTexture(geRDriver_THandle *THandle)
{
	return SWTHandle_FreeTextureHandle(THandle);
}

geBoolean DRIVERCC SWTHandle_LockTextureHandle(geRDriver_THandle *THandle, int32 MipLevel, void **Data)
{
	assert(THandle);
	assert(MipLevel <= THandle->MipLevels && MipLevel >= 0);
	if ( ! THandle->Active )
		{
			geErrorLog_AddString(GE_ERR_BAD_PARAMETER, "SWTHandle_LockTextureHandle: Bad Texture Handle.",NULL);
			return GE_FALSE;
		}
	if ( ! THandle->BitPtr[MipLevel] )
		{
			geErrorLog_AddString(GE_ERR_BAD_PARAMETER, "SWTHandle_LockTextureHandle: Bad Texture Handle in Mip.",NULL);
			return GE_FALSE;
		}

	THandle->Flags	|=(THANDLE_LOCKED << MipLevel);
	*Data			=(uint16*)THandle->BitPtr[MipLevel];

	return	GE_TRUE;
}

geBoolean DRIVERCC SWTHandle_UnLockTextureHandle(geRDriver_THandle *THandle, int32 MipLevel)
{
	assert(THandle);
	if (!(THandle->Flags & (THANDLE_LOCKED << MipLevel)))
		{
			geErrorLog_AddString(GE_ERR_BAD_PARAMETER, "SWTHandle_UnLockTextureHandle: Texture Handle is not Locked.",NULL);
			return GE_FALSE;
		}


	THandle->Flags	&=~(THANDLE_LOCKED << MipLevel);	// This mip is now unlocked
	THandle->Flags	|=THANDLE_UPDATE;					// Mark it for updating...

	return GE_TRUE;
}




geBoolean DRIVERCC SWTHandle_EnumPixelFormats(DRV_ENUM_PFORMAT_CB *Cb, void *Context)
{
	geRDriver_PixelFormat	PixelFormat;

	if(!SD_Display)
	{
		geErrorLog_AddString(GE_ERR_INTERNAL_RESOURCE, "SWTHandle_EnumPixelFormats:  No Mode Set.",NULL);
		return GE_FALSE;
	}


	PixelFormat.PixelFormat	= GE_PIXELFORMAT_8BIT;
	PixelFormat.Flags		= RDRIVER_PF_3D;
	if(!Cb(&PixelFormat, Context))
		{
			return	GE_TRUE;
		}
	PixelFormat.PixelFormat	= GE_PIXELFORMAT_8BIT;
	PixelFormat.Flags		= RDRIVER_PF_3D| RDRIVER_PF_COMBINE_LIGHTMAP;
	if(!Cb(&PixelFormat, Context))
		{
			return	GE_TRUE;
		}

	PixelFormat.PixelFormat	= GE_PIXELFORMAT_16BIT_4444_ARGB;
	PixelFormat.Flags		= RDRIVER_PF_3D;
	if(!Cb(&PixelFormat, Context))
		{
			return	GE_TRUE;
		}
	PixelFormat.PixelFormat	= GE_PIXELFORMAT_16BIT_4444_ARGB;
	PixelFormat.Flags		= RDRIVER_PF_3D | RDRIVER_PF_COMBINE_LIGHTMAP;
	if(!Cb(&PixelFormat, Context))
		{
			return	GE_TRUE;
		}

	PixelFormat.PixelFormat	= GE_PIXELFORMAT_32BIT_XBGR;
	PixelFormat.Flags		= RDRIVER_PF_PALETTE;
	if(!Cb(&PixelFormat, Context))
		{
			return	GE_TRUE;
		}

	if(ClientWindow.G_mask == 0x3e0)	//555
		{
			PixelFormat.PixelFormat	= GE_PIXELFORMAT_16BIT_555_RGB;
			PixelFormat.Flags		= RDRIVER_PF_2D;
			if(!Cb(&PixelFormat, Context))
				{
					return	GE_TRUE;
				}
			PixelFormat.PixelFormat	= GE_PIXELFORMAT_16BIT_555_RGB;
			PixelFormat.Flags		= RDRIVER_PF_2D| RDRIVER_PF_CAN_DO_COLORKEY;
			if(!Cb(&PixelFormat, Context))
				{
					return	GE_TRUE;
				}
		}
	else
		{
			PixelFormat.PixelFormat	= GE_PIXELFORMAT_16BIT_565_RGB;
			PixelFormat.Flags		= RDRIVER_PF_2D;
			if(!Cb(&PixelFormat, Context))
				{
					return	GE_TRUE;
				}
			PixelFormat.PixelFormat	= GE_PIXELFORMAT_16BIT_565_RGB;
			PixelFormat.Flags		= RDRIVER_PF_2D| RDRIVER_PF_CAN_DO_COLORKEY;
			if(!Cb(&PixelFormat, Context))
				{
					return	GE_TRUE;
				}
		}

	PixelFormat.PixelFormat	= GE_PIXELFORMAT_24BIT_RGB;
	PixelFormat.Flags		= RDRIVER_PF_LIGHTMAP;

	if(!Cb(&PixelFormat, Context))
	{
		return	GE_TRUE;
	}

	return	GE_TRUE;
}