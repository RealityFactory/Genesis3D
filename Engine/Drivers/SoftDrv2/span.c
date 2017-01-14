/****************************************************************************************/
/*  Span.C                                                                              */
/*                                                                                      */
/*  Author: Mike Sandige                                                                */
/*  Description:  Span abstracts and contains all the various ROP functions.            */
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
#include <assert.h>
#include "span.h"
#include "Triangle.h"


int32 URight, VRight, RRight,GRight,BRight;	// Globals for optimal access and sharing 
											// between the lighting sampler and the rop 
											// span functions

void GENESISCC Span_LightMapSample(void);


typedef struct
{
	geROP ROP;

	Span_DrawFunction Active;
	Span_DrawFunction Function[GE_SPAN_HARDWARE_VERSIONS][GE_SPAN_DESTINATION_FORMATS];
} Span_FunctionTableEntry;


void GENESISCC Span_C_TMAP_LMAP_Z1(void);

#define SPANROP LSHADE
		void GENESISCC Span_C_LSHADE_555(void) {
				#include "Span_Factory.h"
				}
		#define SPANROP LSHADE + D565
		void GENESISCC Span_C_LSHADE_565(void) {
				#include "Span_Factory.h"
				}

#define SPANROP LSHADE + ZSET 
		void GENESISCC Span_C_LSHADE_ZSET_555(void)	{
				#include "Span_Factory.h"
				}
		#define SPANROP LSHADE + ZSET + D565
		void GENESISCC Span_C_LSHADE_ZSET_565(void) {
				#include "Span_Factory.h"
				}

#define SPANROP LSHADE + ZTEST 
		void GENESISCC Span_C_LSHADE_ZTEST_555(void) {
				#include "Span_Factory.h"
				}
		#define SPANROP LSHADE + ZTEST + D565
		void GENESISCC Span_C_LSHADE_ZTEST_565(void) {
				#include "Span_Factory.h"
				}

#define SPANROP LSHADE + ZTEST + ZSET
		void GENESISCC Span_C_LSHADE_ZTEST_ZSET_555(void) {
				#include "Span_Factory.h"
				}
		#define SPANROP LSHADE + ZTEST + ZSET + D565
		void GENESISCC Span_C_LSHADE_ZTEST_ZSET_565(void) {
				#include "Span_Factory.h"
				}

#define SPANROP LSHADE + AFLAT
		void GENESISCC Span_C_LSHADE_AFLAT_555(void) {
				#include "Span_Factory.h"
				}
		#define SPANROP LSHADE + AFLAT + D565
		void GENESISCC Span_C_LSHADE_AFLAT_565(void) {
				#include "Span_Factory.h"
				}

#define SPANROP LSHADE + AFLAT + ZSET
		void GENESISCC Span_C_LSHADE_AFLAT_ZSET_555(void) {
				#include "Span_Factory.h"
				}
		#define SPANROP LSHADE + AFLAT + ZSET + D565
		void GENESISCC Span_C_LSHADE_AFLAT_ZSET_565(void) {
				#include "Span_Factory.h"
				}

#define SPANROP LSHADE + AFLAT + ZTEST
		void GENESISCC Span_C_LSHADE_AFLAT_ZTEST_555(void) {
				#include "Span_Factory.h"
				}
		#define SPANROP LSHADE + AFLAT + ZTEST + D565
		void GENESISCC Span_C_LSHADE_AFLAT_ZTEST_565(void) {
				#include "Span_Factory.h"
				}

#define SPANROP LSHADE + AFLAT + ZTEST + ZSET
		void GENESISCC Span_C_LSHADE_AFLAT_ZTEST_ZSET_555(void) {
				#include "Span_Factory.h"
				}
		#define SPANROP LSHADE + AFLAT + ZTEST + ZSET + D565
		void GENESISCC Span_C_LSHADE_AFLAT_ZTEST_ZSET_565(void) {
				#include "Span_Factory.h"
				}

