/****************************************************************************************/
/*  THandle.c                                                                           */
/*                                                                                      */
/*  Author: George McBay (gfm@my-deja.com)                                              */
/*  Description: Texture handle manager for OpenGL driver                               */
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
/*                                                                                      */
/****************************************************************************************/

#include <math.h>
#include <stdio.h>

#include "THandle.h"
#include "OglDrv.h"
#include "Render.h"

extern boundTexture;
extern boundTexture2;

geRDriver_THandle	TextureHandles[MAX_TEXTURE_HANDLES];


// Init THandle system (do nothing, for now)
geBoolean THandle_Startup(void)
{
	return GE_TRUE;
}


// Find an empty texture handle
geRDriver_THandle *FindTextureHandle()
{
	int32				i;
	geRDriver_THandle	*THandle;

	THandle = TextureHandles;

	for(i = 0; i < MAX_TEXTURE_HANDLES; i++, THandle++)
	{
		if(!THandle->Active)
		{
			memset(THandle, 0, sizeof(geRDriver_THandle));

			THandle->Active = GE_TRUE;

			return THandle;
		}
	}

	return NULL;
}


// Cleanup a texture handle.  Remove texture from texture memory and free up related
// system memory.
geBoolean DRIVERCC THandle_Destroy(geRDriver_THandle *THandle)
{
	GLint i;


	if(!THandle->Active)
	{
		return	GE_FALSE;
	}

	if(boundTexture == THandle->TextureID)
		boundTexture = -1;
	else if(boundTexture2 == THandle->TextureID)
		boundTexture2 = -1;

	glDeleteTextures(1, &(THandle->TextureID));

	for(i = 0; i < THANDLE_MAX_MIP_LEVELS; i++)
	{
		if(THandle->Data[i] != NULL)
		{
			free(THandle->Data[i]);
			THandle->Data[i] = NULL;
		}
	}

	memset(THandle, 0, sizeof(geRDriver_THandle));	

	return	GE_TRUE;
}


// Cleanup all currently in-use texture handles
geBoolean FreeAllTextureHandles(void)
{
	int32				i;
	geRDriver_THandle	*pTHandle;

	pTHandle = TextureHandles;


	for(i = 0; i < MAX_TEXTURE_HANDLES; i++, pTHandle++)
	{
		if(!pTHandle->Active)
		{
			continue;
		}
		
		THandle_Destroy(pTHandle);
	}

	glDeleteTextures(1, &decalTexObj);
	decalTexObj = -1;
	boundTexture = -1;
	boundTexture2 = -1;

	return GE_TRUE;
}


// Create a new texture handle...
geRDriver_THandle *DRIVERCC THandle_Create(int32 Width, int32 Height, int32 NumMipLevels, 
										   const geRDriver_PixelFormat *PixelFormat)
{
	geRDriver_THandle	*THandle;	
	GLubyte				Log;

	
	THandle = FindTextureHandle();

	if (!THandle)
	{
		SetLastDrvError(DRV_ERROR_GENERIC, "OGL_THandleCreate: No more handles left.");
		goto ExitWithError;
	}

	if(PixelFormat->Flags & RDRIVER_PF_3D)
	{
		char errMsg[64];

		if(Width > maxTextureSize)
		{
			sprintf(errMsg, "OGL_THandleCreate: Width > GL_MAX_TEXTURE_SIZE (%d)", maxTextureSize);
			SetLastDrvError(DRV_ERROR_GENERIC, errMsg);
			goto ExitWithError;
		}

		if (Height > maxTextureSize)
		{
			sprintf(errMsg, "OGL_THandleCreate: Height > GL_MAX_TEXTURE_SIZE (%d)", maxTextureSize);
			SetLastDrvError(DRV_ERROR_GENERIC, errMsg);
			goto ExitWithError;
		}
	}

	THandle->MipLevels			= NumMipLevels;
	THandle->Width				= Width;
	THandle->Height				= Height;
	THandle->PixelFormat		= *PixelFormat;
	THandle->Flags				= 0;
	
	Log							= (uint8)GetLog(Width, Height);
	
	if(THandle->PixelFormat.Flags & RDRIVER_PF_2D)
	{
		THandle->PaddedWidth = SnapToPower2(THandle->Width);
		THandle->PaddedHeight = SnapToPower2(THandle->Height);
	}
	else if(THandle->PixelFormat.Flags & RDRIVER_PF_3D)
	{
		THandle->InvScale = 1.0f / (GLfloat)((1<<Log));
	}
	else
	{
		// Must be a lightmap
		THandle->Flags |= THANDLE_UPDATE_LM;
		THandle->InvScale = 1.0f / (GLfloat)((1<<Log)<<4);	
	}

	// Init an OpenGL texture object to hold this texture
	glGenTextures(1, &(THandle->TextureID));

	return THandle;
		
	ExitWithError:
	{
		return NULL;
	}
}


