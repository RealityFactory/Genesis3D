/****************************************************************************************/
/*  FONT.H                                                                              */
/*                                                                                      */
/*  Author: Thom Robertson                                                              */
/*  Description: Bitmapped font support interface                                       */
/*               This implementation supports any TrueType fonts provided by Windows    */
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
#ifndef GE_FONT_H
#define GE_FONT_H

// includes
#include "genesis.h"
#include "basetype.h"
#include "bitmap.h"
// **************************
// to use this API:

// 2.  geFont_CreateFont().

// 3.  (Optionally) geFont_AddCharacters().
// 3A. Otherwise, IF you intend to use geFont_DrawText(), call geFont_AddBitmapBuffer().

// 4.  Between geEngine_BeginFrame() and geEngine_EndFrame(), and after geEngine_RenderWorld(),
//       geFont_DrawText(). You may call geFont_DrawTextToBitmap() anytime, though.

// 5.  When finished, geFont_Destroy().



//*************************************************************************************** 
// these are bit flags for _DrawText(). Currently only _WORDWRAP is implemented, and without
// it, the function will still wrap, just not on word boundaries.
// Note that these will fail for non ascii fonts.
#define GE_FONT_WRAP                0x00000001  // wrap to fit inside the drawing rect
#define GE_FONT_WORDWRAP            0x00000002  // wrap on word boundaries
#define GE_FONT_JUST_RETURN_FIT     0x00000004  // returns number of characters that fit in drawing rectangle, WITHOUT drawing anything.
#define GE_FONT_JUSTIFY_RIGHT       0x00000008
#define GE_FONT_JUSTIFY_CENTER      0x00000010