#define SPANROP TMAP + LSHADE 
		void GENESISCC Span_C_TMAP_LSHADE_555(void) {
				#include "Span_Factory.h"
				}
		#define SPANROP TMAP + LSHADE + D565
		void GENESISCC Span_C_TMAP_LSHADE_565(void) {
				#include "Span_Factory.h"
				}

#define SPANROP TMAP + LSHADE + ZSET
		void GENESISCC Span_C_TMAP_LSHADE_ZSET_555(void) {
				#include "Span_Factory.h"
				}
		#define SPANROP TMAP + LSHADE + ZSET + D565
		void GENESISCC Span_C_TMAP_LSHADE_ZSET_565(void) {
				#include "Span_Factory.h"
				}

#define SPANROP TMAP + LSHADE + ZTEST
		void GENESISCC Span_C_TMAP_LSHADE_ZTEST_555(void) {
				#include "Span_Factory.h"
				}
		#define SPANROP TMAP + LSHADE + ZTEST + D565
		void GENESISCC Span_C_TMAP_LSHADE_ZTEST_565(void) {
				#include "Span_Factory.h"
				}

#define SPANROP TMAP + LSHADE + ZTEST + ZSET
		void GENESISCC Span_C_TMAP_LSHADE_ZTEST_ZSET_555(void) {
				#include "Span_Factory.h"
				}
		#define SPANROP TMAP + LSHADE + ZTEST + ZSET + D565
		void GENESISCC Span_C_TMAP_LSHADE_ZTEST_ZSET_565(void) {
				#include "Span_Factory.h"
				}

#define SPANROP TMAP + LMAP + ZSET 
		void GENESISCC Span_C_TMAP_LMAP_ZSET_555(void) {
				#include "Span_Factory.h"
				}
		#define SPANROP TMAP + LMAP + ZSET + D565
		void GENESISCC Span_C_TMAP_LMAP_ZSET_565(void) {
				#include "Span_Factory.h"
				}

#define SPANROP TMAP + LMAP + ZTEST + ZSET 
		void GENESISCC Span_C_TMAP_LMAP_ZTEST_ZSET_555(void) {
				#include "Span_Factory.h"
				}
		#define SPANROP TMAP + LMAP + ZTEST + ZSET + D565
		void GENESISCC Span_C_TMAP_LMAP_ZTEST_ZSET_565(void) {
				#include "Span_Factory.h"
				}

#define SPANROP TMAP + LSHADE + AFLAT
		void GENESISCC Span_C_TMAP_LSHADE_AFLAT_555(void) {
				#include "Span_Factory.h"
				}
		#define SPANROP TMAP + LSHADE + AFLAT + D565
		void GENESISCC Span_C_TMAP_LSHADE_AFLAT_565(void) {
				#include "Span_Factory.h"
				}

#define SPANROP TMAP + LSHADE + AFLAT + ZSET
		void GENESISCC Span_C_TMAP_LSHADE_AFLAT_ZSET_555(void) {
				#include "Span_Factory.h"
				}
		#define SPANROP TMAP + LSHADE + AFLAT + ZSET + D565 
		void GENESISCC Span_C_TMAP_LSHADE_AFLAT_ZSET_565(void) {
				#include "Span_Factory.h"
				}

#define SPANROP TMAP + LSHADE + AFLAT + ZTEST
		void GENESISCC Span_C_TMAP_LSHADE_AFLAT_ZTEST_555(void) {
				#include "Span_Factory.h"
				}
		#define SPANROP TMAP + LSHADE + AFLAT + ZTEST + D565 
		void GENESISCC Span_C_TMAP_LSHADE_AFLAT_ZTEST_565(void) {
				#include "Span_Factory.h"
				}

#define SPANROP TMAP + LSHADE + AFLAT + ZTEST + ZSET
		void GENESISCC Span_C_TMAP_LSHADE_AFLAT_ZTEST_ZSET_555(void) {
				#include "Span_Factory.h"
				}
		#define SPANROP TMAP + LSHADE + AFLAT + ZTEST + ZSET + D565 
		void GENESISCC Span_C_TMAP_LSHADE_AFLAT_ZTEST_ZSET_565(void) {
				#include "Span_Factory.h"
				}

