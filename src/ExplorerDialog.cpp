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

#include "PluginInterface.h"
#include "ExplorerDialog.h"
#include "ExplorerResource.h"
#include "ContextMenu.h"
#include "NewDlg.h"
#include "Scintilla.h"
#include "ToolTip.h"
#include "resource.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <dbt.h>


ToolTip		toolTip;


BOOL	DEBUG_ON		= FALSE;
#define DEBUG_FLAG(x)	if(DEBUG_ON == TRUE) DEBUG(x);

#ifndef CSIDL_PROFILE
#define CSIDL_PROFILE (0x0028)
#endif

HANDLE g_hEvent[EID_MAX]	= {NULL};
HANDLE g_hThread			= NULL;



DWORD WINAPI UpdateThread(LPVOID lpParam)
{
	BOOL			bRun			= TRUE;
    DWORD			dwWaitResult    = EID_INIT;
	ExplorerDialog*	dlg             = (ExplorerDialog*)lpParam;

	dlg->NotifyEvent(dwWaitResult);

	while (bRun)
	{
		dwWaitResult = ::WaitForMultipleObjects(EID_MAX, g_hEvent, FALSE, INFINITE);

		if (dwWaitResult == EID_THREAD_END)
		{
			bRun = FALSE;
		}
		else
		{
			dlg->NotifyEvent(dwWaitResult);
		}
	}
	return 0;
}


DWORD WINAPI GetVolumeInformationTimeoutThread(LPVOID lpParam)
{
	DWORD			serialNr		= 0;
	DWORD			space			= 0;
	DWORD			flags			= 0;
	tGetVolumeInfo*	volInfo         = (tGetVolumeInfo*)lpParam;

	*volInfo->pIsValidDrive = GetVolumeInformation(volInfo->pszDrivePathName,
		volInfo->pszVolumeName, volInfo->maxSize, &serialNr, &space, &flags, NULL, 0);

	::SetEvent(g_hEvent[EID_GET_VOLINFO]);

	return 0;
}


static ToolBarButtonUnit toolBarIcons[] = {
	
	{IDM_EX_UNDO,			IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1, TBSTYLE_DROPDOWN},
	{IDM_EX_REDO,			IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1, TBSTYLE_DROPDOWN},	 
	 
	//-------------------------------------------------------------------------------------//
	{0,						IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,	IDI_SEPARATOR_ICON, 0},
	//-------------------------------------------------------------------------------------//
	 
    {IDM_EX_FILE_NEW,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1, 0},
    {IDM_EX_FOLDER_NEW,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1, 0},
	{IDM_EX_SEARCH_FIND,	IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1, 0},
	{IDM_EX_GO_TO_FOLDER,	IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1, 0},

	//-------------------------------------------------------------------------------------//
	{0,						IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, IDI_SEPARATOR_ICON, 0},
	//-------------------------------------------------------------------------------------//
	 
	{IDM_EX_UPDATE,			IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1, 0}
};
					
static int stdIcons[] = {IDB_EX_UNDO, IDB_EX_REDO, IDB_EX_FILENEW, IDB_EX_FOLDERNEW, IDB_EX_FIND, IDB_EX_FOLDERGO, IDB_EX_UPDATE};


static ToolBarButtonUnit toolBarIconsNT[] = {
	
	{IDM_EX_UNDO,			IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1, TBSTYLE_DROPDOWN},
	{IDM_EX_REDO,			IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1, TBSTYLE_DROPDOWN},	 
	 
	//-------------------------------------------------------------------------------------//
	{0,						IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, IDI_SEPARATOR_ICON, 0},
	//-------------------------------------------------------------------------------------//
	 
    {IDM_EX_FILE_NEW,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1, 0},
    {IDM_EX_FOLDER_NEW,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1, 0},
	{IDM_EX_SEARCH_FIND,	IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1, 0},
	{IDM_EX_GO_TO_USER,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1, 0},
	{IDM_EX_GO_TO_FOLDER,	IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1, 0},

	//-------------------------------------------------------------------------------------//
	{0,						IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, IDI_SEPARATOR_ICON, 0},
	//-------------------------------------------------------------------------------------//
	 
	{IDM_EX_UPDATE,			IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1, 0}
};
					
static int stdIconsNT[] = {IDB_EX_UNDO, IDB_EX_REDO, IDB_EX_FILENEW, IDB_EX_FOLDERNEW, IDB_EX_FIND, IDB_EX_FOLDERUSER, IDB_EX_FOLDERGO, IDB_EX_UPDATE};

/** 
 *	Note: On change, keep sure to change order of IDM_EX_... also
 */
static LPTSTR szToolTip[23] = {
	"Previous Folder",
	"Next Folder",
	"New File...",
	"New Folder...",
	"Find in Files...",
	"Folder of Current File",
	"User Folder",
	"Refresh"
};


void ExplorerDialog::GetNameStrFromCmd(UINT resID, LPTSTR* tip)
{
	*tip = szToolTip[resID - IDM_EX_UNDO];
}



ExplorerDialog::ExplorerDialog(void) : DockingDlgInterface(IDD_EXPLORER_DLG)
{
	_hTreeCtrl				= NULL;
	_hListCtrl				= NULL;
	_hHeader				= NULL;
	_hFilter				= NULL;
	_isSelNotifyEnable		= TRUE;
	_isLeftButtonDown		= FALSE;
	_hSplitterCursorUpDown	= NULL;
	_bStartupFinish			= FALSE;
	_hFilterButton			= NULL;
	_bOldRectInitilized		= FALSE;
	_bInitFinish			= FALSE;

}

ExplorerDialog::~ExplorerDialog(void)
{
}


void ExplorerDialog::init(HINSTANCE hInst, NppData nppData, tExProp *prop)
{
	_nppData = nppData;
	DockingDlgInterface::init(hInst, nppData._nppHandle);

	_pExProp = prop;
	_FileList.initProp(_pExProp);
}


void ExplorerDialog::doDialog(bool willBeShown)
{
    if (!isCreated())
	{
		create(&_data);

		// define the default docking behaviour
		_data.uMask			= DWS_DF_CONT_LEFT | DWS_ADDINFO | DWS_ICONTAB;
		_data.pszAddInfo	= _pExProp->szCurrentPath;
		_data.hIconTab		= (HICON)::LoadImage(_hInst, MAKEINTRESOURCE(IDI_EXPLORE), IMAGE_ICON, 0, 0, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);
		_data.pszModuleName	= getPluginFileName();
		_data.dlgID			= DOCKABLE_EXPLORER_INDEX;
		::SendMessage(_hParent, NPPM_DMM_REGASDCKDLG, 0, (LPARAM)&_data);
	}

	display(willBeShown);
}


