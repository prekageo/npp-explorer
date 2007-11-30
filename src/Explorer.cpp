/*
This file is part of Explorer Plugin for Notepad++
Copyright (C)2006 Jens Lorenz <jens.plugin.npp@gmx.de>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/


/* include files */
#include "stdafx.h"
#include "Explorer.h"
#include "ExplorerDialog.h"
#include "FavesDialog.h"
#include "OptionDialog.h"
#include "HelpDialog.h"
#include "ToolTip.h"
#include "SysMsg.h"
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <shellapi.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <dbt.h>
#include <atlbase.h>

#define SHGFI_OVERLAYINDEX 0x000000040


CONST INT	nbFunc	= 6;



/* information for notepad */
CONST TCHAR  PLUGIN_NAME[] = "&Explorer";

TCHAR		configPath[MAX_PATH];
TCHAR		iniFilePath[MAX_PATH];

/* ini file sections */
CONST TCHAR WindowData[]		= "WindowData";
CONST TCHAR Explorer[]			= "Explorer";
CONST TCHAR Faves[]				= "Faves";



/* section Explorer */
CONST TCHAR LastPath[]			= "LastPath";
CONST TCHAR SplitterPos[]		= "SplitterPos";
CONST TCHAR SplitterPosHor[]	= "SplitterPosHor";
CONST TCHAR SortAsc[]			= "SortAsc";
CONST TCHAR SortPos[]			= "SortPos";
CONST TCHAR ColPosName[]		= "ColPosName";
CONST TCHAR ColPosExt[]			= "ColPosExt";
CONST TCHAR ColPosSize[]		= "ColPosSize";
CONST TCHAR ColPosDate[]		= "ColPosDate";
CONST TCHAR ShowHiddenData[]	= "ShowHiddenData";
CONST TCHAR ShowBraces[]		= "ShowBraces";
CONST TCHAR ShowLongInfo[]		= "ShowLongInfo";
CONST TCHAR AddExtToName[]		= "AddExtToName";
CONST TCHAR AutoUpdate[]		= "AutoUpdate";
CONST TCHAR SizeFormat[]		= "SizeFormat";
CONST TCHAR DateFormat[]		= "DateFormat";
CONST TCHAR FilterHistory[]		= "FilterHistory";
CONST TCHAR LastFilter[]		= "LastFilter";
CONST TCHAR TimeOut[]			= "TimeOut";
CONST TCHAR UseSystemIcons[]	= "UseSystemIcons";


/* global values */
HMODULE				hShell32;
NppData				nppData;
HANDLE				g_hModule;
HWND				g_HSource;
TCHAR				g_currentFile[MAX_PATH];
FuncItem			funcItem[nbFunc];
toolbarIcons		g_TBExplorer;
toolbarIcons		g_TBFaves;

/* create classes */
ExplorerDialog		explorerDlg;
FavesDialog			favesDlg;
OptionDlg			optionDlg;
HelpDlg				helpDlg;

/* global explorer params */
tExProp				exProp;

/* global favorite params */
TCHAR				szLastElement[MAX_PATH];

/* get system information */
BOOL				isNotepadCreated	= FALSE;

/* section Faves */
CONST TCHAR			LastElement[]		= "LastElement";

/* for subclassing */
WNDPROC				wndProcNotepad		= NULL;

/* win version */
winVer				gWinVersion			= WV_UNKNOWN;

/* own image list variables */
vector<tDrvMap>		gvDrvMap;
HIMAGELIST			ghImgList			= NULL;

/* current open docs */
vector<string>		g_vStrCurrentFiles;



BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  reasonForCall, 
                       LPVOID lpReserved )
{
	g_hModule = hModule;

    switch (reasonForCall)
    {
		case DLL_PROCESS_ATTACH:
		{
			/* Set function pointers */
			funcItem[0]._pFunc = toggleExplorerDialog;
			funcItem[1]._pFunc = toggleFavesDialog;
			funcItem[2]._pFunc = toggleFavesDialog;
			funcItem[3]._pFunc = openOptionDlg;
			funcItem[4]._pFunc = openOptionDlg;
			funcItem[5]._pFunc = openHelpDlg;
			
			/* Fill menu names */
			strcpy(funcItem[0]._itemName, "&Explorer...");
			strcpy(funcItem[1]._itemName, "&Favorites...");
			strcpy(funcItem[2]._itemName, "----------------");
			strcpy(funcItem[3]._itemName, "Explorer &Options...");
			strcpy(funcItem[4]._itemName, "----------------");
			strcpy(funcItem[5]._itemName, "&Help...");

			/* Set shortcuts */
			funcItem[0]._pShKey = new ShortcutKey;
			funcItem[0]._pShKey->_isAlt		= true;
			funcItem[0]._pShKey->_isCtrl	= true;
			funcItem[0]._pShKey->_isShift	= true;
			funcItem[0]._pShKey->_key		= 0x45;
			funcItem[1]._pShKey = new ShortcutKey;
			funcItem[1]._pShKey->_isAlt		= true;
			funcItem[1]._pShKey->_isCtrl	= true;
			funcItem[1]._pShKey->_isShift	= true;
			funcItem[1]._pShKey->_key		= 0x56;
			funcItem[2]._pShKey				= NULL;
			funcItem[3]._pShKey				= NULL;
			funcItem[4]._pShKey				= NULL;
			funcItem[5]._pShKey				= NULL;

			/* set image list and icon */
			ghImgList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 6, 30);
			ImageList_AddIcon(ghImgList, ::LoadIcon((HINSTANCE)g_hModule, MAKEINTRESOURCE(IDI_FOLDER)));
			ImageList_AddIcon(ghImgList, ::LoadIcon((HINSTANCE)g_hModule, MAKEINTRESOURCE(IDI_FILE)));
			ImageList_AddIcon(ghImgList, ::LoadIcon((HINSTANCE)g_hModule, MAKEINTRESOURCE(IDI_WEB)));
			ImageList_AddIcon(ghImgList, ::LoadIcon((HINSTANCE)g_hModule, MAKEINTRESOURCE(IDI_SESSION)));
			ImageList_AddIcon(ghImgList, ::LoadIcon((HINSTANCE)g_hModule, MAKEINTRESOURCE(IDI_GROUP)));
			ImageList_AddIcon(ghImgList, ::LoadIcon((HINSTANCE)g_hModule, MAKEINTRESOURCE(IDI_PARENTFOLDER)));
			break;
		}	
		case DLL_PROCESS_DETACH:
		{
			/* save settings */
			saveSettings();

			/* destroy image list */
			ImageList_Destroy(ghImgList);

			/* destroy dialogs */
			explorerDlg.destroy();
			favesDlg.destroy();
			optionDlg.destroy();
			helpDlg.destroy();

			/* Remove subclaasing */
			SetWindowLong(nppData._nppHandle, GWL_WNDPROC, (LONG)wndProcNotepad);

			FreeLibrary(hShell32);
	
			delete funcItem[0]._pShKey;
			delete funcItem[1]._pShKey;

			if (g_TBExplorer.hToolbarBmp)
				::DeleteObject(g_TBExplorer.hToolbarBmp);
			if (g_TBExplorer.hToolbarIcon)
				::DestroyIcon(g_TBExplorer.hToolbarIcon);

			if (g_TBFaves.hToolbarBmp)
				::DeleteObject(g_TBFaves.hToolbarBmp);
			if (g_TBFaves.hToolbarIcon)
				::DestroyIcon(g_TBFaves.hToolbarIcon);

			break;
		}
		case DLL_THREAD_ATTACH:
			break;
			
		case DLL_THREAD_DETACH:
			break;
    }

    return TRUE;
}

extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData)
{
	/* stores notepad data */
	nppData = notpadPlusData;

	/* get windows version */
	gWinVersion  = (winVer)::SendMessage(nppData._nppHandle, NPPM_GETWINDOWSVERSION, 0, 0);

	/* load data */
	loadSettings();

	/* initial dialogs */
	explorerDlg.init((HINSTANCE)g_hModule, nppData, &exProp);
	favesDlg.init((HINSTANCE)g_hModule, nppData, szLastElement, &exProp);
	optionDlg.init((HINSTANCE)g_hModule, nppData);
	helpDlg.init((HINSTANCE)g_hModule, nppData);

	/* Subclassing for Notepad */
	wndProcNotepad = (WNDPROC)SetWindowLong(nppData._nppHandle, GWL_WNDPROC, (LPARAM)SubWndProcNotepad);
}

