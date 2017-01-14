/****************************************************************************************/
/*  FONT.C                                                                              */
/*                                                                                      */
/*  Author: Thom Robertson                                                              */
/*  Description: Bitmapped font support implementation                                  */
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
/*Genesis3D Version 1.1 released November 15, 1999                            */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#define	WIN32_LEAN_AND_MEAN
#pragma warning(disable : 4201 4214 4115)
#include <windows.h>
#include <windowsx.h>
#pragma warning(default : 4201 4214 4115)

#include "extbox.h"
#include "ram.h"
#include "wgClip.h"

#include <assert.h>
#include <string.h>

#include "font.h"

#pragma warning (disable:4514)	// unreferenced inline function (caused by Windows)

//*************************************************************************************** 
GENESISAPI geBoolean  GENESISCC geFont_TestDraw(geFont *font, int16 x, int16 y, int16 index);
   // This is a debugging function that you shouldn't have to use.  It outputs an entire
   // bitmap full of characters to the screen and the x,y location.

   // ARGUMENTS:
   // font - pointer to the font to draw with.
   // x and y - screen location to draw the upper left corner of the bitamp at.
   // index - which bitmap to draw.

   // RETURNS:
   // success: GE_TRUE
   // failure: GE_FALSE

   // NOTES:
   // Since fonts can be of an arbitrary point size, an arbitrary amount of 256x256
   // bitmaps is required to represent all of the characters.  The index argument lets you
   // pick which bitmap in the list to display.



//*******************************************************************************
typedef struct geFontBitmap
{
   void *next;
   geBitmap *map;
   int16 freeX, freeY;  // place the next character here.

} geFontBitmap;

//*******************************************************************************
typedef struct geFontCharacterRecord
{
   geFontBitmap *bitmapUsed;  // NULL == not initialized yet.
   GE_Rect bitmapRect;
   int32 offsetX, offsetY, fullWidth;

} geFontCharacterRecord;

//*******************************************************************************
typedef struct geFont 
{
   char fontNameString[64];
   int16 fontSize;
   int16 fontWeight;
   geFontBitmap *bitmapList;
   geFontCharacterRecord characterArray[256];
   const geEngine *Engine;
   geBitmap *buffer;

   geBoolean antialiased;

   int32 refCount;

} geFont;



GENESISAPI geBoolean GENESISCC geFont_DrawUsingDIB(geFont *font, const char *textString, 
                                                   RECT box, geBitmap *targetBitmap,
                                                   const GE_RGBA *Color, uint32 flags);

//*******************************************************************************
//*******************************************************************************
GENESISAPI geFont *GENESISCC geFont_Create(const geEngine *Engine, const char *fontNameString, 
                                               const int fontSize, const int fontWeight,
                                               const geBoolean anti)
{
   geFont *font;
   int i;

   // create a new font
   font = GE_RAM_ALLOCATE_STRUCT(geFont);
   assert(font);

   // initalize the new font to having no data in it.
   font->bitmapList = NULL;
   for (i = 0; i < 256; i++)
      font->characterArray[i].bitmapUsed = NULL;

   strncpy(font->fontNameString, fontNameString,64);
   font->fontNameString[63] = 0;

   font->fontSize = fontSize;
   font->fontWeight = fontWeight;

   font->Engine = Engine;

   font->buffer = NULL;

   font->refCount = 1;

   font->antialiased = anti;

   return font;
}

//*************************************************************************************** 
GENESISAPI void GENESISCC geFont_CreateRef(geFont *font)
{
   assert(font);
   font->refCount++;
}


//*******************************************************************************
GENESISAPI void GENESISCC geFont_Destroy(geFont **font)
{

   geFontBitmap *curFontBitmap, *tempFontBitmap;

   assert(*font);

   (*font)->refCount--;

   // don't destroy this font if there's still a reference out there.
   if ((*font)->refCount > 0)
      return;

   geFont_DestroyBitmapBuffer(*font);

   curFontBitmap = (*font)->bitmapList;
   while (curFontBitmap)
   {
      tempFontBitmap = curFontBitmap->next;

      geEngine_RemoveBitmap((geEngine *) (*font)->Engine, curFontBitmap->map);

      geBitmap_Destroy(&(curFontBitmap->map));
      geRam_Free(curFontBitmap);
      curFontBitmap = tempFontBitmap;
   }

   geRam_Free(*font);
   (*font) = NULL;

}

//****************************************************************************
FIXED PASCAL NEAR FixedFromDouble(double d)
{
    long l;

    l = (long) (d * 65536L);
    return *(FIXED *)&l;
}

//*******************************************************************************
static	void	IdentityMat(LPMAT2 lpMat)
{
    lpMat->eM11 = FixedFromDouble(1);
    lpMat->eM12 = FixedFromDouble(0);
    lpMat->eM21 = FixedFromDouble(0);
    lpMat->eM22 = FixedFromDouble(1);
}