BOOL CALLBACK ExplorerDialog::run_dlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) 
	{
		case WM_INITDIALOG:
		{
			InitialDialog();

			DWORD   dwThreadId   = 0;

			/* create events */
			for (int i = 0; i < EID_MAX; i++)
				g_hEvent[i] = ::CreateEvent(NULL, FALSE, FALSE, NULL);

			/* create thread */
			g_hThread = ::CreateThread(NULL, 0, UpdateThread, this, 0, &dwThreadId);
			break;
		}
		case WM_COMMAND:
		{
			if (((HWND)lParam == _hFilter) && (HIWORD(wParam) == CBN_SELCHANGE))
			{
				::SendMessage(_hSelf, EXM_CHANGECOMBO, 0, 0);
				return TRUE;
			}

			/* Only used on non NT based systems */
			if ((HWND)lParam == _hFilterButton)
			{
				TCHAR	TEMP[MAX_PATH];

				_ComboFilter.getText(TEMP);
				_ComboFilter.addText(TEMP);
				_FileList.filterFiles(TEMP);
			}

			if ((HWND)lParam == _ToolBar.getHSelf())
			{
				tb_cmd(LOWORD(wParam));
				return TRUE;
			}
			break;
		}
		case WM_NOTIFY:
		{
			LPNMHDR		nmhdr = (LPNMHDR)lParam;

			if (nmhdr->hwndFrom == _hTreeCtrl)
			{
				switch (nmhdr->code)
				{
					case NM_RCLICK:
					{
						ContextMenu		cm;
						POINT			pt		= {0};
						TVHITTESTINFO	ht		= {0};
						DWORD			dwpos	= ::GetMessagePos();
						HTREEITEM		hItem	= NULL;

						pt.x = GET_X_LPARAM(dwpos);
						pt.y = GET_Y_LPARAM(dwpos);

						ht.pt = pt;
						::ScreenToClient(_hTreeCtrl, &ht.pt);

						hItem = TreeView_HitTest(_hTreeCtrl, &ht);
						if (hItem != NULL)
						{
							TCHAR	strPathName[MAX_PATH];

							GetFolderPathName(hItem, strPathName);

							cm.SetObjects(strPathName);
							cm.ShowContextMenu(_nppData._nppHandle, _hSelf, pt);
						}
						return TRUE;
					}
					case TVN_SELCHANGED:
					{
						if (_isSelNotifyEnable == TRUE)
						{
							::KillTimer(_hSelf, EXT_UPDATEPATH);
							::SetTimer(_hSelf, EXT_UPDATEPATH, 500, NULL);
						}
						break;
					}
					case TVN_ITEMEXPANDING:
					{
						TVHITTESTINFO	ht		= {0};
						DWORD			dwpos	= ::GetMessagePos();
						HTREEITEM		hItem	= NULL;

						ht.pt.x = GET_X_LPARAM(dwpos);
						ht.pt.y = GET_Y_LPARAM(dwpos);

						::ScreenToClient(_hTreeCtrl, &ht.pt);

						hItem = TreeView_HitTest(_hTreeCtrl, &ht);
						if (hItem != NULL)
						{
							if (!TreeView_GetChild(_hTreeCtrl, hItem)) {
								DrawChildren(hItem);
							} else {
								TCHAR	strPathName[MAX_PATH];
								GetFolderPathName(hItem, strPathName);
								strPathName[strlen(strPathName)-1] = '\0';
								UpdateFolderRecursive(strPathName, hItem);
							}
						}
						break;
					}
					case TVN_KEYDOWN:
					{
						if (((LPNMTVKEYDOWN)lParam)->wVKey == VK_RIGHT)
						{
							HTREEITEM	hItem = TreeView_GetSelection(_hTreeCtrl);
							DrawChildren(hItem);
						}
						return TRUE;
					}
					default:
						break;
				}
			}
			else if ((_bInitFinish == TRUE) && 
				((nmhdr->hwndFrom == _hListCtrl) || (nmhdr->hwndFrom == _hHeader)))
			{
				return _FileList.notify(wParam, lParam);
			}
			else if ((nmhdr->hwndFrom == _ToolBar.getHSelf()) && (nmhdr->code == TBN_DROPDOWN))
			{
				tb_not((LPNMTOOLBAR)lParam);
				return TBDDRET_NODEFAULT;
			}	
			else if (nmhdr->code == TTN_GETDISPINFO)
			{
				LPTOOLTIPTEXT lpttt; 

				lpttt = (LPTOOLTIPTEXT)nmhdr; 
				lpttt->hinst = _hInst; 

				// Specify the resource identifier of the descriptive 
				// text for the given button.
				int resId = int(lpttt->hdr.idFrom);

				LPTSTR	tip	= NULL;
				GetNameStrFromCmd(resId, &tip);
				lpttt->lpszText = tip;
				return TRUE;
			}

			DockingDlgInterface::run_dlgProc(hWnd, Message, wParam, lParam);

		    return FALSE;
		}
		case WM_SIZE:
		case WM_MOVE:
		{
			if (_bStartupFinish == FALSE)
				return TRUE;

			HWND	hWnd		= NULL;
			RECT	rc			= {0};
			RECT	rcWnd		= {0};
			RECT	rcBuff		= {0};

			getClientRect(rc);

			if ((_iDockedPos == CONT_LEFT) || (_iDockedPos == CONT_RIGHT))
			{
				INT		splitterPos	= _pExProp->iSplitterPos;

				if (splitterPos < 50)
					splitterPos = 50;
				else if (splitterPos > (rc.bottom - 100))
					splitterPos = rc.bottom - 100;

				/* set position of toolbar */
				_ToolBar.reSizeTo(rc);
				_Rebar.reSizeTo(rc);

				/* set position of tree control */
				getClientRect(rc);
				rc.top    += 26;
				rc.bottom  = splitterPos;
				::SetWindowPos(_hTreeCtrl, NULL, rc.left, rc.top, rc.right, rc.bottom, SWP_NOZORDER | SWP_SHOWWINDOW);

				/* set splitter */
				getClientRect(rc);
				rc.top	   = (splitterPos + 26);
				rc.bottom  = 6;
				::SetWindowPos(_hSplitterCtrl, NULL, rc.left, rc.top, rc.right, rc.bottom, SWP_NOZORDER | SWP_SHOWWINDOW);

				/* set position of list control */
				getClientRect(rc);
				rc.top	   = (splitterPos + 32);
				rc.bottom -= (splitterPos + 32 + 22);
				::SetWindowPos(_hListCtrl, NULL, rc.left, rc.top, rc.right, rc.bottom, SWP_NOZORDER | SWP_SHOWWINDOW);

				/* set position of filter controls */
				getClientRect(rc);
				rcBuff = rc;

				/* set position of static text */
				if (_hFilterButton == NULL)
				{
					hWnd = ::GetDlgItem(_hSelf, IDC_STATIC_FILTER);
					::GetWindowRect(hWnd, &rcWnd);
					rc.top	     = rcBuff.bottom - 18;
					rc.bottom    = 12;
					rc.left     += 2;
					rc.right     = rcWnd.right - rcWnd.left;
					::SetWindowPos(hWnd, NULL, rc.left, rc.top, rc.right, rc.bottom, SWP_NOZORDER | SWP_SHOWWINDOW);
				}
				else
				{
					rc.top	     = rcBuff.bottom - 21;
					rc.bottom    = 20;
					rc.left     += 2;
					rc.right     = 35;
					::SetWindowPos(_hFilterButton, NULL, rc.left, rc.top, rc.right, rc.bottom, SWP_NOZORDER | SWP_SHOWWINDOW);
				}
				rcBuff.left = rc.right + 4;

				/* set position of combo */
				rc.top		 = rcBuff.bottom - 21;
				rc.bottom	 = 20;
				rc.left		 = rcBuff.left;
				rc.right	 = rcBuff.right - rcBuff.left;
				::SetWindowPos(_hFilter, NULL, rc.left, rc.top, rc.right, rc.bottom, SWP_NOZORDER | SWP_SHOWWINDOW);				
			}
			else
			{
				INT		splitterPos	= _pExProp->iSplitterPosHorizontal;

				if (splitterPos < 50)
					splitterPos = 50;
				else if (splitterPos > (rc.right - 50))
					splitterPos = rc.right - 50;

				/* set position of toolbar */
				rc.right   = splitterPos;
				_ToolBar.reSizeTo(rc);
				_Rebar.reSizeTo(rc);

				/* set position of tree control */
				getClientRect(rc);
				rc.top    += 26;
				rc.bottom -= 26 + 22;
				rc.right   = splitterPos;
				::SetWindowPos(_hTreeCtrl, NULL, rc.left, rc.top, rc.right, rc.bottom, SWP_NOZORDER | SWP_SHOWWINDOW);

				/* set position of filter controls */
				getClientRect(rc);
				rcBuff = rc;

				/* set position of static text */
				if (_hFilterButton == NULL)
				{
					hWnd = ::GetDlgItem(_hSelf, IDC_STATIC_FILTER);
					::GetWindowRect(hWnd, &rcWnd);
					rc.top	     = rcBuff.bottom - 18;
					rc.bottom    = 12;
					rc.left     += 2;
					rc.right     = rcWnd.right - rcWnd.left;
					::SetWindowPos(hWnd, NULL, rc.left, rc.top, rc.right, rc.bottom, SWP_NOZORDER | SWP_SHOWWINDOW);
				}
				else
				{
					rc.top	     = rcBuff.bottom - 21;
					rc.bottom    = 20;
					rc.left     += 2;
					rc.right     = 35;
					::SetWindowPos(_hFilterButton, NULL, rc.left, rc.top, rc.right, rc.bottom, SWP_NOZORDER | SWP_SHOWWINDOW);
				}
				rcBuff.left = rc.right + 4;

				/* set position of combo */
				rc.top		 = rcBuff.bottom - 21;
				rc.bottom	 = 20;
				rc.left		 = rcBuff.left;
				rc.right	 = splitterPos - rcBuff.left;
				::SetWindowPos(_hFilter, NULL, rc.left, rc.top, rc.right, rc.bottom, SWP_NOZORDER | SWP_SHOWWINDOW);

				/* set splitter */
				getClientRect(rc);
				rc.left		 = splitterPos;
				rc.right     = 6;
				::SetWindowPos(_hSplitterCtrl, NULL, rc.left, rc.top, rc.right, rc.bottom, SWP_NOZORDER | SWP_SHOWWINDOW);

				/* set position of list control */
				getClientRect(rc);
				rc.left      = splitterPos + 6;
				rc.right    -= rc.left;
				::SetWindowPos(_hListCtrl, NULL, rc.left, rc.top, rc.right, rc.bottom, SWP_NOZORDER | SWP_SHOWWINDOW);
			}
			break;
		}
		case WM_DRAWITEM:
		{
			DRAWITEMSTRUCT*	pDrawItemStruct	= (DRAWITEMSTRUCT *)lParam;

			if (pDrawItemStruct->hwndItem == _hSplitterCtrl)
			{

				RECT		rc		= pDrawItemStruct->rcItem;
				HDC			hDc		= pDrawItemStruct->hDC;
				HBRUSH		bgbrush	= ::CreateSolidBrush(::GetSysColor(COLOR_BTNFACE));

				/* fill background */
				::FillRect(hDc, &rc, bgbrush);

				::DeleteObject(bgbrush);
				return TRUE;
			}
			break;
		}
		case WM_DESTROY:
		{
			TCHAR	szLastFilter[MAX_PATH];

			::DestroyIcon(_data.hIconTab);
			_ComboFilter.getComboList(_pExProp->vStrFilterHistory);
			_ComboFilter.getText(szLastFilter, MAX_PATH);
			_pExProp->strLastFilter = szLastFilter;

			::SetEvent(g_hEvent[EID_THREAD_END]);

			/* destroy events */
			for (int i = 0; i < EID_MAX; i++)
				::CloseHandle(g_hEvent[i]);

			/* destroy thread */
			::CloseHandle(g_hThread);
			break;
		}
		case EXM_CHANGECOMBO:
		{
			TCHAR	TEMP[MAX_PATH];
			if (_ComboFilter.getSelText(TEMP))
				_FileList.filterFiles(TEMP);
			return TRUE;
		}
		case EXM_OPENDIR:
		{
			if (strlen((LPTSTR)lParam) != 0)
			{
				TCHAR		folderPathName[MAX_PATH]	= "\0";
				TCHAR		folderChildPath[MAX_PATH]	= "\0";

				LPTSTR		szInList		= NULL;
				HTREEITEM	hItem			= TreeView_GetSelection(_hTreeCtrl);

				strcpy(folderPathName, (LPTSTR)lParam);

				if (!((folderPathName[1] == ':') && (folderPathName[2] == '\\')))
				{
					/* get current folder path */
					GetFolderPathName(hItem, folderPathName);
					sprintf(folderPathName, "%s%s\\", folderPathName, (LPTSTR)lParam);

					/* test if selected parent folder */
					if (strcmp((LPTSTR)lParam, "..") == 0)
					{
						/* if so get the parnet folder name and the current one */
						*strrchr(folderPathName, '\\') = '\0';
						*strrchr(folderPathName, '\\') = '\0';
						szInList = strrchr(folderPathName, '\\');

						/* 
						 * if pointer of szInList is smaller as pointer of 
						 * folderPathName, it seems to be a root folder and break
						 */
						if (szInList < folderPathName)
							break;

						strcpy(folderChildPath, &szInList[1]);
						szInList[1] = '\0';
					}
				}

				/* if last char no backslash, add one */
				if (folderPathName[strlen(folderPathName)-1] != '\\')
					strcat(folderPathName, "\\");

				/* select item */
				SelectItem(folderPathName);

				/* set position of selection */
				if (folderChildPath[0] != '\0') {
					_FileList.SelectFolder(folderChildPath);
				} else {
					_FileList.SelectFolder("..");
				}
			}
			return TRUE;
		}
		case EXM_OPENFILE:
		{
			if (strlen((LPTSTR)lParam) != 0)
			{
				TCHAR		folderPathName[MAX_PATH];
				HTREEITEM	hItem = TreeView_GetSelection(_hTreeCtrl);

				/* get current folder path */
				GetFolderPathName(hItem, folderPathName);
				sprintf(folderPathName, "%s%s", folderPathName, (LPTSTR)lParam);

				/* open file */
				::SendMessage(_nppData._nppHandle, WM_DOOPEN, 0, (LPARAM)folderPathName);
			}
			return TRUE;
		}
		case EXM_RIGHTCLICK:
		{
			ContextMenu		cm;
			POINT			pt				= {0};
			vector<string>	files			= *((vector<string>*)lParam);
			HTREEITEM		hItem			= TreeView_GetSelection(_hTreeCtrl);
			DWORD			dwpos			= ::GetMessagePos();
			TCHAR			folderPathName[MAX_PATH];

			GetFolderPathName(hItem, folderPathName);

			pt.x = GET_X_LPARAM(dwpos);
			pt.y = GET_Y_LPARAM(dwpos);

			cm.SetObjects(files);
			cm.ShowContextMenu(_nppData._nppHandle, _hSelf, pt, (strcmp(files[0].c_str(), folderPathName) != 0));

			return TRUE;
		}
		case EXM_UPDATE_PATH :
		{
			TCHAR	pszPath[MAX_PATH];

			_FileList.ToggleStackRec();
			GetFolderPathName(TreeView_GetSelection(_hTreeCtrl), pszPath);
			_FileList.viewPath(pszPath);
			_FileList.ToggleStackRec();

			return TRUE;
		}
		case WM_TIMER:
		{
			if (wParam == EXT_UPDATEDEVICE)
			{
				::KillTimer(_hSelf, EXT_UPDATEDEVICE);
				::SetEvent(g_hEvent[EID_UPDATE_DEVICE]);
			}
			else if (wParam == EXT_UPDATE)
			{
				::KillTimer(_hSelf, EXT_UPDATE);
				::SetEvent(g_hEvent[EID_UPDATE_ACTIVATE]);
			}
			else if (wParam == EXT_UPDATEPATH)
			{
				::KillTimer(_hSelf, EXT_UPDATEPATH);

				TCHAR		strPathName[MAX_PATH];
				HTREEITEM	hItem = TreeView_GetSelection(_hTreeCtrl);

				if (hItem != NULL)
				{
					GetFolderPathName(hItem, strPathName);
					_FileList.viewPath(strPathName, TRUE);
					SetCaption(strPathName);
				}
			}
			return TRUE;
		}
		default:
			return DockingDlgInterface::run_dlgProc(hWnd, Message, wParam, lParam);
	}

	return FALSE;
}

