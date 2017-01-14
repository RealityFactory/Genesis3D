/****************************************************************************************/
/*  w32sal.cpp                                                                          */
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
/*Genesis3D Version 1.1 released November 15, 1999                            */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/


// Mike's notes:
// not very much of this module is being used, and what is being used is only
// being used by the software rasterizer.  This needs to be trimmed.  it would be
// best to take out the superclassing system and replace it with some basic utility 
// functions called from the client.
// I think these would do it:
//     paint()  to repaint
//     activate()  to inform the engine about activation/deactivation
//     fullscreen_toggle() 
//
//
//  for the moment, I've disabled most of the superclassing behavior, so that it
// does not interfere with the application's window (if it is in windowed mode)
// I think this will work (there may be problems with full-screen software 
// rasterization mode)
#define DISABLED_BEHAVIOR


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <winuser.h>
#include <mmsystem.h>
#include <Assert.h>

#include "sal.h"

#include <stdio.h>
#include <dos.h>
#include <malloc.h>
#include <direct.h>
#include <io.h>


//#ifdef SAL_USE_D3D
//	#include "d3dhal.h"
//#endif

//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл SAL statics and globals                                                лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

//
// Common variables
//

static	S32        desktop_w;                 // Windows desktop width, height
static	S32        desktop_h;

static	S32        cursor_state;              // Copy of Windows mouse hide/show state
static	S32        show_count;                // System mouse cursor show count

static	char       debug_log_filename[256];
static	S32        log_to_file;               // TRUE if user wants debug log

static	S32        app_minimized;             // TRUE if this SAL app has been minimized
static	S32        app_active;                // TRUE if this SAL app has input focus
static	S32        app_terminated;            // TRUE if app has received its quit message

static	SALEXITCB  exit_callback;             // Address of mandatory exit callback
static	SALFOCUSCB focus_callback;            // Address of focus callback, if any
static	WNDPROC    window_callback;           // Address of WNDPROC, if any
static	WNDPROC	   OldWindowProc;
static	HANDLE     hSem;                      // Semaphore to limit instances

static	S32        mode_change_request;       // TRUE to toggle window/fullscreen

static	HINSTANCE  hAppInstance;              // Application instance handle
static	HINSTANCE  hDLLInstance;              // DLL instance handle
static	char       szMyAppName[512];          // Application name
static	HWND       hWnd;                      // Handle to application window
static	HANDLE     hHook;                     // Handle to Windows hook object

static	S32 SAL_preference[N_SAL_PREFS];      // Preferences array

static	S32        mode_change_allowed;       // TRUE if window/fullscreen toggle allowed
static	S32        current_window_mode;       // SAL_FULLSCREEN / SAL_WINDOW
static	S32        current_bpp;               // Mode info set by last call to 
static	S32        current_size_X;            //  SAL_set_display_mode
static	S32        current_size_Y;

static	LOGPALETTE       *pLogPal;            // LOGPALETTE structure
static	BITMAPINFO       *pbmi;               // BITMAPINFO structure
static	BITMAPINFOHEADER *pbmih;              // Pointer to pbmi header

static	SAL_RGB32  palette_state[256];        // Current state of all DAC registers

//
// CreateDIBSection() (windowed mode) variables and related data
//

static	SAL_WINAREA area;                     // Location/size of window client area

static	S32         desktop_bpp;              // Windows video mode BPP
static	U32         desktop_R_bitmask;        // Desktop high-color pixel format
static	U32         desktop_G_bitmask;        
static	U32         desktop_B_bitmask;

static	U32         DIB_R_bitmask;            // DIB high-color pixel format
static	U32         DIB_G_bitmask;
static	U32         DIB_B_bitmask;

static	U32         DIB_R_Shift;
static	U32         DIB_G_Shift;
static	U32         DIB_B_Shift;

static	S32         DIB_active;               // CreateDIBSection() active if TRUE

static	U8         *lpDIBBuffer;              // DIB image buffer
static	HBITMAP     hDIB;                     // Handle returned from CreateDIBSection
static	HPALETTE    hPalette;                 // Handle to palette if 8-bit mode in use

static	S32         palette_change_request;   // TRUE to signal palette update

static	RECT        original_window_rect;     // Default size of DIB window
static	SAL_WINAREA original_area;            // Default size of client area
static	LONG		OldWindowLong_GWL_STYLE;	// Old Window GWL_STYLE	
static	LONG		OldWindowLong_GWL_EXSTYLE;	// Old Window GWL_STYLE	

static	RECT       unconstrained_rect;        // Default area of mouse constraint
static	S32        constrain_state;           // 1 if mouse limited to window area
static	S32        constrain_request;         // 1 if mouse should be constrained at next movement

static	HDC Context;
static	HBITMAP DefaultBitmap;

extern "C" long FAR PASCAL MyWndProc(HWND   hWindow, UINT   message,   //)
                       WPARAM wParam,  LPARAM lParam);

//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл DLLMain() function to acquire DLL instance handle, etc.                лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

BOOL WINAPI DllMain(HINSTANCE hinstDLL,//)
                    DWORD     fdwReason,
                    LPVOID    lpvReserved)
{
   if (fdwReason == DLL_PROCESS_ATTACH)
      {
      hDLLInstance = hinstDLL;
      }

   return TRUE;
}

//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл Serve Windows message queue, returning 0 if WM_QUIT message received   лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

