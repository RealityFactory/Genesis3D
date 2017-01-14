/****************************************************************************************/
/*  TSC                                                                                 */
/*                                                                                      */
/*  Author: Charles Bloom                                                               */
/*  Description: tsc accessors                                                          */
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

#ifndef TSC_H
#define TSC_H

#ifdef __cplusplus
extern "C" {
#endif

/**********
*

	routines to access the TSC
	will do nothing unless you compile tsc.c with _TSC turned on

	to convert clocks to seconds we use this MHZ define:

*
********/

#define _TSC_CPU_MHZ	300

	// show() will Pop() two and print the delta to log()
	// does nothing unless debug is on

extern void pushTSC(void);

	// the pop reads once & pop once & take difference

extern double popTSC(void);
extern void showPopTSC(const char *tag);
extern void showPopTSCper(const char *tag,int items,const char *itemTag);

	// primitives

typedef unsigned long tsc_type [2];

extern void readTSC(unsigned long *tsc);
extern double diffTSC(unsigned long *tsc1,unsigned long*tsc2);

#ifdef __cplusplus
}
#endif

#endif