/****************************************************************************
 *	Message handling of header
 */
LRESULT ExplorerDialog::runSplitterProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_LBUTTONDOWN:
		{
			_isLeftButtonDown = TRUE;

			/* set cursor */
			if (_iDockedPos < CONT_TOP)
			{
				::GetCursorPos(&_ptOldPos);
				SetCursor(_hSplitterCursorUpDown);
			}
			else
			{
				::GetCursorPos(&_ptOldPosHorizontal);
				SetCursor(_hSplitterCursorLeftRight);
			}
			break;
		}
		case WM_LBUTTONUP:
		{
			RECT	rc;

			getClientRect(rc);
			_isLeftButtonDown = FALSE;

			/* set cursor */
			if ((_iDockedPos == CONT_LEFT) || (_iDockedPos == CONT_RIGHT))
			{
				SetCursor(_hSplitterCursorUpDown);
				if (_pExProp->iSplitterPos < 50)
				{
					_pExProp->iSplitterPos = 50;
				}
				else if (_pExProp->iSplitterPos > (rc.bottom - 100))
				{
					_pExProp->iSplitterPos = rc.bottom - 100;
				}
			}
			else
			{
				SetCursor(_hSplitterCursorLeftRight);
				if (_pExProp->iSplitterPosHorizontal < 50)
				{
					_pExProp->iSplitterPosHorizontal = 50;
				}
				else if (_pExProp->iSplitterPosHorizontal > (rc.right - 50))
				{
					_pExProp->iSplitterPosHorizontal = rc.right - 50;
				}
			}
			break;
		}
		case WM_MOUSEMOVE:
		{
			if (_isLeftButtonDown == TRUE)
			{
				POINT	pt;
				
				::GetCursorPos(&pt);

				if (_iDockedPos < CONT_TOP)
				{
					if (_ptOldPos.y != pt.y)
					{
						_pExProp->iSplitterPos -= _ptOldPos.y - pt.y;
						::SendMessage(_hSelf, WM_SIZE, 0, 0);
					}
					_ptOldPos = pt;
				}
				else
				{
					if (_ptOldPosHorizontal.x != pt.x)
					{
						_pExProp->iSplitterPosHorizontal -= _ptOldPosHorizontal.x - pt.x;
						::SendMessage(_hSelf, WM_SIZE, 0, 0);
					}
					_ptOldPosHorizontal = pt;
				}
			}

			/* set cursor */
			if (_iDockedPos < CONT_TOP)
				SetCursor(_hSplitterCursorUpDown);
			else
				SetCursor(_hSplitterCursorLeftRight);
			break;
		}
		default:
			break;
	}
	
	return ::CallWindowProc(_hDefaultSplitterProc, hwnd, Message, wParam, lParam);
}

