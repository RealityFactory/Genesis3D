/****************************************************************************************/
/*  PUPPET.C																			*/
/*                                                                                      */
/*  Author: Mike Sandige	                                                            */
/*  Description: Puppet implementation.									.				*/
/*                                                                                      */
/*  Edit History:                                                                       */
/*  08/11/2004 Wendell Buckner                                                          */
/*   BUG FIX: Rendering Transparent Polys properly (2)                                  */
/*	 You must have at least 3 points                                                    */
/*	 Allow processing of 5 or more verts by splitting into seperate gePolys             */
/*  08/10/2004 Wendell Buckner                                                          */
/*   BUG FIX: Dumb copy/paste bug!                                                      */ 
/*  03/24/2004 Wendell Buckner                                                          */
/*   BUG FIX: Rendering Transparent Polys properly (2)                                  */
/*  03/10/2004 Wendell Buckner                                                          */
/*   Ambient light is leeking into objects that should not receive light, each object   */
/*   will have it's   own ambient property so ONLY those objects that should receive,     */
/*   do receive ambient light.                                                          */
/*  02/21/2004 Wendell Buckner                                                          */
/*   DOT3 BUMPMAPPING                                                                   */ 
/*  02/17/2004 Wendell Buckner                                                          */
/*   DOT3 BUMPMAPPING                                                                   */ 
/*  02/12/2004 Wendell Buckner                                                          */ 
/*   SPHEREMAPPING                                                                    */
/*	01/08/2004 Wendell Buckner                                                          */
/*   DOT3 BUMPMAPPING                                                                   */
/*  08/27/2003 Wendell Buckner                                                          */
/*   BUMPMAPPING                                                                        */   
/*	  Remove artificial limit of only 255 bitmaps.                                      */       
/*   Don't just look for a specific bump map pixel format, use the best available of    */
/*   what is found...                                                                   */
/*  08/15/2003 Wendell Buckner 	                                                        */
/*    BUMPMAPPING                                                                       */
/*  04/08/2003 Wendell Buckner 	                                                        */  
/*    BUMPMAPPING                                                                       */
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
/*  Genesis3D Version 1.1 released November 15, 1999                                    */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved                            */
/*                                                                                      */
/****************************************************************************************/
//#define CIRCULAR_SHADOW
#define SHADOW_MAP
//#define PROJECTED_SHADOW

#include <math.h>  //fabs()
#include <assert.h>

#include "Light.h"
#include "World.h"
#include "Trace.h"		//Trace_WorldCollisionExact2()
#include "Surface.h"	//Surf_InSurfBoundingBox()

#include "XFArray.h"
#include "Puppet.h"
#include "Pose.h"
#include "ErrorLog.h"
#include "Ram.h"
#include "TClip.h"

#include "Frustum.h"
#include "ExtBox.h"
#include "BodyInst.h"

#ifdef PROFILE
#include "rdtsc.h"
#endif

#include "Bitmap.h"
#include "Bitmap._h"

#define PUPPET_DEFAULT_MAX_DYNAMIC_LIGHTS 3

typedef struct gePuppet_Color
{
	geFloat				Red,Green,Blue;
} gePuppet_Color;

typedef struct gePuppet_Material
{
	gePuppet_Color		 Color;
	geBoolean			 UseTexture;
	geBitmap			*Bitmap;
	const char			*TextureName;
	const char			*MaterialName;
	const char			*AlphaName;
} gePuppet_Material;

typedef struct gePuppet
{
	geVFile *			 TextureFileContext;
	geBodyInst			*BodyInstance;
	int					 MaterialCount;
	gePuppet_Material	*MaterialArray;
	int					 MaxDynamicLightsToUse;
	int					 LightReferenceBoneIndex;
		
	geVec3d				 FillLightNormal;
	gePuppet_Color		 FillLightColor;			// 0..255
	geBoolean			 UseFillLight;				// use fill light normal
	//Environment mapping...
	geEnvironmentOptions	 internal_env;
	gePuppet_Color		 AmbientLightIntensity;		// 0..1
	geBoolean			 AmbientLightFromFloor;		// use local lighting from floor

	geBoolean			 PerBoneLighting;

	geBoolean			 DoShadow;
	geFloat				 ShadowScale;
	const geBitmap		*ShadowMap;
	int					 ShadowBoneIndex;

    // LWM_ACTOR_RENDERING:
	geFloat				 OverallAlpha ;

		#pragma message ("this goes away!: World")
	geWorld				*World;
	geBoolean			 AmbientLightFromStaticLights;	// use static lights from map   
	geBoolean			 DoTestRayCollision;			//test static light in shadow   
	int					 MaxStaticLightsToUse; 			//max number of light to use

/*  03/10/2004 Wendell Buckner                                                            
     Ambient light is leeking into objects that should not receive light, each object     
     will have it's own ambient property so ONLY those objects that should receive,       
     do receive ambient light.                                                          */
	gePuppet_Color		 Ambient;					// 0..1
// changed QD Shadows
	geBoolean			DoStencilShadow;
	geBoolean			UpdateBodyG;
	const geBodyInst_Geometry *BodyG;
// end change
} gePuppet;

typedef struct
{
	geVec3d			Normal;
	gePuppet_Color	Color;
	geFloat			Distance;
	geFloat			Radius;

/*	01/08/2004 Wendell Buckner
    DOT3 BUMPMAPPING  */
	geVec3d         Position; 

} gePuppet_Light;

typedef struct
{
	geBoolean		UseFillLight;
	geVec3d			FillLightNormal;
	gePuppet_Color	MaterialColor;
	gePuppet_Color  FillLightColor;
	gePuppet_Color	Ambient;
	geVec3d			SurfaceNormal;
	gePuppet_Light StaticLights[MAX_DYNAMIC_LIGHTS];
	int StaticLightCount;
	gePuppet_Light	Lights[MAX_DYNAMIC_LIGHTS];
	int				LightCount;
	geBoolean		PerBoneLighting;
} gePuppet_LightParamGroup;

typedef struct
{
	gePuppet_Light Lights[MAX_DYNAMIC_LIGHTS];
	int LightCount;
} gePuppet_BoneLight;

// Local info stored across multiple puppets to avoid resource waste.
gePuppet_LightParamGroup  gePuppet_StaticLightGrp;
gePuppet_BoneLight		 *gePuppet_StaticBoneLightArray=NULL;
int						  gePuppet_StaticBoneLightArraySize=0;
int						  gePuppet_StaticPuppetCount=0;
int						  gePuppet_StaticFlags[2]={1768710981,560296816};

static geBoolean GENESISCC gePuppet_FetchTextures(gePuppet *P, const geBody *B)
{
	int i;

/* 08/27/2003 Wendell Buckner
    BUMPMAPPING 
	Remove artificial limit of only 255 bitmaps.
/* 04/08/2003 Wendell Buckner 	
    BUMPMAPPING               *
	int BumpBmpNameCount = 0;
	char *BumpBmpNames[255];  */

	assert( P );

/* 08/27/2003 Wendell Buckner
    BUMPMAPPING 
	Remove artificial limit of only 255 bitmaps...
	memset(&BumpBmpNames,0,sizeof(const char *) * 255 );*/

	P->MaterialCount = geBody_GetMaterialCount(B);
	if (P->MaterialCount <= 0)
	{
		return GE_TRUE;
	}
	
	P->MaterialArray = GE_RAM_ALLOCATE_ARRAY(gePuppet_Material, P->MaterialCount);
	if (P->MaterialArray == NULL)
	{
		geErrorLog_Add(ERR_PUPPET_ENOMEM, NULL);
		return GE_FALSE;
	}
	
	for (i=0; i<P->MaterialCount; i++)
	{
		const char *Name;
		geBitmap *Bitmap;
		gePuppet_Material *M = &(P->MaterialArray[i]);
		geBody_GetMaterial( B, i, &(Name), &(Bitmap),
						&(M->Color.Red),&(M->Color.Green),&(M->Color.Blue));

		if (Bitmap == NULL )
		{
			M->Bitmap     = NULL;
			M->UseTexture = GE_FALSE;
		}
		else
		{
			M->UseTexture = GE_TRUE;
			assert( P->World );

			M->Bitmap = Bitmap;
			geBitmap_CreateRef(Bitmap);

			if ( ! geWorld_AddBitmap(P->World,Bitmap) )
			{
				geErrorLog_AddString(-1,"Puppet_FetchTextures : World_AddBitmap", NULL);
				geRam_Free(P->MaterialArray);
				P->MaterialArray = NULL;
				P->MaterialCount = 0;
				return GE_FALSE;
			}
		}
		M->MaterialName = Name;

/* 08/27/2003 Wendell Buckner
    BUMPMAPPING 
	Remove artificial limit of only 255 bitmaps...
/* 04/08/2003 Wendell Buckner 	
    BUMPMAPPING *
	    if ( geBitmap_IsBumpmapName(Name) && (BumpBmpNameCount <255))
		{
		 BumpBmpNames[BumpBmpNameCount] = (char *) Name;
		 BumpBmpNameCount++;
		}; */
	}

/* 08/27/2003 Wendell Buckner
    BUMPMAPPING 
	Remove artificial limit of only 255 bitmaps...
    Don't just look for a specific bump map pixel format, use the best available of what is found...
/* 04/08/2003 Wendell Buckner 	
    BUMPMAPPING *
    for(i= 0; i < BumpBmpNameCount; i++)
	{
	 geBitmap *BumpBmpAlt = NULL;
 	 geBitmap *BumpmapCreated = NULL;

	 BumpmapCreated = geBody_CreateBumpmapByName(B, BumpBmpNames[i], GE_PIXELFORMAT_16BIT_556_UVL); 
	 if(!BumpmapCreated) break;

     BumpBmpAlt = BumpmapCreated;
	 if (BumpBmpAlt) geWorld_AddBitmap (P->World,BumpBmpAlt);
	} */

	do //Create Bump-map Loop...
	{ 

     int BumpMapPixelFormatCount = 0;
     gePixelFormat BumpMapPixelFormats[10];
     gePixelFormat BumpFormat;

/* 02/21/2004 Wendell Buckner
    DOT3 BUMPMAPPING  */
     geBoolean HasBumpDot3 = GE_FALSE;
     HasBumpDot3 = geBitmap_GetEngineSupport(NULL, DRV_SUPPORT_DOT3 );

	 geBitmap_GetBumpMapPixelFormats( NULL, &BumpMapPixelFormats[0],&BumpMapPixelFormatCount);

/* 02/21/2004 Wendell Buckner
    DOT3 BUMPMAPPING 
     if (BumpMapPixelFormatCount == 0 ) break; */
     if ((BumpMapPixelFormatCount == 0) && (!HasBumpDot3) ) break; 
	 
	 for (i=0; i<P->MaterialCount; i++) //Find all bump-maps by name...
	 {
	   const char *Name;
	   geBitmap *Bitmap;
	   gePuppet_Material *M = &(P->MaterialArray[i]);
	   geBody_GetMaterial( B, i, &(Name), &(Bitmap),
						&(M->Color.Red),&(M->Color.Green),&(M->Color.Blue));

/* 02/17/2004 Wendell Buckner
    DOT3 BUMPMAPPING 
	   if ( !geBitmap_IsBumpmapName(Name) ) continue; 

       if (BumpMapPixelFormatCount == 0 ) break; */
       if ( geBitmap_IsBumpmapNameDot3(Name) && HasBumpDot3 )
	   {
 	    geBitmap *BumpBmpAlt = NULL;
 	    geBoolean BumpmapCreated = GE_FALSE;

	    BumpmapCreated = geBody_CreateBumpmapByNameDot3(B, Name); 

		if (BumpmapCreated ) geBody_CreateTangentSpace(B);
	   }
       else if ( geBitmap_IsBumpmapName(Name) && BumpMapPixelFormatCount ) 
	   {
 	    geBitmap *BumpBmpAlt = NULL;
 	    geBitmap *BumpmapCreated = NULL;

//Get the best format...	  
        BumpFormat = BumpMapPixelFormats[BumpMapPixelFormatCount-1];

	    BumpmapCreated = geBody_CreateBumpmapByName(B, Name, BumpFormat); 

	    if(!BumpmapCreated) continue;

        BumpBmpAlt = BumpmapCreated;
	    if (BumpBmpAlt) geWorld_AddBitmap (P->World,BumpBmpAlt);
	   } //Find all bump-maps by name...

	 }// Check if good pixel formats

	}
	while(GE_FALSE); //Create Bump-map Loop...

	return GE_TRUE;
}	

int GENESISCC gePuppet_GetMaterialCount( gePuppet *P )
{
	assert( P );
	return P->MaterialCount;
}

geBoolean GENESISCC gePuppet_GetMaterial( gePuppet *P, int MaterialIndex,
									geBitmap **Bitmap, 
									geFloat *Red, geFloat *Green, geFloat *Blue)
{
	assert( P      );
	assert( Red    );
	assert( Green  );
	assert( Blue   );
	assert( Bitmap );
	assert( MaterialIndex >= 0 );
	assert( MaterialIndex < P->MaterialCount );

	{
		gePuppet_Material *M = &(P->MaterialArray[MaterialIndex]);
		*Bitmap = M->Bitmap;
		*Red    = M->Color.Red;
		*Green  = M->Color.Green;
		*Blue   = M->Color.Blue;
	}
	return GE_TRUE;
}


geBoolean GENESISCC gePuppet_SetMaterial(gePuppet *P, int MaterialIndex, geBitmap *Bitmap, 
										geFloat Red, geFloat Green, geFloat Blue)
{
	assert( P );
	assert( MaterialIndex >= 0 );
	assert( MaterialIndex < P->MaterialCount );

	{
	geBitmap * OldBitmap;
	gePuppet_Material *M = P->MaterialArray + MaterialIndex;

		OldBitmap = M->Bitmap;

		M->Bitmap		= Bitmap;
		M->Color.Red    = Red;
		M->Color.Green  = Green;
		M->Color.Blue   = Blue;

		if ( OldBitmap != Bitmap ) 
		{		
			if ( OldBitmap )
			{
				assert( M->UseTexture );		
				geWorld_RemoveBitmap( P->World, OldBitmap );
				geBitmap_Destroy( &(OldBitmap) );
			}
			
			M->UseTexture = GE_FALSE;

			if ( Bitmap )
			{
				geBitmap_CreateRef(Bitmap);
						
				M->UseTexture = GE_TRUE;

				if ( ! geWorld_AddBitmap(P->World,Bitmap) )
				{
					geErrorLog_AddString(-1,"Puppet_SetMaterial : World_AddBitmap", NULL);
					return GE_FALSE;
				}
			}
		}
	}

	return GE_TRUE;
}
	

