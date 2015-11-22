/****************************************************************************************/
/*  Render.c                                                                            */
/*                                                                                      */
/*  Author: George McBay (gfm@my-deja.com)                                              */
/*  Description: Polygon rasterization functions for OpenGL driver                      */
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
/*                                                                                      */
/****************************************************************************************/

#include <windows.h>
#include <gl/gl.h>

#include "Render.h"
#include "OglDrv.h"
#include "OglMisc.h"
#include "THandle.h"
#include "Win32.h"


DRV_RENDER_MODE		RenderMode = RENDER_NONE;
uint32				Render_HardwareFlags = 0;

GLint				boundTexture = -1;  // Currently bound OpenGL texture object
GLint				boundTexture2 = -1; // Currently bound OpenGL tex object on second TMU

GLint				decalTexObj = -1;

// Render a world polygon without multitexture support.  This will do two-polygon draws,
// one with the regular texture, then another with the lightmap texture.  Clearly we hope
// we don't have to use this function (if multitexture is supported in hardware and OpenGL
// ICD, we won't have to), but its here just in case.
void Render_WorldPolyRegular(DRV_TLVertex *Pnts, int32 NumPoints, geRDriver_THandle *THandle,
							 DRV_LInfo *LInfo, GLfloat shiftU, GLfloat shiftV, 
							 GLfloat scaleU, GLfloat scaleV,
							 GLubyte alpha)
{
	DRV_TLVertex *pPnt;
	GLfloat zRecip;
	GLfloat tu, tv;
	GLint	i;

#ifdef USE_LIGHTMAPS
	if(LInfo != NULL)
	{
		glDepthMask(GL_FALSE);
	}
#endif

	pPnt = Pnts;

	glBegin(GL_TRIANGLE_FAN);	

	for(i = 0; i < NumPoints; i++)
	{	
		zRecip = 1.0f / pPnt->z;   

		tu = (pPnt->u * scaleU + shiftU);
		tv = (pPnt->v * scaleV + shiftV);

		glColor4ub((GLubyte)pPnt->r, (GLubyte)pPnt->g, (GLubyte)pPnt->b, alpha);

		glTexCoord4f(tu * THandle->InvScale * zRecip, 
			tv * THandle->InvScale * zRecip, 0.0f, zRecip);
	
		glVertex3f(pPnt->x, pPnt->y, -1.0f + zRecip);
		
		pPnt++;
	}

	glEnd(); 

#ifdef USE_LIGHTMAPS
	if(LInfo != NULL)
	{
		glDepthMask(GL_TRUE);
		
		glBlendFunc(GL_DST_COLOR,GL_ZERO);

		pPnt = Pnts;

		if(boundTexture != LInfo->THandle->TextureID)
		{
			glBindTexture(GL_TEXTURE_2D, LInfo->THandle->TextureID);
			boundTexture = LInfo->THandle->TextureID;
		}

		if(LInfo->THandle->Flags & THANDLE_UPDATE)
		{
			THandle_Update(LInfo->THandle);
		}

		shiftU = (GLfloat)LInfo->MinU - 8.0f;
		shiftV = (GLfloat)LInfo->MinV - 8.0f;

		glColor4ub(255, 255, 255, 255);

	    glBegin(GL_TRIANGLE_FAN);	
		
		for(i = 0; i < NumPoints; i++)
		{	
			zRecip = 1.0f / pPnt->z;   

			tu = pPnt->u - shiftU;
			tv = pPnt->v - shiftV;

			glTexCoord4f(tu * LInfo->THandle->InvScale * zRecip, 
				tv * LInfo->THandle->InvScale * zRecip, 0.0f, zRecip);

			glVertex3f(pPnt->x, pPnt->y, -1.0f + zRecip);
			
			pPnt++;
		}
		
		glEnd(); 

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
#endif
}


// Render a world polygon using multiple TMUs to map the regular texture and lightmap in one pass 
void Render_WorldPolyMultitexture(DRV_TLVertex *Pnts, int32 NumPoints, geRDriver_THandle *THandle,
								  DRV_LInfo *LInfo, GLfloat shiftU, GLfloat shiftV, 
								  GLfloat scaleU, GLfloat scaleV,
								  GLubyte alpha)
{
	DRV_TLVertex *pPnt;
	GLfloat zRecip;
	GLfloat tu, tv, lu, lv;
	GLfloat shiftU2, shiftV2;
	GLint	i;
	

	if(LInfo != NULL)
	{
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glEnable(GL_TEXTURE_2D);

		if(boundTexture2 != LInfo->THandle->TextureID)
		{
			glBindTexture(GL_TEXTURE_2D, LInfo->THandle->TextureID);
			boundTexture2 = LInfo->THandle->TextureID;
		}

		if(LInfo->THandle->Flags & THANDLE_UPDATE)
		{
			THandle_Update(LInfo->THandle);
		}

		shiftU2 = (GLfloat)LInfo->MinU - 8.0f;
		shiftV2 = (GLfloat)LInfo->MinV - 8.0f;
	}

	pPnt = Pnts;

	glBegin(GL_TRIANGLE_FAN);	

	for(i = 0; i < NumPoints; i++)
	{	
		zRecip = 1.0f / pPnt->z;   

		tu = (pPnt->u * scaleU + shiftU);
		tv = (pPnt->v * scaleV + shiftV);

		glColor4ub((GLubyte)pPnt->r, (GLubyte)pPnt->g, (GLubyte)pPnt->b, alpha);

		glMultiTexCoord4fARB(GL_TEXTURE0_ARB, tu * THandle->InvScale * zRecip, 
			tv * THandle->InvScale * zRecip, 0.0f, zRecip);

		if(LInfo)
		{
			lu = pPnt->u - shiftU2;
			lv = pPnt->v - shiftV2;

			glMultiTexCoord4fARB(GL_TEXTURE1_ARB, lu * LInfo->THandle->InvScale * zRecip, 
				lv * LInfo->THandle->InvScale * zRecip, 0.0f, zRecip);
		}

		glVertex3f(pPnt->x, pPnt->y, -1.0f + zRecip);
		
		pPnt++;
	}

	glEnd(); 

	if(LInfo != NULL)
	{
		glDisable(GL_TEXTURE_2D);
		glActiveTextureARB(GL_TEXTURE0_ARB);
	}
} 

// Entry point to render a world polygon, does some setup and then passed off control
// to a specific WorldPoly function (see above) depending upon whether multitexture is on or
// off
geBoolean DRIVERCC Render_WorldPoly(DRV_TLVertex *Pnts, int32 NumPoints, geRDriver_THandle *THandle, 
									DRV_TexInfo *TexInfo, DRV_LInfo *LInfo, uint32 Flags)
{
	GLfloat	shiftU, shiftV, scaleU, scaleV;
	DRV_TLVertex *pPnt = Pnts;
	GLubyte alpha;
	BOOL	Dynamic = 0;

	if(!RenderingIsOK)
		return GE_TRUE;

	if(Flags & DRV_RENDER_ALPHA)
	{
		alpha = (GLubyte)Pnts->a;
	}
	else
	{
		alpha = 255;
	}


	shiftU = TexInfo->ShiftU;
	shiftV = TexInfo->ShiftV;
	scaleU = 1.0f / TexInfo->DrawScaleU;
	scaleV = 1.0f / TexInfo->DrawScaleV;

	if(boundTexture != THandle->TextureID)
	{
		glBindTexture(GL_TEXTURE_2D, THandle->TextureID);
		boundTexture = THandle->TextureID;
	}

	if(THandle->Flags & THANDLE_UPDATE)
	{
		THandle_Update(THandle);
	}

	if(LInfo != NULL)
	{
		OGLDRV.SetupLightmap(LInfo, &Dynamic);

		if(Dynamic || LInfo->THandle->Flags & THANDLE_UPDATE_LM)
		{
			THandle_DownloadLightmap(LInfo);

			if(Dynamic)
			{
				LInfo->THandle->Flags |= THANDLE_UPDATE_LM;
			}
			else
			{
				LInfo->THandle->Flags &= ~THANDLE_UPDATE_LM;
			} 
		}
	}

	if(multitexture) 
	{
		// Hooray!!
		Render_WorldPolyMultitexture(Pnts, NumPoints, THandle, LInfo, shiftU, shiftV,
			scaleU, scaleV, alpha);
	}
	else
	{
		// Hiss! Boo!
		Render_WorldPolyRegular(Pnts, NumPoints, THandle, LInfo, shiftU, shiftV,
			scaleU, scaleV, alpha);
	}

	OGLDRV.NumRenderedPolys++; 

	return GE_TRUE;
}


// Render a generic plain ol' polygon using Gouraud smooth shading and no texture map.
geBoolean DRIVERCC Render_GouraudPoly(DRV_TLVertex *Pnts, int32 NumPoints, uint32 Flags)
{
	GLint i;
	GLfloat zRecip;
	DRV_TLVertex *pPnt = Pnts;
	GLubyte alpha;

	if(!RenderingIsOK)
		return GE_TRUE;

	if(Flags & DRV_RENDER_ALPHA)
	{
		alpha = (GLubyte)Pnts->a;
	}
	else
	{
		alpha = 255;
	}

	glDisable(GL_TEXTURE_2D);

	if(Flags & DRV_RENDER_NO_ZMASK)
	{
		glDisable(GL_DEPTH_TEST);
	}

 	glBegin(GL_TRIANGLE_FAN);

	for(i = 0; i < NumPoints; i++)
	{
		zRecip = 1.0f / pPnt->z;   
		
		glColor4ub((GLubyte)pPnt->r, (GLubyte)pPnt->g, (GLubyte)pPnt->b, alpha);

		glVertex3f(pPnt->x, pPnt->y, -1.0f + zRecip);

		pPnt++;
	}

	glEnd(); 

	glEnable(GL_TEXTURE_2D); 

	if(Flags & DRV_RENDER_NO_ZMASK)
	{
		glEnable(GL_DEPTH_TEST);
	}

	OGLDRV.NumRenderedPolys++; 

	return GE_TRUE;
}


// Render a non-world polygon (such as on an actor).  
geBoolean DRIVERCC Render_MiscTexturePoly(DRV_TLVertex *Pnts, int32 NumPoints, 
										  geRDriver_THandle *THandle, uint32 Flags)
{
	GLint i;
	GLfloat zRecip;
	DRV_TLVertex *pPnt = Pnts;
	GLubyte alpha;

	if(!RenderingIsOK)
		return GE_TRUE;

	if(Flags & DRV_RENDER_ALPHA)
	{
		alpha = (GLubyte)Pnts->a;
	}
	else
	{
		alpha = 255;
	}

	if(boundTexture != THandle->TextureID)
	{
		glBindTexture(GL_TEXTURE_2D, THandle->TextureID);
		boundTexture = THandle->TextureID;
	}

	if(THandle->Flags & THANDLE_UPDATE)
	{
		THandle_Update(THandle);
	}

	if(Flags & DRV_RENDER_NO_ZMASK)
	{
		glDisable(GL_DEPTH_TEST);
	}

	glBegin(GL_TRIANGLE_FAN);

	for(i = 0; i < NumPoints; i++)
	{
		zRecip = 1.0f / pPnt->z;                    

		glColor4ub((GLubyte)pPnt->r, (GLubyte)pPnt->g, (GLubyte)pPnt->b, alpha);

		glTexCoord4f(pPnt->u * zRecip, pPnt->v * zRecip, 0.0f, zRecip);

		glVertex3f(pPnt->x, pPnt->y, -1.0f + zRecip);

		pPnt++;
	}

	glEnd();  

	if(Flags & DRV_RENDER_NO_ZMASK)
	{
		glEnable(GL_DEPTH_TEST);
	}

	return GE_TRUE;
}

// changed QD Shadows
geBoolean DRIVERCC Render_StencilPoly(DRV_XYZVertex *Pnts, int32 NumPoints, uint32 Flags) 
{
	return GE_TRUE;
}

geBoolean DRIVERCC DrawShadowPoly(geFloat r, geFloat g, geFloat b, geFloat a)
{
	return GE_TRUE;
}
// end change


// Render a 2D decal graphic...
// Notes: unfortunately traditional thru-the-API framebuffer access for OpenGL is 
//        ridculously slow with most ICDs.
//        So I've implemented decals as clamped textures on polygons.
//        This works rather well... usually... and it is fast.  However, it has drawbacks....
//        1) Extra use of texture ram to hold decals
//        2) May look shoddy under some situations on cards with 256x256 texture maximums 
//           (3DFX Voodoo springs to mind) 
//           Reason: If they only support 256x256, large decals like the GTest bitmap font may 
//           be sampled down to 256x256 and then restreched when drawing.  I haven't tested this 
//           against such cards, due to lack of any access to one, but my guess is the resulting 
//           image wont look so hot.
//
// In the future, may want to add platform-specific framebuffer access (GDI, X11, etc) as 
// allowed.  But...for Windows, DirectDraw isn't supported, and to the best of my knowledge, 
// GDI is only reliably supported when not using a double-buffer.  Joy.
geBoolean DRIVERCC DrawDecal(geRDriver_THandle *THandle, RECT *SRect, int32 x, int32 y)
{
	RECT tmpRect, *srcRect;
	GLint width, height;
	GLfloat uClamp, vClamp;
	GLfloat uDiff = 1.0f, vDiff = 1.0f;
	
	if(!RenderingIsOK)
		return GE_TRUE;

	if(!SRect)
	{
		tmpRect.left = 0;
		tmpRect.right = THandle->Width;
		tmpRect.top = 0;
		tmpRect.bottom = THandle->Height;

		srcRect = &tmpRect;
		width = (THandle->Width);
		height = (THandle->Height);
	}
	else
	{
		srcRect = SRect;
		width = (srcRect->right - srcRect->left);
		height = (srcRect->bottom - srcRect->top);
	}
	
	if(x + width <= 0 || y + height <= 0 || x >= ClientWindow.Width || y >= ClientWindow.Height)
	{
		return GE_TRUE;
	}
	
	if(x + width >= (ClientWindow.Width - 1))
	{
		srcRect->right -= ((x + width) - (ClientWindow.Width - 1));
	}

	if(y + height >= (ClientWindow.Height - 1))
	{
		srcRect->bottom -= ((y + height) - (ClientWindow.Height - 1));
	}

	if(x < 0)
	{
		srcRect->left += -x;
		x = 0;
	}
	
	if(y < 0)
	{
		srcRect->top += -y;
		y = 0;
	}

	if(boundTexture != THandle->TextureID)
	{
		glBindTexture(GL_TEXTURE_2D, THandle->TextureID);
		boundTexture = THandle->TextureID;
	}

	if(THandle->Flags & THANDLE_UPDATE)
	{
		THandle_Update(THandle);
	}
	
	glDisable(GL_DEPTH_TEST);
	
	glColor4f(1.0, 1.0, 1.0, 1.0);
	
	glShadeModel(GL_FLAT);
	
	if(THandle->Data[1] == NULL)
	{
		uClamp = width / (GLfloat)THandle->PaddedWidth;
		vClamp = height / (GLfloat)THandle->PaddedHeight;
		
		glMatrixMode(GL_TEXTURE); 
		glPushMatrix();
		glLoadIdentity();
		glTranslatef(srcRect->left / (GLfloat)THandle->PaddedWidth, 
			srcRect->top / (GLfloat)THandle->PaddedHeight, 0.0f);
		
		glBegin(GL_QUADS);
		
		glTexCoord2f(0.0f, 0.0);
		glVertex2i(x, y);
		
		glTexCoord2f(uClamp, 0.0f);
		glVertex2i(x + width, y);
		
		glTexCoord2f(uClamp, vClamp);
		glVertex2i(x + width, y + height);
		
		glTexCoord2f(0.0f, vClamp);
		glVertex2i(x, y + height);
		
		glEnd();  
		
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
	}
	else
	{
	
		glPixelStorei(GL_UNPACK_ROW_LENGTH, THandle->Width);
		glPixelStorei(GL_UNPACK_SKIP_PIXELS, srcRect->left);
		glPixelStorei(GL_UNPACK_SKIP_ROWS, srcRect->top);

		glPixelZoom(1.0, -1.0);

		if(width <= maxTextureSize && height <= maxTextureSize &&
			width == SnapToPower2(width) && height == SnapToPower2(height))
		{
			// Last chance to avoid the dreaded glDrawPixels...Yes, this is faster
			// than glDrawPixels on the ICD's I've tested.  Go figure.
			// (Could add a more complex texture upload/clamp system for
			//  width/heights other than powers of 2, but for now,
			//  this is enough complexity...Sorry, 3DFX)

			if(decalTexObj == -1)
			{
				glGenTextures(1, &decalTexObj);
			}

			glBindTexture(GL_TEXTURE_2D, decalTexObj);
			boundTexture = decalTexObj;

			glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 
					0, GL_RGBA, GL_UNSIGNED_BYTE, THandle->Data[1]); 

			glBegin(GL_QUADS);
			
			glTexCoord2f(0.0f, 0.0);
			glVertex2i(x, y);
			
			glTexCoord2f(1.0f, 0.0f);
			glVertex2i(x + width, y);
			
			glTexCoord2f(1.0f, 1.0f);
			glVertex2i(x + width, y + height);
			
			glTexCoord2f(0.0f, 1.0f);
			glVertex2i(x, y + height);
			
			glEnd();
		}
		else
		{
			glPixelZoom(1.0, -1.0);

			glRasterPos2i(x, y);
			glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, THandle->Data[1]);
		}

		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
		glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
	} 
	
	glShadeModel(GL_SMOOTH);
	
	glEnable(GL_DEPTH_TEST); 
	
	return GE_TRUE; 
}