//*******************************************************************************
void PlaceLetter(int16 locX, int16 locY, geFont *font, geFontBitmap *fontMap, 
                 uint32 asciiValue, unsigned char *cellBuffer, DWORD bufferSize)
{
   GLYPHMETRICS glyphMetrics;
   HDC hdc; 
   MAT2 mat2;
   int32 success, bufferReturnSize;
   uint32 x,y;
   unsigned char *destBits;
   int bufferWidth, possibleH;
   char c;
   int16 mapWidth, mapHeight;
   HFONT win32Font, win32OldFont;
   geBitmap *lock;
   geBitmap_Info Info;
   geBitmap_Info SecondaryInfo;

   uint32 ggoFormat;

   IdentityMat(&mat2);


   mapWidth  = geBitmap_Width (fontMap->map);
   mapHeight = geBitmap_Height(fontMap->map);

	win32Font = CreateFont( -1 * font->fontSize,
    					    0,0,0, font->fontWeight,
    					    0,0,0,0,OUT_TT_ONLY_PRECIS ,0,0,0, font->fontNameString);

   hdc = GetDC(GetDesktopWindow());
   assert(hdc);

   c = (char)asciiValue;

   if (font->antialiased)
      ggoFormat = GGO_GRAY8_BITMAP;
   else
      ggoFormat = GGO_BITMAP;                        

   win32OldFont = SelectObject( hdc, win32Font); 
   bufferReturnSize    = GetGlyphOutline( hdc, asciiValue, ggoFormat, 
                                 &glyphMetrics, 0, NULL, &mat2);
   success    = GetGlyphOutline( hdc, asciiValue, ggoFormat, 
                                 &glyphMetrics, bufferSize, cellBuffer, &mat2);
   assert(GDI_ERROR != success);
//   success    = GetGlyphOutline( hdc, asciiValue, GGO_METRICS, &glyphMetrics, 0, NULL, &mat2);
//   assert(GDI_ERROR != success);
   SelectObject( hdc, win32OldFont); 

   ReleaseDC(GetDesktopWindow(),hdc);

   DeleteObject(win32Font);

   // get buffer width
//   bufferWidth = glyphMetrics.gmCellIncX - glyphMetrics.gmptGlyphOrigin.x;
//   bufferWidth = glyphMetrics.gmBlackBoxX;
   if (glyphMetrics.gmBlackBoxY)
   {
      possibleH = glyphMetrics.gmBlackBoxY-1;
      do 
      {
         possibleH++;
         bufferWidth = bufferReturnSize / possibleH;
      } while (bufferWidth * possibleH != bufferReturnSize);
   }
   else
   bufferWidth = glyphMetrics.gmBlackBoxX;

   if (GGO_BITMAP == ggoFormat)
   {
      bufferWidth/= 8;
      if (bufferWidth <= 0)
         bufferWidth = 1;
   }


   // make sure it's double-word aligned.
   while (bufferWidth%4)
      bufferWidth++;

   font->characterArray[asciiValue].bitmapUsed = fontMap;
   font->characterArray[asciiValue].bitmapRect.Left   = fontMap->freeX;
   font->characterArray[asciiValue].bitmapRect.Top    = fontMap->freeY;
   font->characterArray[asciiValue].bitmapRect.Right  = fontMap->freeX +
                                                   glyphMetrics.gmBlackBoxX;
   font->characterArray[asciiValue].bitmapRect.Bottom = fontMap->freeY +
                                                         glyphMetrics.gmBlackBoxY;
   font->characterArray[asciiValue].offsetX   = glyphMetrics.gmptGlyphOrigin.x;
   font->characterArray[asciiValue].offsetY   = glyphMetrics.gmptGlyphOrigin.y;
   font->characterArray[asciiValue].fullWidth = glyphMetrics.gmCellIncX;

   glyphMetrics.gmptGlyphOrigin.y  = glyphMetrics.gmptGlyphOrigin.y - glyphMetrics.gmBlackBoxY;


   // lock the fontMap surface...
   success = geBitmap_LockForWrite(	fontMap->map, &lock,
                                    0,0);
   success = geBitmap_GetInfo(lock, &Info, &SecondaryInfo);
   mapWidth  = Info.Stride;

   destBits = geBitmap_GetBits(lock);

   // clear the character space to 0s...
   if (GGO_BITMAP == ggoFormat)                        
   {

      for ( y = 0; 
            y < glyphMetrics.gmBlackBoxY; 
            y++)
      {
         for (x = 0;
              x < glyphMetrics.gmBlackBoxX; 
              x++)
         {
			   uint8 temp, mask;
			   temp = cellBuffer[y * bufferWidth + x/8];
            mask = 1 << (7 - (x % 8));

            if (mask&temp)
               destBits[(y+locY)*mapWidth + (x+locX)] = 255;
            else
               destBits[(y+locY)*mapWidth + (x+locX)] = 0;
         }
      }
   }
   else
   {
      for ( y = 0; 
            y < glyphMetrics.gmBlackBoxY; 
            y++)
      {
         for (x = 0;
              x < glyphMetrics.gmBlackBoxX; 
              x++)
         {
			      int temp;
			      temp = 4 * cellBuffer[y * bufferWidth + x];
			      if (temp> 255)
				      temp = 255;
            destBits[(y+locY)*mapWidth + (x+locX)] = temp;
         }
      }
   }
   success = geBitmap_UnLock(lock);
}