extern "C" __declspec(dllexport) LPCSTR getName()
{
	return PLUGIN_NAME;
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(INT *nbF)
{
	*nbF = nbFunc;
	return funcItem;
}

/***
 *	beNotification()
 *
 *	This function is called, if a notification in Scantilla/Notepad++ occurs
 */
extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode)
{
	static
	UINT	currentDoc;
	UINT	currentEdit;

	if ((notifyCode->nmhdr.hwndFrom == nppData._scintillaMainHandle) ||
		(notifyCode->nmhdr.hwndFrom == nppData._scintillaSecondHandle) ||
		(notifyCode->nmhdr.code == TCN_TABDELETE))
	{
		::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&currentEdit);
		g_HSource = (currentEdit == 0)?nppData._scintillaMainHandle:nppData._scintillaSecondHandle;

		char		newPath[MAX_PATH];

		/* update open files */
		::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, 0, (LPARAM)newPath);

		if (strcmp(newPath, g_currentFile) != 0)
		{
			/* update current path in explorer and favorites */
			strcpy(g_currentFile, newPath);
			explorerDlg.NotifyNewFile();
			favesDlg.NotifyNewFile();

			/* update documents list */
			INT			i = 0;
			char		**fileNames;

			INT docCnt = (INT)::SendMessage(nppData._nppHandle, NPPM_GETNBOPENFILES, 0, ALL_OPEN_FILES);

			/* update doc information for file list () */
			fileNames	= (char **)new char*[docCnt];

			for (i = 0; i < docCnt; i++)
				fileNames[i] = (char*)new char[MAX_PATH];

			if (::SendMessage(nppData._nppHandle, NPPM_GETOPENFILENAMES, (WPARAM)fileNames, (LPARAM)docCnt)) {
				UpdateCurrUsedDocs(fileNames, docCnt);
				if (explorerDlg.isVisible())
					RedrawWindow(explorerDlg.getHSelf(), NULL, NULL, TRUE);
				if (favesDlg.isVisible())
					RedrawWindow(favesDlg.getHSelf(), NULL, NULL, TRUE);
			}

			for (i = 0; i < docCnt; i++)
				delete [] fileNames[i];
			delete [] fileNames;

			currentDoc = docCnt;
		}
	}
	if (notifyCode->nmhdr.hwndFrom == nppData._nppHandle)
	{
		if (notifyCode->nmhdr.code == NPPN_TBMODIFICATION)
		{
			/* change menu language */
			NLChangeNppMenu((HINSTANCE)g_hModule, nppData._nppHandle, PLUGIN_NAME, funcItem, nbFunc);

			g_TBExplorer.hToolbarBmp = (HBITMAP)::LoadImage((HINSTANCE)g_hModule, MAKEINTRESOURCE(IDB_TB_EXPLORER), IMAGE_BITMAP, 0, 0, (LR_DEFAULTSIZE | LR_LOADMAP3DCOLORS));
			g_TBFaves.hToolbarBmp = (HBITMAP)::LoadImage((HINSTANCE)g_hModule, MAKEINTRESOURCE(IDB_TB_FAVES), IMAGE_BITMAP, 0, 0, (LR_DEFAULTSIZE | LR_LOADMAP3DCOLORS));
			::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON, (WPARAM)funcItem[DOCKABLE_EXPLORER_INDEX]._cmdID, (LPARAM)&g_TBExplorer);
			::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON, (WPARAM)funcItem[DOCKABLE_FAVORTIES_INDEX]._cmdID, (LPARAM)&g_TBFaves);
		}
		if (notifyCode->nmhdr.code == NPPN_READY)
		{
			explorerDlg.initFinish();
			isNotepadCreated = TRUE;
		}
	}
}

/***
 *	messageProc()
 *
 *	This function is called, if a notification from Notepad occurs
 */
extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
   if (Message == WM_CREATE)
   {
      initMenu();
   }
   return TRUE;
}


/***
 *	ScintillaMsg()
 *
 *	API-Wrapper
 */
UINT ScintillaMsg(UINT message, WPARAM wParam, LPARAM lParam)
{
	return ::SendMessage(g_HSource, message, wParam, lParam);
}

/***
 *	loadSettings()
 *
 *	Load the parameters for plugin
 */