gePuppet *GENESISCC gePuppet_Create(geVFile *TextureFS, const geBody *B, geWorld *World)
{
	gePuppet *P;

	assert( geBody_IsValid(B)!=GE_FALSE );
	#pragma message ("Need geWorld_IsValid()")
	
	P = GE_RAM_ALLOCATE_STRUCT(gePuppet);
	if (P==NULL)
	{
		geErrorLog_Add(ERR_PUPPET_ENOMEM, NULL);
		return NULL;
	}
	
	memset(P,0,sizeof(*P));	

	P->BodyInstance = NULL;
	P->MaxDynamicLightsToUse = PUPPET_DEFAULT_MAX_DYNAMIC_LIGHTS;
	P->LightReferenceBoneIndex = GE_POSE_ROOT_JOINT;

	P->FillLightNormal.X = -0.2f;
	P->FillLightNormal.Y = 1.0f;
	P->FillLightNormal.Z = 0.4f;
	geVec3d_Normalize(&(P->FillLightNormal));
	P->FillLightColor.Red    = 0.25f;
	P->FillLightColor.Green  = 0.25f;
	P->FillLightColor.Blue   = 0.25f;
	P->UseFillLight = GE_TRUE;
	// LWM_ACTOR_RENDERING:
	P->OverallAlpha = 255.0f ;

	P->AmbientLightIntensity.Red   = 0.1f;
	P->AmbientLightIntensity.Green = 0.1f;
	P->AmbientLightIntensity.Blue  = 0.1f;
	P->AmbientLightFromFloor = GE_TRUE;

	P->DoShadow = GE_FALSE;
	P->ShadowScale = 0.0f;
	P->ShadowBoneIndex =  GE_POSE_ROOT_JOINT;
	P->TextureFileContext = TextureFS;

	P->World = World;
	
	P->AmbientLightFromStaticLights = GE_FALSE;	//BY DEFAULT DO NOTHING 
	P->DoTestRayCollision = GE_FALSE; 
	P->MaxStaticLightsToUse = PUPPET_DEFAULT_MAX_DYNAMIC_LIGHTS;
 
	//Set default environment options
	P->internal_env.PercentEnvironment = 0.0f;
	P->internal_env.PercentPuppet = 1.0f;
	P->internal_env.PercentMaterial = 1.0f;
	P->internal_env.UseEnvironmentMapping = GE_FALSE;
	P->internal_env.Supercede = GE_TRUE;

	if (gePuppet_FetchTextures(P,B)==GE_FALSE)
	{
		geRam_Free(P);
		return NULL;
	}

	P->BodyInstance = geBodyInst_Create(B);
	if (P->BodyInstance == NULL)
	{
		geErrorLog_Add(ERR_PUPPET_ENOMEM, NULL);
		gePuppet_Destroy( &P );
		return NULL;
	}
	gePuppet_StaticPuppetCount++;

// changed QD Shadows
	P->DoStencilShadow = GE_FALSE;
	P->UpdateBodyG = GE_TRUE;
	P->BodyG = NULL;
// end change
	return P;
}


void GENESISCC gePuppet_Destroy(gePuppet **P)
{
	assert( P  );
	assert( *P );
	if ( (*P)->BodyInstance )
	{
		geBodyInst_Destroy( &((*P)->BodyInstance) );
		(*P)->BodyInstance = NULL;
	}
	if ( (*P)->MaterialArray )
	{
		gePuppet_Material *M;
		int i;

		for (i=0; i<(*P)->MaterialCount; i++)
		{
			M = &((*P)->MaterialArray[i]);
			if (M->UseTexture )
			{	
/* 08/15/2003 Wendell Buckner 	
    BUMPMAPPING               */
                geBitmap *BumpBmpAlt = NULL;
				
				assert( M->Bitmap );
				geWorld_RemoveBitmap( (*P)->World, M->Bitmap );

/* 02/17/2003 Wendell Buckner 	
    DOT3 BUMPMAPPING               */
                geBitmap_DestroyBumpmapDot3 ( M->Bitmap );

/* 08/15/2003 Wendell Buckner 	
    BUMPMAPPING               */
                BumpBmpAlt = geBitmap_DestroyBumpmap ( M->Bitmap );

                if ( BumpBmpAlt )
				{
				 geWorld_RemoveBitmap( (*P)->World, BumpBmpAlt );
				 geBitmap_Destroy( &(BumpBmpAlt) );
				}

				geBitmap_Destroy( &(M->Bitmap) );
				M->UseTexture = GE_FALSE;
			}
		}


		geRam_Free( (*P)->MaterialArray );
		(*P)->BodyInstance = NULL;
	}
	if ( (*P)->ShadowMap )
	{
		geBitmap_Destroy((geBitmap **)&((*P)->ShadowMap));
		(*P)->ShadowMap = NULL;
	}

	geRam_Free( (*P) );
	*P = NULL;
	
	// clean up any shared resources.
	gePuppet_StaticPuppetCount--;
	if (gePuppet_StaticPuppetCount==0)
	{
		if (gePuppet_StaticBoneLightArray!=NULL)
			geRam_Free(gePuppet_StaticBoneLightArray);

		gePuppet_StaticBoneLightArray=NULL;
		gePuppet_StaticBoneLightArraySize = 0;
	}	
}

void GENESISCC gePuppet_GetStaticLightingOptions(const gePuppet *P,	geBoolean *UseAmbientLightFromStaticLights,	geBoolean *TestRayCollision, int *MaxStaticLightsToUse	)
{	assert( P != NULL);
	assert( UseAmbientLightFromStaticLights );
	assert( MaxStaticLightsToUse );
	*UseAmbientLightFromStaticLights = P->AmbientLightFromStaticLights;
	*TestRayCollision = P->DoTestRayCollision;
	*MaxStaticLightsToUse = P->MaxStaticLightsToUse;
}	

void GENESISCC gePuppet_SetStaticLightingOptions(gePuppet *P, geBoolean UseAmbientLightFromStaticLights, geBoolean TestRayCollision, int MaxStaticLightsToUse	)
{	assert( P!= NULL);
	P->AmbientLightFromStaticLights = UseAmbientLightFromStaticLights;
	P->DoTestRayCollision = TestRayCollision;
	P->MaxStaticLightsToUse = MaxStaticLightsToUse;
}	

void GENESISCC gePuppet_GetLightingOptions(const gePuppet *P,
	geBoolean *UseFillLight,
	geVec3d *FillLightNormal,
	geFloat *FillLightRed,				
	geFloat *FillLightGreen,				
	geFloat *FillLightBlue,				
	geFloat *AmbientLightRed,			
	geFloat *AmbientLightGreen,			
	geFloat *AmbientLightBlue,			
	geBoolean *UseAmbientLightFromFloor,
	int *MaximumDynamicLightsToUse,		
	int *LightReferenceBoneIndex,
	geBoolean *PerBoneLighting
	)
{
	geFloat Scaler;
	assert( P != NULL);
	assert( UseFillLight );
	assert( FillLightNormal );
	assert( FillLightRed );	
	assert( FillLightGreen );	
	assert( FillLightBlue );	
	assert( AmbientLightRed );
	assert( AmbientLightGreen );			
	assert( AmbientLightBlue );			
	assert( UseAmbientLightFromFloor );
	assert( MaximumDynamicLightsToUse );	
	assert( LightReferenceBoneIndex );
		
	*UseFillLight = P->UseFillLight;

	*FillLightNormal = P->FillLightNormal;
	
	Scaler = 255.0f;
	*FillLightRed   = P->FillLightColor.Red * Scaler;
	*FillLightGreen = P->FillLightColor.Green * Scaler;
	*FillLightBlue  = P->FillLightColor.Blue * Scaler;
	
	*AmbientLightRed    = P->AmbientLightIntensity.Red * Scaler;
	*AmbientLightGreen  = P->AmbientLightIntensity.Green * Scaler;
	*AmbientLightBlue   = P->AmbientLightIntensity.Blue * Scaler;
	
	*UseAmbientLightFromFloor  = P->AmbientLightFromFloor;
	*MaximumDynamicLightsToUse = P->MaxDynamicLightsToUse;
	*LightReferenceBoneIndex   = P->LightReferenceBoneIndex;
	*PerBoneLighting		   = P->PerBoneLighting;
}	

void GENESISCC gePuppet_SetLightingOptions(gePuppet *P,
	geBoolean UseFillLight,
	const geVec3d *FillLightNormal,
	geFloat FillLightRed,				// 0 .. 255
	geFloat FillLightGreen,				// 0 .. 255
	geFloat FillLightBlue,				// 0 .. 255
	geFloat AmbientLightRed,			// 0 .. 255
	geFloat AmbientLightGreen,			// 0 .. 255
	geFloat AmbientLightBlue,			// 0 .. 255
	geBoolean UseAmbientLightFromFloor,
	int MaximumDynamicLightsToUse,		// 0 for none
	int LightReferenceBoneIndex,
	geBoolean PerBoneLighting
	)
{
	geFloat Scaler;
	assert( P!= NULL);
	assert( FillLightNormal );
	assert( geVec3d_IsNormalized(FillLightNormal) );
	assert( MaximumDynamicLightsToUse >= 0 );
	assert( (LightReferenceBoneIndex >=0) || (LightReferenceBoneIndex==GE_POSE_ROOT_JOINT));
		
	P->UseFillLight = UseFillLight;

	P->FillLightNormal = *FillLightNormal;
	
	Scaler = 1.0f/255.0f;

	P->FillLightColor.Red   = FillLightRed   * Scaler;
	P->FillLightColor.Green = FillLightGreen * Scaler;
	P->FillLightColor.Blue  = FillLightBlue  * Scaler;
	
	P->AmbientLightIntensity.Red   = AmbientLightRed   * Scaler;
	P->AmbientLightIntensity.Green = AmbientLightGreen * Scaler;
	P->AmbientLightIntensity.Blue  = AmbientLightBlue  * Scaler;
	
	P->AmbientLightFromFloor =UseAmbientLightFromFloor;
	P->MaxDynamicLightsToUse = MaximumDynamicLightsToUse;
	P->LightReferenceBoneIndex = LightReferenceBoneIndex;
	P->PerBoneLighting		 = 	PerBoneLighting;
}	


static int GENESISCC gePuppet_PrepLights(const gePuppet *P, 
	geWorld *World,
	gePuppet_Light *LP,
	const geVec3d *ReferencePoint)
{
	int i,j,cnt;

	Light_LightInfo *L;
	assert( P );
	assert( LP );

	L = (World->LightInfo);
	for(i=0,cnt=0; i<MAX_DYNAMIC_LIGHTS; i++)
	{
		if (L->DynamicLights[i].Active)
		{
			geVec3d *Position = &(L->DynamicLights[i].Pos);
			geVec3d Normal;

			geVec3d_Subtract(Position,ReferencePoint,&Normal);

			LP[cnt].Distance =	Normal.X * Normal.X + 
								Normal.Y * Normal.Y +
								Normal.Z * Normal.Z;
			if (LP[cnt].Distance < L->DynamicLights[i].Radius*L->DynamicLights[i].Radius)
			{
			// changed QuestOfDreams DSpotLight
				if(L->DynamicLights[i].Spot)
				{
					geFloat Angle = -geVec3d_DotProduct(&Normal, &L->DynamicLights[i].Normal);
					if(Angle < L->DynamicLights[i].Angle)
						continue;
				}
			// end change QuestOfDreams		
				LP[cnt].Color.Red = L->DynamicLights[i].Color.r;
				LP[cnt].Color.Green = L->DynamicLights[i].Color.g;
				LP[cnt].Color.Blue = L->DynamicLights[i].Color.b;
				LP[cnt].Radius = L->DynamicLights[i].Radius;
				LP[cnt].Normal = Normal;

/*	01/08/2004 Wendell Buckner
    DOT3 BUMPMAPPING */
	            LP[cnt].Position = *Position; 

				cnt++;
			}
		}
	}

	// sort dynamic lights by distance (squared)
	for(i=0; i<P->MaxDynamicLightsToUse && i<cnt; i++) //rush out    when enough lights sorted
    {
		for(j=i+1; j<cnt; j++)
		{
			if (LP[i].Distance > LP[j].Distance)
			{
				gePuppet_Light Swap = LP[j];
				LP[j] = LP[i];
				LP[i] = Swap;
			}
		}
	}

	// go back and finish setting up closest lights
	for(i=0; i<cnt; i++)
	{
		geFloat Distance = (geFloat)sqrt(LP[i].Distance);
		geFloat OneOverDistance;
		geFloat Scale;
		if (Distance < 1.0f)
			Distance = 1.0f;
	
		OneOverDistance = 1.0f / Distance;
		LP[i].Normal.X *= OneOverDistance;
		LP[i].Normal.Y *= OneOverDistance;
		LP[i].Normal.Z *= OneOverDistance;

		LP[i].Distance = Distance;

		//assert( Distance  < LP[i].Radius );

		Scale = 1.0f - Distance / LP[i].Radius ;
		Scale *= (1.0f/255.0f);
		LP[i].Color.Red *= Scale;
		LP[i].Color.Green *= Scale;
		LP[i].Color.Blue *= Scale;
	}
			
	return cnt;			
}
	
