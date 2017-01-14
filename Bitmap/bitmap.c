//bitmap.z
/****************************************************************************************/
/*  Bitmap.c                                                                            */
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
/*Genesis3D Version 1.1 released November 15, 1999                            */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/

/*}{ *************** head *******************/

/**********
***
*

see @@ for urgent todos !
see <> for todos
see {} for notes/long-term-todos

-------

{} when palettizing lots of mips, blit them together, so we only make
	one closestPal palInfo for all the blits (big speedup).
	(for UpdateMips too)
	perhaps the only way is to keep a cache of the last palInfo made and
	check the palette to see if its reusable.  Check on memory cost of the palInfo.

{} when we blit from one set of mips to another, we could copy the palette (or build the palette!)
	many times.  We must do this in general, because the mips could all have different palettes.
	The answer is to keep track of "palette was just copied from X"
	(actually, setting different palettes on two locked mips will cause bad data!!)

{} make a _BlitOnto which merges with alpha?

*
***
 ********/

#include	<math.h>
#include	<time.h>
#include	<stdio.h>
#include	<assert.h>
#include	<stdlib.h>
#include	<string.h>

#include	"basetype.h"
#include	"getypes.h"
#include	"ram.h"

#include	"vfile.h"
#include	"ErrorLog.h"
#include	"Log.h"
#include	"mempool.h"

#include	"bitmap.h"
#include	"bitmap._h"
#include	"bitmap.__h"
#include	"bitmap_blitdata.h"
#include	"bitmap_gamma.h"

#include	"palcreate.h"
#include	"palettize.h"

#ifdef DO_TIMER
#include	"timer.h"
#endif

#define allocate(ptr)	ptr = geRam_Allocate(sizeof(*ptr))
#define clear(ptr)		memset(ptr,0,sizeof(*ptr))

#define SHIFT_R_ROUNDUP(val,shift)	(((val)+(1<<(shift)) - 1)>>(shift))

/*}{ ************* statics *****************/

//#define DO_TIMER

//#define DONT_USE_ASM

#ifdef _DEBUG
#define Debug(x)	x
static int _Bitmap_Debug_ActiveCount = 0;
static int _Bitmap_Debug_ActiveRefs = 0;
#else
#define Debug(x)
#endif

static int BitmapInit_RefCount = 0;
static MemPool * BitmapPool = NULL;

void geBitmap_Start(void)
{
	if ( BitmapInit_RefCount == 0 )
	{
		BitmapPool = MemPool_Create(sizeof(geBitmap),100,100);
		assert(BitmapPool);
		Palettize_Start();
		PalCreate_Start();
	}
	BitmapInit_RefCount ++;
}

void geBitmap_Stop(void)
{
	assert(BitmapInit_RefCount > 0 );
	BitmapInit_RefCount --;
	if ( BitmapInit_RefCount == 0 )
	{
		assert(BitmapPool);
		MemPool_Destroy(&BitmapPool);
		Palettize_Stop();
		PalCreate_Stop();
	}
}

/*}{ ******** Creator Functions **********************/

geBitmap * geBitmap_Create_Base(void)
{
geBitmap * Bmp;

	geBitmap_Start();

	Bmp = MemPool_GetHunk(BitmapPool);

	Bmp->RefCount = 1;

	Bmp->DriverGamma = Bmp->DriverGammaLast = 1.0f;

	Debug(_Bitmap_Debug_ActiveRefs ++);
	Debug(_Bitmap_Debug_ActiveCount ++);

return Bmp;
}

void geBitmap_Destroy_Base(geBitmap *Bmp)
{
	assert(Bmp);
	assert(Bmp->RefCount == 0);
	Debug(_Bitmap_Debug_ActiveCount --);

	MemPool_FreeHunk(BitmapPool,Bmp);

	geBitmap_Stop();
}

GENESISAPI void GENESISCC	geBitmap_CreateRef(geBitmap *Bmp)
{
	assert(Bmp);
	Bmp->RefCount ++;
	Debug(_Bitmap_Debug_ActiveRefs ++);
}

GENESISAPI geBitmap *	GENESISCC	geBitmap_Create(
	int					 Width,
	int					 Height,
	int					 MipCount,
	gePixelFormat Format)
{
geBitmap * Bmp;

	Bmp = geBitmap_Create_Base();
	if ( ! Bmp )
		return NULL;

	assert( Width > 0 );
	assert( Height > 0 );
	if ( MipCount == 0 )
		MipCount = 1;
	assert( MipCount > 0 );

	Bmp->Info.Width = Width;
	Bmp->Info.Stride = Width;
	Bmp->Info.Height = Height;
	Bmp->Info.Format = Format;

	Bmp->Info.MinimumMip = 0;
	Bmp->Info.MaximumMip = 0;
	Bmp->Info.HasColorKey = GE_FALSE;

	Bmp->SeekMipCount = MipCount;

	if ( Format == GE_PIXELFORMAT_WAVELET )
	{
		geErrorLog_AddString(-1,"Genesis3D 1.0 does not support Wavelet Images",NULL);
		return NULL;
	}

return Bmp;
}

GENESISAPI geBitmap *	GENESISCC	geBitmap_CreateFromInfo(const geBitmap_Info * pInfo)
{
geBitmap * Bmp;

	assert(pInfo);
	assert(geBitmap_Info_IsValid(pInfo));

	Bmp = geBitmap_Create_Base();
	if ( ! Bmp )
		return NULL;

	Bmp->Info = *pInfo;

	if ( Bmp->Info.Stride < Bmp->Info.Width )
		Bmp->Info.Stride = Bmp->Info.Width;

	if ( Bmp->Info.Palette )
		geBitmap_Palette_CreateRef(Bmp->Info.Palette);

	if ( Bmp->Info.Format == GE_PIXELFORMAT_WAVELET )
	{
		geErrorLog_AddString(-1,"Genesis3D 1.0 does not support Wavelet Images",NULL);
		return NULL;
	}

return Bmp;
}

GENESISAPI geBoolean	GENESISCC	 geBitmap_Destroy(geBitmap **Bmp)
{
int			i;
geBitmap *	Bitmap;

	assert(Bmp);

	Bitmap = *Bmp;

	if ( Bitmap )
	{
		if ( Bitmap->LockOwner )
		{
			return geBitmap_UnLock(Bitmap);
		}

		if ( Bitmap->RefCount <= 1 )
		{
			if ( Bitmap->DataOwner )
			{
				geBitmap_Destroy(&(Bitmap->DataOwner));
				Bitmap->DataOwner = NULL;
			}
			else
			{
				if ( Bitmap->Driver )
				{
					geBitmap_DetachDriver(Bitmap,GE_FALSE);
				}

				for	(i = Bitmap->Info.MinimumMip; i <= Bitmap->Info.MaximumMip; i++)
				{
					if	(Bitmap->Data[i])
						geRam_Free(Bitmap->Data[i]);
				}
			}
		}

		Debug(assert(_Bitmap_Debug_ActiveRefs > 0));
		Debug(_Bitmap_Debug_ActiveRefs --);

		Bitmap->RefCount --;

		if ( Bitmap->RefCount <= 0 )
		{
			if	(Bitmap->Alpha)
			{
				geBitmap_Destroy(&Bitmap->Alpha);
			}

			if	(Bitmap->Info.Palette)
			{
				geBitmap_Palette_Destroy(&(Bitmap->Info.Palette));
			}

			if	(Bitmap->DriverInfo.Palette)
			{
				geBitmap_Palette_Destroy(&(Bitmap->DriverInfo.Palette));
			}

			geBitmap_Destroy_Base(Bitmap);

			*Bmp = NULL;

			return GE_TRUE;
		}
	}

return GE_FALSE;
}

geBoolean geBitmap_AllocSystemMip(geBitmap *Bmp,int mip)
{
	if ( ! Bmp )
	{
		return GE_FALSE;
	}

	if ( Bmp->LockOwner && mip != 0 ) return GE_FALSE;

	if ( ! Bmp->Data[mip] )
	{
	int bytes;
		bytes = geBitmap_MipBytes(Bmp,mip);
		if ( bytes == 0 )
		{
			Bmp->Data[mip] = NULL;
			return GE_TRUE;
		}
		Bmp->Data[mip] = geRam_Allocate( bytes );
	}

return (Bmp->Data[mip]) ? GE_TRUE : GE_FALSE;
}

geBoolean geBitmap_AllocPalette(geBitmap *Bmp,gePixelFormat Format,DRV_Driver * Driver)
{
geBitmap_Info * BmpInfo;
	assert(Bmp);

	if ( Driver )
		BmpInfo = &(Bmp->DriverInfo);
	else
		BmpInfo = &(Bmp->Info);

	if ( ! gePixelFormat_IsRaw(Format) )
		Format = GE_PIXELFORMAT_32BIT_XRGB;

	if ( ! BmpInfo->Palette )
	{
		assert( BmpInfo->Format == GE_PIXELFORMAT_8BIT_PAL );

		if ( Driver )
		{
		geBoolean BmpHasAlpha;
			
			BmpHasAlpha = GE_FALSE;
			if ( gePixelFormat_HasGoodAlpha(Bmp->Info.Format) )
				BmpHasAlpha = GE_TRUE;
			else if ( Bmp->Info.Palette && gePixelFormat_HasGoodAlpha(Bmp->Info.Palette->Format) )
				BmpHasAlpha = GE_TRUE;

			if ( BmpHasAlpha || (Bmp->Info.HasColorKey && ! Bmp->DriverInfo.HasColorKey ) )
				Format = GE_PIXELFORMAT_32BIT_ARGB;

			BmpInfo->Palette = geBitmap_Palette_CreateFromDriver(Driver,Format,256);
		}
		else
		{			
			BmpInfo->Palette = geBitmap_Palette_Create(Format,256);
		}
	}

	if ( ! BmpInfo->Palette )
		return GE_FALSE;

	if ( BmpInfo->HasColorKey )
	{
		if ( ! BmpInfo->Palette->HasColorKey )
		{
			BmpInfo->Palette->HasColorKey = GE_TRUE;
			BmpInfo->Palette->ColorKey = 1; // <>
		}
		BmpInfo->Palette->ColorKeyIndex = BmpInfo->ColorKey;
	}

	if ( Driver )
	{
		assert( Bmp->DriverHandle );
		assert( BmpInfo->Palette->DriverHandle );
		if ( ! Driver->THandle_SetPalette(Bmp->DriverHandle,BmpInfo->Palette->DriverHandle) )
		{
			geErrorLog_AddString(-1,"AllocPal : THandle_SetPalette", NULL);
			return GE_FALSE;
		}
	}

	if ( ! Bmp->Info.Palette )
	{
		Bmp->Info.Palette = geBitmap_Palette_CreateCopy(BmpInfo->Palette);
	}

return GE_TRUE;
}

/*}{ *************** Locks *******************/

GENESISAPI geBoolean	GENESISCC	 geBitmap_LockForWrite(
	geBitmap *			Bmp,
	geBitmap **			Target,
	int					MinimumMip,
	int					MaximumMip)
{
int mip;

	assert( geBitmap_IsValid(Bmp) );
	assert( Target);
	assert(MaximumMip >= MinimumMip);
	assert( &Bmp != Target );

	if ( Bmp->LockCount || Bmp->LockOwner )
	{
		geErrorLog_AddString(-1,"LockForWrite : already locked", NULL);
		return GE_FALSE;
	}

	if ( Bmp->DriverHandle )
	{
		if ( (MinimumMip < Bmp->DriverInfo.MinimumMip) ||
			 (MaximumMip > Bmp->DriverInfo.MaximumMip) )
		{
			geErrorLog_AddString(-1,"LockForWrite : Driver : invalid mip", NULL);
			return GE_FALSE;
		}
	}
	else
	{
		if ( (MinimumMip < Bmp->Info.MinimumMip) ||
			 (MaximumMip >= MAXMIPLEVELS) )
		{
			geErrorLog_AddString(-1,"LockForWrite : System : invalid mip", NULL);
			return GE_FALSE;
		}

		if ( MaximumMip > Bmp->Info.MaximumMip )
		{
			if ( ! geBitmap_MakeSystemMips(Bmp,Bmp->Info.MaximumMip,MaximumMip) )
				return GE_FALSE;
			Bmp->Info.MaximumMip = MaximumMip;
		}
	}
	
	for(mip=MinimumMip;mip <= MaximumMip;mip ++)
	{
		if ( Bmp->DriverHandle )
		{
			Target[ mip - MinimumMip ] = geBitmap_CreateLockFromMipOnDriver(Bmp,mip,-1);
		}
		else
		{
			Target[ mip - MinimumMip ] = geBitmap_CreateLockFromMipSystem(Bmp,mip,-1);
		}
		if ( ! Target[ mip - MinimumMip ] )
		{
			geErrorLog_AddString(-1,"LockForWrite : CreateLockFromMip failed", NULL);
			mip--;
			while(mip >= MinimumMip )
			{
				geBitmap_Destroy( & Target[ mip - MinimumMip ] );
				mip--;
			}
			return GE_FALSE;
		}
	}

	assert( Bmp->LockCount == - (MaximumMip - MinimumMip + 1) );

return GE_TRUE;
}

GENESISAPI geBoolean	GENESISCC geBitmap_LockForWriteFormat(
	geBitmap *			Bmp,
	geBitmap **			Target,
	int					MinimumMip,
	int					MaximumMip,
	gePixelFormat 		Format)
{
int mip;

	assert( geBitmap_IsValid(Bmp) );
	assert( Target);
	assert(MaximumMip >= MinimumMip);
	assert( &Bmp != Target );
	
	if ( Bmp->LockCount || Bmp->LockOwner )
	{
		geErrorLog_AddString(-1,"LockForWrite : already locked", NULL);
		return GE_FALSE;
	}

	if ( Format != Bmp->Info.Format && Format != Bmp->DriverInfo.Format )
	{
		geErrorLog_AddString(-1,"LockForWriteFormat : must be System or Driver Format !", NULL);
		return GE_FALSE;
	}

	if ( Format == Bmp->DriverInfo.Format )
	{
		if ( MinimumMip < Bmp->DriverInfo.MinimumMip || MaximumMip > Bmp->DriverInfo.MaximumMip )
		{
			geErrorLog_AddString(-1,"LockForWrite : invalid Driver mip", NULL);
			return GE_FALSE;
		}
	}
	else
	{
		assert( Format == Bmp->Info.Format );

		if ( Bmp->DriverHandle )
		{
			if ( ! geBitmap_Update_DriverToSystem(Bmp) )
			{
				geErrorLog_AddString(-1,"LockForWrite : Update_DriverToSystem", NULL);
				return GE_FALSE;
			}
		}

		// create mips?

		if ( MinimumMip < Bmp->Info.MinimumMip || MaximumMip >= MAXMIPLEVELS )
		{
			geErrorLog_AddString(-1,"LockForWrite : invalid System mip", NULL);
			return GE_FALSE;
		}
		
		if ( ! geBitmap_MakeSystemMips(Bmp,Bmp->Info.MaximumMip,MaximumMip) )
			return GE_FALSE;
		Bmp->Info.MaximumMip = MaximumMip;
	}

	for(mip=MinimumMip;mip <= MaximumMip;mip ++)
	{
		if ( Bmp->DriverHandle && Format == Bmp->DriverInfo.Format )
		{
			Target[ mip - MinimumMip ] = geBitmap_CreateLockFromMipOnDriver(Bmp,mip,-1);
		}
		else
		{
			Target[ mip - MinimumMip ] = geBitmap_CreateLockFromMipSystem(Bmp,mip,-1);
		}

		if ( ! Target[ mip - MinimumMip ] )
		{
			geErrorLog_AddString(-1,"LockForWrite : CreateLockFromMip failed", NULL);
			mip--;
			while(mip >= MinimumMip )
			{
				geBitmap_Destroy( & Target[ mip - MinimumMip ] );
				mip--;
			}
			return GE_FALSE;
		}
	}

	assert( Bmp->LockCount == - (MaximumMip - MinimumMip + 1) );

return GE_TRUE;
}

GENESISAPI geBoolean GENESISCC geBitmap_LockForReadNative(
	const geBitmap *	iBmp,
	geBitmap **			Target,
	int					MinimumMip,
	int					MaximumMip)
{
int mip;
geBitmap * Bmp = (geBitmap *)iBmp;

	assert( geBitmap_IsValid(Bmp) );
	assert( Target);
	assert(MaximumMip >= MinimumMip);
	assert( &Bmp != Target );

	if ( (MinimumMip < Bmp->Info.MinimumMip && MinimumMip < Bmp->DriverInfo.MinimumMip) ||
		 (MaximumMip >= MAXMIPLEVELS) )
	{
		geErrorLog_AddString(-1,"LockForRead : invalid mip", NULL);
		return GE_FALSE;
	}

	if ( Bmp->LockCount < 0 || Bmp->LockOwner )
	{
		geErrorLog_AddString(-1,"LockForRead : already locked", NULL);
		return GE_FALSE;
	}

	for(mip=MinimumMip;mip <= MaximumMip;mip ++)
	{
		// err on the side of *not* choosing the driver data to read from !
		if ( Bmp->DriverHandle && Bmp->DriverDataChanged
			&& mip <= Bmp->DriverInfo.MaximumMip && mip >= Bmp->DriverInfo.MinimumMip)
		{
			Target[ mip - MinimumMip ] = geBitmap_CreateLockFromMipOnDriver(Bmp,mip,1);
		}
		else
		{
			Target[ mip - MinimumMip ] = geBitmap_CreateLockFromMipSystem(Bmp,mip,1);
		}
		if ( ! Target[ mip - MinimumMip ] )
		{
			geErrorLog_AddString(-1,"LockForRead : CreateLockFromMip failed", NULL);
			mip--;
			while(mip >= MinimumMip )
			{
				geBitmap_Destroy( & Target[ mip - MinimumMip ] );
				mip--;
			}
			return GE_FALSE;
		}
	}

return GE_TRUE;
}

GENESISAPI geBoolean	GENESISCC geBitmap_LockForRead(
	const geBitmap *	iBmp,
	geBitmap **			Target,
	int					MinimumMip,
	int					MaximumMip,
	gePixelFormat		Format,
	geBoolean			HasColorKey,
	uint32				ColorKey)
{
int mip;
geBitmap * Bmp = (geBitmap *)iBmp;

	assert( geBitmap_IsValid(Bmp) );
	assert( Target);
	assert(MaximumMip >= MinimumMip);
	assert( &Bmp != Target );

	if ( MinimumMip < Bmp->Info.MinimumMip ||
//		 MaximumMip > Bmp->Info.MaximumMip
		MaximumMip >= MAXMIPLEVELS )
	{
		geErrorLog_AddString(-1,"LockForRead : invalid mip", NULL);
		return GE_FALSE;
	}

	if ( Bmp->LockCount < 0 || Bmp->LockOwner )
	{
		geErrorLog_AddString(-1,"LockForRead : already locked", NULL);
		return GE_FALSE;
	}
	
	for(mip=MinimumMip;mip <= MaximumMip;mip ++)
	{
		Target[ mip - MinimumMip ] = geBitmap_CreateLockFromMip(Bmp,mip, Format,HasColorKey,ColorKey,1);
		if ( ! Target[ mip - MinimumMip ] )
		{
			geErrorLog_AddString(-1,"LockForRead : CreateLockFromMip failed", NULL);
			mip--;
			while(mip >= MinimumMip )
			{
				geBitmap_Destroy( & Target[ mip - MinimumMip ] );
				mip--;
			}
			return GE_FALSE;
		}
	}

return GE_TRUE;
}

geBoolean geBitmap_UnLockArray_NoChange(geBitmap **Locks,int Size)
{
int i;
geBoolean Ret = GE_TRUE;
	assert(Locks);
	for(i=0;i<Size;i++)
	{
		if ( ! geBitmap_UnLock_NoChange(Locks[i]) )
			Ret = GE_FALSE;
	}
return Ret;
}

GENESISAPI geBoolean	GENESISCC geBitmap_UnLockArray(geBitmap **Locks,int Size)
{
int i;
geBoolean Ret = GE_TRUE;
	assert(Locks);
	for(i=0;i<Size;i++)
	{
		if ( ! geBitmap_UnLock(Locks[i]) )
			Ret = GE_FALSE;
	}
return Ret;
}

