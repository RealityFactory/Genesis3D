# Microsoft Developer Studio Generated NMAKE File, Based on GenesisDLL.dsp
!IF "$(CFG)" == ""
CFG=GenesisDLL - Win32 Debug
!MESSAGE No configuration specified. Defaulting to GenesisDLL - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "GenesisDLL - Win32 Release" && "$(CFG)" != "GenesisDLL - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "GenesisDLL.mak" CFG="GenesisDLL - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "GenesisDLL - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "GenesisDLL - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "GenesisDLL - Win32 Release"

OUTDIR=.\ReleaseDLL
INTDIR=.\ReleaseDLL
# Begin Custom Macros
OutDir=.\ReleaseDLL
# End Custom Macros

ALL : "$(OUTDIR)\Genesis.dll" ".\ReleaseDLL\genesisi.lib"


CLEAN :
	-@erase "$(INTDIR)\A_CORONA.obj"
	-@erase "$(INTDIR)\A_STREAK.obj"
	-@erase "$(INTDIR)\actor.obj"
	-@erase "$(INTDIR)\bitmap.obj"
	-@erase "$(INTDIR)\bitmap_blitdata.obj"
	-@erase "$(INTDIR)\bitmap_gamma.obj"
	-@erase "$(INTDIR)\BitmapList.obj"
	-@erase "$(INTDIR)\body.obj"
	-@erase "$(INTDIR)\bodyinst.obj"
	-@erase "$(INTDIR)\Box.obj"
	-@erase "$(INTDIR)\Camera.obj"
	-@erase "$(INTDIR)\CORONA.obj"
	-@erase "$(INTDIR)\crc32.obj"
	-@erase "$(INTDIR)\CSNetMgr.obj"
	-@erase "$(INTDIR)\dirtree.obj"
	-@erase "$(INTDIR)\electric.obj"
	-@erase "$(INTDIR)\engine.obj"
	-@erase "$(INTDIR)\Entities.obj"
	-@erase "$(INTDIR)\Errorlog.obj"
	-@erase "$(INTDIR)\ExtBox.obj"
	-@erase "$(INTDIR)\Fog.obj"
	-@erase "$(INTDIR)\font.obj"
	-@erase "$(INTDIR)\fontbmp.obj"
	-@erase "$(INTDIR)\Frustum.obj"
	-@erase "$(INTDIR)\fsdos.obj"
	-@erase "$(INTDIR)\Fsmemory.obj"
	-@erase "$(INTDIR)\fsvfs.obj"
	-@erase "$(INTDIR)\Gbspfile.obj"
	-@erase "$(INTDIR)\Ge.obj"
	-@erase "$(INTDIR)\geAssert.obj"
	-@erase "$(INTDIR)\genesis.res"
	-@erase "$(INTDIR)\Light.obj"
	-@erase "$(INTDIR)\list.obj"
	-@erase "$(INTDIR)\log.obj"
	-@erase "$(INTDIR)\logo.obj"
	-@erase "$(INTDIR)\LogoActor.obj"
	-@erase "$(INTDIR)\matrix33.obj"
	-@erase "$(INTDIR)\mempool.obj"
	-@erase "$(INTDIR)\motion.obj"
	-@erase "$(INTDIR)\Netplay.obj"
	-@erase "$(INTDIR)\palcreate.obj"
	-@erase "$(INTDIR)\palettize.obj"
	-@erase "$(INTDIR)\paloptimize.obj"
	-@erase "$(INTDIR)\path.obj"
	-@erase "$(INTDIR)\PhysicsJoint.obj"
	-@erase "$(INTDIR)\PhysicsObject.obj"
	-@erase "$(INTDIR)\PhysicsSystem.obj"
	-@erase "$(INTDIR)\pixelformat.obj"
	-@erase "$(INTDIR)\Plane.obj"
	-@erase "$(INTDIR)\pose.obj"
	-@erase "$(INTDIR)\puppet.obj"
	-@erase "$(INTDIR)\QKFrame.obj"
	-@erase "$(INTDIR)\quatern.obj"
	-@erase "$(INTDIR)\Ram.obj"
	-@erase "$(INTDIR)\ramdll.obj"
	-@erase "$(INTDIR)\Sound.obj"
	-@erase "$(INTDIR)\Sound3d.obj"
	-@erase "$(INTDIR)\strblock.obj"
	-@erase "$(INTDIR)\streak.obj"
	-@erase "$(INTDIR)\Surface.obj"
	-@erase "$(INTDIR)\System.obj"
	-@erase "$(INTDIR)\Tclip.obj"
	-@erase "$(INTDIR)\timer.obj"
	-@erase "$(INTDIR)\tkarray.obj"
	-@erase "$(INTDIR)\tkevents.obj"
	-@erase "$(INTDIR)\Trace.obj"
	-@erase "$(INTDIR)\tsc.obj"
	-@erase "$(INTDIR)\User.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\Vec3d.obj"
	-@erase "$(INTDIR)\vfile.obj"
	-@erase "$(INTDIR)\Vis.obj"
	-@erase "$(INTDIR)\vkframe.obj"
	-@erase "$(INTDIR)\WBitmap.obj"
	-@erase "$(INTDIR)\WebUrl.obj"
	-@erase "$(INTDIR)\wgClip.obj"
	-@erase "$(INTDIR)\World.obj"
	-@erase "$(INTDIR)\XFArray.obj"
	-@erase "$(INTDIR)\Xform3d.obj"
	-@erase "$(INTDIR)\yuv.obj"
	-@erase "$(OUTDIR)\Genesis.dll"
	-@erase "$(OUTDIR)\Genesis.exp"
	-@erase "$(OUTDIR)\Genesis.lib"
	-@erase ".\ReleaseDLL\genesisi.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O2 /X /I "..\SDK\DX6SDK\Include" /I "..\Source" /I "World" /I "Engine" /I "Engine\Drivers" /I "Actor" /I "BSP" /I "Math" /I "Entities" /I "Support" /I "Physics" /I "VFile" /I "Bitmap" /I "Bitmap\Compression" /I "..\MSDev60\Include" /I "..\MSDev60\MFC\Include" /I "guWorld" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "BUILDGENESIS" /D "GENESISDLLVERSION" /Fp"$(INTDIR)\GenesisDLL.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\genesis.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\GenesisDLL.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=libcmt.lib kernel32.lib user32.lib gdi32.lib oldnames.lib ole32.lib urlmon.lib uuid.lib winmm.lib /nologo /dll /incremental:no /pdb:"$(OUTDIR)\Genesis.pdb" /machine:I386 /nodefaultlib /out:"$(OUTDIR)\Genesis.dll" /implib:"$(OUTDIR)\Genesis.lib" 