static int  GENESISCC gePuppet_ComputeAmbientLight(
		const gePuppet *P, 
		const geWorld *World, 
		gePuppet_Color *Ambient,
		gePuppet_Light *LP, //Xing studios
		const geVec3d *ReferencePoint)
{
/*  03/10/2004 Wendell Buckner                                                            
     Ambient light is leeking into objects that should not receive light, each object     
     will have it's own ambient property so ONLY those objects that should receive,       
     do receive ambient light.                                                          */
	gePuppet_Color *pcAmbient;

	assert( P );
	assert( World );
	assert( Ambient );
	
	if (P->AmbientLightFromFloor != GE_FALSE)
	{
#define GE_PUPPET_MAX_AMBIENT (0.3f)
		int32			Node, Plane, i;
		geVec3d			Pos1, Pos2, Impact;
		GFX_Node		*GFXNodes;
		Surf_SurfInfo	*Surf;
		GE_RGBA			RGBA;
		//geBoolean		Col1, Col2;
		
		GFXNodes = World->CurrentBSP->BSPData.GFXNodes;
		
		Pos1 = *ReferencePoint;
		
		Pos2 = Pos1;
		
		Pos2.Y -= 30000.0f;
		
		if(!Trace_WorldCollisionExact3((geWorld*)World, &Pos1, &Pos1, &Impact,    &Node, &Plane, NULL))		//just save one test	//xing studios
		{
			// Now find the color of the mesh by getting the lightmap point he is standing    on...
			if (Trace_WorldCollisionExact3((geWorld*)World, &Pos1, &Pos2, &Impact,    &Node, &Plane, NULL))
			{
				Surf = &(World)->CurrentBSP->SurfInfo[GFXNodes[Node].FirstFace];
				if (Surf->LInfo.Face<0)
				{ // FIXME? surface has no light...
					Ambient->Red = Ambient->Green = Ambient->Blue = 0.0f;
				}
				else 
				{
					for (i=0; i< GFXNodes[Node].NumFaces; i++)
					{
						if (Surf_InSurfBoundingBox(Surf, &Impact, 20.0f))
						{
							Light_SetupLightmap(&Surf->LInfo, NULL); 
							
							if (Light_GetLightmapRGB(Surf, &Impact, &RGBA))
							{
								geFloat Scale = 1.0f / 255.0f;
								Ambient->Red = RGBA.r * Scale;
								Ambient->Green = RGBA.g * Scale;
								Ambient->Blue = RGBA.b * Scale;
								if (Ambient->Red > GE_PUPPET_MAX_AMBIENT) 
								{
									Ambient->Red = GE_PUPPET_MAX_AMBIENT;
								}
								if (Ambient->Green > GE_PUPPET_MAX_AMBIENT) 
								{
									Ambient->Green = GE_PUPPET_MAX_AMBIENT;
								}
								if (Ambient->Blue > GE_PUPPET_MAX_AMBIENT) 
								{
									Ambient->Blue = GE_PUPPET_MAX_AMBIENT;
								}
								break;
							}
						}
						Surf++;
					}
				}
			}
		}
	}

/*  03/10/2004 Wendell Buckner                                                            
	Ambient light is leeking into objects that should not receive light, each object     
    will have it's own ambient property so ONLY those objects that should receive,       
	do receive ambient light.                                                          */
	pcAmbient = (gePuppet_Color *) &P->Ambient;
	*pcAmbient = *Ambient;
    gePuppet_StaticLightGrp.Ambient = *Ambient;
	
	if(P->AmbientLightFromStaticLights != GE_FALSE) 
	{
		int i,j,cnt;
		geEntity_EntitySet * entitySet = NULL;
		geEntity * entity = NULL;
		light * aLight;
		spotlight * sLight;
		// 08.05.2004 - begin change gekido
		// made (geWorld*)World instead of just World for GetEntitySet parameter - had a warning during compile
		entitySet = geWorld_GetEntitySet((geWorld*)World, "light");
		if (entitySet != NULL)
			entity = geEntity_EntitySetGetNextEntity(entitySet, entity);
		cnt=0;
		//loop through all static lights and select the ones that touch the actor, with 
		//a max limit of MAX_DYNAMIC_LIGHTS
		for (i=0; entity != NULL && cnt<MAX_DYNAMIC_LIGHTS; i++)
		{
			geVec3d *Position;
			geVec3d Normal;
			geBoolean keepLight = GE_TRUE;
			aLight = (light*)geEntity_GetUserData(entity);
			Position = &(aLight->origin);
			geVec3d_Subtract(Position,ReferencePoint,&Normal);
			LP[cnt].Distance = Normal.X * Normal.X    + 
				Normal.Y * Normal.Y +
				Normal.Z * Normal.Z;
			if (LP[cnt].Distance < aLight->light * aLight->light)
			{
				if (P->DoTestRayCollision == GE_TRUE)
				{
					if (!Trace_WorldCollisionExact3((geWorld*)World, ReferencePoint, Position, NULL,    NULL, NULL, NULL))
					{
						LP[cnt].Color.Red = aLight->color.r;
						LP[cnt].Color.Green = aLight->color.g;
						LP[cnt].Color.Blue = aLight->color.b;
						LP[cnt].Radius = (float)aLight->light;
						LP[cnt].Normal = Normal;
						cnt++;
					}
				}
				else 
				{
					LP[cnt].Color.Red = aLight->color.r;
					LP[cnt].Color.Green = aLight->color.g;
					LP[cnt].Color.Blue = aLight->color.b;
					LP[cnt].Radius = (float)aLight->light;
					LP[cnt].Normal = Normal;
					cnt++;
				}
			}
			entity = geEntity_EntitySetGetNextEntity(entitySet,    entity);
		}
// changed QuestOfDreams 05/02/2004	
		// 08.05.2004 - begin change gekido
		// made (geWorld*)World instead of just World for GetEntitySet parameter - had a warning during compile
		entitySet = geWorld_GetEntitySet((geWorld*)World, "spotlight");
		if (entitySet != NULL)
			entity = geEntity_EntitySetGetNextEntity(entitySet, entity);
		
		//loop through all static lights and select the ones that touch the actor, with    a max limit of MAX_DYNAMIC_LIGHTS
		for (i=0; entity != NULL && cnt<MAX_DYNAMIC_LIGHTS; i++)
		{
			geVec3d *Position;
			geVec3d Normal;
			geBoolean keepLight = GE_TRUE;
			sLight = (spotlight*)geEntity_GetUserData(entity);
			Position = &(sLight->origin);
			geVec3d_Subtract(Position,ReferencePoint,&Normal);
			LP[cnt].Distance = Normal.X * Normal.X    + 
				Normal.Y * Normal.Y +
				Normal.Z * Normal.Z;
			
			if (LP[cnt].Distance < sLight->light * sLight->light)
			{
				geVec3d		LDir;
				geXForm3d	XForm;
				geFloat Angle;
				geFloat		LAngle =(geFloat)cos((sLight->arc/360.0f)*GE_PI); 
			
				LDir.X = (sLight->angles.X / 180.0f) * GE_PI;
				LDir.Y = (sLight->angles.Y / 180.0f) * GE_PI;
				LDir.Z = (sLight->angles.Z / 180.0f) * GE_PI;

				geXForm3d_SetEulerAngles(&XForm, &LDir);

				geXForm3d_GetLeft(&XForm, &LDir);
				LDir.X = -LDir.X;
				LDir.Y = -LDir.Y;
				LDir.Z = -LDir.Z;
				
				Angle = -geVec3d_DotProduct(&Normal, &LDir);
				if(Angle < LAngle)
				{
					entity = geEntity_EntitySetGetNextEntity(entitySet, entity);
					continue;
				}

				if (P->DoTestRayCollision == GE_TRUE)
				{
					if (!Trace_WorldCollisionExact3((geWorld*)World, ReferencePoint, Position, NULL,    NULL, NULL, NULL))
					{
						LP[cnt].Color.Red = sLight->color.r;
						LP[cnt].Color.Green = sLight->color.g;
						LP[cnt].Color.Blue = sLight->color.b;
						LP[cnt].Radius = (float)sLight->light;
						LP[cnt].Normal = Normal;

/*	01/08/2004 Wendell Buckner
    DOT3 BUMPMAPPING  */
	                    LP[cnt].Position = *Position; 

						cnt++;
					}
				}
				else 
				{
					LP[cnt].Color.Red = sLight->color.r;
					LP[cnt].Color.Green = sLight->color.g;
					LP[cnt].Color.Blue = sLight->color.b;
					LP[cnt].Radius = (float)sLight->light;
					LP[cnt].Normal = Normal;

/*	01/08/2004 Wendell Buckner
    DOT3 BUMPMAPPING  */
	                LP[cnt].Position = *Position; 

					cnt++;
				}
			}
			entity = geEntity_EntitySetGetNextEntity(entitySet,    entity);
		}
// end change
		// sort static lights by distance    (squared)
		// for(i=0; i<cnt; i++)
		for(i=0; i<P->MaxDynamicLightsToUse && i<cnt; i++) //rush out    when enough lights sorted
		{
			for(j=i+1; j<cnt; j++)
			{
				if (LP[i].Distance > LP[j].Distance)
				{
					gePuppet_Light Swap = LP[j];
					LP[j] = LP[i];
					LP[i] = Swap;
				}
			}
		}
		
		// go back and finish setting up    closest lights
		for (i=0; i<cnt; i++)
		{
			geFloat Distance = (geFloat)sqrt(LP[i].Distance);
			geFloat OneOverDistance;
			geFloat Scale;
			if (Distance < 1.0f)
				Distance = 1.0f;
			OneOverDistance = 1.0f / Distance;
			LP[i].Normal.X *= OneOverDistance;
			LP[i].Normal.Y *= OneOverDistance;
			LP[i].Normal.Z *= OneOverDistance;
			LP[i].Distance = Distance;
			Scale = 1.0f - Distance / LP[i].Radius    ;
			Scale *= (1.0f/255.0f);
			LP[i].Color.Red *= Scale;
			LP[i].Color.Green *= Scale;
			LP[i].Color.Blue *= Scale;
		}
		
		return cnt;
	} 
	//if ambient light is static
	if (P->AmbientLightFromFloor == GE_FALSE && P->AmbientLightFromStaticLights    == GE_FALSE)
	{
		*Ambient = P->AmbientLightIntensity;

/*  03/10/2004 Wendell Buckner                                                            
     Ambient light is leeking into objects that should not receive light, each object     
     will have it's own ambient property so ONLY those objects that should receive,       
     do receive ambient light.                                                          */
	     *pcAmbient = *Ambient;
		 gePuppet_StaticLightGrp.Ambient = *Ambient;

	}
	return 0;
}

static void GENESISCC gePuppet_SetVertexColor(
	GE_LVertex *v,int BoneIndex)
{
	geFloat RedIntensity,GreenIntensity,BlueIntensity;
	geFloat Color;						
	int l;

	assert( v );
	
	RedIntensity   = gePuppet_StaticLightGrp.Ambient.Red;
	GreenIntensity = gePuppet_StaticLightGrp.Ambient.Green;
	BlueIntensity  = gePuppet_StaticLightGrp.Ambient.Blue;

	if (gePuppet_StaticLightGrp.UseFillLight)
		{
			geFloat Intensity;
			Intensity = gePuppet_StaticLightGrp.FillLightNormal.X * gePuppet_StaticLightGrp.SurfaceNormal.X + 
						gePuppet_StaticLightGrp.FillLightNormal.Y * gePuppet_StaticLightGrp.SurfaceNormal.Y + 
						gePuppet_StaticLightGrp.FillLightNormal.Z * gePuppet_StaticLightGrp.SurfaceNormal.Z;
			if (Intensity > 0.0)
				{
					RedIntensity   += Intensity * gePuppet_StaticLightGrp.FillLightColor.Red;
					GreenIntensity += Intensity * gePuppet_StaticLightGrp.FillLightColor.Green;
					BlueIntensity  += Intensity * gePuppet_StaticLightGrp.FillLightColor.Blue;
				}
		}

	if (gePuppet_StaticLightGrp.PerBoneLighting)
		{
			gePuppet_BoneLight *L;
			L=&(gePuppet_StaticBoneLightArray[BoneIndex]);

			for (l=0; l<L->LightCount; l++)
				{
					geVec3d *LightNormal;
					geFloat Intensity;
				
					LightNormal = &(L->Lights[l].Normal);

					Intensity=	LightNormal->X * gePuppet_StaticLightGrp.SurfaceNormal.X + 
								LightNormal->Y * gePuppet_StaticLightGrp.SurfaceNormal.Y + 
								LightNormal->Z * gePuppet_StaticLightGrp.SurfaceNormal.Z;
					if (Intensity > 0.0f)
						{
							RedIntensity   += Intensity * L->Lights[l].Color.Red;
							GreenIntensity += Intensity * L->Lights[l].Color.Green;
							BlueIntensity  += Intensity * L->Lights[l].Color.Blue;
						}
				}
		}
	else
		{
			for (l=0; l<gePuppet_StaticLightGrp.LightCount; l++)
				{
					geVec3d *LightNormal;
					geFloat Intensity;
				
					LightNormal = &(gePuppet_StaticLightGrp.Lights[l].Normal);

					Intensity=	LightNormal->X * gePuppet_StaticLightGrp.SurfaceNormal.X + 
								LightNormal->Y * gePuppet_StaticLightGrp.SurfaceNormal.Y + 
								LightNormal->Z * gePuppet_StaticLightGrp.SurfaceNormal.Z;
					if (Intensity > 0.0f)
						{
							RedIntensity   += Intensity * gePuppet_StaticLightGrp.Lights[l].Color.Red;
							GreenIntensity += Intensity * gePuppet_StaticLightGrp.Lights[l].Color.Green;
							BlueIntensity  += Intensity * gePuppet_StaticLightGrp.Lights[l].Color.Blue;
						}
				}
		}
	for (l=0; l<gePuppet_StaticLightGrp.StaticLightCount; l++)
	{
		geVec3d *LightNormal;
		float Intensity;
		
		LightNormal = &(gePuppet_StaticLightGrp.StaticLights[l].Normal);
		Intensity= LightNormal->X * gePuppet_StaticLightGrp.SurfaceNormal.X + 
			LightNormal->Y * gePuppet_StaticLightGrp.SurfaceNormal.Y + 
			LightNormal->Z * gePuppet_StaticLightGrp.SurfaceNormal.Z;
		if (Intensity > 0.0f)
		{
			RedIntensity += Intensity * gePuppet_StaticLightGrp.StaticLights[l].Color.Red;
			GreenIntensity += Intensity * gePuppet_StaticLightGrp.StaticLights[l].Color.Green;
			BlueIntensity += Intensity * gePuppet_StaticLightGrp.StaticLights[l].Color.Blue;
		}
	}
	
	Color = gePuppet_StaticLightGrp.MaterialColor.Red * RedIntensity;
	if (Color > 255.0f)
		Color = 255.0f;
	if (Color < 0.0f)
		Color = 0.0f;
	v->r = Color;

	Color = gePuppet_StaticLightGrp.MaterialColor.Green * GreenIntensity;
	if (Color > 255.0f)
		Color = 255.0f;
	if (Color < 0.0f)
		Color = 0.0f;
	v->g = Color;

	Color = gePuppet_StaticLightGrp.MaterialColor.Blue * BlueIntensity;
	if (Color > 255.0f)
		Color = 255.0f;
	if (Color < 0.0f)
		Color = 0.0f;
	v->b = Color;

}

/*	01/08/2004 Wendell Buckner
    DOT3 BUMPMAPPING */
static void GENESISCC gePuppet_SetVertexColorDot3( GE_LVertex *v, int16 VertexIndex, const geXForm3d *Mdl2WldXFA, const geBodyInst *BI, int16 LightType, geBoolean *Reset )
{
	geVec3d tempPosition;

    geFloat d1 = gePuppet_StaticLightGrp.Lights[0].Distance;
	geFloat d2 = gePuppet_StaticLightGrp.StaticLights[0].Distance;

	assert( v );
 
	if ( !d1 || !d2 )
	{
	 geFloat d3;
     d3 = d1;
	 d1 = d2;
	 d2 = d3;
	}

//	a. Get light position
	if ( d1 < d2 )
	 tempPosition = gePuppet_StaticLightGrp.Lights[0].Position;
    else
	 tempPosition = gePuppet_StaticLightGrp.StaticLights[0].Position;

    geBodyInst_SetVertexColorDot3( tempPosition, Mdl2WldXFA, BI, VertexIndex, &v->r, LightType, Reset );
	
} 