geBoolean geBitmap_UnLock_Internal(geBitmap *Bmp,geBoolean Apply)
{
geBoolean Ret = GE_TRUE;

	if ( ! Bmp )
	{
		geErrorLog_AddString(-1,"UnLock : bad bmp", NULL);
		return GE_FALSE;
	}

	assert( Bmp->LockCount == 0 );

	if ( Bmp->LockOwner )
	{
	int DoUpdate = 0;

		assert(Bmp->LockOwner->LockCount != 0);
		if ( Bmp->LockOwner->LockCount > 0 )
		{
			Bmp->LockOwner->LockCount --;
		}
		else if ( Bmp->LockOwner->LockCount < 0 )
		{
			Bmp->LockOwner->LockCount ++;

			if ( Apply )
			{
			geBitmap_Palette * Pal;

				Bmp->LockOwner->Modified[Bmp->Info.MinimumMip] = GE_TRUE;
			
				Pal = Bmp->DriverInfo.Palette ? Bmp->DriverInfo.Palette : Bmp->Info.Palette;
				if ( Pal )
					geBitmap_SetPalette(Bmp->LockOwner,Pal);

				// this palette will be destroyed later on
			}

			if ( Bmp->LockOwner->LockCount == 0 && Apply )
			{
				// last unlock for write
				// if Bmp is on hardware, flag the Data[] as needing update
				if ( Bmp->DriverBitsLocked )
				{
					assert( Bmp->DriverHandle );
					Bmp->LockOwner->DriverDataChanged = GE_TRUE;
					DoUpdate = 1;
				}
				else
				{
					DoUpdate = -1;
				}
			}
		}
		
		if ( Bmp->DriverHandle && Bmp->DriverBitsLocked )
		{
			assert(Bmp->Driver);
			if ( ! Bmp->Driver->THandle_UnLock(Bmp->DriverHandle, Bmp->DriverMipLock) )
			{
				geErrorLog_AddString(-1,"UnLock : thandle_unlock", NULL);
				Ret = GE_FALSE;
			}
			Bmp->DriverBitsLocked = GE_FALSE;
			Bmp->DriverMipLock = 0;
		}

		if ( Bmp->Alpha )
		{
			if ( ! geBitmap_UnLock(Bmp->Alpha) )
				Ret = GE_FALSE;

			Bmp->Alpha = NULL;
		}

		if ( DoUpdate )
		{
			assert(Bmp->LockOwner->LockCount == 0 );
			// we just finished unlocking a lock-for-write
			if ( DoUpdate > 0 )
			{
			//	don't update from driver -> system, leaved the changed data on the driver
			//	we've got DriverDataChanged
			//	if ( ! geBitmap_Update_DriverToSystem(Bmp) )
			//		Ret = GE_FALSE;
			}
			else
			{
				if ( Bmp->LockOwner->DriverHandle )
					if ( ! geBitmap_Update_SystemToDriver(Bmp->LockOwner) )
						Ret = GE_FALSE;
			}
		}

		// we did a CreateRef on the lockowner
		geBitmap_Destroy(&(Bmp->LockOwner));
		Bmp->LockOwner = NULL;

	}
	// else fail ?

	assert(Bmp->RefCount == 1);

	geBitmap_Destroy(&Bmp);

	assert(Bmp == NULL);

return Ret;
}

GENESISAPI geBoolean	GENESISCC geBitmap_UnLock(geBitmap *Bmp)
{
return geBitmap_UnLock_Internal(Bmp,GE_TRUE);
}

geBoolean geBitmap_UnLock_NoChange(geBitmap *Bmp)
{
return geBitmap_UnLock_Internal(Bmp,GE_FALSE);
}

GENESISAPI void *	GENESISCC geBitmap_GetBits(geBitmap *Bmp)
{
void * bits;

	assert( geBitmap_IsValid(Bmp) );

	if ( ! Bmp )
	{
		geErrorLog_AddString(-1,"GetBits : bad bmp", NULL);
		return NULL;
	}

	if ( ! Bmp->LockOwner )	// must be a lock!
	{
		geErrorLog_AddString(-1,"GetBits : not a lock", NULL);
		return NULL;
	}

	if ( Bmp->DriverHandle )
	{
		assert(Bmp->Driver);
		if ( ! Bmp->Driver->THandle_Lock(Bmp->DriverHandle,Bmp->DriverMipLock,&bits) )
		{
			geErrorLog_AddString(-1,"GetBits : THandle_Lock", NULL);
			return NULL;
		}

		Bmp->DriverBitsLocked = GE_TRUE;
	}
	else
	{
		bits = Bmp->Data[0];
	}

return bits;
}

/*}{ ************* _CreateLock_#? *********************/

geBitmap * geBitmap_CreateLock_CopyInfo(geBitmap *BmpSrc,int LockCnt,int mip)
{
geBitmap * Bmp;

	assert( geBitmap_IsValid(BmpSrc) );

	// all _CreateLocks go through here

	Bmp = geBitmap_Create_Base();
	if ( ! Bmp )
		return NULL;

	geBitmap_MakeMipInfo(&(BmpSrc->Info),mip,&(Bmp->Info));
	geBitmap_MakeMipInfo(&(BmpSrc->DriverInfo),mip,&(Bmp->DriverInfo));
	Bmp->DriverFlags = BmpSrc->DriverFlags;
	Bmp->DriverGamma = BmpSrc->DriverGamma;
	Bmp->DriverGammaLast = BmpSrc->DriverGammaLast;
		
	Bmp->Info.Palette = NULL;
	Bmp->DriverInfo.Palette = NULL;

	Bmp->Driver	= BmpSrc->Driver;
	Bmp->PreferredFormat= BmpSrc->PreferredFormat;

	Bmp->LockOwner = BmpSrc;
	geBitmap_CreateRef(BmpSrc); // we do a _Destroy() in UnLock()

	BmpSrc->LockCount += LockCnt;

return Bmp;
}

void geBitmap_MakeMipInfo(geBitmap_Info *Src,int mip,geBitmap_Info * Target)
{
	assert( Src && Target );
	assert( mip >= 0 && mip < MAXMIPLEVELS );
	*Target = *Src;

	Target->Width  = SHIFT_R_ROUNDUP(Target->Width,mip);
	Target->Height = SHIFT_R_ROUNDUP(Target->Height,mip);
	Target->Stride = SHIFT_R_ROUNDUP(Target->Stride,mip);
	Target->MinimumMip = Target->MaximumMip = mip;
}

geBitmap * geBitmap_CreateLockFromMip(geBitmap *Src,int mip,
	gePixelFormat Format,
	geBoolean	HasColorKey,
	uint32		ColorKey,
	int			LockCnt)
{
geBitmap * Ret;

	assert( geBitmap_IsValid(Src) );
	if ( mip < 0 || mip >= MAXMIPLEVELS )
		return NULL;

	// LockForRead always goes through here

	if ( gePixelFormat_BytesPerPel(Format) < 1 )
		return NULL;

	if ( Src->DriverInfo.Format == Format &&
		 GE_BOOLSAME(Src->DriverInfo.HasColorKey,HasColorKey) &&
		 (!HasColorKey || Src->DriverInfo.ColorKey == ColorKey) &&
		 mip >= Src->DriverInfo.MinimumMip && mip <= Src->DriverInfo.MaximumMip )
	{
		return geBitmap_CreateLockFromMipOnDriver(Src,mip,LockCnt);
	}

	if ( Src->DriverHandle )
	{
		if ( ! geBitmap_Update_DriverToSystem(Src) )
		{
			return NULL;
		}
	}
		
	if ( ! Src->Data[mip] )
	{
		if ( ! geBitmap_MakeSystemMips(Src,mip,mip) )
			return NULL;
	}

	if ( Src->Info.Format == Format &&
		 GE_BOOLSAME(Src->Info.HasColorKey,HasColorKey) &&
		 (!HasColorKey || Src->Info.ColorKey == ColorKey) )
	{
		return geBitmap_CreateLockFromMipSystem(Src,mip,LockCnt);
	}

	Ret = geBitmap_CreateLock_CopyInfo(Src,LockCnt,mip);

	if ( ! Ret )
		return NULL;

	Ret->Info.Stride = Ret->Info.Width;	// {} ?

	Ret->Info.Format = Format;
	Ret->Info.ColorKey = ColorKey;
	Ret->Info.HasColorKey = HasColorKey;

	if ( gePixelFormat_HasPalette(Format) && Src->Info.Palette )
	{
		Ret->Info.Palette = Src->Info.Palette;
		geBitmap_Palette_CreateRef(Ret->Info.Palette);
	}

	assert( Ret->Alpha == NULL );
	if ( ! gePixelFormat_HasGoodAlpha(Format) && Src->Alpha )
	{
		if ( ! geBitmap_LockForRead(Src->Alpha,&(Ret->Alpha),mip,mip,GE_PIXELFORMAT_8BIT_GRAY,0,0) )
		{
			geErrorLog_AddString(-1,"CreateLockFromMip : LockForRead failed", NULL);
			geBitmap_Destroy(&Ret);
			return NULL;
		}
		assert( Ret->Alpha );
	}

	assert( geBitmap_IsValid(Ret) );

	if ( ! geBitmap_AllocSystemMip(Ret,0) )
	{
		geBitmap_Destroy(&Ret);
		return NULL;
	}

	if ( ! geBitmap_BlitMip( Src, mip, Ret, 0 ) )
	{
		geErrorLog_AddString(-1,"CreateLockFromMip : BlitMip failed", NULL);
		geBitmap_Destroy(&Ret);
		return NULL;
	}

return Ret;
}

geBitmap * geBitmap_CreateLockFromMipSystem(geBitmap *Src,int mip,int LockCnt)
{
geBitmap * Ret;

	assert( geBitmap_IsValid(Src) );
	if ( mip < Src->Info.MinimumMip || mip >= MAXMIPLEVELS )
		return NULL;

	if ( gePixelFormat_BytesPerPel(Src->Info.Format) < 1 )
		return NULL;

	if ( ! Src->Data[mip] )
	{
		if ( ! geBitmap_MakeSystemMips(Src,mip,mip) )
			return NULL;
	}

	Ret = geBitmap_CreateLock_CopyInfo(Src,LockCnt,mip);

	if ( ! Ret ) return NULL;

	Ret->Data[0] = Src->Data[mip];

	Ret->Info.Palette = Src->Info.Palette;
	if ( Ret->Info.Palette )
		geBitmap_Palette_CreateRef(Ret->Info.Palette);

	Ret->DataOwner = Src;
	geBitmap_CreateRef(Src);

	assert( Ret->Alpha == NULL );
	if ( ! gePixelFormat_HasGoodAlpha(Src->Info.Format) && Src->Alpha )
	{
		if ( ! geBitmap_LockForRead(Src->Alpha,&(Ret->Alpha),mip,mip,GE_PIXELFORMAT_8BIT_GRAY,0,0) )
		{
			geErrorLog_AddString(-1,"CreateLockFromMipSystem : LockForRead failed", NULL);
			geBitmap_Destroy(&Ret);
			return NULL;
		}
		assert( Ret->Alpha );
	}

	assert( geBitmap_IsValid(Ret) );

return Ret;
}

geBitmap * geBitmap_CreateLockFromMipOnDriver(geBitmap *Src,int mip,int LockCnt)
{
geBitmap * Ret;

	assert( geBitmap_IsValid(Src) );
	if ( ! Src->DriverHandle || ! Src->Driver || Src->DriverMipLock || mip < Src->DriverInfo.MinimumMip || mip > Src->DriverInfo.MaximumMip )
		return NULL;

	// the driver can never have Wavelet data
	// {} it could have S3TC data, though..
	assert( gePixelFormat_BytesPerPel(Src->DriverInfo.Format) > 0);

	Ret = geBitmap_CreateLock_CopyInfo(Src,LockCnt,mip);

	if ( ! Ret ) return NULL;

	Ret->DriverMipLock = mip;
	Ret->DriverHandle = Src->DriverHandle;

	Ret->DataOwner = Src;
	geBitmap_CreateRef(Src);

	Ret->DriverInfo.Palette = Src->DriverInfo.Palette;

	if ( ! geBitmap_MakeDriverLockInfo(Ret,mip,&(Ret->DriverInfo)) )
	{
		geErrorLog_AddString(-1,"CreateLockFromMipOnDriver : UpdateInfo failed", NULL);
		geBitmap_Destroy(&Ret);
		return NULL;
	}

	assert(Ret->DriverInfo.Palette == Src->DriverInfo.Palette);

	Ret->Info = Ret->DriverInfo;	//{} shouldn't be necessary
	
	if ( Ret->DriverInfo.Palette )
		geBitmap_Palette_CreateRef(Ret->DriverInfo.Palette);
	if ( Ret->Info.Palette )
		geBitmap_Palette_CreateRef(Ret->Info.Palette);

	assert( geBitmap_IsValid(Ret) );

return Ret;
}

/*}{ ************* Driver Attachment *********************/

#define MAX_DRIVER_FORMATS (100)

static geBoolean EnumPFCB(geRDriver_PixelFormat *pFormat,void *Context)
{
geRDriver_PixelFormat **pDriverFormatsPtr;
	pDriverFormatsPtr = Context;
	**pDriverFormatsPtr = *pFormat;
	(*pDriverFormatsPtr) += 1;
return GE_TRUE;
}

static int NumBitsOn(uint32 val)
{
uint32 count = 0;
	while(val)
	{
		count += val&1;
		val >>= 1;
	}
return count;
}

static geBoolean IsInArray(uint32 Val,uint32 *Array,int Len)
{
	while(Len--)
	{
		if ( Val == *Array )
			return GE_TRUE;
		Array--;
	}
return GE_FALSE;
}

geBoolean geBitmap_ChooseDriverFormat(
	gePixelFormat	SeekFormat1,
	gePixelFormat	SeekFormat2,
	geBoolean		SeekCK,
	geBoolean		SeekAlpha,
	geBoolean		SeekSeparates,
	uint32			SeekFlags,
	geRDriver_PixelFormat *DriverFormatsArray,int ArrayLen,
	geRDriver_PixelFormat *pTarget)
{
int i,rating;
int FormatRating[MAX_DRIVER_FORMATS];
geRDriver_PixelFormat * DriverPalFormat;
geRDriver_PixelFormat * pf;
geBoolean FoundAlpha;
uint32 SeekMajor,SeekMinor;
const gePixelFormat_Operations *seekops,*pfops;

	assert(pTarget && DriverFormatsArray && ArrayLen > 0);

	if ( SeekAlpha )
		SeekCK = GE_FALSE;	// you can't have both

	if ( SeekFlags & RDRIVER_PF_ALPHA )
		SeekFlags = RDRIVER_PF_ALPHA;
	else if ( SeekFlags & RDRIVER_PF_PALETTE )
		SeekFlags = RDRIVER_PF_PALETTE;

	SeekMajor = SeekFlags & RDRIVER_PF_MAJOR_MASK;
	SeekMinor = SeekFlags - SeekMajor;

	pf = DriverFormatsArray;
	DriverPalFormat = NULL;
	for(i=0;i<ArrayLen;i++)
	{
		if ( pf->Flags & RDRIVER_PF_PALETTE )
		{
			if ( ! DriverPalFormat || gePixelFormat_HasGoodAlpha(pf->PixelFormat) )
				DriverPalFormat = pf;
		}
		pf++;
	}

	seekops = gePixelFormat_GetOperations(SeekFormat1);

	for(i=0;i<ArrayLen;i++)
	{
		pf = DriverFormatsArray + i;
		rating = 0;

		/***
		{}

		this is the code to try to pick the closest format the driver has to offer
		the choosing precedence is :

			1. if want alpha, get alpha
			2. match the _3D_ type flags exactly (PF_MAJOR)
			3. match PreferredFormat
			4. match the bitmap's system format
			5. match up the pixel formats masks

		we use fuzzy logic : we apply rating to all the matches and then choose
		the one with the best rating.

		possible todos :
			1. be aware of memory use and format-conversion times, and penalize
				or favor formats accordingly

		note : we currently try to use 1555 for colorkey.

		***/

		if ( (pf->Flags & RDRIVER_PF_MAJOR_MASK) == SeekMajor )
		{
			rating += 1<<23;
		}
		else if ( (pf->Flags & SeekMajor) != SeekMajor )
		{
		    FormatRating[i] = 0;
			continue;
		}
		else
		{
			// advantage to as few different major flags as possible
			// higher priority than similarity of pixelformats
			rating += (32 - NumBitsOn( (pf->Flags ^ SeekFlags) & RDRIVER_PF_MAJOR_MASK ))<<6;
		}

		pfops = gePixelFormat_GetOperations(pf->PixelFormat);

		if ( pf->PixelFormat == SeekFormat1 )
		{
			rating += 1<<16;
		}
		else if ( pf->PixelFormat == SeekFormat2 )
		{
			rating += 1<<15;
		}
		else if ( gePixelFormat_IsRaw(SeekFormat1) && gePixelFormat_IsRaw(pf->PixelFormat) )
		{
		int R,G,B,A;
			// measure similarity
			R = (seekops->RMask >> seekops->RShift) ^ (pfops->RMask >> pfops->RShift);
			G = (seekops->GMask >> seekops->GShift) ^ (pfops->GMask >> pfops->GShift);
			B = (seekops->BMask >> seekops->BShift) ^ (pfops->BMask >> pfops->BShift);
			A = (seekops->AMask >> seekops->AShift) ^ (pfops->AMask >> pfops->AShift);
			R = 8 - NumBitsOn(R);
			G = 8 - NumBitsOn(G);
			B = 8 - NumBitsOn(B);
			A = 4*(8 - NumBitsOn(A)); // right number of A bits is molto importante
			rating += (R + G + B + A);
		}
		else
		{
			// one of the formats is not raw
			// palettized ? compressed ?
			rating += 16;
		}

		FoundAlpha = GE_FALSE;

		if ( NumBitsOn(pfops->AMask) > 2 )
			FoundAlpha = GE_TRUE;

		if ( gePixelFormat_HasPalette(pf->PixelFormat) )
		{
            // if Pixelformat is 8BIT_PAL , look at the palette's format to see if it
			//		had alpha!
			assert(DriverPalFormat);
			if ( gePixelFormat_HasGoodAlpha(DriverPalFormat->PixelFormat) )
				FoundAlpha = GE_TRUE;
		}

		if ( SeekAlpha && FoundAlpha )
			rating += 1<<24; // HIGHEST ! (except separates)
		else if ( (!SeekAlpha) && (!FoundAlpha) )
			rating += 1<<10; // LOWEST
		else if ( SeekAlpha )
		{
			if ( NumBitsOn(pfops->AMask) || pf->Flags & RDRIVER_PF_CAN_DO_COLORKEY )
			{
				// sought Alpha, found CK
				rating += 1<<19;
			}
		}

		if ( (pf->Flags & RDRIVER_PF_HAS_ALPHA) && SeekSeparates )
		{
			// separates doesn't count as alpha unless we asked for it !
			rating += 1<<25; // VERY HIGHEST !
		}
	
		if ( SeekCK ) 
		{
			if ( pf->PixelFormat == GE_PIXELFORMAT_16BIT_1555_ARGB )
			{
				rating += 1<<23; // just lower than alpha
			}
			else if ( FoundAlpha ) //better than nothing!
			{
				rating += 1<<18;	// higher than !FoundAlpha
			}
		}

		if ( GE_BOOLSAME((pf->Flags & RDRIVER_PF_CAN_DO_COLORKEY),SeekCK) )
		{
			rating += 1<<17;
		}

		FormatRating[i] = rating;
	}

	rating = 0;

	for(i=0;i<ArrayLen;i++)
	{
		if ( FormatRating[i] > rating )
		{
			rating = FormatRating[i];
			*pTarget = DriverFormatsArray[i];
		}
	}

	if ( rating == 0)
	{
		geErrorLog_AddString(-1,"ChooseDriverFormat : no valid formats found!", NULL);
		return GE_FALSE;
	}

	return GE_TRUE;
}
#if 0
geBoolean geBitmap_ChooseDriverFormat(
	gePixelFormat	SeekFormat1,
	gePixelFormat	SeekFormat2,
	geBoolean		SeekCK,
	geBoolean		SeekAlpha,
	geBoolean		SeekSeparates,
	uint32			SeekFlags,
	geRDriver_PixelFormat *DriverFormatsArray,int ArrayLen,
	geRDriver_PixelFormat *pTarget)
{
int i,rating;
int FormatRating[MAX_DRIVER_FORMATS];
geRDriver_PixelFormat * DriverPalFormat;
geRDriver_PixelFormat * pf;
geBoolean FoundAlpha;
uint32 SeekMajor,SeekMinor;
const gePixelFormat_Operations *seekops,*pfops;

	assert(pTarget && DriverFormatsArray && ArrayLen > 0);

#if 0 // @@
	if ( SeekAlpha )
		SeekCK = GE_FALSE;	// you can't have both, you bastard!
#endif

	if ( SeekFlags & RDRIVER_PF_ALPHA )
		SeekFlags = RDRIVER_PF_ALPHA;
	else if ( SeekFlags & RDRIVER_PF_PALETTE )
		SeekFlags = RDRIVER_PF_PALETTE;

	if ( ! SeekFormat1 )
		SeekFormat1 = SeekFormat2;

	SeekMajor = SeekFlags & RDRIVER_PF_MAJOR_MASK;
	SeekMinor = SeekFlags - SeekMajor;

	pf = DriverFormatsArray;
	DriverPalFormat = NULL;
	for(i=0;i<ArrayLen;i++)
	{
		if ( pf->Flags & RDRIVER_PF_PALETTE )
		{
			assert( gePixelFormat_IsRaw(pf->PixelFormat) );
			if ( ! DriverPalFormat || gePixelFormat_HasGoodAlpha(pf->PixelFormat) )
				DriverPalFormat = pf;
		}
		pf++;
	}

	seekops = gePixelFormat_GetOperations(SeekFormat1);

	for(i=0;i<ArrayLen;i++)
	{
		pf = DriverFormatsArray + i;
		rating = 0;

		/***
		{}

		this is the code to try to pick the closest format the driver has to offer
		the choosing precedence is :

			1. if want alpha, get alpha
			2. match the _3D_ type flags exactly
			3. match PreferredFormat
			4. match the bitmap's system format
			5. match up the formats masks

		***/

		if ( (pf->Flags & RDRIVER_PF_MAJOR_MASK) == SeekMajor )
		{
			rating += 1<<23;
		}
		else if ( (pf->Flags & SeekMajor) != SeekMajor )
		{
		    FormatRating[i] = 0;
			continue;
		}
		else
		{
			// advantage to as few different major flags as possible
			// higher priority than similarity of pixelformats
			rating += (32 - NumBitsOn( (pf->Flags ^ SeekFlags) & RDRIVER_PF_MAJOR_MASK )) <<6;
		}

		pfops = gePixelFormat_GetOperations(pf->PixelFormat);

		if ( pf->PixelFormat == SeekFormat1 )
		{
			rating += 1<<22;
		}
		else if ( pf->PixelFormat == SeekFormat2 )
		{
			rating += 1<<21;
		}
		else if ( gePixelFormat_IsRaw(SeekFormat1) && gePixelFormat_IsRaw(pf->PixelFormat) )
		{
		int R,G,B,A;
			// measure similarity
			R = (seekops->RMask >> seekops->RShift) ^ (pfops->RMask >> pfops->RShift);
			G = (seekops->GMask >> seekops->GShift) ^ (pfops->GMask >> pfops->GShift);
			B = (seekops->BMask >> seekops->BShift) ^ (pfops->BMask >> pfops->BShift);
			A = (seekops->AMask >> seekops->AShift) ^ (pfops->AMask >> pfops->AShift);
			R = 8 - NumBitsOn(R);
			G = 8 - NumBitsOn(G);
			B = 8 - NumBitsOn(B);
			A = 4*(8 - NumBitsOn(A)); // right number of A bits is molto importante
			rating += R + G + B + A;
		}

		FoundAlpha = GE_FALSE;

		if ( NumBitsOn(pfops->AMask) > 2 )
			FoundAlpha = GE_TRUE;

		if ( gePixelFormat_HasPalette(pf->PixelFormat) )
		{
                        // if Pixelformat is 8BIT_PAL , look at the palette's format to see if it
			//		had alpha!
			assert(DriverPalFormat);
			if ( gePixelFormat_HasGoodAlpha(DriverPalFormat->PixelFormat) )
				FoundAlpha = GE_TRUE;
		}

		if ( SeekAlpha && FoundAlpha )
			rating += 1<<24; // HIGHEST ! (except separates)
		else if ( (!SeekAlpha) && (!FoundAlpha) )
			rating += 1<<16; // LOWEST

		if ( (pf->Flags & RDRIVER_PF_HAS_ALPHA) && SeekSeparates )
		{
			// separates doesn't count as alpha unless we asked for it !
			rating += 1<<25; // VERY HIGHEST !
		}
	
		if ( (pf->PixelFormat == GE_PIXELFORMAT_16BIT_1555_ARGB) && SeekCK )
		{
			rating += 1<<23; // just lower than alpha
		}
		else if ( GE_BOOLSAME((pf->Flags & RDRIVER_PF_CAN_DO_COLORKEY),SeekCK) )
		{
			rating += 1<<20;
		}

		if ( SeekCK && FoundAlpha ) //better than nothing!
		{
			rating += 1<<17;	// higher than !FoundAlpha
		}

		FormatRating[i] = rating;
	}

	rating = 0;

	for(i=0;i<ArrayLen;i++)
	{
		if ( FormatRating[i] > rating )
		{
			rating = FormatRating[i];
			*pTarget = DriverFormatsArray[i];
		}
	}

	if ( rating == 0)
	{
		geErrorLog_AddString(-1,"ChooseDriverFormat : no valid formats found!", NULL);
		return GE_FALSE;
	}

	return GE_TRUE;
}
#endif

