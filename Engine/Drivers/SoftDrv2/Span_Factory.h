/****************************************************************************************/
/*  Span_Factory.H                                                                      */
/*                                                                                      */
/*  Author: Mike Sandige                                                                */
/*  Description:  This is a template to create multiple span line drawing               */
/*                routines.  See Span                                                   */
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

// This generates various span drawing routines
//   The flag bits are
//     TMAP:   indicates texture mapping is used.  
//     LSHADE: indicates gouraud rgb lighting is used.  
//	   ZSET:   indicates z buffer is to be set
//	   ZTEST:  indicates z buffer is to be tested


//  The idea is to break the span line into sub-spans that are perspective correct at the end points.  The
//  sub-span is affine mapped.  So every few pixels a new sub-span end point is computed, and the point 
//  is connected with an affine mapper.  The span is broken down into a series of sub-spans of a fixed length,
//  and the last sub-span (what ever is left over).  

#ifndef SPANROP
#error  must define SPANROP for function creation options.
#endif


#if (SPANROP & ZSET || (SPANROP & ZTEST))
	#define ZBUF	// zbuffering used at all
#else
	#undef ZBUF
#endif

#if (SPANROP & LSHADE) || (SPANROP & LMAP)
	#define RGB		// pixels are rgb lit
#else
	#undef RGB
#endif

#if (SPANROP & TMAP) || (defined(ZBUF))
	#undef AFFINE
#else
	#define AFFINE		// break into subspans
#endif

#if !(  (SPANROP & LMAP) || ( (SPANROP & LSHADE ) || (SPANROP & LFLAT) )  )
	#error must define lighting 
#endif

#if (SPANROP & LMAP) && !(SPANROP & TMAP)
	#error alpha map must accompany texture map
#endif


#if (SPANROP & AMAP) && !(SPANROP & TMAP)
	#error  alpha map is embedded in tmap
#endif