void ExplorerDialog::tb_cmd(UINT message)
{
	switch (message)
	{
		case IDM_EX_UNDO:
		{
			TCHAR	pszPath[MAX_PATH];
			BOOL	dirValid	= TRUE;
			BOOL	selected	= TRUE;

			_FileList.ToggleStackRec();

			do {
				if (dirValid = _FileList.GetPrevDir(pszPath))
					selected = SelectItem(pszPath);
			} while (dirValid && (selected == FALSE));

			if (selected == FALSE)
				_FileList.GetNextDir(pszPath);

			_FileList.ToggleStackRec();
			break;
		}
		case IDM_EX_REDO:
		{
			TCHAR	pszPath[MAX_PATH];
			BOOL	dirValid	= TRUE;
			BOOL	selected	= TRUE;

			_FileList.ToggleStackRec();

			do {
				if (dirValid = _FileList.GetNextDir(pszPath))
					selected = SelectItem(pszPath);
			} while (dirValid && (selected == FALSE));

			if (selected == FALSE)
				_FileList.GetPrevDir(pszPath);

			_FileList.ToggleStackRec();
			break;
		}
		case IDM_EX_FILE_NEW:
		{
			NewDlg		dlg;
			TCHAR		szFileName[MAX_PATH];
			BOOL		bLeave		= FALSE;

			szFileName[0] = '\0';

			dlg.init(_hInst, _hParent);
			while (bLeave == FALSE)
			{
				if (dlg.doDialog(szFileName, "New file") == TRUE)
				{
					/* test if is correct */
					if (IsValidFileName(szFileName))
					{
						TCHAR	pszNewFile[MAX_PATH];

						GetFolderPathName(TreeView_GetSelection(_hTreeCtrl), pszNewFile);
						strcat(pszNewFile, szFileName);
						
						::CloseHandle(::CreateFile(pszNewFile, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));
						::SendMessage(_hParent, WM_DOOPEN, 0, (LPARAM)pszNewFile);

						bLeave = TRUE;
					}
				}
				else
					bLeave = TRUE;
			}
			break;
		}
		case IDM_EX_FOLDER_NEW:
		{
			NewDlg		dlg;
			TCHAR		szFolderName[MAX_PATH];
			BOOL		bLeave			= FALSE;

			szFolderName[0] = '\0';

			dlg.init(_hInst, _hParent);
			while (bLeave == FALSE)
			{
				if (dlg.doDialog(szFolderName, "New folder") == TRUE)
				{
					/* test if is correct */
					if (IsValidFileName(szFolderName))
					{
						TCHAR	pszNewFolder[MAX_PATH];

						GetFolderPathName(TreeView_GetSelection(_hTreeCtrl), pszNewFolder);
						strcat(pszNewFolder, szFolderName);
						
						if (::CreateDirectory(pszNewFolder, NULL) == FALSE)
						{
							::MessageBox(_hSelf, "Folder couldn't be created.", "Error", MB_OK);
						}

						bLeave = TRUE;
					}
				}
				else
					bLeave = TRUE;
			}
			break;
		}
		case IDM_EX_SEARCH_FIND:
		{
			TCHAR	pszPath[MAX_PATH];
			GetFolderPathName(TreeView_GetSelection(_hTreeCtrl), pszPath);
			::SendMessage(_hParent, NPPM_LAUNCH_FINDINFILESDLG, (WPARAM)pszPath, NULL);
			break;
		}
		case IDM_EX_GO_TO_USER:
		{
			TCHAR	pathName[MAX_PATH];

			if (SHGetSpecialFolderPath(_hSelf, pathName, CSIDL_PROFILE, FALSE) == TRUE) {
				strcat(pathName, "\\");
				SelectItem(pathName);
			}
			break;
		}
		case IDM_EX_GO_TO_FOLDER:
		{
			TCHAR	pathName[MAX_PATH];
			::SendMessage(_hParent, NPPM_GETCURRENTDIRECTORY, 0, (LPARAM)pathName);
			strcat(pathName, "\\");
			SelectItem(pathName);
			_FileList.SelectCurFile();
			break;
		}
		case IDM_EX_UPDATE:
		{
			::SetEvent(g_hEvent[EID_UPDATE_USER]);
			break;
		}
		default:
			break;
	}
}

