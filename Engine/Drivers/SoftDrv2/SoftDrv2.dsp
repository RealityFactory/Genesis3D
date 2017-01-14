# Microsoft Developer Studio Project File - Name="SoftDrv2" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=SoftDrv2 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "SoftDrv2.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "SoftDrv2.mak" CFG="SoftDrv2 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "SoftDrv2 - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "SoftDrv2 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Genesis10/Source/Engine/Drivers/SoftDrv2", QADCAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "SoftDrv2 - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SOFTDRV2_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /X /I "..\\" /I "..\..\..\\" /I "..\..\..\support" /I "..\..\..\math" /I "..\..\..\bitmap" /I "..\..\..\..\msdev60\include" /I "..\..\..\..\sdk\dx6sdk\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SOFTDRV2_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /x /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /dll /machine:I386 /nodefaultlib /out:".\Release\Softdrv.dll"

!ELSEIF  "$(CFG)" == "SoftDrv2 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ""
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SOFTDRV2_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "..\\" /I "..\..\..\\" /I "..\..\..\support" /I "..\..\..\math" /I "..\..\..\bitmap" /I "..\..\..\..\msdev60\include" /I "..\..\..\..\sdk\dx6sdk\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SOFTDRV2_EXPORTS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /x /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /dll /debug /machine:I386 /nodefaultlib /out:".\Debug\Softdrv.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "SoftDrv2 - Win32 Release"
# Name "SoftDrv2 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\CPUInfo.c
# End Source File
# Begin Source File

SOURCE=.\DDRAWDisplay.c
# End Source File
# Begin Source File

SOURCE=.\DIBDisplay.c
# End Source File
# Begin Source File

SOURCE=.\display.c
# End Source File
# Begin Source File

SOURCE=.\DisplayModeInfo.c
# End Source File
# Begin Source File

SOURCE=.\DrawDecal.c
# End Source File
# Begin Source File

SOURCE=..\..\..\Support\Ram.c
# End Source File
# Begin Source File

SOURCE=.\softdrv.c
# End Source File
# Begin Source File

SOURCE=.\span.c
# End Source File
# Begin Source File

SOURCE=.\SpanBuffer.c
# End Source File
# Begin Source File

SOURCE=.\SWTHandle.c
# End Source File
# Begin Source File

SOURCE=.\TRaster.c
# End Source File
# Begin Source File

SOURCE=.\Triangle.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\CPUInfo.h
# End Source File
# Begin Source File

SOURCE=.\DDRAWDisplay.h
# End Source File
# Begin Source File

SOURCE=.\DIBDisplay.h
# End Source File
# Begin Source File

SOURCE=.\display.h
# End Source File
# Begin Source File

SOURCE=.\DisplayModeInfo.h
# End Source File
# Begin Source File

SOURCE=.\DrawDecal.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Support\Ram.h
# End Source File
# Begin Source File

SOURCE=.\rop.h
# End Source File
# Begin Source File

SOURCE=.\Softdrv.h
# End Source File
# Begin Source File

SOURCE=.\Span.h
# End Source File
# Begin Source File

SOURCE=.\Span_AffineLoop.h
# End Source File
# Begin Source File

SOURCE=.\Span_Factory.h
# End Source File
# Begin Source File

SOURCE=.\SpanBuffer.h
# End Source File
# Begin Source File

SOURCE=.\SpanEdges_Factory.h
# End Source File
# Begin Source File

SOURCE=.\SWTHandle.h
# End Source File
# Begin Source File

SOURCE=.\traster.h
# End Source File
# Begin Source File

SOURCE=.\triangle.h
# End Source File
# End Group
# Begin Group "Libraries"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\MSDev60\lib\Winmm.lib
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

!IF  "$(CFG)" == "SoftDrv2 - Win32 Release"

!ELSEIF  "$(CFG)" == "SoftDrv2 - Win32 Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\MSDev60\lib\Libcmtd.lib

!IF  "$(CFG)" == "SoftDrv2 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "SoftDrv2 - Win32 Debug"

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

SOURCE=..\..\..\..\MSDev60\lib\Winspool.lib
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Sdk\Dx6sdk\Lib\dxguid.lib
# End Source File
# End Group
# Begin Source File

SOURCE=.\SoftDrv2.mak
# End Source File
# End Target
# End Project
