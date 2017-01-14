/****************************************************************************************/
/*  SpanEdges_Factory.H                                                                 */
/*                                                                                      */
/*  Author: Mike Sandige                                                                */
/*  Description:  This is a template to create multiple function with similar structure,*/
/*                but with slightly different options.  This template creates           */
/*                edge-spanning functions for walking the edges of triangles, generating*/
/*                spans for each scan line.                                             */
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

// This generates various edge-spanning routines
//   The flag bits are
//     TMAP:   indicates texture mapping is used.  
//							1/Z, U/Z, V/Z are stepped and prepared for the span routine
//     SBUF:   indicates span buffering is used.  SpanBuffer is queried and segments are generated.  
//							1/Z, Y are stepped and prepared for the span routine
//     LSHADE: indicates gouraud rgb lighting is used.  
//							R,G,B are stepped and prepared for the span routine
//	   ZBUF:   indicates z buffering is used.  
//							1/Z is stepped and prepared for the span routine
//    Combinations generate the (minimal) combination of stepping and preparation


#ifndef SPANEDGES
error.  must define SPANEDGES for function creation options.
#endif

#define MAX_RGB_VALUE (255<<RGB_FXP_SHIFTER)

// void SpanEdges_xxx(int Height)
{
	while(Height--) 
	{
		Triangle.SpanWidth = Triangle.Right.X - Triangle.Left.X;
		if (Triangle.SpanWidth>0)
			{

				#if SPANEDGES & SBUF
					int Spans = SpanBuffer_ClipAndAdd(Triangle.Left.Y,Triangle.Left.X,Triangle.SpanWidth);
					SpanBuffer_ClipSegment *Segment = &(SpanBuffer_Segments[0]);
					for (;Spans>0; Spans--,Segment++)
						{
							Triangle.DestBits = ((DESTPIXEL *)Triangle.Left.Dest) + Segment->LeftOffset;
							
							#if SPANEDGES & TMAP
							Triangle.ZMapBits = ((ZMAPPIXEL *)(Triangle.Left.Dest + Triangle.ZBufferAddressDelta)) + Segment->LeftOffset;
							#endif

							#if (SPANEDGES & TMAP) || (SPANEDGES & ZBUF)
							OneOverZ = Triangle.Left.OneOverZ + Triangle.Gradients.dOneOverZdX * Segment->LeftOffset;
							#endif
							
							#if SPANEDGES & TMAP
							UOverZ   = Triangle.Left.UOverZ   + Triangle.Gradients.dUOverZdX   * Segment->LeftOffset;
							VOverZ   = Triangle.Left.VOverZ   + Triangle.Gradients.dVOverZdX   * Segment->LeftOffset;
							#endif

							#if (SPANEDGES & LSHADE)
							R = Triangle.Left.R + Triangle.Gradients.dRdX * Segment->LeftOffset;
							if (R<0) R=0; if (R>MAX_RGB_VALUE) R=MAX_RGB_VALUE;	
							G = Triangle.Left.G + Triangle.Gradients.dGdX * Segment->LeftOffset;
							if (G<0) G=0; if (G>MAX_RGB_VALUE) G=MAX_RGB_VALUE;	
							B = Triangle.Left.B + Triangle.Gradients.dBdX * Segment->LeftOffset;
							if (B<0) B=0; if (B>MAX_RGB_VALUE) B=MAX_RGB_VALUE;	

							#endif

							Triangle.SpanWidth = Segment->Width;
							#if SPANEDGES & LMAP
							if (!Triangle.IsLightMapSetup)
								TRaster_LightMapSetup();
							#endif
							TRaster_DrawSpan();
						}
				
				#else
					Triangle.DestBits = (DESTPIXEL *)Triangle.Left.Dest; 
					
					#if SPANEDGES & ZBUF
					Triangle.ZMapBits = (ZMAPPIXEL *)(Triangle.Left.Dest + Triangle.ZBufferAddressDelta);
					#endif

					#if SPANEDGES & TMAP
					UOverZ   = Triangle.Left.UOverZ;
					VOverZ   = Triangle.Left.VOverZ;
					#endif

					#if (SPANEDGES & TMAP) || (SPANEDGES & ZBUF)
					OneOverZ = Triangle.Left.OneOverZ;
					#endif

					#if (SPANEDGES & LSHADE)
					R = Triangle.Left.R;	
					if (R<0) R=0; if (R>MAX_RGB_VALUE) R=MAX_RGB_VALUE;	

					G = Triangle.Left.G;
					if (G<0) G=0; if (G>MAX_RGB_VALUE) G=MAX_RGB_VALUE;	

					B = Triangle.Left.B;
					if (B<0) B=0; if (B>MAX_RGB_VALUE) B=MAX_RGB_VALUE;	

					#endif

					#if SPANEDGES & LMAP
					if (!Triangle.IsLightMapSetup)
						TRaster_LightMapSetup();
					#endif
					TRaster_DrawSpan();
				#endif

			}

		// step left edge
		Triangle.Left.X += Triangle.Left.XStep;			  
		Triangle.Left.Dest += Triangle.Left.DestStep; 
		Triangle.Left.Height--; 
		Triangle.Left.ErrorTerm += Triangle.Left.Numerator;			

		#if SPANEDGES & TMAP
		Triangle.Left.UOverZ += Triangle.Left.UOverZStep;			
		Triangle.Left.VOverZ += Triangle.Left.VOverZStep;		
		#endif

		#if (SPANEDGES & TMAP) || (SPANEDGES & ZBUF)
		Triangle.Left.OneOverZ  += Triangle.Left.OneOverZStep; 
		#endif
	

		#if SPANEDGES & LSHADE
		Triangle.Left.R += Triangle.Left.RStep; 
		Triangle.Left.G += Triangle.Left.GStep; 
		Triangle.Left.B += Triangle.Left.BStep;
		#endif

		#if SPANEDGES & SBUF
		Triangle.Left.Y ++;
		#endif

		if (Triangle.Left.ErrorTerm >= Triangle.Left.Denominator) 
			{
				Triangle.Left.X++; 
				Triangle.Left.Dest+=sizeof(DESTPIXEL); 
				Triangle.Left.ErrorTerm -= Triangle.Left.Denominator; 

				#if SPANEDGES & TMAP
				Triangle.Left.UOverZ    += Triangle.Left.dUOverZdX;	
				Triangle.Left.VOverZ    += Triangle.Left.dVOverZdX;
				#endif

				#if (SPANEDGES & TMAP) || (SPANEDGES & ZBUF)
				Triangle.Left.OneOverZ  += Triangle.Left.dOneOverZdX;	
				#endif


				#if SPANEDGES & LSHADE
				Triangle.Left.R += Triangle.Left.dRdX; 
				Triangle.Left.G += Triangle.Left.dGdX; 
				Triangle.Left.B += Triangle.Left.dBdX;
				#endif
			}

		// step right edge
		Triangle.Right.X += Triangle.Right.XStep; 
		Triangle.Right.ErrorTerm += Triangle.Right.Numerator;
		if (Triangle.Right.ErrorTerm >= Triangle.Right.Denominator)																		\
			{	
				Triangle.Right.X++;  
				Triangle.Right.ErrorTerm -= Triangle.Right.Denominator;
			}													
	}
}


#undef SPANEDGES
