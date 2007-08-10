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


#define		_DEBUG_


#ifndef PLUGININTERFACE_H
#define PLUGININTERFACE_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <zmouse.h>
#include <windowsx.h>
#include <commctrl.h>
#include "Scintilla.h"
#include "rcNotepad.h"
#include <TCHAR.H>

#include <vector>

using namespace std;

#ifdef	_DEBUG_
    static TCHAR cDBG[256];

	#define DEBUG(x)            ::MessageBox(NULL, x, "DEBUG", MB_OK)
	#define DEBUG_VAL(x)        itoa(x,cDBG,10);DEBUG(cDBG)
	#define DEBUG_VAL_INFO(x,y) sprintf(cDBG, "%s: %d", x, y);DEBUG(cDBG)
	#define DEBUG_BRACE(x)      ScintillaMsg(SCI_SETSEL, x, x);DEBUG("Brace on position:")
	#define DEBUG_STRING(x,y)   ScintillaMsg(SCI_SETSEL, x, y);DEBUG("Selection:")
#else
	#define DEBUG(x)
	#define DEBUG_VAL(x)
	#define DEBUG_VAL_INFO(x,y)
	#define DEBUG_BRACE(x)
#endif


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


#define TITLETIP_CLASSNAME "MyToolTip"



#define NPPMSG   (WM_USER + 1000)
	#define NPPM_GETCURRENTSCINTILLA			(NPPMSG + 4)
	#define NPPM_GETCURRENTLANGTYPE				(NPPMSG + 5)
	#define NPPM_SETCURRENTLANGTYPE				(NPPMSG + 6)
	#define NPPM_NBOPENFILES					(NPPMSG + 7)
		#define ALL_OPEN_FILES					0
		#define PRIMARY_VIEW					1
		#define SECOND_VIEW						2

	#define NPPM_GETOPENFILENAMES				(NPPMSG + 8)
	#define NPPM_CANCEL_SCINTILLAKEY			(NPPMSG + 9)
	#define NPPM_BIND_SCINTILLAKEY				(NPPMSG + 10)
	#define NPPM_SCINTILLAKEY_MODIFIED			(NPPMSG + 11)
	#define NPPM_MODELESSDIALOG					(NPPMSG + 12)
		#define MODELESSDIALOGADD				0
		#define MODELESSDIALOGREMOVE			1

	#define NPPM_NBSESSIONFILES					(NPPMSG + 13)
	#define NPPM_GETSESSIONFILES				(NPPMSG + 14)
	#define NPPM_SAVESESSION					(NPPMSG + 15)
	#define NPPM_SAVECURRENTSESSION				(NPPMSG + 16)

	struct sessionInfo {
		char* filePathName;
		int sessionFile;
		char** sessionFileArray;
	};

	#define NPPM_GETOPENFILENAMES_PRIMARY		(NPPMSG + 17)
	#define NPPM_GETOPENFILENAMES_SECOND		(NPPMSG + 18)
	#define NPPM_GETPARENTOF					(NPPMSG + 19)
	#define NPPM_CREATESCINTILLAHANDLE			(NPPMSG + 20)
	#define NPPM_DESTROYSCINTILLAHANDLE			(NPPMSG + 21)
	#define NPPM_GETNBUSERLANG					(NPPMSG + 22)
	#define NPPM_GETCURRENTDOCINDEX				(NPPMSG + 23)
	#define NPPM_SETSTATUSBAR					(NPPMSG + 24)
		#define STATUSBAR_DOC_TYPE				0
		#define STATUSBAR_DOC_SIZE				1
		#define STATUSBAR_CUR_POS				2
		#define STATUSBAR_EOF_FORMAT			3
		#define STATUSBAR_UNICODE_TYPE			4
		#define STATUSBAR_TYPING_MODE			5

	#define NPPM_ENCODE_SCI						(NPPMSG + 26)
	#define NPPM_DECODE_SCI						(NPPMSG + 27)

	#define NPPM_ACTIVATE_DOC					(NPPMSG + 28)
	// NPPM_ACTIVATE_DOC(int index2Activate, int view)

	#define NPPM_LAUNCH_FINDINFILESDLG			(NPPMSG + 29)
	// NPPM_LAUNCH_FINDINFILESDLG(char * dir2Search, char * filtre)

	#define NPPM_DMM_SHOW						(NPPMSG + 30)
	#define NPPM_DMM_HIDE						(NPPMSG + 31)
	#define NPPM_DMM_UPDATEDISPINFO				(NPPMSG + 32)
	//void NPPM_DMM_xxx(0, tTbData->hClient)

	#define NPPM_DMM_REGASDCKDLG				(NPPMSG + 33)
	//void NPPM_DMM_REGASDCKDLG(0, &tTbData)

	#define NPPM_LOADSESSION					(NPPMSG + 34)
	//void NPPM_LOADSESSION(0, const char* file name)

	#define NPPM_DMM_VIEWOTHERTAB				(NPPMSG + 35)
	//void NPPM_DMM_VIEWOTHERTAB(0, tTbData->hClient)

	#define NPPM_RELOADFILE						(NPPMSG + 36)
	//BOOL NPPM_RELOADFILE(BOOL withAlert, char *filePathName2Reload)

	#define NPPM_SWITCHTOFILE					(NPPMSG + 37)
	//BOOL NPPM_SWITCHTOFILE(0, char *filePathName2switch)

	#define NPPM_SAVECURRENTFILE				(NPPMSG + 38)
	//BOOL NPPM_SWITCHCURRENTFILE(0, 0)

	#define NPPM_SAVEALLFILES					(NPPMSG + 39)
	//BOOL NPPM_SAVEALLFILES(0, 0)

	#define NPPM_PIMENU_CHECK					(NPPMSG + 40)
	//void NPPM_PIMENU_CHECK(UINT	funcItem[X]._cmdID, TRUE/FALSE)

	#define NPPM_ADDTOOLBARICON					(NPPMSG + 41)
	//void NPPM_ADDTOOLBARICON(UINT funcItem[X]._cmdID, toolbarIcons icon)
		struct toolbarIcons {
			HBITMAP	hToolbarBmp;
			HICON	hToolbarIcon;
		};

	#define NPPM_GETWINDOWSVERSION				(NPPMSG + 42)
	//winVer NPPM_GETWINDOWSVERSION(0, 0)

	#define NPPM_DMMGETPLUGINHWNDBYNAME			(NPPMSG + 43)
	//HWND NPPM_DMM_GETPLUGINHWNDBYNAME(const char *windowName, const char *moduleName)
	// if moduleName is NULL, then return value is NULL
	// if windowName is NULL, then the first found window handle which matches with the moduleName will be returned
	
	#define NPPM_MAKECURRENTBUFFERDIRTY			(NPPMSG + 44)
	//BOOL NPPM_MAKECURRENTBUFFERDIRTY(0, 0)

	#define NPPM_GETENABLETHEMETEXTUREFUNC		(NPPMSG + 45)
	//BOOL NPPM_GETENABLETHEMETEXTUREFUNC(0, 0)

	#define NPPM_GETPLUGINSCONFIGDIR			(NPPMSG + 46)
	//void NPPM_GETPLUGINSCONFIGDIR(int strLen, char *str)