LINK32_OBJS= \
	"$(INTDIR)\actor.obj" \
	"$(INTDIR)\body.obj" \
	"$(INTDIR)\bodyinst.obj" \
	"$(INTDIR)\motion.obj" \
	"$(INTDIR)\path.obj" \
	"$(INTDIR)\pose.obj" \
	"$(INTDIR)\puppet.obj" \
	"$(INTDIR)\QKFrame.obj" \
	"$(INTDIR)\strblock.obj" \
	"$(INTDIR)\tkarray.obj" \
	"$(INTDIR)\tkevents.obj" \
	"$(INTDIR)\vkframe.obj" \
	"$(INTDIR)\XFArray.obj" \
	"$(INTDIR)\palcreate.obj" \
	"$(INTDIR)\palettize.obj" \
	"$(INTDIR)\paloptimize.obj" \
	"$(INTDIR)\yuv.obj" \
	"$(INTDIR)\bitmap.obj" \
	"$(INTDIR)\bitmap_blitdata.obj" \
	"$(INTDIR)\bitmap_gamma.obj" \
	"$(INTDIR)\pixelformat.obj" \
	"$(INTDIR)\A_CORONA.obj" \
	"$(INTDIR)\A_STREAK.obj" \
	"$(INTDIR)\CORONA.obj" \
	"$(INTDIR)\electric.obj" \
	"$(INTDIR)\logo.obj" \
	"$(INTDIR)\LogoActor.obj" \
	"$(INTDIR)\streak.obj" \
	"$(INTDIR)\BitmapList.obj" \
	"$(INTDIR)\engine.obj" \
	"$(INTDIR)\fontbmp.obj" \
	"$(INTDIR)\System.obj" \
	"$(INTDIR)\Entities.obj" \
	"$(INTDIR)\Box.obj" \
	"$(INTDIR)\crc32.obj" \
	"$(INTDIR)\ExtBox.obj" \
	"$(INTDIR)\quatern.obj" \
	"$(INTDIR)\Vec3d.obj" \
	"$(INTDIR)\Xform3d.obj" \
	"$(INTDIR)\Errorlog.obj" \
	"$(INTDIR)\geAssert.obj" \
	"$(INTDIR)\log.obj" \
	"$(INTDIR)\mempool.obj" \
	"$(INTDIR)\Ram.obj" \
	"$(INTDIR)\ramdll.obj" \
	"$(INTDIR)\dirtree.obj" \
	"$(INTDIR)\fsdos.obj" \
	"$(INTDIR)\Fsmemory.obj" \
	"$(INTDIR)\fsvfs.obj" \
	"$(INTDIR)\vfile.obj" \
	"$(INTDIR)\Fog.obj" \
	"$(INTDIR)\Frustum.obj" \
	"$(INTDIR)\Gbspfile.obj" \
	"$(INTDIR)\Light.obj" \
	"$(INTDIR)\Plane.obj" \
	"$(INTDIR)\Surface.obj" \
	"$(INTDIR)\Trace.obj" \
	"$(INTDIR)\User.obj" \
	"$(INTDIR)\Vis.obj" \
	"$(INTDIR)\WBitmap.obj" \
	"$(INTDIR)\World.obj" \
	"$(INTDIR)\font.obj" \
	"$(INTDIR)\wgClip.obj" \
	"$(INTDIR)\matrix33.obj" \
	"$(INTDIR)\PhysicsJoint.obj" \
	"$(INTDIR)\PhysicsObject.obj" \
	"$(INTDIR)\PhysicsSystem.obj" \
	"$(INTDIR)\Camera.obj" \
	"$(INTDIR)\CSNetMgr.obj" \
	"$(INTDIR)\Ge.obj" \
	"$(INTDIR)\list.obj" \
	"$(INTDIR)\Netplay.obj" \
	"$(INTDIR)\Sound.obj" \
	"$(INTDIR)\Sound3d.obj" \
	"$(INTDIR)\Tclip.obj" \
	"$(INTDIR)\timer.obj" \
	"$(INTDIR)\tsc.obj" \
	"$(INTDIR)\genesis.res" \
	"..\Sdk\Dx6sdk\Lib\dxguid.lib" \
	"$(INTDIR)\WebUrl.obj"

