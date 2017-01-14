/****************************************************************************************/
/*  WBitmap.c                                                                           */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Creates geBitmaps from the data in the BSP, that are used to render    */
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
#include <Assert.h>

//=====================================================================================
//=====================================================================================
#include "WBitmap.h"
#include "GBSPFile.h"
#include "Ram.h"
#include "Bitmap.h"
#include "Errorlog.h"
#include "Bitmap._h"

//	NOTES -
//	WBitmap is the original owner of all the bitmaps in the .BSP file.  They are kind of a hack right now.
//	Right now, the textures are raw data in the .BSP.  This module, takes that data, and creates geBitmaps
//	for each texture in the.BSP file.  The faces are then referred to these geBitmaps, and NOT the texture data.
//	The texture data can then be freed.
//	The way it really should eventually work, is that the bitmaps will be in the .BSP upon load time!!!
//		JP

#define ZeroMem(Ptr)	memset(Ptr,0,sizeof(*Ptr))

typedef struct geWBitmap
{
	char			Name[64];
	geBitmap		*Bitmap;

	int32			VisFrame;

	uint32			Flags;
} geWBitmap;

typedef struct geWBitmap_Pool
{
	int32			NumWBitmaps;

	geWBitmap		*WBitmaps;				// Linear array of WBitmaps created when the Pool was created
} geWBitmap_Pool;

//=====================================================================================
//	geWBitmap_Pool_Create
//=====================================================================================
geWBitmap_Pool *geWBitmap_Pool_Create(GBSP_BSPData *BSPData)
{
	geWBitmap_Pool		*Pool;

	assert(BSPData);

	Pool = GE_RAM_ALLOCATE_STRUCT(geWBitmap_Pool);

	if (!Pool)
	{
		geErrorLog_AddString(-1, "geWBitmap_Pool_Create:  Could not create the Pool.  Out of memory.", NULL);
		goto ExitWithError;
	}

	// Clear out the memory for the Pool
	ZeroMem(Pool);				

	// Create all the bitmaps from the bspdata... (this should eventually just stream right out of the file...)
	if (!geWBitmap_Pool_CreateAllWBitmaps(Pool, BSPData))
	{
		geErrorLog_AddString(-1, "geWBitmap_Pool_Create:  geWBitmap_Pool_CreateAllWBitmaps failed.", NULL);
		goto ExitWithError;
	}

	return Pool;					// Done, good to go

	// Error
	ExitWithError:
	{
		if (Pool)
			geWBitmap_Pool_Destroy(Pool);

		return NULL;
	}
}

//=====================================================================================
//	geWBitmap_Pool_Destroy
//=====================================================================================
void geWBitmap_Pool_Destroy(geWBitmap_Pool *Pool)
{
	assert(Pool);

	geWBitmap_Pool_DestroyAllWBitmaps(Pool);

	geRam_Free(Pool);
}

//=====================================================================================
//	geWBitmap_PoolGetWBitmapCount
//=====================================================================================
int32 geWBitmap_Pool_GetWBitmapCount(geWBitmap_Pool *Pool)
{
	assert(Pool);

	return Pool->NumWBitmaps;
}

//=====================================================================================
//	geWBitmap_Pool_GetBitmapByIndex
//=====================================================================================
geWBitmap *geWBitmap_Pool_GetWBitmapByIndex(geWBitmap_Pool *Pool, int32 Index)
{
	assert(Pool);
	assert(Index >= 0);
	assert(Index < Pool->NumWBitmaps);

	return &Pool->WBitmaps[Index];
}

//=====================================================================================
//	geWBitmap_Pool_GetWBitmapByBitmap
//=====================================================================================
geWBitmap *geWBitmap_Pool_GetWBitmapByBitmap(geWBitmap_Pool *Pool, const geBitmap *Bitmap)
{
	geWBitmap		*pWBitmap;
	int32			i;

	assert(Pool);
	assert(Bitmap);

	pWBitmap = Pool->WBitmaps;

	for (i=0; i< Pool->NumWBitmaps; i++, pWBitmap++)
	{
		if (pWBitmap->Bitmap == Bitmap)
			return pWBitmap;
	}

	return NULL;
}