geRDriver_THandle * geBitmap_CreateTHandle(DRV_Driver *Driver,int Width,int Height,int NumMipLevels,
			gePixelFormat SeekFormat1,gePixelFormat SeekFormat2,geBoolean SeekCK,geBoolean SeekAlpha,geBoolean SeekSeparates,uint32 DriverFlags)
{
geRDriver_PixelFormat DriverFormats[MAX_DRIVER_FORMATS];
geRDriver_PixelFormat *DriverFormatsPtr;
int DriverFormatsCount;
geRDriver_PixelFormat DriverFormat;
geRDriver_THandle * Ret;

	DriverFormatsPtr = DriverFormats;
	Driver->EnumPixelFormats(EnumPFCB,&DriverFormatsPtr);
	DriverFormatsCount = ((uint32)DriverFormatsPtr - (uint32)DriverFormats)/sizeof(*DriverFormatsPtr);
	assert(DriverFormatsCount < MAX_DRIVER_FORMATS && DriverFormatsCount >= 0);

	if ( DriverFormatsCount == 0 )
	{
		geErrorLog_AddString(-1,"Bitmap_CreateTHandle : no formats found!", NULL);
		return NULL;
	}

	if ( ! SeekFormat1 )
		SeekFormat1 = SeekFormat2;
	else if ( ! SeekFormat2 )
		SeekFormat2 = SeekFormat1;

	assert( gePixelFormat_IsValid(SeekFormat1) );
	assert( gePixelFormat_IsValid(SeekFormat2) );

	// now choose DriverFormat
	if ( ! geBitmap_ChooseDriverFormat(SeekFormat1,SeekFormat2,SeekCK,SeekAlpha,SeekSeparates,DriverFlags,
										DriverFormats,DriverFormatsCount,&DriverFormat) )
		return NULL;

	assert( gePixelFormat_IsValid(DriverFormat.PixelFormat) );

#if 1 //{
	Log_Printf("Bitmap : Chose %s for %s",
		gePixelFormat_Description(DriverFormat.PixelFormat),
		gePixelFormat_Description(SeekFormat1));

	if ( SeekFormat1 != SeekFormat2 )
		Log_Printf(" (%s)",gePixelFormat_Description(SeekFormat2));
	if ( SeekCK )
		Log_Printf(" (sought CK)");
	if ( SeekAlpha )
		Log_Printf(" (sought Alpha)");
	if ( DriverFormat.Flags & RDRIVER_PF_2D )
		Log_Printf(" (2D)");
	if ( DriverFormat.Flags & RDRIVER_PF_3D )
		Log_Printf(" (3D)");
	if ( DriverFormat.Flags & RDRIVER_PF_CAN_DO_COLORKEY )
		Log_Printf(" (can CK)");
	if ( DriverFormat.Flags & RDRIVER_PF_PALETTE )
		Log_Printf(" (Palette)");
	if ( DriverFormat.Flags & RDRIVER_PF_ALPHA )
		Log_Printf(" (Alpha)");
	if ( DriverFormat.Flags & RDRIVER_PF_LIGHTMAP )
		Log_Printf(" (Lightmap)");

	Log_Printf("\n");
#endif //}

	Ret = Driver->THandle_Create(
		Width,Height,
		NumMipLevels,
		&DriverFormat);

	if ( ! Ret )
	{
		geErrorLog_AddString(-1, Driver->LastErrorStr, NULL);
		geErrorLog_AddString(-1,"Bitmap_CreateTHandle : Driver->THandle_Create failed", NULL);
	}

return Ret;
}

GENESISAPI	geBoolean	GENESISCC	geBitmap_HasAlpha(const geBitmap * Bmp)
{
	assert( geBitmap_IsValid(Bmp) );
	
	if ( Bmp->Alpha )
		return GE_TRUE;

	if ( gePixelFormat_HasGoodAlpha(Bmp->Info.Format) )
		return GE_TRUE;

	if ( gePixelFormat_HasPalette(Bmp->Info.Format) && Bmp->Info.Palette )
	{
		if ( gePixelFormat_HasGoodAlpha(Bmp->Info.Palette->Format) )
			return GE_TRUE;
	}	

return GE_FALSE;
}

geBoolean	BITMAP_GENESIS_INTERNAL geBitmap_AttachToDriver(geBitmap *Bmp, 
	DRV_Driver * Driver, uint32 DriverFlags)
{

	/**************
	* When you want to change the Driver,
	* I still need a copy of the old one to get the bits out
	* of the old THandles.  That is:
	* 
	* 	AttachDriver(Bmp,Driver)
	* 	<do stuff>
	* 	Change Driver Entries
	* 	AttachDriver(Bmp,Driver)
	* 
	* is forbidden!  The two different options are :
	* 
	* 	1.
	* 
	* 	AttachDriver(Bmp,Driver)
	* 	<do stuff>
	* 	DetachDriver(Bmp)
	* 	Change Driver Entries
	* 	AttachDriver(Bmp,Driver)
	* 
	* 	2.
	* 
	* 	AttachDriver(Bmp,Driver1)
	* 	<do stuff>
	* 	Driver2 = copy of Driver1
	* 	Change Driver2 Entries
	* 	AttachDriver(Bmp,Driver2)
	* 	Free Driver1
	* 
	* This isn't so critical when just changing modes,
	* but is critical when changing drivers.
	* 
	****************/

	assert( geBitmap_IsValid(Bmp) );

	if ( Bmp->LockOwner || Bmp->DataOwner || Bmp->LockCount )
	{
		geErrorLog_AddString(-1,"AttachToDriver : not an isolated bitmap", NULL);
		return GE_FALSE;
	}

	if ( Bmp->DriverHandle && Bmp->Driver == Driver )
	{
		assert( DriverFlags == 0 || DriverFlags == Bmp->DriverFlags );
		return GE_TRUE;
	}

	if ( ! geBitmap_DetachDriver(Bmp,GE_TRUE) )
	{
		geErrorLog_AddString(-1,"AttachToDriver : detach failed", NULL);
		return GE_FALSE;
	}

	if ( DriverFlags == 0 )
	{
		DriverFlags = Bmp->DriverFlags;
		if ( ! DriverFlags )
		{
			//	return GE_FALSE;
			// ? {}
			DriverFlags = RDRIVER_PF_3D;
		}
	}

	if ( Driver )
	{
	int NumMipLevels;
	int Width,Height;
	geBoolean WantAlpha;
	geRDriver_THandle * DriverHandle;

//		if ( Bmp->DriverFlags & RDRIVER_PF_COMBINE_LIGHTMAP )
//			Bmp->SeekMipCount = max(Bmp->SeekMipCount,4);

		NumMipLevels = max(Bmp->SeekMipCount,(Bmp->Info.MaximumMip + 1));
		if ( NumMipLevels > 4 ) NumMipLevels = 4; // {} kind of a hack, our drivers ignore mips > 4

		// make sizes power-of-two and square
		// {} note : must let drivers do this to correctly scale UV's 
		Width	= Bmp->Info.Width;
		Height	= Bmp->Info.Height;

		WantAlpha = geBitmap_HasAlpha(Bmp);
		if ( gePixelFormat_HasGoodAlpha(Bmp->PreferredFormat) )
			WantAlpha = GE_TRUE;

		assert( geBitmap_IsValid(Bmp) );

		DriverHandle = geBitmap_CreateTHandle(Driver,Width,Height,NumMipLevels,
			Bmp->PreferredFormat,Bmp->Info.Format,Bmp->Info.HasColorKey,
			WantAlpha, (Bmp->Alpha) ? GE_TRUE : GE_FALSE,
			DriverFlags);

		assert( geBitmap_IsValid(Bmp) );

		if ( ! DriverHandle )
			return GE_FALSE;

		Bmp->DriverHandle = DriverHandle;
		Bmp->Driver = Driver;
		Bmp->DriverFlags = DriverFlags;

#ifdef _DEBUG
		Bmp->DriverInfo = Bmp->Info;
		assert( geBitmap_IsValid(Bmp) );
#endif
		clear(&(Bmp->DriverInfo));

		if ( ! geBitmap_MakeDriverLockInfo(Bmp,0,&(Bmp->DriverInfo)) )
		{
			geErrorLog_AddString(-1,"AttachToDriver : updateinfo", NULL);
			return GE_FALSE;
		}

		Bmp->DriverInfo.MinimumMip = 0;
		Bmp->DriverInfo.MaximumMip = NumMipLevels - 1;
		
		assert( geBitmap_IsValid(Bmp) );

		assert( geBitmap_IsValid(Bmp) );

		if ( ! geBitmap_Update_SystemToDriver(Bmp) )
		{
			geErrorLog_AddString(-1,"AttachToDriver : Update_SystemToDriver", NULL);
			Driver->THandle_Destroy(Bmp->DriverHandle);
			Bmp->DriverHandle = NULL;
			return GE_FALSE;
		}

		// {} Palette : Update_System calls Blit_Data, which should build it for us 
		//		if Driver is pal & System isn't
	}

return GE_TRUE;
}

geBoolean geBitmap_FixDriverFlags(uint32 *pFlags)
{
uint32 DriverFlags;
	assert(pFlags);
	DriverFlags = *pFlags;
	
	if ( DriverFlags & RDRIVER_PF_COMBINE_LIGHTMAP )
		DriverFlags |= RDRIVER_PF_3D;
	if ( DriverFlags & RDRIVER_PF_CAN_DO_COLORKEY )
	{
		// <> someone is doing this!
		// bad!
		DriverFlags ^= RDRIVER_PF_CAN_DO_COLORKEY;
		//	return GE_FALSE;
	}
	if ( (DriverFlags & RDRIVER_PF_COMBINE_LIGHTMAP) &&
		(DriverFlags & (RDRIVER_PF_LIGHTMAP | RDRIVER_PF_PALETTE) ) )
		return GE_FALSE;
	if ( NumBitsOn(DriverFlags & RDRIVER_PF_MAJOR_MASK) == 0 )
		return GE_FALSE;
	*pFlags = DriverFlags;
return GE_TRUE;
}

geBoolean BITMAP_GENESIS_INTERNAL geBitmap_SetDriverFlags(geBitmap *Bmp,uint32 Flags)
{
	assert( geBitmap_IsValid(Bmp) );
	assert(Flags);
	if ( ! geBitmap_FixDriverFlags(&Flags) )
	{
		Bmp->DriverFlags = 0;
		return GE_FALSE;
	}
	Bmp->DriverFlags = Flags;
return GE_TRUE;
}

geBoolean BITMAP_GENESIS_INTERNAL geBitmap_DetachDriver(geBitmap *Bmp,geBoolean DoUpdate)
{
geBoolean Ret = GE_TRUE;

	assert(geBitmap_IsValid(Bmp) );

	if ( Bmp->LockOwner || Bmp->DataOwner || Bmp->LockCount )
	{
		geErrorLog_AddString(-1,"DetachDriver : not an isolated bitmap!", NULL);
		return GE_FALSE;
	}

	if ( Bmp->RefCount > 1 )
		DoUpdate = GE_TRUE;

	if ( Bmp->Driver && Bmp->DriverHandle )
	{
		if ( DoUpdate )
		{
			if ( ! geBitmap_Update_DriverToSystem(Bmp) )
			{
				geErrorLog_AddString(-1,"DetachDriver : Update_DriverToSystem", NULL);
				Ret = GE_FALSE;
			}
			assert(Bmp->DriverDataChanged == GE_FALSE);
		}
			Bmp->Driver->THandle_Destroy(Bmp->DriverHandle);
		Bmp->DriverHandle = NULL;
	}

	if ( Bmp->DriverInfo.Palette )
	{
		// save it for later in case we re-attach
		if ( ! Bmp->Info.Palette )
		{
		geBitmap_Palette * NewPal;
		gePixelFormat Format;
			Format = Bmp->DriverInfo.Palette->Format;
			NewPal = geBitmap_Palette_Create(Format,256);
			if ( NewPal )
			{
				if ( geBitmap_Palette_Copy(Bmp->DriverInfo.Palette,NewPal) )
				{
					Bmp->Info.Palette = NewPal;
				}
				else
				{
					geBitmap_Palette_Destroy(&NewPal);
				}
			}
		}

		geBitmap_Palette_Destroy(&(Bmp->DriverInfo.Palette));
	}

	if ( Bmp->Alpha )
	{
		if ( ! geBitmap_DetachDriver(Bmp->Alpha,DoUpdate) )
		{
			geErrorLog_AddString(-1,"DetachDriver : detach alpha", NULL);
			Ret = GE_FALSE;
		}
	}

	Bmp->DriverInfo.Width = Bmp->DriverInfo.Height = Bmp->DriverInfo.Stride = 0;
	Bmp->DriverInfo.MinimumMip = Bmp->DriverInfo.MaximumMip = 0;
	Bmp->DriverInfo.ColorKey = Bmp->DriverInfo.HasColorKey = Bmp->DriverInfo.Format = 0;
	Bmp->DriverInfo.Palette = NULL;
	Bmp->DriverMipLock = 0;
	Bmp->DriverBitsLocked = GE_FALSE;
	Bmp->DriverDataChanged = GE_FALSE;
	Bmp->DriverHandle = NULL;
	Bmp->Driver = NULL;

	//Bmp->DriverFlags left intentionally !

return Ret;
}

geBoolean GENESISCC geBitmap_SetGammaCorrection_DontChange(geBitmap *Bmp,geFloat Gamma)
{
	assert(geBitmap_IsValid(Bmp));
	assert( Gamma > 0.0f );

	if ( ! Bmp->DriverGammaSet )
	{
		Bmp->DriverGammaLast = Bmp->DriverGamma;
		Bmp->DriverGamma = Gamma;
	}

return GE_TRUE;
}

GENESISAPI geBoolean GENESISCC geBitmap_SetGammaCorrection(geBitmap *Bmp,geFloat Gamma,geBoolean Apply)
{
	assert(geBitmap_IsValid(Bmp));
	assert( Gamma > 0.0f );

	/***

	there are actually some anomalies involved in exposing this to the user:
		if the driver's gamma correction is turned on, then this Gamma will be applied *in addition* to the driver gamma.
	There's no easy way to avoid that problem.

	we like to expose this to the user so that they can disable software gamma correction on some bitmaps
		(eg. procedurals)
	perhaps provide a Bitmap_DisableGamma(Bmp) instead of SetGamma ?

	**/

	if ( Apply && Bmp->DriverHandle )
	{
		if ( fabs(Bmp->DriverGamma - Gamma) > 0.1f )
		{
			if ( gePixelFormat_BytesPerPel(Bmp->Info.Format) == 0 && Bmp->DriverHandle )
			{
				// system format is compressed, and Bmp is on the card

				if ( (Bmp->DriverGammaLast >= Bmp->DriverGamma && Bmp->DriverGamma >= Gamma) ||
					 (Bmp->DriverGammaLast <= Bmp->DriverGamma && Bmp->DriverGamma <= Gamma) )
				{
					// moving in the same direction

					// invert the old
					if ( ! geBitmap_Gamma_Apply(Bmp,GE_TRUE) )
						return GE_FALSE;

					Bmp->DriverGammaLast = Bmp->DriverGamma;
					Bmp->DriverGamma = Gamma;

					// apply the new
					if ( ! geBitmap_Gamma_Apply(Bmp,GE_FALSE) )
						return GE_FALSE;
				}
				else
				{
					// changed direction so must do an update

					if ( ! geBitmap_Update_DriverToSystem(Bmp) )
						return GE_FALSE;

					Bmp->DriverGammaLast = Bmp->DriverGamma = Gamma;
					
					if ( ! geBitmap_Update_SystemToDriver(Bmp) )
						return GE_FALSE;
				}
			}
			else
			{
				if ( ! geBitmap_Update_DriverToSystem(Bmp) )
					return GE_FALSE;

				Bmp->DriverGammaLast = Bmp->DriverGamma = Gamma;
				
				if ( ! geBitmap_Update_SystemToDriver(Bmp) )
					return GE_FALSE;
			}
		}
	}
	else
	{
		Bmp->DriverGamma = Gamma;
	}
	Bmp->DriverGammaSet = GE_TRUE;

return GE_TRUE;
}

geRDriver_THandle * BITMAP_GENESIS_INTERNAL geBitmap_GetTHandle(const geBitmap *Bmp)
{
//	assert( geBitmap_IsValid(Bmp) );

	return Bmp->DriverHandle;
}

