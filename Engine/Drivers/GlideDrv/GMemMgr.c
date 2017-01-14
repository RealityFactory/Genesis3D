/****************************************************************************************/
/*  GMemMgr.c                                                                           */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Mini memory manager for glide                                          */
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
#include <Windows.h>
#include <Assert.h>

#include "GMemMgr.h"
#include "BaseType.h"
#include "Glide.h"

//========================================================================================================
//========================================================================================================
typedef struct GMemMgr
{
	GrChipID_t		Tmu;						// Tmu that this memory manager is in charge of

	uint32			MinAddress;					// Min address allowed to use for this mgr
	uint32			MaxAddress;					// Max address allowed to use for this mgr

	uint32			CurrentAddress;				
} GMemMgr;

//========================================================================================================
//	GMemMgr_Create
//	Create a memmgr on a tmu, and assumes it has all the data available to it...
//========================================================================================================
GMemMgr *GMemMgr_Create(GrChipID_t Tmu, uint32 MinAddress, uint32 MaxAddress)
{
	GMemMgr *MemMgr;

	MemMgr = malloc(sizeof(GMemMgr));

	if (!MemMgr)
		return NULL;

	// Clear the memmgr 
	memset(MemMgr, 0, sizeof(GMemMgr));

	// Set the default values...
	MemMgr->Tmu = Tmu;

	MemMgr->MinAddress = MinAddress;
	MemMgr->MaxAddress = MaxAddress;

	MemMgr->CurrentAddress = MinAddress;

	return MemMgr;
}

//========================================================================================================
//	GMemMgr_Destroy
//========================================================================================================
void GMemMgr_Destroy(GMemMgr *MemMgr)
{
	assert(MemMgr);

	free(MemMgr);
}

//========================================================================================================
//	GMemMgr_GetTotalMemory
//========================================================================================================
uint32 GMemMgr_GetTotalMemory(GMemMgr *MemMgr)
{
	return (MemMgr->MaxAddress - MemMgr->MinAddress);
}

//========================================================================================================
//	GMemMgr_GetFreeMemory
//========================================================================================================
uint32 GMemMgr_GetFreeMemory(GMemMgr *MemMgr)
{
	return (MemMgr->MaxAddress - MemMgr->CurrentAddress);
}

//========================================================================================================
//	GMemMgr_AllocMem
//========================================================================================================
geBoolean GMemMgr_AllocMem(GMemMgr *MemMgr, uint32 Size, uint32 *Address)
{
	#define	TWO_MEG		(1024*1024*2)
	
	uint32		StartAddr;

	assert(MemMgr);
	assert(GMemMgr_GetFreeMemory(MemMgr) >= Size);

	if (GMemMgr_GetFreeMemory(MemMgr) < Size)
		return GE_FALSE;

	if (((MemMgr->CurrentAddress+Size)%TWO_MEG) < Size)			// Align on 2 meg boundry...
	{
		MemMgr->CurrentAddress = ((MemMgr->CurrentAddress / TWO_MEG)+1) * TWO_MEG;
		if (MemMgr->CurrentAddress+Size >= MemMgr->MaxAddress)
			return GE_FALSE;
	}
	
	while ((MemMgr->CurrentAddress & 7) != 0)
	{
		MemMgr->CurrentAddress++;
		if (MemMgr->CurrentAddress+Size >= MemMgr->MaxAddress)
			return GE_FALSE;
	}

	StartAddr = MemMgr->CurrentAddress;

	MemMgr->CurrentAddress += Size;

	*Address = StartAddr;

	return GE_TRUE;
}

//========================================================================================================
//	GMemMgr_GetTmu
//========================================================================================================
GrChipID_t GMemMgr_GetTmu(GMemMgr *MemMgr)
{
	assert(MemMgr);

	return MemMgr->Tmu;
}

//========================================================================================================
//	GMemMgr_Reset
//========================================================================================================
void GMemMgr_Reset(GMemMgr *MemMgr)
{
	MemMgr->CurrentAddress = MemMgr->MinAddress;
}