/* 02/12/2004 Wendell Buckner
    SPHEREMAPPING */
static void GENESISCC gePuppet_SetSphereMapUV (	GE_LVertex *v, geVec3d *vNormal, const geCamera *Camera )
{
		const geXForm3d *ObjectToCamera;
		geVec3d SphereNormal;

		ObjectToCamera = geCamera_GetCameraSpaceXForm(Camera);

		assert( ObjectToCamera );
		
		geXForm3d_Rotate(ObjectToCamera,vNormal,&SphereNormal);

//but this doesn't work...
        v->u = (SphereNormal.X/2) + 0.5f;
        v->v = (SphereNormal.Y/2) + 0.5f;

} 

static void GENESISCC gePuppet_DrawShadow(const gePuppet *P, 
						const gePose *Joints, 
						geEngine *Engine, 
						geWorld *World, 
						const geCamera *Camera)
{
	GE_LVertex v[3];
	
	geVec3d Impact;
	geBoolean GoodImpact;
	GFX_Plane		Plane;
	geXForm3d RootTransform;
	
	assert( P );
	assert( World );
	assert( Camera );
	assert( Joints );

	assert( (P->ShadowBoneIndex < gePose_GetJointCount(Joints)) || (P->ShadowBoneIndex ==GE_POSE_ROOT_JOINT));
	assert( (P->ShadowBoneIndex >=0)					    	|| (P->ShadowBoneIndex ==GE_POSE_ROOT_JOINT));

	gePose_GetJointTransform(Joints,P->ShadowBoneIndex,&RootTransform);

/* 03/24/2004 Wendell Buckner
    BUG FIX: Rendering Transparent Polys properly (2) */
    geTClip_SetOverallAlpha ( P->OverallAlpha );
	
	{
		geVec3d			Pos1, Pos2;
		GE_Collision	Collision;

		Pos1 = RootTransform.Translation;
			
		Pos2 = Pos1;

		Pos2.Y -= 30000.0f;

		// Get shadow hit plane impact point
		GoodImpact = Trace_GEWorldCollision(World,
		 		NULL,
		 		NULL,
				&Pos1,  
				&Pos2, 
				GE_CONTENTS_SOLID_CLIP,  
				GE_COLLIDE_MODELS,
				0,
				NULL,
				NULL,
				&Collision);

   		//if(GoodImpact) 
	   		Impact = Collision.Impact;
   		//else
	   		//return;
	}

	Impact.Y += 1.0f;

	v[0].r = v[0].b = v[0].g = 0.0f;
	v[1].r = v[1].b = v[1].g = 0.0f;
	v[2].r = v[2].b = v[2].g = 0.0f;

#ifdef SHADOW_MAP
	{
		int i;
		GE_LVertex s[4];
		geVec3d ws[4];
		geVec3d In,Left;
		geVec3d Up;
		geVec3d Zero = {0.0f,0.0f,0.0f};
		
		geVec3d_Subtract(&Impact,&(RootTransform.Translation),&Up);
		geVec3d_Normalize(&Up);
		geVec3d_CrossProduct(&(Plane.Normal),&Up,&Left);
		if (geVec3d_Compare(&Left,&Zero,0.001f)!=GE_FALSE)
			{
				geXForm3d_GetLeft(&(RootTransform),&Left);
			}
		geVec3d_CrossProduct(&Left,&(Plane.Normal),&In);

		geVec3d_Normalize(&Left);
		geVec3d_Normalize(&In);

		s[0].r = s[0].b = s[0].g = 0.0f;
		s[1].r = s[1].b = s[1].g = 0.0f;
		s[2].r = s[2].b = s[2].g = 0.0f;
		s[3].r = s[3].b = s[3].g = 0.0f;

		geVec3d_Scale(&In  ,P->ShadowScale,&In);
		geVec3d_Scale(&Left,P->ShadowScale,&Left);

		s[0].a = s[1].a = s[2].a = s[3].a  = 160.0f;

		s[0].u = 0.0f; s[0].v = 0.0f;
		s[1].u = 1.0f; s[1].v = 0.0f;
		s[2].u = 1.0f; s[2].v = 1.0f;
		s[3].u = 0.0f; s[3].v = 1.0f;
		ws[0].Y = ws[1].Y = ws[2].Y = ws[3].Y = Impact.Y;

		ws[0].X = RootTransform.Translation.X + Left.X - In.X;
		ws[0].Z = RootTransform.Translation.Z + Left.Z - In.Z;

		ws[1].X = RootTransform.Translation.X - Left.X - In.X;
		ws[1].Z = RootTransform.Translation.Z - Left.Z - In.Z;

		ws[2].X = RootTransform.Translation.X - Left.X + In.X;
		ws[2].Z = RootTransform.Translation.Z - Left.Z + In.Z;
		
		ws[3].X = RootTransform.Translation.X + Left.X + In.X;
		ws[3].Z = RootTransform.Translation.Z + Left.Z + In.Z;

		for (i=0; i<4; i++)
		{
			geCamera_Transform(Camera,&ws[i],&ws[i]);
			geCamera_Project(Camera,&ws[i],&ws[i]);
		}

		
		s[0].X = ws[0].X; s[0].Y = ws[0].Y; s[0].Z = ws[0].Z;
		s[1].X = ws[1].X; s[1].Y = ws[1].Y; s[1].Z = ws[1].Z;
		s[2].X = ws[2].X; s[2].Y = ws[2].Y; s[2].Z = ws[2].Z;
		s[3].X = ws[3].X; s[3].Y = ws[3].Y; s[3].Z = ws[3].Z;
		
		geTClip_SetTexture(P->ShadowMap);

		geTClip_Triangle(s);
		s[1] = s[2];
		s[2] = s[3];

		geTClip_Triangle(s);
	}
#endif
	
#ifdef CIRCULAR_SHADOW
	v[0].a = v[1].a = v[2].a = 160.0f;
	v[0].u = v[1].u = v[2].u = 0.5f;
	v[0].v = v[1].v = v[2].v = 0.5f;
	
	v[0].X = Impact.X;
	v[0].Y = v[1].Y = v[2].Y = Impact.Y;
	v[0].Z = Impact.Z;
	
	{
		int steps = 30;
		int i;
		geVec3d V;
		geFloat Angle = 0.0f;
		geFloat DAngleDStep = -(2.0f * 3.14159f / (geFloat)steps);
		geFloat Radius = P->ShadowScale/2.0f;

		V = Impact;
		geCamera_Transform(Camera,&V,&V);
		geCamera_Project(Camera,&V,&V);
		v[0].X = V.X;
		v[0].Y = V.Y;
		v[0].Z = V.Z;

		//geTClip_SetTexture(NULL);
		geTClip_SetTexture(P->ShadowMap);

		V = Impact;
		V.Z += Radius;
		geCamera_Transform(Camera,&V,&V);
		geCamera_Project(Camera,&V,&V);
		v[1].X = V.X;
		v[1].Y = V.Y;
		v[1].Z = V.Z;
		for (i=0; i<steps+1; i++)
			{
				v[2] = v[1];

				V = Impact;
				V.X += (geFloat)(sin( Angle ) * Radius);
				V.Z += (geFloat)(cos( Angle ) * Radius);
				geCamera_Transform(Camera,&V,&V);
				geCamera_Project(Camera,&V,&V);
				v[1].X = V.X;
				v[1].Y = V.Y;
				v[1].Z = V.Z;

				Angle = Angle + DAngleDStep;
				geEngine_RenderPoly(Engine, (GE_TLVertex *)v, 3, P->ShadowMap, 0 );
				//geTClip_Triangle(v);
			}
	}
#endif

#ifdef PROJECTED_SHADOW			
	{
		int i,j,Count;
		geBodyInst_Index *List;
		geBodyInst_Index Command;
		geBody_SkinVertex *SV;
		
		G = geBodyInst_GetShadowGeometry(P->BodyInstance,
						gePose_GetAllJointTransforms(Joints),0,Camera,&Impact);

		if ( G == NULL )
			{
				geErrorLog_Add(ERR_PUPPET_RENDER, NULL);
				return GE_FALSE;
			}

		geTClip_SetTexture(NULL);

		Count = G->FaceCount;
		List  = G->FaceList;

		for (i=0; i<Count; i++)
			{	
				Command = *List;
				List ++;
				//Material = *List;
				List ++;

				assert( Command == GE_BODY_FACE_TRIANGLE );

				{
					geFloat AX,AY,BXMinusAX,BYMinusAY,CYMinusAY,CXMinusAX;
					geBodyInst_Index *List2;
					
					List2 = List;
					SV = &(G->SkinVertexArray[ *List2 ]);
					AX = SV->SVPoint.X;
					AY = SV->SVPoint.Y;
					List2++;
					List2++;
					
					SV = &(G->SkinVertexArray[ *List2 ]);
					BXMinusAX = SV->SVPoint.X - AX;
					BYMinusAY = SV->SVPoint.Y - AY;
					List2++;
					List2++;

					SV = &(G->SkinVertexArray[ *List2 ]);
					CXMinusAX = SV->SVPoint.X - AX;
					CYMinusAY = SV->SVPoint.Y - AY;
					List2++;
					List2++;

					// ZCROSS is z the component of a 2d vector cross product of ABxAC
					//#define ZCROSS(Ax,Ay,Bx,By,Cx,Cy)  ((((Bx)-(Ax))*((Cy)-(Ay))) - (((By)-(Ay))*((Cx)-(Ax))))

					// 2d cross product of AB cross AC   (A is vtx[0], B is vtx[1], C is vtx[2]
					if ( ((BXMinusAX * CYMinusAY) - (BYMinusAY * CXMinusAX)) > 0.0f )
						{
							List = List2;
							continue;
						}
				}						

			#define SOME_SCALE (  255.0f / 40.0f )
				for (j=0; j<3; j++)
					{
						SV = &(G->SkinVertexArray[ *List ]);
						List++;

						v[j].X = SV->SVPoint.X;
						v[j].Y = SV->SVPoint.Y;
						
						v[j].Z = SV->SVPoint.Z;
						//Environment mapping code
						if( P->internal_env.UseEnvironmentMapping )
						{
							v[j].u = ( SV->SVU * P->internal_env.PercentPuppet );
							v[j].v = ( SV->SVV * P->internal_env.PercentPuppet );
							
							if( P->internal_env.Supercede && !strncmp(PM->MaterialName,"env_", 4))
							{
								v[j].u += ( (G->NormalArray[ *List ]).X * P->internal_env.PercentMaterial);
								v[j].v += ( (G->NormalArray[ *List ]).Y * P->internal_env.PercentMaterial);
							}
							else
							{
								v[j].u += ( (G->NormalArray[ *List ]).X * P->internal_env.PercentEnvironment);
								v[j].v += ( (G->NormalArray[ *List ]).Y * P->internal_env.PercentEnvironment);
							}
						}
						else
						{
							v[j].u = SV->SVU;
							v[j].v = SV->SVV;
							
							if( P->internal_env.Supercede && !strncmp(PM->MaterialName,"env_", 4))
							{
								v[j].u += ( (G->NormalArray[ *List ]).X * P->internal_env.PercentMaterial);
								v[j].v += ( (G->NormalArray[ *List ]).Y * P->internal_env.PercentMaterial);
							}
						}
						
						List++;
						v[j].a = (255.0f- (SV->SVU * SOME_SCALE));
					}
			
				if ((v[0].a > 0) && (v[1].a > 0) && (v[2].a > 0))
					{
						geTClip_Triangle(v);
					}
			}
		assert( ((uint32)List) - ((uint32)G->FaceList) == (uint32)(G->FaceListSize) );
	}
#endif
}

//Environment mapping code...
void GENESISCC gePuppet_SetEnvironmentOptions( gePuppet *P, geEnvironmentOptions *envop )
{
	assert ( P );
	assert ( (envop->UseEnvironmentMapping == GE_TRUE) || (envop->UseEnvironmentMapping == GE_FALSE ) );
	assert ( (envop->Supercede == GE_TRUE) || (envop->Supercede == GE_FALSE) );
	assert ( (envop->PercentEnvironment >= 0.0f) && (envop->PercentEnvironment <= 1.0f) );
	assert ( (envop->PercentMaterial >= 0.0f) && (envop->PercentMaterial <= 1.0f) );
	assert ( (envop->PercentPuppet >= 0.0f) && (envop->PercentPuppet <= 1.0f) );

	P->internal_env.UseEnvironmentMapping = envop->UseEnvironmentMapping;
	P->internal_env.Supercede = envop->Supercede;
	P->internal_env.PercentEnvironment = envop->PercentEnvironment;
// changed QD bug fix
//	P->internal_env.PercentMaterial = envop->PercentEnvironment;
	P->internal_env.PercentMaterial = envop->PercentMaterial;
	P->internal_env.PercentPuppet = envop->PercentPuppet;
}

geEnvironmentOptions GENESISCC gePuppet_GetEnvironmentOptions( gePuppet *P )
{
	assert ( P );
	return P->internal_env;
}

/* 03/24/2004 Wendell Buckner
    BUG FIX: Rendering Transparent Polys properly (2) */
	