void loadSettings(void)
{
	/* initialize the config directory */
	::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM)configPath);

	/* Test if config path exist */
	if (PathFileExists(configPath) == FALSE)
	{
		::CreateDirectory(configPath, NULL);
	}

	strcpy(iniFilePath, configPath);
	strcat(iniFilePath, EXPLORER_INI);
	if (PathFileExists(iniFilePath) == FALSE)
	{
		::CloseHandle(::CreateFile(iniFilePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));
	}

	::GetPrivateProfileString(Explorer, LastPath, "C:\\", exProp.szCurrentPath, MAX_PATH, iniFilePath);
	exProp.iSplitterPos				= ::GetPrivateProfileInt(Explorer, SplitterPos, 120, iniFilePath);
	exProp.iSplitterPosHorizontal	= ::GetPrivateProfileInt(Explorer, SplitterPosHor, 200, iniFilePath);
	exProp.bAscending				= ::GetPrivateProfileInt(Explorer, SortAsc, TRUE, iniFilePath);
	exProp.iSortPos					= ::GetPrivateProfileInt(Explorer, SortPos, 0, iniFilePath);
	exProp.iColumnPosName			= ::GetPrivateProfileInt(Explorer, ColPosName, 150, iniFilePath);
	exProp.iColumnPosExt			= ::GetPrivateProfileInt(Explorer, ColPosExt, 50, iniFilePath);
	exProp.iColumnPosSize			= ::GetPrivateProfileInt(Explorer, ColPosSize, 70, iniFilePath);
	exProp.iColumnPosDate			= ::GetPrivateProfileInt(Explorer, ColPosDate, 100, iniFilePath);
	exProp.bShowHidden				= ::GetPrivateProfileInt(Explorer, ShowHiddenData, FALSE, iniFilePath);
	exProp.bViewBraces				= ::GetPrivateProfileInt(Explorer, ShowBraces, TRUE, iniFilePath);
	exProp.bViewLong				= ::GetPrivateProfileInt(Explorer, ShowLongInfo, FALSE, iniFilePath);
	exProp.bAddExtToName			= ::GetPrivateProfileInt(Explorer, AddExtToName, FALSE, iniFilePath);
	exProp.bAutoUpdate				= ::GetPrivateProfileInt(Explorer, AutoUpdate, TRUE, iniFilePath);
	exProp.fmtSize					= (eSizeFmt)::GetPrivateProfileInt(Explorer, SizeFormat, SFMT_KBYTE, iniFilePath);
	exProp.fmtDate					= (eDateFmt)::GetPrivateProfileInt(Explorer, DateFormat, DFMT_ENG, iniFilePath);
	exProp.uTimeout					= ::GetPrivateProfileInt(Explorer, TimeOut, 1000, iniFilePath);
	exProp.bUseSystemIcons			= ::GetPrivateProfileInt(Explorer, UseSystemIcons, TRUE, iniFilePath);

	TCHAR	number[3];
	LPTSTR	pszTemp = new TCHAR[MAX_PATH];
	for (INT i = 0; i < 20; i++)
	{
		sprintf(number, "%d", i);
		if (::GetPrivateProfileString(FilterHistory, number, "", pszTemp, MAX_PATH, iniFilePath) != 0)
			exProp.vStrFilterHistory.push_back(pszTemp);
	}
	::GetPrivateProfileString(Explorer, LastFilter, "*.*", pszTemp, MAX_PATH, iniFilePath);
	exProp.strLastFilter = pszTemp;
	delete [] pszTemp;

	::GetPrivateProfileString(Faves, LastElement, "", szLastElement, MAX_PATH, iniFilePath);
}

/***
 *	saveSettings()
 *
 *	Saves the parameters for plugin
 */
