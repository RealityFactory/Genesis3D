/****************************************************************************************/
/*  RAM.C                                                                               */
/*                                                                                      */
/*  Author:                                                                             */
/*  Description: Replacement for malloc, realloc and free                               */
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
#include <memory.h>
#include <malloc.h>
#include <assert.h>

#ifndef NDEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "ram.h"

/*
  This controls the MINIMAL_CONFIG flag.  Basically, all overflow, underflow,
  and size checking code is always enabled except when NDEBUG is defined...
*/
#ifdef NDEBUG
  // debugging's turned off, so make it minimal config
  #ifndef MINIMAL_CONFIG
    #define MINIMAL_CONFIG
  #endif
#else
  // debugging on, so do full checking
  #ifdef MINIMAL_CONFIG
    #undef MINIMAL_CONFIG
  #endif
#endif

// stupid stuff...
#ifndef GENESISDLLVERSION
void *StupidUnusedPointer;
#endif

// critical allocation stuff...
static int geRam_CriticalAllocationCount = 0;

static geRam_CriticalCallbackFunction geRam_CriticalCallback = NULL;

/*
  increments or decrements a counter.  if the counter is >0
  the critical callback function (if set) is called for a failed memory allocation.
  add is added to the current counter value.  the new counter value is returned.
*/
GENESISAPI int geRam_EnableCriticalCallback(int add)
{
	geRam_CriticalAllocationCount += add;
	return geRam_CriticalAllocationCount;
}


/*
  Set the critical callback function.  geRam_Allocate will call this function
  if it's unable to allocate memory.  Returns the previous critical callback fcn.
*/
GENESISAPI geRam_CriticalCallbackFunction geRam_SetCriticalCallback
    (
      geRam_CriticalCallbackFunction critical_callback
    )
{
    geRam_CriticalCallbackFunction OldCallback;

    OldCallback = geRam_CriticalCallback;
	geRam_CriticalCallback = critical_callback;
    return OldCallback;
}

/*
  If an allocation fails, this function will be called.  If the critical callback
  function is not NULL, then that function will be called.
*/
static int geRam_DoCriticalCallback
    (
      void
    )
{
    if ((geRam_CriticalAllocationCount != 0) && (geRam_CriticalCallback != NULL))
    {
        return geRam_CriticalCallback ();
    }
    else
    {
        return 0;
    }
}


GENESISAPI void * geRam_AllocateClear(uint32 size)
{
void * mem;
	size = (size + 3)&(~(uint32)3);
	mem = geRam_Allocate(size);
	if ( mem )
	{
		memset(mem,0,size);
	}
return mem;
}

#ifdef MINIMAL_CONFIG

    /*
      Minimal configuration acts almost exactly like standard malloc, free,
      and realloc.  The only difference is the critical allocation stuff.
    */


    /*
      Allocate memory of the given size.  In debug mode, the memory is filled
      with 0xA5, and we keep track of the amount of memory allocated.
    */
    void *geRam_Allocate
        (
          uint32 size
        )
    {
        void *p;

        do
        {
            p = malloc(size);
        } while ((p == NULL) && (geRam_DoCriticalCallback ()));

        return p;
    }

    // free an allocated block
    void geRam_Free_
        (
          void *ptr
        )
    {
      free (ptr);
    }

    // reallocate a block...
    // This acts like the standard realloc
GENESISAPI     void *geRam_Realloc
        (
          void *ptr,
          uint32 newsize
        )
    {
        char *p;
        char * NewPtr;

        if (ptr == NULL)
        {
            return geRam_Allocate (newsize);
        }

        // if newsize is NULL, then it's a free and return NULL
        if (newsize == 0)
        {
            geRam_Free (ptr);
            return NULL;
        }

        p = ptr;
        do
        {
            NewPtr = (char *)realloc (p, newsize);
        } while ((NewPtr == NULL) && (geRam_DoCriticalCallback ()));

        return NewPtr;
    }