geBoolean GENESISCC gePuppet_AddToGList ( const GE_TLVertex *Points, int NumPoints, const geBitmap *Bitmap, uint32 Flags, geBoolean Flush )
{
	static gePoly *PuppetTransPolyList = NULL;
	static gePoly *CurrentPuppetTransPoly = NULL;
	gePoly *PuppetTransPoly = NULL;
    geBoolean AddedToGList = GE_FALSE;
	int i = 0;

/* 08/11/2004 Wendell Buckner
    BUG FIX: Rendering Transparent Polys properly (2)
	 Allow processing of 5 or more verts by splitting int seperate gePolys */
	int j = 0;
    int StartPoint = 0;
    int EndPoint = 0;
	int AddPoints = 0;
    int NewPoints = 0;

	do
	{
		if ( !Points ) break;

/* 08/11/2004 Wendell Buckner
    BUG FIX: Rendering Transparent Polys properly (2)
	 You must have at least 3 points 
		if ( NumPoints < 2 ) break; */
		if ( NumPoints < 3 ) break;

		PuppetTransPoly = GE_RAM_ALLOCATE_STRUCT(gePoly);

		if ( !PuppetTransPoly ) break;
	
		PuppetTransPoly->AddOnceNext = NULL;	
		PuppetTransPoly->LeafData = NULL;
		PuppetTransPoly->Next = NULL;	
		PuppetTransPoly->Prev = NULL;    
		PuppetTransPoly->Scale = 1.0f;

		#ifdef _DEBUG
		 PuppetTransPoly->Self1 = PuppetTransPoly;
		 PuppetTransPoly->Self2 = PuppetTransPoly;
		#endif

		if ( Bitmap )
			PuppetTransPoly->Type = GE_TEXTURED_POLY;
		else
			PuppetTransPoly->Type = GE_GOURAUD_POLY;

		PuppetTransPoly->World = NULL;
		PuppetTransPoly->ZOrder = 0.0f;
    
		PuppetTransPoly->Bitmap = (geBitmap*) Bitmap;

/* 08/11/2004 Wendell Buckner
    BUG FIX: Rendering Transparent Polys properly (2)
	 Allow processing of 5 or more verts by splitting int seperate gePolys
		if ( NumPoints > 4 ) NumPoints = 4; 
		PuppetTransPoly->NumVerts = NumPoints; */

		PuppetTransPoly->RenderFlags = Flags | GE_RENDER_DEPTH_SORT_BF;

/* 08/11/2004 Wendell Buckner
    BUG FIX: Rendering Transparent Polys properly (2)
	 Allow processing of 5 or more verts by splitting int seperate gePolys *
		for ( i=0; i < NumPoints; i++)
		{
			PuppetTransPoly->Verts[i].a = Points[i].a; 
			PuppetTransPoly->Verts[i].b = Points[i].b; 
			PuppetTransPoly->Verts[i].g = Points[i].g; 
			PuppetTransPoly->Verts[i].r = Points[i].r;
			PuppetTransPoly->Verts[i].u = Points[i].u;  
			PuppetTransPoly->Verts[i].v = Points[i].v; 
			PuppetTransPoly->Verts[i].X = Points[i].x; 
			PuppetTransPoly->Verts[i].Y = Points[i].y; 
			PuppetTransPoly->Verts[i].Z = Points[i].z; 
		} */
		if ( StartPoint >= 4 )
		{
			PuppetTransPoly->Verts[0].a = Points[0].a; 
			PuppetTransPoly->Verts[0].b = Points[0].b; 
			PuppetTransPoly->Verts[0].g = Points[0].g; 
			PuppetTransPoly->Verts[0].r = Points[0].r;
			PuppetTransPoly->Verts[0].u = Points[0].u;  
			PuppetTransPoly->Verts[0].v = Points[0].v; 
			PuppetTransPoly->Verts[0].X = Points[0].x; 
			PuppetTransPoly->Verts[0].Y = Points[0].y; 
			PuppetTransPoly->Verts[0].Z = Points[0].z; 

			PuppetTransPoly->Verts[1].a = Points[StartPoint-1].a; 
			PuppetTransPoly->Verts[1].b = Points[StartPoint-1].b; 
			PuppetTransPoly->Verts[1].g = Points[StartPoint-1].g; 
			PuppetTransPoly->Verts[1].r = Points[StartPoint-1].r;
			PuppetTransPoly->Verts[1].u = Points[StartPoint-1].u;  
			PuppetTransPoly->Verts[1].v = Points[StartPoint-1].v; 
			PuppetTransPoly->Verts[1].X = Points[StartPoint-1].x; 
			PuppetTransPoly->Verts[1].Y = Points[StartPoint-1].y; 
			PuppetTransPoly->Verts[1].Z = Points[StartPoint-1].z; 

			j = 2;
			NewPoints = 2;
		}
		else
		{
			j = 0;
			NewPoints = 4;
		}

		AddPoints = NumPoints - StartPoint;

		if ( AddPoints > NewPoints )
		{
			EndPoint += NewPoints;
			AddPoints = NewPoints;
		}
		else
		{
			EndPoint += AddPoints;
		}

		AddPoints += j;

		PuppetTransPoly->NumVerts = AddPoints;

		for ( i = StartPoint; i < EndPoint; i++, j++ )
		{
			PuppetTransPoly->Verts[j].a = Points[i].a; 
			PuppetTransPoly->Verts[j].b = Points[i].b; 
			PuppetTransPoly->Verts[j].g = Points[i].g; 
			PuppetTransPoly->Verts[j].r = Points[i].r;
			PuppetTransPoly->Verts[j].u = Points[i].u;  
			PuppetTransPoly->Verts[j].v = Points[i].v; 
			PuppetTransPoly->Verts[j].X = Points[i].x; 
			PuppetTransPoly->Verts[j].Y = Points[i].y; 
			PuppetTransPoly->Verts[j].Z = Points[i].z; 
		}

		if ( !PuppetTransPolyList ) 
			PuppetTransPolyList = PuppetTransPoly;

		if ( CurrentPuppetTransPoly )
		{
			PuppetTransPoly->Prev = CurrentPuppetTransPoly;
			CurrentPuppetTransPoly->Next = PuppetTransPoly;
		}
			
		CurrentPuppetTransPoly = PuppetTransPoly;

		PuppetTransPolyList->Prev = CurrentPuppetTransPoly;

		AddedToGList = GE_TRUE;

/* 08/11/2004 Wendell Buckner
    BUG FIX: Rendering Transparent Polys properly (2)
	 Allow processing of 5 or more verts by splitting int seperate gePolys */
		StartPoint += NewPoints;

		if ( StartPoint >= NumPoints ) break;
	
	} while (GE_TRUE);

	if ( Flush )
	{
	    if ( PuppetTransPolyList )
			GList_AddOperation ( 2, (uint32) PuppetTransPolyList );

		PuppetTransPolyList = NULL;
		CurrentPuppetTransPoly = NULL;
		AddedToGList = GE_TRUE;
	}

	return AddedToGList;
} 

geBoolean GENESISCC gePuppet_IsTransparent ( const geFloat OverallAlpha, const geBitmap **Bitmap, geBoolean *IsTransparent, uint32 Count )
{
	geBoolean PuppetIsTransparent = GE_FALSE;
	uint32 i = 0;

	do
	{
		if ( Count == 0 ) break;

		PuppetIsTransparent = (OverallAlpha != 255.0f);

		memset ( IsTransparent, PuppetIsTransparent, Count );

		if ( PuppetIsTransparent ) break;

		for ( i = 0; i < Count; i++ )
		{	
			if ( !Bitmap[i] ) continue;

			IsTransparent[i] = geBitmap_HasAlpha(Bitmap[i]); 

			if ( !PuppetIsTransparent && IsTransparent[i])
				PuppetIsTransparent = GE_TRUE;

			if ( IsTransparent[i] ) continue;

			IsTransparent[i] = geBitmap_UsesColorKey(Bitmap[i]); 
		}

	} while ( GE_FALSE );

	return PuppetIsTransparent;
}  

//****
//changed QD Clipping
//int32		NumClips;

// LWM_ACTOR_RENDERING
geFloat GENESISCC gePuppet_GetAlpha( const gePuppet *P )
{
	assert( P ) ;
	return P->OverallAlpha ;
}

// LWM_ACTOR_RENDERING
void GENESISCC gePuppet_SetAlpha( gePuppet *P, geFloat Alpha )
{
	assert( P ) ;
	P->OverallAlpha = Alpha ;
}

