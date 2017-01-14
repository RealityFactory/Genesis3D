# Microsoft Developer Studio Generated NMAKE File, Based on SoftDrv2.dsp
!IF "$(CFG)" == ""
CFG=SoftDrv2 - Win32 Debug
!MESSAGE No configuration specified. Defaulting to SoftDrv2 - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "SoftDrv2 - Win32 Release" && "$(CFG)" != "SoftDrv2 - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
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

!IF  "$(CFG)" == "SoftDrv2 - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\Softdrv.dll"


CLEAN :
	-@erase "$(INTDIR)\CPUInfo.obj"
	-@erase "$(INTDIR)\DDRAWDisplay.obj"
	-@erase "$(INTDIR)\DIBDisplay.obj"
	-@erase "$(INTDIR)\display.obj"
	-@erase "$(INTDIR)\DisplayModeInfo.obj"
	-@erase "$(INTDIR)\DrawDecal.obj"
	-@erase "$(INTDIR)\Ram.obj"
	-@erase "$(INTDIR)\softdrv.obj"
	-@erase "$(INTDIR)\span.obj"
	-@erase "$(INTDIR)\SpanBuffer.obj"
	-@erase "$(INTDIR)\SWTHandle.obj"
	-@erase "$(INTDIR)\TRaster.obj"
	-@erase "$(INTDIR)\Triangle.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\Softdrv.dll"
	-@erase "$(OUTDIR)\Softdrv.exp"
	-@erase "$(OUTDIR)\Softdrv.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O2 /X /I "..\\" /I "..\..\..\\" /I "..\..\..\support" /I "..\..\..\math" /I "..\..\..\bitmap" /I "..\..\..\..\msdev60\include" /I "..\..\..\..\sdk\dx6sdk\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SOFTDRV2_EXPORTS" /Fp"$(INTDIR)\SoftDrv2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\SoftDrv2.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /dll /incremental:no /pdb:"$(OUTDIR)\Softdrv.pdb" /machine:I386 /nodefaultlib /out:"$(OUTDIR)\Softdrv.dll" /implib:"$(OUTDIR)\Softdrv.lib" 
LINK32_OBJS= \
	"$(INTDIR)\CPUInfo.obj" \
	"$(INTDIR)\DDRAWDisplay.obj" \
	"$(INTDIR)\DIBDisplay.obj" \
	"$(INTDIR)\display.obj" \
	"$(INTDIR)\DisplayModeInfo.obj" \
	"$(INTDIR)\DrawDecal.obj" \
	"$(INTDIR)\Ram.obj" \
	"$(INTDIR)\softdrv.obj" \
	"$(INTDIR)\span.obj" \
	"$(INTDIR)\SpanBuffer.obj" \
	"$(INTDIR)\SWTHandle.obj" \
	"$(INTDIR)\TRaster.obj" \
	"$(INTDIR)\Triangle.obj" \
	"..\..\..\..\MSDev60\lib\Winmm.lib" \
	"..\..\..\..\MSDev60\lib\Comdlg32.lib" \
	"..\..\..\..\MSDev60\lib\Gdi32.lib" \
	"..\..\..\..\MSDev60\lib\Kernel32.lib" \
	"..\..\..\..\MSDev60\lib\Libcmt.lib" \
	"..\..\..\..\MSDev60\lib\Oldnames.lib" \
	"..\..\..\..\MSDev60\lib\Shell32.lib" \
	"..\..\..\..\MSDev60\lib\User32.lib" \
	"..\..\..\..\MSDev60\lib\Uuid.lib" \
	"..\..\..\..\MSDev60\lib\Advapi32.lib" \
	"..\..\..\..\MSDev60\lib\Winspool.lib" \
	"..\..\..\..\Sdk\Dx6sdk\Lib\dxguid.lib"

"$(OUTDIR)\Softdrv.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "SoftDrv2 - Win32 Debug"

OUTDIR=.
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.
# End Custom Macros

ALL : "$(OUTDIR)\Debug\Softdrv.dll"