#define SPANROP TMAP + LSHADE + AMAP 
		void GENESISCC Span_C_TMAP_LSHADE_AMAP_555(void) {
				#include "Span_Factory.h"
				}
		#define SPANROP TMAP + LSHADE + AMAP + D565
		void GENESISCC Span_C_TMAP_LSHADE_AMAP_565(void) {
				#include "Span_Factory.h"
				}

#define SPANROP TMAP + LSHADE + AMAP + ZSET
		void GENESISCC Span_C_TMAP_LSHADE_AMAP_ZSET_555(void) {
				#include "Span_Factory.h"
				}
		#define SPANROP TMAP + LSHADE + AMAP + ZSET + D565
		void GENESISCC Span_C_TMAP_LSHADE_AMAP_ZSET_565(void) {
				#include "Span_Factory.h"
				}

#define SPANROP TMAP + LSHADE + AMAP + ZTEST
		void GENESISCC Span_C_TMAP_LSHADE_AMAP_ZTEST_555(void) {
				#include "Span_Factory.h"
				}
		#define SPANROP TMAP + LSHADE + AMAP + ZTEST + D565
		void GENESISCC Span_C_TMAP_LSHADE_AMAP_ZTEST_565(void) {
				#include "Span_Factory.h"
				}

#define SPANROP TMAP + LSHADE + AMAP + ZTEST + ZSET
		void GENESISCC Span_C_TMAP_LSHADE_AMAP_ZTEST_ZSET_555(void) {
				#include "Span_Factory.h"
				}
		#define SPANROP TMAP + LSHADE + AMAP + ZTEST + ZSET + D565
		void GENESISCC Span_C_TMAP_LSHADE_AMAP_ZTEST_ZSET_565(void) {
				#include "Span_Factory.h"
				}

#define SPANROP TMAP + LMAP + AMAP 
		void GENESISCC Span_C_TMAP_LMAP_AMAP_555(void) {
				#include "Span_Factory.h"
				}
		#define SPANROP TMAP + LMAP + AMAP + D565
		void GENESISCC Span_C_TMAP_LMAP_AMAP_565(void) {
				#include "Span_Factory.h"
				}

#define SPANROP TMAP + LMAP + AMAP + ZSET
		void GENESISCC Span_C_TMAP_LMAP_AMAP_ZSET_555(void) {
				#include "Span_Factory.h"
				}
		#define SPANROP TMAP + LMAP + AMAP + ZSET + D565
		void GENESISCC Span_C_TMAP_LMAP_AMAP_ZSET_565(void) {
				#include "Span_Factory.h"
				}

#define SPANROP TMAP + LMAP + AMAP + ZTEST
		void GENESISCC Span_C_TMAP_LMAP_AMAP_ZTEST_555(void) {
				#include "Span_Factory.h"
				}
		#define SPANROP TMAP + LMAP + AMAP + ZTEST + D565
		void GENESISCC Span_C_TMAP_LMAP_AMAP_ZTEST_565(void) {
				#include "Span_Factory.h"
				}

#define SPANROP TMAP + LMAP + AMAP + ZTEST + ZSET
		void GENESISCC Span_C_TMAP_LMAP_AMAP_ZTEST_ZSET_555(void) {
				#include "Span_Factory.h"
				}
		#define SPANROP TMAP + LMAP + AMAP + ZTEST + ZSET + D565
		void GENESISCC Span_C_TMAP_LMAP_AMAP_ZTEST_ZSET_565(void) {
				#include "Span_Factory.h"
				}

#define SPANROP TMAP + LMAP + AFLAT + ZTEST + ZSET
		void GENESISCC Span_C_TMAP_LMAP_AFLAT_ZTEST_ZSET_555(void) {
				#include "Span_Factory.h"
				}
		#define SPANROP TMAP + LMAP + AFLAT + ZTEST + ZSET + D565
		void GENESISCC Span_C_TMAP_LMAP_AFLAT_ZTEST_ZSET_565(void) {
				#include "Span_Factory.h"
				}

