/****************************************************************************************/
/*  STRBLOCK.H																			*/
/*                                                                                      */
/*  Author: Mike Sandige	                                                            */
/*  Description: String block interface.												*/
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
// geStrBlock
#ifndef GE_STRBLOCK_H
#define GE_STRBLOCK_H

#include "basetype.h"	// geBoolean
#include "vfile.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct geStrBlock geStrBlock;

geStrBlock *GENESISCC geStrBlock_Create(void);
void GENESISCC geStrBlock_Destroy(geStrBlock **SB);

geBoolean GENESISCC geStrBlock_Append(geStrBlock **ppSB,const char *String);

void GENESISCC geStrBlock_Delete(geStrBlock **ppSB,int Nth);

const char *GENESISCC geStrBlock_GetString(const geStrBlock *SB, int Index);

// untested...
//geBoolean GENESISCC geStrBlock_SetString(geStrBlock **ppSB, int Index, const char *String);
//geBoolean GENESISCC geStrBlock_Insert(geStrBlock **ppSB,int InsertAfterIndex,const char *String);

geBoolean GENESISCC geStrBlock_FindString(const geStrBlock* pSB, const char* String, int* pIndex);

int GENESISCC geStrBlock_GetCount(const geStrBlock *SB);
int GENESISCC geStrBlock_GetChecksum(const geStrBlock *SB);

geStrBlock* GENESISCC geStrBlock_CreateFromFile(geVFile* pFile);
geBoolean GENESISCC geStrBlock_WriteToFile(const geStrBlock *SB, geVFile *pFile);
geBoolean GENESISCC geStrBlock_WriteToBinaryFile(const geStrBlock *SB,geVFile *pFile);

#ifdef __cplusplus
}
#endif

#endif