// Notification code
#define NPPN_FIRST			1000
	// To notify plugins that all the procedures of launchment of notepad++ are done.
	#define NPPN_READY							(NPPN_FIRST + 1)
	//scnNotification->nmhdr.code = NPPN_READY;
	//scnNotification->nmhdr.hwndFrom = hwndNpp;
	//scnNotification->nmhdr.idFrom = 0;

	// To notify plugins that toolbar icons can be registered
	#define NPPN_TB_MODIFICATION				(NPPN_FIRST + 2)
	//scnNotification->nmhdr.code = NPPN_TB_MODIFICATION;
	//scnNotification->nmhdr.hwndFrom = hwndNpp;
	//scnNotification->nmhdr.idFrom = 0;

	// To notify plugins that the current file is about to be closed
	#define NPPN_FILEBEFORECLOSE				(NPPN_FIRST + 3)
	//scnNotification->nmhdr.code = NPPN_FILEBEFORECLOSE;
	//scnNotification->nmhdr.hwndFrom = hwndNpp;
	//scnNotification->nmhdr.idFrom = 0;


#define	RUNCOMMAND_USER							(WM_USER + 3000)
	#define NPPM_GETFULLCURRENTPATH				(RUNCOMMAND_USER + FULL_CURRENT_PATH)
	#define NPPM_GETCURRENTDIRECTORY			(RUNCOMMAND_USER + CURRENT_DIRECTORY)
	#define NPPM_GETFILENAME					(RUNCOMMAND_USER + FILE_NAME)
	#define NPPM_GETNAMEPART					(RUNCOMMAND_USER + NAME_PART)
	#define NPPM_GETEXTPART						(RUNCOMMAND_USER + EXT_PART)
	#define NPPM_GETCURRENTWORD					(RUNCOMMAND_USER + CURRENT_WORD)
	#define NPPM_GETNPPDIRECTORY				(RUNCOMMAND_USER + NPP_DIRECTORY)

		#define VAR_NOT_RECOGNIZED				0
		#define FULL_CURRENT_PATH				1
		#define CURRENT_DIRECTORY				2
		#define FILE_NAME						3
		#define NAME_PART						4
		#define EXT_PART						5
		#define CURRENT_WORD					6
		#define NPP_DIRECTORY					7


