
//#define _TSC	// do this in the project settings!

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
/*Genesis3D Version 1.1 released November 15, 1999                            */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/

typedef unsigned long ulong;

// {
#ifdef _TSC

#pragma message("TSC on")

#include "ram.h"
#include "log.h"
#include <stdio.h>	//sprintf
#include <windows.h>	//outputdebug

#define MemAlloc(size)	geRam_Allocate(size)
#define MemFree(mem)	geRam_Free(mem)

#undef  new
#define new(type)		MemAlloc(sizeof(type))
#undef  destroy
#define destroy(mem)	MemFree(mem)

#include "tsc.h"

typedef struct tscNode tscNode;
struct tscNode 
{
	tscNode *next;
	ulong tsc[2];
};
tscNode * head = NULL;

void pushTSC(void)
{
tscNode *tn;
	tn = new(tscNode);
	if ( !tn ) return;
	tn->next = head;
	head = tn;
	readTSC(tn->tsc);
}

double popTSC(void)
{
tscNode *tn;
ulong tsc[2];
	readTSC(tsc);
	if ( ! head ) return 0.0;
	tn = head;
	head = head->next;
return diffTSC(tn->tsc,tsc);
}

void showPopTSC(const char *tag)
{
double time;
	time = popTSC();
	Log_Printf("%s : %f seconds\n",tag,time);
}

void showPopTSCper(const char *tag,int items,const char *itemTag)
{
double time,per;
	time = popTSC();
	per = (time/(double)items);
	if ( per < 0.000001 ) 
		Log_Printf("%s : %f = %e per %s\n",tag,time,per,itemTag);
	else
		Log_Printf("%s : %f = %f per %s\n",tag,time,per,itemTag);
}

void readTSC(ulong *hi)
{
ulong *lo = hi + 1;
	__asm 
	{
		_emit 0x0F;
		_emit 0x31;	// rdtsc
		mov EBX,hi
		mov DWORD PTR [EBX],EDX;
		mov EBX,lo
		mov DWORD PTR [EBX],EAX;
	}
	return;
}

#define CPU_HZ (_TSC_CPU_MHZ*1000000.0)
#define msw_scale (4294967296.0/CPU_HZ)
#define lsw_scale (1.0/CPU_HZ)

double diffTSC(ulong *tsc1,ulong *tsc2)
{

 return	(tsc2[1] - tsc1[1])*lsw_scale +
		(tsc2[0] - tsc1[0])*msw_scale;

}

#else // _TSC
// }{

#pragma message("TSC off")

void pushTSC(void) {}
double popTSC(void) { return 0.0; }
void showPopTSC(const char *tag) { }
void showPopTSCper(const char *tag,int items,const char *itemTag) { }
void readTSC(ulong *hi) {}
double diffTSC(ulong *tsc1,ulong *tsc2) { return 0; }

#endif // _TSC
// }
