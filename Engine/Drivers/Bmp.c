/****************************************************************************************/
/*  Bmp.c                                                                               */
/*                                                                                      */
/*  Author: Eli Boling                                                                  */
/*  Description: Converts data to bmp format.                                           */
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
#include	<windows.h>

#include	<stdio.h>

#define	WIDTH			640
#define HEIGHT			480

BITMAPFILEHEADER	bfh = 
{
	((unsigned short)'B' | ((unsigned short)'M' << 8)),
	sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFO) + WIDTH * HEIGHT * 2,
	0,
	0,
	sizeof(BITMAPINFOHEADER)
};

BITMAPINFO		bi =
{
	{
	sizeof(BITMAPINFOHEADER),
	WIDTH,
	HEIGHT,
	1,
	24,
	BI_RGB,
	0,
	0,
	0,
	0,
	0
	}
};

#ifdef BITS16
	#define	RED(x)		((unsigned short)((x>>11) & 0x1f))
	#define	GREEN(x)	((unsigned short)((x>>6 ) & 0x1f))
	#define	BLUE(x)		((unsigned short)((x>>0 ) & 0x1f))
#else
	#define	RED(x)		((unsigned short)((x>>11) & 0x1f))
	#define	GREEN(x)	((unsigned short)((x>>5 ) & 63))
	#define	BLUE(x)		((unsigned short)((x>>0 ) & 0x1f))
#endif

int WriteBMP(unsigned short *ScreenBuffer, const char *Name)
{
	FILE *	out;
	int	y;

	out = fopen(Name, "wb");

	if (!out)
		return 0;

	if	(fwrite(&bfh, sizeof(bfh), 1, out) != 1)
		return 0;

	if	(fwrite(&bi, sizeof(bi), 1, out) != 1)
		return 0;

	for	(y = HEIGHT-1; y >= 0; y--)
	{
		int			i;
		unsigned short *p;
		unsigned char	Buff[WIDTH * 3];
		unsigned char *	BuffPtr;

		BuffPtr = &Buff[0];
		p = &ScreenBuffer[y*WIDTH];
		for	(i = 0; i < WIDTH; i++)
		{
			#ifdef BIT16
				unsigned short	c;
			
				c = p[i];
				c = (RED(c) << 10) + (GREEN(c) << 5) + BLUE(c);
				p[i] = c;
			#else
				
				#ifdef VER1
				char c[3];
				c = (char)(GREEN(p[i]) << 2);
				fwrite(&c, 1, 1, out);
				c = (char)(RED(p[i]) << 3);
				fwrite(&c, 1, 1, out);
				c = (char)(BLUE(p[i]) << 3);
				fwrite(&c, 1, 1, out);
				#else
				#if 0
				char c[3];
				c = (char)(BLUE(p[i]) << 3);
				fwrite(&c, 1, 1, out);
				c = (char)(GREEN(p[i]) << 2);
				fwrite(&c, 1, 1, out);
				c = (char)(RED(p[i]) << 3);
				fwrite(&c, 1, 1, out);
				#else
				*BuffPtr++ = (char)(BLUE(p[i]) << 3);
				*BuffPtr++ = (char)(GREEN(p[i]) << 2);
				*BuffPtr++ = (char)(RED(p[i]) << 3);
//				fwrite(&c[0], 3, 1, out);
 				#endif
				#endif
			#endif
		}

		fwrite(&Buff[0], WIDTH * 3, 1, out);
		#ifdef BIT16
			p = &ScreenBuffer[y*WIDTH];
			for	(i = 0; i < WIDTH; i++)
				fwrite(&p[(i+2)%WIDTH], 2, 1, out);
		#endif
	}

	if	(fclose(out))
		return 0;

	return 1;
}