//*******************************************************************************
GENESISAPI geBoolean GENESISCC geFont_AddCharacters(geFont *font, 
                                                  unsigned char leastIndex, 
                                                  unsigned char mostIndex
                                                  )
{
   MAT2 mat2;
   GLYPHMETRICS glyphMetrics;
   HDC hdc;
   DWORD success;
   unsigned char asciiValue;
   int placedLetter;
   int16 mapWidth, mapHeight;
   HFONT win32Font, win32OldFont;
   geFontBitmap *fontBitmap;
   geBitmap_Palette *Palette;
   int i;
   unsigned char *cellBuffer;
   DWORD bufferSize;

   uint32 ggoFormat;

   mat2.eM11.fract = 0;
   mat2.eM11.value = 1;
   mat2.eM12.fract = 0;
   mat2.eM12.value = 0;
   mat2.eM21.fract = 0;
   mat2.eM21.value = 0;
   mat2.eM22.fract = 0;
   mat2.eM22.value = 1;

   // get the character bitmap
   bufferSize = 32768;
   cellBuffer = GE_RAM_ALLOCATE_ARRAY(unsigned char, bufferSize);
   memset(cellBuffer,0xee,bufferSize);


   for (asciiValue = leastIndex; asciiValue <= mostIndex; asciiValue++)
   {
      placedLetter = FALSE;

      if (font->antialiased)
         ggoFormat = GGO_GRAY8_BITMAP;
      else
         ggoFormat = GGO_BITMAP;                        

      // find out the dimensions of the character
   	win32Font = CreateFont( -1 * font->fontSize,
    					    0,0,0, font->fontWeight,
    					    0,0,0,0,OUT_TT_ONLY_PRECIS ,0,0,0, font->fontNameString);

      hdc = GetDC(GetDesktopWindow());
      assert(hdc);

      win32OldFont = SelectObject( hdc, win32Font); 
      success    = GetGlyphOutline( hdc, asciiValue, ggoFormat, 
                                 &glyphMetrics, bufferSize, cellBuffer, &mat2);
      SelectObject( hdc, win32OldFont); 

      ReleaseDC(GetDesktopWindow(),hdc);

      DeleteObject(win32Font);

      fontBitmap = font->bitmapList;
      while (fontBitmap)
      {
         // will it fit into the current bitmap?
         mapWidth  = geBitmap_Width (fontBitmap->map);
         mapHeight = geBitmap_Height(fontBitmap->map);

         // if there's enough vertical space...
         if ((int32)glyphMetrics.gmBlackBoxY <= (mapHeight-1) - fontBitmap->freeY)
         {
            // if there's enough horizontal space...
            if (glyphMetrics.gmCellIncX <= (mapWidth-1) - fontBitmap->freeX)
            {
               // place the letter!
               PlaceLetter(fontBitmap->freeX, fontBitmap->freeY, font,
                           fontBitmap, asciiValue, cellBuffer, bufferSize); 
               placedLetter = TRUE;
               fontBitmap->freeX = fontBitmap->freeX + glyphMetrics.gmBlackBoxX;
            }
            else
            {
               fontBitmap->freeY = fontBitmap->freeY + font->fontSize;
               fontBitmap->freeX = 0;
               // if there's enough vertical space...
               if ((int32)glyphMetrics.gmBlackBoxY <= (mapHeight-1) - fontBitmap->freeY)
               {
                  // if there's enough horizontal space...
                  if (glyphMetrics.gmCellIncX <= (mapWidth-1) - fontBitmap->freeX)
                  {
                     // place the letter!
                     PlaceLetter(fontBitmap->freeX, fontBitmap->freeY, font,
                                 fontBitmap, asciiValue, cellBuffer, bufferSize);
                     placedLetter = TRUE;
                     fontBitmap->freeX = fontBitmap->freeX + glyphMetrics.gmBlackBoxX;
                  }
               }
            }
         }
         fontBitmap = fontBitmap->next;
      }

      if (!placedLetter)
      {
         // gotta create a new font bitmap.
         fontBitmap = GE_RAM_ALLOCATE_STRUCT(geFontBitmap);
         assert(fontBitmap);
         fontBitmap->next = font->bitmapList;
         font->bitmapList = fontBitmap;
         fontBitmap->freeX = 0;
         fontBitmap->freeY = 0;
         fontBitmap->map = geBitmap_Create(256,256, 1, GE_PIXELFORMAT_8BIT); 

         // and a pallette for it.
         Palette = geBitmap_Palette_Create(GE_PIXELFORMAT_32BIT_XRGB, 256);

         for (i = 0; i < 256; i++)
         {
            success = geBitmap_Palette_SetEntryColor(Palette, i, i, i, i, i);
            assert(success);

         }

         success = geBitmap_SetPalette(fontBitmap->map, Palette);
         assert(success);

		success = geBitmap_SetColorKey(fontBitmap->map, TRUE, 0 ,FALSE );


         success = geBitmap_Palette_Destroy(&Palette);         
         assert(success);


         success = geEngine_AddBitmap((geEngine *)font->Engine, fontBitmap->map);
         assert(success);

         PlaceLetter(fontBitmap->freeX, fontBitmap->freeY,  font,
                     fontBitmap, asciiValue, cellBuffer, bufferSize);
         fontBitmap->freeX = fontBitmap->freeX + glyphMetrics.gmBlackBoxX;

     
      }

   }

   geRam_Free(cellBuffer);

   return TRUE;
}

