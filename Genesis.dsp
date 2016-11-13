# Microsoft Developer Studio Project File - Name="Genesis" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=Genesis - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Genesis.mak".
!MESSAGE 
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

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Genesis10/Source", MPPBAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Genesis - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /G5 /MT /W3 /GX /Ot /Ow /Og /Oi /Op /Ob2 /I "..\Genesis3D" /I "World" /I "Engine" /I "Engine\Drivers" /I "Actor" /I "BSP" /I "Math" /I "Entities" /I "Support" /I "Physics" /I "VFile" /I "Bitmap" /I "Bitmap\Compression" /I "..\DX8Lib\include" /I "f:\mssdk7a\include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /FR /Fd"Release/genesis.pdb" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /fo"Lib/genesis.res" /d "NDEBUG"
# SUBTRACT RSC /x
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Lib\Genesis.lib"

!ELSEIF  "$(CFG)" == "Genesis - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /G5 /MTd /W3 /Gm /GX /ZI /Od /I "..\G3D" /I "World" /I "Engine" /I "Engine\Drivers" /I "Actor" /I "BSP" /I "Math" /I "Entities" /I "Support" /I "Physics" /I "VFile" /I "Bitmap" /I "Bitmap\Compression" /I "f:\mssdk7a\include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /Fd"Debug/genesis.pdb" /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
# SUBTRACT RSC /x
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# SUBTRACT BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Lib\GenesisD.lib"

!ENDIF 

# Begin Target

# Name "Genesis - Win32 Release"
# Name "Genesis - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
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

SOURCE=.\World\World.c
# End Source File
# Begin Source File

SOURCE=.\World\World.h
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
# Begin Group "Actor"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Actor\Actor.c
# End Source File
# Begin Source File

SOURCE=.\Actor\Actor.h
# End Source File
# Begin Source File

SOURCE=.\Actor\Body._h
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

SOURCE=.\World\WBitmap.h
# End Source File
# Begin Source File

SOURCE=.\Actor\XFArray.c
# End Source File
# Begin Source File

SOURCE=.\Actor\XFArray.h
# End Source File
# End Group
# Begin Group "BSP"

# PROP Default_Filter ""
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
# Begin Group "Entities"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Entities\Entities.c
# End Source File
# Begin Source File

SOURCE=.\Entities\Entities.h
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
# Begin Source File

SOURCE=.\Camera.c
# End Source File
# Begin Source File

SOURCE=.\Camera.h
# End Source File
# Begin Source File

SOURCE=.\CSNetMgr.c
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

SOURCE=.\NetPlay.c
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
# Begin Source File

SOURCE=.\CSNetMgr.h
# End Source File
# Begin Source File

SOURCE=.\NetPlay.h
# End Source File
# End Group
# End Target
# End Project
