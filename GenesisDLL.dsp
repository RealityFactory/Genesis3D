# Microsoft Developer Studio Project File - Name="GenesisDLL" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=GenesisDLL - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "GenesisDLL.mak".
!MESSAGE 
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

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Genesis20/Source", VBRBAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "GenesisDLL - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseDLL"
# PROP Intermediate_Dir "ReleaseDLL"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GENESISDLL_EXPORTS" /YX /FD /c
# ADD CPP /nologo /G5 /MT /W3 /GX /O2 /Ob2 /I "..\Source" /I "World" /I "Engine" /I "Engine\Drivers" /I "Actor" /I "BSP" /I "Math" /I "Entities" /I "Support" /I "Physics" /I "VFile" /I "Bitmap" /I "Bitmap\Compression" /I "..\G3D" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "BUILDGENESIS" /D "GENESISDLLVERSION" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 libcmt.lib kernel32.lib user32.lib gdi32.lib oldnames.lib ole32.lib urlmon.lib uuid.lib winmm.lib /nologo /dll /machine:I386 /out:"ReleaseDLL/Genesis.dll"
# SUBTRACT LINK32 /nodefaultlib
# Begin Custom Build
IntDir=.\ReleaseDLL
OutDir=.\ReleaseDLL
InputPath=.\ReleaseDLL\Genesis.dll
SOURCE="$(InputPath)"

"$(OUTDIR)\genesisi.lib" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	lib /OUT:$(OUTDIR)\genesisi.lib $(IntDir)\ramdll.obj $(OUTDIR)\genesis.lib

# End Custom Build

!ELSEIF  "$(CFG)" == "GenesisDLL - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "GenesisDLL___Win32_Debug"
# PROP BASE Intermediate_Dir "GenesisDLL___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugDLL"
# PROP Intermediate_Dir "DebugDLL"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GENESISDLL_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /G5 /MTd /W3 /Gm /GX /ZI /Od /I "..\Source" /I "World" /I "Engine" /I "Engine\Drivers" /I "Actor" /I "BSP" /I "Math" /I "Entities" /I "Support" /I "Physics" /I "VFile" /I "Bitmap" /I "Bitmap\Compression" /I "guWorld" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "BUILDGENESIS" /D "GENESISDLLVERSION" /YX /FD /GZ /c
# SUBTRACT CPP /X
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 libcmtd.lib kernel32.lib user32.lib gdi32.lib oldnames.lib ole32.lib urlmon.lib uuid.lib winmm.lib /nologo /dll /debug /machine:I386 /out:"DebugDLL/Genesis.dll" /pdbtype:sept
# SUBTRACT LINK32 /nodefaultlib
# Begin Custom Build
IntDir=.\DebugDLL
OutDir=.\DebugDLL
InputPath=.\DebugDLL\Genesis.dll
SOURCE="$(InputPath)"

"$(OUTDIR)\genesisid.lib" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	lib /OUT:$(OUTDIR)\genesisid.lib $(IntDir)\ramdll.obj $(OUTDIR)\genesis.lib

# End Custom Build

!ENDIF 

# Begin Target

# Name "GenesisDLL - Win32 Release"
# Name "GenesisDLL - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Group "Actor"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Actor\Actor.c
# End Source File
# Begin Source File

SOURCE=.\Actor\Actor.h
# End Source File
# Begin Source File

SOURCE=.\Actor\Body.c
# End Source File
# Begin Source File

SOURCE=.\Actor\Body.h
# End Source File
# Begin Source File

SOURCE=.\Actor\BodyInst.c
# End Source File
# Begin Source File

SOURCE=.\Actor\BodyInst.h
# End Source File
# Begin Source File

SOURCE=.\Actor\Motion.c
# End Source File
# Begin Source File

SOURCE=.\Actor\Motion.h
# End Source File
# Begin Source File

SOURCE=.\Actor\Path.c
# End Source File
# Begin Source File

SOURCE=.\Actor\Path.h
# End Source File
# Begin Source File

SOURCE=.\Actor\Pose.c
# End Source File
# Begin Source File