//*******************************************************************************
GENESISAPI geBoolean GENESISCC geFont_DrawText(geFont *font, const char *textString, 
                                           const GE_Rect *Rect, const GE_RGBA *Color, 
                                           uint32 flags, const GE_Rect *clipRect)
{

   int32 x,y,i;
   int32 tempX;//, tempY;
   int32 lineLen;

   uint32 colorArray[256];
   float  fcolorX, fcolorR, fcolorG, fcolorB;

   int32 stringLen;
   int32 currentCharIndex;
   int32 lineEndIndex;
   geFontCharacterRecord *charRec;

   geBoolean success;
   geBoolean longEnough, endOfText;

   GE_Rect artRect;
   RECT box;
   int32 resultX, resultY;

   geFontBitmap *lastBitmap;
   geBitmap_Palette * palette;

   // unimplemented flags which we don't want you to use yet.
   assert(!(flags & GE_FONT_WRAP           ));
   assert(!(flags & GE_FONT_JUST_RETURN_FIT));
   assert(!(flags & GE_FONT_JUSTIFY_RIGHT  ));
   assert(!(flags & GE_FONT_JUSTIFY_CENTER ));

   lastBitmap = NULL;
   x = 0;
   y = 0;
   stringLen = strlen(textString);
   currentCharIndex = 0;

   if (font->bitmapList)  // if this font has character bitmaps...
   {

      while (currentCharIndex < stringLen)
      {
         // skip leading white space for this line.
         while (isspace(textString[currentCharIndex]) && currentCharIndex < stringLen)
            currentCharIndex++;

         // if, because of the whitespace skip, we're out of text, exit the loop.
         if (currentCharIndex >= stringLen)
            break;

         lineLen = 0;
         lineEndIndex = currentCharIndex;
         longEnough = FALSE;
         endOfText = FALSE;
         while (!longEnough)
         {
            charRec = &(font->characterArray[textString[lineEndIndex]]);

            // if the character has art...
            if (charRec->bitmapUsed)
            {

               if (Rect->Left + lineLen + (charRec->fullWidth) >
                   Rect->Right)
                   longEnough = TRUE;
               else
               {
                  lineLen = lineLen + (charRec->fullWidth);
                  lineEndIndex++;
                  if (lineEndIndex >= stringLen)
                  {
                     longEnough = TRUE;
                     endOfText = TRUE;
                  }
               }
            }
            else
            {
               lineEndIndex++;
               if (lineEndIndex >= stringLen)
               {
                  longEnough = TRUE;
                  endOfText = TRUE;
               }
            }
         }

         // if we're word-wrapping, back up to BEFORE the last hunk of whitespace.
         if (flags & GE_FONT_WORDWRAP && !endOfText)
         {
            tempX = lineEndIndex;
            while (tempX > currentCharIndex && !isspace(textString[tempX]))
               tempX--;

            if (isspace(textString[tempX]))
            {
               while (tempX > currentCharIndex && isspace(textString[tempX]))
                  tempX--;

               lineEndIndex = tempX;
            }
         }

         // aaannnddd,  DRAW!
         for (; currentCharIndex <= lineEndIndex && currentCharIndex < stringLen; currentCharIndex++)
         {
            charRec = &(font->characterArray[textString[currentCharIndex]]);

            if (charRec->bitmapUsed)
            {

               if (charRec->bitmapUsed != lastBitmap)
               {

                  lastBitmap = charRec->bitmapUsed;

                  palette = geBitmap_GetPalette(lastBitmap->map);
                  assert(palette);

                  for (i = 0; i < 256; i++)
                  {

                     fcolorR = Color->r * i / 255;
                     fcolorG = Color->g * i / 255;
                     fcolorB = Color->b * i / 255;
                     fcolorX = Color->a * i / 255;

                     colorArray[i] = gePixelFormat_ComposePixel(	GE_PIXELFORMAT_32BIT_XRGB, (int)fcolorR, (int)fcolorG, (int)fcolorB, (int)fcolorX);
                  }

                  success = geBitmap_Palette_SetData(	palette ,colorArray,GE_PIXELFORMAT_32BIT_XRGB, 256);
           
                  success = geBitmap_SetPalette(lastBitmap->map, palette);
                  assert(success);

               }
            
            
               if (clipRect)
               {
                  artRect = charRec->bitmapRect;

                  if ( CalculateClipping( &artRect, &resultX, &resultY, 
                              Rect->Left + x + charRec->offsetX, 
                              Rect->Top + y + (font->fontSize - charRec->offsetY),
                              *clipRect, GE_CLIP_CORNER)
                     )
                  {
                     success = geEngine_DrawBitmap(font->Engine, charRec->bitmapUsed->map,
        							   &artRect, resultX, resultY);
                  }

               }
               else
               {
                 success = geEngine_DrawBitmap(font->Engine, charRec->bitmapUsed->map,
 	   							   &(charRec->bitmapRect), Rect->Left + x + charRec->offsetX, 
                              Rect->Top + y + (font->fontSize - charRec->offsetY));
               }
               x += charRec->fullWidth;
            }
            else
            {
               // we reached this point because we're trying to draw a character that
               // hasn't been added to the font yet using geFont_AddCharacters().
               assert(0);
            }
         
         }
         y += font->fontSize;
         x = 0;
      }
   }
   else // this font has no attached bitmaps, sooo, we do it another way!
   {

      // at this point, we're trying to put text to the screen using DrawText() and
      // a DIB.  This process REQUIRES an initialized geBitmap as a go-between.
      // Is it initialized?
      assert(font->buffer);

      box.left   = 0;
      box.right  = Rect->Right - Rect->Left;
      box.top    = 0;
      box.bottom = Rect->Bottom - Rect->Top;

      // does the buffer have sufficient dimensions?
      assert (box.right  <= geBitmap_Width (font->buffer)); 
      assert (box.bottom <= geBitmap_Height(font->buffer)); 

      artRect.Left   = box.left;
      artRect.Right  = box.right;
      artRect.Top    = box.top;
      artRect.Bottom = box.bottom;

      success = geFont_DrawTextToBitmap(font, textString, &artRect, Color,
                                          flags, NULL, font->buffer);

      if (clipRect)
      {
         if ( CalculateClipping( &artRect, &resultX, &resultY, 
                     Rect->Left, 
                     Rect->Top,
                     *clipRect, GE_CLIP_CORNER)
            )
         {
            success = geEngine_DrawBitmap(font->Engine, font->buffer,
        					&artRect, resultX, resultY);
         }
      }
      else
      {
         success = geEngine_DrawBitmap(font->Engine, font->buffer,
 	   					&artRect, Rect->Left, Rect->Top);
      }
   }

   return lineEndIndex;
}