// LWM_ACTOR_RENDERING
geBoolean GENESISCC gePuppet_RenderThroughFrustum(const gePuppet *P, 
						const gePose *Joints, 
						const geExtBox *Box, 
						geEngine *Engine, 
						geWorld *World, 
						const geCamera *Camera, 
						Frustum_Info *FInfo)
{
	int32		ClipFlags;
	const geXFArray *JointTransforms;
	const geBodyInst_Geometry *G;
	geVec3d     Scale;


/*	01/08/2004 Wendell Buckner
    DOT3 BUMPMAPPING */
	int BoneCount;
	const geXForm3d *XFA;
	geBoolean ResetDot3 = GE_TRUE;

/* 03/24/2004 Wendell Buckner
    BUG FIX: Rendering Transparent Polys properly (2) */
	geBoolean *MaterialIsTransparent = NULL;
	geBitmap **MaterialArray = NULL;
	int32    MaterialCount = 0;
    int32    i = 0;

// changed QD Performance
	if(P->OverallAlpha==0.0f)
		return GE_TRUE;
// end change
	
	JointTransforms = gePose_GetAllJointTransforms(Joints);
	XFA = geXFArray_GetElements(JointTransforms, &BoneCount);

	assert( P      );
	assert( Engine );
	assert( World  );
	assert( Camera );
	assert( Joints );

	JointTransforms = gePose_GetAllJointTransforms(Joints);

	#pragma message ("Level of detail hacked:")

	gePose_GetScale(Joints,&Scale);
// changed QD Clipping
	//G = geBodyInst_GetGeometry(P->BodyInstance, &Scale, JointTransforms, 0, NULL);
	G = geBodyInst_GetGeometry(P->BodyInstance, &Scale, JointTransforms, 0, Camera);

	if (G == NULL)
	{
		geErrorLog_Add(ERR_PUPPET_RENDER, NULL);
		return GE_FALSE;
	}
// end change

	// Setup clip flags...
	ClipFlags = 0xffff;

// changed QD Clipping 
	{
		//geExtBox	MinMaxs;
		//geVec3d		d1, d2, d3;
		GFX_Plane	*Planes;
		int32		k;
		//geVec3d		Expand = {150.0f, 150.f, 150.0f};
		//int32		OriginalClips;

		//OriginalClips = NumClips;

		//MinMaxs = *Box;
			
		Planes = FInfo->Planes;

		for (k=0; k< FInfo->NumPlanes; k++, Planes++)
		{
			int32		Side;
			
			Planes->Type = PLANE_ANY;
			
			//Side = Trace_BoxOnPlaneSide(&MinMaxs.Min, &MinMaxs.Max, Planes);
			Side = Trace_BoxOnPlaneSide(&(G->Mins), &(G->Maxs), Planes);

			if (Side == PSIDE_BACK)
			{
				//NumClips = OriginalClips;
				return GE_TRUE;
			}
			
			if (Side == PSIDE_FRONT)
			{
				ClipFlags &= ~(1<<k);
			}
			//else
			//	NumClips++;			
		}
	}
// end change


/* 03/24/2004 Wendell Buckner
    BUG FIX: Rendering Transparent Polys properly (2)*/
	MaterialCount = P->MaterialCount;
	MaterialArray = GE_RAM_ALLOCATE_ARRAY ( geBitmap *, MaterialCount );
    MaterialIsTransparent = GE_RAM_ALLOCATE_ARRAY ( geBoolean, MaterialCount );

	for ( i = 0; i < MaterialCount; i++ )
	{
		gePuppet_Material *PM = &(P->MaterialArray[i]);
		MaterialArray[i] = PM->Bitmap;
	}

	gePuppet_IsTransparent ( P->OverallAlpha, MaterialArray, MaterialIsTransparent, MaterialCount );

	{
		int32			NumFaces;
		int32			i;
		geBodyInst_Index	*List;
		geXForm3d		RootTransform;

		gePuppet_StaticLightGrp.UseFillLight		 = P->UseFillLight;
		gePuppet_StaticLightGrp.FillLightNormal		 = P->FillLightNormal;
		gePuppet_StaticLightGrp.FillLightColor.Red	 = P->FillLightColor.Red;
		gePuppet_StaticLightGrp.FillLightColor.Green = P->FillLightColor.Green;
		gePuppet_StaticLightGrp.FillLightColor.Blue  = P->FillLightColor.Blue;
		gePuppet_StaticLightGrp.PerBoneLighting      = P->PerBoneLighting;

		gePose_GetJointTransform(Joints,P->LightReferenceBoneIndex,&(RootTransform));

		// LWM_OPTIMIZATION: It seems that if you know that there are 
		// no dynamic lights anywhere you could skip this test easily
		if (P->MaxDynamicLightsToUse > 0)
		{
			if (P->PerBoneLighting)
			{
				int BoneCount;
				const geXForm3d *XFA = geXFArray_GetElements(JointTransforms, &BoneCount);
				if (BoneCount>0)
				{
					if (gePuppet_StaticBoneLightArraySize < BoneCount)
					{
						gePuppet_BoneLight *LG;
						
						LG = geRam_Realloc(gePuppet_StaticBoneLightArray, sizeof(gePuppet_BoneLight) * BoneCount);
						if (LG==NULL)
						{
							geErrorLog_Add(ERR_PUPPET_RENDER,"Failed to allocate space for bone lighting info cache");
							return GE_FALSE;
						}
						gePuppet_StaticBoneLightArray = LG;
						gePuppet_StaticBoneLightArraySize = BoneCount;
					}
					for (i=0; i<BoneCount; i++)
					{
						gePuppet_StaticBoneLightArray[i].LightCount = gePuppet_PrepLights(P,World,
									gePuppet_StaticBoneLightArray[i].Lights,&(XFA[i].Translation));
					}
				}
			}
			else
			{
				gePuppet_StaticLightGrp.LightCount = gePuppet_PrepLights(P,World
							,gePuppet_StaticLightGrp.Lights,&(RootTransform.Translation));
			}
		}
		else
		{
			gePuppet_StaticLightGrp.LightCount = 0;
		}

		//XING Studios code

/*  03/10/2004 Wendell Buckner                                                            
     Ambient light is leeking into objects that should not receive light, each object     
     will have it's own ambient property so ONLY those objects that should receive,       
     do receive ambient light.                                                          
		gePuppet_StaticLightGrp.StaticLightCount = gePuppet_ComputeAmbientLight(P,World,&(gePuppet_StaticLightGrp.Ambient),gePuppet_StaticLightGrp.StaticLights,&(RootTransform.Translation)); */
		gePuppet_StaticLightGrp.StaticLightCount = gePuppet_ComputeAmbientLight(P,World,(gePuppet_Color *) &(P->Ambient),gePuppet_StaticLightGrp.StaticLights,&(RootTransform.Translation));

	 	NumFaces	= G->FaceCount;
		List		= G->FaceList;
		
		// For each face, clip it to the view frustum supplied...
		for (i=0; i<NumFaces; i++)
		{
			#define MAX_TEMP_VERTS		30		// To be safe...

			int32				Length1, Length2, v, p;
			geVec3d				Dest1[MAX_TEMP_VERTS], *pDest1;
			geVec3d				Dest2[MAX_TEMP_VERTS], *pDest2;
			geVec3d				Verts[MAX_TEMP_VERTS], *pVerts, v1, v2, v3;
			Surf_TexVert		TexVerts[MAX_TEMP_VERTS], Tex1[MAX_TEMP_VERTS], *pTexVerts;
			Surf_TexVert		Tex2[MAX_TEMP_VERTS], *pTex1, *pTex2;			
			Surf_TLVertex		ScreenPts[MAX_TEMP_VERTS];
			GFX_Plane			*FPlanes;
			geBodyInst_Index		Command, Material;
			gePuppet_Material	*PM;
// changed QD Clipping
			//geFloat				Dist;

			Command	= *List;
			List++;
			Material = *List;
			List ++;
			
			assert( Command == GE_BODYINST_FACE_TRIANGLE );
			assert( Material>=0 );
			assert( Material<P->MaterialCount);

			PM = &(P->MaterialArray[Material]);
			gePuppet_StaticLightGrp.MaterialColor = PM->Color;

// changed QD Clipping
			// backface culling
			{
				geBodyInst_Index *List2;
				
				List2 = List;
		
				geVec3d_Copy(&(G->SkinVertexArray[*List2].SVPoint), &Verts[0]);
				List2++;
				List2++;
				geVec3d_Copy(&(G->SkinVertexArray[*List2].SVPoint), &Verts[1]);
				List2++;
				List2++;

				geVec3d_Subtract(&(G->SkinVertexArray[*List2].SVPoint), &Verts[1], &v1);
				List2++;
				List2++;
			
				geVec3d_Subtract(&Verts[0], &Verts[1], &v2);
				geVec3d_CrossProduct(&v1, &v2, &v3);
				geVec3d_Normalize(&v3);

				if(geVec3d_DotProduct(&v3, &Verts[0]) <= 0.0f)
				{
					List = List2;
					continue;
				}
			}
// end change

			Length1 = 3;					//FIXME:  I'm assuming numverts == 3

			pVerts = Verts;
			pTexVerts = TexVerts;

			// AHHH!! Copy over till I get a better way...
			for (v=0; v< Length1; v++, pVerts++, pTexVerts++)
			{
				geBodyInst_SkinVertex	*SVert;
				GE_LVertex lvert;

/*	01/08/2004 Wendell Buckner
    DOT3 BUMPMAPPING */
				geBodyInst_Index vIndex = *List;

/*	03/15/2004 Wendell Buckner
    SPHEREMAPPING */
// changed QD bug fix
				//	geBodyInst_Index nIndex;

				SVert = &G->SkinVertexArray[vIndex];

				List++;

				*pVerts = SVert->SVPoint;

				assert( ((geFloat)fabs(1.0-geVec3d_Length( &(G->NormalArray[ *List ] ))))< 0.001f );

				//Environment mapping code...
				pTexVerts->u = SVert->SVU;
				pTexVerts->v = SVert->SVV;

				if( P->internal_env.UseEnvironmentMapping )
				{
					pTexVerts->u = SVert->SVU*P->internal_env.PercentPuppet;
					pTexVerts->v = SVert->SVV*P->internal_env.PercentPuppet;
					if( P->internal_env.Supercede)
					{
						if(!strncmp(PM->MaterialName,"env_", 4))
						{
							pTexVerts->u += ( (G->NormalArray[ *List ]).X * P->internal_env.PercentMaterial);
							pTexVerts->v += ( (G->NormalArray[ *List ]).Y * P->internal_env.PercentMaterial);
						}
						else
						{
							pTexVerts->u = SVert->SVU;
							pTexVerts->v = SVert->SVV;
						}
					}
					else
					{
						pTexVerts->u += ( (G->NormalArray[ *List ]).X * P->internal_env.PercentEnvironment);
						pTexVerts->v += ( (G->NormalArray[ *List ]).Y * P->internal_env.PercentEnvironment);
					}
				}

// changed QD bug fix
/*	03/15/2004 Wendell Buckner
    SPHEREMAPPING */
			//	nIndex = *List;
				gePuppet_StaticLightGrp.SurfaceNormal = (G->NormalArray[ *List ]);
									
			//	List++;

				//gePuppet_SetVertexColor2(P,PM,&Ambient,SurfaceNormal, Lights,LightCount, pTexVerts);
				gePuppet_SetVertexColor(&lvert, SVert->ReferenceBoneIndex);

/*	01/08/2004 Wendell Buckner
    DOT3 BUMPMAPPING */
				if(geBitmap_IsBumpmapNameDot3(PM->MaterialName)) 
					gePuppet_SetVertexColorDot3(&lvert, vIndex, XFA, P->BodyInstance, 0, &ResetDot3);

				pTexVerts->r = lvert.r;
				pTexVerts->g = lvert.g;
				pTexVerts->b = lvert.b;
				pTexVerts->a = P->OverallAlpha;
/*	03/15/2004 Wendell Buckner
    SPHEREMAPPING */
				if( geBitmap_IsSphereMapName ( PM->MaterialName ) ) 
				{
					gePuppet_SetSphereMapUV ( &lvert, &G->NormalArray[ *List ], Camera );
					pTexVerts->u = lvert.u;
					pTexVerts->v = lvert.v;
				}

				List++;
// end change	
			}
			
			pDest1 = Verts;
			pDest2 = Dest2;
			pTex1 = TexVerts;
			pTex2 = Tex2;

			FPlanes = FInfo->Planes;

			for (p=0; p< FInfo->NumPlanes; p++, FPlanes++)
			{
				if (!(ClipFlags & (1<<p)))
					continue;		// No need to clip...

				assert(Length1 < MAX_TEMP_VERTS);

				if (!Frustum_ClipToPlaneUVRGBA(FPlanes, pDest1, pDest2, pTex1, pTex2, Length1, &Length2))
					break;

				assert(Length2 < MAX_TEMP_VERTS);
				
				if (pDest1 == Dest2)
				{
					pDest1 = Dest1;
					pDest2 = Dest2;
					pTex1 = Tex1;
					pTex2 = Tex2;
				}
				else
				{
					pDest1 = Dest2;
					pDest2 = Dest1;
					pTex1 = Tex2;
					pTex2 = Tex1;
				}
				Length1 = Length2;
			}
			
			assert(Length1 < MAX_TEMP_VERTS);
	  
			if (p != FInfo->NumPlanes)
				continue;				// Can't possibly be visible

			if (Length1 < 3)
				continue;				// Can't possibly be visible
			
// changed QD Clipping
// is already in cameraspace
			//for (v=0; v<Length1; v++)
			//	geCamera_Transform(Camera, &pDest1[v], &pDest2[v]);

			// Project the face, and combine tex coords into one structure (Clipped1)
			//Frustum_ProjectRGBA(pDest2, pTex1, (DRV_TLVertex*)ScreenPts, Length1, Camera);
			Frustum_ProjectRGBA(pDest1, pTex1, (DRV_TLVertex*)ScreenPts, Length1, Camera);
// end change
			// LWM_ACTOR_RENDERING
			#if 1
			ScreenPts[0].a = P->OverallAlpha ;
			#else
			ScreenPts[0].a = 255.0f;
			#endif

/* 03/24/2004 Wendell Buckner
    BUG FIX: Rendering Transparent Polys properly (2) *
             geEngine_RenderPoly(Engine, (GE_TLVertex*)ScreenPts, Length1, PM->Bitmap, 0 );*/
			if ( MaterialIsTransparent[Material] )
/* 08/10/2004 Wendell Buckner
    BUG FIX: Dumb copy/paste bug! 
				gePuppet_AddToGList ( (GE_TLVertex *)v, 3, PM->Bitmap, 0, GE_FALSE ); */
				gePuppet_AddToGList ( (GE_TLVertex *)ScreenPts, Length1, PM->Bitmap, 0, GE_FALSE );
			else
				geEngine_RenderPoly(Engine, (GE_TLVertex*)ScreenPts, Length1, PM->Bitmap, 0 );
		}
	}

	#pragma message ("BUG:  Shadow caused crash in mirrors (oops).  Need to write a RenderShadowThroughFrustum...")
	/*
	if (P->DoShadow)
		{
			gePuppet_DrawShadow(P,Engine,World,Camera);
		}
	*/

/* 03/24/2004 Wendell Buckner
    BUG FIX: Rendering Transparent Polys properly (2) */
	gePuppet_AddToGList ( NULL, 0, NULL, 0, GE_TRUE ); 
	geRam_Free ( MaterialArray );
    geRam_Free ( MaterialIsTransparent );

	return GE_TRUE;
}

#ifdef PROFILE
#define PUPPET_AVERAGE_ACROSS 60
double Puppet_AverageCount[PUPPET_AVERAGE_ACROSS]={
	0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,
	0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,
	0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,
	0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,
	0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,
	0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0};
int Puppet_AverageIndex = 0;
#endif