"$(OUTDIR)\Genesis.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

IntDir=.\ReleaseDLL
OutDir=.\ReleaseDLL
InputPath=.\ReleaseDLL\Genesis.dll
SOURCE="$(InputPath)"

"$(OUTDIR)\genesisi.lib" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	lib /OUT:$(OUTDIR)\genesisi.lib $(IntDir)\ramdll.obj $(OUTDIR)\genesis.lib
<< 
	

!ELSEIF  "$(CFG)" == "GenesisDLL - Win32 Debug"

OUTDIR=.\DebugDLL
INTDIR=.\DebugDLL
# Begin Custom Macros
OutDir=.\DebugDLL
# End Custom Macros

ALL : "$(OUTDIR)\Genesis.dll" ".\DebugDLL\genesisid.lib"


CLEAN :
	-@erase "$(INTDIR)\A_CORONA.obj"
	-@erase "$(INTDIR)\A_STREAK.obj"
	-@erase "$(INTDIR)\actor.obj"
	-@erase "$(INTDIR)\bitmap.obj"
	-@erase "$(INTDIR)\bitmap_blitdata.obj"
	-@erase "$(INTDIR)\bitmap_gamma.obj"
	-@erase "$(INTDIR)\BitmapList.obj"
	-@erase "$(INTDIR)\body.obj"
	-@erase "$(INTDIR)\bodyinst.obj"
	-@erase "$(INTDIR)\Box.obj"
	-@erase "$(INTDIR)\Camera.obj"
	-@erase "$(INTDIR)\CORONA.obj"
	-@erase "$(INTDIR)\crc32.obj"
	-@erase "$(INTDIR)\CSNetMgr.obj"
	-@erase "$(INTDIR)\dirtree.obj"
	-@erase "$(INTDIR)\electric.obj"
	-@erase "$(INTDIR)\engine.obj"
	-@erase "$(INTDIR)\Entities.obj"
	-@erase "$(INTDIR)\Errorlog.obj"
	-@erase "$(INTDIR)\ExtBox.obj"
	-@erase "$(INTDIR)\Fog.obj"
	-@erase "$(INTDIR)\font.obj"
	-@erase "$(INTDIR)\fontbmp.obj"
	-@erase "$(INTDIR)\Frustum.obj"
	-@erase "$(INTDIR)\fsdos.obj"
	-@erase "$(INTDIR)\Fsmemory.obj"
	-@erase "$(INTDIR)\fsvfs.obj"
	-@erase "$(INTDIR)\Gbspfile.obj"
	-@erase "$(INTDIR)\Ge.obj"
	-@erase "$(INTDIR)\geAssert.obj"
	-@erase "$(INTDIR)\genesis.res"
	-@erase "$(INTDIR)\Light.obj"
	-@erase "$(INTDIR)\list.obj"
	-@erase "$(INTDIR)\log.obj"
	-@erase "$(INTDIR)\logo.obj"
	-@erase "$(INTDIR)\LogoActor.obj"
	-@erase "$(INTDIR)\matrix33.obj"
	-@erase "$(INTDIR)\mempool.obj"
	-@erase "$(INTDIR)\motion.obj"
	-@erase "$(INTDIR)\Netplay.obj"
	-@erase "$(INTDIR)\palcreate.obj"
	-@erase "$(INTDIR)\palettize.obj"
	-@erase "$(INTDIR)\paloptimize.obj"
	-@erase "$(INTDIR)\path.obj"
	-@erase "$(INTDIR)\PhysicsJoint.obj"
	-@erase "$(INTDIR)\PhysicsObject.obj"
	-@erase "$(INTDIR)\PhysicsSystem.obj"
	-@erase "$(INTDIR)\pixelformat.obj"
	-@erase "$(INTDIR)\Plane.obj"
	-@erase "$(INTDIR)\pose.obj"
	-@erase "$(INTDIR)\puppet.obj"
	-@erase "$(INTDIR)\QKFrame.obj"
	-@erase "$(INTDIR)\quatern.obj"
	-@erase "$(INTDIR)\Ram.obj"
	-@erase "$(INTDIR)\ramdll.obj"
	-@erase "$(INTDIR)\Sound.obj"
	-@erase "$(INTDIR)\Sound3d.obj"
	-@erase "$(INTDIR)\strblock.obj"
	-@erase "$(INTDIR)\streak.obj"
	-@erase "$(INTDIR)\Surface.obj"
	-@erase "$(INTDIR)\System.obj"
	-@erase "$(INTDIR)\Tclip.obj"
	-@erase "$(INTDIR)\timer.obj"
	-@erase "$(INTDIR)\tkarray.obj"
	-@erase "$(INTDIR)\tkevents.obj"
	-@erase "$(INTDIR)\Trace.obj"
	-@erase "$(INTDIR)\tsc.obj"
	-@erase "$(INTDIR)\User.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\Vec3d.obj"
	-@erase "$(INTDIR)\vfile.obj"
	-@erase "$(INTDIR)\Vis.obj"
	-@erase "$(INTDIR)\vkframe.obj"
	-@erase "$(INTDIR)\WBitmap.obj"
	-@erase "$(INTDIR)\WebUrl.obj"
	-@erase "$(INTDIR)\wgClip.obj"
	-@erase "$(INTDIR)\World.obj"
	-@erase "$(INTDIR)\XFArray.obj"
	-@erase "$(INTDIR)\Xform3d.obj"
	-@erase "$(INTDIR)\yuv.obj"
	-@erase "$(OUTDIR)\Genesis.dll"
	-@erase "$(OUTDIR)\Genesis.exp"
	-@erase "$(OUTDIR)\Genesis.ilk"
	-@erase "$(OUTDIR)\Genesis.lib"
	-@erase "$(OUTDIR)\Genesis.pdb"
	-@erase ".\DebugDLL\genesisid.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "..\SDK\DX6SDK\Include" /I "..\Source" /I "World" /I "Engine" /I "Engine\Drivers" /I "Actor" /I "BSP" /I "Math" /I "Entities" /I "Support" /I "Physics" /I "VFile" /I "Bitmap" /I "Bitmap\Compression" /I "..\MSDev60\Include" /I "..\MSDev60\MFC\Include" /I "guWorld" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "BUILDGENESIS" /D "GENESISDLLVERSION" /Fp"$(INTDIR)\GenesisDLL.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\genesis.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\GenesisDLL.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=libcmtd.lib kernel32.lib user32.lib gdi32.lib oldnames.lib ole32.lib urlmon.lib uuid.lib winmm.lib /nologo /dll /incremental:yes /pdb:"$(OUTDIR)\Genesis.pdb" /debug /machine:I386 /nodefaultlib /out:"$(OUTDIR)\Genesis.dll" /implib:"$(OUTDIR)\Genesis.lib" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\actor.obj" \
	"$(INTDIR)\body.obj" \
	"$(INTDIR)\bodyinst.obj" \
	"$(INTDIR)\motion.obj" \
	"$(INTDIR)\path.obj" \
	"$(INTDIR)\pose.obj" \
	"$(INTDIR)\puppet.obj" \
	"$(INTDIR)\QKFrame.obj" \
	"$(INTDIR)\strblock.obj" \
	"$(INTDIR)\tkarray.obj" \
	"$(INTDIR)\tkevents.obj" \
	"$(INTDIR)\vkframe.obj" \
	"$(INTDIR)\XFArray.obj" \
	"$(INTDIR)\palcreate.obj" \
	"$(INTDIR)\palettize.obj" \
	"$(INTDIR)\paloptimize.obj" \
	"$(INTDIR)\yuv.obj" \
	"$(INTDIR)\bitmap.obj" \
	"$(INTDIR)\bitmap_blitdata.obj" \
	"$(INTDIR)\bitmap_gamma.obj" \
	"$(INTDIR)\pixelformat.obj" \
	"$(INTDIR)\A_CORONA.obj" \
	"$(INTDIR)\A_STREAK.obj" \
	"$(INTDIR)\CORONA.obj" \
	"$(INTDIR)\electric.obj" \
	"$(INTDIR)\logo.obj" \
	"$(INTDIR)\LogoActor.obj" \
	"$(INTDIR)\streak.obj" \
	"$(INTDIR)\BitmapList.obj" \
	"$(INTDIR)\engine.obj" \
	"$(INTDIR)\fontbmp.obj" \
	"$(INTDIR)\System.obj" \
	"$(INTDIR)\Entities.obj" \
	"$(INTDIR)\Box.obj" \
	"$(INTDIR)\crc32.obj" \
	"$(INTDIR)\ExtBox.obj" \
	"$(INTDIR)\quatern.obj" \
	"$(INTDIR)\Vec3d.obj" \
	"$(INTDIR)\Xform3d.obj" \
	"$(INTDIR)\Errorlog.obj" \
	"$(INTDIR)\geAssert.obj" \
	"$(INTDIR)\log.obj" \
	"$(INTDIR)\mempool.obj" \
	"$(INTDIR)\Ram.obj" \
	"$(INTDIR)\ramdll.obj" \
	"$(INTDIR)\dirtree.obj" \
	"$(INTDIR)\fsdos.obj" \
	"$(INTDIR)\Fsmemory.obj" \
	"$(INTDIR)\fsvfs.obj" \
	"$(INTDIR)\vfile.obj" \
	"$(INTDIR)\Fog.obj" \
	"$(INTDIR)\Frustum.obj" \
	"$(INTDIR)\Gbspfile.obj" \
	"$(INTDIR)\Light.obj" \
	"$(INTDIR)\Plane.obj" \
	"$(INTDIR)\Surface.obj" \
	"$(INTDIR)\Trace.obj" \
	"$(INTDIR)\User.obj" \
	"$(INTDIR)\Vis.obj" \
	"$(INTDIR)\WBitmap.obj" \
	"$(INTDIR)\World.obj" \
	"$(INTDIR)\font.obj" \
	"$(INTDIR)\wgClip.obj" \
	"$(INTDIR)\matrix33.obj" \
	"$(INTDIR)\PhysicsJoint.obj" \
	"$(INTDIR)\PhysicsObject.obj" \
	"$(INTDIR)\PhysicsSystem.obj" \
	"$(INTDIR)\Camera.obj" \
	"$(INTDIR)\CSNetMgr.obj" \
	"$(INTDIR)\Ge.obj" \
	"$(INTDIR)\list.obj" \
	"$(INTDIR)\Netplay.obj" \
	"$(INTDIR)\Sound.obj" \
	"$(INTDIR)\Sound3d.obj" \
	"$(INTDIR)\Tclip.obj" \
	"$(INTDIR)\timer.obj" \
	"$(INTDIR)\tsc.obj" \
	"$(INTDIR)\genesis.res" \
	"..\Sdk\Dx6sdk\Lib\dxguid.lib" \
	"$(INTDIR)\WebUrl.obj"

