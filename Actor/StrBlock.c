/****************************************************************************************/
/*  STRBLOCK.C																			*/
/*                                                                                      */
/*  Author: Mike Sandige	                                                            */
/*  Description: String block implementation.											*/
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
// strblock.c
//   a list of strings implemented as a single block of memory for fast
//   loading.  The 'Data' Field is interpreted as an array of integer 
//   offsets relative to the beginning of the data field.  After the int list
//   is the packed string data.  Since no additional allocations are needed 
//   this object can be file loaded as one block. 
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "strblock.h"
#include "ram.h"
#include "errorlog.h"

#define STRBLOCK_MAX_STRINGLEN 255

typedef struct geStrBlock
{
	int Count;
	geStrBlock *SanityCheck;
	union 
		{
			int IntArray[1];		// char offset into CharArray for string[n]
			char CharArray[1];
		} Data;
		
} geStrBlock;


int GENESISCC geStrBlock_GetChecksum(const geStrBlock *SB)
{
	int Count;
	int Len;
	int i,j;
	const char *Str;
	int Checksum=0;
	assert( SB != NULL );

	Count = geStrBlock_GetCount(SB);
	for (i=0; i<Count; i++)
		{
			Str = geStrBlock_GetString(SB,i);
			assert(Str!=NULL);
			Len = strlen(Str);
			for (j=0; j<Len; j++)
				 {
					Checksum += (int)Str[j];
				}
			Checksum = Checksum*3;
		}
	return Checksum;
}

geStrBlock *GENESISCC geStrBlock_Create(void)
{
	geStrBlock *SB;
	
	SB = GE_RAM_ALLOCATE_STRUCT(geStrBlock);

	if ( SB == NULL )
		{
			geErrorLog_Add(ERR_STRBLOCK_ENOMEM, NULL);
			return NULL;
		}
	SB->Count=0;
	SB->SanityCheck = SB;
	return SB;
}


void GENESISCC geStrBlock_Destroy(geStrBlock **SB)
{
	assert( (*SB)->SanityCheck == (*SB) );
	assert(  SB != NULL );
	assert( *SB != NULL );	
	geRam_Free( *SB );
	*SB = NULL;
}


static int GENESISCC geStrBlock_BlockSize(const geStrBlock *B)
{
	int Offset;
	const char *LastStr;
	assert( B != NULL );
	assert( B->SanityCheck == B );

	if ( B->Count == 0 )
		return 0;
	Offset = B->Data.IntArray[B->Count-1];
	LastStr = &(B->Data.CharArray[Offset]);

	return strlen(LastStr) + 1 + Offset;
}


void GENESISCC geStrBlock_Delete(geStrBlock **ppSB,int Nth)
{
	int BlockSize;
	int StringLen;
	int CloseSize;
	const char *String;
	assert(  ppSB  != NULL );
	assert( *ppSB  != NULL );
	assert( Nth >=0 );
	assert( Nth < (*ppSB)->Count );
	assert( (*ppSB)->SanityCheck == (*ppSB) );

	String = geStrBlock_GetString(*ppSB,Nth);
	assert( String != NULL );
	StringLen = strlen(String) + 1;
		
	BlockSize = geStrBlock_BlockSize(*ppSB);

	{
		geStrBlock *B = *ppSB;
		char *ToBeReplaced;
		char *Replacement=NULL;
		int i;
		ToBeReplaced = &((*ppSB)->Data.CharArray[(*ppSB)->Data.IntArray[Nth]]);
		if (Nth< (*ppSB)->Count-1)
			Replacement  = &((*ppSB)->Data.CharArray[(*ppSB)->Data.IntArray[Nth+1]]);
		for (i=Nth+1,CloseSize = 0; i<(*ppSB)->Count ; i++)
			{
				CloseSize += strlen(&((*ppSB)->Data.CharArray[(*ppSB)->Data.IntArray[i]])) +1;
				B->Data.IntArray[i] -= StringLen;
			}
		for (i=0; i<(*ppSB)->Count ; i++)
			{
				B->Data.IntArray[i] -= sizeof(int);
			}
		// crunch out Nth string
		if (Nth< (*ppSB)->Count-1)
			memmove(ToBeReplaced,Replacement,CloseSize);
		// crunch out Nth index
		memmove(&(B->Data.IntArray[Nth]),
				&(B->Data.IntArray[Nth+1]),
				BlockSize - ( sizeof(int) *  (Nth+1) ) );

	}
	
	{
		geStrBlock * NewgeStrBlock;

		NewgeStrBlock = geRam_Realloc( *ppSB, 
			BlockSize				// size of data block
			+ sizeof(geStrBlock)		// size of strblock structure
			- StringLen				// size of dying string
			- sizeof(int) );		// size of new index to string
		if ( NewgeStrBlock != NULL )
			{
				*ppSB = NewgeStrBlock;
				(*ppSB)->SanityCheck = NewgeStrBlock;
			}
	}

	(*ppSB)->Count--;
}