void saveSettings(void)
{
	TCHAR	temp[256];

	::WritePrivateProfileString(Explorer, LastPath, exProp.szCurrentPath, iniFilePath);
	::WritePrivateProfileString(Explorer, SplitterPos, itoa(exProp.iSplitterPos, temp, 10), iniFilePath);
	::WritePrivateProfileString(Explorer, SplitterPosHor, itoa(exProp.iSplitterPosHorizontal, temp, 10), iniFilePath);
	::WritePrivateProfileString(Explorer, SortAsc, itoa(exProp.bAscending, temp, 10), iniFilePath);
	::WritePrivateProfileString(Explorer, SortPos, itoa(exProp.iSortPos, temp, 10), iniFilePath);
	::WritePrivateProfileString(Explorer, ColPosName, itoa(exProp.iColumnPosName, temp, 10), iniFilePath);
	::WritePrivateProfileString(Explorer, ColPosExt, itoa(exProp.iColumnPosExt, temp, 10), iniFilePath);
	::WritePrivateProfileString(Explorer, ColPosSize, itoa(exProp.iColumnPosSize, temp, 10), iniFilePath);
	::WritePrivateProfileString(Explorer, ColPosDate, itoa(exProp.iColumnPosDate, temp, 10), iniFilePath);
	::WritePrivateProfileString(Explorer, ShowHiddenData, itoa(exProp.bShowHidden, temp, 10), iniFilePath);
	::WritePrivateProfileString(Explorer, ShowBraces, itoa(exProp.bViewBraces, temp, 10), iniFilePath);
	::WritePrivateProfileString(Explorer, ShowLongInfo, itoa(exProp.bViewLong, temp, 10), iniFilePath);
	::WritePrivateProfileString(Explorer, AddExtToName, itoa(exProp.bAddExtToName, temp, 10), iniFilePath);
	::WritePrivateProfileString(Explorer, AutoUpdate, itoa(exProp.bAutoUpdate, temp, 10), iniFilePath);
	::WritePrivateProfileString(Explorer, SizeFormat, itoa((INT)exProp.fmtSize, temp, 10), iniFilePath);
	::WritePrivateProfileString(Explorer, DateFormat, itoa((INT)exProp.fmtDate, temp, 10), iniFilePath);
	::WritePrivateProfileString(Explorer, DateFormat, itoa((INT)exProp.fmtDate, temp, 10), iniFilePath);
	::WritePrivateProfileString(Explorer, TimeOut, itoa((INT)exProp.uTimeout, temp, 10), iniFilePath);
	::WritePrivateProfileString(Explorer, UseSystemIcons, itoa(exProp.bUseSystemIcons, temp, 10), iniFilePath);

	for (INT i = exProp.vStrFilterHistory.size() - 1; i >= 0 ; i--)
	{
		::WritePrivateProfileString(FilterHistory, itoa(i, temp, 10), exProp.vStrFilterHistory[i].c_str(), iniFilePath); 
	}
	::WritePrivateProfileString(Explorer, LastFilter, exProp.strLastFilter.c_str(), iniFilePath); 
}


/***
 *	initMenu()
 *
 *	Initialize the menu
 */
void initMenu(void)
{
	HMENU		hMenu	= ::GetMenu(nppData._nppHandle);

	::ModifyMenu(hMenu, funcItem[2]._cmdID, MF_BYCOMMAND | MF_SEPARATOR, 0, 0);
	::ModifyMenu(hMenu, funcItem[4]._cmdID, MF_BYCOMMAND | MF_SEPARATOR, 0, 0);
}


/***
 *	getCurrentHScintilla()
 *
 *	Get the handle of the current scintilla
 */
HWND getCurrentHScintilla(INT which)
{
	return (which == 0)?nppData._scintillaMainHandle:nppData._scintillaSecondHandle;
}	


/**************************************************************************
 *	Interface functions
 */

void toggleExplorerDialog(void)
{
	UINT state = ::GetMenuState(::GetMenu(nppData._nppHandle), funcItem[DOCKABLE_EXPLORER_INDEX]._cmdID, MF_BYCOMMAND);
	explorerDlg.doDialog(state & MF_CHECKED ? false : true);
}

void toggleFavesDialog(void)
{
	UINT state = ::GetMenuState(::GetMenu(nppData._nppHandle), funcItem[DOCKABLE_FAVORTIES_INDEX]._cmdID, MF_BYCOMMAND);
	favesDlg.doDialog(state & MF_CHECKED ? false : true);
}

void openOptionDlg(void)
{
	if (optionDlg.doDialog(&exProp) == IDOK) {
		explorerDlg.redraw();
		favesDlg.redraw();
	}
}

void openHelpDlg(void)
{
	helpDlg.doDialog();
}


/**************************************************************************
 *	Subclass of Notepad
 */
