/****************************************************************************************/
/*  Bitmap_BlitData.c                                                                   */
/*                                                                                      */
/*  Author: Charles Bloom                                                               */
/*  Description:  The Bitmap_BlitData function                                          */
/*					Does all format conversions											*/
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

/*}{*********************************************************************/

#include	<math.h>
#include	<time.h>
#include	<stdio.h>
#include	<assert.h>
#include	<stdlib.h>
#include	<string.h>

#include	"basetype.h"
#include	"getypes.h"
#include	"ram.h"

#include	"bitmap.h"
#include	"bitmap._h"
#include	"bitmap.__h"
#include	"bitmap_blitdata.h"

#include	"vfile.h"
#include	"ErrorLog.h"

#include	"palcreate.h"
#include	"palettize.h"

#include	"tsc.h"

#ifdef DO_TIMER
#include	"timer.h"
#endif

//#define DONT_USE_ASM

/*}{*********************************************************************/

// parameters to the main BlitData call are set up in here & shared
//  with all the children functions

// this may actually be better than being thread-friendly
// because we prevent cache-thrashing

static int SrcXtra,DstXtra;
static int SrcPelBytes,DstPelBytes;
static int SrcRowBytes,DstRowBytes;
static int SrcXtraBytes,DstXtraBytes;
static const gePixelFormat_Operations *SrcOps,*DstOps;
static gePixelFormat SrcFormat,DstFormat;

static const geBitmap_Info * SrcInfo;
static		 geBitmap_Info * DstInfo;
static const void *SrcData;
static		 void *DstData;
static const geBitmap *SrcBmp;
static		 geBitmap *DstBmp;
static const geBitmap_Palette *SrcPal;
static		 geBitmap_Palette *DstPal;
static int SizeX,SizeY;

static gePixelFormat_Decomposer		SrcDecomposePixel;
static gePixelFormat_Composer		DstComposePixel;
static gePixelFormat_ColorGetter	SrcGetColor;
static gePixelFormat_ColorPutter	DstPutColor;
static gePixelFormat_PixelGetter	SrcGetPixel;
static gePixelFormat_PixelPutter	DstPutPixel;

/*}{*********************************************************************/

geBoolean BlitData_Raw(void);
geBoolean BlitData_SameFormat(void);
geBoolean BlitData_Palettize(void);
geBoolean BlitData_DePalettize(void);
geBoolean BlitData_FromSeparateAlpha(void);
geBoolean BlitData_ToSeparateAlpha(void);

