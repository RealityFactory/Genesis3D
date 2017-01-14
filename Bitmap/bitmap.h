#ifndef BITMAP_H
#define BITMAP_H

/****************************************************************************************/
/*  Bitmap.h                                                                            */
/*                                                                                      */
/*  Author: Charles Bloom                                                               */
/*  Description:  Abstract Bitmap system                                                */
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
#include "pixelformat.h"
#include "vfile.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************/

typedef struct geBitmap			geBitmap;
typedef struct geBitmap_Palette	geBitmap_Palette;

typedef struct geBitmap_Info
{
	int						Width;
	int						Height;
	int						Stride;		// stride is in *pixels* ; it is the step to the next line : Stride >= Width
	gePixelFormat			Format;
	int						MinimumMip;	//*including* minimumMip == 0 often
	int						MaximumMip;	//*including* maximumMip == nummips-1
	geBoolean				HasColorKey;
	uint32					ColorKey;	// meaningless unless HasColorKey ; the ColorKey is a Pixel in Format
	geBitmap_Palette *		Palette;
} geBitmap_Info;

/***********************************************************************************/		
// Bitmap methods

// see a big comment at the end of this file

/************************************************************************/

GENESISAPI geBitmap *	GENESISCC	geBitmap_Create(int Width, int Height, int MipCount, gePixelFormat Format ); 
GENESISAPI void			GENESISCC	geBitmap_CreateRef(geBitmap *Bmp);

GENESISAPI geBitmap *	GENESISCC	geBitmap_CreateFromInfo(const geBitmap_Info * pInfo);

GENESISAPI geBitmap *	GENESISCC	geBitmap_CreateFromFile( geVFile *F );
GENESISAPI geBitmap *	GENESISCC	geBitmap_CreateFromFileName(const geVFile *BaseFS,const char *Name);
GENESISAPI geBoolean 	GENESISCC	geBitmap_WriteToFile( const geBitmap *Bmp, geVFile *F );
GENESISAPI geBoolean	GENESISCC	geBitmap_WriteToFileName(const geBitmap * Bmp,const geVFile *BaseFS,const char *Name);
										// BaseFS is not really const if it is a virtual file;
										//  it *is* const if it is a dos directory

GENESISAPI geBoolean 	GENESISCC	geBitmap_Destroy(geBitmap **Bmp);
	// returns whether Bmp was actually destroyed : not success/failure

GENESISAPI geBoolean 	GENESISCC	geBitmap_GetInfo(const geBitmap *Bmp, geBitmap_Info *Info, geBitmap_Info *SecondaryInfo);
	//LockForWrite returns data in Info's format

GENESISAPI geBoolean 	GENESISCC	geBitmap_Blit(const	geBitmap *Src, int SrcPositionX, int SrcPositionY,
										geBitmap *Dst, int DstPositionX, int DstPositionY,
										int SizeX, int SizeY );

GENESISAPI geBoolean 	GENESISCC	geBitmap_BlitMip(const geBitmap * Src, int SrcMip, geBitmap * Dst, int DstMip );
										// don't use this with Src == Dst, use UpdateMips instead !

GENESISAPI geBoolean 	GENESISCC	geBitmap_BlitBitmap(const geBitmap * Src, geBitmap * Dst);

GENESISAPI geBoolean 	GENESISCC	geBitmap_BlitBestMip(const geBitmap * Src, geBitmap * Dst);
										// blits the largest mip from Src that fits in Dst

GENESISAPI geBoolean 	GENESISCC	geBitmap_LockForRead(		// a non-exclusive lock
	const geBitmap *	Bmp,
	geBitmap **			Target,
	int					MinimumMip,
	int					MaximumMip,
	gePixelFormat 		Format,
	geBoolean			RespectColorKey,
	uint32				ColorKey);
									// not really const, stores lock-count, but *data* is const
									// will do a format conversion!

GENESISAPI geBoolean	GENESISCC	geBitmap_LockForReadNative(
	const geBitmap *	Bmp,
	geBitmap **			Target,
	int					MinimumMip,
	int					MaximumMip);
									// lock for read in a format that gaurantee no conversions
									// then do GetInfo on the locks to see what you have!

