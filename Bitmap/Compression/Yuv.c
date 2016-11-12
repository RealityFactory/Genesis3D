#include "yuv.h"
#include "utility.h"
#include <Assert.h>

/****************************************************************************************/
/*  Yuv                                                                                 */
/*                                                                                      */
/*  Author: Charles Bloom                                                               */
/*  Description:  YUV <-> RGB code                                                      */
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

#pragma warning(disable : 4244)

/**************** the YUV routines : ******************/

void RGBb_to_YUVb(const uint8 *RGB,uint8 *YUV)
{
int R = RGB[0], G = RGB[1], B = RGB[2];

	YUV[0] = Y_RGB(R,G,B);
	YUV[1] = U_RGB(R,G,B) + 127;
	YUV[2] = V_RGB(R,G,B) + 127;
}

void YUVb_to_RGBb(const uint8 *YUV,uint8 *RGB)
{
int y,u,v,r,g,b;

	y = YUV[0];
	u = YUV[1] - 127;
	v = YUV[2] - 127;

	r = R_YUV(y,u,v);
	g = G_YUV(y,u,v);
	b = B_YUV(y,u,v);

	RGB[0] = minmax(r,0,255);	// we could get negative ones and whatnot
	RGB[1] = minmax(g,0,255);	//	because the y,u,v are not really 24 bits;
	RGB[2] = minmax(b,0,255);	//	there are regions of YUV space that will never be reached by RGBb_to_YUVb
}


void RGBb_to_YUVb_line(const uint8 *RGB,uint8 *YUV,int len)
{
int R,G,B;

	while(len--)
	{
		R = *RGB++;
		G = *RGB++;
		B = *RGB++;
		*YUV++ = Y_RGB(R,G,B);
		*YUV++ = U_RGB(R,G,B) + 127;
		*YUV++ = V_RGB(R,G,B) + 127;
	}
}

void YUVb_to_RGBb_line(const uint8 *YUV,uint8 *RGB,int len)
{
int y,u,v,r,g,b;

	while(len--)
	{
		y = (*YUV++);
		u = (*YUV++) - 127;
		v = (*YUV++) - 127;

		r = R_YUV(y,u,v);
		g = G_YUV(y,u,v);
		b = B_YUV(y,u,v);

		*RGB++ = minmax(r,0,255);	// we could get negative ones and whatnot
		*RGB++ = minmax(g,0,255);	//	because the y,u,v are not really 24 bits;
		*RGB++ = minmax(b,0,255);	//	there are regions of YUV space that will never be reached by RGBb_to_YUVb
	}
}


void RGBb_to_YUVi(const uint8 *RGB,int *Y,int *U,int *V)
{
int R = RGB[0], G = RGB[1], B = RGB[2];

	*Y = Y_RGB(R,G,B);
	*U = U_RGB(R,G,B) + 127;
	*V = V_RGB(R,G,B) + 127;

	assert( isinrange(*Y,0,255) );
	assert( isinrange(*U,0,255) );
	assert( isinrange(*V,0,255) );
}

void YUVi_to_RGBb(int y,int u,int v,uint8 *RGB)
{
int r,g,b;

// yuv can be kicked out of 0,255 by the wavelet
//	assert( isinrange(y,0,255) );
//	assert( isinrange(u,0,255) );
//	assert( isinrange(v,0,255) );

	u -= 127;
	v -= 127;
	r = R_YUV(y,u,v); // this is just like a matrix multiply
	g = G_YUV(y,u,v);
	b = B_YUV(y,u,v);
	RGB[0] = minmax(r,0,255);	// we could get negative ones and whatnot
	RGB[1] = minmax(g,0,255);	//	because the y,u,v are not really 24 bits;
	RGB[2] = minmax(b,0,255);	//	there are regions of YUV space that will never be reached by RGBb_to_YUVb
}

void RGBi_to_YUVi(int R,int G,int B,int *Y,int *U,int *V)
{
	assert( isinrange(R,0,255) );
	assert( isinrange(G,0,255) );
	assert( isinrange(B,0,255) );

	*Y = Y_RGB(R,G,B);
	*U = U_RGB(R,G,B) + 127;
	*V = V_RGB(R,G,B) + 127;

	assert( isinrange(*Y,0,255) );
	assert( isinrange(*U,0,255) );
	assert( isinrange(*V,0,255) );
}

void YUVi_to_RGBi(int y,int u,int v,int *R,int *G,int *B)
{
int r,g,b;

// yuv can be kicked out of 0,255 by the wavelet
//	assert( isinrange(y,0,255) );
//	assert( isinrange(u,0,255) );
//	assert( isinrange(v,0,255) );

	u -= 127;
	v -= 127;
	r = R_YUV(y,u,v); // this is just like a matrix multiply
	g = G_YUV(y,u,v);
	b = B_YUV(y,u,v);

	*R = minmax(r,0,255);	// we could get negative ones and whatnot
	*G = minmax(g,0,255);	//	because the y,u,v are not really 24 bits;
	*B = minmax(b,0,255);	//	there are regions of YUV space that will never be reached by RGBb_to_YUVb
	// <> MMX does this floor to 0-255 for us in one bang!
}

void YUVi_to_RGBi_line(int *line1,int *line2,int *line3,int len)
{
int y,u,v,r,g,b;

	// <> use MMX

	while(len--)
	{
		y = *line1;
		u = *line2 - 127;
		v = *line3 - 127;

		r = R_YUV(y,u,v);
		g = G_YUV(y,u,v);
		b = B_YUV(y,u,v);

		r = minmax(r,0,255);
		g = minmax(g,0,255);
		b = minmax(b,0,255);

		*line1++ = r;
		*line2++ = g;
		*line3++ = b;
	}
}

void YUVi_to_BGRb_line(int *iline1,int *iline2,int *iline3,uint8 * ibline,int ilen)
{
int y,u,v,r,g,b,len;
int *line1,*line2,*line3;
uint8 * bline;

	line1 = iline1;
	line2 = iline2;
	line3 = iline3;
	bline = ibline;
	len = ilen;

	while(len--)
	{
		y = (*line1++);
		u = (*line2++) - 127;
		v = (*line3++) - 127;

		r = R_YUV(y,u,v);
		g = G_YUV(y,u,v);
		b = B_YUV(y,u,v);

		r = minmax(r,0,255);
		g = minmax(g,0,255);
		b = minmax(b,0,255);

		bline[0] = b;
		bline[1] = g;
		bline[2] = r;
		bline+=3;
	}
}