#else  // MINIMAL_CONFIG
     /*
       For debugging implementations, we add a header and trailer to the
       allocated memory blocks so that we compute memory usage, and catch
       simple over- and under-run errors.
     */

    // yes, this will break if we use more than 2 gigabytes of RAM...
    int32 geRam_CurrentlyUsed       = 0;  // total ram currently in use
    int32 geRam_MaximumUsed         = 0;  // max total ram allocated at any time
    int32 geRam_NumberOfAllocations     = 0;  // current number of blocks allocated
    int32 geRam_MaximumNumberOfAllocations = 0;  // max number of allocations at any time

    // header and trailer stuff...
    static char MemStamp[] = {"!CHECKME!"};
    static const int MemStampSize = sizeof (MemStamp)-1;
    static const int SizeSize = sizeof (uint32);
	// <> CB : pad sized to multiples of 8 !!!
    #define HEADER_SIZE		(((SizeSize		+ MemStampSize)+7)&(~(uint32)7))
    #define EXTRA_SIZE		(((HEADER_SIZE	+ MemStampSize)+7)&(~(uint32)7))
    static const unsigned char AllocFillerByte = (unsigned char)0xA5;
    static const unsigned char FreeFillerByte  = (unsigned char)0xB6;
    /*
      A memory block is allocated that's size + (2*MemStampSize)+SizeSize bytes.
      It's then filled with 0xA5.  The size stamp is placed at the head of the block,
      with the MemStamp being placed directly after the size at the front, and
      also at the end of the block.  The layout is:

      <size><MemStamp><<allocated memory>><MemStamp>
    */
    typedef enum 
	{
		DONT_INITIALIZE = 0, 
		INITIALIZE_MEMORY = 1
	} geRam_MemoryInitialization;

    static void geRam_SetupBlock
          (
            char * p,
            uint32 size,
            geRam_MemoryInitialization InitMem
          )
    {
        if (InitMem == INITIALIZE_MEMORY)
        {
            // fill the memory block
            memset (p+HEADER_SIZE, AllocFillerByte, size);
        }

        // add the size at the front
        *((uint32 *)p) = size;

        // copy the memstamp to the front of the block
        memcpy (p+SizeSize, MemStamp, MemStampSize);

        // and to the end of the block
        memcpy (p+HEADER_SIZE+size, MemStamp, MemStampSize);
    }

#ifndef NDEBUG
GENESISAPI 	void* _geRam_DebugAllocate(uint32 size, const char* pFile, int line)
	{
      char *p;

      do
      {
          p = (char*)_malloc_dbg (size + EXTRA_SIZE, _NORMAL_BLOCK, pFile, line);
      } while ((p == NULL) && geRam_DoCriticalCallback ());

      if (p == NULL)
      {
         return NULL;
      }

      // setup size stamps and memory overwrite checks
      geRam_SetupBlock (p, size, INITIALIZE_MEMORY);

      // and update the allocations stuff
      geRam_NumberOfAllocations++;
      geRam_CurrentlyUsed += size;

      if (geRam_NumberOfAllocations > geRam_MaximumNumberOfAllocations)
      {
          geRam_MaximumNumberOfAllocations = geRam_NumberOfAllocations;
      }
      if (geRam_CurrentlyUsed > geRam_MaximumUsed)
      {
          geRam_MaximumUsed = geRam_CurrentlyUsed;
      }

      return p+HEADER_SIZE;
	}

#else // NDEBUG

GENESISAPI     void * geRam_Allocate (uint32 size)
    {
      char *p;

      do
      {
          p = (char*)malloc (size + EXTRA_SIZE);
      } while ((p == NULL) && geRam_DoCriticalCallback ());

      if (p == NULL)
      {
         return NULL;
      }

      // setup size stamps and memory overwrite checks
      geRam_SetupBlock (p, size, INITIALIZE_MEMORY);

      // and update the allocations stuff
      geRam_NumberOfAllocations++;
      geRam_CurrentlyUsed += size;

      if (geRam_NumberOfAllocations > geRam_MaximumNumberOfAllocations)
      {
          geRam_MaximumNumberOfAllocations = geRam_NumberOfAllocations;
      }
      if (geRam_CurrentlyUsed > geRam_MaximumUsed)
      {
          geRam_MaximumUsed = geRam_CurrentlyUsed;
      }

      return p+HEADER_SIZE;
    }
#endif // NDEBUG

    static char * ram_verify_block
          (
            void * ptr
          )
    {
        char * p = ptr;
        uint32 size;

        if (p == NULL)
        {
            assert (0 && "freeing NULL");
            return NULL;
        }

        // make p point to the beginning of the block
        p -= HEADER_SIZE;

        // get size from block
        size = *((uint32 *)p);

        // check stamp at front
        if (memcmp (p+SizeSize, MemStamp, MemStampSize) != 0)
        {
            assert (0 && "ram_verify_block:  Memory block corrupted at front");
            return NULL;
        }

        // and at back
        if (memcmp (p+HEADER_SIZE+size, MemStamp, MemStampSize) != 0)
        {
            assert (0 && "ram_verify_block:  Memory block corrupted at tail");
            return NULL;
        }

        return p;
    }

GENESISAPI     void geRam_Free_ (void *ptr)
    {
        char *p;
        uint32 size;

        // make sure it's a valid block...
        p = ram_verify_block (ptr);
        if (p == NULL)
        {
            return;
        }

        // gotta get the size before you free it
        size = *((uint32 *)p);

        // fill it with trash...
        memset (p, FreeFillerByte, size+EXTRA_SIZE);

        // free the memory
        free (p);

        // update allocations
        geRam_NumberOfAllocations--;
        assert ((geRam_NumberOfAllocations >= 0) && "free()d more ram than you allocated!");

        geRam_CurrentlyUsed -= size;
        assert ((geRam_CurrentlyUsed >= 0) && "free()d more ram than you allocated!");
    }