CLEAN :
	-@erase "$(INTDIR)\CPUInfo.obj"
	-@erase "$(INTDIR)\DDRAWDisplay.obj"
	-@erase "$(INTDIR)\DIBDisplay.obj"
	-@erase "$(INTDIR)\display.obj"
	-@erase "$(INTDIR)\DisplayModeInfo.obj"
	-@erase "$(INTDIR)\DrawDecal.obj"
	-@erase "$(INTDIR)\Ram.obj"
	-@erase "$(INTDIR)\softdrv.obj"
	-@erase "$(INTDIR)\span.obj"
	-@erase "$(INTDIR)\SpanBuffer.obj"
	-@erase "$(INTDIR)\SWTHandle.obj"
	-@erase "$(INTDIR)\TRaster.obj"
	-@erase "$(INTDIR)\Triangle.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\Debug\Softdrv.dll"
	-@erase "$(OUTDIR)\Debug\Softdrv.ilk"
	-@erase "$(OUTDIR)\Softdrv.exp"
	-@erase "$(OUTDIR)\Softdrv.lib"
	-@erase "$(OUTDIR)\Softdrv.pdb"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "..\\" /I "..\..\..\\" /I "..\..\..\support" /I "..\..\..\math" /I "..\..\..\bitmap" /I "..\..\..\..\msdev60\include" /I "..\..\..\..\sdk\dx6sdk\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SOFTDRV2_EXPORTS" /Fp"$(INTDIR)\SoftDrv2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\SoftDrv2.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /dll /incremental:yes /pdb:"$(OUTDIR)\Softdrv.pdb" /debug /machine:I386 /nodefaultlib /out:"$(OUTDIR)\Debug\Softdrv.dll" /implib:"$(OUTDIR)\Softdrv.lib" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\CPUInfo.obj" \
	"$(INTDIR)\DDRAWDisplay.obj" \
	"$(INTDIR)\DIBDisplay.obj" \
	"$(INTDIR)\display.obj" \
	"$(INTDIR)\DisplayModeInfo.obj" \
	"$(INTDIR)\DrawDecal.obj" \
	"$(INTDIR)\Ram.obj" \
	"$(INTDIR)\softdrv.obj" \
	"$(INTDIR)\span.obj" \
	"$(INTDIR)\SpanBuffer.obj" \
	"$(INTDIR)\SWTHandle.obj" \
	"$(INTDIR)\TRaster.obj" \
	"$(INTDIR)\Triangle.obj" \
	"..\..\..\..\MSDev60\lib\Winmm.lib" \
	"..\..\..\..\MSDev60\lib\Comdlg32.lib" \
	"..\..\..\..\MSDev60\lib\Gdi32.lib" \
	"..\..\..\..\MSDev60\lib\Kernel32.lib" \
	"..\..\..\..\MSDev60\lib\Libcmtd.lib" \
	"..\..\..\..\MSDev60\lib\Oldnames.lib" \
	"..\..\..\..\MSDev60\lib\Shell32.lib" \
	"..\..\..\..\MSDev60\lib\User32.lib" \
	"..\..\..\..\MSDev60\lib\Uuid.lib" \
	"..\..\..\..\MSDev60\lib\Advapi32.lib" \
	"..\..\..\..\MSDev60\lib\Winspool.lib" \
	"..\..\..\..\Sdk\Dx6sdk\Lib\dxguid.lib"

"$(OUTDIR)\Debug\Softdrv.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
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
!IF EXISTS("SoftDrv2.dep")
!INCLUDE "SoftDrv2.dep"
!ELSE 
!MESSAGE Warning: cannot find "SoftDrv2.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "SoftDrv2 - Win32 Release" || "$(CFG)" == "SoftDrv2 - Win32 Debug"
SOURCE=.\CPUInfo.c

"$(INTDIR)\CPUInfo.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\DDRAWDisplay.c

"$(INTDIR)\DDRAWDisplay.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\DIBDisplay.c

"$(INTDIR)\DIBDisplay.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\display.c

"$(INTDIR)\display.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\DisplayModeInfo.c

"$(INTDIR)\DisplayModeInfo.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\DrawDecal.c

"$(INTDIR)\DrawDecal.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=..\..\..\Support\Ram.c

"$(INTDIR)\Ram.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\softdrv.c

"$(INTDIR)\softdrv.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\span.c

"$(INTDIR)\span.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\SpanBuffer.c

"$(INTDIR)\SpanBuffer.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\SWTHandle.c

"$(INTDIR)\SWTHandle.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\TRaster.c

"$(INTDIR)\TRaster.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Triangle.c

"$(INTDIR)\Triangle.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 