geBoolean geBitmap_BlitData_Sub(	const geBitmap_Info * iSrcInfo,const void *iSrcData, const geBitmap *iSrcBmp,
								geBitmap_Info * iDstInfo,void *iDstData,	const geBitmap *iDstBmp,
								int iSizeX,int iSizeY)
{

	// warming up...

	SrcInfo = iSrcInfo;
	DstInfo = iDstInfo;
	SrcData = iSrcData;
	DstData = iDstData;
	SrcBmp  = iSrcBmp;
	DstBmp  = (geBitmap *)iDstBmp;
	SizeX	= iSizeX;
	SizeY	= iSizeY;

	assert(SrcInfo && SrcData && DstInfo && DstData);

	// SrcData & DstData may be the same!
	// SrcBmp  & DstBmp  may be NULL !

	if ( SizeX > SrcInfo->Width || SizeX > DstInfo->Width ||
		 SizeY > SrcInfo->Height|| SizeY > DstInfo->Height)
	{
		geErrorLog_AddString(-1,"Bitmap_BlitData : size mismatch", NULL);	
		return GE_FALSE;
	}

	SrcFormat = SrcInfo->Format;
	DstFormat = DstInfo->Format;

	SrcOps = gePixelFormat_GetOperations(SrcFormat);
	DstOps = gePixelFormat_GetOperations(DstFormat);

	if ( ! SrcOps || ! DstOps )
		{
			geErrorLog_AddString(-1,"Bitmap_BlitData: SrcOps != DstOps",NULL);
			return GE_FALSE;
		}
		
	SrcPelBytes = SrcOps->BytesPerPel;
	DstPelBytes = DstOps->BytesPerPel;

	SrcRowBytes = SrcPelBytes * SrcInfo->Stride;
	DstRowBytes = DstPelBytes * DstInfo->Stride;

	SrcXtra = SrcInfo->Stride - SizeX;
	DstXtra = DstInfo->Stride - SizeX;

	SrcXtraBytes = SrcXtra * SrcPelBytes;
	DstXtraBytes = DstXtra * DstPelBytes;

	DstComposePixel		= DstOps->ComposePixel;
	DstPutColor			= DstOps->PutColor;
	DstPutPixel			= DstOps->PutPixel;
	SrcDecomposePixel	= SrcOps->DecomposePixel;
	SrcGetColor			= SrcOps->GetColor;
	SrcGetPixel			= SrcOps->GetPixel;

	SrcPal = SrcInfo->Palette;
	if ( ! SrcPal && SrcBmp )
		SrcPal = geBitmap_GetPalette(SrcBmp);
	DstPal = DstInfo->Palette;
	if ( ! DstPal && DstBmp )
		DstPal = geBitmap_GetPalette(DstBmp);

	// all systems go!

	/** copy the palette **/

	if ( gePixelFormat_HasPalette(SrcFormat) && ! SrcPal )
	{
		geErrorLog_AddString(-1, "geBitmap_BlitData:  Palettized format, with no palette.", NULL);
		return GE_FALSE;
	}

	if ( SrcPal && gePixelFormat_HasPalette(DstFormat) )
	{
		if ( ! DstInfo->Palette )
		{
		gePixelFormat Format;
			Format = SrcPal->Format;
			if ( ! gePixelFormat_HasAlpha(Format) )
			{
				if ( SrcInfo->HasColorKey && ! DstInfo->HasColorKey )
					Format = GE_PIXELFORMAT_32BIT_ARGB;
			}
			geBitmap_AllocPalette(DstBmp,Format,DstBmp->Driver);
			if ( ! DstInfo->Palette )
				DstInfo->Palette = geBitmap_GetPalette(DstBmp);
		}
		DstPal = DstInfo->Palette;
		if ( ! DstPal )
		{
			geErrorLog_AddString(-1, "geBitmap_BlitData:  couldn't alloc new dest palette.", NULL);
			return GE_FALSE;
		}

		if ( ! geBitmap_Palette_Copy(SrcPal,DstPal) )
		{
			geErrorLog_AddString(-1,"Bitmap_BlitData : Palette_Copy failed", NULL);
			return GE_FALSE;
		}
		
		if ( SrcInfo->HasColorKey )
		{
			if ( ! geBitmap_Palette_SetEntryColor(DstInfo->Palette,SrcInfo->ColorKey,0,0,0,0) )
			{
				geErrorLog_AddString(-1,"Bitmap_BlitData: geBitmap_Palette_SetEntryColor failed",NULL);
				return GE_FALSE;
			}
		}
	}

	/****
	**
	*
		modes:
			0. types are same
			1. Wavelet <-> raw
			2. Pal -> raw (easy)	(now raw means "not wavelet or pal")
			3. raw -> Pal (hard)
			4. raw <-> raw (just bit-ops)

		each of these also exists for separate alpha types
	*
	**
	 ****/

	/****/
	
	if (	SrcBmp && SrcBmp->Alpha && SrcBmp->Alpha->LockOwner && 
		DstBmp && DstBmp->Alpha && DstBmp->Alpha->LockOwner )
	{
		if ( ! geBitmap_BlitBitmap(SrcBmp->Alpha,DstBmp->Alpha) )
			{
				geErrorLog_AddString(-1,"geBitmap_BlitData:  geBitmap_BlitBitmap Failed",NULL);
				return GE_FALSE;
			}
		// now continue through to blit the main bitmap
	}
	else if ( SrcBmp && SrcBmp->Alpha && SrcBmp->Alpha->LockOwner && 
			( gePixelFormat_HasAlpha(DstFormat) || DstInfo->HasColorKey ) )
	{
		// there is no separate alpha -> color key conversion
		// note that we cannot add separate alpha -> CK because the 
		// separate-alpha *target* type has colorkey too !
		if (BlitData_FromSeparateAlpha()==GE_FALSE)
			{
				geErrorLog_AddString(-1,"geBitmap_BlitData:  geBitmap_FromSeparateAlpha Failed",NULL);
				return GE_FALSE;
			}
		else
			return GE_TRUE;

	}
	else if ( DstBmp && DstBmp->Alpha && DstBmp->Alpha->LockOwner && 
			gePixelFormat_HasAlpha(SrcFormat) )
	{
		// there is no separate alpha -> color key conversion
		// note that we cannot add separate alpha -> CK because the 
		// separate-alpha *target* type has colorkey too !
		if (BlitData_ToSeparateAlpha()==GE_FALSE)
			{
				geErrorLog_AddString(-1,"geBitmap_BlitData:  geBitmap_ToSeparateAlpha Failed",NULL);
				return GE_FALSE;
			}
		else
			return GE_TRUE;
	}

	/****/

	if (	SrcFormat == GE_PIXELFORMAT_WAVELET ||
			DstFormat == GE_PIXELFORMAT_WAVELET )
	{
		geErrorLog_AddString(-1,"Bitmap_BlitData : no wavelets in Genesis 1.0", NULL);	
		return GE_FALSE;
	}
	else if ( SrcFormat == DstFormat )
	{
		if (BlitData_SameFormat()==GE_FALSE)
			{
				geErrorLog_AddString(-1,"geBitmap_BlitData: BlitData_SameFormat failed",NULL);
				return GE_FALSE;
			}
		else
			return GE_TRUE;
	}
	else if (	gePixelFormat_HasPalette(SrcFormat) ||
				gePixelFormat_HasPalette(DstFormat) )
	{
		assert(SrcFormat != DstFormat);
		if (	gePixelFormat_HasPalette(SrcFormat) &&
				gePixelFormat_HasPalette(DstFormat) )
			{
				geErrorLog_AddString(-1,"geBitmap_BlitData:  two different palettized.",NULL);
				return GE_FALSE;	// already picked up by SameFormat , or two different palettized = abort!
			}

		if ( gePixelFormat_HasPalette(SrcFormat) )
		{
			if (BlitData_DePalettize()==GE_FALSE)
				{
					geErrorLog_AddString(-1,"geBitmap_BlitData:  BlitData_DePalettize() failed.",NULL);
					return GE_FALSE;	
				}
			else
				return GE_TRUE;
		}
		else
		{
			if ( ! DstInfo->Palette )
			{
				// make it
				if ( DstBmp && DstBmp->Driver )
				{
				gePixelFormat Format;
					Format = SrcInfo->Palette ? SrcInfo->Palette->Format : GE_PIXELFORMAT_32BIT_XRGB;
					if ( ! gePixelFormat_HasAlpha(Format) )
					{
						if ( SrcInfo->HasColorKey && ! DstInfo->HasColorKey )
							Format = GE_PIXELFORMAT_32BIT_ARGB;
					}
					geBitmap_AllocPalette(DstBmp,Format,DstBmp->Driver);
					if ( ! DstInfo->Palette )
						DstInfo->Palette = geBitmap_GetPalette(DstBmp);
				}
				else
				{
					DstInfo->Palette = geBitmap_Palette_Create(PALETTE_FORMAT_DEFAULT,256);
				}
				if ( ! DstInfo->Palette )
				{
					geErrorLog_AddString(-1,"Bitmap_BlitData : Pal create failed", NULL);	
					return GE_FALSE;
				}

				if ( SrcPal )
				{
					geBitmap_Palette_Copy(SrcPal,DstInfo->Palette);
				}
				else if ( DstPal )
				{
					geBitmap_Palette_Copy(DstPal,DstInfo->Palette);
				}
				else // Nobody had a palette !
				{
				geBitmap_Palette * NewPal;
				geBitmap_Info Info;
					Info = *SrcInfo;
					Info.Width = SizeX;
					Info.Height = SizeY;
					NewPal = createPalette(&Info,SrcData);
					if ( ! NewPal )
					{
						geErrorLog_AddString(-1,"Bitmap_BlitData : createPalette failed", NULL);	
						return GE_FALSE;
					}
					geBitmap_Palette_Copy(NewPal,DstInfo->Palette);
					if ( SrcBmp && ((SizeX*SizeY) > ((SrcInfo->Width * SrcInfo->Height)>>2)) )
					{
						geBitmap_SetPalette((geBitmap *)SrcBmp,NewPal);
					}
					geBitmap_Palette_Destroy(&NewPal);
				}

				DstPal = DstInfo->Palette;

				SrcPal = SrcInfo->Palette;
				if ( ! SrcPal && SrcBmp )
					SrcPal = geBitmap_GetPalette(SrcBmp);
			}

			if (BlitData_Palettize()==GE_FALSE)
				{
					geErrorLog_AddString(-1,"geBitmap_BlitData:  BlitData_Palettize failed.",NULL);
					return GE_FALSE;	
				}
			else
				return GE_TRUE;
		}
	}
	else
	{
		if (BlitData_Raw()==GE_FALSE)
			{
				geErrorLog_AddString(-1,"geBitmap_BlitData:  BlitData_Raw failed.",NULL);
				return GE_FALSE;	
			}
		else
			return GE_TRUE;	
	}

	assert(0);
	// must have returned before here
	//return GE_FALSE;
}