geBoolean DRIVERCC THandle_GetInfo(geRDriver_THandle *THandle, int32 MipLevel, geRDriver_THandleInfo *Info)
{

	if(!THandle->Active)
	{
		return GE_FALSE;
	}

	Info->Width			= THandle->Width >> MipLevel;
	Info->Height		= THandle->Height >> MipLevel;
	Info->Stride		= Info->Width;
	Info->Flags  		= 0;
	Info->PixelFormat	= THandle->PixelFormat;

	if(THandle->PixelFormat.Flags & RDRIVER_PF_CAN_DO_COLORKEY)
	{
		Info->Flags = RDRIVER_THANDLE_HAS_COLORKEY;
		Info->ColorKey = 1;
	}
	else
	{
		Info->Flags = 0;
		Info->ColorKey = 0;
	}

	return	GE_TRUE;
}


// Lock a texture for editing by the engine
geBoolean DRIVERCC THandle_Lock(geRDriver_THandle *THandle, int32 MipLevel, void **Data)
{

	// If we've already got data in system mem, return it to the engine as-is
	if(THandle->Data[MipLevel] != NULL)
	{
		THandle->Flags |= (THANDLE_LOCKED << MipLevel);
		*Data = THandle->Data[MipLevel] ;
		return GE_TRUE;
	}

	// Otherwise, grab a new block of memory for the engine to draw on...
	if(THandle->PixelFormat.PixelFormat == GE_PIXELFORMAT_32BIT_ABGR)
	{
		GLint mipWidth, mipHeight;

		if(MipLevel == 0)
		{
			mipWidth = THandle->Width;
			mipHeight = THandle->Height;
		}
		else
		{
			mipWidth =  (THandle->Width / (int)pow(2.0, (double)MipLevel));
			mipHeight = (THandle->Height / (int)pow(2.0, (double)MipLevel));
		}

		THandle->Data[MipLevel] = (GLubyte *)malloc(mipWidth * mipHeight * 4);
	}
	else if(THandle->PixelFormat.PixelFormat == GE_PIXELFORMAT_24BIT_RGB)
	{
		THandle->Data[MipLevel] = (GLubyte *)malloc(THandle->Width * THandle->Height * 3);
	}
	else
	{
		*Data = NULL;
		return GE_FALSE;
	}


	THandle->Flags |= (THANDLE_LOCKED << MipLevel);
	*Data = THandle->Data[MipLevel];

	return GE_TRUE;
}


// Unlocks a texture locked for editing, and sets the texture to be uploaded next time
// it needs to be visible.
geBoolean DRIVERCC THandle_UnLock(geRDriver_THandle *THandle, int32 MipLevel)
{

	if(!(THandle->Flags & (THANDLE_LOCKED << MipLevel)))
	{
		return GE_FALSE;
	}

	if(THandle->Data == NULL)
	{
		return GE_FALSE;
	}

	THandle->Flags	&=~(THANDLE_LOCKED << MipLevel);	

	// Don't update on mips other than level 0.  We ignore the engine-created mips.
	// Somewhat inefficient (since mips will be generated twice), but good visual results.
	// Would be nice if you could tell the engine not to bother with mipping
	if(MipLevel == 0)
	{	
		THandle->Flags	|= THANDLE_UPDATE;					
	}
	
	return GE_TRUE;
}


