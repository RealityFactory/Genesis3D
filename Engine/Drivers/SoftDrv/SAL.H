/****************************************************************************************/
/*  sal.h                                                                               */
/*                                                                                      */
/*  Description:  GDI dib handling code                                                 */
/*                                                                                      */
/*                Code fragments contributed by John Miles                              */
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

#ifndef SAL_VERSION

#define SAL_VERSION      "1.00"
#define SAL_VERSION_DATE "23-May-96"

#endif

#ifndef SAL_H
#define SAL_H

#ifdef WIN32
  #define IS_WIN32 1
#endif

#ifdef _WIN32
  #define IS_WIN32 1
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
// If compiling SAL library, use __declspec(dllexport) for both 
// declarations and definitions
//
// If compiling SAL application, use __declspec(dllimport) for
// declarations -- definitions don't matter
//
// (DOS, other system definitions to be added later)
//
/*
#ifdef IS_WIN32

  #define SALEXPORT WINAPI

  #ifdef BUILD_SAL
    #define DXDEC __declspec(dllexport)
    #define DXDEF __declspec(dllexport)
  #else
    #define DXDEC __declspec(dllimport)
  #endif

#else

  #define SALEXPORT
  #define WINAPI

#endif
*/

#define SALEXPORT WINAPI
#define DXDEC 
#define DXDEF 

//
// Misc. constant definitions  
//

//
// General type definitions for portability
// 

#ifndef US_TYPEDEFS
#define US_TYPEDEFS

  typedef unsigned char   U8 ;
  typedef unsigned short  U16;
  typedef unsigned long   U32;
  typedef          char   C8 ;
  typedef signed   char   S8 ;
  typedef signed   short  S16;
  typedef signed   long   S32;

#endif

#ifndef YES
#define YES 1
#endif

#ifndef NO
#define NO  0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE  0
#endif

#define SAL_FULLSCREEN     0               // Set fullscreen DDraw mode
#define SAL_WINDOW         1               // Set DIB windowed mode
#define SAL_TRY_FULLSCREEN 2               // Try fullscreen, fall back to DIB

#define SAL_FRONT_SURFACE 0
#define SAL_BACK_SURFACE  1

//
// Preference names and default values
//

#define SAL_ALLOW_FRONT_LOCK         0     // Disregard request for front surface lock
#define DEFAULT_AFL                  NO
                                     
#define SAL_BUFFER_IF_NO_LFB         1     // Use system RAM page buffer if no LFB available
#define DEFAULT_BINL                 YES   
                                     
#define SAL_DOS_MONO_DEBUG           2     // Set up monochrome TTY debug monitor for DOS
#define DEFAULT_DMD                  YES

#define SAL_MAX_VIDEO_PAGES          3     // Allocate up to 3 video pages if possible
#define DEFAULT_MVP                  3     

#define SAL_ALLOW_WINDOW_RESIZE      4     // Allow window to be resized
#define DEFAULT_AWR                  YES

#define SAL_PREVENT_ALT_MENU_POPUP   5     // Do not allow ALT to pause app
#define DEFAULT_PAMP                 YES

#define SAL_ALWAYS_ON_TOP            6     // App has normal window Z order
#define DEFAULT_SAOT                 NO

#define SAL_MAXIMIZE_TO_FULLSCREEN   7     // Maximize button switches to FS
#define DEFAULT_MTF                  YES   

#define N_SAL_PREFS                  8     // # of preference types

//
// Structures
//

typedef struct
{
   U32 r;
   U32 g;
   U32 b;
   S32 alpha;
}
SAL_RGB32;

typedef struct
{
   S32 x0;
   S32 y0;
   S32 x1;
   S32 y1;
}
SAL_REGION;

typedef struct
{
   S32 x;
   S32 y;
   S32 width;
   S32 height;
}
SAL_WINAREA;

//
// Function pointer types
//

typedef void (SALEXPORT * SALFOCUSCB) (S32 status);
typedef void (SALEXPORT * SALEXITCB) (void);

//
// Miscellaneous/support services (not system-specific)
//

DXDEC void __cdecl SAL_debug_printf          (char *fmt, ...);