"$(OUTDIR)\Genesis.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

IntDir=.\DebugDLL
OutDir=.\DebugDLL
InputPath=.\DebugDLL\Genesis.dll
SOURCE="$(InputPath)"

"$(OUTDIR)\genesisid.lib" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	lib /OUT:$(OUTDIR)\genesisid.lib $(IntDir)\ramdll.obj $(OUTDIR)\genesis.lib
<< 
	

!ENDIF 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("GenesisDLL.dep")
!INCLUDE "GenesisDLL.dep"
!ELSE 
!MESSAGE Warning: cannot find "GenesisDLL.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "GenesisDLL - Win32 Release" || "$(CFG)" == "GenesisDLL - Win32 Debug"
SOURCE=.\Actor\actor.c

"$(INTDIR)\actor.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Actor\body.c

"$(INTDIR)\body.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Actor\bodyinst.c

"$(INTDIR)\bodyinst.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Actor\motion.c

"$(INTDIR)\motion.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Actor\path.c

"$(INTDIR)\path.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Actor\pose.c

"$(INTDIR)\pose.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Actor\puppet.c

"$(INTDIR)\puppet.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Actor\QKFrame.c

"$(INTDIR)\QKFrame.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Actor\strblock.c

"$(INTDIR)\strblock.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Actor\tkarray.c