GENESISAPI geBoolean 	GENESISCC	geBitmap_LockForWrite(	// an exclusive lock
	geBitmap *			Bmp,
	geBitmap **			Target,
	int					MinimumMip,
	int					MaximumMip);

GENESISAPI geBoolean 	GENESISCC	geBitmap_LockForWriteFormat(
	geBitmap *			Bmp,
	geBitmap **			Target,
	int					MinimumMip,
	int					MaximumMip,
	gePixelFormat 		Format);
									// Format must be one of the two returned in GetInfo !!

GENESISAPI geBoolean 	GENESISCC	geBitmap_UnLock(geBitmap *Bmp);	// must be done on All locked mips
GENESISAPI geBoolean 	GENESISCC	geBitmap_UnLockArray(geBitmap **Locks,int Size);

GENESISAPI geBoolean 	GENESISCC	geBitmap_SetFormat(geBitmap *Bmp, 
							gePixelFormat NewFormat, 
							geBoolean RespectColorKey, uint32 ColorKey,
							const geBitmap_Palette * Palette);
	// _SetFormat may cause you to lose color information!
	// SetFormat does a conversion!
	// if NewFormat is palettized and Palette is NULL, we create a palette for the bitmap!

GENESISAPI geBoolean 	GENESISCC	geBitmap_SetFormatMin(geBitmap *Bmp,gePixelFormat NewFormat);
								// the Min version keeps colorkey & palette from the old format

GENESISAPI geBoolean	GENESISCC	geBitmap_SetColorKey(geBitmap *Bmp, geBoolean HasColorKey, uint32 ColorKey, geBoolean Smart);
	// SetColorKey discards old colorkey information!
	//	does not do a conversion (changes the colorkey in the current data
	// if 'Smart' is on, we don't set HasColorKey to true unless it is actually used!

GENESISAPI geBoolean	GENESISCC	geBitmap_GetAverageColor(const geBitmap *Bmp,int *pR,int *pG,int *pB);
	// tells you the average color; computes it and caches it out

GENESISAPI geBitmap_Palette * 	GENESISCC	geBitmap_GetPalette(const geBitmap *Bmp);
GENESISAPI geBoolean			GENESISCC	geBitmap_SetPalette(geBitmap *Bmp, const geBitmap_Palette *Palette);
	// _SetPal tries to _CreateRef your Palette, so no copy occurs & palettes may be shared
	// you may _Destroy() palette after using it to set (though its bits may not be freed)
	//	(hence Palette is *not* const)
	// Warning : SetPalette on any mip changes the palette of ALL mips !
	// see Palette note at _UnLock
	// _SetPal destroys the bitmap's original palette and refs the new one, 
	//		so if you setpal with the bitmap's palette, there is no net change in ref counts (good!)

GENESISAPI geBoolean	GENESISCC	geBitmap_HasAlpha(const geBitmap * Bmp);
	// returns true if bitmap has *any* type of alpha

GENESISAPI geBitmap *	GENESISCC	geBitmap_GetAlpha(const geBitmap *Bmp);
GENESISAPI geBoolean 	GENESISCC	geBitmap_SetAlpha(geBitmap *Bmp, const geBitmap *AlphaBmp);
	// we Ref the AlphaBmp, so you may destroy it after calling Set()
	// it may be NULL
	// there's only one Alpha per bitmap (for the top Mip) right now

GENESISAPI geBoolean	GENESISCC	geBitmap_SetGammaCorrection(geBitmap *Bmp,geFloat Gamma,geBoolean Apply);
	// this Gamma does not change the *original* (system/secondary) bits
	//	it only affects the appearance when drawn
	// note : if you write to the gamma corrected bits, you must gamma correct manually if you
	//	wish to fit in smoothly with the previous data
	// warning : if you use this function with many different gammas, performance will suffer!
	//	use one global gamma for all bitmaps!  try to let the engine manage gamma for you,
	//	via geEngine_SetGamma !

