# Microsoft Developer Studio Project File - Name="D3DDrv7x" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=D3DDrv7x - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "D3DDrv7x.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "D3DDrv7x.mak" CFG="D3DDrv7x - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "D3DDrv7x - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "D3DDrv7x - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Genesis10/Source/Engine/Drivers/D3DDrv7x", DVPBAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "D3DDrv7x - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "D3DDRV7x_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\..\.." /I "..\..\..\Math" /I "..\\" /I "..\D3DDrv7x" /I "..\..\..\Support" /I "..\..\..\Bitmap" /I ".." /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "D3DDRV_EXPORTS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
# SUBTRACT RSC /x
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 Wininet.lib comdlg32.lib gdi32.lib kernel32.lib libcmt.lib libcmtd.lib oldnames.lib shell32.lib user32.lib uuid.lib advapi32.lib dxguid.lib ddraw.lib d3dim.lib /nologo /dll /machine:I386 /out:"Release/D3D7xDrv.dll"
# SUBTRACT LINK32 /nodefaultlib

!ELSEIF  "$(CFG)" == "D3DDrv7x - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "D3DDRV7x_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /GX /Zi /Od /I "..\..\.." /I "..\..\..\Math" /I "..\\" /I "..\D3DDrv7x" /I "..\..\..\Support" /I "..\..\..\Bitmap" /I ".." /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "D3DDRV7x_EXPORTS" /FR /YX /FD /GZ /c
# SUBTRACT CPP /X
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
# SUBTRACT RSC /x
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 Wininet.lib comdlg32.lib gdi32.lib kernel32.lib libcmt.lib libcmtd.lib oldnames.lib shell32.lib user32.lib uuid.lib advapi32.lib dxguid.lib ddraw.lib d3dim.lib /nologo /dll /debug /machine:I386 /out:"Debug/D3D7xDrv.dll" /pdbtype:sept
# SUBTRACT LINK32 /nodefaultlib

!ENDIF 

# Begin Target

# Name "D3DDrv7x - Win32 Release"
# Name "D3DDrv7x - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\D3d_err.cpp
# End Source File
# Begin Source File

SOURCE=.\D3d_err.h
# End Source File
# Begin Source File

SOURCE=.\D3d_fx.cpp
# End Source File
# Begin Source File

SOURCE=.\D3d_fx.h
# End Source File
# Begin Source File

SOURCE=.\D3d_main.cpp
# End Source File
# Begin Source File

SOURCE=.\D3d_main.h
# End Source File
# Begin Source File

SOURCE=.\D3dcache.cpp
# End Source File
# Begin Source File

SOURCE=.\D3dcache.h
# End Source File
# Begin Source File

SOURCE=.\D3ddrv7x.cpp
# End Source File
# Begin Source File

SOURCE=.\D3ddrv7x.h
# End Source File
# Begin Source File

SOURCE=..\Dcommon.h
# End Source File
# Begin Source File

SOURCE=.\DDMemMgr.c
# End Source File
# Begin Source File

SOURCE=.\DDMemMgr.h
# End Source File
# Begin Source File

SOURCE=.\Gspan.cpp
# End Source File
# Begin Source File

SOURCE=.\Gspan.h
# End Source File
# Begin Source File

SOURCE=.\Pcache.cpp
# End Source File
# Begin Source File

SOURCE=.\Pcache.h
# End Source File
# Begin Source File

SOURCE=.\Render.cpp
# End Source File
# Begin Source File

SOURCE=.\Render.h
# End Source File
# Begin Source File

SOURCE=.\Scene.cpp
# End Source File
# Begin Source File

SOURCE=.\Scene.h
# End Source File
# Begin Source File

SOURCE=.\THandle.cpp
# End Source File
# Begin Source File

SOURCE=.\THandle.h
# End Source File
# Begin Source File

SOURCE=.\tpage.cpp
# End Source File
# Begin Source File

SOURCE=.\TPage.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