#define WM_DOOPEN						(SCINTILLA_USER   + 8)


#define	SC_MAINHANDLE	0
#define SC_SECHANDLE	1


enum LangType {	L_TXT, L_PHP , L_C, L_CPP, L_CS, L_OBJC, L_JAVA, L_RC,\
			    L_HTML, L_XML, L_MAKEFILE, L_PASCAL, L_BATCH, L_INI, L_NFO, L_USER,\
			    L_ASP, L_SQL, L_VB, L_JS, L_CSS, L_PERL, L_PYTHON, L_LUA,\
			    L_TEX, L_FORTRAN, L_BASH, L_FLASH, L_NSIS, L_TCL, L_LISP, L_SCHEME,\
			    L_ASM, L_DIFF, L_PROPS, L_PS, L_RUBY, L_SMALLTALK, L_VHDL, L_KIX, L_AU3,\
			    L_CAML, L_ADA, L_VERILOG, L_MATLAB, L_HASKELL, L_INNO,\
			    // The end of enumated language type, so it should be always at the end
			    L_END};

enum winVer{WV_UNKNOWN, WV_WIN32S, WV_95, WV_98, WV_ME, WV_NT, WV_W2K, WV_XP, WV_S2003, WV_XPX64, WV_VISTA};

struct NppData {
	HWND _nppHandle;
	HWND _scintillaMainHandle;
	HWND _scintillaSecondHandle;
};


struct ShortcutKey { 
	bool _isCtrl; 
	bool _isAlt; 
	bool _isShift; 
	unsigned char _key; 
};

const int itemNameMaxLen = 64;
typedef void (__cdecl * PFUNCPLUGINCMD)();
typedef struct {
	TCHAR			_itemName[itemNameMaxLen];
	PFUNCPLUGINCMD	_pFunc;
	INT				_cmdID;
	bool			_init2Check;
	ShortcutKey*	_pShKey;
} FuncItem;


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

HIMAGELIST GetSmallImageList(BOOL bSystem);
void ExtractIcons(LPCSTR currentPath, LPCSTR fileName, eDevType type, LPINT iIconNormal, LPINT iIconSelected, LPINT iIconOverlayed);

/* Extended Window Funcions */
void ClientToScreen(HWND hWnd, RECT* rect);
void ScreenToClient(HWND hWnd, RECT* rect);
void ErrorMessage(DWORD err);


// 4 mandatory functions for a plugins
extern "C" __declspec(dllexport) void setInfo(NppData);
extern "C" __declspec(dllexport) const char * getName();
extern "C" __declspec(dllexport) void beNotified(SCNotification *);
extern "C" __declspec(dllexport) void messageProc(UINT Message, WPARAM wParam, LPARAM lParam);
extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *);


#endif //PLUGININTERFACE_H