#define SPANROP TMAP + LSHADE + AMAP + AFLAT 
		void GENESISCC Span_C_TMAP_LSHADE_AMAP_AFLAT_555(void) {
				#include "Span_Factory.h"
				}
		#define SPANROP TMAP + LSHADE + AMAP + AFLAT + D565
		void GENESISCC Span_C_TMAP_LSHADE_AMAP_AFLAT_565(void) {
				#include "Span_Factory.h"
				}

#define SPANROP TMAP + LSHADE + AMAP + AFLAT + ZSET 
		void GENESISCC Span_C_TMAP_LSHADE_AMAP_AFLAT_ZSET_555(void) {
				#include "Span_Factory.h"
				}
		#define SPANROP TMAP + LSHADE + AMAP + AFLAT + ZSET + D565
		void GENESISCC Span_C_TMAP_LSHADE_AMAP_AFLAT_ZSET_565(void) {
				#include "Span_Factory.h"
				}

#define SPANROP TMAP + LSHADE + AMAP + AFLAT + ZTEST
		void GENESISCC Span_C_TMAP_LSHADE_AMAP_AFLAT_ZTEST_555(void) {
				#include "Span_Factory.h"
				}
		#define SPANROP TMAP + LSHADE + AMAP + AFLAT + ZTEST + D565
		void GENESISCC Span_C_TMAP_LSHADE_AMAP_AFLAT_ZTEST_565(void) {
				#include "Span_Factory.h"
				}

#define SPANROP TMAP + LSHADE + AMAP + AFLAT + ZTEST + ZSET
		void GENESISCC Span_C_TMAP_LSHADE_AMAP_AFLAT_ZTEST_ZSET_555(void) {
				#include "Span_Factory.h"
				}
		#define SPANROP TMAP + LSHADE + AMAP + AFLAT + ZTEST + ZSET + D565
		void GENESISCC Span_C_TMAP_LSHADE_AMAP_AFLAT_ZTEST_ZSET_565(void) {
				#include "Span_Factory.h"
				}