SOURCE=.\Actor\Pose.h
# End Source File
# Begin Source File

SOURCE=.\Actor\Puppet.c
# End Source File
# Begin Source File

SOURCE=.\Actor\Puppet.h
# End Source File
# Begin Source File

SOURCE=.\Actor\QKFrame.c
# End Source File
# Begin Source File

SOURCE=.\Actor\QKFrame.h
# End Source File
# Begin Source File

SOURCE=.\Actor\StrBlock.c
# End Source File
# Begin Source File

SOURCE=.\Actor\StrBlock.h
# End Source File
# Begin Source File

SOURCE=.\Actor\TKArray.c
# End Source File
# Begin Source File

SOURCE=.\Actor\TKArray.h
# End Source File
# Begin Source File

SOURCE=.\Actor\TKEvents.c
# End Source File
# Begin Source File

SOURCE=.\Actor\TKEvents.h
# End Source File
# Begin Source File

SOURCE=.\Actor\VKFrame.c
# End Source File
# Begin Source File

SOURCE=.\Actor\VKFrame.h
# End Source File
# Begin Source File

SOURCE=.\Actor\XFArray.c
# End Source File
# Begin Source File

SOURCE=.\Actor\XFArray.h
# End Source File
# End Group
# Begin Group "Bitmap"

# PROP Default_Filter ""
# Begin Group "Compression"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Bitmap\Compression\PalCreate.c
# End Source File
# Begin Source File

SOURCE=.\Bitmap\Compression\PalCreate.h
# End Source File
# Begin Source File

SOURCE=.\Bitmap\Compression\Palettize.c
# End Source File
# Begin Source File

SOURCE=.\Bitmap\Compression\Palettize.h
# End Source File
# Begin Source File

SOURCE=.\Bitmap\Compression\PalOptimize.c
# End Source File
# Begin Source File

SOURCE=.\Bitmap\Compression\PalOptimize.h
# End Source File
# Begin Source File

SOURCE=.\Bitmap\Compression\Utility.h
# End Source File
# Begin Source File

SOURCE=.\Bitmap\Compression\Yuv.c
# End Source File
# Begin Source File

SOURCE=.\Bitmap\Compression\Yuv.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\Bitmap\Bitmap.__h
# End Source File
# Begin Source File

SOURCE=.\Bitmap\Bitmap._h
# End Source File
# Begin Source File

SOURCE=.\Bitmap\Bitmap.c
# End Source File
# Begin Source File

SOURCE=.\Bitmap\Bitmap.h
# End Source File
# Begin Source File

SOURCE=.\Bitmap\Bitmap_BlitData.c
# End Source File
# Begin Source File

SOURCE=.\Bitmap\Bitmap_BlitData.h
# End Source File
# Begin Source File

SOURCE=.\Bitmap\Bitmap_Gamma.c
# End Source File
# Begin Source File

SOURCE=.\Bitmap\Bitmap_Gamma.h
# End Source File
# Begin Source File

SOURCE=.\Bitmap\PixelFormat.c
# End Source File
# Begin Source File

SOURCE=.\Bitmap\PixelFormat.h
# End Source File
# End Group
# Begin Group "Engine"

# PROP Default_Filter ""
# Begin Group "Logo"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Engine\Logo\A_Corona.c
# End Source File
# Begin Source File

SOURCE=.\Engine\Logo\A_Streak.c
# End Source File
# Begin Source File

SOURCE=.\Engine\Logo\Corona.c
# End Source File
# Begin Source File

SOURCE=.\Engine\Logo\Electric.c
# End Source File
# Begin Source File

SOURCE=.\Engine\Logo\Electric.h
# End Source File
# Begin Source File

SOURCE=.\Engine\Logo\Logo.c
# End Source File
# Begin Source File

SOURCE=.\Engine\Logo\LogoActor.c
# End Source File
# Begin Source File

SOURCE=.\Engine\Logo\Streak.c
# End Source File
# Begin Source File

SOURCE=.\Engine\Logo\WebUrl.c
# End Source File
# End Group
# Begin Source File