//=====================================================================================
//	geWBitmap_Pool_GetBitmapByIndex
//=====================================================================================
geBitmap *geWBitmap_Pool_GetBitmapByIndex(geWBitmap_Pool *Pool, int32 Index)
{
	assert(Pool);
	assert(Index >= 0);
	assert(Index < Pool->NumWBitmaps);
	assert(Pool->WBitmaps[Index].Bitmap);

	return Pool->WBitmaps[Index].Bitmap;
}

//=====================================================================================
//	geWBitmap_Pool_GetBitmapByName
//=====================================================================================
geBitmap *geWBitmap_Pool_GetBitmapByName(geWBitmap_Pool *Pool, const char *BitmapName)
{
	geWBitmap		*pWBitmap;
	int32			i;

	assert(Pool);
	assert(BitmapName);

	pWBitmap = Pool->WBitmaps;
	for (i=0; i< Pool->NumWBitmaps; i++, pWBitmap++)
	{
		if (!stricmp(pWBitmap->Name, BitmapName))
			return pWBitmap->Bitmap;				// Found it
	}

	return NULL;
}

#define	MAX_MIPS_ALLOWED	4

//=====================================================================================
//	geWBitmap_Pool_CreateAllWBitmaps
//=====================================================================================
geBoolean geWBitmap_Pool_CreateAllWBitmaps(geWBitmap_Pool *Pool, GBSP_BSPData *BSPData)
{
	int32		i;
	geWBitmap	*pWBitmap;
	GFX_Texture	*pGFXTexture;
	uint8		*BitmapIsTransparent;
	GFX_Face	*pFace;
	geBoolean	UseColorKey;
	uint32		ColorKey;

	assert(Pool);
	assert(BSPData);
	assert(Pool->NumWBitmaps == 0);
	assert(Pool->WBitmaps == NULL);

	if (!BSPData->NumGFXTextures)
		return GE_TRUE;

	assert(BSPData->GFXTextures);

	// BitmapIsTransparent is a temporary array, that is filled in with a 1, if any face that uses it, has the
	//	TEXINFO_TRANS flag set.  If they expect to "see" thru the surface, then they should have set this flag 
	//	in the editor.  If this flag is not set, then we won't allow a color key on the surface...
	BitmapIsTransparent = GE_RAM_ALLOCATE_ARRAY(uint8, BSPData->NumGFXTextures);

	if (!BitmapIsTransparent)
	{
		geErrorLog_AddString(-1, "geWBitmap_Pool_CreateAllWBitmaps:  Could not create the BitmapIsTransparent array.  Out of memory.", NULL);
		goto ExitWithError;
	}

	memset(BitmapIsTransparent,0,BSPData->NumGFXTextures);

	// Now, go mark all the bitmaps that have transparent set in the texinfo
	pFace = BSPData->GFXFaces;
	for (i=0; i< BSPData->NumGFXFaces; i++, pFace++)
	{
		GFX_TexInfo		*pTexInfo;

		pTexInfo = &BSPData->GFXTexInfo[pFace->TexInfo];

		assert(pTexInfo->Texture >= 0 || pTexInfo->Texture < BSPData->NumGFXTextures);

		if (pTexInfo->Flags & TEXINFO_TRANS)	
		{
			BitmapIsTransparent[pTexInfo->Texture] = 1;		// This face has the transparent value set, so mark the bitmap it references
		}
	}

	// Create the bitmap pool.
	Pool->WBitmaps = GE_RAM_ALLOCATE_ARRAY(geWBitmap, BSPData->NumGFXTextures);

	if (!Pool->WBitmaps)
	{
		geErrorLog_AddString(-1, "geWBitmap_Pool_CreateAllWBitmaps:  Could not create the Pool bitmaps.  Out of memory.", NULL);
		goto ExitWithError;
	}

	// Clear the bitmap array
	memset(Pool->WBitmaps,0,sizeof(geWBitmap)*BSPData->NumGFXTextures);
	Pool->NumWBitmaps = BSPData->NumGFXTextures;		// Store the number of slots

	pWBitmap = Pool->WBitmaps;
	pGFXTexture = BSPData->GFXTextures;

	for (i=0; i< BSPData->NumGFXTextures; i++, pGFXTexture++, pWBitmap++)
	{
		geBitmap	*Mips[MAX_MIPS_ALLOWED];
		int32		NumMips, m, Width, Height, Stride;
		uint8		*pSrc;

		if (BitmapIsTransparent[i])
		{
			UseColorKey = GE_TRUE;
			ColorKey = 255;
		}
		else
		{
			UseColorKey = GE_FALSE;
			ColorKey = 0;
		}

		strcpy(pWBitmap->Name, pGFXTexture->Name);

		// <> CB this is the main loop where GFX BSP textures are made into bitmaps

		// Get width and height
		Width = pGFXTexture->Width;
		Height = pGFXTexture->Height;
		
		if (pGFXTexture->Flags & TEXTURE_SKYBOX)
			NumMips = 1;
		else
			NumMips = 4;

		assert(NumMips <= MAX_MIPS_ALLOWED);

		// Create the bitmap
		pWBitmap->Bitmap = geBitmap_Create(Width, Height, NumMips, GE_PIXELFORMAT_8BIT);

		if (!pWBitmap->Bitmap)
		{
			geErrorLog_AddString(-1, "geWBitmap_Pool_CreateAllWBitmaps:  Could not create geBitmap.", NULL);
			goto ExitWithError;
		}

		// Set the driver flags
		if (pGFXTexture->Flags & TEXTURE_SKYBOX)
		{
			geBitmap_SetDriverFlags(pWBitmap->Bitmap, RDRIVER_PF_3D);	// Skybox textures do not need to combine with lightmaps
		}
		else
		{
			geBitmap_SetDriverFlags(pWBitmap->Bitmap, RDRIVER_PF_3D | RDRIVER_PF_COMBINE_LIGHTMAP);
		}

		//<> 
		NumMips = 1;
		// Lock all the miplevels
		if (!geBitmap_LockForWrite(pWBitmap->Bitmap, Mips, 0, NumMips-1))
		{
			geErrorLog_AddString(-1, "geWBitmap_Pool_CreateAllWBitmaps:  geBitmap_LockForWrite failed.", NULL);
			goto ExitWithError;
		}

		// Get the src from the .bsp texture data
		pSrc = &BSPData->GFXTexData[pGFXTexture->Offset];

		// Fill all the mip levels
		for (m=0; m< NumMips; m++)
		{
			uint8			*pDest;
			geBitmap_Info	Info;

			if (!geBitmap_GetInfo(Mips[m], &Info, NULL))
			{
				geErrorLog_AddString(-1, "geWBitmap_Pool_CreateAllWBitmaps:  geBitmap_GetInfo failed.", NULL);
				goto ExitWithError;
			}

			pDest = geBitmap_GetBits(Mips[m]);
			assert( pDest );

			Stride = Info.Stride;
			Width  = Info.Width;
			Height = Info.Height;

			assert(Stride >= Width);

			if ( Stride == Width )
			{
				memcpy(pDest,pSrc,Width*Height);
			}
			else
			{
			int h;
				for (h=Height;h--;)
				{
					memcpy(pDest,pSrc,Width);
					pSrc += Width;
					pDest += Stride;
				}
			}

			// Unlock this mip level, it is filled with the data
			if (!geBitmap_UnLock(Mips[m]))
			{
				geErrorLog_AddString(-1, "geWBitmap_Pool_CreateAllWBitmaps:  geBitmap_Unlock failed.", NULL);
				goto ExitWithError;
			}
		}

		// Create the palette...
		{
			geBitmap_Palette		*Pal;
			int32					PalSize,cnt;
			gePixelFormat			Format;
			uint32					*DstPalPtr,*DstPalData;
			DRV_RGB					*SrcPalPtr;

			//Pal = geBitmap_Palette_Create(GE_PIXELFORMAT_32BIT_ARGB, 256);
			Pal = geBitmap_Palette_Create(GE_PIXELFORMAT_32BIT_XRGB, 256);

			if (!Pal)
			{
				geErrorLog_AddString(-1, "geWBitmap_Pool_CreateAllWBitmaps:  geBitmap_Palette_Create failed.", NULL);
				return GE_FALSE;
			}
			
			if (!geBitmap_Palette_Lock(Pal, &DstPalData, &Format, &PalSize))
			{
				geErrorLog_AddString(-1, "geWBitmap_Pool_CreateAllWBitmaps:  geBitmap_Palette_Lock failed.", NULL);
				return GE_FALSE;
			}

			//cnt = sizeof(DRV_Palette); 
			// <> this doesn't seem to respect the pragma pack in dcommon !
				// I get size == 768

			SrcPalPtr = BSPData->GFXPalettes[pGFXTexture->PaletteIndex];
			DstPalPtr = DstPalData;
			for(cnt=PalSize;cnt--;)
			{
				*DstPalPtr++ = ((uint32)0xFF000000) + ((SrcPalPtr->r)<<16) + ((SrcPalPtr->g)<<8) + (SrcPalPtr->b);
				SrcPalPtr ++;
			}
			//debug to see if anyone is using entry 255 :
			// DstPalData[255] = 255;	// color key (it's bright blue & transparent)
			//or black it : DstPalData[255] = 0;

			if (!geBitmap_Palette_UnLock(Pal))
			{
				geErrorLog_AddString(-1, "geWBitmap_Pool_CreateAllWBitmaps:  geBitmap_Palette_UnLock failed.", NULL);
				return GE_FALSE;
			}

 			if (!geBitmap_SetPalette(pWBitmap->Bitmap, Pal))
			{
				geErrorLog_AddString(-1, "geWBitmap_Pool_CreateAllWBitmaps:  geBitmap_SetPalette failed.", NULL);
				return GE_FALSE;
			}

			geBitmap_Palette_Destroy(&Pal);
		} //done making the palette

		if (!geBitmap_SetColorKey(pWBitmap->Bitmap, UseColorKey, ColorKey, GE_TRUE))
			{
				geErrorLog_AddString(-1, "geWBitmap_Pool_CreateAllWBitmaps:  geBitmap_SetColorKey failed.", NULL);
				goto ExitWithError;
			}

	}

	

	return GE_TRUE;

	// Error
	ExitWithError:
	{
		if (Pool->WBitmaps)
		{
			geWBitmap_Pool_DestroyAllWBitmaps(Pool);
			Pool->WBitmaps = NULL;
		}

		if (BitmapIsTransparent)
		{
			geRam_Free(BitmapIsTransparent);
			BitmapIsTransparent = NULL;
		}

		return GE_FALSE;
	}
}

