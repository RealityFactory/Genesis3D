/****************************************************************************************/
/*  ROP.H                                                                               */
/*                                                                                      */
/*  Author: Mike Sandige                                                                */
/*  Description:  This defines the available rops for the software driver triangle      */
/*                rasterizer.                                                           */
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
#ifndef ROP_H
#define ROP_H

#ifdef __cplusplus
extern "C" {
#endif

	
//	light = gourad,map,flat,none(fullbright)
//  alpha = none, map(only with a texture), flat
typedef enum {
			 
//ROP ID								//	 texture   light  alpha		z		z	  	span	span	priority
										//								test	set		test	set							  
GE_ROP_LSHADE,  						//	|	-	|	g	|	-	|	-	|	-	|	-	|	-	|	m		
GE_ROP_LSHADE_ZSET,  					//	|	-	|	g	|	-	|	-	|	+	|	-	|	-	|	m		
GE_ROP_LSHADE_ZTEST,  					//	|	-	|	g	|	-	|	+	|	-	|	-	|	-	|	m		
GE_ROP_LSHADE_ZTESTSET,  				//	|	-	|	g	|	-	|	+	|	+	|	-	|	-	|	h		
GE_ROP_LSHADE_AFLAT,					//	|	-	|	g	|	f	|	-	|	-	|	-	|	-	|	l		
GE_ROP_LSHADE_AFLAT_ZSET,				//	|	-	|	g	|	f	|	-	|	+	|	-	|	-	|	l		
GE_ROP_LSHADE_AFLAT_ZTEST,				//	|	-	|	g	|	f	|	+	|	-	|	-	|	-	|	l		
GE_ROP_LSHADE_AFLAT_ZTESTSET,			//	|	-	|	g	|	f	|	+	|	+	|	-	|	-	|	l		
GE_ROP_TMAP_LSHADE,  					//	|	+	|	g	|	-	|	-	|	-	|	-	|	-	|	m		
GE_ROP_TMAP_LSHADE_ZSET,				//	|	+	|	g	|	-	|	-	|	+	|	-	|	-	|	m		
GE_ROP_TMAP_LSHADE_ZTEST,  				//	|	+	|	g	|	-	|	+	|	-	|	-	|	-	|	m		
GE_ROP_TMAP_LSHADE_ZTESTSET,			//	|	+	|	g	|	-	|	+	|	+	|	-	|	-	|	h		
GE_ROP_TMAP_LMAP_ZSET_SBUF,  			//	|	+	|	m	|	-	|	-	|	+	|	+	|	+	|	h		
GE_ROP_TMAP_LSHADE_ZSET_SBUF,			//	|	+	|	g	|	-	|	-	|	+	|	+	|	+	|	h		
GE_ROP_TMAP_LMAP_ZTESTSET,				//	|	+	|	m	|	-	|	+	|	+	|	-	|	-	|	h		
GE_ROP_TMAP_LSHADE_AFLAT,				//	|	+	|	g	|	f	|	-	|	-	|	-	|	-	|	l		
GE_ROP_TMAP_LSHADE_AFLAT_ZSET,			//	|	+	|	g	|	f	|	-	|	+	|	-	|	-	|	l		
GE_ROP_TMAP_LSHADE_AFLAT_ZTEST,			//	|	+	|	g	|	f	|	+	|	-	|	-	|	-	|	l		
GE_ROP_TMAP_LSHADE_AFLAT_ZTESTSET,		//	|	+	|	g	|	f	|	+	|	+	|	-	|	-	|	l		
GE_ROP_TMAP_LSHADE_AMAP,				//	|	+	|	g	|	m	|	-	|	-	|	-	|	-	|	m		
GE_ROP_TMAP_LSHADE_AMAP_ZSET,			//	|	+	|	g	|	m	|	-	|	+	|	-	|	-	|	m		
GE_ROP_TMAP_LSHADE_AMAP_ZTEST,			//	|	+	|	g	|	m	|	+	|	-	|	-	|	-	|	m		
GE_ROP_TMAP_LSHADE_AMAP_ZTESTSET,		//	|	+	|	g	|	m	|	+	|	+	|	-	|	-	|	m		
GE_ROP_TMAP_LMAP_AMAP,					//	|	+	|	m	|	m	|	-	|	-	|	-	|	-	|	l		
GE_ROP_TMAP_LMAP_AMAP_ZSET,				//	|	+	|	m	|	m	|	-	|	+	|	-	|	-	|	l		
GE_ROP_TMAP_LMAP_AMAP_ZTEST,			//	|	+	|	m	|	m	|	+	|	-	|	-	|	-	|	l		
GE_ROP_TMAP_LMAP_AMAP_ZTESTSET,			//	|	+	|	m	|	m	|	+	|	+	|	-	|	-	|	l		
GE_ROP_TMAP_LMAP_AFLAT_ZTESTSET,		//	|	+	|	m	|	f	|	+	|	+	|	-	|	-	|	h		
GE_ROP_TMAP_LSHADE_AMAP_AFLAT,			//	|	+	|	g	|	mf	|	-	|	-	|	-	|	-	|	m		
GE_ROP_TMAP_LSHADE_AMAP_AFLAT_ZSET,		//	|	+	|	g	|	mf	|	-	|	+	|	-	|	-	|	m		
GE_ROP_TMAP_LSHADE_AMAP_AFLAT_ZTEST,	//	|	+	|	g	|	mf	|	+	|	-	|	-	|	-	|	m		
GE_ROP_TMAP_LSHADE_AMAP_AFLAT_ZTESTSET,	//	|	+	|	g	|	mf	|	+	|	+	|	-	|	-	|	m		
GE_ROP_END,
} geROP;


#ifdef __cplusplus
}
#endif


#endif