geBoolean geBitmap_BlitData(	const geBitmap_Info * iSrcInfo,const void *iSrcData, const geBitmap *iSrcBmp,
								geBitmap_Info * iDstInfo,void *iDstData,	const geBitmap *iDstBmp,
								int iSizeX,int iSizeY)
{
geBoolean Ret;

	Ret = geBitmap_BlitData_Sub(	
							iSrcInfo,iSrcData,iSrcBmp,
							iDstInfo,iDstData,iDstBmp,	
							iSizeX,iSizeY);

return Ret;
}

/*}{*********************************************************************/

geBoolean BlitData_Raw(void)
{
int x,y;
char *SrcPtr,*DstPtr;
int R,G,B,A;
uint32 ColorKey,Pixel;

	if ( ! SrcOps || ! DstOps )
		return GE_FALSE;

	SrcPtr = (char *)SrcData;
	DstPtr = (char *)DstData;

	// this generic converter is pretty slow.
	// fortunately genesis uses mostly the (Pal -> UnPal) conversion
	// or the (Wavelet -> UnPal) conversion

	if ( SrcPelBytes == 0 || DstPelBytes == 0 ) 
	{
		geErrorLog_AddString(-1,"Bitmap_BlitData : invalid format", NULL);
		return GE_FALSE;
	}
	else if ( SrcOps->AMask && ! (DstOps->AMask) && DstInfo->HasColorKey )
	{
		ColorKey = DstInfo->ColorKey;

		// special case for "Src has alpha & Dst doesn't, but has ColorKey"

		for(y=SizeY;y--;)
		{
			for(x=SizeX;x--;)
			{
				SrcGetColor(&SrcPtr,&R,&G,&B,&A);
				if ( A < ALPHA_TO_TRANSPARENCY_THRESHOLD )
				{
					Pixel = ColorKey;
				}
				else
				{
					Pixel = DstComposePixel(R,G,B,A);
					if ( Pixel == ColorKey )
					{
						Pixel ^= 1;
					}
				}
				DstPutPixel(&DstPtr,Pixel);
			}
			SrcPtr += SrcXtraBytes;
			DstPtr += DstXtraBytes;
		}

	return GE_TRUE;
	}
	else if ( SrcInfo->HasColorKey && DstInfo->HasColorKey )
	{
	uint32 DstColorKey;

		ColorKey = SrcInfo->ColorKey;
		DstColorKey = DstInfo->ColorKey;

		for(y=SizeY;y--;)
		{
			for(x=SizeX;x--;)
			{
				Pixel = SrcGetPixel(&SrcPtr);
				if ( Pixel == ColorKey )
				{
					DstPutPixel(&DstPtr,DstColorKey);
				}
				else
				{
					SrcDecomposePixel(Pixel,&R,&G,&B,&A);
					Pixel = DstComposePixel(R,G,B,A);
					if ( Pixel == DstColorKey )
						Pixel ^= 1;
					DstPutPixel(&DstPtr,Pixel);
				}
			}
			SrcPtr += SrcXtraBytes;
			DstPtr += DstXtraBytes;
		}

	return GE_TRUE;
	}
	else if ( DstInfo->HasColorKey )
	{
		ColorKey = DstInfo->ColorKey;

		for(y=SizeY;y--;)
		{
			for(x=SizeX;x--;)
			{
				SrcGetColor(&SrcPtr,&R,&G,&B,&A);
				Pixel = DstComposePixel(R,G,B,A);
				if ( Pixel == ColorKey )
				{
					Pixel ^= 1;
				}
				DstPutPixel(&DstPtr,Pixel);
			}
			SrcPtr += SrcXtraBytes;
			DstPtr += DstXtraBytes;
		}

	return GE_TRUE;
	}
	else if ( SrcInfo->HasColorKey )
	{
		// generic converter does the cases we don't understand

		ColorKey = SrcInfo->ColorKey;

		for(y=SizeY;y--;)
		{
			for(x=SizeX;x--;)
			{
				Pixel = SrcGetPixel(&SrcPtr);
				if ( Pixel == ColorKey )
				{
					DstPutColor(&DstPtr,0,0,0,0);
				}
				else
				{
					SrcDecomposePixel(Pixel,&R,&G,&B,&A);
					DstPutColor(&DstPtr,R,G,B,A);
				}
			}
			SrcPtr += SrcXtraBytes;
			DstPtr += DstXtraBytes;
		}

	return GE_TRUE;
	}
	else
	{
		for(y=SizeY;y--;)
		{
			for(x=SizeX;x--;)
			{
				SrcGetColor(&SrcPtr,&R,&G,&B,&A);
				DstPutColor(&DstPtr,R,G,B,A);
			}
			SrcPtr += SrcXtraBytes;
			DstPtr += DstXtraBytes;
		}

	return GE_TRUE;
	}
}

/*}{*********************************************************************/

