# Microsoft Developer Studio Project File - Name="Explorer" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=Explorer - Win32 Debug
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "Explorer.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausführen von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "Explorer.mak" CFG="Explorer - Win32 Debug"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "Explorer - Win32 Release" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Explorer - Win32 Debug" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Explorer - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Explorer___Win32_Release"
# PROP BASE Intermediate_Dir "Explorer___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "EXPLORER_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\..\src\NewDlg\\" /I "..\..\src\PropDlg\\" /I "..\..\src\Toolbar\\" /I "..\..\src\FileDlg\\" /I "..\..\src\FileList\\" /I "." /I "..\..\src\\" /I "..\..\src\MISC\\" /I "..\..\src\HelpDlg\\" /I "..\..\src\OptionDlg\\" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "EXPLORER_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib shlwapi.lib comctl32.lib /nologo /dll /machine:I386 /out:"C:/Programme/Notepad++/plugins/Explorer.dll"

!ELSEIF  "$(CFG)" == "Explorer - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Explorer___Win32_Debug"
# PROP BASE Intermediate_Dir "Explorer___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DEBUG"
# PROP Intermediate_Dir "DEBUG"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "EXPLORER_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "." /I ".\src\\" /I ".\src\MISC\\" /I ".\src\HelpDlg\\" /I ".\src\OptionDlg\\" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "EXPLORER_EXPORTS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib shlwapi.lib /nologo /dll /debug /machine:I386 /out:"C:/Programme/Notepad++/plugins/Explorer.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "Explorer - Win32 Release"
# Name "Explorer - Win32 Debug"
# Begin Group "Quellcodedateien"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\src\FileList\ComboOrgi.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\MISC\ContextMenu.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\ExplorerDialog.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\FavesDialog.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\FileDlg\FileDlg.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\FileList\FileList.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\HelpDlg\HelpDialog.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\Toolbar\ImageListSet.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\NewDlg\NewDlg.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\OptionDlg\OptionDialog.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\PluginInterface.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\PropDlg\PropDlg.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\StaticDialog.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\stdafx.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\SysMsg.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\Toolbar\ToolBar.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\FileList\ToolTip.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\MISC\TreeHelperClass.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\HelpDlg\URLCtrl.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\MISC\winVersion.cpp
# End Source File
# End Group
# Begin Group "Header-Dateien"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\src\FileList\ComboOrgi.h
# End Source File
# Begin Source File

SOURCE=..\..\src\MISC\ContextMenu.h
# End Source File
# Begin Source File

SOURCE=..\..\src\Docking.h
# End Source File
# Begin Source File

SOURCE=..\..\src\DockingDlgInterface.h
# End Source File
# Begin Source File

SOURCE=..\..\src\ExplorerDialog.h
# End Source File
# Begin Source File

SOURCE=..\..\src\ExplorerResource.h
# End Source File
# Begin Source File

SOURCE=..\..\src\FavesDialog.h
# End Source File
# Begin Source File

SOURCE=..\..\src\FileDlg\FileDlg.h
# End Source File
# Begin Source File

SOURCE=..\..\src\FileList\FileList.h
# End Source File
# Begin Source File

SOURCE=..\..\src\HelpDlg\HelpDialog.h
# End Source File
# Begin Source File

SOURCE=..\..\src\Toolbar\ImageListSet.h
# End Source File
# Begin Source File

SOURCE=..\..\src\NewDlg\NewDlg.h
# End Source File
# Begin Source File

SOURCE=..\..\src\OptionDlg\OptionDialog.h
# End Source File
# Begin Source File

SOURCE=..\..\src\PluginInterface.h
# End Source File
# Begin Source File

SOURCE=..\..\src\PropDlg\PropDlg.h
# End Source File
# Begin Source File

SOURCE=..\..\src\rcNotepad.h
# End Source File
# Begin Source File

SOURCE=..\..\src\resource.h
# End Source File
# Begin Source File

SOURCE=..\..\src\Scintilla.h
# End Source File
# Begin Source File

SOURCE=..\..\src\StaticDialog.h
# End Source File
# Begin Source File

SOURCE=..\..\src\stdafx.h
# End Source File
# Begin Source File

SOURCE=..\..\src\SysMsg.h
# End Source File
# Begin Source File

SOURCE=..\..\src\Toolbar\ToolBar.h
# End Source File
# Begin Source File

SOURCE=..\..\src\FileList\ToolTip.h
# End Source File
# Begin Source File