void serve_queue(void)
{
	/*
	MSG msg;

	if (app_terminated)
	{
		return;
	}

	//
	//	Serve message queue
	//

	while (PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE))
	{
		if (!GetMessage(&msg, NULL, 0, 0 ))
		{
			app_active     = FALSE;
			app_terminated = TRUE;

			(exit_callback)();

			//
			// Return statement should not be reached
			//

			return;
		}

		TranslateMessage(&msg); 
		DispatchMessage(&msg);
	}
	*/
}

//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл Return rectangle containing client-area boundaries in screenspace      лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

RECT *client_screen_rect(void)
{
   static RECT  rect;
   POINT        ul,lr;

   GetClientRect(hWnd, &rect);

   ul.x = rect.left; 
   ul.y = rect.top; 
   lr.x = rect.right; 
   lr.y = rect.bottom; 

   ClientToScreen(hWnd, &ul); 
   ClientToScreen(hWnd, &lr); 

   SetRect(&rect, ul.x, ul.y, 
                  lr.x-1, lr.y-1); 

   return &rect;
}

//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл Mouse/keyboard event management                                        лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

void MOUSE_event(unsigned long wParam, unsigned long lParam)
{
}

void KEYDOWN_event(unsigned long wParam, unsigned long lParam)
{
}

void KEYUP_event(unsigned long wParam, unsigned long lParam)
{
}

void CHAR_event(unsigned long wParam, unsigned long lParam)
{
}


//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл Shut down CreateDIBSection() services                                  лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

void DIB_shutdown(void)
{
   if (constrain_state)
      {
      #ifndef DISABLED_BEHAVIOR
		ClipCursor(&unconstrained_rect);
	  #endif
      constrain_request = 0;
      }

   if (DIB_active && SAL_is_app_active())
      {
      SAL_wipe_surface(SAL_BACK_SURFACE, 0);
      SAL_flip_surface();
      }

   if (hDIB != NULL)
      {
      DeleteObject(hDIB);
      hDIB = NULL;
      }

   if (hPalette != NULL)
      {
      DeleteObject(hPalette);
      hPalette = NULL;
      }
		/*
		//
		//	Make sure we restore the old hWnd parms
		//
		SetWindowLong(hWnd, 
		             GWL_STYLE, 
		             OldWindowLong_GWL_STYLE);
		SetWindowLong(hWnd, 
		             GWL_EXSTYLE, 
		             OldWindowLong_GWL_EXSTYLE);
		SetWindowPos(hWnd, 
		            HWND_TOP, 
		            original_area.x,
		            original_area.y,
					original_area.width,
		            original_area.height,
		            SWP_NOCOPYBITS | SWP_NOZORDER);
		
		ShowWindow(hWnd, SW_SHOWNORMAL);
		*/
   DIB_active = 0;
}

//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл Start up CreateDIBSection() services for windowed display              лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

