/****************************************************************************************/
/*  Bitmap_Gamma.c                                                                      */
/*                                                                                      */
/*  Author: Charles Bloom                                                               */
/*  Description:  The Bitmap_Gamma_Apply function                                       */
/*					Fast Gamma correction routines for various pixel formats			*/
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


#include "bitmap._h"
#include "bitmap.__h"
#include "bitmap_gamma.h"
#include "pixelformat.h"
#include "errorlog.h"
#include <assert.h>
#include <math.h>

/*}{*******************************************************/

static uint32	Gamma_Lut[256];
static uint32	Gamma_Lut_Inverse[256];
static geFloat	ComputedGamma_Lut = 0.0f;

static uint16 	Gamma_565_RGB[1<<16];
static geFloat	ComputedGamma_565_RGB = 0.0f;
static uint16	Gamma_4444_ARGB[1<<16];
static geFloat	ComputedGamma_4444_ARGB = 0.0f;

/*}{*******************************************************/

void geBitmap_Gamma_Compute_Lut(double Gamma);
void geBitmap_GammaCorrect_Data_4444_ARGB(void * Bits,geBitmap_Info * pInfo);
void geBitmap_GammaCorrect_Data_565_RGB(void * Bits,geBitmap_Info * pInfo);
geBoolean geBitmap_GammaCorrect_Data(void * Bits,geBitmap_Info * pInfo, geBoolean Invert);

/*}{*******************************************************/

geBoolean geBitmap_Gamma_Apply(geBitmap * Bitmap,geBoolean Invert)
{
geBoolean Ret = GE_TRUE;
geFloat Gamma;

	assert(Bitmap);

	Gamma = Bitmap->DriverGamma;
	if ( Gamma <= 0.1f ) // assume they meant 1.0f
		return GE_TRUE;

	// Gamma only works on driver data

	// do-nothing gamma:
	if ( fabs(Gamma - 1.0) < 0.1 )
		return GE_TRUE;

	if ( Bitmap->LockOwner )
		Bitmap = Bitmap->LockOwner;
	if ( Bitmap->LockCount || Bitmap->DataOwner )
		return GE_FALSE;

	if ( ! Bitmap->DriverHandle )	// nothing to do
		return GE_TRUE;

	if ( ComputedGamma_Lut != Gamma )
	{
		geBitmap_Gamma_Compute_Lut(Gamma);
	}

	if ( gePixelFormat_HasPalette(Bitmap->DriverInfo.Format) )
	{
	geBitmap_Palette *	Pal;
	geBitmap_Info		PalInfo;
	void *	Bits;
	int		Size;
	gePixelFormat Format;
	
		// gamma correct the palette

		assert(Bitmap->DriverInfo.Palette);
		Pal = Bitmap->DriverInfo.Palette;

		if ( ! geBitmap_Palette_Lock(Pal,&Bits,&Format,&Size) )
			return GE_FALSE;

		geBitmap_Palette_GetInfo(Pal,&PalInfo);

		if ( ! geBitmap_GammaCorrect_Data(Bits,&PalInfo,Invert) )
			Ret = GE_FALSE;

		geBitmap_Palette_UnLock(Pal);
	}
	else
	{
	geBitmap_Info	Info;
	void * 			Bits;
	int				mip,mipCount;
	geBitmap *Locks[8],*Lock;

		assert( Bitmap->DriverInfo.MinimumMip == 0 );
		mipCount = Bitmap->DriverInfo.MaximumMip + 1;

		// old: work directly on the driver bits so that
		//		we don't get any DriverDataChanged or Modified[mip] flags !
		// new: just use UnLock_NoChange

		assert(Bitmap->Driver);
		assert(Bitmap->DriverHandle);

		//if ( mipCount > 1 ) ; true, but pointless
		//	assert(mipCount == 4);

		if ( ! geBitmap_LockForWrite(Bitmap,Locks,0,mipCount-1) )
		{
			geErrorLog_AddString(-1,"geBitmap_Gamma_Apply : LockforWrite failed", NULL);
			return GE_FALSE;
		}

		for(mip=0;mip<mipCount;mip++)
		{
			Lock = Locks[mip];
			
			if ( ! geBitmap_GetInfo(Lock,&Info,NULL) )
			{
				geErrorLog_AddString(-1,"geBitmap_Gamma_Apply : GetInfo failed", NULL);
				return GE_FALSE;
			}

			Bits = geBitmap_GetBits(Lock);
			assert(Bits);

			if ( ! geBitmap_GammaCorrect_Data(Bits,&Info,Invert) )
			{
				geErrorLog_AddString(-1,"geBitmap_Gamma_Apply : GammaCorrect_Data", NULL);
				Ret = GE_FALSE;
			}
		}

		if ( ! geBitmap_UnLockArray_NoChange(Locks,mipCount) )
		{
			geErrorLog_AddString(-1,"geBitmap_Gamma_Apply : UnLock failed", NULL);
			return GE_FALSE;
		}
	}

return Ret;
}

