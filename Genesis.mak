# Microsoft Developer Studio Generated NMAKE File, Based on Genesis.dsp
!IF "$(CFG)" == ""
CFG=Genesis - Win32 Debug
!MESSAGE No configuration specified. Defaulting to Genesis - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "Genesis - Win32 Release" && "$(CFG)" != "Genesis - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Genesis.mak" CFG="Genesis - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Genesis - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "Genesis - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Genesis - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\Genesis.lib"


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
	-@erase "$(INTDIR)\genesis.idb"
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
	-@erase "$(OUTDIR)\Genesis.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O2 /X /I "..\SDK\DXSDK\Include" /I "..\Source" /I "World" /I "Engine" /I "Engine\Drivers" /I "Actor" /I "BSP" /I "Math" /I "Entities" /I "Support" /I "Physics" /I "VFile" /I "Bitmap" /I "Bitmap\Compression" /I "..\MSDev60\Include" /I "..\MSDev60\MFC\Include" /I "..\SDK\DX6SDK\Include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\genesis.pdb" /FD /c 
RSC_PROJ=/l 0x409 /x /fo"$(INTDIR)\genesis.res" /i "..\MSDev60\Include" /i "..\MSDev60\MFC\Include" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Genesis.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\Genesis.lib" 
LIB32_OBJS= \
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
	"$(INTDIR)\A_CORONA.obj" \
	"$(INTDIR)\A_STREAK.obj" \
	"$(INTDIR)\CORONA.obj" \
	"$(INTDIR)\electric.obj" \
	"$(INTDIR)\logo.obj" \
	"$(INTDIR)\LogoActor.obj" \
	"$(INTDIR)\streak.obj" \
	"$(INTDIR)\WebUrl.obj" \
	"$(INTDIR)\BitmapList.obj" \
	"$(INTDIR)\engine.obj" \
	"$(INTDIR)\fontbmp.obj" \
	"$(INTDIR)\System.obj" \
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
	"$(INTDIR)\Box.obj" \
	"$(INTDIR)\crc32.obj" \
	"$(INTDIR)\ExtBox.obj" \
	"$(INTDIR)\quatern.obj" \
	"$(INTDIR)\Vec3d.obj" \
	"$(INTDIR)\Xform3d.obj" \
	"$(INTDIR)\Entities.obj" \
	"$(INTDIR)\Errorlog.obj" \
	"$(INTDIR)\geAssert.obj" \
	"$(INTDIR)\log.obj" \
	"$(INTDIR)\mempool.obj" \
	"$(INTDIR)\Ram.obj" \
	"$(INTDIR)\ramdll.obj" \
	"$(INTDIR)\matrix33.obj" \
	"$(INTDIR)\PhysicsJoint.obj" \
	"$(INTDIR)\PhysicsObject.obj" \
	"$(INTDIR)\PhysicsSystem.obj" \
	"$(INTDIR)\dirtree.obj" \
	"$(INTDIR)\fsdos.obj" \
	"$(INTDIR)\Fsmemory.obj" \
	"$(INTDIR)\fsvfs.obj" \
	"$(INTDIR)\vfile.obj" \
	"$(INTDIR)\palcreate.obj" \
	"$(INTDIR)\palettize.obj" \
	"$(INTDIR)\paloptimize.obj" \
	"$(INTDIR)\yuv.obj" \
	"$(INTDIR)\bitmap.obj" \
	"$(INTDIR)\bitmap_blitdata.obj" \
	"$(INTDIR)\bitmap_gamma.obj" \
	"$(INTDIR)\pixelformat.obj" \
	"$(INTDIR)\font.obj" \
	"$(INTDIR)\wgClip.obj" \
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
	"..\Sdk\Dx6sdk\Lib\dxguid.lib"