S32 DIB_startup(S32   display_size_X,//)
                S32   display_size_Y,
                S32   display_bpp)
{
	HDC      hdc;
	COLORREF color,save;
	RECT     window_rect;

	area.x			= 0;
	area.y			= 0;
	area.width		= display_size_X;
	area.height		= display_size_Y;
	original_area	= area;
	
	GetWindowRect(hWnd, &window_rect);
	original_window_rect = window_rect;
	
	OldWindowLong_GWL_STYLE = GetWindowLong(hWnd, GWL_STYLE);
	OldWindowLong_GWL_EXSTYLE = GetWindowLong(hWnd, GWL_EXSTYLE);

	display_size_X = area.width;
	display_size_Y = area.height;

	// Get desktop size
	//
	/*
	desktop_w = GetSystemMetrics(SM_CXSCREEN);
	desktop_h = GetSystemMetrics(SM_CYSCREEN);

	//
	// Enable caption menu and user preferences
	//

	SetWindowLong(hWnd, 
                 GWL_STYLE, 
                 GetWindowLong(hWnd, GWL_STYLE) & ~WS_POPUP);

	SetWindowLong(hWnd, 
                 GWL_STYLE, 
                 GetWindowLong(hWnd, GWL_STYLE) | (WS_OVERLAPPED  | 
                                                   WS_CAPTION     | 
                                                   WS_SYSMENU     | 
                                                   WS_MINIMIZEBOX));

	if (SAL_get_preference(SAL_ALLOW_WINDOW_RESIZE))
	{
		SetWindowLong(hWnd, 
                    GWL_STYLE, 
                    GetWindowLong(hWnd, GWL_STYLE) | WS_THICKFRAME |
                                                     WS_MAXIMIZEBOX);
	}

	if (SAL_get_preference(SAL_ALWAYS_ON_TOP))
	{
		SetWindowLong(hWnd, 
                    GWL_EXSTYLE, 
                    GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_TOPMOST);
	}
	else
	{
		SetWindowLong(hWnd, 
                    GWL_EXSTYLE, 
                    GetWindowLong(hWnd, GWL_EXSTYLE) & ~WS_EX_TOPMOST);
	}

	//
	// If area not already established, center window's client area on
	// desktop, and size it to correspond to the display size for optimum 
	// performance (no stretching needed)
	//

	//if (area.width == -1)
	{
		area.width  = display_size_X;
		area.height = display_size_Y;

		area.x = ((desktop_w - area.width ) / 2);
		area.y = ((desktop_h - area.height) / 2);
	}

	//
	// Calculate adjusted position of window
	//
	// Do not allow overall window size to exceed desktop size; keep 
	// dividing height and width by 2 until entire window fits
	//
	// If window is offscreen (or almost entirely offscreen), center it
	//

	do
	{
		retry = 0;

		window_rect.left   = area.x;
		window_rect.right  = area.x + area.width - 1;
		window_rect.top    = area.y;
		window_rect.bottom = area.y + area.height - 1;

		AdjustWindowRectEx(&window_rect,
                          GetWindowLong(hWnd, GWL_STYLE),
                         (GetMenu(hWnd) != NULL),
                          GetWindowLong(hWnd, GWL_EXSTYLE));

		if ((window_rect.right - window_rect.left + 1) > desktop_w)
		{
			area.width >>= 1;
			area.x = ((desktop_w - area.width ) / 2);
			retry = 1;
		}

		if ((window_rect.bottom - window_rect.top + 1) > desktop_h)
		{
			area.height >>= 1;
			area.y = ((desktop_h - area.height) / 2);
			retry = 1;
		}

		if ((window_rect.left   >= (desktop_w-16)) ||
			(window_rect.top    >= (desktop_h-16)) ||
			(window_rect.right  <= 16)          ||
			(window_rect.bottom <= 16))
		{
			area.x = ((desktop_w - area.width ) / 2);
			area.y = ((desktop_h - area.height) / 2);
			retry = 1;
		}
	}
   while (retry);

   //
   // Save window and client areas for restoration if maximize button pressed
   //

   original_window_rect = window_rect;
   original_area        = area;

   //
   // Set window size and position
   //

   SetWindowPos(hWnd, 
                HWND_TOP, 
                window_rect.left,
                window_rect.top,
                window_rect.right  - window_rect.left + 1,
                window_rect.bottom - window_rect.top  + 1,
                SWP_NOCOPYBITS | SWP_NOZORDER);

   GetClientRect(hWnd, 
                &client_rect);

   SAL_debug_printf("SAL: Window at (%d,%d), client size = (%d,%d)\n",
      window_rect.left,
      window_rect.top,
      client_rect.right,
      client_rect.bottom);

   //
   // Make window visible
   //

   ShowWindow(hWnd, SW_SHOWNORMAL);

   //
   // If mouse is constrained, limit its travel to the window area
   //

   */

   if (constrain_state)    
      {
      constrain_request = 1;
      }

   //
   // Init DIB globals
   //

   hDIB        = NULL;
   lpDIBBuffer = NULL;
   hPalette    = NULL;

   pbmih->biSize          =  sizeof(BITMAPINFOHEADER);
   pbmih->biWidth         =  (display_size_X);
   pbmih->biHeight        = -(display_size_Y);
   pbmih->biPlanes        =  1;
   pbmih->biBitCount      =  (U16) display_bpp;
   pbmih->biSizeImage     =  0;
   pbmih->biXPelsPerMeter =  0;
   pbmih->biYPelsPerMeter =  0;
   pbmih->biClrUsed       =  0;
   pbmih->biClrImportant  =  0;

   //
   // Get Windows desktop display format (and masks, in high-color modes)
   //

   hdc = GetDC(hWnd);

   desktop_bpp = GetDeviceCaps(hdc, BITSPIXEL) * 
                 GetDeviceCaps(hdc, PLANES);

   SAL_debug_printf("SAL: Desktop = %dx%d, %d BPP\n",desktop_w, 
                                                     desktop_h, 
                                                     desktop_bpp);

   if(desktop_bpp > 8)
   {
	   HBITMAP			TempBmp;
	   BITMAPINFO		TempInfo;
	   OSVERSIONINFO	WinVer;

	   memset(&WinVer, 0, sizeof(OSVERSIONINFO));
	   WinVer.dwOSVersionInfoSize	=sizeof(OSVERSIONINFO);

	   GetVersionEx(&WinVer);
	   if(WinVer.dwPlatformId == VER_PLATFORM_WIN32_NT)
	   {
		   TempBmp	=CreateCompatibleBitmap(hdc, 8, 8);
		   if(TempBmp)
		   {
			   memset(&TempInfo, 0, sizeof(BITMAPINFO));
			   TempInfo.bmiHeader.biSize		=sizeof(BITMAPINFO);
			   TempInfo.bmiHeader.biBitCount	=(U16)desktop_bpp;
			   TempInfo.bmiHeader.biCompression	=BI_BITFIELDS;

			   if(GetDIBits(hdc, TempBmp, 0, 0, NULL, &TempInfo, DIB_RGB_COLORS))
			   {
				   desktop_R_bitmask	=*((U32 *)&TempInfo.bmiColors[0]);
				   desktop_G_bitmask	=*((U32 *)&TempInfo.bmiColors[1]);
				   desktop_B_bitmask	=*((U32 *)&TempInfo.bmiColors[2]);

				   SAL_debug_printf("(%x-%x-%x)\n", desktop_R_bitmask, desktop_G_bitmask, desktop_B_bitmask);
			   }
			   DeleteObject(TempBmp);
		   }
	   }
	   else
	   {
		   save = GetPixel(hdc,0,0);

		  SetPixel(hdc,0,0,RGB(0x08,0x08,0x08));

		  color = GetPixel(hdc,0,0) & 0xffffff;

		  SAL_debug_printf("SAL: Desktop pixel format = 0x%X ",color);

		  switch (color)
			 {
			 //
			 // 0x000000 = 5-5-5
			 //

			 case 0x000000:
            
				SAL_debug_printf("(5-5-5)\n");

				desktop_R_bitmask = 0x007c00;
				desktop_G_bitmask = 0x0003e0;
				desktop_B_bitmask = 0x00001f;
				break;

			 //
			 // 0x000800 = 5-6-5
			 //

			 case 0x000800:
            
				SAL_debug_printf("(5-6-5)\n");

				desktop_R_bitmask = 0x00f800;
				desktop_G_bitmask = 0x0007e0;
				desktop_B_bitmask = 0x00001f;
				break;

			 //
			 // 0x080808 = 8-8-8
			 //

			 case 0x080808:

				SAL_debug_printf("(8-8-8)\n");

				desktop_R_bitmask = 0xff0000;
				desktop_G_bitmask = 0x00ff00;
				desktop_B_bitmask = 0x0000ff;
				break;

			 default:

				SAL_debug_printf("(Unsupported, using 5-6-5)\n");

				if ((desktop_bpp == 15) || (desktop_bpp == 16))
				   {
				   desktop_R_bitmask = 0x00f800;
				   desktop_G_bitmask = 0x0007e0;
				   desktop_B_bitmask = 0x00001f;
				   }
				break;
			 }

		  SetPixel(hdc,0,0,save);
		  }
   }
   ReleaseDC(hWnd, hdc);

   //
   // If DIB and desktop are both in 15/16-BPP mode, set DIB to desktop
   // pixel format for maximum throughput
   // 
   // Otherwise, set DIB to 5-6-5 mode if 16BPP DIB requested, or 8-8-8 mode
   // if 24BPP DIB requested
   //
   // Finally, if 8BPP DIB requested, create GDI palette object based on 
   // current logical palette
   //

   switch (display_bpp)
      {
      case 8:
   
         pbmih->biCompression = BI_RGB;

         hPalette = CreatePalette(pLogPal);
         palette_change_request = TRUE;
         break;

      case 16:

         pbmih->biCompression = BI_BITFIELDS;

         if ((desktop_bpp == 15) || (desktop_bpp == 16))
            {
            *(U32 *) (&(pbmi->bmiColors[0])) = desktop_R_bitmask;
            *(U32 *) (&(pbmi->bmiColors[1])) = desktop_G_bitmask;
            *(U32 *) (&(pbmi->bmiColors[2])) = desktop_B_bitmask;
            DIB_R_bitmask = desktop_R_bitmask;
            DIB_G_bitmask = desktop_G_bitmask;
            DIB_B_bitmask = desktop_B_bitmask;

            }
         else
            {
            *(U32 *) (&(pbmi->bmiColors[0])) = 0x00f800;
            *(U32 *) (&(pbmi->bmiColors[1])) = 0x0007e0;
            *(U32 *) (&(pbmi->bmiColors[2])) = 0x00001f;
            DIB_R_bitmask = 0x00f800;
            DIB_G_bitmask = 0x0007e0;
            DIB_B_bitmask = 0x00001f;
            }
         break;

      case 24:

         pbmih->biCompression = BI_BITFIELDS;

         *(U32 *) (&(pbmi->bmiColors[0])) = 0xff0000;
         *(U32 *) (&(pbmi->bmiColors[1])) = 0x00ff00;
         *(U32 *) (&(pbmi->bmiColors[2])) = 0x0000ff;
         DIB_R_bitmask = 0xff0000;
         DIB_G_bitmask = 0x00ff00;
         DIB_B_bitmask = 0x0000ff;
         break;
      }

   //
   // Allocate the DIB section ("back buffer")
   //

   hdc = GetDC(hWnd);

   hDIB = CreateDIBSection(hdc,             // Device context
                           pbmi,            // BITMAPINFO structure
                           DIB_RGB_COLORS,  // Color data type
                (void **) &lpDIBBuffer,     // Address of image map pointer
                           NULL,            // File
                           0);              // Bitmap file offset

   ReleaseDC(hWnd, hdc);
   
   if (hDIB == NULL)
      {
      SAL_error_box(NULL,"CreateDIBSection() failed\n");
      DIB_shutdown();
      return FALSE;
      }
   
   //
   // Set global flag to indicate CreateDIBSection() is active
   // 

   DIB_active = 1;

   return TRUE;
}