geBoolean geBitmap_Update_SystemToDriver(geBitmap *Bmp)
{
geBitmap * SrcLocks[MAXMIPLEVELS];
geBoolean Ret,MipsChanged;
int mip,mipMin,mipMax;
geRDriver_THandle * SaveDriverHandle;
geBitmap * SaveAlpha;
int SaveMaxMip;
	
	assert( geBitmap_IsValid(Bmp) );

	/**

		this function is totally hacked out, because what we really need to do
		is lock Bmp for Read (system) & Write (driver) , but that's illegal!

	**/

	if ( Bmp->LockOwner )
		Bmp = Bmp->LockOwner;

	if ( Bmp->LockCount > 0 || Bmp->DataOwner )
	{
		geErrorLog_AddString(-1,"Update_SystemToDriver : not an original bitmap", NULL);
		return GE_FALSE;
	}

	if ( ! Bmp->DriverHandle )
	{
		geErrorLog_AddString(-1,"Update_SystemToDriver : no driver data", NULL);
		return GE_FALSE;
	}

	MipsChanged = GE_FALSE;
	for(mip=Bmp->DriverInfo.MinimumMip;mip<=Bmp->DriverInfo.MaximumMip;mip++)
	{
		if ( Bmp->Modified[mip] && mip != Bmp->Info.MinimumMip )
		{
			assert(Bmp->Data[mip]);
			MipsChanged = GE_TRUE;
		}
	}

	//make mips after driver blit
	
#if 0 
	MipsChanged = GE_TRUE;
#endif

	mipMin = Bmp->DriverInfo.MinimumMip;

	if ( MipsChanged )
		mipMax = Bmp->DriverInfo.MaximumMip;
	else
		mipMax = mipMin;


	SaveDriverHandle = Bmp->DriverHandle;
	Bmp->DriverHandle = NULL;	// so Lock() won't use the driver data

	SaveAlpha = Bmp->Alpha;

	if ( Bmp->Alpha && ! gePixelFormat_HasGoodAlpha(Bmp->DriverInfo.Format) && 
			(Bmp->DriverFlags & RDRIVER_PF_HAS_ALPHA) )
	{
		// hide the alpha so that it won't be used to make a colorkey in the target
		// we'll blit it independently later
		Bmp->Alpha = NULL;
	}

	if ( ! geBitmap_LockForReadNative(Bmp,SrcLocks,mipMin,mipMax) )
	{
		geErrorLog_AddString(-1,"Update_SystemToDriver : LockForReadNative", NULL);
		return GE_FALSE;
	}

	Ret = GE_TRUE;
	Bmp->DriverHandle = SaveDriverHandle;
	Bmp->Alpha = NULL;

	// we should have always updated the driver to system before fiddling the system
	assert( ! Bmp->DriverDataChanged );

	/**

		Bmp is a driver BMP

		the SrcLocks are locks of the system bits.

	**/

	for(mip=mipMin;mip <=mipMax;mip++)
	{
	geBitmap *SrcMip;
	void * SrcBits,*DstBits;
	geBitmap_Info DstInfo;

		SrcMip = SrcLocks[mip - mipMin];
		SrcBits = geBitmap_GetBits(SrcMip);

		DstInfo = Bmp->DriverInfo;

		if ( ! geBitmap_MakeDriverLockInfo(Bmp,mip,&DstInfo) )
		{
			geErrorLog_AddString(-1,"Update_SystemToDriver : MakeInfo", NULL);
			Ret = GE_FALSE;
			continue;
		}

		// Zooma! THandle_Lock might lock the Win16 Lock !
		//	this is really bad when _BlitData is a wavelet decompress !
		// {} try this : decompress to a buffer in memory (on a thread)
		//	then THandle_Lock and just do a (prefetching) memcpy
		//#pragma message("Bitmap : minimize time spent in a THandle_Lock!")

		if ( ! Bmp->Driver->THandle_Lock(SaveDriverHandle,mip,&DstBits) )
		{
			geErrorLog_AddString(-1,"Update_SystemToDriver : THandle_Lock", NULL);
			Ret = GE_FALSE;
			continue;
		}

		if ( ! SrcBits || ! DstBits )
		{
			geErrorLog_AddString(-1,"Update_SystemToDriver : No Bits", NULL);
			Ret = GE_FALSE;
			continue;
		}

		assert( DstInfo.Palette == Bmp->DriverInfo.Palette );

		if ( ! geBitmap_BlitData(	&(SrcMip->Info),SrcBits,SrcMip,
									&DstInfo,		DstBits,Bmp,
									SrcMip->Info.Width,SrcMip->Info.Height) )
		{
			geErrorLog_AddString(-1,"Update_SystemToDriver : BlitData", NULL);
			//assert(0);
			Ret = GE_FALSE;
			continue;
		}

		if ( ! Bmp->Driver->THandle_UnLock(SaveDriverHandle,mip) )
		{
			geErrorLog_AddString(-1,"Update_SystemToDriver : THandle_UnLock", NULL);
			Ret = GE_FALSE;
			continue;
		}

		// normally this would be done by the Bitmap_UnLock ,
		//  but since we don't lock ..
		if ( DstInfo.Palette != Bmp->DriverInfo.Palette )
		{
			//assert( OldDstPal == NULL );
			geBitmap_SetPalette(Bmp,DstInfo.Palette);
			geBitmap_Palette_Destroy(&(DstInfo.Palette));
			// must destroy here, since DstInfo is on the stack!
		}
	}

	Bmp->Alpha = SaveAlpha;
	Bmp->DriverBitsLocked = GE_FALSE;
	Bmp->DriverMipLock = 0;
	Bmp->DriverDataChanged = GE_FALSE;

	geBitmap_UnLockArray(SrcLocks, mipMax - mipMin + 1 );

	if ( ! Ret )
	{
		geErrorLog_AddString(-1,"Update_SystemToDriver : Locking and Blitting error", NULL);
	}

	if ( Bmp->Alpha && ! gePixelFormat_HasGoodAlpha(Bmp->DriverInfo.Format) && 
			(Bmp->DriverFlags & RDRIVER_PF_HAS_ALPHA) )
	{
	geRDriver_THandle * AlphaTH;

		// blit the alpha surface to the separate alpha

		AlphaTH = Bmp->Driver->THandle_GetAlpha(Bmp->DriverHandle);
		if ( !AlphaTH || AlphaTH != Bmp->Alpha->DriverHandle)
		{
			if ( ! geBitmap_AttachToDriver(Bmp->Alpha,Bmp->Driver,Bmp->Alpha->DriverFlags | RDRIVER_PF_ALPHA) )
			{
				geErrorLog_AddString(-1,"AttachToDriver : attach Alpha", NULL);
				return GE_FALSE;
			}

			assert(Bmp->Alpha->DriverHandle);
			if ( ! Bmp->Driver->THandle_SetAlpha(Bmp->DriverHandle,Bmp->Alpha->DriverHandle) )
			{
				geErrorLog_AddString(-1,"AttachToDriver : THandle_SetAlpha", NULL);
				geBitmap_DetachDriver(Bmp->Alpha,GE_FALSE);
				geBitmap_DetachDriver(Bmp,GE_FALSE);
				return GE_FALSE;
			}
			
			AlphaTH = Bmp->Driver->THandle_GetAlpha(Bmp->DriverHandle);
			assert(AlphaTH == Bmp->Alpha->DriverHandle);
		}
	}

	// now bits are on driver , gamma correct them

	//for gamma : just Gamma up to mipMax then make mips from it
	//	seems to work

	SaveMaxMip = Bmp->DriverInfo.MaximumMip;
	Bmp->DriverInfo.MaximumMip = mipMax;
	if ( ! geBitmap_Gamma_Apply(Bmp,GE_FALSE) )
	{
		geErrorLog_AddString(-1,"AttachToDriver : Gamma_Apply failed!", NULL);
		Ret = GE_FALSE;
	}
	Bmp->DriverInfo.MaximumMip = SaveMaxMip;

	if ( ! MipsChanged && mipMax < Bmp->DriverInfo.MaximumMip )
	{
		for(mip=mipMax+1;mip<= Bmp->DriverInfo.MaximumMip; mip++)
		{
			if ( ! geBitmap_UpdateMips(Bmp,mip-1,mip) )
			{
				geErrorLog_AddString(-1,"AttachToDriver : UpdateMips on driver failed!", NULL);
				return GE_FALSE;
			}
		}
	}

	Bmp->DriverDataChanged = GE_FALSE; // in case _SetPal freaks us out

return Ret;
}

geBoolean geBitmap_Update_DriverToSystem(geBitmap *Bmp)
{
geBitmap *DriverLocks[MAXMIPLEVELS];
geBoolean Ret;
int mip;
	
	assert( geBitmap_IsValid(Bmp) );

	if ( Bmp->LockOwner )
		Bmp = Bmp->LockOwner;

	if ( Bmp->LockCount > 0 || Bmp->DataOwner )
	{
		geErrorLog_AddString(-1,"Update_DriverToSystem : not an original bitmap", NULL);
		return GE_FALSE;
	}

	if ( ! Bmp->DriverHandle )
	{
		geErrorLog_AddString(-1,"Update_DriverToSystem : no driver data", NULL);
		return GE_FALSE;
	}

	if ( ! Bmp->DriverDataChanged )
		return GE_TRUE;

	// bits are on driver; undo the gamma to copy them home

	Log_Puts("Bitmap : Doing Update_DriverToSystem");

	if ( ! geBitmap_Gamma_Apply(Bmp,GE_TRUE) ) // undo the gamma!
		return GE_FALSE;

	if ( Bmp->Info.Palette && Bmp->DriverInfo.Palette )
	{
		if ( ! geBitmap_Palette_Copy(Bmp->DriverInfo.Palette,Bmp->Info.Palette) )
		{
			geErrorLog_AddString(-1,"Update_DriverToSystem : Palette_Copy", NULL);
		}
	}

	if ( geBitmap_LockForReadNative(Bmp,DriverLocks,
			Bmp->DriverInfo.MinimumMip,Bmp->DriverInfo.MaximumMip) )
	{
		Ret = GE_TRUE;

		for(mip=Bmp->DriverInfo.MinimumMip;mip <=Bmp->DriverInfo.MaximumMip;mip++)
		{	
		geBitmap *MipBmp;
		geBitmap_Info SystemInfo;

			MipBmp = DriverLocks[mip];

			if ( Bmp->Modified[mip] )
			{
			void *DriverBits,*SystemBits;
				DriverBits = geBitmap_GetBits(MipBmp);
				assert( MipBmp->DriverBitsLocked );

				if ( ! geBitmap_AllocSystemMip(Bmp,mip) )
					Ret = GE_FALSE;

				SystemBits = Bmp->Data[mip];

				geBitmap_MakeMipInfo(&(Bmp->Info),mip,&SystemInfo);

				if ( DriverBits && SystemBits )
				{
					// _Update_DriverToSystem
					// {} palette (not) made in AttachToDriver; must be made in here->
					if ( ! geBitmap_BlitData(	&(MipBmp->Info), DriverBits, MipBmp,
												&SystemInfo,	SystemBits, Bmp,
												SystemInfo.Width,SystemInfo.Height) )
						Ret = GE_FALSE;
				}
				else
				{
					Ret = GE_FALSE;
				}
			}
			
			geBitmap_UnLock(DriverLocks[mip]);
		}

		Bmp->DriverDataChanged = GE_FALSE;
	}
	else
	{
		Ret = GE_FALSE;
	}

	if ( ! Ret )
	{
		geErrorLog_AddString(-1,"Update_DriverToSystem : Locking and Blitting error", NULL);
	}

	if ( ! geBitmap_Gamma_Apply(Bmp,GE_FALSE) ) // redo the gamma!
		return GE_FALSE;

return Ret;
}

/*}{ ************* Mip Control *****************/

// Note : all the Mip control 

GENESISAPI geBoolean GENESISCC geBitmap_RefreshMips(geBitmap *Bmp)
{
int mip;

	assert( geBitmap_IsValid(Bmp) );

	if ( Bmp->LockOwner || Bmp->LockCount || Bmp->DataOwner )
		return GE_FALSE;

	for(mip = (Bmp->Info.MinimumMip + 1);mip <= Bmp->Info.MaximumMip;mip++)
	{
		if ( Bmp->Data[mip] && !(Bmp->Modified[mip]) )
		{
		int src;
			src = mip-1;
			while( ! Bmp->Data[src] )
			{
				src--;
				if ( src < Bmp->Info.MinimumMip )
					return GE_FALSE;
			}
			if ( ! geBitmap_UpdateMips(Bmp,src,mip) )
				return GE_FALSE;
		}
	}

#if 0	// never turn off a modified flag
	for(mip=0;mip<MAXMIPLEVELS;mip++)
		Bmp->Modified[mip] = GE_FALSE;
#endif

return GE_TRUE;
}

GENESISAPI geBoolean GENESISCC geBitmap_UpdateMips(geBitmap *Bmp,int fm,int to)
{
geBitmap * Locks[MAXMIPLEVELS];
void *FmBits,*ToBits;
geBitmap_Info FmInfo,ToInfo;
geBoolean Ret = GE_FALSE;

	assert( geBitmap_IsValid(Bmp) );

	if ( Bmp->LockOwner || Bmp->LockCount > 0 || Bmp->DataOwner )
		return GE_FALSE;

	if ( fm >= to )
		return GE_FALSE;

	if ( Bmp->DriverHandle ) 
	{
		//{} this version does *NOT* make new mips if to > Bmp->DriverInfo.MaximumMip

		if ( ! geBitmap_LockForWrite(Bmp,Locks,fm,to) )
			return GE_FALSE;

		if ( geBitmap_GetInfo(Locks[0],&FmInfo,NULL) && geBitmap_GetInfo(Locks[to - fm],&ToInfo,NULL) )
		{
			FmBits = geBitmap_GetBits(Locks[0]);
			ToBits = geBitmap_GetBits(Locks[to - fm]);
		
			if ( FmBits && ToBits )
			{
				Ret = geBitmap_UpdateMips_Data(	&FmInfo, FmBits, 
												&ToInfo, ToBits );
			}
		}

		geBitmap_UnLockArray_NoChange(Locks,to - fm + 1);
	}
	else
	{
		Ret = geBitmap_UpdateMips_System(Bmp,fm,to);
	}

return Ret;
}

geBoolean geBitmap_UpdateMips_System(geBitmap *Bmp,int fm,int to)
{
geBitmap_Info FmInfo,ToInfo;
geBoolean Ret;

	assert( geBitmap_IsValid(Bmp) );

	// this is called to create new mips in LockFor* -> CreateLockFrom* (through MakeSystemMips)

	if ( Bmp->LockOwner )
		Bmp = Bmp->LockOwner;
//	if ( Bmp->LockCount > 0 )
//		return GE_FALSE;
	if ( Bmp->DataOwner )
		return GE_FALSE;

	// {} for compressed data, just don't make mips and say we did!
	if ( gePixelFormat_BytesPerPel(Bmp->Info.Format) < 1 )
		return GE_TRUE;

	while(Bmp->Data[fm] == NULL || fm == to )
	{
		fm--;
		if ( fm < 0 )
			return GE_FALSE;
	}

	if ( fm < Bmp->Info.MinimumMip || fm > Bmp->Info.MaximumMip ||
	     to < fm || to >= MAXMIPLEVELS )
		return GE_FALSE;

	if ( ! Bmp->Data[to] )
	{
		if ( ! geBitmap_AllocSystemMip(Bmp,to) )
			return GE_FALSE;
	}

	assert( to > fm && fm >= 0 );

	FmInfo = ToInfo = Bmp->Info;

	FmInfo.Width = SHIFT_R_ROUNDUP(Bmp->Info.Width ,fm);
	FmInfo.Height= SHIFT_R_ROUNDUP(Bmp->Info.Height,fm);
	FmInfo.Stride= SHIFT_R_ROUNDUP(Bmp->Info.Stride,fm);
	ToInfo.Width = SHIFT_R_ROUNDUP(Bmp->Info.Width ,to);
	ToInfo.Height= SHIFT_R_ROUNDUP(Bmp->Info.Height,to);
	ToInfo.Stride= SHIFT_R_ROUNDUP(Bmp->Info.Stride,to);

	Ret = geBitmap_UpdateMips_Data(	&FmInfo, Bmp->Data[fm],
									&ToInfo, Bmp->Data[to]);

	Bmp->Info.MaximumMip = max(Bmp->Info.MaximumMip,to);

return Ret;
}

geBoolean geBitmap_UpdateMips_Data(	geBitmap_Info * FmInfo,void * FmBits,
									geBitmap_Info * ToInfo,void * ToBits)
{
int fmxtra,tow,toh,toxtra,fmw,fmh,fmstep,x,y,bpp;

	assert( FmInfo && ToInfo && FmBits && ToBits );
	assert( FmInfo->Format == ToInfo->Format && FmInfo->HasColorKey == ToInfo->HasColorKey );

	tow = ToInfo->Width;
	toh = ToInfo->Height;
	toxtra = ToInfo->Stride - ToInfo->Width;
	
	x = ToInfo->Width;
	fmstep = 1;
	while( x < FmInfo->Width )
	{
		fmstep += fmstep;
		x += x;
	}

	fmw = FmInfo->Width;
	fmh = FmInfo->Height;
	fmxtra = (FmInfo->Stride - tow) * fmstep; // amazingly simple and correct! think about it!

	// fmh == 15
	// toh == 8
	// fmstep == 2
	// 7*2 <= 14 -> Ok
	if ( (toh-1)*fmstep > (fmh - 1) )
	{
		geErrorLog_AddString(-1,"UpdateMips_Data : Vertical mip scaling doesn't match horizontal!", NULL);
		return GE_FALSE;
	}

	// {} todo : average for some special cases (16rgb,24rgb,32rgb)

	bpp = gePixelFormat_BytesPerPel(FmInfo->Format);

	if ( fmstep == 2 && bpp > 1 )
	{
	int R1,G1,B1,A1,R2,G2,B2,A2,R3,G3,B3,A3,R4,G4,B4,A4;
	gePixelFormat_ColorGetter GetColor;
	gePixelFormat_ColorPutter PutColor;
	const gePixelFormat_Operations *ops;
	uint8 *fmp,*fmp2,*top;

		fmp = FmBits;
		top = ToBits;

		ops = gePixelFormat_GetOperations(FmInfo->Format);
		GetColor = ops->GetColor;
		PutColor = ops->PutColor;

		fmxtra *= bpp;
		toxtra *= bpp;

		if ( FmInfo->HasColorKey )
		{
		uint32 ck,p1,p2,p3,p4;
		gePixelFormat_PixelGetter GetPixel;
		gePixelFormat_PixelPutter PutPixel;
		gePixelFormat_Decomposer DecomposePixel;
		int32 PixelComposeRTable[]={0x696C6345,0x21657370};

			assert( FmInfo->ColorKey == ToInfo->ColorKey );
			ck = FmInfo->ColorKey;
			GetPixel = ops->GetPixel;
			PutPixel = ops->PutPixel;
			DecomposePixel = ops->DecomposePixel;
		
			// {} the colorkey mip-subsampler
			// slow; yet another reason to not use CK !
			
			for(y=toh;y--;)
			{
				//y = 7, fmh = 15; y*2+1 == fmh : last line is not a double line
				if ( (y+y + 1) == fmh )	fmp2 = fmp;
				else					fmp2 = fmp + (FmInfo->Stride*bpp);
				for(x=tow;x--;)
				{
					p1 = GetPixel(&fmp);
					p2 = GetPixel(&fmp);
					p3 = GetPixel(&fmp2);
					p4 = GetPixel(&fmp2);
					if ( p1 == ck || p4 == ck )
					{
						PutPixel(&top,ck);
					}
					else
					{
						// p1 and p4 are not ck;
						if ( p2 == ck ) p2 = p1;
						if ( p3 == ck ) p3 = p4;
						DecomposePixel(p1,&R1,&G1,&B1,&A1);
						DecomposePixel(p2,&R2,&G2,&B2,&A2);
						DecomposePixel(p3,&R3,&G3,&B3,&A3);
						DecomposePixel(p4,&R4,&G4,&B4,&A4);
						PutColor(&top,(R1+R2+R3+R4+2)>>2,(G1+G2+G3+G4+2)>>2,(B1+B2+B3+B4+2)>>2,(A1+A2+A3+A4+2)>>2);
					}
				}
				fmp += fmxtra;
				top += toxtra;
			}
		}
		else
		{
			for(y=toh;y--;)
			{
				//y = 7, fmh = 15; y*2+1 == fmh : last line is not a double line
				if ( (y+y + 1) == fmh )	fmp2 = fmp;
				else					fmp2 = fmp + (FmInfo->Stride*bpp);
				for(x=tow;x--;)
				{
					GetColor(&fmp ,&R1,&G1,&B1,&A1);
					GetColor(&fmp ,&R2,&G2,&B2,&A2);
					GetColor(&fmp2,&R3,&G3,&B3,&A3);
					GetColor(&fmp2,&R4,&G4,&B4,&A4);
					PutColor(&top,(R1+R2+R3+R4+2)>>2,(G1+G2+G3+G4+2)>>2,(B1+B2+B3+B4+2)>>2,(A1+A2+A3+A4+2)>>2);
				}
				fmp += fmxtra;
				top += toxtra;
			}
		}

		assert( top == (((uint8 *)ToBits) + ToInfo->Stride * ToInfo->Height * bpp ) );
		assert( fmp == (((uint8 *)FmBits) + FmInfo->Stride * ToInfo->Height * 2 * bpp ) );
	}
	else if ( fmstep == 2 && gePixelFormat_HasPalette(FmInfo->Format) )
	{
	int R,G,B;
	uint8 *fmp,*fmp2,*top;
	uint8 paldata[768],*palptr;
	int p;
	palInfo * PalInfo;

		assert(bpp == 1);
		assert(FmInfo->Palette);

		if ( ! geBitmap_Palette_GetData(FmInfo->Palette,paldata,GE_PIXELFORMAT_24BIT_RGB,256) )
			return GE_FALSE;

		if ( ! (PalInfo = closestPalInit(paldata)) )
			return GE_FALSE;

		fmp = FmBits;
		top = ToBits;

		// @@ colorkey?

		for(y=toh;y--;)
		{
			//y = 7, fmh = 15; y*2+1 == fmh : last line is not a double line
			if ( (y*2 + 1) == fmh )	fmp2 = fmp;
			else					fmp2 = fmp + (FmInfo->Stride*bpp);

			for(x=tow;x--;)
			{
				p = *fmp++;
				palptr = paldata + p*3;
				R  = palptr[0]; G  = palptr[1]; B  = palptr[2]; 
				p = *fmp++;
				palptr = paldata + p*3;
				R += palptr[0]; G += palptr[1]; B += palptr[2]; 
				p = *fmp2++;
				palptr = paldata + p*3;
				R += palptr[0]; G += palptr[1]; B += palptr[2]; 
				p = *fmp2++;
				palptr = paldata + p*3;
				R += palptr[0]; G += palptr[1]; B += palptr[2]; 

				R = (R+2)>>2;
				G = (G+2)>>2;
				B = (B+2)>>2;

				p = closestPal(R,G,B,PalInfo);
				*top++ = p;
			}
			fmp += fmxtra;
			top += toxtra;
		}

		closestPalFree(PalInfo);

		assert( top == (((uint8 *)ToBits) + ToInfo->Stride * ToInfo->Height * bpp ) );
		assert( fmp == (((uint8 *)FmBits) + FmInfo->Stride * ToInfo->Height * 2 * bpp ) );
	}
	else
	{
		// we just sub-sample to make mips, so we don't have to
		//	know anything about pixelformat.
		// (btw this spoils the whole point of mips, so we might as well kill the mip!)

		//{} Blend correctly !?

		switch( bpp )
		{
			default:
			{
				return GE_FALSE;
			}
			case 1:
			{
				uint8 *fmp,*top;
				fmp = FmBits;
				top = ToBits;
				for(y=toh;y--;)
				{
					for(x=tow;x--;)
					{
						*top++ = *fmp;
						fmp += fmstep;
					}
					fmp += fmxtra;
					top += toxtra;
				}
				break;
			}
			case 2:
			{
				uint16 *fmp,*top;
				fmp = FmBits;
				top = ToBits;
				for(y=toh;y--;)
				{
					for(x=tow;x--;)
					{
						*top++ = *fmp;
						fmp += fmstep;
					}
					fmp += fmxtra;
					top += toxtra;
				}
				break;
			}
			case 4:
			{
				uint32 *fmp,*top;
				fmp = FmBits;
				top = ToBits;
				for(y=toh;y--;)
				{
					for(x=tow;x--;)
					{
						*top++ = *fmp;
						fmp += fmstep;
					}
					fmp += fmxtra;
					top += toxtra;
				}
				break;
			}
			case 3:
			{
				uint8 *fmp,*top;
				fmp = FmBits;
				top = ToBits;
				fmstep = (fmstep - 1) * 3;
				fmxtra *= 3;
				toxtra *= 3;
				for(y=toh;y--;)
				{
					for(x=tow;x--;)
					{
						*top++ = *fmp++;
						*top++ = *fmp++;
						*top++ = *fmp++;
						fmp += fmstep;
					}
					fmp += fmxtra;
					top += toxtra;
				}
				break;
			}
		}
	}

return GE_TRUE;
}

