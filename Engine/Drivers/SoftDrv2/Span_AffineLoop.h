/****************************************************************************************/
/*  Span_AffineLoop.H                                                                   */
/*                                                                                      */
/*  Author: Mike Sandige                                                                */
/*  Description:  This is a template to create multiple affine span line drawing        */
/*                routines.  See Span, Span_Factory                                     */
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
while(i-- > 0)
	{
		#if SPANROP & ZTEST
		ZFromMap = *(ZMapBits);
		if (ZFromMap > (Z>>16))
		#endif
			{
				#if SPANROP & ZSET
				*(ZMapBits++) = (ZMAPPIXEL)(Z>>16);
				#else
					#if SPANROP & ZTEST
					ZMapBits++;
					#endif
				#endif

				#if !(( SPANROP & AFLAT) || (SPANROP & AMAP))		// NO alpha
					#if (SPANROP & TMAP) 
						#ifdef TEST_LIGHTMAP
							Color = 0xFFFFFF;
						#else
							Color = Palette[*(TextureBits + ((U>>16)&UMask) + TOPDOWN_OR_BOTTOMUP(((V>>16)&VMask) << StrideShift))];
						#endif
						#if SPANROP & D565 
							#ifdef RGB
								*(DestBits++) = (DESTPIXEL) (	((((Color&0xFF)*R)>>15)&0xF800) | (((((Color&0xFF00)>>8)*G)>>20)&0x7E0) | ((((Color&0xFF0000)>>16)*B)>>26) );
							#else
								*(DestBits++) = (DESTPIXEL) (	((Color&0xF8)<<8) /*R*/ | ((Color&0xFC00)>>5) /*G*/ | ((Color&0xF80000)>>19)/*B*/ );
							#endif
						#else
							#ifdef RGB
								*(DestBits++) = (DESTPIXEL) (	((((Color&0xFF)*R)>>16)&0x7C00) | (((((Color&0xFF00)>>8)*G)>>21)&0x3E0) | ((((Color&0xFF0000)>>16)*B)>>26) );
							#else
								*(DestBits++) = (DESTPIXEL) (	((Color&0xF8)<<7) /*R*/ | ((Color&0xF800)>>6) /*G*/ | ((Color&0xF80000)>>19)/*B*/ );
							#endif
						#endif
						
					#else
						#if SPANROP & D565
							#ifdef RGB
								*(DestBits++) = (DESTPIXEL)	(  ((R>>(RGB_FXP_SHIFTER + 3))<<11) | ((G>>(RGB_FXP_SHIFTER + 2))<<5) | (B>>(RGB_FXP_SHIFTER + 3))  );
							#else
								*(DestBits++) = (DESTPIXEL) Color;
							#endif
						#else
							#ifdef RGB
								*(DestBits++) = (DESTPIXEL)(  ((R>>(RGB_FXP_SHIFTER + 3))<<10) | ((G>>(RGB_FXP_SHIFTER + 3))<<5) | (B>>(RGB_FXP_SHIFTER + 3))  );
							#else
								*(DestBits++) = (DESTPIXEL) Color;
							#endif
						#endif
					#endif
				#endif

				#if (SPANROP & AFLAT) || (SPANROP & AMAP)		// alpha map or alpha flat or both
					{
						int32 DColor,AR,AG,AB;
						#if (SPANROP & AMAP) && (SPANROP & AFLAT)
						int32 Alpha,OneMinusAlpha;
						#endif 

						DColor = *DestBits;
						
						#if SPANROP & TMAP	
							#if (SPANROP & AMAP) 
								Color = *( ((ALPHAMAPPIXEL *)TextureBits) + ((U>>16)&UMask) + TOPDOWN_OR_BOTTOMUP(((V>>16)&VMask) << StrideShift));	//4444 argb
								#ifdef RGB	// alpha map and rgb shading
									AR = (((Color & 0xF00)>>8 ) * R) >>(4+RGB_FXP_SHIFTER);
									AG = (((Color & 0x0F0)>>4 ) * G) >>(4+RGB_FXP_SHIFTER);
									AB = ( (Color & 0x00F)      * B) >>(4+RGB_FXP_SHIFTER);
								#else		// alpha map only
									AR = ((Color & 0xF00)>>4 ) ;
									AG = ((Color & 0x0F0)    ) ;
									AB = ((Color & 0x00F)<<4 ) ;
								#endif
								#if (SPANROP & AFLAT)
									Alpha  = ((Color>>12) * A)>>4;
									OneMinusAlpha = 16-Alpha;
								#else
									A = (Color>>12);
									OneMinusA = 16-A;
								#endif
							#else	// texture map without alpha
								Color = Palette[*(TextureBits + ((U>>16)&UMask) + TOPDOWN_OR_BOTTOMUP(((V>>16)&VMask) << StrideShift))];						
								#ifdef RGB
									AR = (( Color & 0xFF    )      * R) >>(8+RGB_FXP_SHIFTER);
									AG = (((Color & 0xFF00  )>>8 ) * G) >>(8+RGB_FXP_SHIFTER);
									AB = (((Color & 0xFF0000)>>16) * B) >>(8+RGB_FXP_SHIFTER);
								#else
									AR = (Color & 0xFF    );
									AG = (Color & 0xFF00  )>>8;
									AB = (Color & 0xFF0000)>>16;
								#endif
							#endif
						#else	
							// no texture
							#ifdef RGB	
								AR = R >>RGB_FXP_SHIFTER;
								AG = G >>RGB_FXP_SHIFTER;
								AB = B >>RGB_FXP_SHIFTER;
							#else	
								AR = (Color & 0xFF    );
								AG = (Color & 0xFF00  )>>8;
								AB = (Color & 0xFF0000)>>16; 
							#endif
						#endif
						
						#if (SPANROP & AMAP) && (SPANROP & AFLAT)
							#if SPANROP & D565
								AR = (((DColor&0xF800)>>8) * OneMinusAlpha) + (AR * Alpha);
								AG = (((DColor&0x7E0)>>3)  * OneMinusAlpha) + (AG * Alpha);
								AB = (((DColor&0x1F)<<3)   * OneMinusAlpha) + (AB * Alpha);
								*(DestBits++) = (DESTPIXEL) ( ( (AR>>7)<<11) | ( (AG>>6)<<5 ) | (AB>>7) );
							#else
								AR = (((DColor&0x7C00)>>7) * OneMinusAlpha) + (AR * Alpha);
								AG = (((DColor&0x3E0)>>2)  * OneMinusAlpha) + (AG * Alpha);
								AB = (((DColor&0x1F)<<3)   * OneMinusAlpha) + (AB * Alpha);
								*(DestBits++) = (DESTPIXEL) ( ( (AR>>7)<<10) | ( (AG>>7)<<5 ) | (AB>>7) );
							#endif
						#else
							#if SPANROP & D565
								AR = (((DColor&0xF800)>>8) * OneMinusA) + (AR * A);
								AG = (((DColor&0x7E0)>>3)  * OneMinusA) + (AG * A);
								AB = (((DColor&0x1F)<<3)   * OneMinusA) + (AB * A);
								*(DestBits++) = (DESTPIXEL) ( ( (AR>>7)<<11) | ( (AG>>6)<<5 ) | (AB>>7) );
							#else
								AR = (((DColor&0x7C00)>>7) * OneMinusA) + (AR * A);
								AG = (((DColor&0x3E0)>>2)  * OneMinusA) + (AG * A);
								AB = (((DColor&0x1F)<<3)   * OneMinusA) + (AB * A);
								*(DestBits++) = (DESTPIXEL) ( ( (AR>>7)<<10) | ( (AG>>7)<<5 ) | (AB>>7) );
							#endif
						#endif

					}
				#endif
												
			}
		#if SPANROP & ZTEST
		else
			{
				ZMapBits++;
				DestBits++;
			}
		#endif
	
		#if SPANROP & TMAP
		U += dU;
		V += dV;
		#endif
		#ifdef RGB
		R += dR;
		G += dG;
		B += dB;
		#endif
		#ifdef ZBUF
		Z += dZ;
		#endif
	}