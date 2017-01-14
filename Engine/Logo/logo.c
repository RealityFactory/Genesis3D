/****************************************************************************************/
/*  LOGO.C                                                                              */
/*                                                                                      */
/*  Author: Eli Boling                                                                  */
/*  Description: The Genesis3D Logo implementation                                      */
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
#define WIN32_LEAN_AND_MEAN
#pragma warning(disable : 4201 4214 4115)
#include <windows.h>
#include <mmsystem.h> //timeGetTime
#pragma warning(default : 4201 4214 4115)

#include	<math.h>

#include	"genesis.h"
#include	"engine.h"
#include	"bitmap.h"
#include	"PixelFormat.h"
#include	"errorlog.h"
#include	"electric.h"

extern	unsigned char	LogoActor_act[];
extern	int				LogoActor_act_Length;
extern	unsigned char	Corona_bmp[];
extern	int				Corona_bmp_Length;
extern	unsigned char	Streak_bmp[];
extern	int				Streak_bmp_Length;
extern	unsigned char	WebUrl_bmp[];
extern	int				WebUrl_bmp_Length;
extern	unsigned char	A_Corona_bmp[];
extern	int				A_Corona_bmp_Length;
extern	unsigned char	A_Streak_bmp[];
extern	int				A_Streak_bmp_Length;

#pragma warning (disable:4514)	// unreferenced inline function (caused by Windows)

static void SubLarge(LARGE_INTEGER *start, LARGE_INTEGER *end, LARGE_INTEGER *delta)
{
	_asm {
		mov ebx,dword ptr [start]
		mov esi,dword ptr [end]

		mov eax,dword ptr [esi+0]
		sub eax,dword ptr [ebx+0]

		mov edx,dword ptr [esi+4]
		sbb edx,dword ptr [ebx+4]

		mov ebx,dword ptr [delta]
		mov dword ptr [ebx+0],eax
		mov dword ptr [ebx+4],edx
	}
}

static	geBoolean	GetBonePosition(geActor *Actor, const char *BoneName, geVec3d *Pos)
{
	geXForm3d	BoneXForm;
	geBoolean	Result;

	Result = geActor_GetBoneTransform(Actor, BoneName, &BoneXForm);
	*Pos = BoneXForm.Translation;
	return Result;
}

static	geLight *AddBoneLight(geWorld *World, geActor *Actor, const char *BoneName, 
							 geFloat R, geFloat G, geFloat B, int Intensity)
{
	geXForm3d	BoneXForm;
	geLight *	Light;
	GE_RGBA		Color;

	geActor_GetBoneTransform(Actor, BoneName, &BoneXForm);
	BoneXForm.Translation.Z += 50;
	Color.r = R;
	Color.g = G;
	Color.b = B;
	Color.a = 255.0f;
	Light = geWorld_AddLight(World);
	if (Light)
		geWorld_SetLightAttributes(World, Light, &BoneXForm.Translation, &Color, (geFloat)Intensity, GE_FALSE);
	return Light;
}

static	geBitmap *	GetABitmap(void *BmpData, int BmpLength, void *AlphaData, int AlphaLength)
{
	geBitmap *				Bitmap;
	geBitmap *				Alpha;
	geVFile *				MemFile;
	geVFile_MemoryContext	Context;

	Context.Data	   = BmpData;
	Context.DataLength = BmpLength;
	
	MemFile = geVFile_OpenNewSystem(NULL, GE_VFILE_TYPE_MEMORY, NULL, &Context, GE_VFILE_OPEN_READONLY);
	if	(!MemFile)
		return NULL;
	Bitmap = geBitmap_CreateFromFile(MemFile);
	geVFile_Close(MemFile);
	if	(!Bitmap)
		return NULL;

	Context.Data	   = AlphaData;
	Context.DataLength = AlphaLength;
	
	MemFile = geVFile_OpenNewSystem(NULL, GE_VFILE_TYPE_MEMORY, NULL, &Context, GE_VFILE_OPEN_READONLY);
	if	(!MemFile)
	{
		geBitmap_Destroy(&Bitmap);
		return NULL;
	}
	Alpha = geBitmap_CreateFromFile(MemFile);
	geVFile_Close(MemFile);
	if	(!Alpha)
	{
		geBitmap_Destroy(&Bitmap);
		return NULL;
	}

	if	(!geBitmap_SetAlpha(Bitmap, Alpha))
	{
		geBitmap_Destroy(&Bitmap);
		geBitmap_Destroy(&Alpha);
		return NULL;
	}

	geBitmap_Destroy(&Alpha);

	geBitmap_SetPreferredFormat(Bitmap, GE_PIXELFORMAT_16BIT_4444_ARGB);

	return Bitmap;
}