/*}{*******************************************************/

geBoolean geBitmap_GammaCorrect_Data(void * Bits,geBitmap_Info * pInfo, geBoolean Invert)
{
const gePixelFormat_Operations * ops;
uint32 bpp,w,h,xtra,x,y;
uint32 * Lut;
gePixelFormat Format;
gePixelFormat_Decomposer	Decompose;
gePixelFormat_Composer		Compose;
gePixelFormat_ColorGetter	GetColor;
gePixelFormat_ColorPutter	PutColor;

	Format = pInfo->Format;
	ops = gePixelFormat_GetOperations(Format);
	if ( ! ops )
		return GE_FALSE;
	
	Decompose	= ops->DecomposePixel;
	Compose		= ops->ComposePixel;
	GetColor 	= ops->GetColor;
	PutColor	= ops->PutColor;

	assert( Compose && Decompose && GetColor && PutColor );

	if ( ! Invert )
	{
		if ( Format == GE_PIXELFORMAT_16BIT_565_RGB )
		{
			geBitmap_GammaCorrect_Data_565_RGB(Bits,pInfo);
			return GE_TRUE;
		}
		else if ( Format == GE_PIXELFORMAT_16BIT_4444_ARGB )
		{
			geBitmap_GammaCorrect_Data_4444_ARGB(Bits,pInfo);
			return GE_TRUE;
		}
	}

	if ( Invert )
		Lut = Gamma_Lut_Inverse;
	else
		Lut = Gamma_Lut;

	bpp = ops->BytesPerPel;
	w = pInfo->Width;
	h = pInfo->Height;
	xtra = pInfo->Stride - w;

	if ( pInfo->HasColorKey )
	{
		switch(bpp)
		{
			default:
			case 0:
				return GE_FALSE;
			case 1:
			{
			uint8 *ptr,ck;
				ck = (uint8)pInfo->ColorKey;
				ptr = Bits;
				for(y=h;y--;)
				{
					for(x=w;x--;)
					{
						if ( *ptr != ck )
						{
							*ptr = (uint8)Lut[*ptr];
							if ( *ptr == ck )
								*ptr ^= 1;
						}
						ptr++;
					}
					ptr += xtra;
				}
				assert( (int)(ptr) == ( ((int)Bits) + pInfo->Height * pInfo->Stride ) );
				break;
			}
			case 2:
			{
			uint16 *ptr,ck;
			uint32 R,G,B,A,Pixel;
				ck = (uint16)pInfo->ColorKey;
				ptr = Bits;
				for(y=h;y--;)
				{
					for(x=w;x--;)
					{
						if ( *ptr == ck )
						{
							ptr++;
						}
						else
						{
							Decompose(*ptr,&R,&G,&B,&A);
							R = Lut[R];
							G = Lut[G];
							B = Lut[B];
							Pixel = Compose(R,G,B,A);
							if ( Pixel == ck )
								Pixel ^= 1;
							*ptr++ = (uint16)Pixel;
						}
					}
					ptr += xtra;
				}
				assert( (int)(ptr) == ( ((int)Bits) + pInfo->Height * pInfo->Stride * 2 ) );
				break;
			}

			case 3:
			{
			uint8 *ptr;
			uint32 R,G,B,A,Pixel,ck;
				ptr = Bits;
				xtra *= 3;
				ck = pInfo->ColorKey;
				for(y=h;y--;)
				{
					for(x=w;x--;)
					{
						Pixel = (ptr[0]<<16) + (ptr[1]<<8) + ptr[2];
						if ( Pixel == ck )
						{
							ptr+=3;
						}
						else
						{
							Decompose(Pixel,&R,&G,&B,&A);
							R = Lut[R];
							G = Lut[G];
							B = Lut[B];
							Pixel = Compose(R,G,B,A);	
							if ( Pixel == ck )
								Pixel ^= 1;
							*ptr++ = (uint8)((Pixel>>16)&0xFF);
							*ptr++ = (uint8)((Pixel>>8)&0xFF);
							*ptr++ = (uint8)((Pixel)&0xFF);
						}
					}
					ptr += xtra;
				}
				assert( (int)(ptr) == ( ((int)Bits) + pInfo->Height * pInfo->Stride * 3 ) );
				break;
			}

			case 4:
			{
			uint32 *ptr,ck;
			uint32 R,G,B,A,Pixel;
				ck = pInfo->ColorKey;
				ptr = Bits;
				for(y=h;y--;)
				{
					for(x=w;x--;)
					{
						if ( *ptr == ck )
						{
							ptr++;
						}
						else
						{
							Decompose(*ptr,&R,&G,&B,&A);
							R = Lut[R];
							G = Lut[G];
							B = Lut[B];
							Pixel = Compose(R,G,B,A);
							if ( Pixel == ck )
								Pixel ^= 1;
							*ptr++ = Pixel;
						}
					}
					ptr += xtra;
				}
				assert( (int)(ptr) == ( ((int)Bits) + pInfo->Height * pInfo->Stride * 4 ) );
				break;
			}
		}
	}
	else
	{
		switch(bpp)
		{
			default:
			case 0:
				return GE_FALSE;
			case 1:
			{
			uint8 *ptr;
				ptr = Bits;
				for(y=h;y--;)
				{
					for(x=w;x--;)
					{
						*ptr++ = (uint8)Lut[*ptr];
					}
					ptr += xtra;
				}
				assert( (int)(ptr) == ( ((int)Bits) + pInfo->Height * pInfo->Stride ) );
				break;
			}
			case 2:
			{
			uint16 *ptr;
			uint32 R,G,B,A;
				ptr = Bits;
				for(y=h;y--;)
				{
					for(x=w;x--;)
					{
						Decompose(*ptr,&R,&G,&B,&A);
						R = Lut[R];
						G = Lut[G];
						B = Lut[B];
						*ptr++ = (uint16)Compose(R,G,B,A);
					}
					ptr += xtra;
				}
				assert( (int)(ptr) == ( ((int)Bits) + pInfo->Height * pInfo->Stride * 2 ) );
				break;
			}

			case 3:
			{
			uint8 *ptr,*ptrz;
			uint32 R,G,B,A;
				ptr = Bits;
				xtra *= 3;
				for(y=h;y--;)
				{
					for(x=w;x--;)
					{
						ptrz = ptr;
						GetColor(&ptrz,&R,&G,&B,&A);
						R = Lut[R];
						G = Lut[G];
						B = Lut[B];
						PutColor(&ptr,R,G,B,A);
					}
					ptr += xtra;
				}
				assert( (int)(ptr) == ( ((int)Bits) + pInfo->Height * pInfo->Stride * 3 ) );
				break;
			}

			case 4:
			{
			uint32 *ptr;
			uint32 R,G,B,A;
				ptr = Bits;
				for(y=h;y--;)
				{
					for(x=w;x--;)
					{
						Decompose(*ptr,&R,&G,&B,&A);
						R = Lut[R];
						G = Lut[G];
						B = Lut[B];
						*ptr++ = Compose(R,G,B,A);
					}
					ptr += xtra;
				}
				assert( (int)(ptr) == ( ((int)Bits) + pInfo->Height * pInfo->Stride * 4 ) );
				break;
			}
		}
	}

	return GE_TRUE;
}

