/****************************************************************************************/
/*  GEASSERT.C                                                                          */
/*                                                                                      */
/*  Author:                                                                             */
/*  Description: Replacement for assert implementation                                  */
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
#include "geAssert.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
	   
// See geAssert.h for details.

void geAssertDefault(void *, void *, unsigned);
geAssertCallbackFn *geAssertCallback = &geAssertDefault;

geAssertCallbackFn *geSetAssertCallback( geAssertCallbackFn *newAssertCallback )
{
	geAssertCallbackFn *oldCallback = geAssertCallback;
	geAssertCallback = newAssertCallback;
	return oldCallback;
}

void geAssertDefault( void *exp, void *file, unsigned line )
{
#ifdef _DEBUG
	_assert( exp, file, line );	
#endif
}


// This might seem a little redundant, but I needed a unique name
// for the place that all geAsserts would begin.  From here, I
// call the geAssertCallback routine, whatever it has been 
// assigned to be. -Ken
void geAssertEntryPoint( void *exp, void *file, unsigned line )
{
	geAssertCallback( exp, file, line );
}

//-------------------------------------------

static geAssert_CriticalShutdownCallback CriticalCallBack = NULL;
static void * CriticalCallBackContext = NULL;

void geAssert_SetCriticalShutdownCallback( geAssert_CriticalShutdownCallback CB ,void *Context)
{
	CriticalCallBack = CB;
	CriticalCallBackContext = Context;
}

#ifndef NDEBUG

#include <windows.h>
#include <signal.h>

#define MAX_ASSERT_STRING_LENGTH 4096

void __cdecl _assert (void *expr,void *filename,unsigned lineno)
{
int nCode;
char assertbuf[MAX_ASSERT_STRING_LENGTH];	
static int in_assert_cnt = 0; // a semaphore

	if ( in_assert_cnt )
		return;
	in_assert_cnt++;

	if ( (strlen(expr) + strlen(filename) + 100) < MAX_ASSERT_STRING_LENGTH )
		sprintf(assertbuf,"assert(%s) \n FILE %s : LINE : %d\n",expr,filename,lineno);
	else
		sprintf(assertbuf," assert string longer than %d characters!\n",MAX_ASSERT_STRING_LENGTH);

    nCode = MessageBox(NULL,assertbuf,
        "Genesis3D Exception",
        MB_ABORTRETRYIGNORE|MB_ICONHAND|MB_SETFOREGROUND|MB_TASKMODAL);

    if (nCode == IDIGNORE)
	{
		in_assert_cnt --;
        return;
	}

    // Abort: abort the program
    if (nCode == IDABORT)
    {
		// CriticalCallBack does things like shut down the driver
		// if we retry or ignore, don't do it, so..
		if ( CriticalCallBack )
		{
			CriticalCallBack(CriticalCallBackContext);
		}

       // raise abort signal
       raise(SIGABRT);

        // We usually won't get here, but it's possible that
        //   SIGABRT was ignored.  So exit the program anyway.

        _exit(3);
    }

    // Retry: call the debugger
	// minimal code from here out so that the debugger can easily step back
	//	to the asserting line of code :

    if (nCode == IDRETRY)
        __asm { int 3 };

	in_assert_cnt --;
}

#endif	// NDEBUG




