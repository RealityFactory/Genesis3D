/****************************************************************************************/
/*  ERRORLOG.C                                                                          */
/*                                                                                      */
/*  Author: Mike Sandige                                                                */
/*  Description: Generic error logging system implementation                            */
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
#include <windows.h>
#include <stdio.h>
#include <assert.h>		// assert()	
#include <stdlib.h>
#include <string.h>		// memmove(), strncpy() strncat()

#include "ErrorLog.h"   

#define MAX_ERRORS 30  //  ...

#define MAX_USER_NAME_LEN	128		// Needed just 10 more chars! (was 100) JP
#define MAX_CONTEXT_LEN		128		// How big should this be?  Must be big enough to allow full paths to files, etc...

typedef struct
{
	geErrorLog_ErrorIDEnumType ErrorID;
	char String[MAX_USER_NAME_LEN+1];
	char Context[MAX_CONTEXT_LEN+1];
} geErrorType;

typedef struct
{
	int ErrorCount;
	int MaxErrors;
	geErrorType ErrorList[MAX_ERRORS];
} geErrorLogType;

geErrorLogType geErrorLog_Locals = {0,MAX_ERRORS};

GENESISAPI void geErrorLog_Clear(void)
	// clears error history
{
	geErrorLog_Locals.ErrorCount = 0;
}
	
GENESISAPI int  geErrorLog_Count(void)
	// reports size of current error log
{
	return 	geErrorLog_Locals.ErrorCount;
}


GENESISAPI void geErrorLog_AddExplicit(geErrorLog_ErrorClassType Error, 
	const char *ErrorIDString,
	const char *ErrorFileString,
	int LineNumber,
	const char *UserString,
	const char *Context)
{
	char	*SDst;
	char	*CDst;

	assert( geErrorLog_Locals.ErrorCount >= 0 );

	if (geErrorLog_Locals.ErrorCount>=MAX_ERRORS)
	{	// scoot list down by one (lose oldest error)
		memmove(
			(char *)(&( geErrorLog_Locals.ErrorList[0] )),
			(char *)(&( geErrorLog_Locals.ErrorList[1] )),
			sizeof(geErrorType) * (geErrorLog_Locals.MaxErrors-1) );
		geErrorLog_Locals.ErrorCount = geErrorLog_Locals.MaxErrors-1;
	}

	assert( geErrorLog_Locals.ErrorCount < geErrorLog_Locals.MaxErrors );

	SDst = geErrorLog_Locals.ErrorList[geErrorLog_Locals.ErrorCount].String;

	// Copy new error info
	if (ErrorIDString != NULL)
		{
			strncpy(SDst,ErrorIDString,MAX_USER_NAME_LEN);
		}

	strncat(SDst," ",MAX_USER_NAME_LEN);

	if (ErrorFileString!=NULL)
		{
			const char* pModule = strrchr(ErrorFileString, '\\');
			if(!pModule)
				pModule = ErrorFileString;
			else
				pModule++; // skip that backslash
			strncat(SDst,pModule,MAX_USER_NAME_LEN);
			strncat(SDst," ",MAX_USER_NAME_LEN);
		}
	
	{
		char Number[20];
		itoa(LineNumber,Number,10);
		strncat(SDst,Number,MAX_USER_NAME_LEN);
	}
	
	if (UserString != NULL)
		{
			if (UserString[0]!=0)
				{
					strncat(SDst," ",MAX_USER_NAME_LEN);
					strncat(SDst,UserString,MAX_USER_NAME_LEN);
				}
		}

	CDst = geErrorLog_Locals.ErrorList[geErrorLog_Locals.ErrorCount].Context;

	// Clear the context string in the errorlog to prepare for a new one
	memset(CDst, 0, sizeof(char)*MAX_CONTEXT_LEN);

	if (Context != NULL)
	{
		if (Context[0]!=0)
		{
			//strncat(SDst," ",MAX_USER_NAME_LEN);
			strncat(SDst,Context,MAX_USER_NAME_LEN);
		}
	}

	geErrorLog_Locals.ErrorCount++;

#ifndef NDEBUG
#pragma message ("Clean up the OutputDebugStrings in geErrorLog_AddExplicit")
{
	char	buff[100];
	sprintf(buff, "ErrorLog: %d -", Error);
	OutputDebugString(buff);
	OutputDebugString(SDst);
	OutputDebugString("\r\n");
}
#endif
}



GENESISAPI geBoolean geErrorLog_AppendStringToLastError(const char *String)
{
	char *SDst;
	if (String == NULL)
		{
			return GE_FALSE;
		}

	if (geErrorLog_Locals.ErrorCount>0)
		{
			SDst = geErrorLog_Locals.ErrorList[geErrorLog_Locals.ErrorCount-1].String;

			strncat(SDst,String,MAX_USER_NAME_LEN);
			return GE_TRUE;
		}
	else
		{
			return GE_FALSE;
		}
}

GENESISAPI geBoolean geErrorLog_Report(int history, geErrorLog_ErrorClassType *error, const char **UserString)
{
	assert( error != NULL );

	if ( (history > geErrorLog_Locals.ErrorCount) || (history < 0))
		{
			return GE_FALSE;
		}
	
	
	*error = geErrorLog_Locals.ErrorList[history].ErrorID;
	*UserString = geErrorLog_Locals.ErrorList[history].String;
	return GE_TRUE;
}

