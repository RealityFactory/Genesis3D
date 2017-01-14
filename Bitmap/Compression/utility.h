#ifndef __COMPUTIL_UTILITY_H
#define __COMPUTIL_UTILITY_H

/****************************************************************************************/
/*  Utility.h			                                                                */
/*                                                                                      */
/*  Author: Charles Bloom                                                               */
/*  Description:  Macros				                                                */
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
#include "ram.h"
#include "errorlog.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>	// for memcpy,memset

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int uint;

/****************************************/

#define BrandoError(str)	geErrorLog_AddString(-1,str,NULL)

#ifndef NULL
#define NULL (0)
#endif

#define sizeofpointer sizeof(void *)

#define PaddedSize(a) (((a)+3) & (~3))

#define IsOdd(a)  ( ((uint32)a)&1 )
#define SignOf(a) (((a) < 0) ? -1 : 1)

#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

#define minmax(x,lo,hi) ( (x)<(lo)?(lo):( (x)>(hi)?(hi):(x)) )
#define putminmax(x,lo,hi) x = minmax(x,lo,hi)
#define putmin(x,lo) x = min(x,lo)
#define putmax(x,hi) x = max(x,hi)
#define max3(a,b,c) max(max(a,b),c)
#define max4(a,b,c,d) max(a,max3(b,c,d))
#define min3(a,b,c) min(min(a,b),c)
#define min4(a,b,c,d) min(a,min3(b,c,d))

#ifndef mabs
#define mabs(i) ((i) < 0 ? -(i) : (i))
#endif

#define isinrange(x,lo,hi)	( (x) >= (lo) && (x) <= (hi) )

#define getuint32(bptr) ( ((((uint8 *)(bptr))[0])<<24) + (((uint8 *)(bptr))[1]<<16) + (((uint8 *)(bptr))[2]<<8) + (((uint8 *)(bptr))[3]) )
#define getuint16(bptr) ( (((uint8 *)(bptr))[0]<<8) + (((uint8 *)(bptr))[1]) )

/****************************************/

#ifndef strofval
#define strofval(x)	(#x)
#endif

#ifndef new
#define new(type)		geRam_AllocateClear(sizeof(type))
#endif

#ifndef destroy
#define destroy(mem)	do { if ( mem ) { geRam_Free(mem); (mem) = NULL; } } while(0)
#endif

#ifndef newarray
#define newarray(type,num)	geRam_AllocateClear((num)*sizeof(type))
#endif

#ifndef memclear
#define memclear(mem,size)	memset(mem,0,size);
#endif

/****************************************/

#ifdef __cplusplus
}
#endif

#endif // __COMPUTIL_UTILITY_H