#if 0
	// as of yet un needed.  and this is untested
geBoolean GENESISCC geStrBlock_SetString(geStrBlock **ppSB, int Index, const char *String)
{
	assert(  ppSB  != NULL );
	assert( *ppSB  != NULL );
	assert( Index >=0 );
	assert( Index < (*ppSB)->Count );
	assert( String != NULL );

	if (geStrBlock_Insert(ppSB,Index,String) == GE_FALSE)
		{
			geErrorLog_Add(ERR_STRBLOCK_ENOMEM, NULL);
			return GE_FALSE;
		}
			
	geStrBlock_Delete(ppSB,Index);
	return GE_TRUE;
}
#endif

#if 0
	// as of yet un needed.  and this is untested
geBoolean GENESISCC geStrBlock_Insert(geStrBlock **ppSB,int InsertAfterIndex,const char *String)
{
	int BlockSize;
	int StringLen;
	int MoveSize;
	
	assert(  ppSB  != NULL );
	assert( *ppSB  != NULL );
	assert( InsertAfterIndex >=-1);
	assert( InsertAfterIndex < (*ppSB)->Count );
	assert( (*ppSB)->SanityCheck == (*ppSB) );
	assert( String != NULL );
	
	if (InsertAfterIndex == (*ppSB)->Count - 1)
		{
			if (geStrBlock_Append(ppSB,String)==GE_FALSE)
				{
					geErrorLog_Add(-1, NULL);
					return GE_FALSE;
				}
			return GE_TRUE;
		}

	StringLen = strlen(String) + 1;
		
	BlockSize = geStrBlock_BlockSize(*ppSB);

	{
		geStrBlock * NewgeStrBlock;

		NewgeStrBlock = geRam_Realloc( *ppSB, 
			BlockSize				// size of data block
			+ sizeof(geStrBlock)	// size of strblock structure
			+ StringLen				// size of new string
			+ sizeof(int) );		// size of new index to string
		if ( NewgeStrBlock != NULL )
			{
				*ppSB = NewgeStrBlock;
				(*ppSB)->SanityCheck = NewgeStrBlock;
			}
	}



	{
		geStrBlock *B = *ppSB;
		char *Chars = B->Data.CharArray;
		int  *Table = B->Data.IntArray;
		char *MoveFrom;
		char *MoveTo=NULL;
		int i;
		
		MoveFrom  = &(Chars[Table[InsertAfterIndex+1]]);
		
		MoveTo = MoveFrom + StringLen;
		
		for (i=InsertAfterIndex+1,MoveSize = 0; i<B->Count ; i++)
			{
				MoveSize += strlen(&(Chars[Table[i]])) +1;
				Table[i] += StringLen;
			}
		for (i=0; i<(*ppSB)->Count ; i++)
			{
				Table[i] += sizeof(int);
			}
		// make room for string
		memmove(MoveFrom,MoveTo,MoveSize);
		// make room for new index
		memmove(&(Table[InsertAfterIndex+1]),
				&(Table[InsertAfterIndex+2]),
				BlockSize - ( sizeof(int) *  (InsertAfterIndex+2) ) );
		Table[InsertAfterIndex+1] = Table[InsertAfterIndex] 
						+ strlen(&(Chars[Table[InsertAfterIndex]])) +1;

	}
	
	
	(*ppSB)->Count++;
	return GE_TRUE;


}
#endif