//=====================================================================================
//	geWBitmap_Pool_DestroyAllWBitmaps
//=====================================================================================
void geWBitmap_Pool_DestroyAllWBitmaps(geWBitmap_Pool *Pool)
{
	assert(Pool);

	if (Pool->WBitmaps)
	{
		int32			i;
		geWBitmap		*pWBitmap;

		pWBitmap = Pool->WBitmaps;
		
		for (i=0; i< Pool->NumWBitmaps; i++, pWBitmap++)
		{
			// Destroy the geBitmap 
			if (pWBitmap->Bitmap)
			{
				geBitmap_Destroy(&pWBitmap->Bitmap);
				pWBitmap->Bitmap = NULL;
			}
		}

		geRam_Free(Pool->WBitmaps);
	}

	Pool->WBitmaps = NULL;	// Just to be sure
}

//=====================================================================================
//	geWBitmap_GetFlags
//=====================================================================================
uint32 geWBitmap_GetFlags(geWBitmap *WBitmap)
{
	assert(WBitmap);

	return WBitmap->Flags;
}

//=====================================================================================
//	geWBitmap_GetBitmap
//=====================================================================================
geBitmap *geWBitmap_GetBitmap(geWBitmap *WBitmap)
{
	assert(WBitmap);

	return WBitmap->Bitmap;
}

//=====================================================================================
//	geWBitmap_GetVisFrame
//=====================================================================================
int32 geWBitmap_GetVisFrame(geWBitmap *WBitmap)
{
	assert(WBitmap);

	return WBitmap->VisFrame;
}

//=====================================================================================
//	geWBitmap_SetVisFrame
//=====================================================================================
geBoolean geWBitmap_SetVisFrame(geWBitmap *WBitmap, int32 VisFrame)
{
	assert(WBitmap);

	WBitmap->VisFrame = VisFrame;

	return GE_TRUE;
}
