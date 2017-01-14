/****************************************************************************************/
/*  Triangle.c                                                                          */
/*                                                                                      */
/*  Author: Mike Sandige	                                                            */
/*  Description:  Edge and Gradient calculations for triangle rasterizater              */
/*                                                                                      */
/*  Code fragments from Chris Hecker's texture mapping articles used with               */
/*  permission.  http://www.d6.com/users/checker                                        */
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
#if 0
// for debugging
#include <stdio.h>		//sprintf
#include <windows.h>	//OutputDebugString()
#endif

#include <assert.h>
#include <math.h>		// fabs
#include <string.h>		// memcpy
#include "swthandle.h"
#include "basetype.h"
#include "triangle.h"



#ifndef USE_DIBS
#define DEST565 
#endif





#define MAX(AA,BB)  ((AA)>(BB)?(AA):(BB))
#define MIN(AA,BB)  ((AA)<(BB)?(AA):(BB))


geBoolean GENESISCC Triangle_GradientsCompute( 
					Triangle_Gradients *G, 
					const DRV_TLVertex *pVertices, 
					float TextureWidth, 
					float TextureHeight)
{
	float OneOverdX;
	float OneOverdY;
	float Denominator;
	float Width02  = pVertices[0].x-pVertices[2].x;
	float Height02 = pVertices[0].y-pVertices[2].y;
	float Width12  = pVertices[1].x-pVertices[2].x;
	float Height12 = pVertices[1].y-pVertices[2].y;
	float d02,d12;
	float Size;
	assert( pVertices[0].z != 0.0f );
	assert( pVertices[1].z != 0.0f );
	assert( pVertices[2].z != 0.0f );
	
	Denominator = ( (Width12 * Height02) - (Width02 * Height12));


	if (Denominator == 0.0f)
		return GE_FALSE;

	OneOverdX = 1.0f / Denominator;

	OneOverdY = -OneOverdX;

	{
		float Right   = MAX(pVertices[0].x,MAX(pVertices[1].x,pVertices[2].x));
		float Left    = MIN(pVertices[0].x,MIN(pVertices[1].x,pVertices[2].x));
		float Top     = MIN(pVertices[0].y,MIN(pVertices[1].y,pVertices[2].y));
		float Bottom  = MAX(pVertices[0].y,MAX(pVertices[1].y,pVertices[2].y));
		Size = MAX(Right-Left,Bottom-Top);
	}

	if (Size < Triangle.MaxAffineSize)
		G->Affine = 1;
	else
		G->Affine = 0;


	if (Triangle.ROPFlags & (TMAP | ZBUF))
		{
			float zmax = MAX(pVertices[0].z,MAX(pVertices[1].z,pVertices[2].z));
			float zmin = MIN(pVertices[0].z,MIN(pVertices[1].z,pVertices[2].z));
			// G->FZScale is used to scale the range of all the interpolators to fit nicely 
			// in the predeterminted fixed point ranges.  These fixed point ranges are setup
			// to minimize visible errors.
			// The following code is unfortunate.  Due to the limited range of the fixed 
			// point math, the FZScale has a limited ability to scale everything else.
			// the following ranges are uesd to keep FZScale within known good boundries. 
			//   It's still possible to break the rasterizer by using Z values that are too 
			//   large or too small.
			if (zmax-zmin > 255.0f ) 
				{
					G->FZScale = zmin;
				}
			else
				{
					if (zmin<80.0f) 
						G->FZScale = zmin;
					else
						{
							if (zmax>500.0f) zmax=500.0f;
							G->FZScale = zmax;
						}
				}
			#if 0
				{// debugging code:
					char s[1000];
					sprintf(s,"z[0]=%f\t\tz[1]=%f\t\tz[2]=%f\t\tScale=%f\n",
						pVertices[0].z,pVertices[1].z,pVertices[2].z,G->FZScale);
					OutputDebugString(s);
				}
			#endif
			if (!G->Affine)
				{		
					G->OneOverZ[0] = G->FZScale/pVertices[0].z;
					G->OneOverZ[1] = G->FZScale/pVertices[1].z;
					G->OneOverZ[2] = G->FZScale/pVertices[2].z;
				}
			else
				{
					float OneOverZScale = 1.0f / G->FZScale;
					G->OneOverZ[0] = pVertices[0].z  * OneOverZScale;
					G->OneOverZ[1] = pVertices[1].z  * OneOverZScale;
					G->OneOverZ[2] = pVertices[2].z  * OneOverZScale;
				}

			d02 = G->OneOverZ[0] - G->OneOverZ[2];
			d12 = G->OneOverZ[1] - G->OneOverZ[2];
			G->FdOneOverZdX = OneOverdX *  ((d12 * Height02) - (d02 * Height12));
			G->dOneOverZdY = OneOverdY *  ((d12 * Width02 ) - (d02 * Width12 ));
		
			G->dOneOverZdX = FXFL_OOZ(G->FdOneOverZdX);
			G->ZScale      =  FXFL_Z(G->FZScale);
		}

	if (Triangle.ROPFlags & TMAP)		
		{
			G->UOverZ[0]   = ((pVertices[0].u * TextureWidth )+ 0.5f);
			G->VOverZ[0]   = ((pVertices[0].v * TextureHeight)+ 0.5f);
			G->UOverZ[1]   = ((pVertices[1].u * TextureWidth )+ 0.5f);
			G->VOverZ[1]   = ((pVertices[1].v * TextureHeight)+ 0.5f);
			G->UOverZ[2]   = ((pVertices[2].u * TextureWidth )+ 0.5f);
			G->VOverZ[2]   = ((pVertices[2].v * TextureHeight) + 0.5f);
			
			if (!G->Affine)
				{		
					G->UOverZ[0]   *=  G->OneOverZ[0];
					G->VOverZ[0]   *=  G->OneOverZ[0];
					G->UOverZ[1]   *=  G->OneOverZ[1];
					G->VOverZ[1]   *=  G->OneOverZ[1];
					G->UOverZ[2]   *=  G->OneOverZ[2];
					G->VOverZ[2]   *=  G->OneOverZ[2];
				}
		

			d02 = G->UOverZ[0] - G->UOverZ[2];
			d12 = G->UOverZ[1] - G->UOverZ[2];
			G->FdUOverZdX   = OneOverdX *  ((d12 * Height02) - (d02 * Height12));
			G->dUOverZdY   = OneOverdY *  ((d12 * Width02 ) - (d02 * Width12 ));

			d02 = G->VOverZ[0] - G->VOverZ[2];
			d12 = G->VOverZ[1] - G->VOverZ[2];
			G->FdVOverZdX   = OneOverdX *  ((d12 * Height02) - (d02 * Height12));
			G->dVOverZdY   = OneOverdY *  ((d12 * Width02 ) - (d02 * Width12));

			G->dUOverZdX   =  FXFL_OZ(G->FdUOverZdX);
			G->dVOverZdX   =  FXFL_OZ(G->FdVOverZdX);
		}
					

	if (Triangle.ROPFlags & LSHADE)
		{
			// can clamp these things higher to remove more small negative overruns.  
			d02 = (pVertices[0].r) - (pVertices[2].r);
			d12 = (pVertices[1].r) - (pVertices[2].r);
			G->FdRdX = OneOverdX * ((d12 * Height02) - (d02 * Height12));
			G->dRdY  = OneOverdY * ((d12 * Width02) - (d02 * Width12));

			d02 = (pVertices[0].g) - (pVertices[2].g);
			d12 = (pVertices[1].g) - (pVertices[2].g);
			G->FdGdX = OneOverdX * ((d12 * Height02) - (d02 * Height12));
			G->dGdY  = OneOverdY * ((d12 * Width02) - (d02 * Width12));

			d02 = (pVertices[0].b) - (pVertices[2].b);
			d12 = (pVertices[1].b) - (pVertices[2].b);
			G->FdBdX = OneOverdX * ((d12 * Height02) - (d02 * Height12));
			G->dBdY  = OneOverdY * ((d12 * Width02) - (d02 * Width12));

			G->dRdX = FXFL_RGB(G->FdRdX);
			G->dGdX = FXFL_RGB(G->FdGdX);
			G->dBdX = FXFL_RGB(G->FdBdX);
		}
	
	if (!G->Affine)
		{
			float ChangeIndicator = (float)fabs(G->FdOneOverZdX) + (float)fabs(G->dOneOverZdY);

			if (Triangle.ROPFlags & LMAP) 
				{
					// can maybe infer from the lightmap density what's best to do here.
					G->SubSpanWidth = 16;
					G->SubSpanShift  = 4;
				}
			else
				{
					if ( ChangeIndicator < 0.0005f) 
						{
							G->SubSpanWidth = 128;
							G->SubSpanShift  = 7;
						}
					else if ( ChangeIndicator < 0.001f) 
						{
							G->SubSpanWidth = 64;
							G->SubSpanShift  = 6;
						}
					else if ( ChangeIndicator < 0.005f)
						{
							G->SubSpanWidth = 32;
							G->SubSpanShift  = 5;
						}
					else
						{
							G->SubSpanWidth = 16;
							G->SubSpanShift  = 4;
						}
				}
		}
	else
		{
			G->dOneOverZdX = OOZ_FXP_TO_16_16(G->dOneOverZdX) * Z_FXP_TO_INT(G->ZScale);
			G->dUOverZdX   = OZ_FXP_TO_16_16 (G->dUOverZdX);
			G->dVOverZdX   = OZ_FXP_TO_16_16 (G->dVOverZdX);
		}

	return GE_TRUE;
}

