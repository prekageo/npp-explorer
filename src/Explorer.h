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


#ifndef EXPLORER_H
#define EXPLORER_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <zmouse.h>
#include <windowsx.h>
#include <commctrl.h>

#include "PluginInterface.h"
#include "Notepad_plus_rc.h"
#include "NativeLang_def.h"

#include <TCHAR.H>
#include <vector>
using namespace std;



#define DOCKABLE_EXPLORER_INDEX		0
#define DOCKABLE_FAVORTIES_INDEX	1


extern enum winVer gWinVersion;


/******************** faves ***************************/

#define	ICON_FOLDER		0
#define	ICON_FILE		1
#define	ICON_WEB		2
#define	ICON_SESSION	3
#define	ICON_GROUP		4
#define	ICON_PARENT		5


typedef enum {
	FAVES_FOLDERS = 0,
	FAVES_FILES,
	FAVES_WEB,
	FAVES_SESSIONS,
	FAVES_ITEM_MAX
} eFavesElements;

static LPTSTR cFavesItemNames[11] = {
	_T("[Folders]"),
	_T("[Files]"),
	_T("[Web]"),
	_T("[Sessions]")
};


#define FAVES_PARAM				0x0000000F
#define FAVES_PARAM_MAIN		0x00000010
#define FAVES_PARAM_GROUP		0x00000020
#define FAVES_PARAM_LINK		0x00000040
#define FAVES_PARAM_EXPAND		0x00000100


typedef struct TItemElement {
	UINT						uParam;
	LPSTR						pszName;
	LPSTR						pszLink;
	vector<TItemElement>		vElements;
} tItemElement, *PELEM;

typedef vector<TItemElement>::iterator		ELEM_ITR;


/******************** explorer ***************************/

static LPTSTR cColumns[5] = {
	_T("Name"),
	_T("Ext."),
	_T("Size"),
	_T("Date")
};



static TCHAR FAVES_DATA[]		= _T("\\Favorites.dat");
static TCHAR EXPLORER_INI[]		= _T("\\Explorer.ini");
static TCHAR CONFIG_PATH[]		= _T("\\plugins\\Config");



/********************************************************/

/* see in notepad sources */
#define TCN_TABDROPPED (TCN_FIRST - 10)
#define TCN_TABDROPPEDOUTSIDE (TCN_FIRST - 11)
#define TCN_TABDELETE (TCN_FIRST - 12)

/********************************************************/

#define TITLETIP_CLASSNAME "MyToolTip"


typedef enum {
	DEVT_DRIVE,
	DEVT_DIRECTORY,
	DEVT_FILE
} eDevType;


typedef enum {
	SFMT_BYTES,
	SFMT_KBYTE,
	SFMT_DYNAMIC,
	SFMT_DYNAMIC_EX,
	SFMT_MAX
} eSizeFmt;


const LPTSTR pszSizeFmt[18] = {
	_T("Bytes"),
	_T("kBytes"),
	_T("Dynamic x b/k/M"),
	_T("Dynamic x,x b/k/M")
};

typedef enum {
	DFMT_ENG,
	DFMT_GER,
	DFMT_MAX
} eDateFmt;

const LPTSTR pszDateFmt[12] = {
	_T("Y/M/D HH:MM"),
	_T("D.M.Y HH:MM")
};

typedef struct {
	char	cDrv;
	UINT	pos;
} tDrvMap;

typedef struct {
	/* pointer to global current path */
	TCHAR			szCurrentPath[MAX_PATH];
	INT				iSplitterPos;
	INT				iSplitterPosHorizontal;
	BOOL			bAscending;
	INT				iSortPos;
	INT				iColumnPosName;
	INT				iColumnPosExt;
	INT				iColumnPosSize;
	INT				iColumnPosDate;
	BOOL			bShowHidden;
	BOOL			bViewBraces;
	BOOL			bViewLong;
	BOOL			bAddExtToName;
	BOOL			bAutoUpdate;
	eSizeFmt		fmtSize;
	eDateFmt		fmtDate;
	vector<string>	vStrFilterHistory;
	string			strLastFilter;
	UINT			uTimeout;
	BOOL			bUseSystemIcons;
} tExProp;




UINT ScintillaMsg(UINT message, WPARAM wParam = 0, LPARAM lParam = 0);

void loadSettings(void);
void saveSettings(void);
void initMenu(void);

void toggleExplorerDialog(void);
void toggleFavesDialog(void);
void openOptionDlg(void);
void openHelpDlg(void);

LRESULT CALLBACK SubWndProcNotepad(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


#define	ALLOW_PARENT_SEL	1

BOOL VolumeNameExists(LPTSTR rootDrive, LPTSTR volumeName);
bool IsValidFileName(LPTSTR pszFileName);
bool IsValidFolder(WIN32_FIND_DATA Find);
bool IsValidParentFolder(WIN32_FIND_DATA Find);
bool IsValidFile(WIN32_FIND_DATA Find);
BOOL HaveChildren(LPTSTR parentFolderPathName);

/* Get Image Lists */
HIMAGELIST GetSmallImageList(BOOL bSystem);
void ExtractIcons(LPCSTR currentPath, LPCSTR fileName, eDevType type, LPINT iIconNormal, LPINT iIconSelected, LPINT iIconOverlayed);

/* Resolve Links */
HRESULT ResolveShortCut(LPCTSTR lpszShortcutPath, LPTSTR lpszFilePath, int maxBuf);

/* current open files */
void UpdateCurrUsedDocs(LPTSTR *ppFiles, UINT numFiles);
BOOL IsFileOpen(LPCTSTR pCurrFile);

/* Extended Window Funcions */
void ClientToScreen(HWND hWnd, RECT* rect);
void ScreenToClient(HWND hWnd, RECT* rect);
void ErrorMessage(DWORD err);


#endif //EXPLORER_H