void ExplorerDialog::tb_not(LPNMTOOLBAR lpnmtb)
{
	INT			i = 0;
	TCHAR		TEMP[MAX_PATH];
	LPSTR		*pszPathes;
	INT			iElements	= 0;
		
	_FileList.ToggleStackRec();

	/* get element cnt */
	if (lpnmtb->iItem == IDM_EX_UNDO) {
		iElements = _FileList.GetPrevDirs(NULL);
	} else if (lpnmtb->iItem == IDM_EX_REDO) {
		iElements = _FileList.GetNextDirs(NULL);
	}

	/* allocate elements */
	pszPathes	= (LPSTR*)new LPSTR[iElements];
	for (i = 0; i < iElements; i++)
		pszPathes[i] = (LPSTR)new TCHAR[MAX_PATH];

	/* get directories */
	if (lpnmtb->iItem == IDM_EX_UNDO) {
		_FileList.GetPrevDirs(pszPathes);
	} else if (lpnmtb->iItem == IDM_EX_REDO) {
		_FileList.GetNextDirs(pszPathes);
	}

	POINT	pt		= {0};
	HMENU	hMenu	= ::CreatePopupMenu();

	for (i = 0; i < iElements; i++)
	{
		/* test if folder exists */
		struct _stat st = {0};
		strcpy(TEMP, pszPathes[i]);
		TEMP[strlen(TEMP)-1] = '\0';
		if (!_stat(TEMP, &st))
			::AppendMenu(hMenu, MF_STRING, i+1, pszPathes[i]);
	}

	::GetCursorPos(&pt);
	INT cmd = ::TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, _hSelf, NULL);
	::DestroyMenu(hMenu);
	_Rebar.redraw();

	/* select element */
	if (cmd) {
		SelectItem(pszPathes[cmd-1]);
		_FileList.OffsetItr(lpnmtb->iItem == IDM_EX_UNDO ? -cmd : cmd);
	}

	for (i = 0; i < iElements; i++)
		delete [] pszPathes[i];
	delete [] pszPathes;

	_FileList.ToggleStackRec();
}