//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл Copy current contents of DIB buffer to output window                   лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

void DIB_refresh_surface(void)
{
   HDC hdc;

   hdc = GetDC(hWnd);

   //
   // Select palette if desktop running in 8-bit mode
   //
   // If palette has changed, realize it
   //

   if (desktop_bpp == 8)
      {
      SelectPalette(hdc, 
                    hPalette, 
                    0);

      if (palette_change_request)
         {
         palette_change_request = 0;
         RealizePalette(hdc);
         }
      }

   //
   // Disable Boolean operations during stretching
   //

   SetStretchBltMode(hdc, COLORONCOLOR);

   //
   // Stretch bitmap to conform to the size of the output window
   //
   /*
   StretchDIBits(hdc,            // Destination DC
                 0,              // Destination X
                 0,              // Destination Y
                 area.width,     // Destination (client area) width
                 area.height,    // Destination (client area) height 
                 0,              // Source X
                 0,              // Source Y
                 current_size_X, // Source (back buffer) width
                 current_size_Y, // Source (back buffer) height
                 lpDIBBuffer,    // Pointer to source (back buffer)
                 pbmi,           // Bitmap info for back buffer
                 DIB_RGB_COLORS, // Bitmap contains index values
                 SRCCOPY);       // Do normal copy with stretching
   */
   StretchDIBits(hdc,            // Destination DC
                 0,              // Destination X
                 0,              // Destination Y
                 current_size_X,     // Destination (client area) width
                 current_size_Y,    // Destination (client area) height 
                 0,              // Source X
                 0,              // Source Y
                 current_size_X, // Source (back buffer) width
                 current_size_Y, // Source (back buffer) height
                 lpDIBBuffer,    // Pointer to source (back buffer)
                 pbmi,           // Bitmap info for back buffer
                 DIB_RGB_COLORS, // Bitmap contains index values
                 SRCCOPY);       // Do normal copy with stretching
   
   ReleaseDC(hWnd, hdc);
}