GENESISAPI geBoolean		GENESISCC	geBitmap_SetPreferredFormat(geBitmap *Bmp,gePixelFormat Format);
GENESISAPI gePixelFormat	GENESISCC	geBitmap_GetPreferredFormat(const geBitmap *Bmp);

GENESISAPI void *		GENESISCC	geBitmap_GetBits(geBitmap *Bmp);	// works only on a Lock()

GENESISAPI geBoolean 	GENESISCC	geBitmap_RefreshMips(geBitmap *Bmp);	// rebuilds mips; *tries* to be smart & not overwrite manually-fixed mips
												// RefreshMips does *not* build mips that don't exist
GENESISAPI geBoolean 	GENESISCC	geBitmap_UpdateMips(geBitmap *Bmp,int SourceMip,int TargetMip);	
												// will create the target if it doesn't exist;
												// will overwrite manually-fixed mips!
GENESISAPI geBoolean 	GENESISCC	geBitmap_SetMipCount(geBitmap *Bmp,int Count);
												// creates or destroys to match the new count

GENESISAPI geBoolean 	GENESISCC	geBitmap_ClearMips(geBitmap *Bmp);	// Destroy all mips (except the first) !
												// use with care! this is not polite!

// Shortcuts
GENESISAPI int			GENESISCC	geBitmap_Width(const geBitmap *Bitmap);
GENESISAPI int			GENESISCC	geBitmap_Height(const geBitmap *Bitmap);
GENESISAPI uint32		GENESISCC	geBitmap_MipBytes(const geBitmap * Bitmap,int mip);

/**
*
* if Bitmap is a lock for read, functions that modify it return failure
* if Bitmap is a lock for write, functions that modify it attempt to
*	modify the owner of the lock
*
* warning : if you lock multiple mips for write, and then modify one of the mips
*		(such as via SetPalette) it may affect the owner and all sibling mips!
*		doing different SetPalettes with different palettes on different locked mips 
*		has undefined behavior!
*
**/

#ifdef _DEBUG
GENESISAPI uint32		GENESISCC	geBitmap_Debug_GetCount(void);

	// assert this is zero before you shutdown !

#endif

/***********************************************************************************/

typedef enum
{
	GE_BITMAP_STREAMING_ERROR=0,
	GE_BITMAP_STREAMING_NOT,
	GE_BITMAP_STREAMING_STARTED,
	GE_BITMAP_STREAMING_IDLE,
	GE_BITMAP_STREAMING_CHANGED,
	GE_BITMAP_STREAMING_DATADONE,
	GE_BITMAP_STREAMING_DONE,
} geBitmap_StreamingStatus;

GENESISAPI geBitmap_StreamingStatus GENESISCC geBitmap_GetStreamingStatus(const geBitmap *Bmp);

		/** on a file which is streaming, the sequence of returns looks like :

			GE_BITMAP_STREAMING_IDLE
			GE_BITMAP_STREAMING_CHANGED
			GE_BITMAP_STREAMING_IDLE
			GE_BITMAP_STREAMING_IDLE
			GE_BITMAP_STREAMING_CHANGED
			...
			GE_BITMAP_STREAMING_DONE
			GE_BITMAP_STREAMING_NOT
			GE_BITMAP_STREAMING_NOT
			GE_BITMAP_STREAMING_NOT
			...

		Status >= GE_BITMAP_STREAMING_STARTED means streaming has started & is in progress

		the user should never see _STARTED or _DATADONE

		***/

/***********************************************************************************/

// palette methods :

GENESISAPI geBitmap_Palette * 	GENESISCC	geBitmap_Palette_Create(gePixelFormat Format,int Size);

GENESISAPI geBitmap_Palette * 	GENESISCC	geBitmap_Palette_CreateCopy(const geBitmap_Palette *Palette);

GENESISAPI geBitmap_Palette *	GENESISCC	geBitmap_Palette_CreateFromFile(geVFile *F);

GENESISAPI geBitmap_Palette *	GENESISCC	geBitmap_Palette_CreateFromBitmap(geBitmap * Bmp,geBoolean Slow);
												// does GetPalette, and if NULL, then
												// it create an optimal palette for a
												//	non-palettized bitmap
												//	(this is a create, you must destroy later!)
												// put Slow == TRUE for higher quality & slower

