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


static ToolBarButtonUnit toolBarIcons[] = {
	
	{IDM_EX_UNDO,			IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1},
	{IDM_EX_REDO,			IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1},	 
	 
	//-------------------------------------------------------------------------------------//
	{0,						IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, IDI_SEPARATOR_ICON},
	//-------------------------------------------------------------------------------------//
	 
    {IDM_EX_FILE_NEW,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1},
    {IDM_EX_FOLDER_NEW,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1},
	{IDM_EX_SEARCH_FIND,	IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1},
	{IDM_EX_GO_TO_FOLDER,	IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1},

	//-------------------------------------------------------------------------------------//
	{0,						IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, IDI_SEPARATOR_ICON},
	//-------------------------------------------------------------------------------------//
	 
	{IDM_EX_UPDATE,			IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1}
};
					
static int stdIcons[] = {IDB_EX_UNDO, IDB_EX_REDO, IDB_EX_FILENEW, IDB_EX_FOLDERNEW, IDB_EX_FIND, IDB_EX_FOLDERGO, IDB_EX_UPDATE};


static ToolBarButtonUnit toolBarIconsNT[] = {
	
	{IDM_EX_UNDO,			IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1},
	{IDM_EX_REDO,			IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1},	 
	 
	//-------------------------------------------------------------------------------------//
	{0,						IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, IDI_SEPARATOR_ICON},
	//-------------------------------------------------------------------------------------//
	 
    {IDM_EX_FILE_NEW,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1},
    {IDM_EX_FOLDER_NEW,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1},
	{IDM_EX_SEARCH_FIND,	IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1},
	{IDM_EX_GO_TO_USER,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1},
	{IDM_EX_GO_TO_FOLDER,	IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1},

	//-------------------------------------------------------------------------------------//
	{0,						IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, IDI_SEPARATOR_ICON},
	//-------------------------------------------------------------------------------------//
	 
	{IDM_EX_UPDATE,			IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1}
};
					
static int stdIconsNT[] = {IDB_EX_UNDO, IDB_EX_REDO, IDB_EX_FILENEW, IDB_EX_FOLDERNEW, IDB_EX_FIND, IDB_EX_FOLDERUSER, IDB_EX_FOLDERGO, IDB_EX_UPDATE};

/** 
 *	Note: On change, keep sure to change order of IDM_EX_... also
 */
static char* szToolTip[23] = {
	"Previous Folder",
	"Next Folder",
	"New File...",
	"New Folder...",
	"Find in Files...",
	"Folder of Current File",
	"User Folder",
	"Refresh"
};


void ExplorerDialog::GetNameStrFromCmd(UINT resID, char** tip)
{
	*tip = szToolTip[resID - IDM_EX_UNDO];
}



ExplorerDialog::ExplorerDialog(void) : DockingDlgInterface(IDD_EXPLORER_DLG)
{
	_wasVisible			= FALSE;
	_hTreeCtrl			= NULL;
	_hListCtrl			= NULL;
	_hFilter			= NULL;
	_isSelNotifyEnable	= TRUE;
	_isLeftButtonDown	= FALSE;
	_hSplitterCursorUpDown	= NULL;
	_bStartupFinish		= FALSE;
	_hFilterButton		= NULL;
}

ExplorerDialog::~ExplorerDialog(void)
{
}


void ExplorerDialog::init(HINSTANCE hInst, NppData nppData, tExProp *prop)
{
	_nppData = nppData;
	DockingDlgInterface::init(hInst, nppData._nppHandle);

	_pExProp = prop;
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
		::SendMessage(_hParent, WM_DMM_REGASDCKDLG, 0, (LPARAM)&_data);

		/* initial tree */
		UpdateDevices();

		/* initilize file list */
		_FileList.SetToolBarInfo(&_ToolBar , IDM_EX_UNDO, IDM_EX_REDO);

		/* set data */
		SelectItem(_pExProp->szCurrentPath);

		/* initilize combo */
		_ComboFilter.setComboList(_pExProp->vStrFilterHistory);
		_ComboFilter.addText("*.*");

		/* Update "Go to Folder" icon */
		NotifyNewFile();

		_wasVisible = TRUE;
	}

	display(willBeShown);
}