//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл Flip CreateDIBSection() surface (simulated by StretchDIB())            лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

void DIB_flip_surface(void)
{
	//while (1)
	//{
		//serve_queue();

		if (!SAL_is_app_active())
		{
			Sleep(10);
		}
		else
		{
			DIB_refresh_surface();
		    //break;
		}
	//} 
}

DXDEF void WINAPI SAL_blit_surface(void)
{
	DIB_flip_surface();
}
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл Wipe CreateDIBSection() surface                                        лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

void DIB_wipe_surface           (S32        surface,//)
                                 U32        color)
{
   memset(lpDIBBuffer,
          color,
          current_size_X * current_size_Y * (current_bpp / 8));
}

//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл Return pointer to CreateDIBSection() surface                           лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

void DIB_lock_surface         (S32        surface,//)
                               U8       **ptr,
                               S32       *pitch)
{
   if (ptr != NULL)
      {
      *ptr = lpDIBBuffer;
      }

   if (pitch != NULL)
      {
      *pitch = current_size_X * (current_bpp / 8);
      }
}

//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл Release pointer to CreateDIBSection() surface                          лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

void DIB_release_surface        (S32        surface,//)
                                 S32        perform_flip)
{
   if (perform_flip)
      {
      SAL_flip_surface();
      }
}

//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл Start up SAL services                                                  лл
//лл                                                                        лл
//лл Initialize preferences                                                 лл
//лл                                                                        лл
//лл Non-zero: Success                                                      лл
//лл        0: Failure                                                      лл
//лл       -1: Attempt to launch multiple instances                         лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

DXDEF S32 WINAPI SAL_startup(BOOL FileLog)
{
   log_to_file = FileLog;
   strcpy(debug_log_filename, "debug.txt");

   app_active             =  FALSE;  // App not activated yet
   app_terminated         =  FALSE;  // App not terminated yet
   app_minimized          =  FALSE;  // App not minimized by default

   show_count             =  0;      // Mouse cursor show count
   cursor_state           =  1;      // (Windows cursor is on by default)
   constrain_state        =  0;      // Mouse is not constrained to window
   constrain_request      =  0;      // Mouse should not be constrained at next movement

   area.width             = -1;

   focus_callback         =  NULL;
   window_callback        =  NULL;
   hWnd                   =  NULL;
   hHook                  =  NULL;
   mode_change_request    =  FALSE;
   hSem                   =  NULL;

   DIB_active             =  FALSE;

   mode_change_allowed    =  FALSE;
   current_window_mode    =  SAL_FULLSCREEN;
   current_bpp            =  0;
   current_size_X         =  0;
   current_size_Y         =  0;

   palette_change_request = FALSE;

   //
   // Get original mouse constraint area (normally entire screen)
   //
	#ifndef DISABLED_BEHAVIOR
	  GetClipCursor(&unconstrained_rect);
	#endif
   //
   // Allocate palette and bitmap structures at startup time to avoid
   // memory fragmentation during mode switches
   //
   // Clear palette structure to black to avoid screen flash when
   // setting initial mode
   //

   pLogPal = (LOGPALETTE *) 
             malloc (sizeof(LOGPALETTE) + (sizeof(PALETTEENTRY) * 256));

   if (pLogPal == NULL)
      {
      return 0;
      }

   memset(pLogPal,0, sizeof(LOGPALETTE) + (sizeof(PALETTEENTRY) * 256));

   pLogPal->palVersion    = 0x300;
   pLogPal->palNumEntries = 256;

   pbmi = (BITMAPINFO *) 
          malloc(sizeof (BITMAPINFOHEADER) + (sizeof (RGBQUAD) * 256));

   if (pbmi == NULL)
      {
      free(pLogPal);
      return 0;
      }
 
   memset(pbmi, 0, sizeof (BITMAPINFOHEADER) + (sizeof (RGBQUAD) * 256));

   pbmih = &(pbmi->bmiHeader);

   return TRUE;
}

//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл System Abstraction Layer exit function                                 лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

DXDEF void WINAPI SAL_shutdown(void)
{
//	if (OldWindowProc)
//	{
//		SetWindowLong( hWnd, GWL_WNDPROC, (LONG)OldWindowProc);
//		OldWindowProc = NULL;
//	}

   if (DIB_active)
      {
      DIB_shutdown();
      }

   if (pbmi != NULL)
      {
      free(pbmi);
      pbmi = NULL;
      }

   if (pLogPal != NULL)
      {
      free(pLogPal);
      pLogPal = NULL;
      }
}

//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл Set SAL operational preferences and policies                           лл
//лл                                                                        лл
//лл May be called by applications which need to alter the default          лл
//лл behavior of the SAL system                                             лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

DXDEF S32 WINAPI SAL_set_preference(U32 number, S32 value)
{
   S32 old;

   old = SAL_preference[number];

   SAL_preference[number] = value;

   return old;
}

//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл Get SAL operational preferences and policies                           лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

DXDEF S32 WINAPI SAL_get_preference(U32 number)
{
   return SAL_preference[number];
}

//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл Monochrome debug printf() function                                     лл
//лл                                                                        лл
//лл Text written to logfile if WIN.INI / DOS environment option enabled    лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