GENESISAPI geBoolean		GENESISCC	geBitmap_Palette_SortColors(geBitmap_Palette * P,geBoolean Slower);

GENESISAPI geBoolean		GENESISCC	geBitmap_Palette_CreateRef(geBitmap_Palette *Palette);

GENESISAPI geBoolean		GENESISCC	geBitmap_Palette_Destroy(geBitmap_Palette ** ppPalette);

GENESISAPI geBoolean		GENESISCC	geBitmap_Palette_WriteToFile(const geBitmap_Palette *Palette,geVFile *F);

GENESISAPI geBoolean		GENESISCC	geBitmap_Palette_SetFormat(geBitmap_Palette * Palette,gePixelFormat Format);

GENESISAPI geBoolean		GENESISCC	geBitmap_Palette_Copy(const geBitmap_Palette * Src,geBitmap_Palette * Target);

GENESISAPI geBoolean		GENESISCC	geBitmap_Palette_GetInfo(const	geBitmap_Palette *P,geBitmap_Info *Into);
												// get the info as if it were a bitmap; Into->Height == 1

GENESISAPI geBoolean		GENESISCC	geBitmap_Palette_Lock(geBitmap_Palette *Palette, void **pBits, gePixelFormat *pFormat,int *pSize);
												// pFormat & pSize are optional

GENESISAPI geBoolean		GENESISCC	geBitmap_Palette_UnLock(geBitmap_Palette *Palette);
											// palette unlock does NOT notify the bitmap that the palette has changed.
											// call Bitmap_SetPalette() with the same palette pointer 
											// 	to tell the bitmap that it must to some processing
											// (don't worry, it won't duplicate it or copy it onto itself)

GENESISAPI geBoolean		GENESISCC	geBitmap_Palette_GetData(const geBitmap_Palette *P,      void *Into,gePixelFormat Format,int Colors);
GENESISAPI geBoolean		GENESISCC	geBitmap_Palette_SetData(      geBitmap_Palette *P,const void *From,gePixelFormat Format,int Colors);
											// does Lock/UnLock for you
											// From and Into are arrays of Colors*gePixelFormat_BytesPerPel bytes

GENESISAPI geBoolean		GENESISCC	geBitmap_Palette_SetEntryColor(      geBitmap_Palette *P,int Color,int R,int G,int B,int A);
GENESISAPI geBoolean		GENESISCC	geBitmap_Palette_GetEntryColor(const geBitmap_Palette *P,int Color,int *R,int *G,int *B,int *A);
											// Set/Get does Lock/Unlock for you ; these are slow! do not use these to work on all the colors!

GENESISAPI geBoolean		GENESISCC	geBitmap_Palette_SetEntry(      geBitmap_Palette *P,int Color,uint32 Pixel);
GENESISAPI geBoolean		GENESISCC	geBitmap_Palette_GetEntry(const geBitmap_Palette *P,int Color,uint32 *Pixel);

/***********************************************************************************/

/************************************************************************

A brief tutorial on the Bitmap system, by Charles Bloom, cbloom@wildtangent.com

The Bitmap is a smart wrapper for complex functionality.  You give it hints to
the opaque Bitmap object, and it tries its best to follow those hints, but it
may not always do so.  The Bitmap is the owner of its bits; you must Lock the
bitmap to get permission to touch those bits, and UnLock to tell the bitmap
you are done.  The format may change between two Locks.  Bitmaps can also be
multiply owned, so you should account for the fact that others may touch your
bitmap between your uses.

The Bitmap contains one or two pixel-sets representing an image.  The "primary" is
a fast-blitting version of the image, and the "secondary" is a storage version
(eventually wavelet compressed) which can be used to rebuild the primary if it is
freed or damaged.  Both cary a generalized format.

Let's do an example.  I want to load a bitmap, set it up for drawing with the
genesis Engine, and then blit some interactive stuff into it.

************************************************************************/

#if 0
// {
//-----------------------------------------------------------------------------

void Init(geEngine * Engine);
void Shutdown(void);
void Draw(void);
void DrawPolite(void);