//*******************************************************************************
GENESISAPI void GENESISCC geFont_DestroyBitmapBuffer( geFont *font ) 
{
   geBoolean success;
   assert(font);
   if (font->buffer)
   {
      success = geEngine_RemoveBitmap((geEngine *)font->Engine, font->buffer);
      assert(success);

      geBitmap_Destroy(&font->buffer);
   }

}

//*******************************************************************************
GENESISAPI geBoolean GENESISCC geFont_AddBitmapBuffer(
                                  geFont *font, const uint32 width, const uint32 height) 
{
   geBoolean success;
   geBitmap_Palette * palette;
   int i;

   assert(font);

   assert(width  < 1000);  // just checking for absurd values that indicate 
   assert(height < 1000);  // an un-initialized state.

   if (font->buffer)
      geFont_DestroyBitmapBuffer( font );

   font->buffer = geBitmap_Create(width, height, 1, GE_PIXELFORMAT_8BIT); 
   assert(font->buffer);

   // and a pallette for it.
   palette = geBitmap_Palette_Create(GE_PIXELFORMAT_32BIT_XRGB, 256);
   assert(palette);

   for (i = 0; i < 256; i++)
   {
      success = geBitmap_Palette_SetEntryColor(palette, i, i, i, i, i);
      assert(success);

   }

   success = geBitmap_SetPalette(font->buffer, palette);
   assert(success);

	success = geBitmap_SetColorKey(font->buffer, TRUE, 0 ,FALSE );

   success = geEngine_AddBitmap((geEngine *)font->Engine, font->buffer);
   assert(success);

   return GE_TRUE;

}

