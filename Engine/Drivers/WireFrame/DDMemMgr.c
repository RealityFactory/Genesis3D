/****************************************************************************************/
/*  DDMemMgr.c                                                                          */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Mini D3D memory manager                                                */
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

#include "BaseType.h"
#include "DDMemMgr.h"

#define DDMEMMGR_MAX_PARTITIONS				16

typedef struct DDMemMgr_Partition
{
	geBoolean	Active;
	uint32		FreeMem;
	uint32		TotalMem;

} DDMemMgr_Partition;

typedef struct DDMemMgr
{
	uint32				TotalMem;
	uint32				FreeMem;
	DDMemMgr_Partition	Partitions[DDMEMMGR_MAX_PARTITIONS];			
} DDMemMgr;

//============================================================================
//	DDMemMgr_Create
//============================================================================
DDMemMgr *DDMemMgr_Create(uint32 Size)
{
	DDMemMgr	*MemMgr;

	MemMgr = (DDMemMgr*)malloc(sizeof(DDMemMgr));

	if (!MemMgr)
		return NULL;

	memset(MemMgr, 0, sizeof(DDMemMgr));

	MemMgr->TotalMem = Size;
	MemMgr->FreeMem = Size;

	return MemMgr;
}

//============================================================================
//	DDMemMgr_Destroy
//============================================================================
void DDMemMgr_Destroy(DDMemMgr *MemMgr)
{
	assert(MemMgr);

	free(MemMgr);
}

//============================================================================
//	DDMemMgr_Reset
//============================================================================
void DDMemMgr_Reset(DDMemMgr *MemMgr)
{
	int32		i;

	assert(MemMgr);

	MemMgr->FreeMem = MemMgr->TotalMem;

	for (i=0; i<DDMEMMGR_MAX_PARTITIONS; i++)
		memset(&MemMgr->Partitions[i], 0, sizeof(DDMemMgr_Partition));
}

//============================================================================
//	DDMemMgr_GetFreeMem
//============================================================================
uint32 DDMemMgr_GetFreeMem(DDMemMgr *MemMgr)
{
	assert(MemMgr);
	return MemMgr->FreeMem;
}

//============================================================================
//	DDMemMgr_PartitionCreate
//============================================================================
DDMemMgr_Partition *DDMemMgr_PartitionCreate(DDMemMgr *MemMgr, uint32 Size)
{
	int32					i;
	DDMemMgr_Partition		*pPartition;

	assert(MemMgr);

	if (Size > MemMgr->FreeMem)
		return NULL;
		
	pPartition = MemMgr->Partitions;

	for (i=0; i< DDMEMMGR_MAX_PARTITIONS; i++, pPartition++)
	{
		if (!pPartition->Active)
		{
			assert(pPartition->TotalMem == 0);
			assert(pPartition->FreeMem == 0);

			pPartition->TotalMem = Size;
			pPartition->FreeMem = Size;
			pPartition->Active = GE_TRUE;

			MemMgr->FreeMem -= Size;

			assert(MemMgr->FreeMem >= 0);

			return pPartition;
		}
	}

	return NULL;
}

//============================================================================
//	DDMemMgr_PartitionDestroy
//============================================================================
void DDMemMgr_PartitionDestroy(DDMemMgr_Partition *Partition)
{
	assert(Partition);
	assert(Partition->Active);

	memset(Partition, 0, sizeof(DDMemMgr_Partition));
}

//============================================================================
//	DDMemMgr_PartitionReset
//============================================================================
void DDMemMgr_PartitionReset(DDMemMgr_Partition *Partition)
{
	assert(Partition->Active);
	assert(Partition->FreeMem >= 0);

	Partition->FreeMem = Partition->TotalMem;
}

//============================================================================
//	DDMemMgr_PArtitionGetTotalMem
//============================================================================
uint32 DDMemMgr_PartitionGetTotalMem(DDMemMgr_Partition *Partition)
{
	assert(Partition);
	assert(Partition->TotalMem >= 0);

	return Partition->TotalMem;
}

//============================================================================
//	DDMemMgr_PArtitionGetFreeMem
//============================================================================
uint32 DDMemMgr_PartitionGetFreeMem(DDMemMgr_Partition *Partition)
{
	assert(Partition);
	assert(Partition->FreeMem >= 0);

	return Partition->FreeMem;
}

//============================================================================
//	DDMemMgr_PartitionAllocMem
//============================================================================
geBoolean DDMemMgr_PartitionAllocMem(DDMemMgr_Partition *Partition, uint32 Size)
{
	assert(Partition->Active);

	if (Partition->FreeMem < Size)
		return GE_FALSE;

	Partition->FreeMem -= Size;

	assert(Partition->FreeMem >= 0);

	return GE_TRUE;
}
