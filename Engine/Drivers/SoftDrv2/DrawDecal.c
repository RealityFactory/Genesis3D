/****************************************************************************************/
/*  DrawDecal.H                                                                         */
/*                                                                                      */
/*  Author: Ken Baird, Mike Sandige                                                     */
/*  Description:  This is a simple 2d blitter for the Genesis3d software driver.        */
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

#include "DrawDecal.h"
#include "SoftDrv.h"

static int32		BWidth, BHeight, BStride;
static int32		DrawWidth, DrawHeight;
static int32		EbpAdd, EdiAdd;
static uint16		*BBitPtr16;
static uint8		*BBitPtr;
static uint16		*pScrPtr16bpp;
static uint32		*pScrPtr32bpp;

geBoolean DRIVERCC DrawDecal(geRDriver_THandle *THandle, RECT *SRect, int32 x, int32 y)
{

	if(!SD_Active)
	{
		return	GE_TRUE;
	}
	
	BWidth		=THandle->Width;
	BHeight		=THandle->Height;
	DrawWidth	=THandle->Width;
	DrawHeight	=THandle->Height;
	
	#if 0
	if(SD_ProcessorHas3DNow )
	{
		U32	*PalPtr;
		BBitPtr		=(U8 *)THandle->BitPtr[0];

		if ( ! THandle->PalHandle )
			return 0;

		PalPtr	=(U32 *)THandle->PalHandle->BitPtr[0];

		if(SRect)
		{
			BBitPtr		+=SRect->top * DrawWidth + SRect->left;
			DrawHeight	=(SRect->bottom - SRect->top);
			DrawWidth	=(SRect->right - SRect->left);
		}

		if(x < 0)
		{
			if(x + DrawWidth <= 0)
			{
				return	GE_TRUE;
			}
			BBitPtr		-=x;
			DrawWidth	+=x;
			x			=0;
		}

		if(y < 0)
		{
			if(y + DrawHeight <= 0)
			{
				return	GE_TRUE;
			}
			BBitPtr		-=y * BWidth;
			DrawHeight	+=y;
			y			=0;
		}
		
		if(x >= ClientWindow.Width)
		{
			return	GE_TRUE;
		}
		if(y >= ClientWindow.Height)
		{
			return	GE_TRUE;
		}

		if(x + DrawWidth >= (ClientWindow.Width-1))
			DrawWidth -= (x+DrawWidth) - (ClientWindow.Width-1);

		if(y + DrawHeight >= (ClientWindow.Height-1))
			DrawHeight -=  (y+DrawHeight)- (ClientWindow.Height-1);

		if(DrawWidth <= 0)
			return GE_TRUE;
		
		if(DrawHeight <= 0)
			return GE_TRUE;

		pScrPtr32bpp	=(U32 *)(ClientWindow.Buffer);
		pScrPtr32bpp	=&pScrPtr32bpp[y * ClientWindow.Width + x];

		__asm
		{
			push ecx
			push esi
			push edi
			push ebp

			mov	ebx,PalPtr

			mov ebp, pScrPtr32bpp
			mov edi, BBitPtr

			mov ecx, DrawWidth

			mov edx, ClientWindow.PixelPitch
			shl ecx,2
			sub edx, ecx
			mov EbpAdd, edx

			mov edx, BWidth
			mov ecx, DrawHeight
			sub edx, DrawWidth
			mov EdiAdd, edx


		NextHeight:
		
			push ecx
			mov ecx, DrawWidth

			lea	ebp,[4*ecx+ebp]

			add edi, ecx
//			shl ecx, 2
//			add ebp, ecx

			neg ecx

			NextWidth:
				xor	eax,eax
				mov al, [edi+ecx]
				
				cmp al,0ffh
				je Skip

//				cmp al,01h
//				je Skip

				mov	eax,[ebx+eax*4]

				mov [ebp+ecx*4], eax

			Skip:
				inc ecx
				jnz NextWidth

			add ebp, EbpAdd
			add edi, EdiAdd

			pop ecx
			
			sub ecx, 1
			jnz NextHeight

			pop ebp
			pop edi
			pop esi
			pop ecx
		}
	}
	else
	#endif
	{
		BBitPtr16	=THandle->BitPtr[0];
		if(SRect)
		{
			BBitPtr16	+=SRect->top * DrawWidth + SRect->left;
			DrawHeight	=(SRect->bottom - SRect->top);
			DrawWidth	=(SRect->right - SRect->left);
		}

		if(x < 0)
		{
			if(x + DrawWidth <= 0)
			{
				return	GE_TRUE;
			}
			BBitPtr16	-=x;
			DrawWidth	+=x;
			x			=0;
		}

		if(y < 0)
		{
			if(y + DrawHeight <= 0)
			{
				return	GE_TRUE;
			}
			BBitPtr16	-=y*BWidth;
			DrawHeight	+=y;
			y			=0;
		}
		
		if(x >= ClientWindow.Width)
		{
			return	GE_TRUE;
		}
		if(y >= ClientWindow.Height)
		{
			return	GE_TRUE;
		}

		if(x + DrawWidth >= (ClientWindow.Width - 1))
		{
			DrawWidth	-=(x + DrawWidth) - (ClientWindow.Width - 1);
		}
		if(y + DrawHeight >= (ClientWindow.Height - 1))
		{
			DrawHeight	-=(y + DrawHeight) - (ClientWindow.Height - 1);
		}

		if(DrawWidth <= 0)
		{
			return	GE_TRUE;
		}
		if(DrawHeight <= 0)
		{
			return	GE_TRUE;
		}

		pScrPtr16bpp	=(U16 *)(ClientWindow.Buffer);
		pScrPtr16bpp	=&pScrPtr16bpp[y * (ClientWindow.PixelPitch >> 1) + x];

		__asm
		{
			push ecx
			push esi
			push edi
			push ebp

			mov ebp, pScrPtr16bpp
			mov edi, BBitPtr16

			mov ecx, DrawWidth

			mov edx, ClientWindow.PixelPitch
			sub edx, ecx
			sub edx, ecx
			mov EbpAdd, edx

			mov edx, BWidth
			add edx, BWidth
			sub edx, ecx
			sub edx, ecx
			mov EdiAdd, edx

			mov ecx, DrawHeight

		NextHeightA:
		
			push ecx
			mov ecx, DrawWidth

			shl ecx, 1
			add ebp, ecx
			add edi, ecx

			neg ecx

			NextWidthA:
				mov ax, [edi+ecx]
				
				cmp ax, 0x1
				je SkipA

				mov [ebp+ecx], ax

			SkipA:
				add ecx, 2
				jnz NextWidthA

			add ebp, EbpAdd
			add edi, EdiAdd

			pop ecx
			
			sub ecx, 1
			jnz NextHeightA

			pop ebp
			pop edi
			pop esi
			pop ecx
		}
	}
	return GE_TRUE;
}