"$(INTDIR)\tkarray.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Actor\tkevents.c

"$(INTDIR)\tkevents.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Actor\vkframe.c

"$(INTDIR)\vkframe.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Actor\XFArray.c

"$(INTDIR)\XFArray.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Bitmap\Compression\palcreate.c

"$(INTDIR)\palcreate.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Bitmap\Compression\palettize.c

"$(INTDIR)\palettize.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Bitmap\Compression\paloptimize.c

"$(INTDIR)\paloptimize.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Bitmap\Compression\yuv.c

"$(INTDIR)\yuv.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Bitmap\bitmap.c

"$(INTDIR)\bitmap.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Bitmap\bitmap_blitdata.c

"$(INTDIR)\bitmap_blitdata.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Bitmap\bitmap_gamma.c

"$(INTDIR)\bitmap_gamma.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Bitmap\pixelformat.c

"$(INTDIR)\pixelformat.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Engine\Logo\A_CORONA.c

"$(INTDIR)\A_CORONA.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Engine\Logo\A_STREAK.c

"$(INTDIR)\A_STREAK.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Engine\Logo\CORONA.c

"$(INTDIR)\CORONA.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Engine\Logo\electric.c

"$(INTDIR)\electric.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Engine\Logo\logo.c

"$(INTDIR)\logo.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Engine\Logo\LogoActor.c