GENESISAPI geBoolean GENESISCC geBitmap_ClearMips(geBitmap *Bmp)
{
int mip;
DRV_Driver * Driver;

	// WARNING ! This destroys any mips!

	assert( geBitmap_IsValid(Bmp) );

	if ( Bmp->LockOwner || Bmp->LockCount || Bmp->DataOwner )
		return GE_FALSE;

	if ( Bmp->SeekMipCount == 0 && Bmp->Info.MaximumMip == 0 )
		return GE_TRUE;

	Driver = Bmp->Driver;
	if ( Driver )
	{
		if ( ! geBitmap_DetachDriver(Bmp,GE_TRUE) )
			return GE_FALSE;
	}
	assert(Bmp->Driver == NULL);

	mip = Bmp->Info.MinimumMip;
	if ( mip == 0 ) 
		mip++;

	Bmp->SeekMipCount = mip;

	for( ; mip <= Bmp->Info.MaximumMip ; mip++)
	{
		if ( Bmp->Data[mip] )
		{
			geRam_Free( Bmp->Data[mip] );
			Bmp->Data[mip] = NULL;
		}
	}

	Bmp->Info.MaximumMip = Bmp->Info.MinimumMip;

	if ( Driver )
	{
		if ( ! geBitmap_AttachToDriver(Bmp,Driver,0) )
			return GE_FALSE;
	}

return GE_TRUE;
}

GENESISAPI geBoolean 	GENESISCC	geBitmap_SetMipCount(geBitmap *Bmp,int Count)
{
DRV_Driver * Driver;

	assert( geBitmap_IsValid(Bmp) );

	if ( Bmp->LockOwner || Bmp->LockCount || Bmp->DataOwner )
		return GE_FALSE;

	if ( Bmp->SeekMipCount == Count )
	{
		Driver = NULL;
	}
	else
	{
		Driver = Bmp->Driver;
		if ( Driver )
		{
			if ( ! geBitmap_DetachDriver(Bmp,GE_TRUE) )
				return GE_FALSE;
		}
		assert(Bmp->Driver == NULL);
	}

	Bmp->SeekMipCount = Count;

// @@ don't do this ?
//	if ( Bmp->Info.MaximumMip < (Count-1) )
//		geBitmap_MakeSystemMips(Bmp,0,Count-1);
//

	if ( Driver )
	{
		if ( ! geBitmap_AttachToDriver(Bmp,Driver,0) )
			return GE_FALSE;
	}

return GE_TRUE;
}

geBoolean geBitmap_MakeSystemMips(geBitmap *Bmp,int low,int high)
{
int mip;

	assert( geBitmap_IsValid(Bmp) );

	// this is that CreateLockFromMip uses to make its new data

	if ( Bmp->LockOwner )
		Bmp = Bmp->LockOwner;
//	if ( Bmp->LockCount > 0 )
//		return GE_FALSE;
	if ( Bmp->DataOwner )
		return GE_FALSE;

	// {} for compressed data, just don't make mips and say we did!
	if ( gePixelFormat_BytesPerPel(Bmp->Info.Format) < 1 )
		return GE_TRUE;

	if ( low < 0 || high >= MAXMIPLEVELS || low > high )
		return GE_FALSE;

	for( mip = low; mip <= high; mip++)
	{
		if ( ! Bmp->Data[mip] )
		{
			if ( ! geBitmap_AllocSystemMip(Bmp,mip) )
				return GE_FALSE;
	
			if ( mip != 0 )
			{
				if ( ! geBitmap_UpdateMips_System(Bmp,mip-1,mip) )
					return GE_FALSE;
			}
		}
	}

	Bmp->Info.MinimumMip = min(Bmp->Info.MinimumMip,low);
	Bmp->Info.MaximumMip = max(Bmp->Info.MaximumMip,high);

return GE_TRUE;
}

/*}{ ******* Miscellany ***********/

GENESISAPI uint32 GENESISCC geBitmap_MipBytes(const geBitmap *Bmp,int mip)
{
uint32 bytes;
	if ( ! Bmp )
		return 0;
	bytes = gePixelFormat_BytesPerPel(Bmp->Info.Format) * 
						SHIFT_R_ROUNDUP(Bmp->Info.Stride,mip) *
						SHIFT_R_ROUNDUP(Bmp->Info.Height,mip);
return bytes;
}

GENESISAPI geBoolean GENESISCC geBitmap_GetInfo(const geBitmap *Bmp, geBitmap_Info *Info, geBitmap_Info *SecondaryInfo)
{
	assert( geBitmap_IsValid(Bmp) );

	assert(Info);

	if ( Bmp->DriverHandle )
	{
		*Info = Bmp->DriverInfo;
	}
	else
	{
		*Info = Bmp->Info;
	}

	if ( SecondaryInfo )
		*SecondaryInfo = Bmp->Info;

	return GE_TRUE;
}

geBoolean geBitmap_MakeDriverLockInfo(geBitmap *Bmp,int mip,geBitmap_Info *Into)
{
geRDriver_THandleInfo TInfo;

	// MakeDriverLockInfo also doesn't full out the full info, so it must be a valid info first!
	// Bmp also gets some stuff written into him.

	assert(Bmp && Into); // not necessarily valid

	if ( ! Bmp->DriverHandle || ! Bmp->Driver || mip < Bmp->DriverInfo.MinimumMip || mip > Bmp->DriverInfo.MaximumMip )
		return GE_FALSE;

	if ( ! Bmp->Driver->THandle_GetInfo(Bmp->DriverHandle,mip,&TInfo) )
	{
		geErrorLog_AddString(-1,"MakeDriverLockInfo : THandle_GetInfo", NULL);
		return GE_FALSE;
	}

	Bmp->DriverMipLock	= mip;
	Bmp->DriverFlags	= TInfo.PixelFormat.Flags;

	Into->Width			= TInfo.Width;
	Into->Height		= TInfo.Height;
	Into->Stride		= TInfo.Stride;
	Into->Format		= TInfo.PixelFormat.PixelFormat;
	Into->ColorKey		= TInfo.ColorKey;

	if ( TInfo.Flags & RDRIVER_THANDLE_HAS_COLORKEY )
		Into->HasColorKey = GE_TRUE;
	else
		Into->HasColorKey = GE_FALSE;

	Into->MinimumMip = Into->MaximumMip = mip;

	if ( gePixelFormat_HasPalette(Into->Format) && Into->Palette && Into->Palette->HasColorKey )
	{
		Into->HasColorKey = GE_TRUE;
		Into->ColorKey = Into->Palette->ColorKeyIndex;
	}

return GE_TRUE;
}

GENESISAPI int GENESISCC	geBitmap_Width(const geBitmap *Bmp)
{
	assert(Bmp);
return(Bmp->Info.Width);
}

GENESISAPI int GENESISCC	geBitmap_Height(const geBitmap *Bmp)
{
	assert(Bmp);
return(Bmp->Info.Height);
}

GENESISAPI geBoolean GENESISCC geBitmap_Blit(const geBitmap *Src, int SrcPositionX, int SrcPositionY,
						geBitmap *Dst, int DstPositionX, int DstPositionY,
						int SizeX, int SizeY )
{
	assert( geBitmap_IsValid(Src) );
	assert( geBitmap_IsValid(Dst) );
	return geBitmap_BlitMipRect(Src,0,SrcPositionX,SrcPositionY,
								Dst,0,DstPositionX,DstPositionY,
								SizeX,SizeY);
}

GENESISAPI geBoolean GENESISCC geBitmap_BlitBitmap(const geBitmap * Src, geBitmap * Dst )
{
	assert( geBitmap_IsValid(Src) );
	assert( geBitmap_IsValid(Dst) );
	assert( Src != Dst );
	return geBitmap_BlitMipRect(Src,0,0,0,Dst,0,0,0,-1,-1);
}

GENESISAPI geBoolean GENESISCC geBitmap_BlitBestMip(const geBitmap * Src, geBitmap * Dst )
{
int Width,Mip;
	assert( geBitmap_IsValid(Src) );
	assert( geBitmap_IsValid(Dst) );
	assert( Src != Dst );
	for(Mip=0;	(Width = SHIFT_R_ROUNDUP(Src->Info.Width,Mip)) > Dst->Info.Width ; Mip++) ;
	return geBitmap_BlitMipRect(Src,Mip,0,0,Dst,0,0,0,-1,-1);
}

GENESISAPI geBoolean GENESISCC geBitmap_BlitMip(const geBitmap * Src, int SrcMip, geBitmap * Dst, int DstMip )
{
	assert( geBitmap_IsValid(Src) );
	assert( geBitmap_IsValid(Dst) );
	return geBitmap_BlitMipRect(Src,SrcMip,0,0,Dst,DstMip,0,0,-1,-1);
}

geBoolean geBitmap_BlitMipRect(const geBitmap * Src, int SrcMip, int SrcX,int SrcY,
									 geBitmap * Dst, int DstMip, int DstX,int DstY,
							int SizeX,int SizeY)
{
geBitmap * SrcLock,* DstLock;
geBoolean SrcUnLock,DstUnLock;
geBitmap_Info *SrcLockInfo,*DstLockInfo;
uint8 *SrcBits,*DstBits;
	
	assert(Src && Dst);
	assert( Src != Dst );
	// <> if Src == Dst we could still do this, but we assert SrcMip != DstMip & be smart

	SrcUnLock = DstUnLock = GE_FALSE;

	if ( Src->LockOwner )
	{
		assert( Src->LockOwner->LockCount );
		if ( SrcMip != 0 )
		{
			geErrorLog_AddString(-1,"BlitMipRect : Src is a lock and mip != 0", NULL);
			goto fail;
		}

		SrcLock = (geBitmap *)Src;
	}
	else
	{
		if ( ! geBitmap_LockForReadNative((geBitmap *)Src,&SrcLock,SrcMip,SrcMip) )
		{
			geErrorLog_AddString(-1,"BlitMipRect : LockForReadNative", NULL);
			goto fail;
		}
		SrcUnLock = GE_TRUE;
	}

	if ( Dst->LockOwner )
	{
		if ( DstMip != 0 )
			goto fail;
//		if ( Dst->LockOwner->LockCount >= 0 )
//			goto fail;
//		{} can't check this, cuz we use _BlitMip to create locks for read
		DstLock = Dst;
	}
	else
	{
		if ( ! geBitmap_LockForWrite(Dst,&DstLock,DstMip,DstMip) )
		{
			geErrorLog_AddString(-1,"BlitMipRect : LockForWrite", NULL);
			goto fail;
		}
		DstUnLock = GE_TRUE;
	}

	Src = Dst = NULL;

	if ( SrcLock->DriverHandle ) 
		SrcLockInfo = &(SrcLock->DriverInfo);
	else
		SrcLockInfo = &(SrcLock->Info);

	if ( DstLock->DriverHandle ) 
		DstLockInfo = &(DstLock->DriverInfo);
	else
		DstLockInfo = &(DstLock->Info);

	if ( ! (SrcBits = geBitmap_GetBits(SrcLock)) || 
		 ! (DstBits = geBitmap_GetBits(DstLock)) )
	{
		geErrorLog_AddString(-1,"BlitMipRect : GetBits", NULL);
		goto fail;
	}

	if ( SizeX < 0 )
		SizeX = min(SrcLockInfo->Width,DstLockInfo->Width);
	if ( SizeY < 0 )
		SizeY = min(SrcLockInfo->Height,DstLockInfo->Height);

	if (( (SrcX + SizeX) > SrcLockInfo->Width ) ||
		( (SrcY + SizeY) > SrcLockInfo->Height) ||
		( (DstX + SizeX) > DstLockInfo->Width ) ||
		( (DstY + SizeY) > DstLockInfo->Height))
	{
		geErrorLog_AddString(-1,"BlitMipRect : dimensions bad", NULL);
		goto fail;
	}

	SrcBits += gePixelFormat_BytesPerPel(SrcLockInfo->Format) * ( SrcY * SrcLockInfo->Stride + SrcX );
	DstBits += gePixelFormat_BytesPerPel(DstLockInfo->Format) * ( DstY * DstLockInfo->Stride + DstX );

	// _BlitMipRect : made palette
	if ( ! geBitmap_BlitData(	SrcLockInfo,SrcBits,SrcLock,
								DstLockInfo,DstBits,DstLock,
								SizeX,SizeY) )
	{
		goto fail;
	}

	if ( SrcUnLock ) geBitmap_UnLock(SrcLock);
	if ( DstUnLock ) geBitmap_UnLock(DstLock);

	return GE_TRUE;

	fail:

	if ( SrcUnLock ) geBitmap_UnLock(SrcLock);
	if ( DstUnLock ) geBitmap_UnLock(DstLock);

	return GE_FALSE;
}

GENESISAPI geBoolean 	GENESISCC	geBitmap_SetFormatMin(geBitmap *Bmp,gePixelFormat NewFormat)
{
geBitmap_Palette * Pal;

	assert(geBitmap_IsValid(Bmp));

	Pal = geBitmap_GetPalette(Bmp);
	if ( Bmp->Info.HasColorKey )
	{
	uint32 CK;
		if ( gePixelFormat_IsRaw(NewFormat) )
		{
			if ( gePixelFormat_IsRaw(Bmp->Info.Format) )
			{
				CK = gePixelFormat_ConvertPixel(Bmp->Info.Format,Bmp->Info.ColorKey,NewFormat);
			}
			else if ( gePixelFormat_HasPalette(Bmp->Info.Format) )
			{
				assert(Pal);
				geBitmap_Palette_GetEntry(Pal,Bmp->Info.ColorKey,&CK);
				CK = gePixelFormat_ConvertPixel(Pal->Format,CK,NewFormat);
				if ( ! CK ) CK = 1;
			}
		}
		else
		{
			if ( gePixelFormat_HasPalette(NewFormat) )
			{
				CK = 255;
			}
			else
			{
				CK = 1;
			}
		}
		
		return geBitmap_SetFormat(Bmp,NewFormat,GE_TRUE,CK,Pal);
	}
	else
	{
		return geBitmap_SetFormat(Bmp,NewFormat,GE_FALSE,0,Pal);
	}
}

GENESISAPI geBoolean GENESISCC geBitmap_SetFormat(geBitmap *Bmp, 
							gePixelFormat NewFormat, 
							geBoolean HasColorKey, uint32 ColorKey,
							const geBitmap_Palette *Palette )
{
	assert( geBitmap_IsValid(Bmp) );

	if ( Bmp->LockOwner || Bmp->LockCount || Bmp->DataOwner )
	{
		geErrorLog_AddString(-1,"SetFormat : not an original bitmap", NULL);
		return GE_FALSE;	
	}
	// can't do _SetFormat on a locked mip, cuz it would change the size of all the locked mips = no good

	// always affects the non-Driver copy

	if ( NewFormat == GE_PIXELFORMAT_WAVELET )
	{
		geErrorLog_AddString(-1,"Genesis3D 1.0 does not support Wavelet Images",NULL);
		return GE_FALSE;
	}

	if ( NewFormat == Bmp->Info.Format )
	{
		// but not wavelet

		if ( gePixelFormat_HasPalette(NewFormat) && Palette )
		{
			if ( ! geBitmap_SetPalette(Bmp,(geBitmap_Palette *)Palette) )
				return GE_FALSE;
		}

		if ( (! HasColorKey )
			|| ( HasColorKey && Bmp->Info.HasColorKey && ColorKey == Bmp->Info.ColorKey ) )
		{
			Bmp->Info.HasColorKey = HasColorKey;
			Bmp->Info.ColorKey = ColorKey;
			return GE_TRUE;
		}
		else
		{
		geBitmap_Info OldInfo;

			OldInfo = Bmp->Info;

			assert(HasColorKey);

			// just change the colorkey

			Bmp->Info.HasColorKey = HasColorKey;
			Bmp->Info.ColorKey = ColorKey;

			if ( Bmp->Data[Bmp->Info.MinimumMip] == NULL )
				return GE_TRUE;
		
			assert(Bmp->Info.MinimumMip == 0); //{} this is just out of laziness

			// _SetFormat : same format
			if ( ! geBitmap_BlitData(	&OldInfo,		Bmp->Data[Bmp->Info.MinimumMip], NULL,
										&(Bmp->Info),	Bmp->Data[Bmp->Info.MinimumMip], NULL,
										Bmp->Info.Width, Bmp->Info.Height) )
			{
				return GE_FALSE;
			}

			return GE_TRUE;
		}
	}
	else
	{
	geBitmap_Info OldInfo;
	int OldBPP,NewBPP;
	int OldMaxMips;
	DRV_Driver * Driver;

		if ( gePixelFormat_HasPalette(NewFormat) )
		{
			if ( Palette )
			{
				if ( ! geBitmap_SetPalette(Bmp,(geBitmap_Palette *)Palette) )
					return GE_FALSE;
			}
			else
			{
				if ( ! geBitmap_GetPalette(Bmp) && ! gePixelFormat_HasPalette(Bmp->Info.Format) )
				{
				geBitmap_Palette *NewPal;
					NewPal = geBitmap_Palette_CreateFromBitmap(Bmp,GE_FALSE);
					if ( ! NewPal )
					{
						geErrorLog_AddString(-1,"_SetFormat : createPaletteFromBitmap failed", NULL);
						return GE_FALSE;
					}
					if ( ! geBitmap_SetPalette(Bmp,NewPal) )
						return GE_FALSE;
					geBitmap_Palette_Destroy(&NewPal);
				}
			}
		}

		Driver = Bmp->Driver;
		if ( Driver )
			if ( ! geBitmap_DetachDriver(Bmp,GE_TRUE) )
				return GE_FALSE;

		OldBPP = gePixelFormat_BytesPerPel(Bmp->Info.Format);
		NewBPP = gePixelFormat_BytesPerPel(NewFormat);

		OldInfo = Bmp->Info;
		Bmp->Info.Format = NewFormat;
		Bmp->Info.HasColorKey = HasColorKey;
		Bmp->Info.ColorKey = ColorKey;

		// {} this is not very polite; we do restore them later, though...
		OldMaxMips = max(Bmp->Info.MaximumMip,Bmp->DriverInfo.MaximumMip);
		geBitmap_ClearMips(Bmp);		

		if ( Bmp->Data[Bmp->Info.MinimumMip] == NULL && 
				Bmp->DriverHandle == NULL )
			return GE_TRUE;

		if ( OldBPP == NewBPP )
		{
		geBitmap * Lock;
		void * Bits;
			// can work in place
			if ( ! geBitmap_LockForWrite(Bmp,&Lock,0,0) )
				return GE_FALSE;

			if ( ! (Bits = geBitmap_GetBits(Lock)) )
			{
				geBitmap_UnLock(Lock);
				return GE_FALSE;
			}

			// _SetFormat : new format
			if ( ! geBitmap_BlitData(	&OldInfo,		Bits, Lock,
										&(Lock->Info),	Bits, Lock,
										Lock->Info.Width, Lock->Info.Height) )
			{
				geBitmap_UnLock(Lock);
				return GE_FALSE;
			}

			geBitmap_UnLock(Lock);
		}
		else // NewFormat is raw && != OldFormat
		{
		geBitmap OldBmp;
		geBitmap *Lock,*SrcLock;
		void *Bits,*OldBits;

			OldBmp = *Bmp;
			OldBmp.Info = OldInfo;

			// clear out the Bmp for putting the new format in
			Bmp->Info.Stride = Bmp->Info.Width;
			Bmp->Data[0] = NULL;
			Bmp->Alpha = NULL;

			if ( ! geBitmap_AllocSystemMip(Bmp,0) )
				return GE_FALSE;

			if ( ! geBitmap_LockForReadNative(&OldBmp,&SrcLock,0,0) )
				return GE_FALSE;

			if ( ! geBitmap_LockForWrite(Bmp,&Lock,0,0) )
				return GE_FALSE;

			if ( ! (Bits = geBitmap_GetBits(Lock)) )
			{
				geBitmap_UnLock(Lock);
				return GE_FALSE;
			}
			if ( ! (OldBits = geBitmap_GetBits(SrcLock)) )
			{
				geBitmap_UnLock(Lock);
				return GE_FALSE;
			}

			// _SetFormat : new format
			if ( ! geBitmap_BlitData(	&OldInfo,		OldBits,		SrcLock,
										&(Lock->Info),	Bits,			Lock,
										Lock->Info.Width, Lock->Info.Height) )
			{
				// try to undo as well as possible
				return GE_FALSE;
			}
		
			geBitmap_UnLock(Lock);
			geBitmap_UnLock(SrcLock);

			if ( OldBmp.Data[0] )
			{
				geRam_Free(OldBmp.Data[0]);
				OldBmp.Data[0] = NULL;
			}

			if ( gePixelFormat_HasGoodAlpha(NewFormat) )
			{
				geBitmap_Destroy(&(OldBmp.Alpha));
			}
			else
			{
				Bmp->Alpha = OldBmp.Alpha;
			}
		}

		{
		int mip;
			mip = Bmp->Info.MinimumMip;
			while( mip < OldMaxMips )
			{
				geBitmap_UpdateMips(Bmp,mip,mip+1);
				mip++;
			}
		}

		if ( Driver )
		{		
			if ( ! geBitmap_AttachToDriver(Bmp,Driver,0) )
				return GE_FALSE;
		}
	}

return GE_TRUE;
}