geBoolean BlitData_FromSeparateAlpha(void)
{
geBitmap_Info AlphaInfo;
void * AlphaData;
uint8 *SrcPtr,*DstPtr,*AlphaPtr;
int x,y,R,G,B,A;
uint32 ColorKey,Pixel;
int AlphaXtra;

	/*******
	**
		support the extra Alpha Bmp
		the common case is 8bit + 8bit -> 4444

		we're pretty lazy about this; it's not optimized for speed

	**
	 ******/

	#pragma message("Bitmap_BlitData: inconsistent handling of separates with color keys!")

	SrcPtr = (uint8 *)SrcData;
	DstPtr = (uint8 *)DstData;

	if ( ! geBitmap_GetInfo(SrcBmp->Alpha,&AlphaInfo,NULL) )
		return GE_FALSE;
	if ( AlphaInfo.Format != GE_PIXELFORMAT_8BIT_GRAY )
	{
		geErrorLog_AddString(-1,"Bitmap_BlitData : Alpha must be grayscale", NULL);
		return GE_FALSE;
	}

	AlphaData = geBitmap_GetBits(SrcBmp->Alpha);
	if ( ! AlphaData )
		return GE_FALSE;

	AlphaPtr = (uint8 *)AlphaData;
	AlphaXtra = AlphaInfo.Stride - SizeX;

	if ( gePixelFormat_HasPalette(SrcFormat) )
	{
		if ( SrcFormat == DstFormat )
		{
		uint8 Pixel,DstColorKey;

			assert(DstInfo->HasColorKey);
			
			DstColorKey = (uint8)DstInfo->ColorKey;
			for(y=SizeY;y--;)
			{
				for(x=SizeX;x--;)
				{
					Pixel = *SrcPtr++;
					A = *AlphaPtr++;
					if ( A < ALPHA_TO_TRANSPARENCY_THRESHOLD )
						*DstPtr++ = DstColorKey;
					else
						*DstPtr++ = Pixel;
				}
				SrcPtr += SrcXtra;
				DstPtr += DstXtraBytes;
				AlphaPtr += AlphaXtra;
			}
			return GE_TRUE;
		}
		else if ( SrcFormat == GE_PIXELFORMAT_8BIT )
		{
		uint8 *PalPtr,PalData[768];
		int pal;

			if ( ! geBitmap_Palette_GetData(SrcPal,PalData,GE_PIXELFORMAT_24BIT_RGB,256) )
				return GE_FALSE;

			// with seperate alpha

			if ( ! gePixelFormat_HasAlpha(DstFormat) && DstInfo->HasColorKey )
			{
			uint32 Pixel,DstColorKey;
				// source is palettized
				// dest has color key and no alpha
				DstColorKey = DstInfo->ColorKey;
				for(y=SizeY;y--;)
				{
					for(x=SizeX;x--;)
					{
						pal = *SrcPtr++;
						PalPtr = &PalData[3*pal];
						R = *PalPtr++;
						G = *PalPtr++;
						B = *PalPtr;
						A = *AlphaPtr++;
						if ( A < 128 )
						{
							DstPutPixel(&DstPtr,DstColorKey);
						}
						else
						{
							Pixel = DstComposePixel(R,G,B,255);
							if ( Pixel == DstColorKey )
								Pixel ^= 1;
							DstPutPixel(&DstPtr,Pixel);
						}
					}
					SrcPtr += SrcXtra;
					DstPtr += DstXtraBytes;
					AlphaPtr += AlphaXtra;
				}
			}
			else if ( DstInfo->HasColorKey )
			{
			uint32 Pixel,DstColorKey;
				// source is palettized
				// dest has alpha and color key
				DstColorKey = DstInfo->ColorKey;
				for(y=SizeY;y--;)
				{
					for(x=SizeX;x--;)
					{
						pal = *SrcPtr++;
						PalPtr = &PalData[3*pal];
						R = *PalPtr++;
						G = *PalPtr++;
						B = *PalPtr;
						A = *AlphaPtr++;
						Pixel = DstComposePixel(R,G,B,A);
						if ( Pixel == DstColorKey )
							Pixel ^= 1;
						DstPutPixel(&DstPtr,Pixel);
					}
					SrcPtr += SrcXtra;
					DstPtr += DstXtraBytes;
					AlphaPtr += AlphaXtra;
				}
			}
			else
			{
				// source is palettized
				// dest has alpha and no color key
				for(y=SizeY;y--;)
				{
					for(x=SizeX;x--;)
					{
						pal = *SrcPtr++;
						PalPtr = &PalData[3*pal];
						R = *PalPtr++;
						G = *PalPtr++;
						B = *PalPtr;
						A = *AlphaPtr++;
						DstPutColor(&DstPtr,R,G,B,A);
					}
					SrcPtr += SrcXtra;
					DstPtr += DstXtraBytes;
					AlphaPtr += AlphaXtra;
				}
			}

			return GE_TRUE;
		}
		else
		{
			return GE_FALSE;
		}
		assert("should not get here" == NULL);
	}
	else
	{
		// this generic converter is pretty slow.
		// fortunately genesis uses mostly the (Pal -> UnPal) conversion
		// or the (Wavelet -> UnPal) conversion

		// Src is not palettized

		assert( ! SrcOps->AMask );
		assert( DstOps->AMask || DstInfo->HasColorKey );

		// <> doesn't do -> palettize
		//	should never get a (separates)->(palettized) with current driver, but bad to assume...
		// perhaps the best thing is to do separates -> 32bitRGBA then do 32bitRGBA -> Dest with the normal converters

		if ( gePixelFormat_HasPalette(DstFormat) )
		{
			geErrorLog_AddString(-1,"Bitmap_BlitData : FromSeparateAlpha : doesn't do Palettize!", NULL);
			return GE_FALSE;
		}

		if ( SrcPelBytes == 0 ) 
		{
			geErrorLog_AddString(-1,"Bitmap_BlitData : FromSeparateAlpha : bad Src format", NULL);
			return GE_FALSE;
		}
		else if ( DstPelBytes == 0 || ! DstPutColor || ! DstComposePixel ) 
		{
			geErrorLog_AddString(-1,"Bitmap_BlitData : FromSeparateAlpha : bad Dst format", NULL);
			return GE_FALSE;
		}


		if ( DstOps->AMask )
		{

			//separates -> alpha

			if ( SrcInfo->HasColorKey && DstInfo->HasColorKey )
			{
			uint32 DstColorKey;

				ColorKey	= SrcInfo->ColorKey;
				DstColorKey	= DstInfo->ColorKey;

				// with seperate alpha

				for(y=SizeY;y--;)
				{
					for(x=SizeX;x--;)
					{
						Pixel = SrcGetPixel(&SrcPtr);
						if ( Pixel == ColorKey )
						{
							AlphaPtr++;
							DstPutPixel(&DstPtr,DstColorKey);
						}
						else
						{
							SrcDecomposePixel(Pixel,&R,&G,&B,&A);
							A = *AlphaPtr++;
							Pixel = DstComposePixel(R,G,B,A);
							if ( Pixel == DstColorKey )
								Pixel ^= 1;
							DstPutPixel(&DstPtr,Pixel);
						}
					}
					SrcPtr += SrcXtraBytes;
					DstPtr += DstXtraBytes;
					AlphaPtr += AlphaXtra;
				}

			return GE_TRUE;
			}
			else if ( DstInfo->HasColorKey )
			{
				ColorKey = DstInfo->ColorKey;

				// with seperate alpha

				for(y=SizeY;y--;)
				{
					for(x=SizeX;x--;)
					{
						SrcGetColor(&SrcPtr,&R,&G,&B,&A);
						A = *AlphaPtr++;
						Pixel = DstComposePixel(R,G,B,A);
						if ( Pixel == ColorKey )
						{
							Pixel ^= 1;
						}
						DstPutPixel(&DstPtr,Pixel);
					}
					SrcPtr += SrcXtraBytes;
					DstPtr += DstXtraBytes;
					AlphaPtr += AlphaXtra;
				}

			return GE_TRUE;
			}
			else if ( SrcInfo->HasColorKey )
			{
				// with seperate alpha

				ColorKey = SrcInfo->ColorKey;

				for(y=SizeY;y--;)
				{
					for(x=SizeX;x--;)
					{
						Pixel = SrcGetPixel(&SrcPtr);
						if ( Pixel == ColorKey )
						{
							AlphaPtr++;
							DstPutColor(&DstPtr,0,0,0,0);
						}
						else
						{
							SrcDecomposePixel(Pixel,&R,&G,&B,&A);
							A = *AlphaPtr++;
							DstPutColor(&DstPtr,R,G,B,A);
						}
					}
					SrcPtr += SrcXtraBytes;
					DstPtr += DstXtraBytes;
					AlphaPtr += AlphaXtra;
				}

			return GE_TRUE;
			}
			else
			{
				// with seperate alpha
				for(y=SizeY;y--;)
				{
					for(x=SizeX;x--;)
					{
						SrcGetColor(&SrcPtr,&R,&G,&B,&A);
						A = *AlphaPtr++;
						DstPutColor(&DstPtr,R,G,B,A);
					}
					SrcPtr += SrcXtraBytes;
					DstPtr += DstXtraBytes;
					AlphaPtr += AlphaXtra;
				}

			return GE_TRUE;
			}

		}
		else
		{
		uint32 DstColorKey;

			ColorKey	= SrcInfo->ColorKey;
			DstColorKey	= DstInfo->ColorKey;

			assert(DstInfo->HasColorKey);

			for(y=SizeY;y--;)
			{
				for(x=SizeX;x--;)
				{
					SrcGetColor(&SrcPtr,&R,&G,&B,&A);
					A = *AlphaPtr++;
					if ( A < 128 )
					{
						DstPutPixel(&DstPtr,DstColorKey);
					}
					else
					{
						Pixel = DstComposePixel(R,G,B,255);
						if ( Pixel == ColorKey )
							Pixel ^= 1;
						DstPutPixel(&DstPtr,Pixel);
					}
				}
				SrcPtr += SrcXtraBytes;
				DstPtr += DstXtraBytes;
				AlphaPtr += AlphaXtra;
			}

			return GE_TRUE;
		}
	}

	assert("should not get here" == NULL);
return GE_FALSE;
// end Seperate Alpha conversions
}