DXDEF void __cdecl SAL_debug_printf(char *fmt, ...)
{
   static char work_string[4096];
   FILE       *log;

   if ((fmt == NULL) || (strlen(fmt) > sizeof(work_string)))
      {
      strcpy(work_string, "(String missing or too large)");
      }
   else
      {
      va_list ap;

      va_start(ap, 
               fmt);

      vsprintf(work_string, 
               fmt, 
               ap);

      va_end  (ap);
      }

   if (log_to_file)
      {
      log = fopen(debug_log_filename,"a+t");
      
      if (log != NULL)
         {
         fprintf(log,"%s\n",work_string);
         fclose(log);
         }
      }

   OutputDebugString(work_string);
}

//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл Message box display function                                           лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

DXDEF void __cdecl SAL_error_box(C8 *caption, C8 *fmt, ...)
{
   static char work_string[4096];
   S32         restore_cursor;

   restore_cursor = cursor_state;

   if (!cursor_state)
      {
		#ifndef DISABLED_BEHAVIOR
			ShowCursor(1);
		#endif
      }

   if ((fmt == NULL) || (strlen(fmt) > sizeof(work_string)))
      {
      strcpy(work_string, "(String missing or too large)");
      }
   else
      {
      va_list ap;

      va_start(ap, 
               fmt);

      vsprintf(work_string, 
               fmt, 
               ap);

      va_end  (ap);
      }

   SAL_debug_printf("%s\n",work_string);

   //
   // If DirectDraw active, display GDI surface for dialog box
   // 

   //
   // If in 8-bpp mode, save current palette and switch to standard palette
   // 

   if (caption == NULL)
      {
      MessageBox(hWnd, 
                 work_string,
                 "SAL Error", 
                 MB_OK);
      }
   else
      {
      MessageBox(hWnd, 
                 work_string,
                 caption,
                 MB_OK);
      }

   if (!restore_cursor)
      {
		#ifndef DISABLED_BEHAVIOR
			ShowCursor(0);
		#endif
      }
}

DXDEF BOOL WINAPI SAL_set_main_window(HWND hWindow)
{
	hWnd = hWindow;

	app_active = TRUE;

//	OldWindowProc = (WNDPROC)SetWindowLong( hWnd, GWL_WNDPROC, (LONG)MyWndProc);

//	if (!OldWindowProc)
//		return FALSE;

	return TRUE;
}
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл Set display mode and window size                                       лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

DXDEF S32        WINAPI SAL_set_display_mode        (S32   display_size_X,//)
                                                     S32   display_size_Y,
                                                     S32   display_bpp,
                                                     S32   initial_window_mode,
                                                     S32   allow_mode_switch)
{
   S32 result;

   //
   // Shut down current video mode
   //

   if (DIB_active)
   {
		  DIB_shutdown();
   }

   //
   // Hide or show mouse cursor, as appropriate
   //

   if (show_count < 1)
      {
      //
      // Hide mouse cursor if it's currently visible
      //

      if (cursor_state)
         {
         cursor_state = 0;
			#ifndef DISABLED_BEHAVIOR
				ShowCursor(0);
			#endif
			
         }
      }
   else
      {
      //
      // Show mouse cursor if it's currently hidden
      //

      if (!cursor_state)
         {
         cursor_state = 1;
			#ifndef DISABLED_BEHAVIOR
				ShowCursor(1);
			#endif
         }
      }

   //
   // Start up new video mode
   //

   if (initial_window_mode == SAL_FULLSCREEN)
      {
      //
      // SAL_FULLSCREEN: Set up fullscreen video mode using DirectDraw
      //

      }
   else if (initial_window_mode == SAL_WINDOW)
      {
      //
      // SAL_WINDOW: Set up window using CreateDIBSection()
      //

      result = DIB_startup(display_size_X,
                           display_size_Y,
                           display_bpp);
	}
	else if (initial_window_mode == SAL_TRY_FULLSCREEN)
	{
		assert(0);

      //
      // SAL_TRY_FULLSCREEN: Use DirectDraw if possible/available, otherwise
      //                     fall back automatically to CreateDIBSection()
      //

      }
   else
      {
      //
      // Unknown window mode, return failure
      //

      return FALSE;
      }

   //
   // If successful, update global variables to reflect new video mode
   //

   if (result)
      {
      mode_change_allowed = allow_mode_switch;
      current_window_mode = initial_window_mode;
      current_bpp         = display_bpp;
      current_size_X      = display_size_X;
      current_size_Y      = display_size_Y;
      }

   return result;
}

//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл Return current window mode                                             лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

DXDEF S32 WINAPI  SAL_window_status           (void)
{
   return current_window_mode;
}

//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл Return area of client window in screen coordinates                     лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

DXDEF void WINAPI SAL_client_area             (SAL_WINAREA *area)
{
   RECT *rect;

   if (DIB_active)
      {
      rect = client_screen_rect();

      area->x      = rect->left;
      area->y      = rect->top;
      area->width  = rect->right  - rect->left + 1;
      area->height = rect->bottom - rect->top  + 1;
      }

}

//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл Return area of entire window in screen coordinates                     лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

DXDEF void WINAPI SAL_window_area             (SAL_WINAREA *area)
{
   RECT window_rect;

   if (DIB_active)
      {
      GetWindowRect(hWnd, &window_rect);

      area->x      = window_rect.left;
      area->y      = window_rect.top;
      area->width  = window_rect.right  - window_rect.left + 1;
      area->height = window_rect.bottom - window_rect.top  + 1;
      }

}

//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл Return TRUE if app has input focus                                     лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