"$(INTDIR)\LogoActor.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Engine\Logo\streak.c

"$(INTDIR)\streak.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Engine\Logo\WebUrl.c

"$(INTDIR)\WebUrl.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Engine\BitmapList.c

"$(INTDIR)\BitmapList.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Engine\engine.c

"$(INTDIR)\engine.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Engine\fontbmp.c

"$(INTDIR)\fontbmp.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Engine\System.c

"$(INTDIR)\System.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Entities\Entities.c

"$(INTDIR)\Entities.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Math\Box.c

"$(INTDIR)\Box.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Math\crc32.c

"$(INTDIR)\crc32.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Math\ExtBox.c

"$(INTDIR)\ExtBox.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Math\quatern.c

"$(INTDIR)\quatern.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Math\Vec3d.c

"$(INTDIR)\Vec3d.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Math\Xform3d.c

"$(INTDIR)\Xform3d.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Support\Errorlog.c

"$(INTDIR)\Errorlog.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Support\geAssert.c

"$(INTDIR)\geAssert.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Support\log.c

"$(INTDIR)\log.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Support\mempool.c

"$(INTDIR)\mempool.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Support\Ram.c

"$(INTDIR)\Ram.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Support\ramdll.c

"$(INTDIR)\ramdll.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\VFile\dirtree.c