#ifndef NDEBUG

GENESISAPI     void * _geRam_DebugRealloc (void *ptr, uint32 newsize, const char* pFile, int line)
    {
        char *p;
        char * NewPtr;
        uint32 size;

        // if realloc is called with NULL, just treat it like an alloc
        if (ptr == NULL)
        {
            return _geRam_DebugAllocate(newsize, pFile, line);
        }

        // verify the block
        p = ram_verify_block (ptr);
        if (p == NULL)
        {
            return NULL;
        }

        // if newsize is NULL, then it's a free and return NULL
        if (newsize == 0)
        {
            geRam_Free (ptr);
            return NULL;
        }

        // gotta get the size before I realloc it...
        size = *((uint32 *)p);

        do
        {
            NewPtr = (char *)_realloc_dbg(p, newsize+EXTRA_SIZE, _NORMAL_BLOCK, pFile, line);
        } while ((NewPtr == NULL) && geRam_DoCriticalCallback ());

        // if allocation failed, return NULL...
        if (NewPtr == NULL)
        {
            return NULL;
        }

        geRam_SetupBlock (NewPtr, newsize, DONT_INITIALIZE);

        geRam_CurrentlyUsed += (newsize - size);
        if (geRam_CurrentlyUsed > geRam_MaximumUsed)
        {
            geRam_MaximumUsed = geRam_CurrentlyUsed;
        }
        assert ((geRam_CurrentlyUsed >= 0) && "free()d more ram than you allocated!");

        return NewPtr + HEADER_SIZE;
    }

#else // NDEBUG

GENESISAPI     void * geRam_Realloc (void *ptr, uint32 newsize)
    {
        char *p;
        char * NewPtr;
        uint32 size;

        // if realloc is called with NULL, just treat it like an alloc
        if (ptr == NULL)
        {
            return geRam_Allocate (newsize);
        }

        // verify the block
        p = ram_verify_block (ptr);
        if (p == NULL)
        {
            return NULL;
        }

        // if newsize is NULL, then it's a free and return NULL
        if (newsize == 0)
        {
            geRam_Free (ptr);
            return NULL;
        }

        // gotta get the size before I realloc it...
        size = *((uint32 *)p);

        do
        {
            NewPtr = (char *)realloc (p, newsize+EXTRA_SIZE);
        } while ((NewPtr == NULL) && geRam_DoCriticalCallback ());

        // if allocation failed, return NULL...
        if (NewPtr == NULL)
        {
            return NULL;
        }

        geRam_SetupBlock (NewPtr, newsize, DONT_INITIALIZE);

        geRam_CurrentlyUsed += (newsize - size);
        if (geRam_CurrentlyUsed > geRam_MaximumUsed)
        {
            geRam_MaximumUsed = geRam_CurrentlyUsed;
        }
        assert ((geRam_CurrentlyUsed >= 0) && "free()d more ram than you allocated!");

        return NewPtr + HEADER_SIZE;
    }

#endif // NDEBUG

#ifndef NDEBUG

GENESISAPI void geRam_ReportAllocations(void)
{
	_CrtDumpMemoryLeaks();
}

#endif

    // for external programs that allocate memory some other way.
    // Here they can use ram to keep track of the memory.
GENESISAPI     void geRam_AddAllocation (int n, uint32 size)
    {
        // and update the allocations stuff
        geRam_NumberOfAllocations += n;
        geRam_CurrentlyUsed += size;

        if (geRam_NumberOfAllocations > geRam_MaximumNumberOfAllocations)
        {
            geRam_MaximumNumberOfAllocations = geRam_NumberOfAllocations;
        }
        if (geRam_CurrentlyUsed > geRam_MaximumUsed)
        {
            geRam_MaximumUsed = geRam_CurrentlyUsed;
        }
    }

#endif // MINIMAL_CONFIG


#ifndef NDEBUG
geBoolean geRam_IsValidPtr(void *ptr)
{
char * p = ptr;
uint32 size;

	if (p == NULL) return GE_FALSE;

	// make p point to the beginning of the block
	p -= HEADER_SIZE;

	// get size from block
	size = *((uint32 *)p);

	// check stamp at front
	if (memcmp (p+SizeSize, MemStamp, MemStampSize) != 0)
	{
		return GE_FALSE;
	}

	// and at back
	if (memcmp (p+HEADER_SIZE+size, MemStamp, MemStampSize) != 0)
	{
		return GE_FALSE;
	}

return GE_TRUE;
}
#endif