/*}{*********************************************************************/

geBoolean BlitData_ToSeparateAlpha(void)
{
geBitmap_Info AlphaInfo;
void * AlphaData;
uint8 *SrcPtr,*DstPtr,*AlphaPtr;
int x,y,R,G,B,A;
uint32 ColorKey,Pixel;
int AlphaXtra;

	/*******
	**
		support the extra Alpha Bmp
		the common case is (4444) -> (8bit + 8bit)

		we're pretty lazy about this; it's not optimized for speed

	**
	 ******/

	SrcPtr = (uint8 *)SrcData;
	DstPtr = (uint8 *)DstData;

	if ( ! geBitmap_GetInfo(DstBmp->Alpha,&AlphaInfo,NULL) )
		return GE_FALSE;
	if ( AlphaInfo.Format != GE_PIXELFORMAT_8BIT_GRAY )
	{
		geErrorLog_AddString(-1,"Bitmap_BlitData : Alpha must be grayscale", NULL);
		return GE_FALSE;
	}

	AlphaData = geBitmap_GetBits(DstBmp->Alpha);
	if ( ! AlphaData )
		return GE_FALSE;

	AlphaPtr = (uint8 *)AlphaData;
	AlphaXtra = AlphaInfo.Stride - SizeX;

	if ( gePixelFormat_HasPalette(DstFormat) )
	{
		// <>
		geErrorLog_AddString(-1,"BlitData : doesn't support blit to palettized separates now", NULL);
		// (alpha) -> pal + separate
		//	requires palettization !!
		return GE_FALSE;
	}
	else
	{
		// this generic converter is pretty slow.
		// fortunately genesis uses mostly the (Pal -> UnPal) conversion
		// or the (Wavelet -> UnPal) conversion

		assert( SrcOps->AMask && !(DstOps->AMask) );

		if ( SrcPelBytes == 0 || DstPelBytes == 0 ) 
		{
			geErrorLog_AddString(-1,"Bitmap_BlitData : bad formats", NULL);
			return GE_FALSE;
		}
		else if ( SrcInfo->HasColorKey && DstInfo->HasColorKey )
		{
		uint32 DstColorKey;

			ColorKey = SrcInfo->ColorKey;
			DstColorKey = DstInfo->ColorKey;

			// with seperate alpha

			for(y=SizeY;y--;)
			{
				for(x=SizeX;x--;)
				{
					Pixel = SrcGetPixel(&SrcPtr);
					if ( Pixel == ColorKey )
					{
						*AlphaPtr++ = 0;
						DstPutPixel(&DstPtr,DstColorKey);
					}
					else
					{
						SrcDecomposePixel(Pixel,&R,&G,&B,&A);
						Pixel = DstComposePixel(R,G,B,255);
						if ( Pixel == DstColorKey )	Pixel ^= 1;
						DstPutPixel(&DstPtr,Pixel);
						*AlphaPtr++ = A;
					}
				}
				SrcPtr += SrcXtraBytes;
				DstPtr += DstXtraBytes;
				AlphaPtr += AlphaXtra;
			}
		}
		else if ( DstInfo->HasColorKey )
		{
			ColorKey = DstInfo->ColorKey;

			// with seperate alpha

			for(y=SizeY;y--;)
			{
				for(x=SizeX;x--;)
				{
					SrcGetColor(&SrcPtr,&R,&G,&B,&A);
					Pixel = DstComposePixel(R,G,B,255);
					if ( Pixel == ColorKey ) Pixel ^= 1;
					*AlphaPtr++ = A;
					DstPutPixel(&DstPtr,Pixel);
				}
				SrcPtr += SrcXtraBytes;
				DstPtr += DstXtraBytes;
				AlphaPtr += AlphaXtra;
			}
		}
		else if ( SrcInfo->HasColorKey )
		{
			// with seperate alpha

			ColorKey = SrcInfo->ColorKey;

			for(y=SizeY;y--;)
			{
				for(x=SizeX;x--;)
				{
					Pixel = SrcGetPixel(&SrcPtr);
					if ( Pixel == ColorKey )
					{
						*AlphaPtr++ = 0;
						DstPutColor(&DstPtr,0,0,0,0);
					}
					else
					{
						SrcDecomposePixel(Pixel,&R,&G,&B,&A);
						DstPutColor(&DstPtr,R,G,B,255);
						*AlphaPtr++ = A;
					}
				}
				SrcPtr += SrcXtraBytes;
				DstPtr += DstXtraBytes;
				AlphaPtr += AlphaXtra;
			}
		}
		else
		{
			// with seperate alpha
			for(y=SizeY;y--;)
			{
				for(x=SizeX;x--;)
				{
					SrcGetColor(&SrcPtr,&R,&G,&B,&A);
					DstPutColor(&DstPtr,R,G,B,255);
					*AlphaPtr++ = A;
				}
				SrcPtr += SrcXtraBytes;
				DstPtr += DstXtraBytes;
				AlphaPtr += AlphaXtra;
			}
		}

		assert( AlphaPtr	== (((uint8 *)AlphaData) + AlphaInfo.Stride * SizeY) );
		assert( SrcPtr		== (((uint8 *)SrcData) + SrcRowBytes * SizeY ) );
		assert( DstPtr		== (((uint8 *)DstData) + DstRowBytes * SizeY ) );

	return GE_TRUE;
	}

	// end Seperate Alpha conversions

return GE_FALSE;
}