"$(INTDIR)\dirtree.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\VFile\fsdos.c

"$(INTDIR)\fsdos.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\VFile\Fsmemory.c

"$(INTDIR)\Fsmemory.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\VFile\fsvfs.c

"$(INTDIR)\fsvfs.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\VFile\vfile.c

"$(INTDIR)\vfile.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\World\Fog.c

"$(INTDIR)\Fog.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\World\Frustum.c

"$(INTDIR)\Frustum.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\World\Gbspfile.c

"$(INTDIR)\Gbspfile.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\World\Light.c

"$(INTDIR)\Light.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\World\Plane.c

"$(INTDIR)\Plane.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\World\Surface.c

"$(INTDIR)\Surface.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\World\Trace.c

"$(INTDIR)\Trace.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\World\User.c

"$(INTDIR)\User.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\World\Vis.c

"$(INTDIR)\Vis.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\World\WBitmap.c

"$(INTDIR)\WBitmap.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\World\World.c

"$(INTDIR)\World.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Font\font.c

"$(INTDIR)\font.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Font\wgClip.c

"$(INTDIR)\wgClip.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Physics\matrix33.c

"$(INTDIR)\matrix33.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Physics\PhysicsJoint.c

"$(INTDIR)\PhysicsJoint.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Physics\PhysicsObject.c

"$(INTDIR)\PhysicsObject.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Physics\PhysicsSystem.c

"$(INTDIR)\PhysicsSystem.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Camera.c

"$(INTDIR)\Camera.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\CSNetMgr.c

"$(INTDIR)\CSNetMgr.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Ge.c

"$(INTDIR)\Ge.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\genesis.rc

"$(INTDIR)\genesis.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\list.c

"$(INTDIR)\list.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Netplay.c

"$(INTDIR)\Netplay.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Sound.c

"$(INTDIR)\Sound.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Sound3d.c

"$(INTDIR)\Sound3d.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Tclip.c

"$(INTDIR)\Tclip.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\timer.c

"$(INTDIR)\timer.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\tsc.c

"$(INTDIR)\tsc.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 