void ExplorerDialog::NotifyEvent(DWORD event)
{
	POINT	pt;

    LONG oldCur = ::SetClassLong(_hSelf, GCL_HCURSOR, (LONG)_hCurWait);
    ::EnableWindow(_hSelf, FALSE);
	::GetCursorPos(&pt);
	::SetCursorPos(pt.x, pt.y);

	switch(event)	
	{
		case EID_INIT :
		{
			/* initial tree */
			UpdateDevices();

			/* initilize file list */
			_FileList.SetToolBarInfo(&_ToolBar , IDM_EX_UNDO, IDM_EX_REDO);

			/* set data */
			SelectItem(_pExProp->szCurrentPath);

			/* initilize combo */
			_ComboFilter.setComboList(_pExProp->vStrFilterHistory);
			_ComboFilter.addText("*.*");
			_ComboFilter.setText((LPTSTR)_pExProp->strLastFilter.c_str());
			_FileList.filterFiles((LPTSTR)_pExProp->strLastFilter.c_str());

			/* Update "Go to Folder" icon */
			NotifyNewFile();

			/* finish with init */
			_bInitFinish = TRUE;
			break;
		}
		case EID_UPDATE_DEVICE :
		{
			UpdateDevices();
			break;
		}
		case EID_UPDATE_USER :
		{
			/* No break!! */
			UpdateDevices();
		}
		case EID_UPDATE_ACTIVATE :
		{
			UpdateFolders();
			::SendMessage(_hSelf, EXM_UPDATE_PATH, 0, 0);
			break;
		}
		default:
			break;
	}

    ::SetClassLong(_hSelf, GCL_HCURSOR, oldCur);
	::EnableWindow(_hSelf, TRUE);
	::GetCursorPos(&pt);
	::SetCursorPos(pt.x, pt.y);
}


void ExplorerDialog::InitialDialog(void)
{
	/* get handle of dialogs */
	_hTreeCtrl		= ::GetDlgItem(_hSelf, IDC_TREE_FOLDER);
	_hListCtrl		= ::GetDlgItem(_hSelf, IDC_LIST_FILE);
	_hHeader		= ListView_GetHeader(_hListCtrl);
	_hSplitterCtrl	= ::GetDlgItem(_hSelf, IDC_BUTTON_SPLITTER);
	_hFilter		= ::GetDlgItem(_hSelf, IDC_COMBO_FILTER);

	if (gWinVersion < WV_NT) {
		_hFilterButton = ::GetDlgItem(_hSelf, IDC_BUTTON_FILTER);
		::DestroyWindow(::GetDlgItem(_hSelf, IDC_STATIC_FILTER));
	} else {
		::DestroyWindow(::GetDlgItem(_hSelf, IDC_BUTTON_FILTER));
	}

	/* subclass splitter */
	_hSplitterCursorUpDown		= ::LoadCursor(_hInst, MAKEINTRESOURCE(IDC_UPDOWN));
	_hSplitterCursorLeftRight	= ::LoadCursor(_hInst, MAKEINTRESOURCE(IDC_LEFTRIGHT));
	::SetWindowLong(_hSplitterCtrl, GWL_USERDATA, reinterpret_cast<LONG>(this));
	_hDefaultSplitterProc = reinterpret_cast<WNDPROC>(::SetWindowLong(_hSplitterCtrl, GWL_WNDPROC, reinterpret_cast<LONG>(wndDefaultSplitterProc)));

	/* Load Image List */
	::SendMessage(_hTreeCtrl, TVM_SETIMAGELIST, TVSIL_NORMAL, (LPARAM)GetSmallImageList(_pExProp->bUseSystemIcons));

	/* initial file list */
	_FileList.init(_hInst, _hSelf, _hListCtrl);

	/* create toolbar */
	if (gWinVersion < WV_NT) {
		_ToolBar.init(_hInst, _hSelf, 16, toolBarIcons, sizeof(toolBarIcons)/sizeof(ToolBarButtonUnit), true, stdIcons, sizeof(stdIcons)/sizeof(int));
	} else {
		_ToolBar.init(_hInst, _hSelf, 16, toolBarIconsNT, sizeof(toolBarIconsNT)/sizeof(ToolBarButtonUnit), true, stdIconsNT, sizeof(stdIconsNT)/sizeof(int));
	}
	_ToolBar.display();
	_Rebar.init(_hInst, _hSelf, &_ToolBar);
	_Rebar.display();

	/* initial combo */
	_ComboFilter.init(_hFilter);

	/* load cursor */
	_hCurWait = ::LoadCursor(NULL, IDC_WAIT);
}

void ExplorerDialog::SetCaption(LPTSTR path)
{
	if (isVisible())
	{
		/* store current path and update caption */
		strcpy(_pExProp->szCurrentPath, path);
		updateDockingDlg();
	}
}

BOOL ExplorerDialog::SelectItem(LPTSTR path)
{
	BOOL				folderExists	= FALSE;

	TCHAR				TEMP[MAX_PATH];
	TCHAR				pszItemName[MAX_PATH];
	TCHAR				pszShortPath[MAX_PATH];
	TCHAR				pszCurrPath[MAX_PATH];
	BOOL				isRoot			= TRUE;
	HTREEITEM			hItem			= TreeView_GetRoot(_hTreeCtrl);
	HTREEITEM			hItemSel		= NULL;
	HTREEITEM			hLastItem		= NULL;

	/* test if folder exists */
	struct _stat st = {0};
	strcpy(TEMP, path);
	TEMP[strlen(TEMP)-1] = '\0';
	folderExists = (_stat(TEMP, &st) ? FALSE : TRUE);

	if (folderExists == TRUE)
	{
		/* empty pszCurrPath */
		pszCurrPath[0] = '\0';

		/* disabled detection of TVN_SELCHANGED notification */
		_isSelNotifyEnable = FALSE;

		do
		{
			GetItemText(hItem, pszItemName, MAX_PATH);

			/* truncate item name if we are in root */
			if (isRoot == TRUE)
				pszItemName[2] = '\0';

			/* create temporary path name in long and short form */
			sprintf(TEMP, "%s%s\\", pszCurrPath, pszItemName);
			GetShortPathName(TEMP, pszShortPath, MAX_PATH);
			size_t	length	= strlen(TEMP);

			if ((strnicmp(path, TEMP, length) == 0) || 
				(strnicmp(path, pszShortPath, length) == 0))
			{
				/* found -> store item for correct selection */
				hItemSel = hItem;

				/* expand, if possible and get child item */
				if (TreeView_GetChild(_hTreeCtrl, hItem) == NULL)
				{
					/* if no child item available, draw them */
					TreeView_SelectItem(_hTreeCtrl, hItem);
					DrawChildren(hItem);
				}
				hItem = TreeView_GetChild(_hTreeCtrl, hItem);

				/* store item */
				hLastItem = hItem;

				/* set current selected path */
				strcpy(pszCurrPath, TEMP);

				/* only on first case it is a root */
				isRoot = FALSE;
			}
			else
			{
				/* search for next item in list */
				hItem = TreeView_GetNextItem(_hTreeCtrl, hItem, TVGN_NEXT);
			}
		} while (hItem != NULL);

		/* view path */
		if (pszCurrPath[0] != '\0')
		{
			/* select last selected item */
			TreeView_SelectItem(_hTreeCtrl, hItemSel);
			TreeView_EnsureVisible(_hTreeCtrl, hItemSel);

			_FileList.viewPath(pszCurrPath, TRUE);
			SetCaption(pszCurrPath);
		}

		/* enable detection of TVN_SELCHANGED notification */
		_isSelNotifyEnable = TRUE;
	}

	return folderExists;
}