LRESULT CALLBACK SubWndProcNotepad(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT			ret		= 0;

	switch (message)
	{
		case WM_ACTIVATE:
		{
			if (((LOWORD(wParam) == WA_ACTIVE) || (LOWORD(wParam) == WA_CLICKACTIVE)) && 
				(isNotepadCreated == TRUE) && ((HWND)lParam != hWnd))
			{
				if (exProp.bAutoUpdate == TRUE) {
					::KillTimer(explorerDlg.getHSelf(), EXT_UPDATEACTIVATE);
					::SetTimer(explorerDlg.getHSelf(), EXT_UPDATEACTIVATE, 200, NULL);
				} else {
					::KillTimer(explorerDlg.getHSelf(), EXT_UPDATEACTIVATEPATH);
					::SetTimer(explorerDlg.getHSelf(), EXT_UPDATEACTIVATEPATH, 200, NULL);
				}
			}
			ret = ::CallWindowProc(wndProcNotepad, hWnd, message, wParam, lParam);
			break;
		}
		case WM_DEVICECHANGE:
		{
			if ((wParam == DBT_DEVICEARRIVAL) || (wParam == DBT_DEVICEREMOVECOMPLETE))
			{
				::KillTimer(explorerDlg.getHSelf(), EXT_UPDATEDEVICE);
				::SetTimer(explorerDlg.getHSelf(), EXT_UPDATEDEVICE, 1000, NULL);
			}
			ret = ::CallWindowProc(wndProcNotepad, hWnd, message, wParam, lParam);
			break;
		}
		case WM_COMMAND:
		{
			if (wParam == IDM_FILE_SAVESESSION)
			{
				favesDlg.SaveSession();
				return TRUE;
			}
		}
		default:
			ret = ::CallWindowProc(wndProcNotepad, hWnd, message, wParam, lParam);
			break;
	}

	return ret;
}

/**************************************************************************
 *	Functions for file system
 */
BOOL VolumeNameExists(LPTSTR rootDrive, LPTSTR volumeName)
{
	BOOL	bRet = FALSE;

	if ((volumeName[0] != '\0') && (GetVolumeInformation(rootDrive, volumeName, MAX_PATH, NULL, NULL, NULL, NULL, 0) == TRUE))
	{
		bRet = TRUE;
	}
	return bRet;
}

bool IsValidFileName(LPTSTR pszFileName)
{
	if (strpbrk(pszFileName, "\\/:*?\"<>") == NULL)
		return true;

	::MessageBox(NULL, "Filename does not contain any of this characters:\n       \\ / : * ? \" < >", "Error", MB_OK);
	return false;
}