#ifdef __cplusplus
extern "C" {
#endif

// opaque structure headers.
typedef struct geFont geFont;			// an instance of a font


//*************************************************************************************** 
GENESISAPI geFont *GENESISCC geFont_Create(const geEngine *Engine, const char *fontNameString, 
                                               const int fontSize,
											   const int fontWeight , const geBoolean antialiased) ;
   // Creates a font, and returns a pointer to it.
   // Pass in the string name of the TrueType font (case sensitive), and the height in pixels.
   
   // ARGUMENTS:
   // fontNameString - char pointer to a string containing the case sensitive name of the font.
   // fontSize - the pixel height of the requested font.

   // RETURNS:
   // success: pointer to the newly created font.
   // failure: NULL.

   // NOTE: the new font set has NO actual characters in it at first.  You must add characters
   // to it with the _AddCharacters() function before you can use the font.
   // NOTE: all fonts start out with a grayscale palette, with the range 0 to 128.

//*************************************************************************************** 
GENESISAPI void GENESISCC geFont_CreateRef(geFont *font);


//*************************************************************************************** 
GENESISAPI void GENESISCC geFont_Destroy(geFont **font);
   // destroys a font.

   // ARGUMENTS:
   // font - pointer to the font to be destroyed.

   // RETURNS:
   // nothing.

//*************************************************************************************** 
GENESISAPI geBoolean GENESISCC geFont_AddCharacters(geFont *font, 
                                                  unsigned char leastIndex, 
                                                  unsigned char mostIndex
                                                  );
   // Adds a set of characters to the font defined by the ascii range passed in 
   // (leastIndex and mostIndex, inclusive).

   // ARGUMENTS:
   // font - pointer to the font to add characters to.
   // e - pointer to a valid geEngine.
   // leastIndex and mostIndex - the ASCII range of characters to add.
   // cellBuffer - an allocated hunk of ram to temproarily store the character image
   // bufferSize - length of the above buffer

   // RETURNS:
   // success: GE_TRUE.
   // failure: GE_FALSE.

   // NOTES:
   // This is the function that actually uses the
   // Win32 GetGlyphOutline() function to draw the character onto a geBitmap, which can be
   // blitted to the screen.


//*******************************************************************************
GENESISAPI void GENESISCC geFont_DestroyBitmapBuffer( geFont *font );
   // destroys any valid "scratch-pad" buffer attached to the geFont.
   // ARGUMENTS:
   // font - pointer to the geFont.
   //
   // NOTES:
   // you'll rarely need to call this function; it's called by geFont_Destroy() anyway.
   // Calling this function with a geFont that has no initialized buffer doesn't
   // hurt anything.

//*******************************************************************************
GENESISAPI geBoolean GENESISCC geFont_AddBitmapBuffer(
                                  geFont *font, const uint32 width, const uint32 height);
   // Adds a geBitmap to the geFont, to be used as a temporary "scratch-pad".  This is
   // required for using geFont_DrawText() when no characters have been added.

   // ARGUMENTS:
   // font - pointer to the geFont to add a buffer to.
   // width and height - the size of the buffer to create.  Make sure this size is >=
   // the biggest rectagle of text you'll want to write to the screen using this geFont
   // and DrawText().

   // RETURNS:
   // success: GE_TRUE.
   // failure: GE_FALSE.

   // NOTES:
   // You don't need to call this function IF you _AddCharacters() to this geFont.
   // You call this function for each geFont you need to use.  geFont's don't share buffers.
   // if you call this function on a geFont that already has a valid buffer, the buffer is
   // destroyed, and replaced by the new one.

//*************************************************************************************** 
GENESISAPI geBoolean GENESISCC geFont_DrawText(geFont *font, const char *textString, 
                                           const GE_Rect *Rect, const GE_RGBA *Color, 
                                           uint32 flags, const GE_Rect *clipRect);
   // This is the function you put between geEngine_BeginFrame() and _EndFrame(), the function
   // that draws text to the screen.

   // ARGUMENTS:
   // font - pointer to the font to draw with.  IF the font has NO characters in it
   //  (added by geFont_AddCharacters() ) then a different, more windows-intensive way is
   //  used to draw out the characters.
   // textString - pointer to the text string to output to the screen.
   // Rect - screen rectangle to place the text within.
   // Color - RGB color the text should be.
   // flags - a bitfield of GE_FONT_ values.
   // clipRect - pointer to a screen rectangle to clip the text to.  MAY BE NULL, in which
   // case the text is only clipped by the boundaries of the screen.

   // RETURNS:
   // success: GE_TRUE.
   // failure: GE_FALSE.

   // NOTES:
   // Assuming you've added characters to the font, characters which have NOT been added
   // WILL cause an assert if you try to draw them.  
   // Only GE_FONTSET_WORDWRAP is meaningfull right now.  Using any other flags will cause
   // an assert.
   // As stated above, you can use an entirely different way of creating a string, by
   // making a font with no characters in it.  This
   // jumps through Windows DIB hoops, and draws the text in a non-anti-aliased, but
   // (hopefully) more unicode-tolerant way (DrawText() ).


//*************************************************************************************** 
GENESISAPI geBoolean GENESISCC geFont_DrawTextToBitmap(geFont *font, const char *textString, 
                                           const GE_Rect *Rect, const GE_RGBA *Color, 
                                           uint32 flags, const GE_Rect *clipRect,
                                           geBitmap *targetBitmap);
   // This is the function you put between geEngine_BeginFrame() and _EndFrame(), the function
   // that draws text to the screen.

   // ARGUMENTS:
   // font - pointer to the font to draw with.  IF the font has NO characters in it
   //  (added by geFont_AddCharacters() ) then a different, more windows-intensive way is
   //  used to draw out the characters.
   // textString - pointer to the text string to output to the screen.
   // Rect - screen rectangle to place the text within.
   // Color - RGB color the text should be.
   // flags - a bitfield of GE_FONT_ values.
   // clipRect - pointer to a screen rectangle to clip the text to.  MAY BE NULL, in which
   // case the text is only clipped by the boundaries of the screen.
   // targetBitmap - pointer to a target bitmap to draw the text into.  MAY NOT BE NULL,
   // and MUST BE GE_PIXELFORMAT_8BIT.

   // RETURNS:
   // success: GE_TRUE.
   // failure: GE_FALSE.

   // NOTES:
   // Assuming you've added characters to the font, characters which have NOT been added
   // WILL cause an assert if you try to draw them.  
   // Only GE_FONTSET_WORDWRAP is meaningfull right now.  Using any other flags will cause
   // an assert.
   // As stated above, you can use an entirely different way of creating a string, by
   // making a font with no characters in it.  This
   // jumps through Windows DIB hoops, and draws the text in a non-anti-aliased, but
   // (hopefully) more unicode-tolerant way (DrawText() ).
   // The Color argument is will be used to modify the existing palette of the targetBitmap
   // passed in.  Therefore, you won't be able to _DrawTextToBitmap() a red piece of text,
   // then a green piece, then a blue piece.  You'll end up with three lines of blue text.


//*************************************************************************************** 
GENESISAPI int32 GENESISCC geFont_GetStringPixelWidth (geFont *font, const char *textString);
GENESISAPI int32 GENESISCC geFont_GetStringPixelHeight(geFont *font, const char *textString);
   // These two functions return the pixel width and height of the string passed in.

   // ARGUMENTS:
   // font - pointer to the font to draw with.
   // textString - pointer to the text string to output to the screen.

   // RETURNS:
   // success: a positive value in pixels.  IF the text passed in contains characters
   //          which haven't been added to the font yet, BUT other characters HAVE
   //          been added, the function asserts.
   // failure: -1.
   // NOTES:
   // these two functions assume no text wrapping!

//*************************************************************************************** 
GENESISAPI geBitmap* GENESISCC geFont_GetBuffer(geFont *font);
   // This function returns a pointer to the drawing buffer contained by the font.

   // ARGUMENTS:
   // font - pointer to the font.

   // RETURNS:
   // a valid pointer to a geBitmap, OR NULL, signifying that the buffer wasn't initialized.


//*************************************************************************************** 
GENESISAPI geBoolean GENESISCC geFont_GetCharMap(geFont *font, uint8 character, GE_Rect *Rect, 
												 geBitmap **targetBitmap, int32 *fullWidth, int32 *fullHeight, 
												 int32 *offsetX, int32 *offsetY);

//*************************************************************************************** 
GENESISAPI void GENESISCC geFont_EnableAntialiasing(geFont *font, const geBoolean anti);
//*************************************************************************************** 
GENESISAPI geBoolean GENESISCC geFont_IsAntialiased(geFont *font);

#ifdef __cplusplus
}
#endif

#endif