/*}{*********************************************************************/

geBoolean BlitData_SameFormat(void)
{
char *SrcPtr,*DstPtr;
gePixelFormat Format;

	Format = SrcFormat;
	SrcPtr = (char *)SrcData;
	DstPtr = (char *)DstData;

	if ( (!DstInfo->HasColorKey) || 
			( SrcInfo->HasColorKey && DstInfo->HasColorKey && SrcInfo->ColorKey == DstInfo->ColorKey ) )
	{
	int RowBytes,SrcStepBytes,DstStepBytes,y;
		// just a mem-copy, with strides
		
		RowBytes = SizeX * SrcPelBytes;
		SrcStepBytes = SrcXtraBytes + RowBytes;
		DstStepBytes = DstXtraBytes + RowBytes;
		for(y=SizeY;y--;)
		{
			memcpy( DstPtr, SrcPtr, RowBytes );
			SrcPtr += SrcStepBytes;
			DstPtr += DstStepBytes;
		}

		return GE_TRUE;
	}
	else // same format, different color key
	{
	int x,y;
	uint32 Pixel,DstColorKey;

		//this is common

		assert(DstInfo->HasColorKey);
		DstColorKey = DstInfo->ColorKey;
		
		if ( SrcInfo->HasColorKey )
		{
		uint32 SrcColorKey ;

			SrcColorKey = SrcInfo->ColorKey;

			assert(SrcColorKey != DstColorKey);
			
			// start : formats same, source & dest have different color key
			
			switch(SrcPelBytes)
			{
				default:
					return GE_FALSE;
				case 1:
				{
				uint8 *pSrc,*pDst;
					pSrc = (uint8 *)SrcPtr;
					pDst = (uint8 *)DstPtr;

					for(y=SizeY;y--;)
					{
						for(x=SizeX;x--;)
						{
							Pixel = *pSrc++;
							if ( Pixel == SrcColorKey )
								Pixel = DstColorKey;
							else if ( Pixel == DstColorKey )
								Pixel = SrcColorKey;
							*pDst++ = (uint8)Pixel;
						}
						pSrc += SrcXtra;
						pDst += DstXtra;
					}
					return GE_TRUE;
				}
				case 2:
				{
				uint16 *pSrc,*pDst;
					pSrc = (uint16 *)SrcPtr;
					pDst = (uint16 *)DstPtr;

					for(y=SizeY;y--;)
					{
						for(x=SizeX;x--;)
						{
							Pixel = *pSrc++;
							if ( Pixel == SrcColorKey )
								Pixel = DstColorKey;
							else if ( Pixel == DstColorKey )
								Pixel = SrcColorKey;
							*pDst++ = (uint16)Pixel;
						}
						pSrc += SrcXtra;
						pDst += DstXtra;
					}
					return GE_TRUE;
				}
				case 3:
				{
				uint8 *pSrc,*pDst;
					pSrc = (uint8 *)SrcPtr;
					pDst = (uint8 *)DstPtr;

					for(y=SizeY;y--;)
					{
						for(x=SizeX;x--;)
						{
							Pixel = (pSrc[0]<<16) + (pSrc[1]<<8) + pSrc[2];
							if ( Pixel == SrcColorKey )
								Pixel = DstColorKey;
							else if ( Pixel == DstColorKey )
								Pixel = SrcColorKey;
							pDst[0] = (uint8)(Pixel>>16);
							pDst[1] = (uint8)((Pixel>>8)&0xFF);
							pDst[2] = (uint8)(Pixel&0xFF);
							pSrc += 3;
							pDst += 3;
						}
						pSrc += SrcXtraBytes;
						pDst += DstXtraBytes;
					}
					return GE_TRUE;
				}
				case 4:
				{
				uint32 *pSrc,*pDst;
					pSrc = (uint32 *)SrcPtr;
					pDst = (uint32 *)DstPtr;

					for(y=SizeY;y--;)
					{
						for(x=SizeX;x--;)
						{
							Pixel = *pSrc++;
							if ( Pixel == SrcColorKey )
								Pixel = DstColorKey;
							else if ( Pixel == DstColorKey )
								Pixel = SrcColorKey;
							*pDst++ = Pixel;
						}
						pSrc += SrcXtra;
						pDst += DstXtra;
					}
					return GE_TRUE;
				}
			}

			// end : formats same, source & dest have different color key
		}
		else
		{
		
			// start : formats same, dest had color key, source doesn't

			switch(SrcPelBytes)
			{
				default:
					return GE_FALSE;
				case 1:
				{
				uint8 *pSrc,*pDst;
					pSrc = (uint8 *)SrcPtr;
					pDst = (uint8 *)DstPtr;

					for(y=SizeY;y--;)
					{
						for(x=SizeX;x--;)
						{
							Pixel = *pSrc++;
							if ( Pixel == DstColorKey )
								Pixel ^= 1;
							*pDst++ = (uint8)Pixel;
						}
						pSrc += SrcXtra;
						pDst += DstXtra;
					}
					return GE_TRUE;
				}
				case 2:
				{
				uint16 *pSrc,*pDst;
					pSrc = (uint16 *)SrcPtr;
					pDst = (uint16 *)DstPtr;

					for(y=SizeY;y--;)
					{
						for(x=SizeX;x--;)
						{
							Pixel = *pSrc++;
							if ( Pixel == DstColorKey )
								Pixel ^= 1;
							*pDst++ = (uint16)Pixel;
						}
						pSrc += SrcXtra;
						pDst += DstXtra;
					}
					return GE_TRUE;
				}
				case 3:
				{
				uint8 *pSrc,*pDst;
					pSrc = (uint8 *)SrcPtr;
					pDst = (uint8 *)DstPtr;

					for(y=SizeY;y--;)
					{
						for(x=SizeX;x--;)
						{
							Pixel = (pSrc[0]<<16) + (pSrc[1]<<8) + pSrc[2];
							if ( Pixel == DstColorKey )
								Pixel ^= 1;
							pDst[0] = (uint8)(Pixel>>16);
							pDst[1] = (uint8)((Pixel>>8)&0xFF);
							pDst[2] = (uint8)(Pixel&0xFF);
							pSrc += 3;
							pDst += 3;
						}
						pSrc += SrcXtraBytes;
						pDst += DstXtraBytes;
					}
					return GE_TRUE;
				}
				case 4:
				{
				uint32 *pSrc,*pDst;
					pSrc = (uint32 *)SrcPtr;
					pDst = (uint32 *)DstPtr;

					for(y=SizeY;y--;)
					{
						for(x=SizeX;x--;)
						{
							Pixel = *pSrc++;
							if ( Pixel == DstColorKey )
								Pixel ^= 1;
							*pDst++ = Pixel;
						}
						pSrc += SrcXtra;
						pDst += DstXtra;
					}
					return GE_TRUE;
				}
			}
			
			// end : formats same, dest had color key, source doesn't
		}

		return GE_TRUE;
	}
// must have returned by now
}
/*}{*********************************************************************/