//*******************************************************************************
GENESISAPI geBoolean GENESISCC geFont_DrawTextToBitmap(geFont *font, const char *textString, 
                                           const GE_Rect *Rect, const GE_RGBA *Color, 
                                           uint32 flags, const GE_Rect *clipRect,
                                           geBitmap *targetBitmap)
{

   int32 x,y,i;
   int32 tempX;//, tempY;
   int32 lineLen;

   uint32 colorArray[256];
   float  fcolorX, fcolorR, fcolorG, fcolorB;

   int32 stringLen;
   int32 currentCharIndex;
   int32 lineEndIndex;
   geFontCharacterRecord *charRec;

   geBoolean success;
   geBoolean longEnough, endOfText;

   GE_Rect artRect;
   RECT box;
   int32 resultX, resultY;

   geFontBitmap *lastBitmap;
   geBitmap_Palette * palette;


   // unimplemented flags which we don't want you to use yet.
   assert(!(flags & GE_FONT_WRAP           ));
   assert(!(flags & GE_FONT_JUST_RETURN_FIT));
   assert(!(flags & GE_FONT_JUSTIFY_RIGHT  ));
   assert(!(flags & GE_FONT_JUSTIFY_CENTER ));

   assert(targetBitmap);

   lastBitmap = NULL;
   x = 0;
   y = 0;
   stringLen = strlen(textString);
   currentCharIndex = 0;

   if (font->bitmapList)  // if this font has character bitmaps...
   {

      while (currentCharIndex < stringLen)
      {
         // skip leading white space for this line.
         while (isspace(textString[currentCharIndex]) && currentCharIndex < stringLen)
            currentCharIndex++;

         // if, because of the whitespace skip, we're out of text, exit the loop.
         if (currentCharIndex >= stringLen)
            break;

         lineLen = 0;
         lineEndIndex = currentCharIndex;
         longEnough = FALSE;
         endOfText = FALSE;
         while (!longEnough)
         {
            charRec = &(font->characterArray[textString[lineEndIndex]]);

            // if the character has art...
            if (charRec->bitmapUsed)
            {

               if (Rect->Left + lineLen + (charRec->fullWidth) >
                   Rect->Right)
                   longEnough = TRUE;
               else
               {
                  lineLen = lineLen + (charRec->fullWidth);
                  lineEndIndex++;
                  if (lineEndIndex >= stringLen)
                  {
                     longEnough = TRUE;
                     endOfText = TRUE;
                  }
               }
            }
            else
            {
               lineEndIndex++;
               if (lineEndIndex >= stringLen)
               {
                  longEnough = TRUE;
                  endOfText = TRUE;
               }
            }
         }

         // if we're word-wrapping, back up to BEFORE the last hunk of whitespace.
         if (flags & GE_FONT_WORDWRAP && !endOfText)
         {
            tempX = lineEndIndex;
            while (tempX > currentCharIndex && !isspace(textString[tempX]))
               tempX--;

            if (isspace(textString[tempX]))
            {
               while (tempX > currentCharIndex && isspace(textString[tempX]))
                  tempX--;

               lineEndIndex = tempX;
            }
         }

         // aaannnddd,  DRAW!
         for (; currentCharIndex <= lineEndIndex && currentCharIndex < stringLen; currentCharIndex++)
         {
            charRec = &(font->characterArray[textString[currentCharIndex]]);

            if (charRec->bitmapUsed)
            {

               if (charRec->bitmapUsed != lastBitmap)
               {

                  lastBitmap = charRec->bitmapUsed;

                  palette = geBitmap_GetPalette(lastBitmap->map);
                  assert(palette);

                  for (i = 0; i < 256; i++)
                  {

                     fcolorR = Color->r * i / 255;
                     fcolorG = Color->g * i / 255;
                     fcolorB = Color->b * i / 255;
                     fcolorX = Color->a * i / 255;

                     colorArray[i] = gePixelFormat_ComposePixel(	GE_PIXELFORMAT_32BIT_XRGB, (int)fcolorR, (int)fcolorG, (int)fcolorB, (int)fcolorX);
                  }

                  success = geBitmap_Palette_SetData(	palette ,colorArray,GE_PIXELFORMAT_32BIT_XRGB, 256);
           
                  success = geBitmap_SetPalette(lastBitmap->map, palette);
                  assert(success);

               }
            
            
               if (clipRect)
               {
                  artRect = charRec->bitmapRect;

                  if ( CalculateClipping( &artRect, &resultX, &resultY, 
                              Rect->Left + x + charRec->offsetX, 
                              Rect->Top + y + (font->fontSize - charRec->offsetY),
                              *clipRect, GE_CLIP_CORNER)
                     )
                  {
                     success = geBitmap_Blit(charRec->bitmapUsed->map, artRect.Left, artRect.Top,
										   targetBitmap, resultX, resultY,
										   artRect.Right - artRect.Left, artRect.Bottom - artRect.Top );
                  }

               }
               else
               {
                  success = geBitmap_Blit(charRec->bitmapUsed->map, charRec->bitmapRect.Left, 
                              charRec->bitmapRect.Top, targetBitmap, 
                              Rect->Left + x + charRec->offsetX, Rect->Top + y + (font->fontSize - charRec->offsetY),
									   charRec->bitmapRect.Right  - charRec->bitmapRect.Left, 
                              charRec->bitmapRect.Bottom - charRec->bitmapRect.Top );
               }
               x += charRec->fullWidth;
            }
            else
            {
               // we reached this point because we're trying to draw a character that
               // hasn't been added to the font yet using geFont_AddCharacters().
               assert(0);
            }
         
         }
         y += font->fontSize;
         x = 0;
      }
   }
   else // this font has no attached bitmaps, sooo, we do it another way!
   {
      box.left   = Rect->Left;
      box.right  = Rect->Right;
      box.top    = Rect->Top;
      box.bottom = Rect->Bottom;
      success = geFont_DrawUsingDIB(font, textString, box, targetBitmap, Color, flags);
   }

   return lineEndIndex;
}


//*******************************************************************************
GENESISAPI geBoolean GENESISCC geFont_TestDraw(geFont *font, int16 x, int16 y, int16 index)
{
   geRect Source;
   geFontBitmap *fontBitmap;
   geBoolean success;

   fontBitmap = font->bitmapList;
   for (; index > 0 && fontBitmap->next; index--)
      fontBitmap = fontBitmap->next;

   assert(fontBitmap);

   Source.Top    = 0;
   Source.Left   = 0;
   Source.Right  = 255;
   Source.Bottom = 255;

   success = geEngine_DrawBitmap(font->Engine, fontBitmap->map,
 								&Source, x, y);

   return success;
}