GENESISAPI geBoolean GENESISCC geBitmap_SetColorKey(geBitmap *Bmp, geBoolean HasColorKey, uint32 ColorKey , geBoolean Smart)
{
	assert( geBitmap_IsValid(Bmp) );

	if ( Bmp->LockOwner || Bmp->LockCount || Bmp->DataOwner )
	{
		geErrorLog_AddString(-1,"SetColorKey : not an original bitmap", NULL);
		return GE_FALSE;	
	}

	// see comments in SetFormat

	if ( Bmp->DriverHandle )
		geBitmap_Update_DriverToSystem(Bmp);

	if ( HasColorKey && 
			((uint32)ColorKey>>1) >= ((uint32)1<<(gePixelFormat_BytesPerPel(Bmp->Info.Format)*8 - 1)) )
	{
		geErrorLog_AddString(-1,"geBitmap_SetColorKey : invalid ColorKey pixel!", NULL);
		return GE_FALSE;
	}
	if ( HasColorKey && gePixelFormat_HasAlpha(Bmp->Info.Format) )
	{
		geErrorLog_AddString(-1,"geBitmap_SetColorKey : non-fatal : Alpha and ColorKey together won't work right", NULL);
	}

	if ( HasColorKey && Smart && Bmp->Data[0] )
	{
		Bmp->Info.HasColorKey = GE_TRUE;
		Bmp->Info.ColorKey = ColorKey;
		if ( ! geBitmap_UsesColorKey(Bmp) )
		{
			Bmp->Info.HasColorKey = GE_FALSE;
			Bmp->Info.ColorKey = 1;
		}
	}
	else
	{
		Bmp->Info.HasColorKey = HasColorKey;
		Bmp->Info.ColorKey = ColorKey;
	}

	if ( Bmp->DriverHandle )
		geBitmap_Update_SystemToDriver(Bmp);

return GE_TRUE;
}

geBoolean geBitmap_UsesColorKey(const geBitmap * Bmp)
{
void * Bits;
const gePixelFormat_Operations * ops;
int x,y,w,h,s;
uint32 pel,ColorKey;

	if ( ! Bmp->Info.HasColorKey )
		return GE_FALSE;

	if ( ! Bmp->Data[0] )
	{
		geErrorLog_AddString(-1,"UsesColorKey : no data!", NULL);
		return GE_TRUE;
	}

	assert( Bmp->Info.MinimumMip == 0 );

	Bits = Bmp->Data[0];
	ops = gePixelFormat_GetOperations(Bmp->Info.Format);
	assert(ops);

	w = Bmp->Info.Width;
	h = Bmp->Info.Height;
	s = Bmp->Info.Stride;

	ColorKey = Bmp->Info.ColorKey;

	switch(ops->BytesPerPel)
	{
		case 0:
			geErrorLog_AddString(-1,"UsesColorKey : invalid format", NULL);
			return GE_TRUE;
		case 3:
			#pragma message("Bitmap : UsesColorKey : no 24bit Smart ColorKey")
			geErrorLog_AddString(-1,"UsesColorKey : no 24bit Smart ColorKey", NULL);
			return GE_TRUE;	
		case 1:
		{
		uint8 * ptr;
			ptr = Bits;
			for(y=h;y--;)
			{
				for(x=w;x--;)
				{
					pel = *ptr++;
					if ( pel == ColorKey )
					{
						return GE_TRUE;	
					}
				}
				ptr += (s-w);
			}
			break;
		}
		case 2:
		{
		uint16 * ptr;
			ptr = Bits;
			for(y=h;y--;)
			{
				for(x=w;x--;)
				{
					pel = *ptr++;
					if ( pel == ColorKey )
					{
						return GE_TRUE;	
					}
				}
				ptr += (s-w);
			}
			break;
		}
		case 4:
		{
		uint32 * ptr;
			ptr = Bits;
			for(y=h;y--;)
			{
				for(x=w;x--;)
				{
					pel = *ptr++;
					if ( pel == ColorKey )
					{
						return GE_TRUE;	
					}
				}
				ptr += (s-w);
			}
			break;
		}
	}
return GE_FALSE;
}


GENESISAPI geBoolean GENESISCC geBitmap_SetPalette(geBitmap *Bmp, const geBitmap_Palette *Palette)
{
	assert(Bmp); // not nec. valid
	assert( geBitmap_Palette_IsValid(Palette) );

	if ( Bmp->LockOwner )
		Bmp = Bmp->LockOwner;

/* //{} breaks PalCreate
	if ( Bmp->LockCount > 0 || Bmp->DataOwner )
	{
		geErrorLog_AddString(-1,"SetPalette : not an original bitmap", NULL);
		return GE_FALSE;
	}
*/

	// warning : Bitmap_Blitdata calls us when it auto-creates a palette!

	// note that when we _SetPalette on a bitmap, all its write-locked children
	//	also get new palettes

	if ( Bmp->Info.Palette != Palette )
	{
		// save the palette even if we're not palettized, for later use
		if ( Palette->Driver )
		{
			if ( ! geBitmap_AllocPalette(Bmp,Palette->Format,NULL) )
				return GE_FALSE;
			
			if ( ! geBitmap_Palette_Copy(Palette,Bmp->Info.Palette) )
				return GE_FALSE;
		}
		else
		{
			if ( Bmp->Info.Palette )
				geBitmap_Palette_Destroy(&(Bmp->Info.Palette));

			Bmp->Info.Palette = (geBitmap_Palette *)Palette;
			geBitmap_Palette_CreateRef(Bmp->Info.Palette);
		}
	}

	if ( gePixelFormat_HasPalette(Bmp->DriverInfo.Format) &&
		Bmp->DriverInfo.Palette != Palette )
	{
		if ( Palette->Driver == Bmp->Driver && 
			( ! Palette->HasColorKey || ! Bmp->DriverInfo.ColorKey ||
				(uint32)Palette->ColorKeyIndex == Bmp->DriverInfo.ColorKey ) )
		{
			if ( Bmp->DriverInfo.Palette )
				geBitmap_Palette_Destroy(&(Bmp->DriverInfo.Palette));
			Bmp->DriverInfo.Palette = (geBitmap_Palette *)Palette;
			geBitmap_Palette_CreateRef(Bmp->DriverInfo.Palette);
		}
		else if ( Bmp->DriverInfo.Palette )
		{
			if ( ! geBitmap_Palette_Copy(Palette,Bmp->DriverInfo.Palette) )
				return GE_FALSE;
		}
		else
		{
			if ( ! geBitmap_AllocPalette(Bmp,0,Bmp->Driver) )
				return GE_FALSE;

			if ( ! geBitmap_Palette_Copy(Palette,Bmp->DriverInfo.Palette) )
				return GE_FALSE;
		}
	}

	if ( Bmp->DriverHandle )
	{
		// if one has pal and other doesn't this is real change!
		if (	gePixelFormat_HasPalette(Bmp->Info.Format) &&
			  ! gePixelFormat_HasPalette(Bmp->DriverInfo.Format) )
		{
			// this over-rides any driver changes!
			Bmp->DriverDataChanged = GE_FALSE;
			if ( ! geBitmap_Update_SystemToDriver(Bmp) )
				return GE_FALSE;
		}
		else if ( ! gePixelFormat_HasPalette(Bmp->Info.Format) &&
				gePixelFormat_HasPalette(Bmp->DriverInfo.Format) )
		{
			Bmp->DriverDataChanged = GE_TRUE;
		}
	}

	assert( geBitmap_IsValid(Bmp) );

return GE_TRUE;
}

GENESISAPI geBitmap_Palette * GENESISCC geBitmap_GetPalette(const geBitmap *Bmp)
{
	if ( ! Bmp ) return NULL;

	if ( Bmp->Driver && Bmp->DriverInfo.Palette )
	{
		assert(Bmp->Info.Palette);
		return Bmp->DriverInfo.Palette;
	}

	return Bmp->Info.Palette;
}


GENESISAPI geBitmap * GENESISCC geBitmap_GetAlpha(const geBitmap *Bmp)
{
	if ( ! Bmp ) return NULL;
	return Bmp->Alpha;
}

GENESISAPI geBoolean GENESISCC geBitmap_SetAlpha(geBitmap *Bmp, const geBitmap *AlphaBmp)
{
	assert( geBitmap_IsValid(Bmp) );
	
	if ( Bmp->LockOwner )
		Bmp = Bmp->LockOwner;
	if ( Bmp->LockCount > 0 || Bmp->DataOwner )
	{
		geErrorLog_AddString(-1,"SetAlpha : not an original bitmap", NULL);
		return GE_FALSE;
	}

	if ( AlphaBmp == Bmp->Alpha )
		return GE_TRUE;

	if ( Bmp->DriverHandle )
	{
		geBitmap_Update_DriverToSystem(Bmp);
	}

	if ( Bmp->Alpha )
	{
		geBitmap_Destroy(&(Bmp->Alpha));
	}

	Bmp->Alpha = (geBitmap *)AlphaBmp;
	if ( AlphaBmp )
	{
		assert( geBitmap_IsValid(AlphaBmp) );
		geBitmap_CreateRef(Bmp->Alpha);
	}

	if ( Bmp->DriverHandle )
	{
		// upload the new alpha to the driver bitmap
		geBitmap_Update_SystemToDriver(Bmp);
	}

return GE_TRUE;
}

GENESISAPI geBoolean GENESISCC geBitmap_SetPreferredFormat(geBitmap *Bmp,gePixelFormat Format)
{

	if ( Bmp->LockOwner )
		Bmp = Bmp->LockOwner;
	if ( Bmp->LockCount > 0 || Bmp->DataOwner )
	{
		geErrorLog_AddString(-1,"SetPrefferedFormat : not an original bitmap", NULL);
		return GE_FALSE;
	}

	if ( Bmp->PreferredFormat != Format )
	{
	DRV_Driver * Driver;
		Bmp->PreferredFormat = Format;
		Driver = Bmp->Driver;
		if ( Driver )
		{
			if ( ! geBitmap_DetachDriver(Bmp,GE_TRUE) )
				return GE_FALSE;
			if ( ! geBitmap_AttachToDriver(Bmp,Driver,0) )
				return GE_FALSE;
		}
	}

return GE_TRUE;
}

GENESISAPI gePixelFormat GENESISCC geBitmap_GetPreferredFormat(const geBitmap *Bmp)
{
	if ( ! Bmp ) return 0;
return Bmp->PreferredFormat;
}

/*}{ ************** FILE I/O ************************/


GENESISAPI geBitmap * GENESISCC geBitmap_CreateFromFileName(const geVFile *BaseFS,const char *Name)
{
geVFile * File;
geBitmap * Bitmap;

	if ( BaseFS )
	{
		File = geVFile_Open((geVFile *)BaseFS, Name, GE_VFILE_OPEN_READONLY);
	}
	else
	{
		File = geVFile_OpenNewSystem(NULL,GE_VFILE_TYPE_DOS,Name,NULL,GE_VFILE_OPEN_READONLY);
	}
	if ( ! File )
		return NULL;
	Bitmap = geBitmap_CreateFromFile(File);
	geVFile_Close(File);

return Bitmap;
}

GENESISAPI geBoolean GENESISCC geBitmap_WriteToFileName(const geBitmap * Bmp,const geVFile *BaseFS,const char *Name)
{
geVFile * File;
geBoolean Ret;

	if ( BaseFS )
	{
		File = geVFile_Open((geVFile *)BaseFS, Name, GE_VFILE_OPEN_CREATE);
	}
	else
	{
		File = geVFile_OpenNewSystem(NULL,GE_VFILE_TYPE_DOS,Name,NULL,GE_VFILE_OPEN_CREATE);
	}

	if ( ! File )
		return GE_FALSE;

	Ret = geBitmap_WriteToFile(Bmp,File);

	geVFile_Close(File);

return Ret;
}

// GeBm Tag in 4 bytes {}
typedef uint32			geBmTag_t;
#define GEBM_TAG		((geBmTag_t)0x6D426547)	// "GeBm"

// version in a byte
#define GEBM_VERSION			(((uint32)GEBM_VERSION_MAJOR<<4) + (uint32)GEBM_VERSION_MINOR)
#define VERSION_MAJOR(Version)	(((Version)>>4)&0x0F)
#define VERSION_MINOR(Version)	((Version)&0x0F)

#define MIP_MASK				(0xF)
#define MIP_FLAG_COMPRESSED		(1<<4)
#define MIP_FLAG_PAETH_FILTERED	(1<<5)

static geBoolean geBitmap_ReadFromBMP(geBitmap * Bmp,geVFile * F);

GENESISAPI geBitmap * GENESISCC geBitmap_CreateFromFile(geVFile *F)
{
geBitmap *	Bmp;
geBmTag_t Tag;

	assert(F);

	if ( ! geVFile_Read(F, &Tag, sizeof(Tag)) )
		return NULL;

	Bmp = geBitmap_Create_Base();
	if ( ! Bmp )
		return NULL;

	if ( Tag == GEBM_TAG )
	{
	uint8 flags;
	uint8 Version;
	int mip;

		// see WriteToFile for comments on the file format

		if ( ! geVFile_Read(F, &Version, sizeof(Version)) )
			goto fail;

		if ( VERSION_MAJOR(Version) != VERSION_MAJOR(GEBM_VERSION) )
		{
			geErrorLog_AddString(-1,"CreateFromFile : incompatible GeBm version", NULL);	
			goto fail;
		}

		if ( ! geBitmap_ReadInfo(Bmp,F) )
			goto fail;

		if ( Bmp->Info.Palette )
		{
			Bmp->Info.Palette = NULL;
			if ( ! ( Bmp->Info.Palette = geBitmap_Palette_CreateFromFile(F)) )
				goto fail;
		}

		if ( Bmp->Info.Format == GE_PIXELFORMAT_WAVELET )
		{
			geErrorLog_AddString(-1,"Genesis3D 1.0 does not support Wavelet Images",NULL);
		}
		else
		{
			for(;;)
			{
				if ( ! geVFile_Read(F, &flags, sizeof(flags)) )
					goto fail;

				mip = flags & MIP_MASK;

				if ( mip > Bmp->Info.MaximumMip )
					break;

				assert(mip >= Bmp->Info.MinimumMip );
				assert( Bmp->Info.Stride == Bmp->Info.Width );

				if ( ! geBitmap_AllocSystemMip(Bmp,mip) )
					goto fail;

				if ( flags & MIP_FLAG_COMPRESSED )
				{
			#ifdef DO_LZ
				geVFile * LzF;

					LzF = geVFile_OpenNewSystem(F,GE_VFILE_TYPE_LZ,NULL,NULL,GE_VFILE_OPEN_READONLY);
					if ( ! LzF )
					{
						geErrorLog_AddString(-1,"Bitmap_CreateFromFile : LZ File Open failed",NULL);
						return GE_FALSE;
					}

					if ( ! geVFile_Read(LzF, Bmp->Data[mip], geBitmap_MipBytes(Bmp,mip) ) )
					{
						geVFile_Close(LzF);
						geErrorLog_AddString(-1,"Bitmap_CreateFromFile : LZ File Read failed",NULL);
						return GE_FALSE;
					}

					if ( ! geVFile_Close(LzF) )
					{
						geErrorLog_AddString(-1,"Bitmap_CreateFromFile : LZ File Close failed",NULL);
						return GE_FALSE;
					}
			#endif
				}
				else
				{
					if ( ! geVFile_Read(F, Bmp->Data[mip], geBitmap_MipBytes(Bmp,mip) ) )
						goto fail;
				}

				if ( flags & MIP_FLAG_PAETH_FILTERED )
				{
					geErrorLog_AddString(-1,"Bitmap_CreateFromFile : Paeth Filter not supported in this version!",NULL);
					return GE_FALSE;
				}

				Bmp->Modified[mip] = GE_TRUE;
			}
		}

		if( Bmp->Alpha )
		{
			if ( ! (Bmp->Alpha = geBitmap_CreateFromFile(F)) )
				goto fail;
		}
	}	// end geBitmap reader
	else 
	{
		if ( ! geVFile_Seek(F, - (int)sizeof(Tag), GE_VFILE_SEEKCUR) )
			goto fail;

		if ( (Tag&0xFFFF) == 0x4D42 )	// 'BM'
		{
		
			if ( ! geBitmap_ReadFromBMP(Bmp,F) )
				goto fail;
		}
		else
		{
			// geErrorLog_AddString(-1,"CreateFromFile : unknown format", NULL);
			goto fail;
		}
	}

	return Bmp;

fail:
	assert(Bmp);

	geBitmap_Destroy(&Bmp);
	return NULL;
}