/*}{*******************************************************/

void geBitmap_GammaCorrect_Data_565_RGB(void * Bits,geBitmap_Info * pInfo)
{
uint32 w,h,xtra,x,y;
uint16 * ptr;

	if ( ComputedGamma_Lut != ComputedGamma_565_RGB )
	{
	uint32 r,g,b,R,G,B;
	uint32 ipel,opel;

		// compute 565 lookup table
		for(r=0;r<32;r++)
		{
			R = Gamma_Lut[ (r<<3) ] >> 3;
			for(g=0;g<64;g++)
			{
				G = Gamma_Lut[ (g<<2) ] >> 2;
				for(b=0;b<32;b++)
				{
					B = Gamma_Lut[ (b<<3) ] >> 3;
					ipel = (r<<11) + (g<<5) + b;
					opel = (R<<11) + (G<<5) + B;
					assert( opel < 65536 );
					Gamma_565_RGB[ipel] = (uint16)opel;
				}
			}
		}
		ComputedGamma_565_RGB = ComputedGamma_Lut;
	}

	w		= pInfo->Width;
	h		= pInfo->Height;
	xtra	= pInfo->Stride - w;
	ptr 	= Bits;

	if ( pInfo->HasColorKey )
	{
	uint16 ck;
		ck = (uint16) pInfo->ColorKey;
		
		for(y=h;y--;)
		{
			for(x=w;x--;)
			{
				if ( *ptr != ck )
				{
					*ptr = Gamma_565_RGB[ *ptr ];
					if ( *ptr == ck )
						*ptr ^= 1;
				}
				ptr ++;
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
				*ptr++ = Gamma_565_RGB[ *ptr ];
			}
			ptr += xtra;
		}
	}

	assert( (int)(ptr) == ( ((int)Bits) + pInfo->Height * pInfo->Stride * 2 ) );

}
					