SOURCE=.\Engine\BitmapList.c
# End Source File
# Begin Source File

SOURCE=.\Engine\BitmapList.h
# End Source File
# Begin Source File

SOURCE=.\Engine\Engine.c
# End Source File
# Begin Source File

SOURCE=.\Engine\Engine.h
# End Source File
# Begin Source File

SOURCE=.\Engine\FontBmp.c
# End Source File
# Begin Source File

SOURCE=.\Engine\System.c
# End Source File
# Begin Source File

SOURCE=.\Engine\System.h
# End Source File
# End Group
# Begin Group "Entities"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Entities\Entities.c
# End Source File
# Begin Source File

SOURCE=.\Entities\Entities.h
# End Source File
# End Group
# Begin Group "Math"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Math\Box.c
# End Source File
# Begin Source File

SOURCE=.\Math\Box.h
# End Source File
# Begin Source File

SOURCE=.\Math\CRC32.c
# End Source File
# Begin Source File

SOURCE=.\Math\CRC32.h
# End Source File
# Begin Source File

SOURCE=.\Math\ExtBox.c
# End Source File
# Begin Source File

SOURCE=.\Math\ExtBox.h
# End Source File
# Begin Source File

SOURCE=.\Math\Quatern.c
# End Source File
# Begin Source File

SOURCE=.\Math\Quatern.h
# End Source File
# Begin Source File

SOURCE=.\Math\Vec3d.c
# End Source File
# Begin Source File

SOURCE=.\Math\Vec3d.h
# End Source File
# Begin Source File

SOURCE=.\Math\XForm3d.c
# End Source File
# Begin Source File

SOURCE=.\Math\XForm3d.h
# End Source File
# End Group
# Begin Group "Support"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Support\BaseType.h
# End Source File
# Begin Source File

SOURCE=.\Support\ErrorLog.c
# End Source File
# Begin Source File

SOURCE=.\Support\ErrorLog.h
# End Source File
# Begin Source File

SOURCE=.\Support\geAssert.c
# End Source File
# Begin Source File

SOURCE=.\Support\geAssert.h
# End Source File
# Begin Source File

SOURCE=.\Support\Log.c
# End Source File
# Begin Source File

SOURCE=.\Support\Log.h
# End Source File
# Begin Source File

SOURCE=.\Support\MemPool.c
# End Source File
# Begin Source File

SOURCE=.\Support\MemPool.h
# End Source File
# Begin Source File

SOURCE=.\Support\Ram.c
# End Source File
# Begin Source File

SOURCE=.\Support\Ram.h
# End Source File
# Begin Source File

SOURCE=.\Support\RamDLL.c
# End Source File
# End Group
# Begin Group "VFile"

# PROP Default_Filter ""
# Begin Source File

SOURCE=".\VFile\DirTreeCommon.c"
# End Source File
# Begin Source File

SOURCE=".\VFile\DirTreeCommon.h"
# End Source File
# Begin Source File

SOURCE=.\VFile\DirTree.c
# End Source File
# Begin Source File

SOURCE=.\VFile\DirTree.h
# End Source File
# Begin Source File

SOURCE=.\VFile\FSDos.c
# End Source File
# Begin Source File

SOURCE=.\VFile\FSDos.h
# End Source File
# Begin Source File

SOURCE=.\VFile\FSMemory.c
# End Source File
# Begin Source File

SOURCE=.\VFile\FSMemory.h
# End Source File
# Begin Source File

SOURCE=.\VFile\FSVfs.c
# End Source File
# Begin Source File

SOURCE=.\VFile\FSVfs.h
# End Source File
# Begin Source File

SOURCE=.\VFile\VFile._h
# End Source File
# Begin Source File

SOURCE=.\VFile\VFile.c
# End Source File
# Begin Source File

SOURCE=.\VFile\VFile.h
# End Source File
# Begin Source File

SOURCE=.\VFile\VFile_Structs.h
# End Source File
# End Group
# Begin Group "World"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\World\Fog.c
# End Source File
# Begin Source File

SOURCE=.\World\Fog.h
# End Source File
# Begin Source File