geBoolean GENESISCC geStrBlock_FindString(const geStrBlock* pSB, const char* String, int* pIndex)
{
	int i;
	int Count;
	const char *Str;

	assert(pSB != NULL);
	assert(String != NULL);
	assert(pIndex != NULL);
	assert( pSB->SanityCheck == pSB );

	Count = geStrBlock_GetCount(pSB);
	for (i=0; i<Count; i++)
	{
		Str = geStrBlock_GetString(pSB,i);
		if(strcmp(String, Str) == 0)
		{
			*pIndex = i;
			return GE_TRUE;
		}
	}
	return GE_FALSE;
}


geBoolean GENESISCC geStrBlock_Append(geStrBlock **ppSB,const char *String)
{
	int BlockSize;
	assert(  ppSB  != NULL );
	assert( *ppSB  != NULL );
	assert( String != NULL );
	assert( (*ppSB)->SanityCheck == (*ppSB) );

	if (strlen(String)>=STRBLOCK_MAX_STRINGLEN)
		{
			geErrorLog_Add(ERR_STRBLOCK_STRLEN, NULL);
			return GE_FALSE;
		}

	BlockSize = geStrBlock_BlockSize(*ppSB);

	{
		geStrBlock * NewgeStrBlock;

		NewgeStrBlock = geRam_Realloc( *ppSB, 
			BlockSize				// size of data block
			+ sizeof(geStrBlock)		// size of strblock structure
			+ strlen(String) + 1		// size of new string
			+ sizeof(int) );		// size of new index to string
		if ( NewgeStrBlock == NULL )
			{
				geErrorLog_Add(ERR_STRBLOCK_ENOMEM, NULL);
				return GE_FALSE;
			}
		*ppSB = NewgeStrBlock;
		(*ppSB)->SanityCheck = NewgeStrBlock;
	}

	{
		geStrBlock *B = *ppSB;
		int i;
		for (i=0; i<B->Count; i++)
			{
				B->Data.IntArray[i] += sizeof(int);
			}
		if (B->Count > 0)
			{
				memmove(&(B->Data.IntArray[B->Count+1]),
						&(B->Data.IntArray[B->Count]),
						BlockSize - sizeof(int) * B->Count);
			}
		B->Data.IntArray[B->Count] = BlockSize + sizeof(int);
		strcpy(&(B->Data.CharArray[B->Data.IntArray[B->Count]]),String);
	}
	(*ppSB)->Count++;
	return GE_TRUE;
}

	
const char *GENESISCC geStrBlock_GetString(const geStrBlock *SB, int Index)
{
	assert( SB != NULL );
	assert( Index >= 0 );
	assert( Index < SB->Count );
	assert( SB->SanityCheck == SB );
	return &(SB->Data.CharArray[SB->Data.IntArray[Index]]);
}

int GENESISCC geStrBlock_GetCount(const geStrBlock *SB)
{
	assert( SB != NULL);
	assert( SB->SanityCheck == SB );
	return SB->Count;
}


#define CHECK_FOR_WRITE(uu) if(uu <= 0) { geErrorLog_Add( ERR_STRBLOCK_FILE_WRITE, NULL ); return GE_FALSE; }

#define STRBLOCK_ASCII_FILE_TYPE 0x4B4C4253	// 'SBLK'
#define STRBLOCK_BIN_FILE_TYPE 0x424B4253	// 'SBKB'
#define STRBLOCK_FILE_VERSION 0x00F0		// Restrict version to 16 bits

#define STRBLOCK_STRINGARRAY_ID		"Strings"
#define STRBLOCK_NUM_ASCII_IDS       1 	// Keep this up to date