geBoolean GENESISCC gePuppet_Render(	const gePuppet *P, 
							const gePose *Joints,
							geEngine *Engine, 
							geWorld *World, 
							const geCamera *Camera, 
							geExtBox *TestBox, 
							Frustum_Info *FInfo)
{
	const geXFArray *JointTransforms;
	geVec3d Scale;
	#ifdef PROFILE
	rdtsc_timer_type RDTSCStart,RDTSCEnd;
	#endif
	geRect ClippingRect;
	geBoolean Clipping = GE_TRUE;
// changed QD Clipping
	int32 ClipFlags;

	//char name[128];
	int i;//, j;
	//geBoolean flag;

	#define BACK_EDGE (1.0f)

//	const geBodyInst_Geometry *G;
// end change

/*	01/08/2004 Wendell Buckner
    DOT3 BUMPMAPPING */
	int BoneCount;
	const geXForm3d *XFA;
	geBoolean ResetDot3 = GE_TRUE;

/* 03/24/2004 Wendell Buckner
    BUG FIX: Rendering Transparent Polys properly (2) */
	geBoolean *MaterialIsTransparent = NULL;
	geBitmap **MaterialArray = NULL;
	int32    MaterialCount = 0;

// changed QD 
	((gePuppet*)P)->BodyG = NULL;
// changed QD Performance
	if(P->OverallAlpha==0.0f)
		return GE_TRUE;
// end change

	JointTransforms = gePose_GetAllJointTransforms(Joints);
	XFA = geXFArray_GetElements(JointTransforms, &BoneCount);

	assert( P      );
	assert( Engine );
	assert( World  );
	assert( Camera );

	#ifdef PROFILE
	rdtsc_read(&RDTSCStart);
    rdtsc_zero(&RDTSCEnd);
	#endif

// changed QD Clipping
/*	flag = GE_FALSE;
	j = Engine->DriverInfo.NumSubDrivers;
	for(i=0;i<j;i++)
	{
		strcpy(name, Engine->DriverInfo.SubDrivers[i].Name);
		if(name[0]=='G') // Glide driver
		{
			flag = GE_TRUE;
			break;
		}
	}*/
// end change
	
	geCamera_GetClippingRect(Camera,&ClippingRect);
	
	if (TestBox != NULL)
	{
		// see if the test box is visible on the screen.  If not: don't draw actor.
		// (transform and project it to the screen, then check extents of that projection
		//  against the clipping rect)
		geVec3d BoxCorners[8];
		const geXForm3d *ObjectToCamera;
		geVec3d Maxs,Mins;
		#define BIG_NUMBER (99e9f)  
		int i;
		geBoolean ZFarEnable;
		geFloat ZFar;

		BoxCorners[0] = TestBox->Min;
		BoxCorners[1] = BoxCorners[0];  BoxCorners[1].X = TestBox->Max.X;
		BoxCorners[2] = BoxCorners[0];  BoxCorners[2].Y = TestBox->Max.Y;
		BoxCorners[3] = BoxCorners[0];  BoxCorners[3].Z = TestBox->Max.Z;
		BoxCorners[4] = TestBox->Max;
		BoxCorners[5] = BoxCorners[4];  BoxCorners[5].X = TestBox->Min.X;
		BoxCorners[6] = BoxCorners[4];  BoxCorners[6].Y = TestBox->Min.Y;
		BoxCorners[7] = BoxCorners[4];  BoxCorners[7].Z = TestBox->Min.Z;

		ObjectToCamera = geCamera_GetCameraSpaceXForm(Camera);
		assert( ObjectToCamera );

		geVec3d_Set(&Maxs,-BIG_NUMBER,-BIG_NUMBER,-BIG_NUMBER);
		geVec3d_Set(&Mins, BIG_NUMBER, BIG_NUMBER, BIG_NUMBER);
		for (i=0; i<8; i++)
		{
			geVec3d V;
			geXForm3d_Transform(  ObjectToCamera,&(BoxCorners[i]),&(BoxCorners[i]));
			geCamera_Project( Camera,&(BoxCorners[i]),&V);
			if (V.X > Maxs.X ) Maxs.X = V.X;
			if (V.X < Mins.X ) Mins.X = V.X;
			if (V.Y > Maxs.Y ) Maxs.Y = V.Y;
			if (V.Y < Mins.Y ) Mins.Y = V.Y;
			if (V.Z > Maxs.Z ) Maxs.Z = V.Z;
			if (V.Z < Mins.Z ) Mins.Z = V.Z;
		}
		
		// Reject against ZFar clipplane if enabled...
		geCamera_GetFarClipPlane(Camera, &ZFarEnable, &ZFar);
		if (ZFarEnable) 
		{
			if ( (Maxs.X < ClippingRect.Left) 
				|| (Mins.X > ClippingRect.Right)
				|| (Maxs.Z < BACK_EDGE) 				//Test X and Z first, and Y
				|| (Mins.Z > ZFar)
				|| (Maxs.Y < ClippingRect.Top) 
				|| (Mins.Y > ClippingRect.Bottom))
			{
				// not gonna draw: box is not visible.
				// changed QD Shadows
				// actor is not visible on screen but may cast a shadow that is visible
				((gePuppet*)P)->UpdateBodyG = GE_TRUE;
				// end change
				return GE_TRUE;
			}
			
		} 
		else 
		{
			if ( (Maxs.X < ClippingRect.Left) 
				|| (Mins.X > ClippingRect.Right)
				|| (Maxs.Y < ClippingRect.Top) 
				|| (Mins.Y > ClippingRect.Bottom)
				|| (Maxs.Z < BACK_EDGE))
			{
				// not gonna draw: box is not visible.
				// changed QD Shadows
				// actor is not visible on screen but may cast a shadow that is visible
				((gePuppet*)P)->UpdateBodyG = GE_TRUE;
				// end change
				return GE_TRUE;
			}
		} 
	} 

	Engine->DebugInfo.NumActors++;
// changed QD Clipping
// geTClip not used anymore...
/*	geTClip_SetupEdges(Engine,
						(geFloat)ClippingRect.Left,
						(geFloat)ClippingRect.Right,
						(geFloat)ClippingRect.Top,
						(geFloat)ClippingRect.Bottom,
						BACK_EDGE);
*/	
// end change
	JointTransforms = gePose_GetAllJointTransforms(Joints);

	#pragma message ("Level of detail hacked:")
	gePose_GetScale(Joints,&Scale);

// changed QD Shadows
	//G = geBodyInst_GetGeometry(P->BodyInstance, &Scale, JointTransforms, 0,Camera);
	((gePuppet*)P)->BodyG = geBodyInst_GetGeometry(P->BodyInstance, &Scale, JointTransforms, 0, Camera);
	
	//if(G == NULL)
	if(P->BodyG == NULL)
	{
		geErrorLog_Add(ERR_PUPPET_RENDER, NULL);
		return GE_FALSE;
	}

	((gePuppet*)P)->UpdateBodyG = GE_FALSE;
// end change

// changed QD Clipping
/*
#ifdef ONE_OVER_Z_PIPELINE
#define TEST_Z_OUT(zzz, edge) 		((zzz) > (edge)) 
#define TEST_Z_IN(zzz, edge) 		((zzz) < (edge)) 
#pragma message ("test this! this is untested")
#else
#define TEST_Z_OUT(zzz, edge) 		((zzz) < (edge)) 
#define TEST_Z_IN(zzz, edge) 		((zzz) > (edge)) 
#endif


	// check for trivial rejection:
	{
		if( (G->Maxs.X < ClippingRect.Left) 
			|| (G->Mins.X > ClippingRect.Right)
			|| ( TEST_Z_OUT( G->Maxs.Z, BACK_EDGE) )		//test Y last
			|| (G->Maxs.Y < ClippingRect.Top) 
			|| (G->Mins.Y > ClippingRect.Bottom) )
		{
			// not gonna draw
			return GE_TRUE;
		}

		if (   (G->Maxs.X < ClippingRect.Right) 
			&& (G->Mins.X > ClippingRect.Left)
			&& (G->Maxs.Y < ClippingRect.Bottom) 
			&& (G->Mins.Y > ClippingRect.Top)
			&& ( TEST_Z_IN( G->Mins.Z, BACK_EDGE) ) )
		{
			// not gonna clip
			Clipping = GE_FALSE;
		}
		else
		{
			Clipping = GE_TRUE;
		} 
	}
*/
	// Setup clip flags...
	ClipFlags = 0xffff;

	{		
		GFX_Plane	*Planes;
		int32		k;
		
		
		Planes = FInfo->Planes;

		for (k=0; k< FInfo->NumPlanes; k++, Planes++)
		{
			int32		Side;
			
			Planes->Type = PLANE_ANY;
		
			//Side = Trace_BoxOnPlaneSide(&(G->Mins), &(G->Maxs), Planes);
			Side = Trace_BoxOnPlaneSide(&(P->BodyG->Mins), &(P->BodyG->Maxs), Planes);

			if (Side == PSIDE_BACK)
			{
				return GE_TRUE;
			}
			
			if (Side == PSIDE_FRONT)
			{
				ClipFlags &= ~(1<<k);
			}
		}
	}
// end change

/* 03/24/2004 Wendell Buckner
    BUG FIX: Rendering Transparent Polys properly (2) */
	MaterialCount = P->MaterialCount;
	MaterialArray = GE_RAM_ALLOCATE_ARRAY ( geBitmap *, MaterialCount );
    MaterialIsTransparent = GE_RAM_ALLOCATE_ARRAY ( geBoolean, MaterialCount );

	for ( i = 0; i < MaterialCount; i++ )
	{
		gePuppet_Material *PM = &(P->MaterialArray[i]);				 
		MaterialArray[i] = PM->Bitmap;
	}

	gePuppet_IsTransparent ( P->OverallAlpha, MaterialArray, MaterialIsTransparent, MaterialCount );

// changed QD Clipping
//	geTClip_SetOverallAlpha ( P->OverallAlpha );

	{
		//GE_LVertex v[3];
		int i,j,Count;
		geBodyInst_Index *List;
		//geBodyInst_Index Command;
		geXForm3d RootTransform;
		//gePuppet_Material *PM;
		//geBodyInst_Index Material,LastMaterial;

// end change

		gePuppet_StaticLightGrp.UseFillLight		 = P->UseFillLight;
		gePuppet_StaticLightGrp.FillLightNormal		 = P->FillLightNormal;
		gePuppet_StaticLightGrp.FillLightColor.Red	 = P->FillLightColor.Red;
		gePuppet_StaticLightGrp.FillLightColor.Green = P->FillLightColor.Green;
		gePuppet_StaticLightGrp.FillLightColor.Blue  = P->FillLightColor.Blue;
		gePuppet_StaticLightGrp.PerBoneLighting		 = P->PerBoneLighting;
		
		gePose_GetJointTransform(Joints,P->LightReferenceBoneIndex,&(RootTransform));

		if (P->MaxDynamicLightsToUse > 0)
		{
			if (P->PerBoneLighting)
			{
				int BoneCount;
				const geXForm3d *XFA = geXFArray_GetElements(JointTransforms, &BoneCount);
				if (BoneCount>0)
				{
					if (gePuppet_StaticBoneLightArraySize < BoneCount)
					{
						gePuppet_BoneLight *LG;
							
						LG = geRam_Realloc(gePuppet_StaticBoneLightArray, sizeof(gePuppet_BoneLight) * BoneCount);
						if (LG==NULL)
						{
							geErrorLog_Add(ERR_PUPPET_RENDER,"Failed to allocate space for bone lighting info cache");
							return GE_FALSE;
						}
						gePuppet_StaticBoneLightArray = LG;
						gePuppet_StaticBoneLightArraySize = BoneCount;
					}
					for (i=0; i<BoneCount; i++)
					{
						gePuppet_StaticBoneLightArray[i].LightCount = gePuppet_PrepLights(P,World,
									gePuppet_StaticBoneLightArray[i].Lights,&(XFA[i].Translation));
					}
				}
			}
			else
			{
				gePuppet_StaticLightGrp.LightCount = gePuppet_PrepLights(P,World
							,gePuppet_StaticLightGrp.Lights,&(RootTransform.Translation));
			}
		}
		else
		{
			gePuppet_StaticLightGrp.LightCount = 0;
		}

		
   //XING Studios code

/*  03/10/2004 Wendell Buckner                                                            
     Ambient light is leeking into objects that should not receive light, each object     
     will have it's own ambient property so ONLY those objects that should receive,       
     do receive ambient light.                                                          
		gePuppet_StaticLightGrp.StaticLightCount = gePuppet_ComputeAmbientLight(P,World,&(gePuppet_StaticLightGrp.Ambient),gePuppet_StaticLightGrp.StaticLights ,&(RootTransform.Translation)); */
		gePuppet_StaticLightGrp.StaticLightCount = gePuppet_ComputeAmbientLight(P,World,(gePuppet_Color *)&(P->Ambient),gePuppet_StaticLightGrp.StaticLights ,&(RootTransform.Translation));

// changed QD Shadows
   		//Count = G->FaceCount;
		//List  = G->FaceList;
		Count = P->BodyG->FaceCount;
		List  = P->BodyG->FaceList;
// changed QD Clipping
/*		#if 1
		v[0].a = v[1].a= v[2].a = P->OverallAlpha ;
		#else
		v[0].a = v[1].a= v[2].a = 255.0f;
		#endif
*/

		//LastMaterial = -1;

		for (i=0; i<Count; i++)
		{
// changed QD Clipping
			geBodyInst_SkinVertex *SV;
			int32				Length1, Length2, p;
			geVec3d				Dest1[MAX_TEMP_VERTS], *pDest1;
			geVec3d				Dest2[MAX_TEMP_VERTS], *pDest2;
			geVec3d				Verts[MAX_TEMP_VERTS], *pVerts, v1, v2, v3;
			Surf_TexVert		TexVerts[MAX_TEMP_VERTS], Tex1[MAX_TEMP_VERTS], *pTexVerts;
			Surf_TexVert		Tex2[MAX_TEMP_VERTS], *pTex1, *pTex2;			
			Surf_TLVertex		ScreenPts[MAX_TEMP_VERTS];
			GFX_Plane			*FPlanes;
			geBodyInst_Index	Command, Material;
			gePuppet_Material	*PM;
// end change
			
			Command = *List;
			List ++;
			Material = *List;
			List ++;

			assert( Command == GE_BODYINST_FACE_TRIANGLE );
			assert( Material>=0 );
			assert( Material<P->MaterialCount);

// changed QD Clipping			
			// facing the camera?
			{
				geBodyInst_Index *List2;
				
				List2 = List;
		
				//geVec3d_Copy(&(G->SkinVertexArray[*List2].SVPoint), &Verts[0]);
				geVec3d_Copy(&(P->BodyG->SkinVertexArray[*List2].SVPoint), &Verts[0]);
				List2++;
				List2++;
				//geVec3d_Copy(&(G->SkinVertexArray[*List2].SVPoint), &Verts[1]);
				geVec3d_Copy(&(P->BodyG->SkinVertexArray[*List2].SVPoint), &Verts[1]);
				List2++;
				List2++;

				//geVec3d_Subtract(&(G->SkinVertexArray[*List2].SVPoint), &Verts[1], &v1);
				geVec3d_Subtract(&(P->BodyG->SkinVertexArray[*List2].SVPoint), &Verts[1], &v1);
				List2++;
				List2++;
				//geVec3d_Subtract(&Verts[2], &Verts[1], &v1);
				geVec3d_Subtract(&Verts[0], &Verts[1], &v2);
				geVec3d_CrossProduct(&v1, &v2, &v3);
				geVec3d_Normalize(&v3);

				if(geVec3d_DotProduct(&v3, &Verts[0]) >= 0.0f)
				{
					List = List2;
					continue;
				}
			}

// changed QD
// LastMaterial is always -1
		//	if ( Material != LastMaterial )
			{
				PM = &(P->MaterialArray[Material]);
// changed QD Clipping
			//	geTClip_SetTexture(PM->Bitmap);
				gePuppet_StaticLightGrp.MaterialColor = PM->Color;
			}

			pVerts = Verts;
			pTexVerts = TexVerts;


			//for(j=0; j<3; j++)
			for(j=0; j<3; j++, pVerts++, pTexVerts++)
			{
								
				GE_LVertex lvert;
// end change
/*	01/08/2004 Wende
ll Buckner
    DOT3 BUMPMAPPING */
				geBodyInst_Index vIndex = *List;

/*	03/15/2004 Wendell Buckner
    SPHEREMAPPING */
			//	geBodyInst_Index nIndex;

				//SV = &(G->SkinVertexArray[ *List ]);
				SV = &(P->BodyG->SkinVertexArray[ *List ]);
				List++;

				*pVerts = SV->SVPoint;

// changed QD
				//Environment mapping code...
				pTexVerts->u = SV->SVU;
				pTexVerts->v = SV->SVV;

				if( P->internal_env.UseEnvironmentMapping )
				{					
					pTexVerts->u *= (P->internal_env.PercentPuppet);//= SV->SVU*P->internal_env.PercentPuppet;
					pTexVerts->v *= (P->internal_env.PercentPuppet);//= SV->SVV*P->internal_env.PercentPuppet;
					
					if( P->internal_env.Supercede)
					{
						if(!strncmp(PM->MaterialName,"env_", 4))
						{
							//pTexVerts->u += ( (G->NormalArray[*List]).X * P->internal_env.PercentMaterial);
							//pTexVerts->v += ( (G->NormalArray[*List]).Y * P->internal_env.PercentMaterial);
							pTexVerts->u += ( (P->BodyG->NormalArray[*List]).X * P->internal_env.PercentMaterial);
							pTexVerts->v += ( (P->BodyG->NormalArray[*List]).Y * P->internal_env.PercentMaterial);
						}
						else
						{
							pTexVerts->u = SV->SVU;
							pTexVerts->v = SV->SVV;
						}
					}
					else
					{
						//pTexVerts->u += ( (G->NormalArray[ *List ]).X * P->internal_env.PercentEnvironment);
						//pTexVerts->v += ( (G->NormalArray[ *List ]).Y * P->internal_env.PercentEnvironment);
						pTexVerts->u += ( (P->BodyG->NormalArray[ *List ]).X * P->internal_env.PercentEnvironment);
						pTexVerts->v += ( (P->BodyG->NormalArray[ *List ]).Y * P->internal_env.PercentEnvironment);
					}
				}
			
/*	03/15/2004 Wendell Buckner
    SPHEREMAPPING */
			//	nIndex = *List;
				//gePuppet_StaticLightGrp.SurfaceNormal = (G->NormalArray[ *List ]);
				gePuppet_StaticLightGrp.SurfaceNormal = (P->BodyG->NormalArray[ *List ]);
							
			//	List++;

				//gePuppet_SetVertexColor2(P,PM,&Ambient,SurfaceNormal, Lights,LightCount, pTexVerts);
				gePuppet_SetVertexColor(&lvert,SV->ReferenceBoneIndex);

/*	01/08/2004 Wendell Buckner
    DOT3 BUMPMAPPING */
				if(geBitmap_IsBumpmapNameDot3(PM->MaterialName)) 
					gePuppet_SetVertexColorDot3(&lvert, vIndex, XFA, P->BodyInstance, 0, &ResetDot3);

				pTexVerts->r = lvert.r;
				pTexVerts->g = lvert.g;
				pTexVerts->b = lvert.b;
				pTexVerts->a = P->OverallAlpha;

/*	03/15/2004 Wendell Buckner
    SPHEREMAPPING */
				if( geBitmap_IsSphereMapName( PM->MaterialName ) ) 
				{
					//gePuppet_SetSphereMapUV(&lvert, &G->NormalArray[*List], Camera);
					gePuppet_SetSphereMapUV(&lvert, &(P->BodyG->NormalArray[*List]), Camera);
					pTexVerts->u = lvert.u;
					pTexVerts->v = lvert.v;
				}

				List++;
			}

			pDest1 = Verts;
			pDest2 = Dest2;
			pTex1 = TexVerts;
			pTex2 = Tex2;
			Length1 = 3;

			FPlanes = FInfo->Planes;

			for (p=0; p< FInfo->NumPlanes; p++, FPlanes++)
			{
				if (!(ClipFlags & (1<<p)))
					continue;		// No need to clip...

				assert(Length1 < MAX_TEMP_VERTS);

				if (!Frustum_ClipToPlaneUVRGBA(FPlanes, pDest1, pDest2, pTex1, pTex2, Length1, &Length2))
					break;

				assert(Length2 < MAX_TEMP_VERTS);
				
				if (pDest1 == Dest2)
				{
					pDest1 = Dest1;
					pDest2 = Dest2;
					pTex1 = Tex1;
					pTex2 = Tex2;
				}
				else
				{
					pDest1 = Dest2;
					pDest2 = Dest1;
					pTex1 = Tex2;
					pTex2 = Tex1;
				}
				Length1 = Length2;
			}
			
			assert(Length1 < MAX_TEMP_VERTS);
	  
			if (p != FInfo->NumPlanes)
				continue;				// Can't possibly be visible

			if (Length1 < 3)
				continue;				// Can't possibly be visible
						
			// Project the face, and combine tex coords into one structure 
			Frustum_ProjectRGBA(pDest1, pTex1, (DRV_TLVertex*)ScreenPts, Length1, Camera);

			ScreenPts[0].a = P->OverallAlpha ;

/* 03/24/2004 Wendell Buckner
    BUG FIX: Rendering Transparent Polys properly (2) *
             geEngine_RenderPoly(Engine, (GE_TLVertex*)ScreenPts, Length1, PM->Bitmap, 0 );*/
			if ( MaterialIsTransparent[Material] )
/* 08/10/2004 Wendell Buckner*/
				gePuppet_AddToGList ( (GE_TLVertex *)ScreenPts, Length1, PM->Bitmap, 0, GE_FALSE );
			else
				geEngine_RenderPoly(Engine, (GE_TLVertex*)ScreenPts, Length1, PM->Bitmap, 0 );
// end change
		}
		//assert( ((uint32)List) - ((uint32)G->FaceList) == (uint32)(G->FaceListSize) );
		assert( ((uint32)List) - ((uint32)P->BodyG->FaceList) == (uint32)(P->BodyG->FaceListSize) );
	}

	if (P->DoShadow)
	{
		gePuppet_DrawShadow(P,Joints,Engine,World,Camera);
	}


	#ifdef PROFILE
	{
		double Count=0.0;
		int i;

		rdtsc_read(&RDTSCEnd);
		rdtsc_delta(&RDTSCStart,&RDTSCEnd,&RDTSCEnd);
		geEngine_Printf(Engine, 320,10,"Puppet Render Time=%f",(double)(rdtsc_cycles(&RDTSCEnd)/200000000.0));
		geEngine_Printf(Engine, 320,30,"Puppet Render Cycles=%f",(double)(rdtsc_cycles(&RDTSCEnd)));
		#if 1
		Puppet_AverageCount[(Puppet_AverageIndex++)%PUPPET_AVERAGE_ACROSS] = rdtsc_cycles(&RDTSCEnd);
		for (i=0; i<PUPPET_AVERAGE_ACROSS; i++)
			{	
				Count+=Puppet_AverageCount[i];
			}
		Count /= (double)PUPPET_AVERAGE_ACROSS;

		geEngine_Printf(Engine, 320,60,"Puppet AVG Render Time=%f",(double)(Count/200000000.0));
		geEngine_Printf(Engine, 320,90,"Puppet AVG Render Cycles=%f",(double)(Count));
		#endif
				
	}
	#endif

/* 03/24/2004 Wendell Buckner
    BUG FIX: Rendering Transparent Polys properly (2) */
	gePuppet_AddToGList ( NULL, 0, NULL, 0, GE_TRUE ); 
	geRam_Free ( MaterialArray );
    geRam_Free ( MaterialIsTransparent );

	return GE_TRUE;
}