DXDEF S32 WINAPI  SAL_is_app_active           (void)
{
   return app_active;
}

//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл Register app function to be called when focus gained or lost           лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

DXDEF SALFOCUSCB WINAPI SAL_register_focus_callback (SALFOCUSCB fn)
{
   SALFOCUSCB old;

   old = focus_callback;

   focus_callback = fn;

   return old;
}

//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл Register application WNDPROC function                                  лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

DXDEF WNDPROC WINAPI SAL_register_WNDPROC     (WNDPROC fn)
{
   WNDPROC old;

   old = window_callback;

   window_callback = fn;

   return old;
}

//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл Service Windows message queue, handling any mode change request        лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

DXDEF void WINAPI SAL_serve_message_queue(void)
{
   S32 result;

   serve_queue();

   if (mode_change_request && mode_change_allowed)
      {
      mode_change_request = 0;   

      result = SAL_set_display_mode(
                  current_size_X,
                  current_size_Y,
                  current_bpp,
                  current_window_mode ^ (SAL_FULLSCREEN ^ SAL_WINDOW),
                  TRUE);

      //
      // If window mode toggle did not succeed, restore original mode
      //

      if (!result)
         {
         SAL_debug_printf("SAL: SAL_set_display_mode() failed, restoring old mode\n");

         SAL_set_display_mode(
            current_size_X,
            current_size_Y,
            current_bpp,
            current_window_mode,
            TRUE);
         }
      }
}

//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл Set a single palette entry                                             лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

DXDEF void WINAPI SAL_set_palette_entry      (S32        index,//)
                                              SAL_RGB32 *entry,
                                              S32        wait_flag)
{
   //
   // If we're not in 8BPP mode, bail out
   // 

   if (current_bpp != 8)
      {
      return;
      }

   //
   // Update global palette state
   //

   palette_state[index] = *entry;

   pLogPal->palPalEntry[index].peRed   = pbmi->bmiColors[index].rgbRed   = (unsigned char) entry->r;
   pLogPal->palPalEntry[index].peGreen = pbmi->bmiColors[index].rgbGreen = (unsigned char) entry->g;
   pLogPal->palPalEntry[index].peBlue  = pbmi->bmiColors[index].rgbBlue  = (unsigned char) entry->b;
   pLogPal->palPalEntry[index].peFlags = NULL;

   //
   // Update DirectDraw palette, if appropriate
   //


   if (DIB_active)
      {
      if (hPalette != NULL)
         {
         DeleteObject(hPalette);
         }

      hPalette = CreatePalette(pLogPal);

      palette_change_request = TRUE;
      }
}

//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл Retrieve a single palette entry                                        лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

DXDEF void WINAPI SAL_get_palette_entry      (S32        index,//)
                                              SAL_RGB32 *entry)
{
   *entry = palette_state[index];
}

//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл Set a range of palette entries                                         лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

DXDEF void WINAPI SAL_set_palette_range      (S32        index,//)
                                              S32        num_entries,
                                              SAL_RGB32 *entry_list,
                                              S32        wait_flag)
{
   S32 i;
   S32 j;

   //
   // If we're not in 8BPP mode, bail out
   // 

   if (current_bpp != 8)
      {
      return;
      }

   //
   // Update global palette state
   //

   for (i=0; i < num_entries; i++)
      {
      palette_state[index+i] = entry_list[i];
      }

   for (i=0, j=index; i < num_entries; i++, j++)
      {
      pLogPal->palPalEntry[j].peRed   = pbmi->bmiColors[j].rgbRed   = (unsigned char) entry_list[i].r;
      pLogPal->palPalEntry[j].peGreen = pbmi->bmiColors[j].rgbGreen = (unsigned char) entry_list[i].g;
      pLogPal->palPalEntry[j].peBlue  = pbmi->bmiColors[j].rgbBlue  = (unsigned char) entry_list[i].b;
      pLogPal->palPalEntry[j].peFlags = NULL;
      }

   //
   // Update DirectDraw palette, if appropriate
   //


   if (DIB_active)
      {
      if (hPalette != NULL)
         {
         DeleteObject(hPalette);
         }

      hPalette = CreatePalette(pLogPal);

      palette_change_request = TRUE;
      }
}
                                   
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл Retrieve a range of palette entries                                    лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

DXDEF void WINAPI SAL_get_palette_range      (S32        index,//)
                                              S32        num_entries,
                                              SAL_RGB32 *entry_list)
{
   for (int i=0; i < num_entries; i++)
      {
      entry_list[i] = palette_state[index+i];
      }
}

//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл Make back surface visible                                              лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

DXDEF void WINAPI SAL_flip_surface           (void)
{
	if (DIB_active)
      {
      DIB_flip_surface();
      }
}

//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл Clear a surface to a desired color                                     лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

DXDEF void WINAPI SAL_wipe_surface           (S32        surface,//)
                                              U32        color)
{
	if (DIB_active)
      {
      DIB_wipe_surface(surface, color);
      }
}

//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл Request a pointer to a surface's memory block                          лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

DXDEF void WINAPI SAL_lock_surface           (S32        surface,//)
                                              U8       **ptr,
                                              S32       *pitch)
{
	if (DIB_active)
      {
      DIB_lock_surface(surface, ptr, pitch);
      }
}

//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл Release surface memory pointer, optionally performing page flip        лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

DXDEF void WINAPI SAL_release_surface        (S32        surface,//)
                                              S32        perform_flip)
{
	if (DIB_active)
      {
      DIB_release_surface(surface, perform_flip);
      }
}

//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл Request a pointer to a region of surface memory                        лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