Span_FunctionTableEntry Span_FunctionTable[GE_ROP_END] =
{//ROP ID						
{GE_ROP_LSHADE,	  					NULL,{{Span_C_LSHADE_555,Span_C_LSHADE_565},{NULL,NULL},{NULL,NULL}} },
{GE_ROP_LSHADE_ZSET,  				NULL,{{Span_C_LSHADE_ZSET_555,Span_C_LSHADE_ZSET_565},{NULL,NULL},{NULL,NULL}} },
{GE_ROP_LSHADE_ZTEST,  				NULL,{{Span_C_LSHADE_ZTEST_555,Span_C_LSHADE_ZTEST_565},{NULL,NULL},{NULL,NULL}} },	
{GE_ROP_LSHADE_ZTESTSET,  			NULL,{{Span_C_LSHADE_ZTEST_ZSET_555,Span_C_LSHADE_ZTEST_ZSET_565},{NULL,NULL},{NULL,NULL}} },	
{GE_ROP_LSHADE_AFLAT,				NULL,{{Span_C_LSHADE_AFLAT_555,Span_C_LSHADE_AFLAT_565},{NULL,NULL},{NULL,NULL}} },
{GE_ROP_LSHADE_AFLAT_ZSET,			NULL,{{Span_C_LSHADE_AFLAT_ZSET_555,Span_C_LSHADE_AFLAT_ZSET_565},{NULL,NULL},{NULL,NULL}} },
{GE_ROP_LSHADE_AFLAT_ZTEST,			NULL,{{Span_C_LSHADE_AFLAT_ZTEST_555,Span_C_LSHADE_AFLAT_ZTEST_565},{NULL,NULL},{NULL,NULL}} },
{GE_ROP_LSHADE_AFLAT_ZTESTSET,		NULL,{{Span_C_LSHADE_AFLAT_ZTEST_ZSET_555,Span_C_LSHADE_AFLAT_ZTEST_ZSET_565},{NULL,NULL},{NULL,NULL}} },
{GE_ROP_TMAP_LSHADE,  				NULL,{{Span_C_TMAP_LSHADE_555,Span_C_TMAP_LSHADE_565},{NULL,NULL},{NULL,NULL}} },
{GE_ROP_TMAP_LSHADE_ZSET,			NULL,{{Span_C_TMAP_LSHADE_ZSET_555,Span_C_TMAP_LSHADE_ZSET_565},{NULL,NULL},{NULL,NULL}} },
{GE_ROP_TMAP_LSHADE_ZTEST,  		NULL,{{Span_C_TMAP_LSHADE_ZTEST_555,Span_C_TMAP_LSHADE_ZTEST_565},{NULL,NULL},{NULL,NULL}} },
{GE_ROP_TMAP_LSHADE_ZTESTSET,		NULL,{{Span_C_TMAP_LSHADE_ZTEST_ZSET_555,Span_C_TMAP_LSHADE_ZTEST_ZSET_565},{NULL,NULL},{NULL,NULL}} },
{GE_ROP_TMAP_LMAP_ZSET_SBUF,  		NULL,{{Span_C_TMAP_LMAP_ZSET_555,Span_C_TMAP_LMAP_ZSET_565},{NULL,NULL},{NULL,NULL}} },
{GE_ROP_TMAP_LSHADE_ZSET_SBUF,		NULL,{{Span_C_TMAP_LSHADE_ZSET_555,Span_C_TMAP_LSHADE_ZSET_565},{NULL,NULL},{NULL,NULL}} },
{GE_ROP_TMAP_LMAP_ZTESTSET,	  		NULL,{{Span_C_TMAP_LMAP_ZTEST_ZSET_555,Span_C_TMAP_LMAP_ZTEST_ZSET_565},{NULL,NULL},{NULL,NULL}} },
{GE_ROP_TMAP_LSHADE_AFLAT,			NULL,{{Span_C_TMAP_LSHADE_AFLAT_555,Span_C_TMAP_LSHADE_AFLAT_565},{NULL,NULL},{NULL,NULL}} },
{GE_ROP_TMAP_LSHADE_AFLAT_ZSET,		NULL,{{Span_C_TMAP_LSHADE_AFLAT_ZSET_555,Span_C_TMAP_LSHADE_AFLAT_ZSET_565},{NULL,NULL},{NULL,NULL}} },	
{GE_ROP_TMAP_LSHADE_AFLAT_ZTEST,	NULL,{{Span_C_TMAP_LSHADE_AFLAT_ZTEST_555,Span_C_TMAP_LSHADE_AFLAT_ZTEST_565},{NULL,NULL},{NULL,NULL}} },	
{GE_ROP_TMAP_LSHADE_AFLAT_ZTESTSET,	NULL,{{Span_C_TMAP_LSHADE_AFLAT_ZTEST_ZSET_555,Span_C_TMAP_LSHADE_AFLAT_ZTEST_ZSET_565},{NULL,NULL},{NULL,NULL}} },	
{GE_ROP_TMAP_LSHADE_AMAP,			NULL,{{Span_C_TMAP_LSHADE_AMAP_555,Span_C_TMAP_LSHADE_AMAP_565},{NULL,NULL},{NULL,NULL}} },	
{GE_ROP_TMAP_LSHADE_AMAP_ZSET,		NULL,{{Span_C_TMAP_LSHADE_AMAP_ZSET_555,Span_C_TMAP_LSHADE_AMAP_ZSET_565},{NULL,NULL},{NULL,NULL}} },	
{GE_ROP_TMAP_LSHADE_AMAP_ZTEST,		NULL,{{Span_C_TMAP_LSHADE_AMAP_ZTEST_555,Span_C_TMAP_LSHADE_AMAP_ZTEST_565},{NULL,NULL},{NULL,NULL}} },	
{GE_ROP_TMAP_LSHADE_AMAP_ZTESTSET,	NULL,{{Span_C_TMAP_LSHADE_AMAP_ZTEST_ZSET_555,Span_C_TMAP_LSHADE_AMAP_ZTEST_ZSET_565},{NULL,NULL},{NULL,NULL}} },	
{GE_ROP_TMAP_LMAP_AMAP,				NULL,{{Span_C_TMAP_LMAP_AMAP_555,Span_C_TMAP_LMAP_AMAP_565},{NULL,NULL},{NULL,NULL}} },	
{GE_ROP_TMAP_LMAP_AMAP_ZSET,		NULL,{{Span_C_TMAP_LMAP_AMAP_ZSET_555,Span_C_TMAP_LMAP_AMAP_ZSET_565},{NULL,NULL},{NULL,NULL}} },	
{GE_ROP_TMAP_LMAP_AMAP_ZTEST,		NULL,{{Span_C_TMAP_LMAP_AMAP_ZTEST_555,Span_C_TMAP_LMAP_AMAP_ZTEST_565},{NULL,NULL},{NULL,NULL}} },	
{GE_ROP_TMAP_LMAP_AMAP_ZTESTSET,	NULL,{{Span_C_TMAP_LMAP_AMAP_ZTEST_ZSET_555,Span_C_TMAP_LMAP_AMAP_ZTEST_ZSET_565},{NULL,NULL},{NULL,NULL}} },	
{GE_ROP_TMAP_LMAP_AFLAT_ZTESTSET,	NULL,{{Span_C_TMAP_LMAP_AFLAT_ZTEST_ZSET_555,Span_C_TMAP_LMAP_AFLAT_ZTEST_ZSET_565},{NULL,NULL},{NULL,NULL}} },	
{GE_ROP_TMAP_LSHADE_AMAP_AFLAT,			NULL,{{Span_C_TMAP_LSHADE_AMAP_AFLAT_555,Span_C_TMAP_LSHADE_AMAP_AFLAT_565},{NULL,NULL},{NULL,NULL}} },	
{GE_ROP_TMAP_LSHADE_AMAP_AFLAT_ZSET,	NULL,{{Span_C_TMAP_LSHADE_AMAP_AFLAT_ZSET_555,Span_C_TMAP_LSHADE_AMAP_AFLAT_ZSET_565},{NULL,NULL},{NULL,NULL}} },	
{GE_ROP_TMAP_LSHADE_AMAP_AFLAT_ZTEST,	NULL,{{Span_C_TMAP_LSHADE_AMAP_AFLAT_ZTEST_555,Span_C_TMAP_LSHADE_AMAP_AFLAT_ZTEST_565},{NULL,NULL},{NULL,NULL}} },	
{GE_ROP_TMAP_LSHADE_AMAP_AFLAT_ZTESTSET,NULL,{{Span_C_TMAP_LSHADE_AMAP_AFLAT_ZTEST_ZSET_555,Span_C_TMAP_LSHADE_AMAP_AFLAT_ZTEST_ZSET_565},{NULL,NULL},{NULL,NULL}} },	
};