//void GENESISCC Span_C_xxx(void )
{
	#ifndef AFFINE
	int32 SubSpanOneOverZ;
	int OneOverSubSpanWidth;
	int  SubSpanWidth = Triangle.Gradients.SubSpanWidth; 
	int  SubSpanShift = Triangle.Gradients.SubSpanShift;
	#endif
	// int32 OneOverZ,UOverZ,VOverZ;	// globals
	// int32 R,G,B;						// globals
	// int32 URight,VRight;				// globals
	int  i;
	int W=Triangle.SpanWidth;
	DESTPIXEL    *DestBits    = Triangle.DestBits;

	#if SPANROP & TMAP
	int32 U, V;
	int32 dU, dV;
	int32 UMask = Triangle.UMask;
	int32 VMask = Triangle.VMask;
	TEXTUREPIXEL *TextureBits = Triangle.TextureBits;
	#if !(SPANROP & AMAP)
		uint32 *Palette            = Triangle.Palette;
	#endif
	int32 StrideShift = Triangle.StrideShift;
	int32 SubSpanUOverZ, SubSpanVOverZ;
	#endif
	#if (SPANROP & TMAP) || (SPANROP & LFLAT) 
	uint32 Color;
	#endif
	
	#ifdef RGB
	int32 dR, dG, dB;
	#endif
	
	#if (defined(ZBUF) || (SPANROP & TMAP))
	int32 ZRight;
	#endif
	
	#ifdef ZBUF
	int32 Z, dZ;
	ZMAPPIXEL    *ZMapBits    = Triangle.ZMapBits;
	int32 ZScale        = Triangle.Gradients.ZScale;
	#endif
	
	#if SPANROP & ZTEST
	ZMAPPIXEL     ZFromMap;
	#endif


	#if SPANROP & LFLAT
		#if SPANROP & D565
			Color = (  ((R>>(RGB_FXP_SHIFTER + 3))<<11) | ((G>>(RGB_FXP_SHIFTER + 2))<<5) | (B>>(RGB_FXP_SHIFTER + 3))  );
		#else
			Color = (  ((R>>(RGB_FXP_SHIFTER + 3))<<10) | ((G>>(RGB_FXP_SHIFTER + 3))<<5) | (B>>(RGB_FXP_SHIFTER + 3))  );
		#endif
	#endif
	
	#if SPANROP & LSHADE
	dR = Triangle.Gradients.dRdX;
	dG = Triangle.Gradients.dGdX;
	dB = Triangle.Gradients.dBdX;
	#endif
	
	if (Triangle.Gradients.Affine)
		{
			W = Triangle.SpanWidth;
			#if SPANROP & TMAP
			U = UOverZ;
			V = VOverZ;
			dU = Triangle.Gradients.dUOverZdX;
			dV = Triangle.Gradients.dVOverZdX;
			#endif
			#ifdef ZBUF
			Z = OneOverZ;
			dZ = Triangle.Gradients.dOneOverZdX;
			#endif
			#if SPANROP & LMAP
			{
				URight = U;
				VRight = V;
				Span_LightMapSample();
				R=RRight;G=GRight;B=BRight;
				OneOverSubSpanWidth = Triangle.SmallDivideTable[W];
				URight = U + W * dU;
				VRight = V + W * dV;
			}
			#endif
			goto AffineLoop;
		}

#ifndef AFFINE
	// either ZBUF or TMAP 
	
	ZRight = OOZ_MUL_PREP( (OOZ_NUMERATOR/(  OOZ_DIV_PREP(OneOverZ)|0x1 )));

	#if SPANROP & TMAP
	URight = (ZRight * OZ_MUL_PREP(UOverZ));  
	VRight = (ZRight * OZ_MUL_PREP(VOverZ));
	U = URight;
	V = VRight;
		#if SPANROP & LMAP
		Span_LightMapSample();
		R=RRight;G=GRight;B=BRight;
		#endif
	#endif

	#ifdef ZBUF
	Z = OOZ_MUL_Z(ZRight,ZScale);
	#endif
	

	if (W>SubSpanWidth)
		{
			SubSpanOneOverZ = Triangle.Gradients.dOneOverZdX << SubSpanShift;
			#if SPANROP & TMAP
			SubSpanUOverZ   = Triangle.Gradients.dUOverZdX   << SubSpanShift;
			SubSpanVOverZ   = Triangle.Gradients.dVOverZdX   << SubSpanShift;
			#endif
			while(W > SubSpanWidth)
				{
					OneOverZ += SubSpanOneOverZ;
					ZRight = OOZ_MUL_PREP( (OOZ_NUMERATOR/(  OOZ_DIV_PREP(OneOverZ)|0x1 )));
					i  = SubSpanWidth;
					W -= SubSpanWidth;

					#if SPANROP & TMAP
					UOverZ   += SubSpanUOverZ;
					URight = (ZRight * OZ_MUL_PREP(UOverZ));  
					dU = (URight - U)>> SubSpanShift;
				
					VOverZ   += SubSpanVOverZ;
					VRight = (ZRight * OZ_MUL_PREP(VOverZ));
					dV = (VRight - V)>> SubSpanShift;
					#endif

					#ifdef ZBUF
					ZRight = OOZ_MUL_Z(ZRight,ZScale);
					dZ = (ZRight - Z)>> SubSpanShift;
					#endif
			
					#if SPANROP & LMAP
					Span_LightMapSample();
					dR = (RRight - R)>> SubSpanShift;
					dG = (GRight - G)>> SubSpanShift;
					dB = (BRight - B)>> SubSpanShift;
					#endif


					#include "Span_AffineLoop.h"
				}
		}
#endif		//AFFINE
			
	if (W>0)
		{
			#ifndef AFFINE
			OneOverSubSpanWidth = Triangle.SmallDivideTable[W];
			OneOverZ += Triangle.Gradients.dOneOverZdX * W;
			ZRight = OOZ_MUL_PREP( (OOZ_NUMERATOR/(  OOZ_DIV_PREP(OneOverZ)|0x1 )));
			#endif
			
			#if SPANROP & TMAP
			UOverZ   += Triangle.Gradients.dUOverZdX   * W;
			URight = (ZRight * OZ_MUL_PREP(UOverZ));  
			dU = ( ( ((URight - U)>>12) * (OneOverSubSpanWidth)))>>4;
			
			VOverZ   += Triangle.Gradients.dVOverZdX   * W;
			VRight = (ZRight * OZ_MUL_PREP(VOverZ));
			dV = ( ( ((VRight - V)>>12) * (OneOverSubSpanWidth)))>>4;
			#endif

			#ifdef ZBUF
			ZRight = OOZ_MUL_Z(ZRight,ZScale);
			dZ = ( ( ((ZRight - Z)>>12) * (OneOverSubSpanWidth)))>>4;
			#endif
	
			AffineLoop:	

			#if SPANROP & LMAP
			Span_LightMapSample();
			dR = ( ( ((RRight - R)>>12) * (OneOverSubSpanWidth)))>>4;
			dG = ( ( ((GRight - G)>>12) * (OneOverSubSpanWidth)))>>4;
			dB = ( ( ((BRight - B)>>12) * (OneOverSubSpanWidth)))>>4;
			#endif

			i=W;
			#include "Span_AffineLoop.h"
		}	



}


#undef SPANROP
#undef ZBUF
#undef RGB


