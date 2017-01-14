/****************************************************************************************/
/*  GEASSERT.H                                                                          */
/*                                                                                      */
/*  Author:                                                                             */
/*  Description: Replacement for assert interface                                       */
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
#ifndef GE_ASSERT_H
#define GE_ASSERT_H

#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

// You should use geAssert() anywhere in the Genesis engine that
// you would normally use assert().
//
// If you wish to be called back when asserts happen, use the
// routine geAssertSetCallback().  It returns the address of
// the callback routine that you're replacing.


#ifdef NDEBUG

	#define geAssert(exp)

#else

	extern void geAssertEntryPoint( void *, void *, unsigned );

	#define geAssert(exp) (void)( (exp) || (geAssertEntryPoint(#exp, __FILE__, __LINE__), 0) )

#endif

/************************************************************/

typedef void geAssertCallbackFn( void *exp, void *file, unsigned line );

geAssertCallbackFn *geAssertSetCallback( geAssertCallbackFn *newAssertCallback );

typedef void (*geAssert_CriticalShutdownCallback) (void *Context);

extern void geAssert_SetCriticalShutdownCallback( geAssert_CriticalShutdownCallback CB , void *Context);

/************************************************************/

#ifdef __cplusplus
}
#endif

#endif