bool IsValidFolder(WIN32_FIND_DATA Find)
{
	if ((Find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && 
		(!(Find.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) || exProp.bShowHidden) &&
		 (strcmp(Find.cFileName, ".") != 0) && 
		 (strcmp(Find.cFileName, "..") != 0) &&
		 (Find.cFileName[0] != '?'))
		return true;

	return false;
}

bool IsValidParentFolder(WIN32_FIND_DATA Find)
{
	if ((Find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && 
		(!(Find.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) || exProp.bShowHidden) &&
		 (strcmp(Find.cFileName, ".") != 0) &&
		 (Find.cFileName[0] != '?'))
		return true;

	return false;
}

bool IsValidFile(WIN32_FIND_DATA Find)
{
	if (!(Find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && 
		(!(Find.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) || exProp.bShowHidden))
		return true;

	return false;
}

BOOL HaveChildren(LPTSTR parentFolderPathName)
{
	WIN32_FIND_DATA		Find		= {0};
	HANDLE				hFind		= NULL;
	BOOL				bFound		= TRUE;
	BOOL				bRet		= FALSE;

	if (parentFolderPathName[strlen(parentFolderPathName) - 1] != '\\')
		strcat(parentFolderPathName, "\\");

	/* add wildcard */
	strcat(parentFolderPathName, "*");

	if ((hFind = ::FindFirstFile(parentFolderPathName, &Find)) == INVALID_HANDLE_VALUE)
		return FALSE;

	do
	{
		if (IsValidFolder(Find) == TRUE)
		{
			bFound = FALSE;
			bRet = TRUE;
		}
	} while ((FindNextFile(hFind, &Find)) && (bFound == TRUE));

	::FindClose(hFind);

	return bRet;
}

/**************************************************************************
 *	Get system images
 */
HIMAGELIST GetSmallImageList(BOOL bSystem)
{
	static
	HIMAGELIST		himlSys	= NULL;
	HIMAGELIST		himl	= NULL;
	SHFILEINFO		sfi		= {0};

	if (bSystem) {
		if (himlSys == NULL) {
			himlSys = (HIMAGELIST)SHGetFileInfo("C:\\", 0, &sfi, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
		}
		himl = himlSys;
	} else {
		himl = ghImgList;
	}

	return himl;
}

void ExtractIcons(LPCSTR currentPath, LPCSTR volumeName, eDevType type, 
	LPINT piIconNormal, LPINT piIconSelected, LPINT piIconOverlayed)
{
	SHFILEINFO		sfi	= {0};
	UINT			length = strlen(currentPath) - 1;
	TCHAR			TEMP[MAX_PATH];
	UINT			stOverlay = (piIconOverlayed ? SHGFI_OVERLAYINDEX : 0);

	strcpy(TEMP, currentPath);
	if (TEMP[length] == '*')
		TEMP[length] = '\0';
	else if (TEMP[length] != '\\')
		strcat(TEMP, "\\");

	if (volumeName != NULL)
		strcat(TEMP, volumeName);

	if (exProp.bUseSystemIcons == FALSE)
	{
		/* get drive icon in any case correct */
		if (type == DEVT_DRIVE)
		{
			SHGetFileInfo(TEMP, 
				0, 
				&sfi, 
				sizeof(SHFILEINFO), 
				SHGFI_ICON | SHGFI_SMALLICON | stOverlay);
			::DestroyIcon(sfi.hIcon);
			SHGetFileInfo(TEMP, 
				FILE_ATTRIBUTE_NORMAL, 
				&sfi, 
				sizeof(SHFILEINFO), 
				SHGFI_ICON | SHGFI_SMALLICON | stOverlay);

			/* find drive icon in own image list */
			UINT	onPos = 0;
			for (UINT i = 0; i < gvDrvMap.size(); i++) {
				if (gvDrvMap[i].cDrv == TEMP[0]) {
					onPos = gvDrvMap[i].pos;
					break;
				}
			}
			/* if not found add to list otherwise replace new drive icon */
			if (onPos == 0) {
				onPos = ImageList_AddIcon(ghImgList, sfi.hIcon);
				tDrvMap drvMap = {TEMP[0], onPos};
				gvDrvMap.push_back(drvMap);
			} else {
				ImageList_ReplaceIcon(ghImgList, onPos, sfi.hIcon);
			}
			::DestroyIcon(sfi.hIcon);

			*piIconNormal	= onPos;
			*piIconSelected	= onPos;
		}
		else if (type == DEVT_DIRECTORY)
		{
			*piIconNormal	= ICON_FOLDER;
			*piIconSelected	= ICON_FOLDER;
		}
		else
		{
			*piIconNormal	= ICON_FILE;
			*piIconSelected	= ICON_FILE;
		}
		if (piIconOverlayed != NULL)
			*piIconOverlayed = 0;
	}
	else
	{
		/* get normal and overlayed icon */
		if ((type == DEVT_DIRECTORY) || (type == DEVT_DRIVE))
		{
			SHGetFileInfo(TEMP, 
				0, 
				&sfi, 
				sizeof(SHFILEINFO), 
				SHGFI_ICON | SHGFI_SMALLICON | stOverlay);
			if (type == DEVT_DRIVE)
			{
				::DestroyIcon(sfi.hIcon);
				SHGetFileInfo(TEMP, 
					FILE_ATTRIBUTE_NORMAL, 
					&sfi, 
					sizeof(SHFILEINFO), 
					SHGFI_ICON | SHGFI_SMALLICON | stOverlay | SHGFI_USEFILEATTRIBUTES);
			}
		}
		else
		{
			SHGetFileInfo(TEMP, 
				FILE_ATTRIBUTE_NORMAL, 
				&sfi, 
				sizeof(SHFILEINFO), 
				SHGFI_ICON | SHGFI_SMALLICON | stOverlay | SHGFI_USEFILEATTRIBUTES);
		}
		::DestroyIcon(sfi.hIcon);

		*piIconNormal	= sfi.iIcon & 0x000000ff;
		if (piIconOverlayed != NULL)
			*piIconOverlayed = sfi.iIcon >> 24;

		/* get selected icon */
		if (type == DEVT_DIRECTORY)
		{
			SHGetFileInfo(TEMP, 
				0, 
				&sfi, 
				sizeof(SHFILEINFO), 
				SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_OPENICON);
			*piIconSelected = sfi.iIcon;
		}
		else
		{
			*piIconSelected = *piIconNormal;
		}
	}
}

/**************************************************************************
 *	Resolve files if they are shortcuts
 */
HRESULT ResolveShortCut(LPCTSTR lpszShortcutPath, LPTSTR lpszFilePath, int maxBuf)
{
    HRESULT hRes = S_FALSE;
    CComPtr<IShellLink> ipShellLink;
        // buffer that receives the null-terminated string for the drive and path
    TCHAR szPath[MAX_PATH];     
        // buffer that receives the null-terminated string for the description
    TCHAR szDesc[MAX_PATH]; 
        // structure that receives the information about the shortcut
    WIN32_FIND_DATA wfd;    
    WCHAR wszTemp[MAX_PATH];

    lpszFilePath[0] = '\0';

    // Get a pointer to the IShellLink interface
    hRes = CoCreateInstance(CLSID_ShellLink,
                            NULL, 
                            CLSCTX_INPROC_SERVER,
                            IID_IShellLink,
                            (void**)&ipShellLink); 

    if (hRes == S_OK) 
    { 
        // Get a pointer to the IPersistFile interface
        CComQIPtr<IPersistFile> ipPersistFile(ipShellLink);

        // IPersistFile is using LPCOLESTR, so make sure that the string is Unicode
#if !defined _UNICODE
        MultiByteToWideChar(CP_ACP, 0, lpszShortcutPath,
                                       -1, wszTemp, MAX_PATH);
#else
        wcsncpy(wszTemp, lpszShortcutPath, MAX_PATH);
#endif

        // Open the shortcut file and initialize it from its contents
        hRes = ipPersistFile->Load(wszTemp, STGM_READ); 
        if (hRes == S_OK) 
        {
            // Try to find the target of a shortcut, even if it has been moved or renamed
            hRes = ipShellLink->Resolve(NULL, SLR_UPDATE); 
            if (hRes == S_OK) 
            {
                // Get the path to the shortcut target
                hRes = ipShellLink->GetPath(szPath, 
                                     MAX_PATH, &wfd, SLGP_RAWPATH); 
				if (hRes == S_OK) 
				{
					// Get the description of the target
					hRes = ipShellLink->GetDescription(szDesc, MAX_PATH); 
					if (hRes == S_OK) 
					{
		                lstrcpyn(lpszFilePath, szPath, maxBuf);
					}
				}
            } 
        } 
    } 

    return hRes;
}


/**************************************************************************
 *	Current docs
 */
void UpdateCurrUsedDocs(LPTSTR* pFiles, UINT numFiles)
{
	TCHAR	pszLongName[MAX_PATH];

	/* clear old elements */
	g_vStrCurrentFiles.clear();

	for (UINT i = 0; i < numFiles; i++)
	{
		/* store only long pathes */
		if (GetLongPathName(pFiles[i], pszLongName, MAX_PATH) != 0)
		{
			g_vStrCurrentFiles.push_back(pszLongName);
		}
	}
}

BOOL IsFileOpen(LPCTSTR pCurrFile)
{
	for (UINT iCurrFiles = 0; iCurrFiles < g_vStrCurrentFiles.size(); iCurrFiles++)
	{
		if (stricmp(g_vStrCurrentFiles[iCurrFiles].c_str(), pCurrFile) == 0)
		{
			return TRUE;
		}
	}
	return FALSE;
}


/**************************************************************************
 *	Windows helper functions
 */
void ClientToScreen(HWND hWnd, RECT* rect)
{
	POINT		pt;

	pt.x		 = rect->left;
	pt.y		 = rect->top;
	::ClientToScreen( hWnd, &pt );
	rect->left   = pt.x;
	rect->top    = pt.y;

	pt.x		 = rect->right;
	pt.y		 = rect->bottom;
	::ClientToScreen( hWnd, &pt );
	rect->right  = pt.x;
	rect->bottom = pt.y;
}

void ScreenToClient(HWND hWnd, RECT* rect)
{
	POINT		pt;

	pt.x		 = rect->left;
	pt.y		 = rect->top;
	::ScreenToClient( hWnd, &pt );
	rect->left   = pt.x;
	rect->top    = pt.y;

	pt.x		 = rect->right;
	pt.y		 = rect->bottom;
	::ScreenToClient( hWnd, &pt );
	rect->right  = pt.x;
	rect->bottom = pt.y;
}

void ErrorMessage(DWORD err)
{
	LPVOID	lpMsgBuf;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) & lpMsgBuf, 0, NULL);	// Process any inserts in lpMsgBuf.
	::MessageBox(NULL, (LPCTSTR) lpMsgBuf, "Error", MB_OK | MB_ICONINFORMATION);

	LocalFree(lpMsgBuf);
}