static geStrBlock *GENESISCC geStrBlock_CreateFromBinaryFile(geVFile *pFile);

geStrBlock* GENESISCC geStrBlock_CreateFromFile(geVFile* pFile)
{
	uint32 u, v;
	geStrBlock* SB;
	char	VersionString[32];

	assert( pFile != NULL );

	if(geVFile_Read(pFile, &u, sizeof(u)) == GE_FALSE)
	{
		geErrorLog_Add( ERR_STRBLOCK_FILE_READ , NULL);
		return NULL;
	}

	if(u == STRBLOCK_ASCII_FILE_TYPE)
	{
		int NumItemsNeeded=0;
		int NumItemsRead = 0;
		char line[STRBLOCK_MAX_STRINGLEN];

		SB = geStrBlock_Create();
		if( SB == NULL )
		{
			geErrorLog_Add(ERR_STRBLOCK_ENOMEM, NULL);
			return NULL;
		}

		// Read and build the version.  Then determine the number of items to read.
		if	(geVFile_GetS(pFile, VersionString, sizeof(VersionString)) == GE_FALSE)
		{
			geErrorLog_Add( ERR_STRBLOCK_FILE_READ , NULL);
			return NULL;
		}
		if	(sscanf(VersionString, "%X.%X\n", &u, &v) != 2)
		{
			geErrorLog_Add( ERR_STRBLOCK_FILE_READ , NULL);
			return NULL;
		}
		v |= (u << 8);
		if(v >= STRBLOCK_FILE_VERSION)
		{
			NumItemsNeeded = STRBLOCK_NUM_ASCII_IDS;
		}

		while(NumItemsRead < NumItemsNeeded)
		{
			if(geVFile_GetS(pFile, line, STRBLOCK_MAX_STRINGLEN) == GE_FALSE)
				{
					geErrorLog_Add( ERR_STRBLOCK_FILE_READ , NULL);
					break; // got to read something
				}
			else if(strnicmp(line, STRBLOCK_STRINGARRAY_ID, sizeof(STRBLOCK_STRINGARRAY_ID)-1) == 0)
			{
				int i,Count;
				if(sscanf(line + sizeof(STRBLOCK_STRINGARRAY_ID)-1, "%d", &Count) != 1)
					{						
						geErrorLog_Add( ERR_STRBLOCK_FILE_READ , NULL);
						break;		 
					}
				for (i=0; i<Count;i++)
					{
						if(geVFile_GetS(pFile, line, STRBLOCK_MAX_STRINGLEN) == GE_FALSE)
							{
								geErrorLog_Add( ERR_STRBLOCK_FILE_READ , NULL);
								break; // got to read something
							}
						if ( line[0] != 0 )
							line[strlen(line)-1] = 0;	// remove trailing /n  (textmode)
						if ( line[0] != 0 )
							{
								int len = strlen(line)-1;
								if (line[len] == 13)  // remove trailing /r  (binary file mode)
									{
										line[len] = 0;
									}
							}
						if (geStrBlock_Append(&SB,line)==GE_FALSE)
							{
								break; // error logged in _Append
							}
					}
				NumItemsRead++;
			}

			
		}

		if(NumItemsNeeded == NumItemsRead)
		{
			return SB;
		}
		else
		{
			geErrorLog_Add( ERR_STRBLOCK_FILE_PARSE , NULL);
			geStrBlock_Destroy(&SB); // try to destroy it
			return NULL;
		}
	}
	else
	{
		if (u!=STRBLOCK_BIN_FILE_TYPE)
			{
				geErrorLog_Add( ERR_STRBLOCK_FILE_PARSE , NULL);
				return NULL;
			}
		return geStrBlock_CreateFromBinaryFile(pFile);
	}
			
	geErrorLog_Add( ERR_STRBLOCK_FILE_PARSE , NULL);
	return NULL;
}