DXDEF void WINAPI SAL_lock_region            (S32        surface,//)
                                              SAL_REGION region,
                                              U8       **ptr,
                                              S32       *pitch)
{

}

//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл Release surface region pointer                                         лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

DXDEF void WINAPI SAL_release_region         (S32        surface,//)
                                              SAL_REGION region)
{
}

//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл Show system mouse cursor                                               лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

DXDEF void WINAPI SAL_show_system_mouse      (void)
{
   ++show_count;

   if (show_count == 1)
      {
      //
      // Show count has become 1, so show mouse cursor if it's currently 
      // hidden
      //

      if (!cursor_state)
         {
         cursor_state = 1;
			#ifndef DISABLED_BEHAVIOR
				ShowCursor(1);
			#endif
         }
      }
}

//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл Hide system mouse cursor                                               лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

DXDEF void WINAPI SAL_hide_system_mouse      (void)
{
   --show_count;

   if (show_count == 0)
      {
      //
      // Show count has become 0, so hide mouse cursor if it's currently
      // visible
      //

      if (cursor_state)
         {
         cursor_state = 0;
			#ifndef DISABLED_BEHAVIOR
				ShowCursor(0);
			#endif
         }
      }
}

//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл Constrain mouse to limits of client area window in DIB mode            лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

DXDEF void WINAPI SAL_constrain_mouse(void)
{
   constrain_state = 1;

   if (DIB_active)
      {
      constrain_request = 1;
      }
}

//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл Unconstrain mouse                                                      лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

DXDEF void WINAPI SAL_unconstrain_mouse(void)
{
   constrain_state = 0;

   if (DIB_active)
      {
      #ifndef DISABLED_BEHAVIOR
		ClipCursor(&unconstrained_rect);
	  #endif
      constrain_request = 0;
      }
}

//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
//лл                                                                        лл
//лл Get pixel format in current mode                                       лл
//лл                                                                        лл
//лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

DXDEF void WINAPI SAL_get_pixel_format(S32 *pixel_pitch, //)
                                       S32 *bytes_per_pixel,
                                       S32 *R_shift,
                                       U32 *R_mask,
                                       S32 *R_width,
                                       S32 *G_shift,
                                       U32 *G_mask,
                                       S32 *G_width,
                                       S32 *B_shift,
                                       U32 *B_mask,
                                       S32 *B_width)

{
   S32 red_shift;
   U32 red_mask;
   S32 red_width;
   S32 grn_shift;
   U32 grn_mask;
   S32 grn_width;
   S32 blu_shift;
   U32 blu_mask;
   S32 blu_width;
   S32 i;

   //
   // Handle palettized (8 BPP) modes
   //
   // Write 8-bit pixel pitch and visible bytes/pixel fields
   // If we're in 8BPP mode, exit without altering RGB fields
   //

   if (current_bpp == 8)
      {
      if (pixel_pitch     != NULL) *pixel_pitch     = 1;
      if (bytes_per_pixel != NULL) *bytes_per_pixel = 1;

      return;
      }

   //
   // Handle hi-color (16+ BPP) modes
   //
   // If using DirectDraw, do a GetPixelFormat() call
   // 
   // If using CreateDIBSection(), return mask values used to create DIB
   // 

   if (DIB_active)
      {
      if (pixel_pitch     != NULL) *pixel_pitch     = (current_bpp / 8);
      if (bytes_per_pixel != NULL) *bytes_per_pixel = (current_bpp / 8);

      red_mask = DIB_R_bitmask;
      grn_mask = DIB_G_bitmask;
      blu_mask = DIB_B_bitmask;
      }

   //
   // Derive shift, width values from masks
   //

   for (i=31; i >= 0; i--)
      {
      if (red_mask & (1 << i))
         {
         red_shift = i;
         }

      if (grn_mask & (1 << i))
         {
         grn_shift = i;
         }

      if (blu_mask & (1 << i))
         {
         blu_shift = i;
         }
      }

   for (i=0; i <= 31; i++)
      {
      if (red_mask & (1 << i))
         {
         red_width = i - red_shift + 1;
         }

      if (grn_mask & (1 << i))
         {
         grn_width = i - grn_shift + 1;
         }

      if (blu_mask & (1 << i))
         {
         blu_width = i - blu_shift + 1;
         }
      }
   //
   // Pass all requested values back to the caller
   //

   if (R_shift != NULL) *R_shift = red_shift;
   if (G_shift != NULL) *G_shift = grn_shift;
   if (B_shift != NULL) *B_shift = blu_shift;

   if (R_mask  != NULL) *R_mask  = red_mask;
   if (G_mask  != NULL) *G_mask  = grn_mask;
   if (B_mask  != NULL) *B_mask  = blu_mask;

   if (R_width != NULL) *R_width = red_width;
   if (G_width != NULL) *G_width = grn_width;
   if (B_width != NULL) *B_width = blu_width;
}

DXDEF HBITMAP WINAPI GetDIBHandle(void)
{
   return hDIB;
}

DXDEF void WINAPI SAL_GetBackBufferDC(HDC *dc)
{
	if (DIB_active) 
	{
		*dc = CreateCompatibleDC ( 0 );
		DefaultBitmap = SelectBitmap ( *dc, hDIB );
	}
}

DXDEF void WINAPI SAL_ReleaseBackBufferDC(HDC dc)
{

	if (DIB_active) 
	{
		SelectBitmap ( dc, DefaultBitmap );
		DeleteDC ( dc );
		DeleteObject ( DefaultBitmap );
    }
}