// Do an actual card upload (well, at least tell the OpenGL driver you'd like one when it 
// gets a chance) of a texture.  Called from the Render_* functions when they require
// use of a texture that is marked for updating (THANDLE_UPDATE)
void THandle_Update(geRDriver_THandle *THandle)
{		
	if(THandle->PixelFormat.Flags & RDRIVER_PF_2D)
	{
		if(THandle->PixelFormat.PixelFormat == GE_PIXELFORMAT_24BIT_RGB)
		{
			if(THandle->Width <= maxTextureSize && THandle->Height <= maxTextureSize)
			{
				GLubyte *dest;

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
			
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_PRIORITY, 0.5f);

				dest = malloc(THandle->PaddedWidth * THandle->PaddedHeight * 4);

				CkBlit24_32(dest, THandle->PaddedWidth, THandle->PaddedHeight, THandle->Data[0], 
					THandle->Width, THandle->Height);

				glTexImage2D(GL_TEXTURE_2D, 0, 4, THandle->PaddedWidth, THandle->PaddedHeight, 
					0, GL_RGBA, GL_UNSIGNED_BYTE, dest); 

				free(dest);
			}
			else
			{
				if(THandle->Data[1] != NULL)
				{
					free(THandle->Data[1]);
				}

				THandle->Data[1] = malloc(THandle->Width * THandle->Height * 4);

				CkBlit24_32(THandle->Data[1], THandle->Width, THandle->Height, THandle->Data[0],
					THandle->Width, THandle->Height);
			}
		}
	}
	else
	{
#ifdef USE_LINEAR_INTERPOLATION 
 #ifdef TRILINEAR_INTERPOLATION
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
 #else 
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
 #endif
#else
 #ifdef TRILINEAR_INTERPOLATION
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
 #else
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
 #endif
#endif 

		if(THandle->PixelFormat.PixelFormat == GE_PIXELFORMAT_32BIT_ABGR)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_PRIORITY, 1.0f);
			
			gluBuild2DMipmaps(GL_TEXTURE_2D, 4, THandle->Width, THandle->Height, GL_RGBA,
				GL_UNSIGNED_BYTE, THandle->Data[0]);
		}
		else
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
			
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_PRIORITY, 0.0f);
			
			gluBuild2DMipmaps(GL_TEXTURE_2D, 3, THandle->Width, THandle->Height, 
				GL_RGB, GL_UNSIGNED_BYTE, THandle->Data[0]);
		}
	}

	THandle->Flags &= ~THANDLE_UPDATE;
}


// Take engine supplied lightmap raw-RGB data and put it into a texture handle.
void THandle_DownloadLightmap(DRV_LInfo *LInfo)
{
	GLubyte *tempBits;

	THandle_Lock(LInfo->THandle, 0, (void**)&tempBits);

	memcpy(tempBits, LInfo->RGBLight[0], LInfo->THandle->Width * LInfo->THandle->Height * 3);

	THandle_UnLock(LInfo->THandle, 0);
}


// Reset the THandle system
geBoolean DRIVERCC DrvResetAll(void)
{
	return	FreeAllTextureHandles();
}


S32 SnapToPower2(S32 Width)
{
	if(Width > 1 && Width <= 2) 
	{
		Width = 2;
	}
	else if(Width > 2 && Width <= 4)
	{
		Width = 4;
	}
	else if(Width > 4 && Width <= 8) 
	{
		Width = 8;
	}
	else if(Width > 8 && Width <= 16)
	{
		Width =16;
	}
	else if(Width > 16 && Width <= 32)
	{
		Width = 32;
	}
	else if(Width > 32 && Width <= 64) 
	{
		Width = 64;
	}
	else if(Width > 64 && Width <= 128) 
	{
		Width = 128;
	}
	else if(Width > 128 && Width <= 256) 
	{
		Width = 256;
	}
	else if(Width > 256 && Width <= 512) 
	{
		Width = 512;
	}
	else if(Width > 512 && Width <= 1024) 
	{
		Width = 1024;
	}
	else if(Width > 1024 && Width <= 2048) 
	{
		Width = 2048;
	}
	else if (Width > 2048 && Width <= 4096 )
	{
		Width = 4096;
	}
	else if (Width > 4096 && Width <= 8192 )
	{
		Width = 8192;
	}
    else if (Width > 8192 && Width <= 16384)
	{
		Width = 16384;
	}

	return Width;
}


uint32 Log2(uint32 P2)
{
	uint32		p = 0;
	int32		i = 0;
	
	for (i = P2; i > 0; i>>=1)
		p++;

	return (p-1);
}


int32 GetLog(int32 Width, int32 Height)
{
	int32	LWidth = SnapToPower2(max(Width, Height));
	
	return Log2(LWidth);
}