void geBitmap_GammaCorrect_Data_4444_ARGB(void * Bits,geBitmap_Info * pInfo)
{
uint32 w,h,xtra,x,y;
uint16 * ptr;

	if ( ComputedGamma_Lut != ComputedGamma_4444_ARGB )
	{
	uint32 r,g,b,a,R,G,B;
	uint32 ipel,opel;

		// compute 4444 lookup table
		for(r=0;r<16;r++)
		{
			R = Gamma_Lut[r<<4] >> 4;
			for(g=0;g<16;g++)
			{
				G = Gamma_Lut[g<<4] >> 4;
				for(b=0;b<16;b++)
				{
					B = Gamma_Lut[b<<4] >> 4;
					for(a=0;a<16;a++)
					{
						ipel = (a<<12) + (r<<8) + (g<<4) + b;
						opel = (a<<12) + (R<<8) + (G<<4) + B;
						assert( opel < 65536 );
						Gamma_4444_ARGB[ipel] = (uint16)opel;
					}
				}
			}
		}
		ComputedGamma_4444_ARGB = ComputedGamma_Lut;
	}

	w		= pInfo->Width;
	h		= pInfo->Height;
	xtra	= pInfo->Stride - w;
	ptr 	= Bits;

	if ( pInfo->HasColorKey )
	{
	uint16 ck;
		ck = (uint16) pInfo->ColorKey;
		
		for(y=h;y--;)
		{
			for(x=w;x--;)
			{
				if ( *ptr != ck )
				{
					*ptr = Gamma_4444_ARGB[ *ptr ];
					if ( *ptr == ck )
						*ptr ^= 1;
				}
				ptr ++;
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
				*ptr++ = Gamma_4444_ARGB[ *ptr ];
			}
			ptr += xtra;
		}
	}

	assert( (int)(ptr) == ( ((int)Bits) + pInfo->Height * pInfo->Stride * 2 ) );
}
																	
void geBitmap_Gamma_Compute_Lut(double Gamma)
{
uint32 c,gc,lgc;

	lgc = 0;
	for(c=0;c<256;c++)
	{
		gc = (uint32)( 255.0 * pow( c * (1.0/255.0) , 1.0 / Gamma ) + 0.4 );
		if ( gc > 255 ) gc = 255;
		Gamma_Lut[c] = gc;
		assert( lgc <= gc );
		for(lgc;lgc<=gc;lgc++)
			Gamma_Lut_Inverse[lgc] = c;
		lgc = gc;
	}
	for(gc;gc<256;gc++)
		Gamma_Lut_Inverse[gc] = 255;

	Gamma_Lut[0] = 0;
	Gamma_Lut_Inverse[0] = 0;
	Gamma_Lut[255] = 255;
	Gamma_Lut_Inverse[255] = 255;

	ComputedGamma_Lut = (float)Gamma;
}

/*}{*******************************************************/