// changed QD Shadows
//geBoolean DRIVERCC BeginScene(geBoolean Clear, geBoolean ClearZ, RECT *WorldRect)
geBoolean DRIVERCC BeginScene(geBoolean Clear, geBoolean ClearZ, geBoolean ClearStencil, RECT *WorldRect)
{

	if(Clear)
	{
		glClear(GL_COLOR_BUFFER_BIT);
	}
	
	if(ClearZ)
	{
		glClear(GL_DEPTH_BUFFER_BIT);
	}

	OGLDRV.NumRenderedPolys = 0;

	return GE_TRUE;
}


geBoolean DRIVERCC EndScene(void)
{	
	if(RenderingIsOK)
		FlipGLBuffers();
	
	return GE_TRUE;
}


geBoolean DRIVERCC BeginWorld(void)
{
	RenderMode = RENDER_WORLD;

	OGLDRV.NumWorldPixels = 0;
	OGLDRV.NumWorldSpans = 0;

	return TRUE;
}


geBoolean DRIVERCC EndWorld(void)
{
	RenderMode = RENDER_NONE;


	return TRUE;
}


geBoolean DRIVERCC BeginMeshes(void)
{
	RenderMode = RENDER_MESHES;

	return TRUE;
}


geBoolean DRIVERCC EndMeshes(void)
{
	RenderMode = RENDER_NONE;

	return TRUE;
}


geBoolean DRIVERCC BeginModels(void)
{
	RenderMode = RENDER_MODELS;

	return TRUE;
}


geBoolean DRIVERCC EndModels(void)
{
	RenderMode = RENDER_NONE;

	return TRUE;
}

// changed QD Shadows
geBoolean DRIVERCC BeginShadowVolumes(void)
{
	return TRUE;
}

geBoolean DRIVERCC EndShadowVolumes(void)
{
	return TRUE;
}
// end change