//*******************************************************************************
GENESISAPI geBoolean GENESISCC geFont_DrawUsingDIB(geFont *font, const char *textString, 
                                                   RECT box, geBitmap *targetBitmap,
                                                   const GE_RGBA *Color, uint32 flags)
{

   geBoolean success;

   HDC      hdc, drawDC;

   float  fcolorR, fcolorG, fcolorB;

   int32          x,y;

   uint8          *lpDIBBuffer, tempChar;              // DIB image buffer
   HBITMAP     hDIB, oldMap;                     // Handle returned from CreateDIBSection
   HPALETTE    hPalette;                 // Handle to palette if 8-bit mode in use
	HFONT oldFont, winFont;
	LOGPALETTE       *pLogPal;            // LOGPALETTE structure
	BITMAPINFO       *pbmi;               // BITMAPINFO structure
	BITMAPINFOHEADER *pbmih;              // Pointer to pbmi header

   int locX, locY;

   geBitmap *lock;
   geBitmap_Info Info;
   geBitmap_Info SecondaryInfo;

   unsigned char *destBits;
   int16 mapWidth;


   int i;

   int desktop_bpp, display_bpp;

   int targetSizeX, targetSizeY;

   //
   // Init DIB stuff
   //

   desktop_bpp = 8;
   display_bpp = 8;

   targetSizeX = geBitmap_Width (targetBitmap) + ( 4 - ( geBitmap_Width (targetBitmap) % 4 ) );
   targetSizeY = geBitmap_Height(targetBitmap);

   hDIB        = NULL;
   lpDIBBuffer = NULL;
   hPalette    = NULL;

   pLogPal = (LOGPALETTE *) 
             GE_RAM_ALLOCATE_ARRAY (char, sizeof(LOGPALETTE) + (sizeof(PALETTEENTRY) * 256));

   assert(pLogPal);

   memset(pLogPal,0, sizeof(LOGPALETTE) + (sizeof(PALETTEENTRY) * 256));

   pLogPal->palVersion    = 0x300;
   pLogPal->palNumEntries = 256;

   pbmi = (BITMAPINFO *) 
          GE_RAM_ALLOCATE_ARRAY(char,sizeof (BITMAPINFOHEADER) + (sizeof (RGBQUAD) * 256));

   if (pbmi == NULL)
   {
      geRam_Free(pLogPal);
      assert(0);
   }
 
   memset(pbmi, 0, sizeof (BITMAPINFOHEADER) + (sizeof (RGBQUAD) * 256));

   pbmih = &(pbmi->bmiHeader);

   pbmih->biSize          =  sizeof(BITMAPINFOHEADER);
   pbmih->biWidth         =  (targetSizeX);
   pbmih->biHeight        = -(targetSizeY);
   pbmih->biPlanes        =  1;
   pbmih->biBitCount      =  (uint16) 8;
   pbmih->biSizeImage     =  0;
   pbmih->biXPelsPerMeter =  0;
   pbmih->biYPelsPerMeter =  0;
   pbmih->biClrUsed       =  0;
   pbmih->biClrImportant  =  0;
  
   pbmih->biCompression = BI_RGB;


   // set palette entries
   for (i = 0; i < 256; i++)
   {
      fcolorR = Color->r * i / 255;
      fcolorG = Color->g * i / 255;
      fcolorB = Color->b * i / 255;

      pbmi->bmiColors[i].rgbRed   = (unsigned char) fcolorR;
      pbmi->bmiColors[i].rgbGreen = (unsigned char) fcolorG;
      pbmi->bmiColors[i].rgbBlue  = (unsigned char) fcolorB;
   
   }

   hPalette = CreatePalette(pLogPal);

   //
   // Allocate the DIB section ("back buffer")
   //

   hdc = GetDC(GetDesktopWindow());

   hDIB = CreateDIBSection(hdc,             // Device context
                           pbmi,            // BITMAPINFO structure
                           DIB_RGB_COLORS,  // Color data type
                (void **) &lpDIBBuffer,     // Address of image map pointer
                           NULL,            // File
                           0);              // Bitmap file offset

   ReleaseDC(GetDesktopWindow(), hdc);
   
   assert(hDIB);

   // now we DrawText to the DIB

   drawDC = CreateCompatibleDC(NULL);

	winFont = CreateFont( -1 * font->fontSize,
    					    0,0,0, font->fontWeight,
    					    0,0,0,0,OUT_TT_ONLY_PRECIS ,0,0,0, font->fontNameString);
   assert(winFont);

   oldFont = SelectObject( drawDC, winFont); 
   oldMap  = SelectObject( drawDC, hDIB); 

	SetTextColor(drawDC, RGB(Color->r,Color->g,Color->b));
   SetBkColor  (drawDC, RGB(0,0,0));

 	SetBkMode( drawDC, OPAQUE );

	// we are accounting for the fact that Windows GDI stuff doesn't seem to draw on the end of a rect
	box.bottom++;
	box.right++;

   if (flags & GE_FONT_WORDWRAP)
      DrawText(drawDC, textString, strlen(textString), &box, DT_WORDBREAK);
   else
      DrawText(drawDC, textString, strlen(textString), &box, 0);

	// undoing the for-windows hack
	box.bottom--;
	box.right--;

	SelectObject(drawDC,oldMap);
	SelectObject(drawDC,oldFont);

   DeleteDC(drawDC);

   DeleteObject(winFont);


   // now we copy the bits in the DIB to the targetBitmap
   success = geBitmap_LockForWrite(	targetBitmap, &lock,
                                    0,0);

   success = geBitmap_GetInfo(lock, &Info, &SecondaryInfo);

   if (!gePixelFormat_IsRaw(Info.Format))
   {
      success = geBitmap_UnLock(lock);

      if (gePixelFormat_IsRaw(SecondaryInfo.Format))
      {
         success = geBitmap_LockForWriteFormat(	targetBitmap, &lock,0,0, SecondaryInfo.Format);
         assert(success);
         Info = SecondaryInfo;
      }
      else
      {
         success = geBitmap_SetFormat(targetBitmap, GE_PIXELFORMAT_24BIT_RGB, GE_TRUE, 0, NULL);
         assert(success);
         success = geBitmap_SetColorKey(targetBitmap, GE_TRUE, 0, GE_FALSE);
         assert(success);
         success = geBitmap_LockForWrite(	targetBitmap, &lock,0,0);
         assert(success);
         success = geBitmap_GetInfo(lock, &Info, &SecondaryInfo);
         assert(success);
      }

   }

   mapWidth  = Info.Stride;

   destBits = geBitmap_GetBits(lock);

   if (GE_PIXELFORMAT_8BIT == Info.Format)
   {
      locX = 0;
      locY = 0;

      for ( y = box.top; 
            y <= box.bottom; 
            y++)
      {
         for (x = box.left;
              x <= box.right; 
              x++)
         {
            tempChar = lpDIBBuffer[y * targetSizeX + x];
            destBits[(y+locY)*mapWidth + (x+locX)] = tempChar;
         }
      }
   }
  	else
	{
   	uint8 * bptr;

		// oh well, do our best
		// bitmaps must have had a good reason to not give us the format we preferred,

		for(y=box.top; y <= box.bottom; y++)
		{
			for(x=box.left; x <= box.right; x++)
			{
   			uint32 R,G,B;

				// put a color in any format
            tempChar = lpDIBBuffer[y * targetSizeX + x];

            R = (uint32)(Color->r * tempChar / 255);
            G = (uint32)(Color->g * tempChar / 255);
            B = (uint32)(Color->b * tempChar / 255);

				// we use alpha of 255 for opaque

   			bptr = destBits + (Info.Stride * y * gePixelFormat_BytesPerPel(Info.Format)) 
                              + gePixelFormat_BytesPerPel(Info.Format) * x;
				gePixelFormat_PutColor(Info.Format,&bptr,R,G,B,255);
			}

		}
	}

   success = geBitmap_UnLock(lock);


   // finally, clean up!
   DeleteObject(hDIB);
   DeleteObject(hPalette);


   geRam_Free(pbmi);
   geRam_Free(pLogPal);

   return TRUE;
}