/**
 *	UpdateDevices()
 */
void ExplorerDialog::UpdateDevices(void)
{
	BOOL			bDefaultDevice  = FALSE;
	DWORD			driveList		= ::GetLogicalDrives();
	BOOL			isValidDrive	= FALSE;

	HTREEITEM		hCurrentItem	= TreeView_GetNextItem(_hTreeCtrl, TVI_ROOT, TVGN_CHILD);

	TCHAR			drivePathName[]	= " :\\\0\0";	// it is longer for function 'HaveChildren()'
	TCHAR			TEMP[MAX_PATH];
	TCHAR			volumeName[MAX_PATH];

	for (int i = 1; i < 32; i++)
	{
		drivePathName[0] = 'A' + i;

		if (0x01 & (driveList >> i))
		{
			/* create volume name */
			isValidDrive = ExploreVolumeInformation(drivePathName, TEMP, MAX_PATH);
			if (isValidDrive == TRUE)
			{
				sprintf(volumeName, "%c: [%s]", 'A' + i, TEMP);
			}
			else
			{
				sprintf(volumeName, "%c:", 'A' + i);
			}

			if (hCurrentItem != NULL)
			{
				/* get current volume name in list and test if name is changed */
				GetItemText(hCurrentItem, TEMP, MAX_PATH);

				if (strcmp(volumeName, TEMP) == 0)
				{
					/* if names are equal, go to next item in tree */
					hCurrentItem = TreeView_GetNextItem(_hTreeCtrl, hCurrentItem, TVGN_NEXT);
				}
				else if (volumeName[0] == TEMP[0])
				{
					BOOL		haveChildren	= FALSE;

					/* if names are not the same but the drive letter are equal, rename item */
					int			iIconNormal		= 0;
					int			iIconSelected	= 0;
					int			iIconOverlayed	= 0;

					/* if device was removed, get child is not necessary */
					if (isValidDrive == TRUE)
					{
						/* have children */
						haveChildren = HaveChildren(drivePathName);
						/* correct modified drivePathName */
						drivePathName[3]			= '\0';
					}

					/* get icons */
					ExtractIcons(drivePathName, NULL, DEVT_DRIVE, &iIconNormal, &iIconSelected, &iIconOverlayed);
					UpdateItem(hCurrentItem, volumeName, iIconNormal, iIconSelected, iIconOverlayed, 0, haveChildren);
					DeleteChildren(hCurrentItem);
					hCurrentItem = TreeView_GetNextItem(_hTreeCtrl, hCurrentItem, TVGN_NEXT);
				}
				else
				{
					/* insert the device when new and not present before */
					HTREEITEM	hItem	= TreeView_GetNextItem(_hTreeCtrl, hCurrentItem, TVGN_PREVIOUS);
					InsertChildFolder(volumeName, TVI_ROOT, hItem, isValidDrive);
				}
			}
			else
			{
				InsertChildFolder(volumeName, TVI_ROOT, TVI_LAST, isValidDrive);
			}
		}
		else
		{
			if (hCurrentItem != NULL)
			{
				/* get current volume name in list and test if name is changed */
				GetItemText(hCurrentItem, TEMP, MAX_PATH);

				if (drivePathName[0] == TEMP[0])
				{
					HTREEITEM	pPrevItem	= hCurrentItem;
					hCurrentItem = TreeView_GetNextItem(_hTreeCtrl, hCurrentItem, TVGN_NEXT);
					TreeView_DeleteItem(_hTreeCtrl, pPrevItem);
				}
			}
		}
	}
}

void ExplorerDialog::UpdateFolders(void)
{
	TCHAR			pszPath[MAX_PATH];
	HTREEITEM		hCurrentItem	= TreeView_GetChild(_hTreeCtrl, TVI_ROOT);

	while (hCurrentItem != NULL)
	{
		if (TreeView_GetItemState(_hTreeCtrl, hCurrentItem, TVIS_EXPANDED) & TVIS_EXPANDED)
		{
			GetItemText(hCurrentItem, pszPath, MAX_PATH);
			pszPath[2] = '\0';
			UpdateFolderRecursive(pszPath, hCurrentItem);
		}
		hCurrentItem = TreeView_GetNextItem(_hTreeCtrl, hCurrentItem, TVGN_NEXT);
	}
}

