/****************************************************************************************/
/*  CPUInfo.C                                                                           */
/*                                                                                      */
/*  Author:  Mike Sandige, Ken Baird                                                    */
/*  Description:  simple cpu capabilities tests                                         */
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

#define WIN32_LEAN_AND_MEAN
#pragma warning(disable : 4201 4214 4115)
#include <windows.h>		// EXCEPTION_EXECUTE_HANDLER
#include <string.h>			// strnlen
#pragma warning(default : 4201 4214 4115; disable : 4514)

#include "CPUInfo.h"

#define CPUID _asm _emit 0fh _asm _emit 0a2h

static int Flag_CPUID = TRUE; 
static int Flag_RDTSC = TRUE;


void Test_CPU_bits(void)
{
	Flag_CPUID = FALSE;
	Flag_RDTSC = FALSE;
_asm
    {
	pushad                    // (1) play nice and save everything
    pushfd                    // eax = ebx = extended flags
    pop     eax                              
    mov     ebx,eax

    xor     eax,200000h       // toggle bit 21

    push    eax               // extended flags = eax
	popfd
    xor     eax,ebx           // if bit 21 r/w then eax <> 0

    jz      done              // can't toggle id bit (21) no cpuid here
    mov     eax,1             // get standard features
    mov     Flag_CPUID,eax    // (and set cpuid flag to true)

	CPUID
    test    edx,10h           // is rdtsc available?
    jz      done

    mov     Flag_RDTSC,1
done:
	popad                     // (1) restore everything
    }
}





static uint32	CPUInfo_GetCPUIDEAX(uint32 funcNum)
{
	uint32	retval;

	Test_CPU_bits();
	if (Flag_CPUID)
		{
			__try
			{
				_asm
				{
					mov	eax,funcNum
					CPUID
					mov	retval,eax
				}
			}__except(EXCEPTION_EXECUTE_HANDLER)
			{
				retval	=0;
			}
		}
	else
		{
			retval = 0;
		}

	return	retval;
}

static uint32	CPUInfo_GetCPUIDEDX(uint32 funcNum)
{
	uint32	retval;

	Test_CPU_bits();
	if (Flag_CPUID)
		{
			__try
			{
				_asm
				{
					mov	eax,funcNum
					CPUID
					mov	retval,edx
				}
			}__except(EXCEPTION_EXECUTE_HANDLER)
			{
				retval	=0;
			}
		}
	else
		{
			retval = 0;
		}
	
	return	retval;
}


static uint32	CPUInfo_GetCPUIDString(uint32 funcNum, char *szId)
{
	uint32	retval;
	
	Test_CPU_bits();
	if (Flag_CPUID)
		{
			__try
			{
				_asm
				{
					mov	eax,funcNum
					CPUID
					mov	retval,eax
					mov	eax,szId
					mov	dword ptr[eax],ebx
					mov	dword ptr[eax+4],edx
					mov	dword ptr[eax+8],ecx
				}
			}__except(EXCEPTION_EXECUTE_HANDLER)
			{
				retval	=0;
			}
		}
	else
		{
			retval = 0;
		}

	return	retval;
}


#define CPUINFO_VENDOR_STRING_LENGTH 16
#define CPUINFO_AMD_ID_STRING   "AuthenticAMD"
#define CPUINFO_INTEL_ID_STRING "GenuineIntel"

geBoolean CPUInfo_TestFor3DNow(void)
{
	char VendorString[CPUINFO_VENDOR_STRING_LENGTH];

	CPUInfo_GetCPUIDString(0, VendorString);
	if(strncmp(VendorString, CPUINFO_AMD_ID_STRING,strlen(CPUINFO_AMD_ID_STRING))==0)
		{
			uint32	TypeFlags	=CPUInfo_GetCPUIDEAX(0x80000000);
			if(TypeFlags)	//extended functions supported
				{
					TypeFlags	=CPUInfo_GetCPUIDEDX(0x80000001);
					if (TypeFlags & (1<<23))
						return GE_TRUE;
				}
		}
	return GE_FALSE;
}

geBoolean CPUInfo_TestForMMX(void)
{
	char VendorString[CPUINFO_VENDOR_STRING_LENGTH];

	CPUInfo_GetCPUIDString(0, VendorString);
	if(strncmp(VendorString, CPUINFO_AMD_ID_STRING,strlen(CPUINFO_AMD_ID_STRING))==0)
		{
			uint32	TypeFlags	=CPUInfo_GetCPUIDEAX(0x80000000);
			if(TypeFlags)	//extended functions supported
				{
					TypeFlags	=CPUInfo_GetCPUIDEDX(0x80000001);
					if (TypeFlags & (1<<23))
						return GE_TRUE;
				}
		}
	else if(strncmp(VendorString, CPUINFO_INTEL_ID_STRING, strlen(CPUINFO_INTEL_ID_STRING))==0)
	{
		uint32	TypeFlags				=CPUInfo_GetCPUIDEDX(0x1);
		if (TypeFlags & (1<<23))
			return GE_TRUE;
	}

	return GE_FALSE;
}