"$(OUTDIR)\Genesis.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Genesis - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\Genesis.lib"


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
	-@erase "$(INTDIR)\genesis.idb"
	-@erase "$(INTDIR)\genesis.pdb"
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
	-@erase "$(OUTDIR)\Genesis.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "..\SDK\DX6SDK\Include" /I "..\Source" /I "World" /I "Engine" /I "Engine\Drivers" /I "Actor" /I "BSP" /I "Math" /I "Entities" /I "Support" /I "Physics" /I "VFile" /I "Bitmap" /I "Bitmap\Compression" /I "..\MSDev60\Include" /I "..\MSDev60\MFC\Include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\genesis.pdb" /FD /GZ /c 
RSC_PROJ=/l 0x409 /x /fo"$(INTDIR)\genesis.res" /i "..\MSDev60\Include" /i "..\MSDev60\MFC\Include" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Genesis.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\Genesis.lib" 
LIB32_OBJS= \
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
	"$(INTDIR)\A_CORONA.obj" \
	"$(INTDIR)\A_STREAK.obj" \
	"$(INTDIR)\CORONA.obj" \
	"$(INTDIR)\electric.obj" \
	"$(INTDIR)\logo.obj" \
	"$(INTDIR)\LogoActor.obj" \
	"$(INTDIR)\streak.obj" \
	"$(INTDIR)\WebUrl.obj" \
	"$(INTDIR)\BitmapList.obj" \
	"$(INTDIR)\engine.obj" \
	"$(INTDIR)\fontbmp.obj" \
	"$(INTDIR)\System.obj" \
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
	"$(INTDIR)\Box.obj" \
	"$(INTDIR)\crc32.obj" \
	"$(INTDIR)\ExtBox.obj" \
	"$(INTDIR)\quatern.obj" \
	"$(INTDIR)\Vec3d.obj" \
	"$(INTDIR)\Xform3d.obj" \
	"$(INTDIR)\Entities.obj" \
	"$(INTDIR)\Errorlog.obj" \
	"$(INTDIR)\geAssert.obj" \
	"$(INTDIR)\log.obj" \
	"$(INTDIR)\mempool.obj" \
	"$(INTDIR)\Ram.obj" \
	"$(INTDIR)\ramdll.obj" \
	"$(INTDIR)\matrix33.obj" \
	"$(INTDIR)\PhysicsJoint.obj" \
	"$(INTDIR)\PhysicsObject.obj" \
	"$(INTDIR)\PhysicsSystem.obj" \
	"$(INTDIR)\dirtree.obj" \
	"$(INTDIR)\fsdos.obj" \
	"$(INTDIR)\Fsmemory.obj" \
	"$(INTDIR)\fsvfs.obj" \
	"$(INTDIR)\vfile.obj" \
	"$(INTDIR)\palcreate.obj" \
	"$(INTDIR)\palettize.obj" \
	"$(INTDIR)\paloptimize.obj" \
	"$(INTDIR)\yuv.obj" \
	"$(INTDIR)\bitmap.obj" \
	"$(INTDIR)\bitmap_blitdata.obj" \
	"$(INTDIR)\bitmap_gamma.obj" \
	"$(INTDIR)\pixelformat.obj" \
	"$(INTDIR)\font.obj" \
	"$(INTDIR)\wgClip.obj" \
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
	"..\Sdk\Dx6sdk\Lib\dxguid.lib"

"$(OUTDIR)\Genesis.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
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
!IF EXISTS("Genesis.dep")
!INCLUDE "Genesis.dep"
!ELSE 
!MESSAGE Warning: cannot find "Genesis.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "Genesis - Win32 Release" || "$(CFG)" == "Genesis - Win32 Debug"
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


SOURCE=.\Entities\Entities.c

"$(INTDIR)\Entities.obj" : $(SOURCE) "$(INTDIR)"
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


SOURCE=.\Font\font.c

"$(INTDIR)\font.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Font\wgClip.c

"$(INTDIR)\wgClip.obj" : $(SOURCE) "$(INTDIR)"
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