static geBitmap * myBM = NULL;
static geEngine * myEngine = NULL;

void Init(geEngine * Engine)
{
geBoolean success;
geBitmap_Info Info;

	myEngine = Engine;	// this is not looked well upon; for ease of demonstration only!
	assert(Engine);

	myBM = geBitmap_CreateFromFileName(NULL,"mybitmap.bmp");

	// CreateFromFile can load windows BMP files, or custom GeBm files.

	assert(myBM);

	// get the main info; I don't care about the secondary, so leave it NULL

	success = geBitmap_GetInfo(myBM,&Info,NULL);
	assert(success);

	// make sure I loaded a bitmap in the format I understand !

	if ( Info.Format == GE_PIXELFORMAT_8BIT_PAL )
	{
		// I want palette index 255 to act as transparency, so I must use SetColorKey

		success = geBitmap_SetColorKey(myBM,GE_TRUE,255);
		assert(success);

		// just for fun, let's modify the palette:
		if (1)
		{
		geBitmap_Palette * Pal;

			// get the palette ; I don't care if its primary or secondary, so
			/// I don't use the Info.Palette field

			Pal = geBitmap_GetPalette(myBM);
			assert(Pal);

			// I'm only fiddling one entry, so don't bother with a full Lock() UnLock()
			//  sequence on the palette

			// make palette index zero bright red; we use alpha = 255 for opaque

			success = geBitmap_Palette_SetEntryColor(Pal,0,255,0,0,255);
			assert(success);

			// tell the bitmap system you've changed the palette; this function
			//  is smart enough to not do unecessary copies or whatever.

			success = geBitmap_SetPalette(myBM,Pal);
			assert(success);
		}

	}
	else
	{
		// otherwise, treat black as transparent, in whatever format I have

		success = geBitmap_SetColorKey(myBM,GE_TRUE,gePixelFormat_ComposePixel(Info.Format,0,0,0,0));
		assert(success);
	}	

	// note that I did NOT use SetFormat.  SetFormat may do a conversion, and since the original
	//	bitmap was created without colorkey, it would have been converted to a new format but
	//	kept its property of having no colorkey!
	// (SetFormat will fiddle the bits and whatever way necessary to keep bitmaps as visually similar
	//		as possible)

	// I want to fiddle the fast format in 565 later, so cue the bitmap to try to give me that format.

	success = geBitmap_SetPreferredFormat(myBM,GE_PIXELFORMAT_16BIT_565_RGB);
	assert(success);

	// Add it to the engine so it can be used for drawing.

	success = geEngine_AddBitmap(myEngine,myBM);
	assert(success);
}

void Shutdown(void)
{
geBoolean WasDestroyed;

	assert(myBM);
	
	// clean up

	geEngine_RemoveBitmap(myEngine,myBM);

	WasDestroyed = geBitmap_Destroy(&myBM);

	// someone else might have done _CreateRef on our bitmap,
	//  so we can't be sure it's actually destroyed.
	// this code is still ready to be run again with a new call to Init()

	//assert(WasDestroyed);

	myBM = NULL;
	myEngine = NULL;
}