SOURCE=.\World\Frustum.c
# End Source File
# Begin Source File

SOURCE=.\World\Frustum.h
# End Source File
# Begin Source File

SOURCE=.\World\GBSPFile.c
# End Source File
# Begin Source File

SOURCE=.\World\GBSPFile.h
# End Source File
# Begin Source File

SOURCE=.\World\Light.c
# End Source File
# Begin Source File

SOURCE=.\World\Light.h
# End Source File
# Begin Source File

SOURCE=.\World\Plane.c
# End Source File
# Begin Source File

SOURCE=.\World\Plane.h
# End Source File
# Begin Source File

SOURCE=.\World\Surface.c
# End Source File
# Begin Source File

SOURCE=.\World\Surface.h
# End Source File
# Begin Source File

SOURCE=.\World\Trace.c
# End Source File
# Begin Source File

SOURCE=.\World\Trace.h
# End Source File
# Begin Source File

SOURCE=.\World\User.c
# End Source File
# Begin Source File

SOURCE=.\World\User.h
# End Source File
# Begin Source File

SOURCE=.\World\Vis.c
# End Source File
# Begin Source File

SOURCE=.\World\Vis.h
# End Source File
# Begin Source File

SOURCE=.\World\WBitmap.c
# End Source File
# Begin Source File

SOURCE=.\World\WBitmap.h
# End Source File
# Begin Source File

SOURCE=.\World\World.c
# End Source File
# Begin Source File

SOURCE=.\World\World.h
# End Source File
# End Group
# Begin Group "Font"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Font\Font.c
# End Source File
# Begin Source File

SOURCE=.\Font\Font.h
# End Source File
# Begin Source File

SOURCE=.\Font\wgClip.c
# End Source File
# Begin Source File

SOURCE=.\Font\wgClip.H
# End Source File
# End Group
# Begin Group "Physics"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Physics\Matrix33.c
# End Source File
# Begin Source File

SOURCE=.\Physics\Matrix33.h
# End Source File
# Begin Source File

SOURCE=.\Physics\PhysicsJoint.c
# End Source File
# Begin Source File

SOURCE=.\Physics\PhysicsJoint.h
# End Source File
# Begin Source File

SOURCE=.\Physics\PhysicsObject.c
# End Source File
# Begin Source File

SOURCE=.\Physics\PhysicsObject.h
# End Source File
# Begin Source File

SOURCE=.\Physics\PhysicsSystem.c
# End Source File
# Begin Source File

SOURCE=.\Physics\PhysicsSystem.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\Camera.c
# End Source File
# Begin Source File

SOURCE=.\Camera.h
# End Source File
# Begin Source File

SOURCE=.\Ge.c
# End Source File
# Begin Source File

SOURCE=.\Genesis.h
# End Source File
# Begin Source File

SOURCE=.\Genesis.rc
# End Source File
# Begin Source File

SOURCE=.\GeTypes.h
# End Source File
# Begin Source File

SOURCE=.\List.c
# End Source File
# Begin Source File

SOURCE=.\List.h
# End Source File
# Begin Source File

SOURCE=.\PtrTypes.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\Sound.c
# End Source File
# Begin Source File

SOURCE=.\Sound.h
# End Source File
# Begin Source File

SOURCE=.\Sound3d.c
# End Source File
# Begin Source File

SOURCE=.\Sound3d.h
# End Source File
# Begin Source File

SOURCE=.\Sprite.c
# End Source File
# Begin Source File

SOURCE=.\Sprite.h
# End Source File
# Begin Source File

SOURCE=.\TClip.c
# End Source File
# Begin Source File

SOURCE=.\TClip.h
# End Source File
# Begin Source File

SOURCE=.\Timer.c
# End Source File
# Begin Source File

SOURCE=.\Timer.h
# End Source File
# Begin Source File

SOURCE=.\TSC.c
# End Source File
# Begin Source File

SOURCE=.\TSC.h
# End Source File
# End Group
# Begin Group "Libraries"

# PROP Default_Filter ""
# End Group
# End Target
# End Project
