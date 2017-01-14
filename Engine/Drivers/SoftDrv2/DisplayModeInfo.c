/****************************************************************************************/
/*  DisplayModeInfo.C                                                                   */
/*                                                                                      */
/*  Author:  Mike Sandige                                                               */
/*  Description:  This is a simple container to hold information about available display*/
/*                modes for the software driver.                                        */
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
#include <stdlib.h>
#include <assert.h>
#include "DisplayModeInfo.h"

#ifdef GENESIS_VERSION_2
#include "errorlog.h"
#else
#define geErrorLog_AddString(Error,xx,yy) 
#endif


#define DISPLAYMODES_MAX 16

typedef struct DisplayModeInfo_Data
{
	int		Width;
	int		Height;
	int		BitsPerPixel;
	uint32	Flags;
}	DisplayModeInfo_Data;


typedef struct DisplayModeInfo
{
	DisplayModeInfo_Data	 Mode[DISPLAYMODES_MAX];
	int						 ModeCount;
}	DisplayModeInfo;


DisplayModeInfo *DisplayModeInfo_Create(void)
{
	DisplayModeInfo *Info;

	Info = malloc(sizeof(*Info));
	if (Info == NULL)
		{
			geErrorLog_AddString(GE_ERR_MEMORY_RESOURCE,"DisplayModeInfo:  unable to get memory for object",NULL);
			return NULL;
		}
	Info-> ModeCount = 0;
	return Info;
}

void DisplayModeInfo_Destroy(DisplayModeInfo **Info)
{
	assert ( Info != NULL );
	assert (*Info != NULL );
	free ( *Info );
	*Info = NULL;
}

int DisplayModeInfo_GetModeCount(DisplayModeInfo *Info)
{
	assert( Info != NULL );

	return Info->ModeCount;
}

geBoolean DisplayModeInfo_AddEntry(DisplayModeInfo *Info, 
				int Width,
				int Height,
				int BitsPerPixel,
				uint32 Flags)
{
	assert( Info != NULL );

	if (Info->ModeCount<DISPLAYMODES_MAX)
		{
			Info->Mode[Info->ModeCount].Width        = Width;
			Info->Mode[Info->ModeCount].Height       = Height;
			Info->Mode[Info->ModeCount].BitsPerPixel = BitsPerPixel;
			Info->Mode[Info->ModeCount].Flags        = Flags;
			Info->ModeCount++;
			return GE_TRUE;
		}
	else
		{
			geErrorLog_AddString(GE_ERR_INTERNAL_RESOURCE,"GE_DisplayModeInfo_AddEntry:Too many modes",NULL);
			return GE_FALSE;
		}
}

geBoolean DisplayModeInfo_GetNth(DisplayModeInfo *Info, int Nth,
				int *Width, 
				int *Height,
				int *BitsPerPixel,
				uint32 *Flags)
{
	assert( Info != NULL );
	assert( Width        != NULL );
	assert( Height       != NULL );
	assert( BitsPerPixel != NULL );
	assert( Flags        != NULL );
	if ((Nth < 0) || (Nth > Info->ModeCount))
		{
			geErrorLog_AddString(GE_ERR_BAD_PARAMETER,"DisplayModeInfo_GetNth:  bad mode index",geErrorLog_IntToString(Nth));
			return GE_FALSE;
		}

	*Width        = Info->Mode[Nth].Width;
	*Height       = Info->Mode[Nth].Height;
	*BitsPerPixel = Info->Mode[Nth].BitsPerPixel;
	*Flags        = Info->Mode[Nth].Flags;
	return GE_TRUE;
}