void Draw(void)
{
geBitmap * Lock;
geBoolean success;
geBitmap_Info Info;
uint16 *bits,*bptr;
int x,y;

	// lets fiddle the bits.
	// we need to lock the bitmap for write.
	//	LockForWrite is an exclusive lock, unlike LockForRead which is non-blocking
	// request our favorite format, and only lock Mip 0 (the full size bitmap)

	success = geBitmap_LockForWriteFormat(myBM,&Lock,0,0,GE_PIXELFORMAT_16BIT_565_RGB);
	if ( ! success )
	{
		// well, we tried to be nice; if we were very polite, we would do a LockForWrite
		// here, and try to fiddle the bits in whatever format we got; However, we aren't
		// that polite, so we just do a _SetFormat
		//
		// note that we are destroying the original bitmap by changing its format
		// we should only do this if we are going to draw into the bitmap

		success = geBitmap_SetFormat(myBM,GE_PIXELFORMAT_16BIT_565_RGB,GE_TRUE,0,NULL);
		assert(success);

		// now we should be able to get the bits we want, *but* they may not be the
		// primary (fast) format; oh well, it's the best we can do...
		// (if you must have the fastest bits, then use only _LockForWrite, never LockForWriteFormat,
		// which might have to do a conversion)

		success = geBitmap_LockForWriteFormat(myBM,&Lock,0,0,GE_PIXELFORMAT_16BIT_565_RGB);
		assert(success);
	}

	// now Lock is our bitmap in 565
	// we do a GetInfo because the Lock's info could be different than
	//	the original bitmap's (particularly the Palette & the Stride)

	success = geBitmap_GetInfo(Lock,&Info,NULL);
	assert(success);

	// you can only call _GetBits on a locked bitmap

	bits = geBitmap_GetBits(Lock);
	assert( bits );

	bptr = bits;
	for(y=0; y < Info.Height; y++)
	{
		for(x=0; x < Info.Width; x++)
		{
		uint16 R,G,B;
			// make a silly 565 gradient
			R = x & 0x1F;
			G = x & 0x3F;
			B = y & 0x1F;

			*bptr++ = (R<<11) + (G<<5) + B;
		}

		// note that bptr is a word pointer, and Stride is in pixels :

		bptr += Info.Stride -  Info.Width;
	}
	bits = bptr = NULL;

	// you call Unlock on all the mips you locked - not on the original bitmap!

	success = geBitmap_UnLock(Lock);
	assert(success);

	// now, we only fiddled the full-size Mip, and there might be more,
	//  so lets percolate the changes into the smaller mips:

	success = geBitmap_RefreshMips(myBM);
	assert(success);

	// a null rect means use the whole bitmap;
	// Engine_DrawBitmap blits a 2d decal to the framebuffer (fast)

	success = geEngine_DrawBitmap(myEngine,myBM,NULL,0,0);
	assert(success);

}

void DrawPolite(void)
{
geBitmap * Lock;
geBoolean success;
geBitmap_Info Info;
void *bits;
int x,y;

	// this function does the same thing as Draw() , but is more polite
	// lock in the fastest format (whatever it is)
	// because we did SetPreferred, this should be 565_RGB, but might not be

	success = geBitmap_LockForWrite(myBM,&Lock,0,0);
	assert(success);

	success = geBitmap_GetInfo(Lock,&Info,NULL);
	assert(success);

	bits = geBitmap_GetBits(Lock);
	assert( bits );

	if ( Info.Format == GE_PIXELFORMAT_16BIT_565_RGB )
	{
	uint16 *wptr;

		// our favorite format

		wptr = bits;
		for(y=0; y < Info.Height; y++)
		{
			for(x=0; x < Info.Width; x++)
			{
			uint16 R,G,B;
				// make a silly 565 gradient
				R = x & 0x1F;
				G = x & 0x3F;
				B = y & 0x1F;

				*wptr++ = (R<<11) + (G<<5) + B;
			}
			wptr += Info.Stride -  Info.Width;
		}
	}
	else
	{
	uint8 * bptr;

		// oh well, do our best
		// bitmaps must have had a good reason to not give us the format we preferred,

		bptr = bits;
		for(y=0; y < Info.Height; y++)
		{
			for(x=0; x < Info.Width; x++)
			{
			uint32 R,G,B;

				// put a color in any format

				R = (x & 0x1F)<<3;
				G = (x & 0x3F)<<2;
				B = (y & 0x1F)<<3;

				// we use alpha of 255 for opaque

				gePixelFormat_PutColor(Info.Format,&bptr,R,G,B,255);
			}

			bptr += (Info.Stride -  Info.Width) * gePixelFormat_BytesPerPel(Info.Format);
		}
	}
	bits = NULL;

	// same as before:

	success = geBitmap_UnLock(Lock);
	assert(success);

	success = geBitmap_RefreshMips(myBM);
	assert(success);

	success = geEngine_DrawBitmap(myEngine,myBM,NULL,0,0);
	assert(success);

}

// end tutorial on the Bitmap system
//-----------------------------------------------------------------------------
// }

/***********************************************************************************/

#endif
#ifdef __cplusplus
}
#endif


#endif