BOOL CALLBACK ExplorerDialog::run_dlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) 
	{
		case WM_INITDIALOG:
		{
			/* get handle of dialogs */
			_hTreeCtrl		= ::GetDlgItem(_hSelf, IDC_TREE_FOLDER);
			_hListCtrl		= ::GetDlgItem(_hSelf, IDC_LIST_FILE);
			_hSplitterCtrl	= ::GetDlgItem(_hSelf, IDC_BUTTON_SPLITTER);
			_hFilter		= ::GetDlgItem(_hSelf, IDC_COMBO_FILTER);

			if (GetVersion() & 0x80000000)
			{
				_hFilterButton = ::GetDlgItem(_hSelf, IDC_BUTTON_FILTER);
				::DestroyWindow(::GetDlgItem(_hSelf, IDC_STATIC_FILTER));
			}
			else
			{
				::DestroyWindow(::GetDlgItem(_hSelf, IDC_BUTTON_FILTER));
			}

			InitialDialog();
			break;
		}
		case WM_COMMAND:
		{
			if (((HWND)lParam == _hFilter) && (HIWORD(wParam) == CBN_SELCHANGE))
			{
				char	TEMP[MAX_PATH];

				if (_ComboFilter.getSelText(TEMP))
					_FileList.filterFiles(TEMP);
				return TRUE;
			}

			/* Only used on non NT based systems */
			if ((HWND)lParam == _hFilterButton)
			{
				char*	TEMP = (char*)new char[MAX_PATH];

				_ComboFilter.getText(TEMP);
				_ComboFilter.addText(TEMP);
				_FileList.filterFiles(TEMP);
				delete [] TEMP;
			}

			if ((HWND)lParam == _ToolBar.getHSelf())
			{
				tb_cmd(LOWORD(wParam));
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
						POINT			pt			= {0};
						TVHITTESTINFO	ht			= {0};
						DWORD			dwpos		= ::GetMessagePos();
						HTREEITEM		hItem		= NULL;

						pt.x = GET_X_LPARAM(dwpos);
						pt.y = GET_Y_LPARAM(dwpos);

						ht.pt = pt;
						::ScreenToClient(_hTreeCtrl, &ht.pt);

						hItem = TreeView_HitTest(_hTreeCtrl, &ht);
						if (hItem != NULL)
						{
							char*	strPathName	= (char*)new char[MAX_PATH];

							GetFolderPathName(hItem, strPathName);

							cm.SetObjects(strPathName);
							cm.ShowContextMenu(_nppData._nppHandle, _hSelf, pt);

							delete [] strPathName;
						}
						break;
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
						POINT			pt			= {0};
						TVHITTESTINFO	ht			= {0};
						DWORD			dwpos		= ::GetMessagePos();
						HTREEITEM		hItem		= NULL;

						pt.x = GET_X_LPARAM(dwpos);
						pt.y = GET_Y_LPARAM(dwpos);

						ht.pt = pt;
						::ScreenToClient(_hTreeCtrl, &ht.pt);

						hItem = TreeView_HitTest(_hTreeCtrl, &ht);

						if (hItem != NULL)
						{
							if (!TreeView_GetChild(_hTreeCtrl, hItem))
							{
								DrawChildren(hItem);
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
						break;
					}
					default:
						break;
				}
			}
			else if (nmhdr->hwndFrom == _hListCtrl)
			{
				_FileList.notify(wParam, lParam);
			}
			else if (nmhdr->code == TTN_GETDISPINFO)
			{
				LPTOOLTIPTEXT lpttt; 

				lpttt = (LPTOOLTIPTEXT)nmhdr; 
				lpttt->hinst = _hInst; 

				// Specify the resource identifier of the descriptive 
				// text for the given button.
				int resId = int(lpttt->hdr.idFrom);

				char*	tip	= NULL;
				GetNameStrFromCmd(resId, &tip);
				lpttt->lpszText = tip;
			}

			DockingDlgInterface::run_dlgProc(hWnd, Message, wParam, lParam);

		    return FALSE;
		}
		case WM_SIZE:
		case WM_MOVE:
		{
			HWND	hWnd		= NULL;
			RECT	rc			= {0};
			RECT	rcWnd		= {0};
			RECT	rcBuff		= {0};

			if ((_iDockedPos == CONT_LEFT) || (_iDockedPos == CONT_RIGHT))
			{
				INT		splitterPos	= _pExProp->iSplitterPos;

				if (_wasVisible == FALSE)
				{
					/* init "old" client rect size */
					getClientRect(_rcOldSize);				
				}

				/* triggered by notification NPPN_READY */
				if (_bStartupFinish == TRUE)
				{
					/* set splitter position (%) */
					getClientRect(rc);
					_pExProp->iSplitterPos -= ((_rcOldSize.bottom - rc.bottom) / 2);
					splitterPos	= _pExProp->iSplitterPos;
				}

				/* store old client rect */
				getClientRect(_rcOldSize);

				/* correct real splitter position */
				if (splitterPos < 0)
				{
					splitterPos = 0;
				}
				else if (splitterPos > (rc.bottom - 54))
				{
					splitterPos = rc.bottom - 54;
				}

				/* set position of toolbar */
				getClientRect(rc);
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

				if (_wasVisible == FALSE)
				{
					/* init "old" client rect size */
					getClientRect(_rcOldSizeHorizontal);				
				}

				/* triggered by notification NPPN_READY */
				if (_bStartupFinish == TRUE)
				{
//					/* set splitter position (%) */
					getClientRect(rc);
//					_pExProp->iSplitterPosHorizontal -= ((_rcOldSizeHorizontal.right - rc.right) / 2);
					splitterPos	= _pExProp->iSplitterPosHorizontal;
				}

				/* store old client rect */
				getClientRect(_rcOldSizeHorizontal);

				/* correct real splitter position */
				if (splitterPos < 0)
				{
					splitterPos = 0;
				}
				else if (splitterPos > (rc.right - 6))
				{
					splitterPos = rc.right - 6;
				}

				/* set position of toolbar */
				getClientRect(rc);
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
			::DestroyIcon(_data.hIconTab);
			_ComboFilter.getComboList(_pExProp->vStrFilterHistory);
			break;
		}
		case EXM_CHANGECOMBO:
		{
			char*	TEMP = (char*)new char[MAX_PATH];

			_ComboFilter.getText(TEMP);
			_FileList.filterFiles(TEMP);
			delete [] TEMP;

			return TRUE;
		}
		case EXM_OPENDIR:
		{
			if (strlen((char*)lParam) != 0)
			{
				char*		szInList		= NULL;
				char*		folderChildPath	= NULL;
				char*		folderPathName	= (char*) new char[MAX_PATH];
				HTREEITEM	hItem = TreeView_GetSelection(_hTreeCtrl);

				strcpy(folderPathName, (char*)lParam);

				if (!((folderPathName[1] == ':') && (folderPathName[2] == '\\')))
				{
					/* get current folder path */
					GetFolderPathName(hItem, folderPathName);
					sprintf(folderPathName, "%s%s\\", folderPathName, (char*)lParam);

					/* test if selected parent folder */
					if (strcmp((char*)lParam, "..") == 0)
					{
						folderChildPath = (char*) new char[MAX_PATH];

						/* if so get the parnet folder name and the current one */
						*strrchr(folderPathName, '\\') = '\0';
						*strrchr(folderPathName, '\\') = '\0';
						szInList = strrchr(folderPathName, '\\');

						/* 
						 * if pointer of szInList is smaller as pointer of 
						 * folderPathName, it seems to be a root folder and break
						 */
						if (szInList < folderPathName)
						{
							delete [] folderPathName;
							return TRUE;
						}

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
				if (folderChildPath != NULL)
				{
					_FileList.SelectFolder(folderChildPath);
				}
				else
				{
					_FileList.SelectFolder("..");
				}

				delete [] folderPathName;

				if (folderChildPath != NULL)
					delete [] folderChildPath;
			}
			return TRUE;
		}
		case EXM_OPENFILE:
		{
			if (strlen((char*)lParam) != 0)
			{
				char*		folderPathName	= (char*) new char[MAX_PATH];
				HTREEITEM	hItem = TreeView_GetSelection(_hTreeCtrl);

				/* get current folder path */
				GetFolderPathName(hItem, folderPathName);
				sprintf(folderPathName, "%s%s", folderPathName, (char*)lParam);

				/* open file */
				::SendMessage(_nppData._nppHandle, WM_DOOPEN, 0, (LPARAM)folderPathName);

				delete [] folderPathName;
			}
			return TRUE;
		}
		case EXM_RIGHTCLICK:
		{
			ContextMenu		cm;
			vector<string>	files			= *((vector<string>*)lParam);
			POINT			pt				= {0};
			HTREEITEM		hItem			= TreeView_GetSelection(_hTreeCtrl);
			DWORD			dwpos			= ::GetMessagePos();
			char*			folderPathName	= (char*) new char[MAX_PATH];

			GetFolderPathName(hItem, folderPathName);

			pt.x = GET_X_LPARAM(dwpos);
			pt.y = GET_Y_LPARAM(dwpos);

			cm.SetObjects(files);
			cm.ShowContextMenu(_nppData._nppHandle, _hSelf, pt, (strcmp(files[0].c_str(), folderPathName) != 0));

			delete [] folderPathName;

			return TRUE;
		}
		case WM_TIMER:
		{
			if (wParam == EXT_UPDATEDEVICE)
			{
				::KillTimer(_hSelf, EXT_UPDATEDEVICE);
				UpdateDevices();
			}
			if (wParam == EXT_UPDATE)
			{
				::KillTimer(_hSelf, EXT_UPDATE);
				tb_cmd(IDM_EX_UPDATE);
			}
			if (wParam == EXT_UPDATEPATH)
			{
				::KillTimer(_hSelf, EXT_UPDATEPATH);

				HTREEITEM	hItem = TreeView_GetSelection(_hTreeCtrl);

				char*		strPathName	= (char*)new char[MAX_PATH];

				if (hItem != NULL)
				{
					GetFolderPathName(hItem, strPathName);
					_FileList.viewPath(strPathName, TRUE);
					SetCaption(strPathName);
				}
				delete [] strPathName;
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
			if (_iDockedPos < CONT_TOP)
			{
				SetCursor(_hSplitterCursorUpDown);
				if (_pExProp->iSplitterPos < 0)
				{
					_pExProp->iSplitterPos = 0;
				}
				else if (_pExProp->iSplitterPos > (rc.bottom - 56))
				{
					_pExProp->iSplitterPos = rc.bottom - 56;
				}
			}
			else
			{
				SetCursor(_hSplitterCursorLeftRight);
				if (_pExProp->iSplitterPosHorizontal < 0)
				{
					_pExProp->iSplitterPosHorizontal = 0;
				}
				else if (_pExProp->iSplitterPosHorizontal > (rc.right - 6))
				{
					_pExProp->iSplitterPosHorizontal = rc.right - 6;
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
			bool	dirValid	= TRUE;
			char*	pszPath		= (char*)new char[MAX_PATH];

			_FileList.ToggleStackRec();

			do {
				dirValid = _FileList.GetPrevDir(pszPath);
			} while (dirValid && (SelectItem(pszPath) == FALSE));

			_FileList.ToggleStackRec();

			delete [] pszPath;
			break;
		}
		case IDM_EX_REDO:
		{
			bool	dirValid	= TRUE;
			char*	pszPath		= (char*)new char[MAX_PATH];

			_FileList.ToggleStackRec();

			do {
				dirValid = _FileList.GetNextDir(pszPath);
			} while (dirValid && (SelectItem(pszPath) == FALSE));

			_FileList.ToggleStackRec();

			delete [] pszPath;
			break;
		}
		case IDM_EX_FILE_NEW:
		{
			NewDlg		dlg;
			char*		szFileName = (char*)new char[MAX_PATH];

			szFileName[0] = '\0';

			dlg.init(_hInst, _hParent);
			if (dlg.doDialog(szFileName, "New file") == TRUE)
			{
				char*	pszNewFile = (char*)new char[MAX_PATH];


				GetFolderPathName(TreeView_GetSelection(_hTreeCtrl), pszNewFile);
				strcat(pszNewFile, szFileName);
				
				::CloseHandle(::CreateFile(pszNewFile, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));
				::SendMessage(_hParent, WM_DOOPEN, 0, (LPARAM)pszNewFile);

				delete [] pszNewFile;
			}
			delete [] szFileName;
			break;
		}
		case IDM_EX_FOLDER_NEW:
		{
			NewDlg		dlg;
			char*		szFolderName = (char*)new char[MAX_PATH];

			szFolderName[0] = '\0';

			dlg.init(_hInst, _hParent);
			if (dlg.doDialog(szFolderName, "New folder") == TRUE)
			{
				char*	pszNewFolder = (char*)new char[MAX_PATH];

				GetFolderPathName(TreeView_GetSelection(_hTreeCtrl), pszNewFolder);
				strcat(pszNewFolder, szFolderName);
				
				if (::CreateDirectory(pszNewFolder, NULL) == FALSE)
				{
					::MessageBox(_hSelf, "Folder couldn't be created.", "Error", MB_OK);
				}

				delete [] pszNewFolder;
			}
			delete [] szFolderName;
			break;
		}
		case IDM_EX_SEARCH_FIND:
		{
			char*	pszPath = (char*)new char[MAX_PATH];

			GetFolderPathName(TreeView_GetSelection(_hTreeCtrl), pszPath);
			::SendMessage(_hParent, WM_LAUNCH_FINDINFILESDLG, (WPARAM)pszPath, NULL);

			delete [] pszPath;
			break;
		}
		case IDM_EX_GO_TO_USER:
		{
			char*	pathName	= (char*)new char[MAX_PATH];

			if (SHGetSpecialFolderPath(_hSelf, pathName, CSIDL_PROFILE, FALSE) == TRUE)
			{
				strcat(pathName, "\\");
				SelectItem(pathName);
			}

			delete [] pathName;

			break;
		}
		case IDM_EX_GO_TO_FOLDER:
		{
			char*	pathName	= (char*)new char[MAX_PATH];

			::SendMessage(_hParent, WM_GET_CURRENTDIRECTORY, 0, (LPARAM)pathName);
			strcat(pathName, "\\");
			SelectItem(pathName);

			delete [] pathName;

			break;
		}
		case IDM_EX_UPDATE:
		{
			char*	pszPath = (char*)new char[MAX_PATH];

			UpdateDevices();
			UpdateFolders();

			_FileList.ToggleStackRec();
			GetFolderPathName(TreeView_GetSelection(_hTreeCtrl), pszPath);
			_FileList.viewPath(pszPath);
			_FileList.ToggleStackRec();

			delete [] pszPath;
			break;
		}
		default:
			break;
	}
}

void ExplorerDialog::InitialDialog(void)
{
	/* subclass splitter */
	_hSplitterCursorUpDown		= ::LoadCursor(_hInst, MAKEINTRESOURCE(IDC_UPDOWN));
	_hSplitterCursorLeftRight	= ::LoadCursor(_hInst, MAKEINTRESOURCE(IDC_LEFTRIGHT));
	::SetWindowLong(_hSplitterCtrl, GWL_USERDATA, reinterpret_cast<LONG>(this));
	_hDefaultSplitterProc = reinterpret_cast<WNDPROC>(::SetWindowLong(_hSplitterCtrl, GWL_WNDPROC, reinterpret_cast<LONG>(wndDefaultSplitterProc)));

	/* Load Image List */
	SHFILEINFO		sfi	= {0};
	_hImageListSmall = GetSystemImageList(TRUE);
	::SendMessage(_hTreeCtrl, TVM_SETIMAGELIST, TVSIL_NORMAL, (LPARAM)_hImageListSmall);

	/* initial file list */
	_FileList.init(_hInst, _hSelf, _hListCtrl, _hImageListSmall, _pExProp);

	/* create toolbar */
	if (GetVersion() & 0x80000000)
	{
		_ToolBar.init(_hInst, _hSelf, 16, toolBarIcons, sizeof(toolBarIcons)/sizeof(ToolBarButtonUnit), true, stdIcons, sizeof(stdIcons)/sizeof(int));
	}
	else
	{
		_ToolBar.init(_hInst, _hSelf, 16, toolBarIconsNT, sizeof(toolBarIconsNT)/sizeof(ToolBarButtonUnit), true, stdIconsNT, sizeof(stdIconsNT)/sizeof(int));
	}
	_ToolBar.display();
	_Rebar.init(_hInst, _hSelf, &_ToolBar);
	_Rebar.display();

	/* initial combo */
	_ComboFilter.init(_hFilter);
}

void ExplorerDialog::SetCaption(char* path)
{
	/* store current path */
	strcpy(_pExProp->szCurrentPath, path);

	/* update text */
	updateDockingDlg();
}

BOOL ExplorerDialog::SelectItem(char* path)
{
	BOOL				folderExist = FALSE;

	char*				TEMP		= NULL;
	char*				itemName	= NULL;
	char*				ptr			= NULL;
	BOOL				isRoot		= TRUE;
	HTREEITEM			hItem		= TreeView_GetRoot(_hTreeCtrl);
	HTREEITEM			hItemSel	= NULL;
	HTREEITEM			hLastItem	= NULL;

	TEMP		= (char*)new char[MAX_PATH];
	itemName	= (char*)new char[MAX_PATH];

	/* copy path into tokenizer and search for first element */
	strcpy(TEMP, path);
	ptr = strtok(TEMP, "\\");

	/* disabled detection of TVN_SELCHANGED notification */
	_isSelNotifyEnable = FALSE;

	do
	{
		GetItemText(hItem, itemName, MAX_PATH);

		/* trunkate item name if we are in root */
		if (isRoot == TRUE)
			itemName[2] = '\0';

		if (stricmp(ptr, itemName) == 0)
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

			/* get next token of path */
			ptr = strtok(NULL, "\\");
			isRoot = FALSE;
		}
		else
		{
			/* search for next item in list */
			hItem = TreeView_GetNextItem(_hTreeCtrl, hItem, TVGN_NEXT);
		}
	} while ((ptr != NULL) && (hItem != NULL));

	delete [] TEMP;
	delete [] itemName;

	/* select last selected item */
	TreeView_SelectItem(_hTreeCtrl, hItemSel);
	TreeView_EnsureVisible(_hTreeCtrl, hItemSel);

	/* when folder is correct selected view path */
	if (ptr == NULL)
	{
		_FileList.viewPath(path, TRUE);
		SetCaption(path);
		folderExist = TRUE;
	}

	/* enable detection of TVN_SELCHANGED notification */
	_isSelNotifyEnable = TRUE;

	return folderExist;
}


/**
 *	UpdateDevices()
 */
void ExplorerDialog::UpdateDevices(void)
{
	BOOL			bDefaultDevice  = FALSE;
	DWORD			serialNr		= 0;
	DWORD			space			= 0;
	DWORD			flags			= 0;
	DWORD			driveList		= ::GetLogicalDrives();
	BOOL			isValidDrive	= FALSE;

	HTREEITEM		pCurrentItem	= TreeView_GetNextItem(_hTreeCtrl, TVI_ROOT, TVGN_CHILD);

	char			drivePathName[]	= " :\\\0\0";	// it is longer for function 'HaveChildren()'
	char*			volumeName		= (char*)new char[MAX_PATH];
	char*			TEMP			= (char*)new char[MAX_PATH];

	for (int i = 1; i < 32; i++)
	{
		drivePathName[0] = 'A' + i;

		if (0x01 & (driveList >> i))
		{
			/* create volume name */
			isValidDrive = GetVolumeInformation(drivePathName, TEMP, MAX_PATH, &serialNr, &space, &flags, NULL, 0);

			if (isValidDrive == TRUE)
			{
				sprintf(volumeName, "%c: [%s]", 'A' + i, TEMP);
			}
			else
			{
				sprintf(volumeName, "%c:", 'A' + i);
			}

			if (pCurrentItem != NULL)
			{
				/* get current volume name in list and test if name is changed */
				GetItemText(pCurrentItem, TEMP, MAX_PATH);

				if (strcmp(volumeName, TEMP) == 0)
				{
					/* if names are equal, go to next item in tree */
					pCurrentItem = TreeView_GetNextItem(_hTreeCtrl, pCurrentItem, TVGN_NEXT);
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
					ExtractIcons(drivePathName, NULL, true, &iIconNormal, &iIconSelected, &iIconOverlayed);
					UpdateItem(pCurrentItem, volumeName, iIconNormal, iIconSelected, iIconOverlayed, haveChildren);
					DeleteChildren(pCurrentItem);
					pCurrentItem = TreeView_GetNextItem(_hTreeCtrl, pCurrentItem, TVGN_NEXT);
				}
				else
				{
					/* insert the device when new and not present before */
					HTREEITEM	pPrevItem	= TreeView_GetNextItem(_hTreeCtrl, pCurrentItem, TVGN_PREVIOUS);
					HTREEITEM	pItem		= InsertChildFolder(volumeName, TVI_ROOT, pPrevItem, isValidDrive);
					if (isValidDrive == TRUE)
					{
						DrawChildren(pItem);
					}
				}
			}
			else
			{
				HTREEITEM	pItem = InsertChildFolder(volumeName, TVI_ROOT, TVI_LAST, isValidDrive);

// Changed becaues of long startup times!
//				if (isValidDrive == TRUE)
//				{
//					DrawChildren(pItem);
//				}
			}
		}
		else
		{
			if (pCurrentItem != NULL)
			{
				/* get current volume name in list and test if name is changed */
				GetItemText(pCurrentItem, TEMP, MAX_PATH);

				if (drivePathName[0] == TEMP[0])
				{
					HTREEITEM	pPrevItem	= pCurrentItem;
					pCurrentItem = TreeView_GetNextItem(_hTreeCtrl, pCurrentItem, TVGN_NEXT);
					TreeView_DeleteItem(_hTreeCtrl, pPrevItem);
				}
			}
		}
	}

	delete [] volumeName;
	delete [] TEMP;
}

void ExplorerDialog::RemoveDrive(char* drivePathName)
{
	char*		TEMP		 = (char*)new char[MAX_PATH];
	HTREEITEM	pCurrentItem = TreeView_GetNextItem(_hTreeCtrl, TVI_ROOT, TVGN_CHILD);

	GetItemText(pCurrentItem, TEMP, MAX_PATH);
	while (pCurrentItem != NULL)
	{
		if (drivePathName[0] == TEMP[0])
		{
			TreeView_DeleteItem(_hTreeCtrl, pCurrentItem);
		}

		pCurrentItem = TreeView_GetNextItem(_hTreeCtrl, pCurrentItem, TVGN_NEXT);
		GetItemText(pCurrentItem, TEMP, MAX_PATH);
	}

	delete [] TEMP;
}

void ExplorerDialog::UpdateFolders(void)
{
	char*			pszPath			= (char*)new char[MAX_PATH];
	char*			TEMP			= (char*)new char[MAX_PATH];
	HTREEITEM		pCurrentItem	= TreeView_GetChild(_hTreeCtrl, TVI_ROOT);
	DWORD			serialNr		= 0;
	DWORD			space			= 0;
	DWORD			flags			= 0;

	while (pCurrentItem != NULL)
	{
		GetItemText(pCurrentItem, pszPath, MAX_PATH);
		pszPath[2] = '\\';
		pszPath[3] = '\0';

		if (GetVolumeInformation(pszPath, TEMP, MAX_PATH, &serialNr, &space, &flags, NULL, 0))
		{
			pszPath[2] = '\0';
			UpdateFolderRecursive(pszPath, pCurrentItem);
		}
		pCurrentItem = TreeView_GetNextItem(_hTreeCtrl, pCurrentItem, TVGN_NEXT);
	}

	delete [] pszPath;
	delete [] TEMP;
}

void ExplorerDialog::UpdateFolderRecursive(char* pszParentPath, HTREEITEM pParentItem)
{
	WIN32_FIND_DATA		Find			= {0};
	HANDLE				hFind			= NULL;
	TVITEM				item			= {0};

	char*				pszItem			= (char*) new char[MAX_PATH];
	char*				pszPath			= (char*) new char[MAX_PATH];
	char*				pszSearch		= (char*) new char[MAX_PATH];
	HTREEITEM			pCurrentItem	= TreeView_GetNextItem(_hTreeCtrl, pParentItem, TVGN_CHILD);

	strcpy(pszSearch, pszParentPath);

	/* add wildcard */
	strcat(pszSearch, "\\*");

	if ((hFind = ::FindFirstFile(pszSearch, &Find)) == INVALID_HANDLE_VALUE)
	{
		delete [] pszItem;
		delete [] pszPath;
		delete [] pszSearch;
		return;
	}

	do
	{
		if (IsValidFolder(Find) == TRUE)
		{
			if (GetItemText(pCurrentItem, pszItem, MAX_PATH) == TRUE)
			{
				/* compare current item and the current folder name */
				while ((strcmp(pszItem, Find.cFileName) != 0) && (pCurrentItem != NULL))
				{
					HTREEITEM	pPrevItem = NULL;

					/* if it's not equal delete or add new item */
					if (FindFolderAfter(Find.cFileName, pCurrentItem) == TRUE)
					{
						pPrevItem		= pCurrentItem;
						pCurrentItem	= TreeView_GetNextItem(_hTreeCtrl, pCurrentItem, TVGN_NEXT);
						TreeView_DeleteItem(_hTreeCtrl, pPrevItem);
					}
					else
					{
						pPrevItem = TreeView_GetNextItem(_hTreeCtrl, pCurrentItem, TVGN_PREVIOUS);

						/* Note: If pCurrentItem is the first item in the list pPrevItem is NULL */
						if (pPrevItem == NULL)
							pCurrentItem = InsertChildFolder(Find.cFileName, pParentItem, TVI_FIRST);
						else
							pCurrentItem = InsertChildFolder(Find.cFileName, pParentItem, pPrevItem);
					}

					if (pCurrentItem != NULL)
						GetItemText(pCurrentItem, pszItem, MAX_PATH);
				}

				/* get current path */
				sprintf(pszPath, "%s\\%s", pszParentPath, pszItem);

				/* update icons and expandable information */
				int			iIconNormal		= 0;
				int			iIconSelected	= 0;
				int			iIconOverlayed	= 0;
				BOOL		haveChildren	= HaveChildren(pszPath);

				/* correct by HaveChildren() modified pszPath */
				pszPath[strlen(pszPath) - 2] = '\0';

				/* get icons and update item */
				ExtractIcons(pszPath, NULL, true, &iIconNormal, &iIconSelected, &iIconOverlayed);
				UpdateItem(pCurrentItem, pszItem, iIconNormal, iIconSelected, iIconOverlayed, haveChildren);

				/* update recursive */
				if (TreeView_GetChild(_hTreeCtrl, pCurrentItem) != NULL)
				{
					UpdateFolderRecursive(pszPath, pCurrentItem);
				}

				/* select next item */
				pCurrentItem = TreeView_GetNextItem(_hTreeCtrl, pCurrentItem, TVGN_NEXT);
			}
			else
			{
				pCurrentItem = InsertChildFolder(Find.cFileName, pParentItem);
				pCurrentItem = TreeView_GetNextItem(_hTreeCtrl, pCurrentItem, TVGN_NEXT);
			}
		}

	} while (FindNextFile(hFind, &Find));

	/* delete possible not existed items */
	while (pCurrentItem != NULL)
	{
		HTREEITEM	pPrevItem	= pCurrentItem;
		pCurrentItem			= TreeView_GetNextItem(_hTreeCtrl, pCurrentItem, TVGN_NEXT);
		TreeView_DeleteItem(_hTreeCtrl, pPrevItem);
	}

	::FindClose(hFind);

	delete [] pszItem;
	delete [] pszPath;
	delete [] pszSearch;
}

BOOL ExplorerDialog::FindFolderAfter(char* itemName, HTREEITEM pAfterItem)
{
	BOOL		isFound			= FALSE;
	char*		pszItem			= (char*) new char[MAX_PATH];
	HTREEITEM	pCurrentItem	= TreeView_GetNextItem(_hTreeCtrl, pAfterItem, TVGN_NEXT);

	while (pCurrentItem != NULL)
	{
		GetItemText(pCurrentItem, pszItem, MAX_PATH);
		if (strcmp(itemName, pszItem) == 0)
		{
			isFound = TRUE;
			pCurrentItem = NULL;
		}
		else
		{
			pCurrentItem = TreeView_GetNextItem(_hTreeCtrl, pCurrentItem, TVGN_NEXT);
		}
	}

	return isFound;
}


void ExplorerDialog::GetFolderPathName(HTREEITEM currentItem, char* folderPathName)
{
	/* return if current folder is root folder */
	if (currentItem == TVI_ROOT)
	{
		return;
	}

	/* delete folder path name */
	folderPathName[0]	= '\0';

	/* create temp resources */
	char*	TEMP	= (char*)new char[MAX_PATH];
	char*	szName	= (char*)new char[MAX_PATH];

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

	delete [] TEMP;
	delete [] szName;
}


void ExplorerDialog::NotifyNewFile(void)
{
	if (isCreated())
	{
		char*	TEMP	= (char*)new char[MAX_PATH];

		::SendMessage(_hParent, WM_GET_CURRENTDIRECTORY, 0, (LPARAM)TEMP);
		_ToolBar.enable(IDM_EX_GO_TO_FOLDER, (strlen(TEMP) != 0));

		delete [] TEMP;
	}
}