geBoolean GENESISCC Span_SetOutputMode( geSpan_DestinationFormat DestFormat, geSpan_CPU CPU)
{
	int i;

	assert( DestFormat >= 0 );
	assert( DestFormat < GE_SPAN_DESTINATION_FORMATS );
	assert( CPU >= 0 );
	assert( CPU < GE_SPAN_HARDWARE_VERSIONS );

	for (i=0; i< GE_ROP_END; i++)
		{
			assert( Span_FunctionTable[i].ROP == i );
			Span_FunctionTable[i].Active = Span_FunctionTable[i].Function[GE_SPAN_HARDWARE_INTEL][DestFormat];
			if (Span_FunctionTable[i].Function[CPU][DestFormat]!=NULL)
				Span_FunctionTable[i].Active = Span_FunctionTable[i].Function[CPU][DestFormat];
		}
	return GE_TRUE;
}

Span_DrawFunction GENESISCC Span_GetDrawFunction(geROP ROP)
{
	assert( ROP >= 0 );
	assert( ROP < GE_ROP_END );
	assert( Span_FunctionTable[ROP].ROP == ROP );
	assert( Span_FunctionTable[ROP].Active != NULL );

	return Span_FunctionTable[ROP].Active;
}

									// Palette format is X B G R
									// 16 bit 555 format is xRGB
						#ifdef NOISE_FILTER
										*(DestBits++) = (DESTPIXEL) (	(((((Color&0xFF)*R)+Triangle.RandomTable[Triangle.RandomTableIndex++])>>16)&0x7C00) 
													 |	((((((Color&0xFF00)>>8)*G)+Triangle.RandomTable[Triangle.RandomTableIndex++])>>21)&0x3E0)
													 |	(((((Color&0xFF0000)>>16)*B)+Triangle.RandomTable[Triangle.RandomTableIndex++])>>26) );
						#endif
						