void GENESISCC FloorDivMod( int32 Numerator, int32 Denominator, int32 *Floor,
				int32 *Mod )
{
	assert(Denominator > 0);		// we assume it's positive
	if(Numerator >= 0) 
		{
			// positive case, C is okay
			*Floor = Numerator / Denominator;
			*Mod   = Numerator % Denominator;
		} 
	else 
		{
			// Numerator is negative, do the right thing
			*Floor = -((-Numerator) / Denominator);
			*Mod   =   (-Numerator) % Denominator;
			if(*Mod) 
				{
					// there is a remainder
					*Floor = *Floor -1; 
					*Mod = Denominator - *Mod;
				}
		}
}

static int32 GENESISCC Ceil28_4( int32 Value ) 
{
	int32 ReturnValue;
	int32 Numerator = Value - 1 + 16;
	if(Numerator >= 0) 
		{
			ReturnValue = Numerator/16;
		} 
	else 
		{
			// deal with negative numerators correctly
			ReturnValue = -((-Numerator)/16);
			ReturnValue -= ((-Numerator) % 16) ? 1 : 0;
		}
	return ReturnValue;
}

void GENESISCC Triangle_EdgeCompute( 
		Triangle_Edge *E, 
		const Triangle_Gradients *Gradients, 
		const DRV_TLVertex *pVertices, 
		int Top, 
		int Bottom,
		int IsLeftEdge)
{
	int YEnd;
	int TopY,BottomY,TopX=0,BottomX;
	TopY    = (int32)(pVertices[Top].y * 16.0f);
	BottomY = (int32)(pVertices[Bottom].y * 16.0f); 
	E->Y = Ceil28_4( TopY );
	YEnd = Ceil28_4( BottomY );
	E->Height	= YEnd - E->Y;

	if (!E->Height)
		return;

	{
		int32 dN = BottomY-TopY;
		if (dN > 0)
			{
				int32 dM,InitialNumerator;
				//int32 dM   = (int32)(FWidth  * 16.0f);
				TopX	= (int32)(pVertices[Top].x * 16.0f);
				BottomX = (int32)(pVertices[Bottom].x * 16.0f);
				dM = BottomX - TopX;
			
				InitialNumerator = dM*16*E->Y - dM*TopY +	dN*TopX - 1 + dN*16;
				FloorDivMod(InitialNumerator,dN*16,&(E->X),&(E->ErrorTerm));
				FloorDivMod(dM*16,dN*16,&(E->XStep),&(E->Numerator));
				E->Denominator = dN*16;
			}
		else
			{
				E->XStep=0;
				E->X = (int)(pVertices[Top].x);
			}
	}

	if (IsLeftEdge)
		{
			float XPrestep		= E->X - (float)TopX * (1.0f/16.0f);
			float YPrestep		= E->Y - (float)TopY * (1.0f/16.0f);
			
			if (Triangle.ROPFlags & (TMAP | ZBUF))
				{
					E->OneOverZ		= FXFL_OOZ(Gradients->OneOverZ[Top] + YPrestep * Gradients->dOneOverZdY	+ XPrestep * Gradients->FdOneOverZdX);
					E->OneOverZStep	= FXFL_OOZ(E->XStep * Gradients->FdOneOverZdX	+ Gradients->dOneOverZdY);
					E->dOneOverZdX  = Gradients->dOneOverZdX;
				}

			if (Triangle.ROPFlags & TMAP)
				{
					E->UOverZ		= FXFL_OZ(Gradients->UOverZ[Top] 	+ YPrestep * Gradients->dUOverZdY	+ XPrestep * Gradients->FdUOverZdX);
					E->UOverZStep	= FXFL_OZ(E->XStep * Gradients->FdUOverZdX + Gradients->dUOverZdY);
					E->dUOverZdX    = Gradients->dUOverZdX;

					E->VOverZ		= FXFL_OZ(Gradients->VOverZ[Top] 	+ YPrestep * Gradients->dVOverZdY	+ XPrestep * Gradients->FdVOverZdX);
					E->VOverZStep	= FXFL_OZ(E->XStep * Gradients->FdVOverZdX + Gradients->dVOverZdY);
					E->dVOverZdX    = Gradients->dVOverZdX;
				}

			if (Triangle.ROPFlags & LSHADE)
				{
					E->R			= FXFL_RGB( (pVertices[Top].r) + 0.5f	+ YPrestep * Gradients->dRdY		+ XPrestep * Gradients->FdRdX);
					E->RStep		= FXFL_RGB(E->XStep * Gradients->FdRdX + Gradients->dRdY);
					E->dRdX			= Gradients->dRdX;

					E->G			= FXFL_RGB( (pVertices[Top].g) + 0.5f 	+ YPrestep * Gradients->dGdY		+ XPrestep * Gradients->FdGdX);
					E->GStep		= FXFL_RGB(E->XStep * Gradients->FdGdX + Gradients->dGdY);
					E->dGdX			= Gradients->dGdX;

					E->B			= FXFL_RGB( (pVertices[Top].b) + 0.5f 	+ YPrestep * Gradients->dBdY		+ XPrestep * Gradients->FdBdX);
					E->BStep		= FXFL_RGB(E->XStep * Gradients->FdBdX + Gradients->dBdY);
					E->dBdX			= Gradients->dBdX;
				}

			if (Gradients->Affine)
				{
					if (Triangle.ROPFlags & (TMAP | ZBUF))
						{
							E->OneOverZ     = OOZ_FXP_TO_16_16(	E->OneOverZ		)  * Z_FXP_TO_INT(Gradients->ZScale);
							E->OneOverZStep = OOZ_FXP_TO_16_16(	E->OneOverZStep	)  * Z_FXP_TO_INT(Gradients->ZScale);
						}
					if (Triangle.ROPFlags & TMAP)
						{
							E->UOverZ		= OZ_FXP_TO_16_16 (	E->UOverZ		);
							E->UOverZStep	= OZ_FXP_TO_16_16 (	E->UOverZStep	);
							E->VOverZ		= OZ_FXP_TO_16_16 (	E->VOverZ		);
							E->VOverZStep	= OZ_FXP_TO_16_16 (	E->VOverZStep	);
						}
				} 	
		}
}