SOURCE=..\..\src\MISC\TreeHelperClass.h
# End Source File
# Begin Source File

SOURCE=..\..\src\HelpDlg\URLCtrl.h
# End Source File
# Begin Source File

SOURCE=..\..\src\Window.h
# End Source File
# Begin Source File

SOURCE=..\..\src\MISC\winVersion.h
# End Source File
# End Group
# Begin Group "Ressourcendateien"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\..\res\ColumnHeaderEndHighXP.bmp
# End Source File
# Begin Source File

SOURCE=..\..\res\ColumnHeaderEndNorm.bmp
# End Source File
# Begin Source File

SOURCE=..\..\res\ColumnHeaderEndNormXP.bmp
# End Source File
# Begin Source File

SOURCE=..\..\res\ColumnHeaderSortDownHigh.bmp
# End Source File
# Begin Source File

SOURCE=..\..\res\ColumnHeaderSortDownHighXP.bmp
# End Source File
# Begin Source File

SOURCE=..\..\res\ColumnHeaderSortDownNorm.bmp
# End Source File
# Begin Source File

SOURCE=..\..\res\ColumnHeaderSortDownNormXP.bmp
# End Source File
# Begin Source File

SOURCE=..\..\res\ColumnHeaderSortUpHighXP.bmp
# End Source File
# Begin Source File

SOURCE=..\..\res\ColumnHeaderSortUpNorm.bmp
# End Source File
# Begin Source File

SOURCE=..\..\res\ColumnHeaderSortUpNormXP.bmp
# End Source File
# Begin Source File

SOURCE=..\..\res\ColumnHeaderSpanHighXP.bmp
# End Source File
# Begin Source File

SOURCE=..\..\res\ColumnHeaderSpanNorm.bmp
# End Source File
# Begin Source File

SOURCE=..\..\res\ColumnHeaderSpanNormXP.bmp
# End Source File
# Begin Source File

SOURCE=..\..\res\ColumnHeaderStartHighXP.bmp
# End Source File
# Begin Source File

SOURCE=..\..\res\ColumnHeaderStartNorm.bmp
# End Source File
# Begin Source File

SOURCE=..\..\res\explore.bmp
# End Source File
# Begin Source File

SOURCE=..\..\res\explore.ico
# End Source File
# Begin Source File

SOURCE=..\..\src\ExplorerDialog.rc
# End Source File
# Begin Source File

SOURCE=..\..\res\file.ico
# End Source File
# Begin Source File

SOURCE=..\..\res\findFile.bmp
# End Source File
# Begin Source File

SOURCE=..\..\res\folder.ico
# End Source File
# Begin Source File

SOURCE=..\..\res\folderGo.bmp
# End Source File
# Begin Source File

SOURCE=..\..\res\folderUser.bmp
# End Source File
# Begin Source File

SOURCE=..\..\res\group.ico
# End Source File
# Begin Source File

SOURCE=..\..\res\Heart.bmp
# End Source File
# Begin Source File

SOURCE=..\..\res\Heart.ico
# End Source File
# Begin Source File

SOURCE=..\..\res\leftright.cur
# End Source File
# Begin Source File

SOURCE=..\..\res\linkDelete.bmp
# End Source File
# Begin Source File

SOURCE=..\..\res\linkEdit.bmp
# End Source File
# Begin Source File

SOURCE=..\..\res\linkNew.bmp
# End Source File
# Begin Source File

SOURCE=..\..\res\linkNewFile.bmp
# End Source File
# Begin Source File

SOURCE=..\..\res\linkNewFolder.bmp
# End Source File
# Begin Source File

SOURCE=..\..\res\newFile.bmp
# End Source File
# Begin Source File

SOURCE=..\..\res\newFolder.bmp
# End Source File
# Begin Source File

SOURCE=..\..\res\parent.ico
# End Source File
# Begin Source File

SOURCE=..\..\res\readme.txt
# End Source File
# Begin Source File

SOURCE=..\..\res\redo.bmp
# End Source File
# Begin Source File

SOURCE=..\..\res\session.ico
# End Source File
# Begin Source File

SOURCE=..\..\res\trash.bmp
# End Source File
# Begin Source File

SOURCE=..\..\res\undo.bmp
# End Source File
# Begin Source File

SOURCE=..\..\res\update.bmp
# End Source File
# Begin Source File

SOURCE=..\..\res\updown.cur
# End Source File
# Begin Source File

SOURCE=..\..\res\web.ico
# End Source File
# End Group
# End Target
# End Project