DXDEC  S32 WINAPI  SAL_get_preference        (U32   number);
                   
DXDEC  S32 WINAPI  SAL_set_preference        (U32   number, 
                                              S32   value);

//
// General services (not system-specific)
//

DXDEC void WINAPI SAL_set_palette_entry      (S32        index,
                                              SAL_RGB32 *entry,
                                              S32        wait_flag);
                                             
DXDEC void WINAPI SAL_get_palette_entry      (S32        index,
                                              SAL_RGB32 *entry);

DXDEC void WINAPI SAL_set_palette_range      (S32        index,
                                              S32        num_entries,
                                              SAL_RGB32 *entry_list,
                                              S32        wait_flag);
                                             
DXDEC void WINAPI SAL_get_palette_range      (S32        index,
                                              S32        num_entries,
                                              SAL_RGB32 *entry_list);

DXDEC void WINAPI SAL_get_pixel_format       (S32       *pixel_pitch,
                                              S32       *bytes_per_pixel,
                                              S32       *R_shift,
                                              U32       *R_mask,
                                              S32       *R_width,
                                              S32       *G_shift,
                                              U32       *G_mask,
                                              S32       *G_width,
                                              S32       *B_shift,
                                              U32       *B_mask,
                                              S32       *B_width);


DXDEC void WINAPI SAL_flip_surface           (void);
DXDEF void WINAPI SAL_blit_surface				(void);

DXDEC void WINAPI SAL_wipe_surface           (S32        surface,
                                              U32        color);

DXDEC void WINAPI SAL_lock_surface           (S32        surface,
                                              U8       **ptr,
                                              S32       *pitch);

DXDEC void WINAPI SAL_release_surface        (S32        surface,
                                              S32        perform_flip);

DXDEC void WINAPI SAL_lock_region            (S32        surface,
                                              SAL_REGION region,
                                              U8       **ptr,
                                              S32       *pitch);

DXDEC void WINAPI SAL_release_region         (S32        surface,
                                              SAL_REGION region);

//
// Mouse services
//

DXDEC void WINAPI SAL_show_system_mouse      (void);
DXDEC void WINAPI SAL_hide_system_mouse      (void);

DXDEC void WINAPI SAL_constrain_mouse        (void);
DXDEC void WINAPI SAL_unconstrain_mouse      (void);

//
// System-specific functions for Win32 version
//

#ifdef IS_WIN32
  
  DXDEC S32        WINAPI SAL_startup                 (BOOL FileLog);
  DXDEC void       WINAPI SAL_shutdown                (void);

  DXDEC BOOL WINAPI SAL_set_main_window(HWND hWindow);
  // CHANGE! (12/18/96)
  // Must have a menu in my app. 
  DXDEC HWND WINAPI SAL_create_main_window  (const char *MenuName);
                                                      
  DXDEC S32        WINAPI SAL_set_display_mode        (S32   display_size_X,
                                                       S32   display_size_Y,
                                                       S32   display_bpp,
                                                       S32   initial_window_mode,
                                                       S32   allow_mode_switch);

  DXDEC S32        WINAPI SAL_window_status           (void);
  DXDEC void       WINAPI SAL_window_area             (SAL_WINAREA *area);
  DXDEC void       WINAPI SAL_client_area             (SAL_WINAREA *area);
  DXDEC S32        WINAPI SAL_set_window_area         (SAL_WINAREA *area);
  DXDEC S32        WINAPI SAL_is_app_active           (void);
  DXDEC SALFOCUSCB WINAPI SAL_register_focus_callback (SALFOCUSCB fn);
  DXDEC WNDPROC    WINAPI SAL_register_WNDPROC        (WNDPROC fn);

  DXDEC void       WINAPI SAL_serve_message_queue     (void);

  DXDEC void      __cdecl SAL_error_box               (C8 *caption,
                                                       C8 *fmt, ...);
  // CHANGE! (12/18/96) John Pollard
  DXDEC void WINAPI SAL_GetBackBufferDC(HDC *dc);
  DXDEC void WINAPI SAL_ReleaseBackBufferDC(HDC dc);

#endif

#ifdef __cplusplus
}
#endif

#endif
