//this file is part of Explorer Plugin for Notepad++
//Copyright (C)2005 Jens Lorenz <jens.plugin.npp@gmx.de>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

/* include files */
#include "stdafx.h"
#include "PluginInterface.h"
#include "ExplorerDialog.h"
#include "FavesDialog.h"
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



#define SHGFI_OVERLAYINDEX 0x000000040


const int	nbFunc	= 3;



/* information for notepad */
const char  PLUGIN_NAME[] = "Explorer";

char        configPath[MAX_PATH];
char		iniFilePath[MAX_PATH];

/* ini file sections */
const char WindowData[]		= "WindowData";
const char Explorer[]		= "Explorer";
const char Faves[]			= "Faves";

/* section Explorer */
const char SplitterPos[]	= "SplitterPos";
const char ColumnPos[]		= "ColumnPos";
const char LastPath[]		= "LastPath";

/* section Faves */
const char LastElement[]	= "LastElement";

/* for subclassing */
WNDPROC	wndProcNotepad = NULL;


/* global values */
HMODULE			hShell32;
NppData			nppData;
HANDLE			g_hModule;
HWND			g_HSource;
FuncItem		funcItem[nbFunc];
toolbarIcons	g_TBExplorer;
toolbarIcons	g_TBFaves;


/* get system information */
BOOL	isDragFullWin = FALSE;

/* create classes */
ExplorerDialog		explorerDlg;
FavesDialog			favesDlg;
HelpDlg				helpDlg;


/* dialog params */
RECT	rcDlg					= {0, 0, 0, 0};
INT		iSplitterPosExplorer	= 0;
INT		iColumnPosExplorer		= 0;
char	szLastPath[MAX_PATH];
INT		iSplitterPosFaves		= 0;
INT		iColumnPosFaves			= 0;
char	currentPath[MAX_PATH];


/* recognition of changed document */
char	szLastElement[MAX_PATH];



BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  reasonForCall, 
                       LPVOID lpReserved )
{
	g_hModule = hModule;

    switch (reasonForCall)
    {
		case DLL_PROCESS_ATTACH:
		{
			char	nppPath[MAX_PATH];

			GetModuleFileName((HMODULE)hModule, nppPath, sizeof(nppPath));
            // remove the module name : get plugins directory path
			PathRemoveFileSpec(nppPath);
 
			// cd .. : get npp executable path
			PathRemoveFileSpec(nppPath);

			/* make ini file path if not exist */
			strcpy(configPath, nppPath);
			strcat(configPath, "\\plugins\\Config");
			
			if (PathFileExists(configPath) == FALSE)
			{
				::CreateDirectory(configPath, NULL);
			}

			strcpy(iniFilePath, configPath);
			strcat(iniFilePath, "\\Explorer.ini");
			if (PathFileExists(iniFilePath) == FALSE)
			{
				::CloseHandle(::CreateFile(iniFilePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));
			}

			/* Set function pointers */
			funcItem[0]._pFunc = toggleExplorerDialog;
			funcItem[1]._pFunc = toggleFavesDialog;
			funcItem[2]._pFunc = openHelpDlg;
			
			/* Fill menu names */
			strcpy(funcItem[0]._itemName, "View Explorer");
			strcpy(funcItem[1]._itemName, "View Favorites");
			strcpy(funcItem[2]._itemName, "Help");

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

			iSplitterPosExplorer = ::GetPrivateProfileInt(Explorer, SplitterPos, 120, iniFilePath);
			iColumnPosExplorer	 = ::GetPrivateProfileInt(Explorer, ColumnPos, 200, iniFilePath);
			::GetPrivateProfileString(Explorer, LastPath, "C:\\", szLastPath, MAX_PATH, iniFilePath);

			::GetPrivateProfileString(Faves, LastElement, "", szLastElement, MAX_PATH, iniFilePath);
			break;
		}	
		case DLL_PROCESS_DETACH:
		{
			char	temp[256];

			::WritePrivateProfileString(Explorer, SplitterPos, itoa(iSplitterPosExplorer, temp, 10), iniFilePath);
			::WritePrivateProfileString(Explorer, ColumnPos, itoa(iColumnPosExplorer, temp, 10), iniFilePath);
			::WritePrivateProfileString(Explorer, LastPath, szLastPath, iniFilePath);

			::WritePrivateProfileString(Faves, LastElement, LastElement, iniFilePath);

			/* destroy dialogs */
			explorerDlg.destroy();
			favesDlg.destroy();

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

	/* initial dialogs */
	explorerDlg.init((HINSTANCE)g_hModule, nppData, szLastPath, &iSplitterPosExplorer, &iColumnPosExplorer);
	favesDlg.init((HINSTANCE)g_hModule, nppData, szLastElement);
	helpDlg.init((HINSTANCE)g_hModule, nppData);

	/* Subclassing for Notepad */
	wndProcNotepad = (WNDPROC)SetWindowLong(nppData._nppHandle, GWL_WNDPROC, (LPARAM)SubWndProcNotepad);
}

extern "C" __declspec(dllexport) const char * getName()
{
	return PLUGIN_NAME;
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *nbF)
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
	UINT	currentEdit;

	if ((notifyCode->nmhdr.hwndFrom == nppData._scintillaMainHandle) ||
		(notifyCode->nmhdr.hwndFrom == nppData._scintillaSecondHandle))
	{
		::SendMessage(nppData._nppHandle, WM_GETCURRENTSCINTILLA, 0, (LPARAM)&currentEdit);
		g_HSource = (currentEdit == 0)?nppData._scintillaMainHandle:nppData._scintillaSecondHandle;

		char		newPath[MAX_PATH];

		/* update open files */
		::SendMessage(nppData._nppHandle, WM_GET_FULLCURRENTPATH, 0, (LPARAM)newPath);

		if (strcmp(newPath, currentPath) != 0)
		{
			strcpy(currentPath, newPath);
			explorerDlg.NotifyNewFile();
			favesDlg.NotifyNewFile();
		}
	}
	if (notifyCode->nmhdr.hwndFrom == nppData._nppHandle)
	{
		if (notifyCode->nmhdr.code == NPPN_TB_MODIFICATION)
		{
			g_TBExplorer.hToolbarBmp = (HBITMAP)::LoadImage((HINSTANCE)g_hModule, MAKEINTRESOURCE(IDB_TB_EXPLORER), IMAGE_BITMAP, 0, 0, (LR_DEFAULTSIZE | LR_LOADMAP3DCOLORS));
			g_TBFaves.hToolbarBmp = (HBITMAP)::LoadImage((HINSTANCE)g_hModule, MAKEINTRESOURCE(IDB_TB_FAVES), IMAGE_BITMAP, 0, 0, (LR_DEFAULTSIZE | LR_LOADMAP3DCOLORS));
			::SendMessage(nppData._nppHandle, WM_ADDTOOLBARICON, (WPARAM)funcItem[DOCKABLE_EXPLORER_INDEX]._cmdID, (LPARAM)&g_TBExplorer);
			::SendMessage(nppData._nppHandle, WM_ADDTOOLBARICON, (WPARAM)funcItem[DOCKABLE_FAVORTIES_INDEX]._cmdID, (LPARAM)&g_TBFaves);
		}
		if (notifyCode->nmhdr.code == NPPN_READY)
		{
			explorerDlg.initFinish();
		}
	}
}

/***
 *	messageProc()
 *
 *	This function is called, if a notification from Notepad occurs
 */
extern "C" __declspec(dllexport) void messageProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
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
 *	initMenu()
 *
 *	Initialize the menu
 */
void initMenu(void)
{
}


/***
 *	getCurrentHScintilla()
 *
 *	Get the handle of the current scintilla
 */
HWND getCurrentHScintilla(int which)
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

void openHelpDlg(void)
{
	helpDlg.doDialog();
}


/**************************************************************************
 *	Subclass of Notepad
 */
LRESULT CALLBACK SubWndProcNotepad(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT			ret = 0;

	switch (message)
	{
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
BOOL VolumeNameExists(char* rootDrive, char* volumeName)
{
	BOOL	bRet = FALSE;

	if ((volumeName[0] != '\0') && (GetVolumeInformation(rootDrive, volumeName, MAX_PATH, NULL, NULL, NULL, NULL, 0) == TRUE))
	{
		bRet = TRUE;
	}
	return bRet;
}

bool IsValidFolder(WIN32_FIND_DATA Find)
{
	if ((Find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && !(Find.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) &&
		 (strcmp(Find.cFileName, ".") != 0) && (strcmp(Find.cFileName, "..") != 0))
		return true;

	return false;
}

bool IsValidParentFolder(WIN32_FIND_DATA Find)
{
	if ((Find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && !(Find.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) &&
		 (strcmp(Find.cFileName, "..") == 0))
		return true;

	return false;
}

bool IsValidFile(WIN32_FIND_DATA Find)
{
	if (!(Find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && !(Find.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
		return true;

	return false;
}

BOOL HaveChildren(char* parentFolderPathName)
{
	WIN32_FIND_DATA		Find		= {0};
	HANDLE				hFind		= NULL;
	BOOL				bFound		= TRUE;
	BOOL				bRet		= FALSE;

	if (parentFolderPathName[strlen(parentFolderPathName) - 1] != '\\')
	{
		strcat(parentFolderPathName, "\\");
	}

	/* add wildcard */
	strcat(parentFolderPathName, "*");

	if ((hFind = ::FindFirstFile(parentFolderPathName, &Find)) == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

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

#pragma comment(lib, "comctl32.lib")

//
//	Typedefs for SHELL APIs
//
typedef BOOL (WINAPI * SHGIL_PROC)	(HIMAGELIST *phLarge, HIMAGELIST *phSmall);
typedef BOOL (WINAPI * FII_PROC)	(BOOL fFullInit);


// code from: http://support.microsoft.com/kb/192055/en-us
HIMAGELIST GetSystemImageList(BOOL fSmall)
{
	HIMAGELIST	himlLarge;
	HIMAGELIST	himlSmall;
    HIMAGELIST  himl;

	SHFILEINFO		sfi	= {0};

	if (getWindowsVersion() > WV_NT)
	{
		if (fSmall)
			himl = (HIMAGELIST)SHGetFileInfo("C:\\", 0, &sfi, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
		else
			himl = (HIMAGELIST)SHGetFileInfo("C:\\", 0, &sfi, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX);
	}
	else
	{
		SHGIL_PROC	Shell_GetImageLists;
		FII_PROC	FileIconInit;

		// Don't free until we terminate - otherwise, the image-lists will be destroyed
		if(hShell32 == 0)
			hShell32 = LoadLibrary("shell32.dll");

		if(hShell32 == 0)
			return NULL;

		// Get Undocumented APIs from Shell32.dll: 
		// Shell_GetImageLists and FileIconInit
		//
		Shell_GetImageLists  = (SHGIL_PROC)  GetProcAddress(hShell32, (LPCSTR)71);
		FileIconInit		 = (FII_PROC)    GetProcAddress(hShell32, (LPCSTR)660);

		// FreeIconList@8 = ord 227

		if(Shell_GetImageLists == 0)
		{
			FreeLibrary(hShell32);
			return NULL;
		}

		// Initialize imagelist for this process - function not present on win95/98
		if(FileIconInit != 0)
			FileIconInit(TRUE);

		// Get handles to the large+small system image lists!
		Shell_GetImageLists(&himlLarge, &himlSmall);

		if (fSmall = FALSE)
			himl = himlLarge;
		else
			himl = himlSmall;

		/*
		 *  You need to create a temporary, empty .lnk file that you can use to
		 *  pass to IShellIconOverlay::GetOverlayIndex. You could just enumerate
		 *  down from the Start Menu folder to find an existing .lnk file, but
		 *  there is a very remote chance that you will not find one. By creating
		 *  your own, you know this code will always work.
		 */ 
		HRESULT           hr;
		IShellFolder      *psfDesktop = NULL;
		IShellFolder      *psfTempDir = NULL;
		IMalloc           *pMalloc = NULL;
		LPITEMIDLIST      pidlTempDir = NULL;
		LPITEMIDLIST      pidlTempFile = NULL;
		TCHAR             szTempDir[MAX_PATH];
		TCHAR             szTempFile[MAX_PATH] = TEXT("");
		TCHAR             szFile[MAX_PATH];
		HANDLE            hFile;
		int               i;
		OLECHAR           szOle[MAX_PATH];
		DWORD             dwAttributes;
		DWORD             dwEaten;
		IShellIconOverlay *psio = NULL;
		int               nIndex;

		// Get the desktop folder.
		hr = SHGetDesktopFolder(&psfDesktop);
		if(FAILED(hr))
		{
			goto exit;
		}

		// Get the shell's allocator.
		hr = SHGetMalloc(&pMalloc);
		if(FAILED(hr))
		{
			goto exit;
		}

		// Get the TEMP directory.
		if(!GetTempPath(MAX_PATH, szTempDir))
		{
			/*
			 *  There might not be a TEMP directory. If this is the case, use the
			 *  Windows directory.
			*/ 
			if(!GetWindowsDirectory(szTempDir, MAX_PATH))
			{
				hr = E_FAIL;
				goto exit;
			}
		}

		// Create a temporary .lnk file.
		if(szTempDir[lstrlen(szTempDir) - 1] != '\\')
		{
			lstrcat(szTempDir, TEXT("\\"));
		}
		for(i = 0, hFile = INVALID_HANDLE_VALUE; INVALID_HANDLE_VALUE == hFile; i++)
		{
			lstrcpy(szTempFile, szTempDir);
			wsprintf(szFile, TEXT("temp%d.lnk"), i);
			lstrcat(szTempFile, szFile);

			hFile = CreateFile(szTempFile, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

			// Do not try this more than 100 times.
			if(i > 100)
			{
				hr = E_FAIL;
				goto exit;
			}
		}

		// Close the file you just created.
		CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;

		// Get the PIDL for the directory.
		::MultiByteToWideChar(CP_UTF8, 0, szTempDir, -1, szOle, MAX_PATH);
		hr = psfDesktop->ParseDisplayName(NULL, NULL, szOle, &dwEaten, &pidlTempDir, &dwAttributes);
		if(FAILED(hr))
		{
			goto exit;
		}

		// Get the IShellFolder for the TEMP directory.
		hr = psfDesktop->BindToObject(pidlTempDir, NULL, IID_IShellFolder, (LPVOID*)&psfTempDir);
		if(FAILED(hr))
		{
			goto exit;
		}

		/*
		 *  Get the IShellIconOverlay interface for this folder. If this fails,
		 *  it could indicate that you are running on a pre-Internet Explorer 4.0
		 *  shell, which doesn't support this interface. If this is the case, the
		 *  overlay icons are already in the system image list.
		 */ 
		hr = psfTempDir->QueryInterface(IID_IShellIconOverlay, (LPVOID*)&psio);
		if(FAILED(hr))
		{
			goto exit;
		}

		// Get the PIDL for the temporary .lnk file.
		::MultiByteToWideChar(CP_UTF8, 0, szFile, -1, szOle, MAX_PATH);
		hr = psfTempDir->ParseDisplayName(NULL, NULL, szOle, &dwEaten, &pidlTempFile, &dwAttributes);
		if(FAILED(hr))
		{
			goto exit;
		}

		/*
		 *  Get the overlay icon for the .lnk file. This causes the shell
		 *  to put all of the standard overlay icons into your copy of the system
		 *  image list.
		 */ 
		hr = psio->GetOverlayIndex(pidlTempFile, &nIndex);

exit:
		// Delete the temporary file.
		DeleteFile(szTempFile);

		if(psio) psio->Release();
		if(INVALID_HANDLE_VALUE != hFile) CloseHandle(hFile);
		if(psfTempDir) psfTempDir->Release();
		if(pMalloc)
		{
			if(pidlTempFile) pMalloc->Free(pidlTempFile);
			if(pidlTempDir)  pMalloc->Free(pidlTempDir);
			pMalloc->Release();
		}
		if(psfDesktop) psfDesktop->Release();
	}

    return himl;
}

void ExtractIcons(const char* currentPath, const char* volumeName, bool isDir, int* iIconNormal, int* iIconSelected, int* iIconOverlayed)
{
	SHFILEINFO		sfi	= {0};
	char			TEMP[MAX_PATH];

	strcpy(TEMP, currentPath);
	if (TEMP[strlen(TEMP) - 1] == '*')
	{
		TEMP[strlen(TEMP) - 1] = '\0';
	}
	else if (TEMP[strlen(TEMP) - 1] != '\\')
	{
		strcat(TEMP, "\\");
	}

	if (volumeName != NULL)
	{
		strcat(TEMP, volumeName);
	}


	/* get normal and overlayed icon */
	SHGetFileInfo(TEMP, 0, &sfi, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_OVERLAYINDEX);

//	if (sfi.iIcon == 0)
//		SHGetFileInfo(TEMP, 0, &sfi, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_OVERLAYINDEX);

	*iIconNormal	= sfi.iIcon & 0x000000ff;
	*iIconOverlayed = sfi.iIcon >> 24;

	if (sfi.hIcon != NULL)
		::DestroyIcon(sfi.hIcon);

	/* get selected icon */
	if (isDir)
	{
		SHGetFileInfo(TEMP, 0, &sfi, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_OPENICON);
		*iIconSelected = sfi.iIcon;
	}
	else
	{
		*iIconSelected = 0;
	}
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