//*******************************************************************************
GENESISAPI int32 GENESISCC geFont_GetStringPixelWidth (geFont *font, const char *textString)
{

   if (font->bitmapList)
   {
      int32 i, width;

      width = 0;
      for (i = 0; i < (int32)strlen(textString); i++)
      {
         assert (font->characterArray[textString[i]].bitmapUsed);

         width += font->characterArray[textString[i]].fullWidth;
      }

      return width;
   }
   else
   {
      SIZE sizeInfo;
      HDC hdc;
	   HFONT oldFont, winFont;

      hdc = GetDC(GetDesktopWindow());
   	winFont = CreateFont( -1 * font->fontSize,
    					    0,0,0, font->fontWeight,
    					    0,0,0,0,OUT_TT_ONLY_PRECIS ,0,0,0, font->fontNameString);
      assert(winFont);

      oldFont = SelectObject( hdc, winFont); 

      GetTextExtentPoint32(  hdc, textString, strlen(textString), &sizeInfo);

   	SelectObject(hdc,oldFont);
      DeleteObject(winFont);

      ReleaseDC(GetDesktopWindow(), hdc);

      return sizeInfo.cx;
   }
}

//*******************************************************************************
GENESISAPI int32 GENESISCC geFont_GetStringPixelHeight(geFont *font, const char *textString)
{

      SIZE sizeInfo;
      HDC hdc;
	   HFONT oldFont, winFont;

      hdc = GetDC(GetDesktopWindow());
   	winFont = CreateFont( -1 * font->fontSize,
    					    0,0,0, font->fontWeight,
    					    0,0,0,0,OUT_TT_ONLY_PRECIS ,0,0,0, font->fontNameString);
      assert(winFont);

      oldFont = SelectObject( hdc, winFont); 

      GetTextExtentPoint32(  hdc, textString, strlen(textString), &sizeInfo);

   	SelectObject(hdc,oldFont);
      DeleteObject(winFont);

      ReleaseDC(GetDesktopWindow(), hdc);

      return sizeInfo.cy;
}

//*******************************************************************************
GENESISAPI geBitmap* GENESISCC geFont_GetBuffer(geFont *font)
{

   return font->buffer;
}


//*******************************************************************************
GENESISAPI geBoolean GENESISCC geFont_GetCharMap(geFont *font, uint8 character, GE_Rect *Rect, 
												 geBitmap **targetBitmap, int32 *fullWidth, int32 *fullHeight, 
												 int32 *offsetX, int32 *offsetY)
{


	assert(font);
	assert(font->characterArray[character].bitmapUsed);

	*targetBitmap = font->characterArray[character].bitmapUsed->map;

	*Rect = font->characterArray[character].bitmapRect;

	*fullWidth = font->characterArray[character].fullWidth;
	*fullHeight = font->fontSize;
	*offsetX = font->characterArray[character].offsetX;
	*offsetY = font->characterArray[character].offsetY;

	return GE_TRUE;
}


//*******************************************************************************
GENESISAPI void GENESISCC geFont_EnableAntialiasing(geFont *font, const geBoolean anti)
{
   font->antialiased = anti;
}

//*******************************************************************************
GENESISAPI geBoolean GENESISCC geFont_IsAntialiased(geFont *font)
{
   return font->antialiased;
}