geBoolean BlitData_DePalettize(void)
{
	// pal -> unpal : easy
	if ( SrcFormat == GE_PIXELFORMAT_8BIT )
	{
	uint8 * SrcPtr;
	geBitmap_Palette * DstPal;
	int x,y,pal;
	const gePixelFormat_Operations *SrcOps,*DstOps;

		x = y = pal = 0; //touch 'em

		SrcOps = gePixelFormat_GetOperations(SrcPal->Format);
		DstOps = gePixelFormat_GetOperations(DstFormat);
		if ( ! SrcOps || ! DstOps )
		{
			return GE_FALSE;
		}

		// NO special cases
		// just convert the Palette to the desired format, then do raw writes!

		DstPal = geBitmap_Palette_Create(DstFormat,256);
		if ( ! DstPal )
		{
			geErrorLog_AddString(-1,"Bitmap_BlitData : Palette_Create failed", NULL);	
			return GE_FALSE;
		}

		// we do all alpha & colorkey by manipulating the DstPal lookup table !

		//{} all these _geBitmap_Palette functions need failure checking

		if ( ! geBitmap_Palette_Copy(SrcPal,DstPal) )
		{
			geErrorLog_AddString(-1,"Bitmap_BlitData : Palette_Copy failed", NULL);
			geBitmap_Palette_Destroy(&DstPal);
			return GE_FALSE;
		}

		if ( SrcInfo->HasColorKey )
		{
			if ( ! geBitmap_Palette_SetEntryColor(DstPal,SrcInfo->ColorKey,0,0,0,0) )
			{
				geBitmap_Palette_Destroy(&DstPal);
				return GE_FALSE;
			}
		}

		if ( DstInfo->HasColorKey ) // everything in genesis has colorkey!
		{
		int pal;
		uint32 Pixel;
			for(pal=0;pal<DstPal->Size;pal++)
			{
				//{} all this GetEntry/SetEntry is awfully slow
				geBitmap_Palette_GetEntry(DstPal,pal,&Pixel);
				if ( Pixel == DstInfo->ColorKey )
				{
					geBitmap_Palette_SetEntry(DstPal,pal,Pixel^1);
				}
			}
			
		}

		if ( SrcInfo->HasColorKey && DstInfo->HasColorKey )
		{
			if ( ! geBitmap_Palette_SetEntry(DstPal,SrcInfo->ColorKey,DstInfo->ColorKey) )
			{
				geBitmap_Palette_Destroy(&DstPal);
				return GE_FALSE;
			}
		}

		if ( SrcOps->AMask && ! DstOps->AMask && DstInfo->HasColorKey )
		{
		int pal,R,G,B,A;
		uint32 Pixel;

			// if Src format has alpha & Dst format doesn't, turn it into color key

			for(pal=0;pal<DstPal->Size;pal++)
			{
				geBitmap_Palette_GetEntry(SrcPal,pal,&Pixel);
				if ( SrcInfo->HasColorKey && Pixel == SrcInfo->ColorKey )
				{
					A = 0;
				}
				else
				{
					gePixelFormat_DecomposePixel(SrcPal->Format,Pixel,&R,&G,&B,&A);
				}
				if ( A < ALPHA_TO_TRANSPARENCY_THRESHOLD )
					geBitmap_Palette_SetEntry(DstPal,pal,DstInfo->ColorKey);
			}
		}

		SrcPtr = (uint8 *)SrcData;

		// Pal -> UnPal loops : very common & very fast

		switch( gePixelFormat_BytesPerPel(DstFormat) )
		{
			default:
			{
				geBitmap_Palette_Destroy(&DstPal);
				return GE_FALSE;
			}
			case 1:
			{
			uint8 *DstPtr,*PalData;
				PalData = (uint8 *)DstPal->Data;
				DstPtr  = (uint8 *)DstData;
				for(y=SizeY;y--;)
				{

					#ifdef DONT_USE_ASM

					for(x=SizeX;x--;)
					{
						pal = *SrcPtr++;
						*DstPtr++ = PalData[pal];
					}

					#else

					#pragma message("Bitmap_Blitdata :using assembly DePalettize code")
					// {} is this minimal push safe in _fastcall ? aparently so!

					__asm
					{
						push ebp

						mov ecx,SizeX
						mov esi,SrcPtr
						mov edi,DstPtr
						mov ebp,PalData

						xor eax,eax
						xor edx,edx
						
					moredata1:

						mov al, BYTE PTR [esi]
						mov dl, BYTE PTR [ebp + eax]
						mov BYTE PTR [edi], dl

						inc esi
						inc edi
						dec ecx

						jnz moredata1

						pop ebp
					}

					SrcPtr += SizeX;
					DstPtr += SizeX;

					#endif

					SrcPtr += SrcXtra;
					DstPtr += DstXtra;
				}
				break;
			}
			case 2:
			{
			uint16 *DstPtr,*PalData;

				PalData = (uint16 *)DstPal->Data;
				DstPtr  = (uint16 *)DstData;

			#ifdef DO_TIMER
			{
			#pragma message("Blitdata : doing timer")
				TIMER_VARS(WordCopy);

				timerFP = fopen("q:\\timer.log","at+");
				Timer_Start();
				TIMER_P(WordCopy);
			#endif // DO_TIMER

				for(y=SizeY;y--;)
				{
					#ifdef DONT_USE_ASM

					for(x=SizeX;x--;)
					{
						pal = *SrcPtr++;
						*DstPtr++ = PalData[pal];
					}

					#else

					#if 1 // {
					if ( (SizeX&1) == 0 )
					{
						assert( (((uint32)PalData)&3) == 0 );
						assert( (((uint32)DstPtr )&3) == 0 );

						// pair two pixels so we can output in dwords
						
						__asm
						{
							//pusha
							push ebp

							mov ecx,SizeX
							mov esi,SrcPtr
							mov edi,DstPtr
							mov ebp,PalData

							xor eax,eax
							
						moredata2_z:

							//WordCopy : 0.000664 secs
							//	about 12 clocks per pixel (!?)

							// this is godly fast

							movzx eax, BYTE PTR [esi+0]
							movzx eax, WORD PTR [ebp + eax*2]

							movzx edx, BYTE PTR [esi+1]
							movzx edx, WORD PTR [ebp + edx*2]
							shl edx,16

							xor eax,edx

							mov DWORD PTR [edi], eax

							add esi,2
							add edi,4

							sub ecx,2
							jnz moredata2_z

#if 0 //{ 
						// the old bad way:
						// 0.000710 secs
						moredata2_z:

							//xor edx,edx	//xor edx,0 ; sneaky trick?

							//mov al, BYTE PTR [esi]
							movzx eax, BYTE PTR [esi]
							inc esi

							movzx edx, WORD PTR [ebp + eax*2]
							//mov dx, WORD PTR [ebp + eax*2]

							movzx eax, BYTE PTR [esi]
							inc esi

							// make room fo a new dx
							//shl edx,16
							//mov dx, WORD PTR [ebp + eax*2]	// !! STALL !! ; movzx eax, [] instead?
							// byte order is wrong; fix with rol; 1 clock
							//rol edx,16

							movzx eax, WORD PTR [ebp + eax*2]	// can I do this?
							shl eax,16
							xor edx,eax

							mov DWORD PTR [edi], edx
							add edi,4

							sub ecx,2
							jnz moredata2_z
#endif //}

							pop ebp
							//popa
						}

					}
					else
					#endif //}
					{

						__asm
						{
							//pusha
							push ebp

							mov ecx,SizeX
							mov esi,SrcPtr
							mov edi,DstPtr
							mov ebp,PalData

							xor eax,eax
							xor edx,edx
							
						moredata2:

							// about 14 clocks (!)

							//mov al, BYTE PTR [esi]
							movzx eax, BYTE PTR [esi]
							//movzx edx, WORD PTR [ebp + eax*2]
							mov dx, WORD PTR [ebp + eax*2]
							mov WORD PTR [edi], dx

							inc esi
							add edi,2

							dec ecx
							jnz moredata2

							pop ebp
							//popa
						}
					}

					SrcPtr += SizeX;
					DstPtr += SizeX;

					#endif

					SrcPtr += SrcXtra;
					DstPtr += DstXtra;
				}

				#ifdef DO_TIMER
				TIMER_Q(WordCopy);
				TIMER_COUNT();
				Timer_Stop();
				if ( timerFP )
				{
					TIMER_REPORT(WordCopy);
				}
			}
			#endif

				// C , Debug :
				//WordCopy             : 0.001243 : 99.4 %
				// asm paired : mov al,
				//WordCopy             : 0.000858 : 99.1 %
				// asm paired : movzx eax,
				//WordCopy             : 0.000798 : 98.9 %
				// asm paired : with xor edx,0 & movzx edx,
				//WordCopy             : 0.000903 : 99.2 %
				// asm paired : with xor edx,0 & mov dx,
				//WordCopy             : 0.000941 : 99.4 %
				// asm : not paired 
				//WordCopy             : 0.000765 : 98.8 %
				// asm : paired, using xor edx,eax !
				//WordCopy             : 0.000710 : 98.9 %

				break;
			}
			case 3:
			{
			uint8 *DstPtr,*PalData;
				PalData = (uint8 *)DstPal->Data;
				DstPtr  = (uint8 *)DstData;

//				pushTSC();

				for(y=SizeY;y--;)
				{

					#ifdef DONT_USE_ASM
					{
					uint8 *PalPtr;

						for(x=SizeX;x--;)
						{
							pal = *SrcPtr++;
							PalPtr = PalData + (3*pal);
							*DstPtr++ = *PalPtr++;
							*DstPtr++ = *PalPtr++;
							*DstPtr++ = *PalPtr;
						}

					}
					#else

					__asm
					{
						push ebp

						mov ecx,SizeX
						mov esi,SrcPtr
						mov edi,DstPtr
						mov ebp,PalData

						xor eax,eax
						xor edx,edx
						
					moredata3:

						movzx eax, BYTE PTR [esi]
						inc esi

						imul eax,3
						add eax,ebp
						mov dl, BYTE PTR [eax]
						mov BYTE PTR [edi], dl
						inc edi
						mov dl, BYTE PTR [eax + 1]
						mov BYTE PTR [edi], dl
						inc edi
						mov dl, BYTE PTR [eax + 2]
						mov BYTE PTR [edi], dl
						inc edi

						dec ecx
						jnz moredata3

						pop ebp
					}

					SrcPtr += SizeX;
					DstPtr += SizeX*3;

					#endif

					SrcPtr += SrcXtra;
					DstPtr += DstXtra;
				}
				
//				showPopTSCper("depal 24bit",SizeX*SizeY,"pixel");

				break;
			}
			case 4:
			{
			uint32 *DstPtr,*PalData;
				PalData = (uint32 *)DstPal->Data;
				DstPtr  = (uint32 *)DstData;
				for(y=SizeY;y--;)
				{
					#ifdef DONT_USE_ASM

					for(x=SizeX;x--;)
					{
						pal = *SrcPtr++;
						*DstPtr++ = PalData[pal];
					}

					#else

					assert( (((uint32)PalData)&3) == 0);
					assert( (((uint32)DstPtr)&3) == 0);

					__asm
					{
						push ebp

						mov ecx,SizeX
						mov esi,SrcPtr
						mov edi,DstPtr
						mov ebp,PalData

						xor eax,eax
						
					moredata4:

						mov al, BYTE PTR [esi]
						mov edx, DWORD PTR [ebp + eax*4]
						mov DWORD PTR [edi], edx

						inc esi
						add edi,4

						dec ecx
						jnz moredata4

						pop ebp
					}

					SrcPtr += SizeX;
					DstPtr += SizeX;

					#endif

					SrcPtr += SrcXtra;
					DstPtr += DstXtra;
				}
				break;
			}
		}

		geBitmap_Palette_Destroy(&DstPal);

		return GE_TRUE;
	}
return GE_FALSE;
}

/*}{*********************************************************************/

geBoolean BlitData_Palettize(void)
{
	// unpal -> pal : hard
return palettizePlane(	SrcInfo,SrcData,
						DstInfo,DstData,
						SizeX,SizeY);
}

/*}{*********************************************************************/