GENESISAPI geBoolean GENESISCC geBitmap_WriteToFile(const geBitmap *Bmp, geVFile *F)
{
geBmTag_t geBM_Tag;
uint8  geBM_Version;
uint8 flags;
int mip;
	
	assert(Bmp && F);
	assert( geBitmap_IsValid(Bmp) );

	geBM_Tag = GEBM_TAG;
	geBM_Version = GEBM_VERSION;

	if ( Bmp->DriverHandle )
	{
		if ( ! geBitmap_Update_DriverToSystem((geBitmap *)Bmp) )
		{
			geErrorLog_AddString(-1,"WriteToFile : Update_DriverToSystem", NULL);	
			return GE_FALSE;
		}
	}

	if ( ! geVFile_Write(F, &geBM_Tag, sizeof(geBM_Tag)) )
		return GE_FALSE;

	if ( ! geVFile_Write(F, &geBM_Version, sizeof(geBM_Version)) )
		return GE_FALSE;

	if ( ! geBitmap_WriteInfo(Bmp,F) )
		return GE_FALSE;

	#ifdef COUNT_HEADER_SIZES
		Header_Sizes += 15;
	#endif

	// the pointer Bmp->Info.Palette serves as boolean : HasPalette
	if ( Bmp->Info.Palette )
	{
		if ( ! geBitmap_Palette_WriteToFile(Bmp->Info.Palette,F) )
			return GE_FALSE;
	}

	if ( Bmp->Info.Format == GE_PIXELFORMAT_WAVELET )
	{
		geErrorLog_AddString(-1,"Genesis3D 1.0 does not support Wavelet Images",NULL);
	}
	else
	{
		for( mip = Bmp->Info.MinimumMip; mip <= Bmp->Info.MaximumMip; mip++ )
		{

			// write out all the interesting mips :
			//	the first one, and then mips which are not just
			//	sub-samples of the first (eg. that have been user-set)

			if ( (mip == Bmp->Info.MinimumMip || Bmp->Modified[mip]) && Bmp->Data[mip] )
			{
			uint8 * MipData;
			geBoolean MipDataAlloced;
			uint32 MipDataLen;

				MipDataLen = SHIFT_R_ROUNDUP(Bmp->Info.Width,mip) * SHIFT_R_ROUNDUP(Bmp->Info.Height,mip) *
								gePixelFormat_BytesPerPel(Bmp->Info.Format);

				if ( Bmp->Info.Stride == Bmp->Info.Width )
				{
					MipData = Bmp->Data[mip];
					MipDataAlloced = GE_FALSE;
				}
				else
				{
				int w,h,s,y;
				uint8 * fptr,*tptr;
				
					if ( ! (MipData = geRam_Allocate(MipDataLen) ) )
					{
						geErrorLog_AddString(-1,"Bitmap_WriteToFile : Ram_Alloc failed!",NULL);
						return GE_FALSE;
					}

					MipDataAlloced = GE_TRUE;

					s = SHIFT_R_ROUNDUP(Bmp->Info.Stride,mip)* gePixelFormat_BytesPerPel(Bmp->Info.Format);
					w = SHIFT_R_ROUNDUP(Bmp->Info.Width,mip) * gePixelFormat_BytesPerPel(Bmp->Info.Format);
					h = SHIFT_R_ROUNDUP(Bmp->Info.Height,mip);

					fptr = Bmp->Data[mip];
					tptr = MipData;
					for(y=h;y--;)
					{
						memcpy(tptr,fptr,w);
						fptr += s;
						tptr += w;
					}
				}

				assert( mip <= MIP_MASK );
				flags = mip;
			#ifdef DO_LZ
				flags |= MIP_FLAG_COMPRESSED;
			#endif

				if ( ! geVFile_Write(F, &flags, sizeof(flags)) )
					return GE_FALSE;

			#ifdef DO_LZ
			{
			geVFile * LzF;
				LzF = geVFile_OpenNewSystem(F,GE_VFILE_TYPE_LZ,NULL,NULL,GE_VFILE_OPEN_CREATE);
				if ( ! LzF )
				{
					if ( MipDataAlloced )
						geRam_Free(MipData);
					geErrorLog_AddString(-1,"Bitmap_WriteToFile : LZ File Open failed",NULL);
					return GE_FALSE;
				}

				if ( ! geVFile_Write(LzF, MipData, MipDataLen ) )
					return GE_FALSE;

				if ( ! geVFile_Close(LzF) )
				{
					if ( MipDataAlloced )
						geRam_Free(MipData);
					geErrorLog_AddString(-1,"Bitmap_WriteToFile : LZ File Close failed",NULL);
					return GE_FALSE;
				}
			}
			#else
				if ( ! geVFile_Write(F, MipData, MipDataLen ) )
					return GE_FALSE;
			#endif

				if ( MipDataAlloced )
					geRam_Free(MipData);
			}
		}
		
		// mip > MaximumMip signals End-Of-Mips

		flags = MIP_MASK;
		if ( ! geVFile_Write(F, &flags, sizeof(flags)) )
			return GE_FALSE;
	}

	// the pointer Bmp->Alpha serves as boolean : HasAlpha

	if( Bmp->Alpha )
	{
		if ( ! geBitmap_WriteToFile(Bmp->Alpha,F) )
			return GE_FALSE;
	}

return GE_TRUE;
}

/*}{********** Windows BMP Types *******/

#pragma pack(1)
typedef struct 
{
	uint32      biSize;
	long		biWidth;
	long		biHeight;
	uint16      biPlanes;
	uint16      biBitCount;
	uint32      biCompression;
	uint32      biSizeImage;
	long		biXPelsPerMeter;
	long		biYPelsPerMeter;
	uint32      biClrUsed;
	uint32      biClrImportant;
} BITMAPINFOHEADER;

typedef struct 
{
	uint16   bfType;
	uint32   bfSize;
	uint16   bfReserved1;
	uint16   bfReserved2;
	uint32   bfOffBits;
} BITMAPFILEHEADER;

typedef struct 
{
    uint8    B;
    uint8    G;
    uint8    R;
    uint8    rgbReserved;
} RGBQUAD;
#pragma pack()

static geBoolean geBitmap_ReadFromBMP(geBitmap * Bmp,geVFile * F)
{
BITMAPFILEHEADER 	bmfh;
BITMAPINFOHEADER	bmih;
int bPad,myRowWidth,bmpRowWidth,pelBytes;

	// Windows Bitmap

	if ( ! geVFile_Read(F, &bmfh, sizeof(bmfh)) )
		return GE_FALSE;

	assert(bmfh.bfType == 0x4D42);

	bPad = bmfh.bfOffBits;

	if ( ! geVFile_Read(F, &bmih, sizeof(bmih)) )
		return GE_FALSE;

	if ( bmih.biSize > sizeof(bmih) )
	{
		geVFile_Seek(F, bmih.biSize - sizeof(bmih), GE_VFILE_SEEKCUR);
	}
	else if ( bmih.biSize < sizeof(bmih) )
	{
		geErrorLog_AddString(-1,"CreateFromFile : bmih size bad", NULL);	
		return GE_FALSE;
	}

	if ( bmih.biCompression )
	{
		geErrorLog_AddString(-1,"CreateFromFile : only BI_RGB BMP compression supported", NULL);
		return GE_FALSE;
	}

	bPad -= sizeof(bmih) + sizeof(bmfh);

	switch (bmih.biBitCount) 
	{
		case 8:			/* colormapped image */
			if ( bmih.biClrUsed == 0 ) bmih.biClrUsed = 256;

			if ( ! (Bmp->Info.Palette = geBitmap_Palette_Create(GE_PIXELFORMAT_32BIT_XRGB,bmih.biClrUsed)) )
				return GE_FALSE;

			if ( ! geVFile_Read(F, Bmp->Info.Palette->Data, bmih.biClrUsed * 4) )
				return GE_FALSE;

			bPad -= bmih.biClrUsed * 4;

			Bmp->Info.Format = GE_PIXELFORMAT_8BIT_PAL;
			pelBytes = 1;
			break;
		case 16:			
			Bmp->Info.Format = GE_PIXELFORMAT_16BIT_555_RGB;
			// tried 555,565_BGR & RGB, seems to have too much green
			pelBytes = 2;
			break;
		case 24:			
			Bmp->Info.Format = GE_PIXELFORMAT_24BIT_BGR;
			pelBytes = 3;
			break;
		case 32:			
			Bmp->Info.Format = GE_PIXELFORMAT_32BIT_XRGB; // surprisingly sane !?
			pelBytes = 4;
			break;
		default:
			return GE_FALSE;
	}

	if ( bPad < 0 )
	{
		geErrorLog_AddString(-1,"CreateFromFile : bPad bad", NULL);
		return GE_FALSE;
	}

	geVFile_Seek(F, bPad, GE_VFILE_SEEKCUR);
	
	Bmp->Info.Width = bmih.biWidth;
	Bmp->Info.Height = abs(bmih.biHeight);
	Bmp->Info.Stride = ((bmih.biWidth+3)&(~3));

	Bmp->Info.HasColorKey = GE_FALSE;

	myRowWidth	= Bmp->Info.Stride * pelBytes;
	bmpRowWidth = (((bmih.biWidth * pelBytes) + 3)&(~3));

	assert( bmpRowWidth <= myRowWidth );

	if ( ! geBitmap_AllocSystemMip(Bmp,0) )
		return GE_FALSE;

	if ( bmih.biHeight > 0 )
	{
	int y;
	char * row;
		row = Bmp->Data[0];
		row += (Bmp->Info.Height - 1) * myRowWidth;
		for(y= Bmp->Info.Height;y--;)
		{
			if ( ! geVFile_Read(F, row, bmpRowWidth) )
				return GE_FALSE;				
			row -= myRowWidth;
		}
	}
	else
	{
	int y;
	char * row;
		row = Bmp->Data[0];
		for(y= Bmp->Info.Height;y--;)
		{
			if ( ! geVFile_Read(F, row, bmpRowWidth) )
				return GE_FALSE;				
			row += myRowWidth;
		}
	}

return GE_TRUE;
}	// end BMP reader

/*}{ *** Packed Info IO ***/

#define INFO_FLAG_WH_ARE_LOG2	(1<<0)
#define INFO_FLAG_HAS_CK     	(1<<1)
#define INFO_FLAG_HAS_ALPHA  	(1<<2)
#define INFO_FLAG_HAS_PAL    	(1<<3)

#define INFO_FLAG_IF_NOT_LOG2_ARE_BYTE	(1<<5)

geBoolean geBitmap_ReadInfo(geBitmap *Bmp,geVFile * F)
{
uint8 data[4];
uint8 flags;
uint8 b;
uint16 w;
geBitmap_Info * pi;

	pi = &(Bmp->Info);

	if ( ! geVFile_Read(F,data,3) )
		return GE_FALSE;

	flags = data[0];

	pi->Format = data[1]; // could go in 5 bits

	b = data[2];

	pi->MaximumMip = (b>>4)&0xF;
	Bmp->SeekMipCount = (b)&0xF;

	if ( flags & INFO_FLAG_HAS_PAL )
		pi->Palette  = (geBitmap_Palette *)1;
	if ( flags & INFO_FLAG_HAS_ALPHA )
		Bmp->Alpha = (geBitmap *)1;

	if ( flags & INFO_FLAG_WH_ARE_LOG2 )
	{
	int logw,logh;

		if ( ! geVFile_Read(F,&b,1) )
			return GE_FALSE;

		logw = (b>>4)&0xF;
		logh = (b   )&0xF;

		pi->Width = 1<<logw;
		pi->Height= 1<<logh;
	}
	else if ( flags & INFO_FLAG_IF_NOT_LOG2_ARE_BYTE )
	{
		if ( ! geVFile_Read(F,&b,1) )
			return GE_FALSE;
		pi->Width = b;
		if ( ! geVFile_Read(F,&b,1) )
			return GE_FALSE;
		pi->Height = b;
	}
	else
	{
		if ( ! geVFile_Read(F,&w,2) )
			return GE_FALSE;
		pi->Width = w;
		if ( ! geVFile_Read(F,&w,2) )
			return GE_FALSE;
		pi->Height = w;
	}

	if ( (flags & INFO_FLAG_HAS_CK) && gePixelFormat_BytesPerPel(pi->Format) > 0 )
	{
	uint8 * ptr;
		pi->HasColorKey = GE_TRUE;

		if ( ! geVFile_Read(F,data,gePixelFormat_BytesPerPel(pi->Format)) )
			return GE_FALSE;
		
		ptr = data;
		pi->ColorKey = gePixelFormat_GetPixel(pi->Format,&ptr);
	}

	pi->Stride = pi->Width;

	return GE_TRUE;
}

geBoolean geBitmap_WriteInfo(const geBitmap *Bmp,geVFile * F)
{
uint8 data[64];
uint8 * ptr;
uint8 flags;
uint8 b;
int len,logw,logh;
const geBitmap_Info * pi;

/*
	bit flags :
		W&H are log2
		HasCK
		HasAlpha
		HasPal

		W&H logs in 1 byte, or W & H each in 2 bytes

		Format in 5 bits
		MaxMip in 3 bits
		Bmp->SeekMipCount in 3 bits

		CK in bpp bytes
*/

	pi = &(Bmp->Info);
	flags = 0;
	ptr = data + 1; // flags will go there

	assert( pi->Width < 65536 && pi->Height < 65536 );
	assert( pi->MinimumMip == 0 );
	assert( gePixelFormat_IsValid(pi->Format) );

	*ptr++ = pi->Format; // could go in 5 bits

	b = (pi->MaximumMip << 4) + Bmp->SeekMipCount; // could go in 6 bits
	*ptr++ = b;

	if ( pi->Palette )
		flags |= INFO_FLAG_HAS_PAL;
	if ( Bmp->Alpha )
		flags |= INFO_FLAG_HAS_ALPHA;

	for(logw=0;(1<<logw) < pi->Width;logw++);
	for(logh=0;(1<<logh) < pi->Height;logh++);

	if ( (1<<logw) == pi->Width && (1<<logh) == pi->Height )
	{
		flags |= INFO_FLAG_WH_ARE_LOG2;
		assert( logw <= 0xF && logh <= 0xF );
		b = (logw<<4) + logh;
		*ptr++ = b;
	}
	else
	{
		if ( pi->Width < 256 && pi->Height < 256 )
		{
			flags |= INFO_FLAG_IF_NOT_LOG2_ARE_BYTE;
			*ptr++ = pi->Width;
			*ptr++ = pi->Height;
		}
		else
		{
			*((uint16 *)ptr) = pi->Width;  ptr += 2;
			*((uint16 *)ptr) = pi->Height; ptr += 2;
		}
	}

	if ( pi->HasColorKey && gePixelFormat_BytesPerPel(pi->Format) > 0 )
	{
		flags |= INFO_FLAG_HAS_CK;

		gePixelFormat_PutPixel(pi->Format,&ptr,pi->ColorKey);
	}

	*data = flags;
	len = (int)(ptr - data);

	if ( ! geVFile_Write(F,data,len) )
		return GE_FALSE;

	return GE_TRUE;
}

/*}{ ***************** Palette Functions *******************/

geBoolean geBitmap_Palette_BlitData(gePixelFormat SrcFormat,const void *SrcData,const geBitmap_Palette * SrcPal,
									gePixelFormat DstFormat,	  void *DstData,const geBitmap_Palette * DstPal,
									int Pixels)
{
char *SrcPtr,*DstPtr;
geBoolean SrcHasCK,DstHasCK;
uint32 SrcCK,DstCK;
int SrcCKi,DstCKi;

	assert( SrcData && DstData );

	assert( gePixelFormat_IsRaw(SrcFormat) );
	assert( gePixelFormat_IsRaw(DstFormat) );

	SrcPtr = (char *)SrcData;
	DstPtr = (char *)DstData;

	if ( SrcPal && SrcPal->HasColorKey )
	{
		SrcHasCK = GE_TRUE;
		SrcCK = SrcPal->ColorKey;
		SrcCKi = SrcPal->ColorKeyIndex;
	}
	else
	{
		SrcHasCK = GE_FALSE;
	}

	if ( DstPal && DstPal->HasColorKey )
	{
		DstHasCK = GE_TRUE;
		DstCK = DstPal->ColorKey;
		DstCKi = DstPal->ColorKeyIndex;
	}
	else
	{
		DstHasCK = GE_FALSE;
	}

#if 0 // {} ?
	if ( SrcHasCK && DstHasCK )
	{
		if ( DstCKi == -1 )
			DstCKi = SrcCKi;
	}
#endif

	// no, can't do this, and if SrcCKi < 0 then it's just ignored, which is correct
	//assert( SrcCKi >= 0 );
	//assert( DstCKi >= 0 );

	// CK -> no CK : do nothing
	// no CK -> CK : avoid CK
	// CK -> CK    : assert the CKI's are the same; change color at CKI

	{
	uint32 Pixel;
	int p,R,G,B,A;
	const gePixelFormat_Operations *SrcOps,*DstOps;
	gePixelFormat_Composer		ComposePixel;
	gePixelFormat_Decomposer	DecomposePixel;
	gePixelFormat_PixelPutter	PutPixel;
	gePixelFormat_PixelGetter	GetPixel;

		SrcOps = gePixelFormat_GetOperations(SrcFormat);
		DstOps = gePixelFormat_GetOperations(DstFormat);
		assert(SrcOps && DstOps);

		GetPixel = SrcOps->GetPixel;
		DecomposePixel = SrcOps->DecomposePixel;
		ComposePixel = DstOps->ComposePixel;
		PutPixel = DstOps->PutPixel;

		if ( SrcOps->AMask && ! DstOps->AMask )
		{
			// alpha -> CK in the palette
			for(p=0;p<Pixels;p++)
			{
				Pixel = GetPixel(&SrcPtr);
				DecomposePixel(Pixel,&R,&G,&B,&A);
				if ( SrcHasCK && ( p == SrcCKi || Pixel == SrcCK ) ) 
					A = 0;
				Pixel = ComposePixel(R,G,B,A);

				if ( DstHasCK )
				{
					if ( p == DstCKi || A < 128 )
						Pixel = DstCK;
					else if ( Pixel == DstCK )
						Pixel ^= 1;

					// BTW this makes dark blue into dark purple on glide
				}
				PutPixel(&DstPtr,Pixel);
			}
		}
		else if ( ! SrcOps->AMask && DstOps->AMask )
		{
			// CK -> alpha in the palette
			for(p=0;p<Pixels;p++)
			{
				Pixel = GetPixel(&SrcPtr);
				DecomposePixel(Pixel,&R,&G,&B,&A);
				if ( SrcHasCK && ( p == SrcCKi || Pixel == SrcCK ) ) 
					A = 0;

				Pixel = ComposePixel(R,G,B,A);
				if ( DstHasCK )
				{
					if ( p == DstCKi )
						Pixel = DstCK;
					else if ( Pixel == DstCK )
						Pixel ^= 1;
				}
				PutPixel(&DstPtr,Pixel);
			}
		}
		else
		{
			// both have alpha or both don't
			for(p=0;p<Pixels;p++)
			{
				Pixel = GetPixel(&SrcPtr);
				DecomposePixel(Pixel,&R,&G,&B,&A);
				if ( (SrcHasCK && ( p == SrcCKi || Pixel == SrcCK )) ||
					 DstHasCK && p == DstCKi ) 
				{
					Pixel = DstCK;
				}
				else
				{
					Pixel = ComposePixel(R,G,B,A);
					if ( DstHasCK && Pixel == DstCK )
						Pixel ^= 1;
				}
				PutPixel(&DstPtr,Pixel);
			}
		}
	}

return GE_TRUE;
}

GENESISAPI geBitmap_Palette * GENESISCC geBitmap_Palette_Create(gePixelFormat Format,int Size)
{
geBitmap_Palette * P;
int DataBytes;
const gePixelFormat_Operations * ops;

	ops = gePixelFormat_GetOperations(Format);
	if ( ! ops->RMask )
	{
		geErrorLog_AddString(-1,"geBitmap_Palette_Create : Invalid format for a palette!", NULL);
		return NULL;
	}

	DataBytes = gePixelFormat_BytesPerPel(Format) * Size;
	if ( DataBytes == 0 )
	{
		geErrorLog_AddString(-1,"geBitmap_Palette_Create : Invalid format for a palette!", NULL);
		return NULL;
	}

	allocate(P);
	if ( ! P ) return NULL;
	clear(P);

	P->Size = Size;
	P->Format = Format;
	if ( ! (P->Data = geRam_Allocate(DataBytes)) )
	{
		geRam_Free(P);
		return NULL;
	}

	P->RefCount = 1;
	P->LockCount = 0;

	P->HasColorKey = GE_FALSE;

return P;
}

GENESISAPI geBoolean GENESISCC geBitmap_Palette_CreateRef(geBitmap_Palette *P)
{
	if ( ! P || P->RefCount < 1 )
		return GE_FALSE;
	P->RefCount ++;
return GE_TRUE;
}

GENESISAPI geBitmap_Palette * GENESISCC geBitmap_Palette_CreateFromBitmap(geBitmap * Bmp,geBoolean Slow)
{
geBitmap_Palette * Pal;
	Pal = geBitmap_GetPalette(Bmp);
	if ( Pal )
	{
		geBitmap_Palette_CreateRef(Pal);
		return Pal;
	}
	else
	{
		return createPaletteFromBitmap(Bmp, Slow);
	}
}

geBitmap_Palette * BITMAP_GENESIS_INTERNAL geBitmap_Palette_CreateFromDriver(DRV_Driver * Driver,gePixelFormat Format,int Size)
{
geBitmap_Palette * P;
geRDriver_THandleInfo TInfo;

	assert(Driver);

	allocate(P);
	if ( ! P ) return NULL;
	clear(P);

	P->Size = Size;
	P->Driver = Driver;	

	// {} the pixelformat passed in here has non-trivial implications when the
	//		driver provides more than one possible palette type

	assert( gePixelFormat_IsRaw(Format) );

	P->DriverHandle = geBitmap_CreateTHandle(Driver,Size,1,1,
			Format,0,0,gePixelFormat_HasAlpha(Format),0,RDRIVER_PF_PALETTE);
	if ( ! P->DriverHandle )
	{
		geErrorLog_AddString(-1,"Palette_CreateFromDriver : CreateTHandle", NULL);	
		geRam_Free(P);
		return NULL;
	}

	Driver->THandle_GetInfo(P->DriverHandle,0,&TInfo);
	P->Format = TInfo.PixelFormat.PixelFormat;

	P->HasColorKey = (TInfo.Flags & RDRIVER_THANDLE_HAS_COLORKEY) ? GE_TRUE : GE_FALSE;
	P->ColorKey = TInfo.ColorKey;
	P->ColorKeyIndex = -1;

	P->RefCount = 1;

return P;
}

GENESISAPI geBitmap_Palette * GENESISCC geBitmap_Palette_CreateCopy(const geBitmap_Palette *Palette)
{
geBitmap_Palette * P;

	if ( ! Palette )
		return NULL;

	if ( Palette->Driver )
	{
		P = geBitmap_Palette_CreateFromDriver(Palette->Driver,Palette->Format,Palette->Size);
	}
	else
	{
		P = geBitmap_Palette_Create(Palette->Format,Palette->Size);
	}

	if ( ! P ) return NULL;

	if ( ! geBitmap_Palette_Copy(Palette,P) )
	{
		geBitmap_Palette_Destroy(&P);
		return NULL;
	}

return P;
}

