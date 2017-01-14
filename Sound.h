/****************************************************************************************/
/*  Sound.h                                                                             */
/*                                                                                      */
/*  Author: Brian Adelberg                                                              */
/*  Description: DirectSound wrapper                                                    */
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
#ifndef	GE_SOUND_H
#define	GE_SOUND_H

#ifdef	__cplusplus
extern "C" {
#endif


// GENESIS_PUBLIC_APIS

typedef struct geSound_System	geSound_System;
typedef struct geSound_Def		geSound_Def;
typedef struct geSound			geSound;


#ifdef _INC_WINDOWS
	// Windows.h must be previously included for this api to be exposed.
GENESISAPI	geSound_System *geSound_CreateSoundSystem(HWND hWnd);
#endif

GENESISAPI	void			geSound_DestroySoundSystem(geSound_System *Sound);


GENESISAPI	geSound_Def	   *geSound_LoadSoundDef(geSound_System *SoundS, geVFile *File);
GENESISAPI	void			geSound_FreeSoundDef(geSound_System *SoundS, 
									geSound_Def *SoundDef);

GENESISAPI	geSound		   *geSound_PlaySoundDef(geSound_System *SoundS, 
									geSound_Def *SoundDef, 
									geFloat Volume, 
									geFloat Pan, 
									geFloat Frequency, 
									geBoolean Loop);
GENESISAPI	geBoolean		geSound_StopSound(geSound_System *SoundS, geSound *Sound);
GENESISAPI	geBoolean		geSound_ModifySound(geSound_System *SoundS, 
									geSound *Sound, 
									geFloat Volume, 
									geFloat Pan, 
									geFloat Frequency);
GENESISAPI	geBoolean		geSound_SoundIsPlaying(geSound_System *SoundS, geSound *Sound);
GENESISAPI	geBoolean		geSound_SetMasterVolume( geSound_System *SoundS, geFloat Volume );

// GENESIS_PRIVATE_APIS

#ifdef	__cplusplus
}
#endif

#endif