// changed QD Shadows
static void GENESISCC AddEdge(geBodyInst_Index *pEdges, int *NumEdges, 
							  geBodyInst_Index v0, geBodyInst_Index v1, const geBodyInst_Geometry *G)
{
	int i;
    // Remove interior edges (which appear in the list twice)
    for(i=0;i<*NumEdges;i++)
    {
        if((pEdges[2*i]==v1) && (pEdges[2*i+1]==v0))
		{
            if(*NumEdges > 1)
            {
                pEdges[2*i] = pEdges[2*(*NumEdges-1)];
                pEdges[2*i+1] = pEdges[2*(*NumEdges-1)+1];
            }
            (*NumEdges)--;
            return;
        }
    }

    pEdges[2*(*NumEdges)] = v0;
    pEdges[2*(*NumEdges)+1] = v1;
    (*NumEdges)++;
}

geBoolean GENESISCC gePuppet_RenderShadowVolume(gePuppet *P,
					const gePose *Joints,
					geEngine *Engine, 
					geWorld *World,
					const geCamera *Camera,
					GFX_Plane* FPlanes,
					geVec3d *LightPos,
					geFloat Radius,
					int LightType,
					geVec3d* Dir, 
					geFloat Arc,
					geBoolean ZPass)
{	
	geXForm3d	RootTransform;
	geVec3d		Dest1[30], Dest2[30], *pDest1, *pDest2, Pos1;
	int32		Length1, Length2;
	GFX_Plane	*pFPlanes;
	int32		p;
	geFloat		SVLength;
	geVec3d		LPos;
	
	assert(P);
	assert(World);
	assert(Camera);
	assert(Joints);

	if(!P->DoStencilShadow)
		return GE_TRUE;

	if(P->OverallAlpha==0.0f)
		return GE_TRUE;

	gePose_GetJointTransform(Joints,P->ShadowBoneIndex,&RootTransform);

	SVLength = 0.0f;

	Pos1 = RootTransform.Translation;
	geVec3d_Subtract(LightPos, &Pos1, &Pos1);

	if(geVec3d_LengthSquared(&Pos1) > (Radius*Radius))
		return GE_TRUE;
	else
	{
		if(LightType == 1)
		{
			geFloat Angle = -geVec3d_DotProduct(&Pos1, Dir);
			if(Angle < Arc)
				return GE_TRUE;
		}

		{
			if(Trace_WorldCollisionExact3((geWorld*)World, &(RootTransform.Translation), LightPos, NULL, NULL, NULL, NULL))
				return GE_TRUE;
		}

		SVLength = Radius - geVec3d_Length(&Pos1);
	}	

	pFPlanes = FPlanes;

	geCamera_Transform(Camera, LightPos, &LPos);

	{
		int i, j, Count;
		geBodyInst_Index *List;
		geBodyInst_Index Command;
		geVec3d	Scale;
		int NumEdges;
		geBodyInst_Index *pEdges;


		gePose_GetScale(Joints, &Scale);		

		if(P->BodyG == NULL || P->UpdateBodyG)
		{
			P->BodyG = geBodyInst_GetGeometry(P->BodyInstance,&Scale,gePose_GetAllJointTransforms(Joints),0,Camera);
			P->UpdateBodyG = GE_FALSE;
		}
			
		if(P->BodyG == NULL)
		{
			geErrorLog_Add(ERR_PUPPET_RENDER, NULL);
			return GE_FALSE;
		}

		Count = P->BodyG->FaceCount;
		List  = P->BodyG->FaceList;
		
		NumEdges = 0;
		pEdges = GE_RAM_ALLOCATE_ARRAY(geBodyInst_Index, Count*6);
		
		if ( pEdges == NULL )
		{
			geErrorLog_Add(ERR_BODY_ENOMEM, NULL);
			pEdges = NULL;
			return GE_FALSE;
		}

// determine lit faces 
		for (i=0; i<Count; i++)
		{	

			Command = *List;
			List++;
			//Material = *List;
			List++;
			//assert( Command == GE_BODY_FACE_TRIANGLE );

			{
				geVec3d *vert[3];
				geVec3d help[3];
				geVec3d cross1;
				geVec3d cross2;
				geVec3d FaceNormal;
				geFloat pDist;
								
				geBodyInst_Index *List2;
						
				List2 = List;

				for(j=0;j<3;j++)
				{				
					vert[j]=&(P->BodyG->SkinVertexArray[ *List2 ].SVPoint);
					List2++;
					List2++;
				}
			
				geVec3d_Subtract(vert[1], vert[0], &cross1);
				geVec3d_Subtract(vert[2], vert[0], &cross2);
				
				geVec3d_CrossProduct(&cross1, &cross2, &FaceNormal);
								
				geVec3d_Normalize(&FaceNormal);
				pDist = geVec3d_DotProduct(vert[0], &FaceNormal);

				// using the same triangles for front and back cap saves computing
				// the direction for extruding for 1 cap
				// using the unlit faces for the caps gives better results...
				if((geVec3d_DotProduct(&LPos, &FaceNormal)) < pDist)
        		{	
					geBodyInst_Index Index1, Index2, Index3;
								
					Index1 = *List;
					List++;	
					List++;
					Index2 = *List;
					List++;	
					List++;
					Index3 = *List;

					AddEdge(pEdges, &NumEdges, Index1, Index2, P->BodyG);
					AddEdge(pEdges, &NumEdges, Index2, Index3, P->BodyG);
					AddEdge(pEdges, &NumEdges, Index3, Index1, P->BodyG);

					
					{
						// prepare back cap
						// need to render back cap even if using zpass method, since the shadow volume is 
						// not extruded to infinity (or far enough to be offscreen)
						for(j=0;j<3;j++)
						{		
							geVec3d_Subtract(vert[j], &LPos, &help[j]);							
							geVec3d_Normalize(&help[j]);

							geVec3d_AddScaled(vert[j], &help[j], SVLength, &Dest1[j]);
					
						}					
				
						pDest1 = Dest1;
						pDest2 = Dest2;
						Length1 = 3;

						for (p=0; p<5; p++)
						{
							if(Frustum_ClipToPlane(&pFPlanes[p], pDest1, pDest2, Length1, &Length2)==GE_FALSE)
								break;
			
							if (pDest1 == Dest2)
							{
								pDest1 = Dest1;
								pDest2 = Dest2;
							}
							else
							{
								pDest1 = Dest2;
								pDest2 = Dest1;
							}
							Length1 = Length2;
						}

						if(Length2>2)
						{
							for (j=0; j< Length1; j++)
							{
								geCamera_Project(Camera, &pDest1[j], &pDest1[j]);
							}
								
							geEngine_RenderPolyStencil(Engine, pDest1, Length1, 0);
						}
	
						if(!ZPass) // need front cap for zfail method
						{
							// prepare front cap
							// bah!!! have to shift 0.2f to avoid z-fighting
							for(j=0;j<3;j++)
							{						
								geVec3d_AddScaled(vert[j], &help[j], -0.2f, &Dest1[j]);
							}

							//reverse vertex order for front cap;
							geVec3d_Copy(&Dest1[0], &help[0]);
							geVec3d_Copy(&Dest1[2], &Dest1[0]);
							geVec3d_Copy(&help[0], &Dest1[2]);

							pDest1 = Dest1;
							pDest2 = Dest2;
							Length1 = 3;

							for (p=0; p<5; p++)
							{
								if(Frustum_ClipToPlane(&pFPlanes[p], pDest1, pDest2, Length1, &Length2)==GE_FALSE)
									break;
			
								if (pDest1 == Dest2)
								{
									pDest1 = Dest1;
									pDest2 = Dest2;
								}
								else
								{
									pDest1 = Dest2;
									pDest2 = Dest1;
								}
								Length1 = Length2;
							}

							if(Length2>2)
							{
								for (j=0; j< Length1; j++)
								{
									geCamera_Project(Camera, &pDest1[j], &pDest1[j]);
								}	
				
								geEngine_RenderPolyStencil(Engine, pDest1, Length1, 0);
							}
						}
					}
				}
				
				List = List2;	
				
			}
		}

// extrude and render the silhouette edges
		for(i=0;i<NumEdges;i++)
		{				
			geVec3d help;
			
			// point1
			geVec3d_Copy(&(P->BodyG->SkinVertexArray[pEdges[2*i+1]].SVPoint),&Dest1[0]);
		
			geVec3d_Subtract(&Dest1[0], &LPos, &help);
			geVec3d_Normalize(&help);

			geVec3d_AddScaled(&Dest1[0], &help, -0.2f, &Dest1[0]);
	
			// point2
			geVec3d_AddScaled(&(P->BodyG->SkinVertexArray[pEdges[2*i+1]].SVPoint), &help, SVLength, &Dest1[1]);
					
			// point3
			geVec3d_Copy(&(P->BodyG->SkinVertexArray[pEdges[2*i]].SVPoint),&Dest1[2]);
		
			geVec3d_Subtract( &Dest1[2], &LPos, &help);
			geVec3d_Normalize(&help);

			geVec3d_AddScaled(&Dest1[2], &help, SVLength, &Dest1[2]);

			// point4
			geVec3d_AddScaled(&(P->BodyG->SkinVertexArray[pEdges[2*i]].SVPoint), &help, -0.2f, &Dest1[3]);

			
			pDest1 = Dest1;
			pDest2 = Dest2;
			Length1 = 4; //quad

			for (p=0; p<5; p++)
			{
				if(Frustum_ClipToPlane(&pFPlanes[p], pDest1, pDest2, Length1, &Length2)==GE_FALSE)
					break;
			
				if (pDest1 == Dest2)
				{
					pDest1 = Dest1;
					pDest2 = Dest2;
				}
				else
				{
					pDest1 = Dest2;
					pDest2 = Dest1;
				}
				Length1 = Length2;
			}

			if(Length2<3)
				continue;

			for (j=0; j< Length1; j++)
			{
				geCamera_Project(Camera, &pDest1[j], &pDest1[j]);
			}
			
			geEngine_RenderPolyStencil(Engine, pDest1, Length1, 0 );
		}

		if(pEdges!=NULL)
			geRam_Free(pEdges);
		assert( ((uint32)List) - ((uint32)P->BodyG->FaceList) == (uint32)(P->BodyG->FaceListSize) );

	}

	return GE_TRUE;
}

void GENESISCC gePuppet_BodyGeometryNeedsUpdate(gePuppet *P)
{
	if(P)
		P->UpdateBodyG = GE_TRUE;
}

void GENESISCC gePuppet_SetStencilShadow(gePuppet *P, geBoolean DoStencilShadow)
{
	assert( P );
	assert( (DoStencilShadow==GE_FALSE) || (DoStencilShadow==GE_TRUE));

	P->DoStencilShadow = DoStencilShadow;
}
// end change 

void GENESISCC gePuppet_SetShadow(gePuppet *P, geBoolean DoShadow, 
		geFloat Scale, const geBitmap *ShadowMap,
		int BoneIndex)
{
	assert( P );
	assert( (DoShadow==GE_FALSE) || (DoShadow==GE_TRUE));

	if ( P->ShadowMap )
		geBitmap_Destroy((geBitmap **)&(P->ShadowMap));

	P->DoShadow = DoShadow;
	P->ShadowScale = Scale;
	P->ShadowMap = ShadowMap;
	P->ShadowBoneIndex = BoneIndex;

	if ( P->ShadowMap )
		geBitmap_CreateRef((geBitmap *)P->ShadowMap);
}

