# Microsoft Developer Studio Project File - Name="SoftDrv" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=SoftDrv - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "SoftDrv.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "SoftDrv.mak" CFG="SoftDrv - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "SoftDrv - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "SoftDrv - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Genesis10/Source/Engine/Drivers/SoftDrv", EVPBAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "SoftDrv - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SOFTDRV_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /X /I "..\..\.." /I "..\..\..\Support" /I "..\\" /I "..\..\..\Math" /I "..\..\..\Bitmap" /I "..\..\..\..\MsDev60\Include" /I "..\..\..\..\Sdk\Dx6SDK\Include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SOFTDRV_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 /nologo /dll /machine:I386 /nodefaultlib

!ELSEIF  "$(CFG)" == "SoftDrv - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SOFTDRV_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "..\..\.." /I "..\..\..\Support" /I "..\\" /I "..\..\..\Math" /I "..\..\..\Bitmap" /I "..\..\..\..\MsDev60\Include" /I "..\..\..\..\Sdk\Dx6SDK\Include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SOFTDRV_EXPORTS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 /nologo /dll /debug /machine:I386 /nodefaultlib /pdbtype:sept

!ENDIF 

# Begin Target

# Name "SoftDrv - Win32 Release"
# Name "SoftDrv - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\3dnowspan.h
# End Source File
# Begin Source File

SOURCE=.\amdspan.asm

!IF  "$(CFG)" == "SoftDrv - Win32 Release"

# Begin Custom Build
InputPath=.\amdspan.asm

".\amdspan.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\..\masm\bin\ml -c -I ..\..\..\..\masm\include -coff amdspan.asm

# End Custom Build

!ELSEIF  "$(CFG)" == "SoftDrv - Win32 Debug"

# Begin Custom Build
InputPath=.\amdspan.asm

".\amdspan.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\..\masm\bin\ml -c -I ..\..\..\..\masm\include -coff amdspan.asm

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dmodes.c
# End Source File
# Begin Source File

SOURCE=.\dmodes.h
# End Source File
# Begin Source File

SOURCE=.\drawspan.c
# End Source File
# Begin Source File

SOURCE=.\drawspan.h
# End Source File
# Begin Source File

SOURCE=.\register.c
# End Source File
# Begin Source File

SOURCE=.\Register.h
# End Source File
# Begin Source File

SOURCE=.\render.c
# End Source File
# Begin Source File

SOURCE=.\Render.h
# End Source File
# Begin Source File

SOURCE=.\Sal.h
# End Source File
# Begin Source File

SOURCE=.\scene.c
# End Source File
# Begin Source File

SOURCE=.\Scene.h
# End Source File
# Begin Source File

SOURCE=.\softdrv.c
# End Source File
# Begin Source File

SOURCE=.\Softdrv.h
# End Source File
# Begin Source File

SOURCE=.\span.c
# End Source File
# Begin Source File

SOURCE=.\Span.h
# End Source File
# Begin Source File

SOURCE=.\system.c
# End Source File
# Begin Source File

SOURCE=.\System.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Math\Vec3d.c
# End Source File
# Begin Source File

SOURCE=..\..\..\Math\Vec3d.h
# End Source File
# Begin Source File

SOURCE=.\W32sal.cpp
# End Source File
# Begin Source File

SOURCE=.\x86span555.c
# End Source File
# Begin Source File

SOURCE=.\x86span555.h
# End Source File
# Begin Source File

SOURCE=.\x86span565.c
# End Source File
# Begin Source File

SOURCE=.\x86span565.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Math\Xform3d.c
# End Source File
# Begin Source File

SOURCE=..\..\..\Math\Xform3d.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "Libraries"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\MSDev60\lib\Winspool.lib
# End Source File
# Begin Source File

SOURCE=..\..\..\..\MSDev60\lib\Comdlg32.lib
# End Source File
# Begin Source File

SOURCE=..\..\..\..\MSDev60\lib\Gdi32.lib
# End Source File
# Begin Source File

SOURCE=..\..\..\..\MSDev60\lib\Kernel32.lib
# End Source File
# Begin Source File

SOURCE=..\..\..\..\MSDev60\lib\Libcmt.lib

!IF  "$(CFG)" == "SoftDrv - Win32 Release"

!ELSEIF  "$(CFG)" == "SoftDrv - Win32 Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\MSDev60\lib\Libcmtd.lib

!IF  "$(CFG)" == "SoftDrv - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "SoftDrv - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\MSDev60\lib\Oldnames.lib
# End Source File
# Begin Source File

SOURCE=..\..\..\..\MSDev60\lib\Shell32.lib
# End Source File
# Begin Source File

SOURCE=..\..\..\..\MSDev60\lib\User32.lib
# End Source File
# Begin Source File

SOURCE=..\..\..\..\MSDev60\lib\Uuid.lib
# End Source File
# Begin Source File

SOURCE=..\..\..\..\MSDev60\lib\Advapi32.lib
# End Source File
# Begin Source File

SOURCE=..\..\..\..\MSDev60\lib\Winmm.lib
# End Source File
# End Group
# End Target
# End Project