geBoolean DoSplashScreen(geEngine *Engine, geDriver_Mode *DriverMode)
{
	geActor_Def *		ActorDef;
	geCamera *			Camera;
	geVFile *			MemFile;
	geVFile_MemoryContext	Context;
	geBoolean			Result;
	geActor *			Actor;
	geWorld *			World;
	geFloat				StartTime;
	geFloat				CurrentTime;
	geFloat				EndTime;
	geMotion *			Motion;
	geXForm3d			CameraXForm;
	geRect 				Rect;
	geXForm3d			ActorXForm;
	geBoolean			KeepGoing;
	int32				Width, Height;

	geVec3d				LightingNormal;
	LARGE_INTEGER		CurrentTic;

	_Electric_BoltEffect *Bolt;
	geVec3d				BoltStart;
	geVec3d				BoltEnd;

	geBitmap *			Corona;
	geBitmap *			Streak;
	geBitmap *			WebUrl;
	GE_LVertex 			CoronaVert;
	GE_LVertex 			StreakVert;
	geVec3d				CoronaVertex;
	geLight *			CoronaLight;

static	geBoolean		DisplayedOnceAlready = GE_FALSE;

	if	(DisplayedOnceAlready == GE_TRUE)
		return GE_TRUE;
		
	DisplayedOnceAlready = GE_TRUE;

	Actor = NULL;
	Camera = NULL;
	World = NULL;

	geDriver_ModeGetWidthHeight(DriverMode, &Width, &Height);

	if (Width == -1)
	{
		RECT	R;
	
		GetClientRect(Engine->hWnd, &R);
		
		Rect.Left = R.left;
		Rect.Right = R.right;
		Rect.Top = R.top;
		Rect.Bottom = R.bottom;
	}
	else
	{
		Rect.Left = 0;
		Rect.Right = Width-1;
		Rect.Top = 0;
		Rect.Bottom = Height-1;
	}

	World = geWorld_Create(NULL);

	if	(!World)
		goto fail;

	// Add the world to the engine, so we can render it
	if (!geEngine_AddWorld(Engine, World))
	{
		geErrorLog_AddString(-1, "DoSplashScreen:  geEngine_AddWorld failed.", NULL);
		goto fail;
	}

	// Open the actor def file
	Context.Data = LogoActor_act;
	Context.DataLength = LogoActor_act_Length;
	MemFile = geVFile_OpenNewSystem(NULL, GE_VFILE_TYPE_MEMORY, NULL, &Context, GE_VFILE_OPEN_READONLY);
	if	(!MemFile)
		return GE_FALSE;
	
	// Create the actor def
	ActorDef = geActor_DefCreateFromFile(MemFile);
	geVFile_Close(MemFile);
	if	(!ActorDef)
		return GE_FALSE;
	
	// Create the actor form the actor def
	Actor = geActor_Create(ActorDef);
	// Remove the ref count that was just added by geACtor_Create
	geActor_DefDestroy(&ActorDef);
	if	(!Actor)
		goto fail;	// Oops
	
	// Add the actor to the NULL world
	Result = geWorld_AddActor(World, Actor, GE_ACTOR_RENDER_ALWAYS, 0xffffffff);
	if	(Result == GE_FALSE)
		goto fail;
	
	Motion = geActor_GetMotionByIndex(ActorDef, 0);
	if	(!Motion)
		goto fail;

	if	(geMotion_GetTimeExtents(Motion, &StartTime, &EndTime) == GE_FALSE)
		goto fail;
	CurrentTime = StartTime;

	Camera = geCamera_Create(2.0f, &Rect);
	if	(!Camera)
		goto fail;

	geXForm3d_SetTranslation(&CameraXForm, 0.0f, 40.0f, 800.0f);
	geCamera_SetWorldSpaceXForm(Camera, &CameraXForm);
	geXForm3d_SetXRotation(&ActorXForm, -3.1415926f / 2.0f);
	geActor_SetPose(Actor, Motion, 0.0f, &ActorXForm);

#define BRIGHTNESS (1.7f)
#define BRIGHTEN(XX)  ((((XX)*BRIGHTNESS)>255.0f)?255.0f:((XX)*BRIGHTNESS) )

#define FILL_LIGHT_RED   (10.0f + 20.0f)
#define FILL_LIGHT_GREEN (10.0f + 20.0f)
#define FILL_LIGHT_BLUE  (10.0f + 20.0f)

#define AMB_LIGHT_RED    (5.0f + 0.0f)
#define AMB_LIGHT_GREEN  (5.0f + 0.0f)
#define AMB_LIGHT_BLUE   (5.0f + 0.0f)

#define	GEAR_R			 BRIGHTEN(251.0f)
#define	GEAR_G			 BRIGHTEN(155.0f)
#define	GEAR_B			 BRIGHTEN(110.0f)

#define	PISTON_R		 BRIGHTEN(150.0f)
#define	PISTON_G		 BRIGHTEN(80.0f)
#define	PISTON_B		 BRIGHTEN(4.0f)

#define GENESISB_R		 BRIGHTEN(255.0f)
#define GENESISB_G		 BRIGHTEN(255.0f)
#define GENESISB_B		 BRIGHTEN(255.0f)

#define BONE01_R		 BRIGHTEN(75.0f)
#define BONE01_G		 BRIGHTEN(56.0f)
#define BONE01_B		 BRIGHTEN(2.0f)

#define LINKTO_R		 BRIGHTEN(75.0f)
#define LINKTO_G		 BRIGHTEN(40.0f)
#define LINKTO_B		 BRIGHTEN(2.0f)

#define CORONALIGHT1_R  (100.0f)
#define CORONALIGHT1_G	(100.0f)
#define CORONALIGHT1_B  (255.0f)

#define CORONALIGHT2_R  (255.0f)
#define CORONALIGHT2_G	(255.0f)
#define CORONALIGHT2_B  (255.0f)

#define CORONALIGHT_RADIUS (1600.0f)

	geVec3d_Set(&LightingNormal, -0.3f, /*1.0f*/0.5f, 0.4f);
	geVec3d_Normalize(&LightingNormal);
	geActor_SetLightingOptions(Actor,
							   GE_TRUE,
							   &LightingNormal,
							   FILL_LIGHT_RED,FILL_LIGHT_GREEN,FILL_LIGHT_BLUE,
							   AMB_LIGHT_RED,AMB_LIGHT_GREEN,AMB_LIGHT_BLUE,
							   GE_FALSE,
							   20,
							   NULL,
							   GE_TRUE);


	AddBoneLight(World, Actor, "GEAR01B", GEAR_R, GEAR_G, GEAR_B, 200);
	AddBoneLight(World, Actor, "GEAR02B", GEAR_R, GEAR_G, GEAR_B, 200);
	AddBoneLight(World, Actor, "GEAR03B", GEAR_R, GEAR_G, GEAR_B, 200);
	AddBoneLight(World, Actor, "GEAR04B", GEAR_R, GEAR_G, GEAR_B, 200);
	AddBoneLight(World, Actor, "GEAR05B", GEAR_R, GEAR_G, GEAR_B, 200);
	AddBoneLight(World, Actor, "GEAR06B", GEAR_R, GEAR_G, GEAR_B, 200);
	AddBoneLight(World, Actor, "GEAR07B", GEAR_R, GEAR_G, GEAR_B, 200);
	AddBoneLight(World, Actor, "GEAR08B", GEAR_R, GEAR_G, GEAR_B, 200);

	AddBoneLight(World, Actor, "GENESISB", 255, 255, 255, 200);
	AddBoneLight(World, Actor, "BONE01", 75, 56, 2, 60);
	AddBoneLight(World, Actor, "LINKTO", 75, 40, 2, 200);

	AddBoneLight(World, Actor, "R_CYL01", PISTON_R, PISTON_G, PISTON_B, 150);
	AddBoneLight(World, Actor, "R_CYL02", PISTON_R, PISTON_G, PISTON_B, 150);
	AddBoneLight(World, Actor, "L_CYL01", PISTON_R, PISTON_G, PISTON_B, 150);
	AddBoneLight(World, Actor, "L_CYL02", PISTON_R, PISTON_G, PISTON_B, 150);

	CoronaLight = AddBoneLight(World, Actor, "CORONA", 0,0,0, (int)CORONALIGHT_RADIUS);

	Bolt = _Electric_BoltEffectCreate(32, 4, 0.5f);

	if	(!Bolt)
		goto fail;

	Corona = GetABitmap(Corona_bmp, Corona_bmp_Length, A_Corona_bmp, A_Corona_bmp_Length);
	if	(!Corona)
		goto fail;
	Streak = GetABitmap(Streak_bmp, Streak_bmp_Length, A_Streak_bmp, A_Streak_bmp_Length);
	if	(!Streak)
		goto fail;
	WebUrl = GetABitmap(WebUrl_bmp, WebUrl_bmp_Length, WebUrl_bmp, WebUrl_bmp_Length );
	if	(!WebUrl)
		goto fail;

	geWorld_AddBitmap(World, Corona);
	geWorld_AddBitmap(World, Streak);
	geWorld_AddBitmap(World, WebUrl);

	GetBonePosition(Actor, "CORONA", (geVec3d *)&CoronaVert.X);
	CoronaVert.u = CoronaVert.v = 0.0f;
	CoronaVert.r = CoronaVert.g = CoronaVert.b = CoronaVert.a = 255.0f;
	CoronaVert.X += 30.0f;
	StreakVert = CoronaVert;

	GetBonePosition(Actor, "CORONA", &CoronaVertex);
	CoronaVertex.Z += 100.0f;
	CoronaVertex.X += 30.0f;

	QueryPerformanceCounter(&CurrentTic);

	KeepGoing = GE_TRUE;

	while	(KeepGoing)		// Play the entire animation
	{
		LARGE_INTEGER		NowTic, DeltaTic;
		geFloat				CoronaScale;
		geFloat				StreakScale;
					 
		if	(CurrentTime >= EndTime)
		{
			CurrentTime = EndTime;
			KeepGoing = GE_FALSE;
		}

		geActor_SetPose(Actor, Motion, CurrentTime, &ActorXForm);
		
		if	(!geEngine_BeginFrame(Engine, Camera, GE_TRUE))
			goto fail;

		GetBonePosition(Actor, "BONE22", &BoltStart);
		GetBonePosition(Actor, "BONE28", &BoltEnd);

		if (CoronaLight)
			{
				static float Random = 0.76324f;
				GE_RGBA Color;
				geFloat Attenuation = 0.97f * CurrentTime;
				Attenuation += (Random*0.15f)*(CurrentTime>1.0f?0:(1.0f-CurrentTime));
				Random = 1.0f - Random*Random;
				Color.r = CORONALIGHT1_R * Attenuation;
				Color.g = CORONALIGHT1_G * Attenuation;
				Color.b = CORONALIGHT1_B * Attenuation;
				if (Color.r>200.0f) Color.r = 200.0f;
				if (Color.g>200.0f) Color.g = 200.0f;
				if (Color.b>255.0f) Color.b = 255.0f;
				Color.a = 255.0f;
				geWorld_SetLightAttributes(	World,CoronaLight,&CoronaVertex,
							&Color,	CORONALIGHT_RADIUS,GE_FALSE);
			}

		
		if (CurrentTime < 1.0f)
			{
				_Electric_BoltEffectAnimate(Bolt, &BoltStart, &BoltEnd);
				_Electric_BoltEffectRender(World, Bolt, &CameraXForm);
			}
		if	(CurrentTime > 1.0f)
		{
			CoronaScale = (CurrentTime - 1.0f) * 1.75f;
			if	(CoronaScale > 1.0f)
				CoronaScale = 1.0f;
			CoronaVert.a = 255.0f * CoronaScale;
			StreakScale = CurrentTime * 5.0f;
			if	(StreakScale > 10.0f)
				StreakScale = 10.0f;
			geWorld_AddPolyOnce(World, &CoronaVert, 1, Corona, GE_TEXTURED_POINT, GE_RENDER_DO_NOT_OCCLUDE_OTHERS | GE_RENDER_DO_NOT_OCCLUDE_SELF, 3.0f);
			geWorld_AddPolyOnce(World, &StreakVert, 1, Streak, GE_TEXTURED_POINT, GE_RENDER_DO_NOT_OCCLUDE_OTHERS | GE_RENDER_DO_NOT_OCCLUDE_SELF, StreakScale);
		}

		// add web url poly
		{
			GE_LVertex 		WebVert;

			WebVert.r = 255.0f;
			WebVert.g = 255.0f;
			WebVert.b = 255.0f;
			WebVert.a = 255.0f;
			WebVert.u = 0.0f;
			WebVert.v = 0.0f;
			WebVert.X = 0.0f;
			WebVert.Y = 0.0f;
			WebVert.Z = 350.0f;

			geWorld_AddPolyOnce( World, &WebVert, 1, WebUrl, GE_TEXTURED_POINT, GE_RENDER_DO_NOT_OCCLUDE_OTHERS | GE_RENDER_DO_NOT_OCCLUDE_SELF, 1.0 );
		}

		if	(!geEngine_RenderWorld(Engine, World, Camera, 0.0f))
			goto fail;

		if	(!geEngine_EndFrame(Engine))
			goto fail;

		QueryPerformanceCounter(&NowTic);

		SubLarge(&CurrentTic, &NowTic, &DeltaTic);

//		CurrentTime += ((float)DeltaTic.LowPart / (float)Engine->CPUInfo.Freq) / 10.0f;
//		CurrentTime += ((float)DeltaTic.LowPart / (float)Engine->CPUInfo.Freq) / 45.0f;
		CurrentTime += ((float)DeltaTic.LowPart / (float)Engine->CPUInfo.Freq) / 75.0f;
	}

	Sleep(500);

	// Remove the actor from the world
	geWorld_RemoveActor(World,Actor);
	// Destroy the actor
	geActor_Destroy(&Actor);

	// Rmeove the world from the engine
	if (!geEngine_RemoveWorld(Engine, World))
	{
		geErrorLog_AddString(-1, "DoSplashScreen:  geEngine_RemoveWorld failed.", NULL);
		goto fail;
	}

	// Destroy the world
	geWorld_Free(World);
	// Destroy the camera
	geCamera_Destroy(&Camera);
	
	return GE_TRUE;

fail:
	if (Bolt)
		_Electric_BoltEffectDestroy(Bolt);
	if	(Actor)
		geActor_Destroy(&Actor);
	if	(World)
		geWorld_Free(World);
	if	(Camera)
		geCamera_Destroy(&Camera);

	Engine->Changed = GE_TRUE;
	
	return GE_FALSE;
}