void ExplorerDialog::UpdateFolderRecursive(LPTSTR pszParentPath, HTREEITEM hParentItem)
{
	WIN32_FIND_DATA		Find			= {0};
	HANDLE				hFind			= NULL;
	TVITEM				item			= {0};
	
	vector<tItemList>	vFolderList;
	tItemList			listElement;
	TCHAR				pszItem[MAX_PATH];
	TCHAR				pszPath[MAX_PATH];
	TCHAR				pszSearch[MAX_PATH];
	HTREEITEM			hCurrentItem	= TreeView_GetNextItem(_hTreeCtrl, hParentItem, TVGN_CHILD);

	strcpy(pszSearch, pszParentPath);

	/* add wildcard */
	strcat(pszSearch, "\\*");

	if ((hFind = ::FindFirstFile(pszSearch, &Find)) == INVALID_HANDLE_VALUE)
		return;

	/* find folders */
	do
	{
		if (IsValidFolder(Find) == TRUE)
		{
			listElement.strName			= Find.cFileName;
			listElement.dwAttributes	= Find.dwFileAttributes;
			vFolderList.push_back(listElement);
		}
	} while (FindNextFile(hFind, &Find));

	::FindClose(hFind);

	/* sort data */
	QuickSortItems(&vFolderList, 0, vFolderList.size()-1);

	/* update tree */
	for (size_t i = 0; i < vFolderList.size(); i++)
	{
		if (GetItemText(hCurrentItem, pszItem, MAX_PATH) == TRUE)
		{
			/* compare current item and the current folder name */
			while ((strcmp(pszItem, vFolderList[i].strName.c_str()) != 0) && (hCurrentItem != NULL))
			{
				HTREEITEM	pPrevItem = NULL;

				/* if it's not equal delete or add new item */
				if (FindFolderAfter((LPTSTR)vFolderList[i].strName.c_str(), hCurrentItem) == TRUE)
				{
					pPrevItem		= hCurrentItem;
					hCurrentItem	= TreeView_GetNextItem(_hTreeCtrl, hCurrentItem, TVGN_NEXT);
					TreeView_DeleteItem(_hTreeCtrl, pPrevItem);
				}
				else
				{
					pPrevItem = TreeView_GetNextItem(_hTreeCtrl, hCurrentItem, TVGN_PREVIOUS);

					/* Note: If hCurrentItem is the first item in the list pPrevItem is NULL */
					if (pPrevItem == NULL)
						hCurrentItem = InsertChildFolder((LPTSTR)vFolderList[i].strName.c_str(), hParentItem, TVI_FIRST);
					else
						hCurrentItem = InsertChildFolder((LPTSTR)vFolderList[i].strName.c_str(), hParentItem, pPrevItem);
				}

				if (hCurrentItem != NULL)
					GetItemText(hCurrentItem, pszItem, MAX_PATH);
			}

			/* get current path */
			sprintf(pszPath, "%s\\%s", pszParentPath, pszItem);

			/* update icons and expandable information */
			int					iIconNormal		= 0;
			int					iIconSelected	= 0;
			int					iIconOverlayed	= 0;
			BOOL				haveChildren	= HaveChildren(pszPath);
			BOOL				bHidden			= FALSE;

			/* correct by HaveChildren() modified pszPath */
			pszPath[strlen(pszPath) - 2] = '\0';

			/* get icons and update item */
			ExtractIcons(pszPath, NULL, DEVT_DIRECTORY, &iIconNormal, &iIconSelected, &iIconOverlayed);
			bHidden = ((vFolderList[i].dwAttributes & FILE_ATTRIBUTE_HIDDEN) != 0);
			UpdateItem(hCurrentItem, pszItem, iIconNormal, iIconSelected, iIconOverlayed, bHidden, haveChildren);

			/* update recursive */
			if (TreeView_GetItemState(_hTreeCtrl, hCurrentItem, TVIS_EXPANDED) & TVIS_EXPANDED)
			{
				UpdateFolderRecursive(pszPath, hCurrentItem);
			}

			/* select next item */
			hCurrentItem = TreeView_GetNextItem(_hTreeCtrl, hCurrentItem, TVGN_NEXT);
		}
		else
		{
			hCurrentItem = InsertChildFolder((LPTSTR)vFolderList[i].strName.c_str(), hParentItem);
			hCurrentItem = TreeView_GetNextItem(_hTreeCtrl, hCurrentItem, TVGN_NEXT);
		}
	}

	/* delete possible not existed items */
	while (hCurrentItem != NULL)
	{
		HTREEITEM	pPrevItem	= hCurrentItem;
		hCurrentItem			= TreeView_GetNextItem(_hTreeCtrl, hCurrentItem, TVGN_NEXT);
		TreeView_DeleteItem(_hTreeCtrl, pPrevItem);
	}
}

BOOL ExplorerDialog::FindFolderAfter(LPTSTR itemName, HTREEITEM pAfterItem)
{
	TCHAR		pszItem[MAX_PATH];
	BOOL		isFound			= FALSE;
	HTREEITEM	hCurrentItem	= TreeView_GetNextItem(_hTreeCtrl, pAfterItem, TVGN_NEXT);

	while (hCurrentItem != NULL)
	{
		GetItemText(hCurrentItem, pszItem, MAX_PATH);
		if (strcmp(itemName, pszItem) == 0)
		{
			isFound = TRUE;
			hCurrentItem = NULL;
		}
		else
		{
			hCurrentItem = TreeView_GetNextItem(_hTreeCtrl, hCurrentItem, TVGN_NEXT);
		}
	}

	return isFound;
}


void ExplorerDialog::GetFolderPathName(HTREEITEM currentItem, LPTSTR folderPathName)
{
	/* return if current folder is root folder */
	if (currentItem == TVI_ROOT)
		return;

	/* delete folder path name */
	folderPathName[0]	= '\0';

	/* create temp resources */
	TCHAR	TEMP[MAX_PATH];
	TCHAR	szName[MAX_PATH];

	/* join elements together */
	while (currentItem != NULL)
	{
		GetItemText(currentItem, szName, MAX_PATH);
		sprintf(TEMP, "%s\\%s", szName, folderPathName);
		strcpy(folderPathName, TEMP);
		currentItem = TreeView_GetNextItem(_hTreeCtrl, currentItem, TVGN_PARENT);
	}

	if (strlen(folderPathName) > 3)
	{
		/* remove drive name */
		strcpy(TEMP, folderPathName);
		for (int i = 3; TEMP[i] != '\\'; i++);

		sprintf(folderPathName, "%c:%s", TEMP[0], &TEMP[i]);
	}
}


void ExplorerDialog::NotifyNewFile(void)
{
	if (isCreated())
	{
		TCHAR	TEMP[MAX_PATH];
		::SendMessage(_hParent, NPPM_GETCURRENTDIRECTORY, 0, (LPARAM)TEMP);
		_ToolBar.enable(IDM_EX_GO_TO_FOLDER, (strlen(TEMP) != 0));
	}
}


BOOL ExplorerDialog::ExploreVolumeInformation(LPCTSTR pszDrivePathName, LPTSTR pszVolumeName, UINT maxSize)
{
	tGetVolumeInfo	volInfo;
	DWORD			dwThreadId		= 0;
    DWORD			dwWaitResult    = 0;
	BOOL			isValidDrive	= FALSE;
	HANDLE			hThread			= NULL;

	volInfo.pszDrivePathName	= pszDrivePathName;
	volInfo.pszVolumeName		= pszVolumeName;
	volInfo.maxSize				= maxSize;
	volInfo.pIsValidDrive		= &isValidDrive;

	hThread = ::CreateThread(NULL, 0, GetVolumeInformationTimeoutThread, &volInfo, 0, &dwThreadId);
	dwWaitResult = ::WaitForSingleObject(g_hEvent[EID_GET_VOLINFO], _pExProp->uTimeout);

	if (dwWaitResult == WAIT_TIMEOUT)
	{
		::TerminateThread(hThread, 0);
		isValidDrive = FALSE;
	}
	::CloseHandle(hThread);

	return isValidDrive;
}