geBoolean GENESISCC geStrBlock_WriteToFile(const geStrBlock *SB,geVFile *pFile)
{
	uint32 u;
	int i,count;

	assert( SB != NULL );
	assert( pFile != NULL );
	assert( SB->SanityCheck == SB );


	// Write the format flag
	u = STRBLOCK_ASCII_FILE_TYPE;
	if(geVFile_Write(pFile, &u, sizeof(u)) == GE_FALSE)
	{
		geErrorLog_Add( ERR_STRBLOCK_FILE_WRITE , NULL);
		return GE_FALSE;
	}

	// Write the version
	if	(geVFile_Printf(pFile, " %X.%.2X\n", (STRBLOCK_FILE_VERSION & 0xFF00) >> 8, 
									STRBLOCK_FILE_VERSION & 0x00FF) == GE_FALSE)
	{
		geErrorLog_Add( ERR_STRBLOCK_FILE_WRITE , NULL);
		return GE_FALSE;
	}

	count = geStrBlock_GetCount(SB);
	if	(geVFile_Printf(pFile, "%s %d\n", STRBLOCK_STRINGARRAY_ID,count) == GE_FALSE)
	{
		geErrorLog_Add( ERR_STRBLOCK_FILE_WRITE , NULL);
		return GE_FALSE;
	}
	for (i=0; i<count; i++)
		{
			assert( strlen(geStrBlock_GetString(SB,i))<STRBLOCK_MAX_STRINGLEN);
			if	(geVFile_Printf(pFile,"%s\n",geStrBlock_GetString(SB,i)) == GE_FALSE)
			{
				geErrorLog_Add( ERR_STRBLOCK_FILE_WRITE , NULL);
				return GE_FALSE;
			}
		}
	return GE_TRUE;
}

typedef struct
{
	int Count;
	uint32 Size;
} geStrBlock_BinaryFileHeader;

static geStrBlock *GENESISCC geStrBlock_CreateFromBinaryFile(geVFile *pFile)
{
	geStrBlock *SB;
	geStrBlock_BinaryFileHeader Header;

	assert( pFile != NULL );

	if (geVFile_Read(pFile, &Header,sizeof(geStrBlock_BinaryFileHeader)) == GE_FALSE)
		{
			geErrorLog_Add( ERR_STRBLOCK_FILE_READ , NULL);
			return NULL;
		}
	
	SB = geRam_Allocate( sizeof(geStrBlock) + Header.Size );
	if( SB == NULL )
		{
			geErrorLog_Add(ERR_STRBLOCK_ENOMEM, NULL);
			return NULL;	
		}
	SB->SanityCheck = SB;
	SB->Count = Header.Count; 

	if (geVFile_Read(pFile, &(SB->Data),Header.Size) == GE_FALSE)
		{
			geErrorLog_Add( ERR_STRBLOCK_FILE_READ , NULL);
			return NULL;
		}
	return SB;
}
			

geBoolean GENESISCC geStrBlock_WriteToBinaryFile(const geStrBlock *SB,geVFile *pFile)
{
	uint32 u;
	geStrBlock_BinaryFileHeader Header;

	assert( SB != NULL );
	assert( pFile != NULL );
	assert( SB->SanityCheck == SB );

	// Write the format flag
	u = STRBLOCK_BIN_FILE_TYPE;
	if(geVFile_Write(pFile, &u, sizeof(u)) == GE_FALSE)
		{
			geErrorLog_Add( ERR_STRBLOCK_FILE_WRITE , NULL);
			return GE_FALSE;
		}

	Header.Size = geStrBlock_BlockSize(SB);
	Header.Count = SB->Count;

	if(geVFile_Write(pFile, &Header, sizeof(geStrBlock_BinaryFileHeader)) == GE_FALSE)
		{
			geErrorLog_Add( ERR_STRBLOCK_FILE_WRITE , NULL);
			return GE_FALSE;
		}
	
	if (geVFile_Write(pFile, &(SB->Data),Header.Size) == GE_FALSE)
		{
			geErrorLog_Add( ERR_STRBLOCK_FILE_WRITE , NULL);
			return GE_FALSE;
		}
		
	return GE_TRUE;
}
