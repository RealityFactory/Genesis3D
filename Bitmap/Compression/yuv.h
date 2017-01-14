#ifndef GE_BRANDO_YUV_H
#define GE_BRANDO_YUV_H

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
/*  Genesis3D Version 1.1 released November 15, 1999                                 */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/

#include "basetype.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void RGBb_to_YUVb(const uint8 *RGB,uint8 *YUV);
extern void YUVb_to_RGBb(const uint8 *YUV,uint8 *RGB);
extern void RGBb_to_YUVb_line(const uint8 *RGB,uint8 *YUV,int array);
extern void YUVb_to_RGBb_line(const uint8 *YUV,uint8 *RGB,int array);

extern void RGBb_to_YUVi(const uint8 *RGB,int *Y,int *U,int *V);
extern void YUVi_to_RGBb(int y,int u,int v,uint8 *RGB);
extern void RGBi_to_YUVi(int R,int G,int B,int *Y,int *U,int *V);
extern void YUVi_to_RGBi(int y,int u,int v,int *R,int *G,int *B);

extern void YUVi_to_RGBi_line(int *line1,int *line2,int *line3,int len);
extern void YUVi_to_BGRb_line(int *line1,int *line2,int *line3,uint8 * bline,int len);

/**************************************************************/

#define YUV_SHIFT 	14
#define YUV_HALF	(1<<(YUV_SHIFT-1))
#define YUV_ONE		(1<<YUV_SHIFT)
#define Y_R   ((int)( 0.29900 * YUV_ONE ))
#define Y_G   ((int)( 0.58700 * YUV_ONE ))
#define Y_B   ((int)( 0.11400 * YUV_ONE ))
#define U_R   ((int)(-0.16874 * YUV_ONE ))
#define U_G   ((int)(-0.33126 * YUV_ONE ))
#define U_B   ((int)( 0.50000 * YUV_ONE ))
#define V_R   ((int)(-0.50000 * YUV_ONE ))	// ** important sign change of 'V' from jpeg default
#define V_G   ((int)( 0.41869 * YUV_ONE ))
#define V_B   ((int)( 0.08131 * YUV_ONE ))
#define R_Y   (    				YUV_ONE )       
#define R_U   (0)
#define R_V   ((int)(-1.40200 * YUV_ONE ))
#define G_Y   (    				YUV_ONE )       
#define G_U   ((int)(-0.34414 * YUV_ONE ))
#define G_V   ((int)( 0.71414 * YUV_ONE ))
#define B_Y   (     			YUV_ONE )       
#define B_U   ((int)( 1.77200 * YUV_ONE ))
#define B_V   (0)       

#define Y_RGB(R,G,B) (( Y_R * (R) + Y_G * (G) + Y_B * (B) + YUV_HALF ) >> YUV_SHIFT)
#define U_RGB(R,G,B) (( U_R * (R) + U_G * (G) + U_B * (B) + YUV_HALF ) >> YUV_SHIFT)
#define V_RGB(R,G,B) (( V_R * (R) + V_G * (G) + V_B * (B) + YUV_HALF ) >> YUV_SHIFT)
#define R_YUV(Y,U,V) (( R_Y * (Y) + R_U * (U) + R_V * (V) + YUV_HALF ) >> YUV_SHIFT)
#define G_YUV(Y,U,V) (( G_Y * (Y) + G_U * (U) + G_V * (V) + YUV_HALF ) >> YUV_SHIFT)
#define B_YUV(Y,U,V) (( B_Y * (Y) + B_U * (U) + B_V * (V) + YUV_HALF ) >> YUV_SHIFT)

#define RGB_to_YUV_macro(R,G,B,Y,U,V) \\
do { Y = Y_RGB(R,G,B); U = U_RGB(R,G,B) + 127; V = V_RGB(R,G,B) + 127; } while(0)

#define YUV_to_RGB_macro(Y,U,V,R,G,B) \\
do {	R = R_YUV(Y,(U)-127,(V)-127); G = G_YUV(Y,(U)-127,(V)-127); B = B_YUV(Y,(U)-127,(V)-127); \\
		R = minmax(R,0,255); G = minmax(G,0,255); B = minmax(B,0,255); } while(0)

/**************************************************************/

#ifdef __cplusplus
}
#endif

#endif