void GENESISCC Span_LightMapSample(void)
{	// use bilinear filter to sample the lightmap 
	int32 LMU,LMV;
	unsigned char *LM0,*LM1;
	unsigned char *LM2,*LM3;
	int C01,C23;
	int UFract01,VFract01;
	
	LMU = ((URight - Triangle.LightMapShiftU)>>8) * Triangle.LightMapScaleU;
	//LMU = (URight>>8)*Triangle.LightMapScaleU - Triangle.LightMapShiftU;
	// Clamp LMU to stay bounded to lightmap (no tiling)
	if (LMU<0) LMU=0;
	if (LMU>Triangle.LightMapMaxU)	LMU = Triangle.LightMapMaxU;

	LMV = ((VRight - Triangle.LightMapShiftV)>>8) * Triangle.LightMapScaleV;
	//LMV = (VRight>>8)*Triangle.LightMapScaleV - Triangle.LightMapShiftV;

	// Clamp LMV to stay bounded to lightmap (no tiling)
	if (LMV<0) LMV=0;
	if (LMV>Triangle.LightMapMaxV) LMV = Triangle.LightMapMaxV;
	
	// address base corner into lightmap by LMU,LMV
	LM0 = Triangle.LightMapBits + (3*(LMU>>16) + TOPDOWN_OR_BOTTOMUP((LMV>>16) * Triangle.LightMapStride));
	#pragma message ("is there a clamping problem here somewhere?  see a hi-res lightmap only rendering...")

	#if 1
		// address other corners, clamping
		if ((LMV>>16) < (Triangle.LightMapHeight-1)) 
			LM2 = LM0 + TOPDOWN_OR_BOTTOMUP(Triangle.LightMapStride);
		else
			LM2 = LM0;
		if ((LMU>>16) < (Triangle.LightMapWidth-1))
			{
				LM1 = LM0 + 3;
				LM3 = LM2 + 3;
			}
		else
			{
				LM1 = LM0;
				LM3 = LM2;
			}
		UFract01 = (LMU&0xFFFF);
		VFract01 = (LMV&0xFFFF);
		C01 =    (*LM0) + ((( *LM1 - *LM0 ) * UFract01)>>16);		
		C23 =    (*LM2) + ((( *LM3 - *LM2 ) * UFract01)>>16);		
		RRight =(   C01     + (((  C23 -  C01 ) * VFract01)>>16))<<RGB_FXP_SHIFTER;

		LM0++; LM1++;
		C01 =    (*LM0) + ((( *LM1 - *LM0 ) * UFract01)>>16);		
		
		LM2++; LM3++;
		C23 =    (*LM2) + ((( *LM3 - *LM2 ) * UFract01)>>16);		
		GRight =(   C01     + (((  C23 -  C01 ) * VFract01)>>16))<<RGB_FXP_SHIFTER;			

		LM0++; LM1++; 
		C01 =    (*LM0) + ((( *LM1 - *LM0 ) * UFract01)>>16);		

		LM2++; LM3++;
		C23 =    (*LM2) + ((( *LM3 - *LM2 ) * UFract01)>>16);		
		BRight =(   C01 + (((  C23 -  C01 ) * VFract01)>>16))<<RGB_FXP_SHIFTER;			
	#else
		RRight = *LM0<<RGB_FXP_SHIFTER; LM0++;
		GRight = *LM0<<RGB_FXP_SHIFTER; LM0++;
		BRight = *LM0<<RGB_FXP_SHIFTER; 
		#pragma message ("lightmap filtering disabled")
	#endif

	//R=RRight;G=GRight;B=BRight;
	//dR = dG= dB = 0;
}
	

