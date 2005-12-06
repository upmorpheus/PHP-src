# Microsoft Developer Studio Project File - Name="json" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=json - Win32 Debug_TS
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "json.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "json.mak" CFG="json - Win32 Debug_TS"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "json - Win32 Debug_TS" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "json - Win32 Release_TS" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "json - Win32 Debug_TS"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug_TS"
# PROP BASE Intermediate_Dir "Debug_TS"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_TS"
# PROP Intermediate_Dir "Debug_TS"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JSON_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\.." /I "..\..\main" /I "..\..\Zend" /I "..\..\TSRM" /I "json_c" /D HAVE_JSON=1 /D "ZEND_WIN32" /D "PHP_WIN32" /D ZEND_DEBUG=1 /D ZTS=1 /D COMPILE_DL_JSON=1 /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JSON_EXPORTS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x1009 /d "_DEBUG"
# ADD RSC /l 0x1009 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 iconv.lib php4ts_debug.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"..\..\Debug_TS/php_json.dll" /pdbtype:sept /libpath:"..\..\Debug_TS"

!ELSEIF  "$(CFG)" == "json - Win32 Release_TS"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release_TS"
# PROP BASE Intermediate_Dir "Release_TS"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_TS"
# PROP Intermediate_Dir "Release_TS"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JSON_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\.." /I "..\..\main" /I "..\..\Zend" /I "..\..\TSRM" /I "json_c" /D HAVE_JSON=1 /D "ZEND_WIN32" /D ZEND_DEBUG=0 /D "PHP_WIN32" /D ZTS=1 /D COMPILE_DL_JSON=1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JSON_EXPORTS" /D "HAVE_FCNTL_H" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x1009 /d "NDEBUG"
# ADD RSC /l 0x1009 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 iconv.lib php4ts.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"..\..\Release_TS/php_json.dll" /libpath:"..\..\Release_TS"

!ENDIF 

# Begin Target

# Name "json - Win32 Debug_TS"
# Name "json - Win32 Release_TS"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=".\json.c"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\php_json.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "json_c"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\json_c\arraylist.c
# End Source File
# Begin Source File

SOURCE=.\json_c\arraylist.h
# End Source File
# Begin Source File

SOURCE=.\json_c\bits.h
# End Source File
# Begin Source File

SOURCE=.\json_c\ConvertUTF.c
# End Source File
# Begin Source File

SOURCE=.\json_c\ConvertUTF.h
# End Source File
# Begin Source File

SOURCE=.\json_c\debug.c
# End Source File
# Begin Source File

SOURCE=.\json_c\debug.h
# End Source File
# Begin Source File

SOURCE=.\json_c\json.h
# End Source File
# Begin Source File

SOURCE=.\json_c\json_object.c
# End Source File
# Begin Source File

SOURCE=.\json_c\json_object.h
# End Source File
# Begin Source File

SOURCE=.\json_c\json_object_private.h
# End Source File
# Begin Source File

SOURCE=.\json_c\json_tokener.c
# End Source File
# Begin Source File

SOURCE=.\json_c\json_tokener.h
# End Source File
# Begin Source File

SOURCE=.\json_c\json_util.c
# End Source File
# Begin Source File

SOURCE=.\json_c\json_util.h
# End Source File
# Begin Source File

SOURCE=.\json_c\linkhash.c
# End Source File
# Begin Source File

SOURCE=.\json_c\linkhash.h
# End Source File
# Begin Source File

SOURCE=.\json_c\ossupport.c
# End Source File
# Begin Source File

SOURCE=.\json_c\ossupport.h
# End Source File
# Begin Source File

SOURCE=.\json_c\printbuf.c
# End Source File
# Begin Source File

SOURCE=.\json_c\printbuf.h
# End Source File
# End Group
# End Target
# End Project