GENESISAPI geBoolean GENESISCC geBitmap_Palette_Destroy(geBitmap_Palette ** ppPalette)
{
geBitmap_Palette * Palette;
	assert(ppPalette);
	if ( Palette = *ppPalette )
	{
		if ( Palette->LockCount )
			return GE_FALSE;
		Palette->RefCount --;
		if ( Palette->RefCount <= 0 )
		{
			if ( Palette->Data )
				geRam_Free(Palette->Data);
			if ( Palette->DriverHandle )
			{
				Palette->Driver->THandle_Destroy(Palette->DriverHandle);
				Palette->DriverHandle = NULL;
			}
			geRam_Free(Palette);
		}
	}
	*ppPalette = NULL;
return GE_TRUE;
}

GENESISAPI geBoolean GENESISCC geBitmap_Palette_Lock(geBitmap_Palette *P, void **pBits, gePixelFormat *pFormat,int *pSize)
{
	assert(P);
	assert(pBits);

	if ( P->LockCount )
		return GE_FALSE;
	P->LockCount++;

	*pBits = NULL;

	if ( P->Data )
	{
		*pBits		= P->Data;
		if ( pFormat )
			*pFormat= P->Format;
		if ( pSize )
			*pSize	= P->Size;
	}
	else if ( P->DriverHandle )
	{
	geRDriver_THandleInfo TInfo;

		if ( ! P->Driver->THandle_GetInfo(P->DriverHandle,0,&TInfo) )
			return GE_FALSE;

		if ( TInfo.Height != 1 )
			return GE_FALSE;

		if ( ! (P->Driver->THandle_Lock(P->DriverHandle,0,pBits)) )
			*pBits = NULL;

		P->DriverBits = *pBits;

		if ( pFormat )
			*pFormat = TInfo.PixelFormat.PixelFormat;
		if ( pSize )
			*pSize = TInfo.Width;
	}

	return (*pBits) ? GE_TRUE : GE_FALSE;
}

GENESISAPI geBoolean GENESISCC geBitmap_Palette_UnLock(geBitmap_Palette *P)
{
	assert(P);
	if ( P->LockCount <= 0 )
		return GE_FALSE;
	P->LockCount--;
	if ( P->LockCount == 0 )
	{
		if ( P->HasColorKey )
		{
			if ( P->ColorKeyIndex >= 0 && P->ColorKeyIndex < P->Size )
			{
			uint8 *Bits,*pBits;
			uint32 Pixel;
			int p;
			const gePixelFormat_Operations *ops;
			gePixelFormat_PixelPutter	PutPixel;
			gePixelFormat_PixelGetter	GetPixel;

				if ( P->Data )
				{
					Bits = P->Data;
				}
				else if ( P->DriverBits )
				{
					Bits = P->DriverBits;
				}

				ops = gePixelFormat_GetOperations(P->Format);
				assert(ops);

				GetPixel = ops->GetPixel;
				PutPixel = ops->PutPixel;

				for(p=0;p<P->Size;p++)
				{
					pBits = Bits;
					Pixel = GetPixel(&Bits);
					if ( p == P->ColorKeyIndex )
					{
						PutPixel(&pBits,P->ColorKey);
					}
					else if ( Pixel == P->ColorKey )
					{
						Pixel ^= 1;
						PutPixel(&pBits,Pixel);
					}
				}
			}
		}
		if ( P->DriverHandle )
		{
			if ( ! P->Driver->THandle_UnLock(P->DriverHandle,0) )
				return GE_FALSE;
			P->DriverBits = NULL;
		}
	}
return GE_TRUE;
}

GENESISAPI geBoolean GENESISCC geBitmap_Palette_SetFormat(geBitmap_Palette * P,gePixelFormat Format)
{
void * NewData;
	
	assert(P);

	if ( P->DriverHandle ) // can't change format on card!
		return GE_FALSE;

	assert( ! P->HasColorKey ); // can't have colorkey accept on Glide

	if ( Format == P->Format )
		return GE_TRUE;

	NewData = geRam_Allocate( gePixelFormat_BytesPerPel(Format) * P->Size );
	if ( ! NewData )
		return GE_FALSE;

	if ( ! geBitmap_Palette_BlitData(P->Format,P->Data,NULL,Format,NewData,NULL,P->Size) )
	{
		geRam_Free(NewData);
		return GE_FALSE;
	}

	geRam_Free(P->Data);
	P->Data = NewData;
	P->Format = Format;

return GE_TRUE;
}

GENESISAPI geBoolean GENESISCC geBitmap_Palette_GetData(const geBitmap_Palette *P,void *Into,gePixelFormat Format,int Size)
{
gePixelFormat FmFormat;
const void *FmData;
int FmSize;
geBoolean Ret;

	assert(P);
	assert(Into);

	if ( ! geBitmap_Palette_Lock((geBitmap_Palette *)P,(void **)&FmData,&FmFormat,&FmSize) )
		return GE_FALSE;

	if ( FmSize < Size )
		Size = FmSize;

	Ret = geBitmap_Palette_BlitData(FmFormat,FmData,P,Format,Into,NULL,Size);
	
	geBitmap_Palette_UnLock((geBitmap_Palette *)P);

return Ret;
}

GENESISAPI geBoolean GENESISCC geBitmap_Palette_GetInfo(const geBitmap_Palette *P,geBitmap_Info *pInfo)
{
	assert(P && pInfo);

	pInfo->Width = pInfo->Stride = P->Size;
	pInfo->Height = 1;

	pInfo->Format = P->Format;
	pInfo->HasColorKey = P->HasColorKey;
	pInfo->ColorKey = P->ColorKey;
	pInfo->MaximumMip = pInfo->MinimumMip = 0;
	pInfo->Palette = NULL;

return GE_TRUE;
}

GENESISAPI geBoolean GENESISCC geBitmap_Palette_SetData(geBitmap_Palette *P,const void *From,gePixelFormat Format,int Colors)
{
gePixelFormat PalFormat;
void *PalData;
int PalSize;
geBoolean Ret;

	assert(P);
	assert(From);

	if ( ! geBitmap_Palette_Lock(P,&PalData,&PalFormat,&PalSize) )
		return GE_FALSE;

	if ( PalSize < Colors )
		Colors = PalSize;

	Ret = geBitmap_Palette_BlitData(Format,From,NULL,PalFormat,PalData,P,Colors);
	
	if ( ! geBitmap_Palette_UnLock(P) )
		return GE_FALSE;

return Ret;
}

GENESISAPI geBoolean GENESISCC geBitmap_Palette_Copy(const geBitmap_Palette * Fm,geBitmap_Palette * To)
{
gePixelFormat FmFormat,ToFormat;
void *FmData,*ToData;
int FmSize,ToSize;
geBoolean Ret;

	assert(Fm);
	assert(To);
	if ( Fm == To )
		return GE_TRUE;

	if ( ! geBitmap_Palette_Lock((geBitmap_Palette *)Fm,&FmData,&FmFormat,&FmSize) )
		return GE_FALSE;

	if ( ! geBitmap_Palette_Lock(To,&ToData,&ToFormat,&ToSize) )
	{
		geBitmap_Palette_UnLock((geBitmap_Palette *)Fm);
		return GE_FALSE;
	}

	if ( FmSize > ToSize )
	{
		Ret = GE_FALSE;
	}
	else
	{
		Ret = geBitmap_Palette_BlitData(FmFormat,FmData,Fm,ToFormat,ToData,To,FmSize);
	}
	
	geBitmap_Palette_UnLock((geBitmap_Palette *)Fm);
	geBitmap_Palette_UnLock(To);

return Ret;
}

GENESISAPI geBoolean GENESISCC geBitmap_Palette_SetEntryColor(geBitmap_Palette *P,int Color,int R,int G,int B,int A)
{
	assert(P);
	
	if ( A < 80 && ! gePixelFormat_HasAlpha(P->Format) && P->HasColorKey )
	{
		return geBitmap_Palette_SetEntry(P,Color,P->ColorKey);
	}
	else if ( P->HasColorKey )
	{
	uint32 Pixel;

		// might have alpha AND colorkey !

		if ( Color == P->ColorKeyIndex ) // and A > 80 because of the above
			return GE_FALSE;

		Pixel = gePixelFormat_ComposePixel(P->Format,R,G,B,A);
		if ( Pixel == P->ColorKey )
			Pixel ^= 1;
			
		return geBitmap_Palette_SetEntry(P,Color,Pixel);
	}
	else
	{
		return geBitmap_Palette_SetEntry(P,Color,gePixelFormat_ComposePixel(P->Format,R,G,B,A));
	}
}

GENESISAPI geBoolean GENESISCC geBitmap_Palette_GetEntryColor(const geBitmap_Palette *P,int Color,int *R,int *G,int *B,int *A)
{
uint32 Pixel;
	assert(P);
	if ( P->HasColorKey )
	{
		if ( Color == P->ColorKeyIndex )
		{
			*R = *G = *B = *A = 0;
			return GE_TRUE;
		}
		else
		{
			if ( ! geBitmap_Palette_GetEntry(P,Color,&Pixel) )
				return GE_FALSE;
			if ( Pixel == P->ColorKey )
			{
				*R = *G = *B = *A = 0;
			}
			else
			{
				gePixelFormat_DecomposePixel(P->Format,Pixel,R,G,B,A);
			}
		}
	}
	else
	{
		if ( ! geBitmap_Palette_GetEntry(P,Color,&Pixel) )
			return GE_FALSE;
		gePixelFormat_DecomposePixel(P->Format,Pixel,R,G,B,A);
	}
	return GE_TRUE;
}

GENESISAPI geBoolean GENESISCC geBitmap_Palette_SetEntry(geBitmap_Palette *P,int Color,uint32 Pixel)
{
	assert(P);

	if ( P->HasColorKey )
	{
		if ( Color == P->ColorKeyIndex )
			return GE_TRUE;
	}

	if ( P->Data )
	{
	char *Data;

		if ( Color >= P->Size )
			return GE_FALSE;

		Data = (char *)(P->Data) + Color * gePixelFormat_BytesPerPel(P->Format);
		gePixelFormat_PutPixel(P->Format,&Data,Pixel);
	}
	else
	{
	char *Data;
	gePixelFormat Format;
	int Size;

		if ( ! geBitmap_Palette_Lock(P,&Data,&Format,&Size) )
			return GE_FALSE;

		if ( Color >= Size )
		{
			geBitmap_Palette_UnLock(P);
			return GE_FALSE;
		}

		Data += Color * gePixelFormat_BytesPerPel(Format);
		gePixelFormat_PutPixel(Format,&Data,Pixel);

		geBitmap_Palette_UnLock(P);
	}
return GE_TRUE;
}

GENESISAPI geBoolean GENESISCC geBitmap_Palette_GetEntry(const geBitmap_Palette *P,int Color,uint32 *Pixel)
{

	assert(P);

	if ( P->Data )
	{
	char *Data;

		if ( Color >= P->Size )
			return GE_FALSE;

		Data = (char *)(P->Data) + Color * gePixelFormat_BytesPerPel(P->Format);
		*Pixel = gePixelFormat_GetPixel(P->Format,&Data);
	}
	else
	{
	char *Data;
	gePixelFormat Format;
	int Size;

		// must cast away const cuz we don't have a lockforread/write on palettes

		if ( ! geBitmap_Palette_Lock((geBitmap_Palette *)P,&Data,&Format,&Size) )
			return GE_FALSE;

		if ( Color >= Size )
		{
			geBitmap_Palette_UnLock((geBitmap_Palette *)P);
			return GE_FALSE;
		}

		Data += Color * gePixelFormat_BytesPerPel(Format);
		*Pixel = gePixelFormat_GetPixel(Format,&Data);

		geBitmap_Palette_UnLock((geBitmap_Palette *)P);
	}
return GE_TRUE;
}

#define PALETTE_INFO_FORMAT_MASK	(0x1F)
#define PALETTE_INFO_FLAG_SIZE256	(1<<5)	// 5 is the low
#define PALETTE_INFO_FLAG_COMPRESS	(1<<6)

GENESISAPI geBitmap_Palette * GENESISCC geBitmap_Palette_CreateFromFile(geVFile *F)
{
geBitmap_Palette * P;
int Size;
gePixelFormat Format;
uint8 flags,b;

	if ( ! geVFile_Read(F, &flags, sizeof(flags)) )
		return NULL;

	Format = flags & PALETTE_INFO_FORMAT_MASK;

	if ( flags & PALETTE_INFO_FLAG_SIZE256 )
	{
		Size = 256;
	}
	else
	{
		if ( ! geVFile_Read(F, &b, sizeof(b)) )
			return NULL;
		Size = b;
	}

	P = geBitmap_Palette_Create(Format,Size);
	if ( ! P )
		return NULL;

	if ( flags & PALETTE_INFO_FLAG_COMPRESS )
	{
		geErrorLog_AddString(-1,"Bitmap_Palette_CreateFromFile : codePal failed!",NULL);
		return GE_FALSE;
	}
	else
	{
		if ( ! geVFile_Read(F, P->Data, gePixelFormat_BytesPerPel(P->Format) * P->Size) )
		{
			geRam_Free(P);
			return NULL;
		}
	}

return P;
}

GENESISAPI geBoolean GENESISCC geBitmap_Palette_WriteToFile(const geBitmap_Palette *P,geVFile *F)
{
int Size;
gePixelFormat Format;
void *Data;

	assert(P);

	assert( P->HasColorKey == GE_FALSE ); // system palettes can't have color key!

	// we usually write the palette's header in one byte :^)

	if ( ! geBitmap_Palette_Lock((geBitmap_Palette *)P,&Data,&Format,&Size) )
		return GE_FALSE;

	{
	uint8 b;

		b = Format;
		assert( b < 32 );
		if ( Size == 256 )
			b |= PALETTE_INFO_FLAG_SIZE256;

		if ( ! geVFile_Write(F, &b, sizeof(b)) )
		{
			geBitmap_Palette_UnLock((geBitmap_Palette *)P);
			return GE_FALSE;
		}

		if ( Size != 256 )
		{
			assert(Size < 256);
			b = Size;
			
			if ( ! geVFile_Write(F, &b, sizeof(b)) )
			{
				geBitmap_Palette_UnLock((geBitmap_Palette *)P);
				return GE_FALSE;
			}
		}
	}

	if ( ! geVFile_Write(F, Data, gePixelFormat_BytesPerPel(Format) * Size) )
	{
		geBitmap_Palette_UnLock((geBitmap_Palette *)P);
		return GE_FALSE;
	}
		
	geBitmap_Palette_UnLock((geBitmap_Palette *)P);

return GE_TRUE;
}

/*}{ ******************** IsValid funcs **************************/

// {} put ErrorLogs indicating where we failed in _IsValid

geBoolean geBitmap_IsValid(const geBitmap *Bmp)
{
	if ( ! Bmp ) return GE_FALSE;

	assert( Bmp->RefCount >= 1 );

	assert( ! (Bmp->LockCount && Bmp->LockOwner) );

	assert( !( (Bmp->DriverDataChanged || Bmp->DriverBitsLocked) &&
			! Bmp->DriverHandle ) );
	assert( ! (Bmp->DriverHandle && ! Bmp->Driver) );

	if ( ! geBitmap_Info_IsValid(&(Bmp->Info)) )
		return GE_FALSE;

	if ( Bmp->DriverHandle && ! geBitmap_Info_IsValid(&(Bmp->DriverInfo)) )
		return GE_FALSE;

	if ( Bmp->LockOwner && Bmp->Alpha )
		assert( Bmp->Alpha->LockOwner );

	if ( Bmp->LockOwner )
	{
		assert(Bmp->LockOwner != Bmp);
		assert( Bmp->LockOwner->LockCount );
	}

	if ( Bmp->DataOwner )
	{
		assert(Bmp->DataOwner != Bmp);
		assert( Bmp->DataOwner->RefCount >= 2 );
	}

	if ( Bmp->Alpha )
	{
		assert(Bmp->Alpha != Bmp);
		if ( ! geBitmap_IsValid(Bmp->Alpha) )
			return GE_FALSE;
	}

return GE_TRUE;
}

geBoolean geBitmap_Info_IsValid(const geBitmap_Info *Info)
{
	if ( ! Info ) return GE_FALSE;

	assert( Info->Width > 0 && Info->Height > 0 && Info->Stride >= Info->Width );

	assert( Info->MinimumMip >= 0 && Info->MaximumMip < MAXMIPLEVELS && Info->MinimumMip <= Info->MaximumMip );

	assert( Info->Format > GE_PIXELFORMAT_NO_DATA && Info->Format < GE_PIXELFORMAT_COUNT );

//	ok to have palette on non-palettized
//	if ( ! gePixelFormat_HasPalette(Info->Format) && Info->Palette )
//		return GE_FALSE;

	if ( Info->Palette )
		if ( ! geBitmap_Palette_IsValid(Info->Palette) )
			return GE_FALSE;

return GE_TRUE;
}

geBoolean geBitmap_Palette_IsValid(const geBitmap_Palette *Pal)
{
	if ( ! Pal ) return GE_FALSE;

	assert(  Pal->Data ||  Pal->DriverHandle );
	assert( !Pal->Data || !Pal->DriverHandle );

	assert( (Pal->Driver && Pal->DriverHandle) ||
		(! Pal->Driver && ! Pal->DriverHandle) );

	assert( Pal->RefCount >= 1 && Pal->Size >= 1 );
	assert( Pal->Format > GE_PIXELFORMAT_NO_DATA && Pal->Format < GE_PIXELFORMAT_COUNT );

return GE_TRUE;
}

#ifdef _DEBUG
GENESISAPI uint32 GENESISCC geBitmap_Debug_GetCount(void)
{
	assert(  _Bitmap_Debug_ActiveRefs >=  _Bitmap_Debug_ActiveCount );

//	Log_Printf("geBitmap_Debug_GetCount : Refs = %d\n",_Bitmap_Debug_ActiveRefs);

//	geBitmap_Gamma_Debug_Report();

	if (  _Bitmap_Debug_ActiveCount == 0 )
		assert(_Bitmap_Debug_ActiveRefs == 0 );

	return _Bitmap_Debug_ActiveCount;
}
#endif

/*}{ ******************** Average Color **************************/

GENESISAPI geBoolean GENESISCC geBitmap_GetAverageColor(const geBitmap *Bmp,int *pR,int *pG,int *pB)
{
	{
	int bpp,x,y,w,h,xtra,dock;
	gePixelFormat Format;
	uint8 * ptr;
	uint32 R,G,B,A,Rt,Gt,Bt,cnt,ck;

		//{} Rt == Rtotal , probably won't overflow; we can handle a 4096x4095 solid-white image

		if ( Bmp->DriverHandle && Bmp->DriverDataChanged )
		{
			// must use the driver bits
			if ( ! geBitmap_Update_DriverToSystem((geBitmap *)Bmp) )
			{
				geErrorLog_AddString(-1,"Bitmap_AverageColor : DriverToSystem failed!",NULL);
				return GE_FALSE;
			}
		}

		Format = Bmp->Info.Format;
		bpp = gePixelFormat_BytesPerPel(Format);
		ptr = Bmp->Data[0];	

		if ( ! ptr || bpp < 1 )
		{
			geErrorLog_AddString(-1,"Bitmap_AverageColor : no data!",NULL);
			return GE_FALSE;
		}

		w = Bmp->Info.Width;
		h = Bmp->Info.Height;
		xtra = (Bmp->Info.Stride - w)*bpp;
		ck = Bmp->Info.ColorKey;
		dock = Bmp->Info.HasColorKey;

		Rt = Gt = Bt = cnt = 0;

		if ( gePixelFormat_HasPalette(Format) )
		{
			// <> Blech!
			geErrorLog_AddString(-1,"Bitmap_AverageColor : doesn't support palettized yet!",NULL);
			#pragma message("Bitmap_AverageColor : doesn't support palettized yet!")
			return GE_FALSE;
		}
		else
		{
		const gePixelFormat_Operations * ops;
		gePixelFormat_ColorGetter GetColor;
		gePixelFormat_PixelGetter GetPixel;
		gePixelFormat_Decomposer Decomposer;

			assert( gePixelFormat_IsRaw(Format) );

			ops = gePixelFormat_GetOperations(Format);
			GetColor = ops->GetColor;
			GetPixel = ops->GetPixel;
			Decomposer = ops->DecomposePixel;

			if ( dock )
			{
				for(y=h;y--;)
				{
					for(x=w;x--;)
					{
					uint32 Pixel;
						Pixel = GetPixel(&ptr);
						if ( Pixel != ck )
						{
							Decomposer(Pixel,&R,&G,&B,&A);
							Rt += R; Gt += G; Bt += B;
							cnt ++;
						}
					}
					ptr += xtra;
				}
			}
			else
			{
				for(y=h;y--;)
				{
					for(x=w;x--;)
					{
						GetColor(&ptr,&R,&G,&B,&A);
						if ( A > 80 )
						{
							Rt += R; Gt += G; Bt += B;
							cnt ++;
						}
					}
					ptr += xtra;
				}
			}
		}

		if ( pR ) *pR = (Rt + (cnt>>1)) / cnt;
		if ( pG ) *pG = (Gt + (cnt>>1)) / cnt;
		if ( pB ) *pB = (Bt + (cnt>>1)) / cnt;
	}

return GE_TRUE;
}

/*}{ ******************** EOF **************************/

