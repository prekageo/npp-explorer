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


#include "FileList.h"
#include "resource.h"
#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shellapi.h>


#ifndef LVM_SETSELECTEDCOLUMN
#define LVM_SETSELECTEDCOLUMN (LVM_FIRST + 140)
#endif

#ifndef WH_MOUSE_LL
#define WH_MOUSE_LL 14
#endif


#define LVIS_SELANDFOC	(LVIS_SELECTED|LVIS_FOCUSED)


static HWND		hWndServer		= NULL;
static HHOOK	hookMouse		= NULL;

static LRESULT CALLBACK hookProcMouse(INT nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0)
    {
		switch (wParam)
		{
			case WM_MOUSEMOVE:
			case WM_NCMOUSEMOVE:
				::PostMessage(hWndServer, wParam, 0, 0);
				break;
			case WM_LBUTTONUP:
			case WM_NCLBUTTONUP:
				::PostMessage(hWndServer, wParam, 0, 0);
				return TRUE;
			default: 
				break;
		}
	}
	return ::CallNextHookEx(hookMouse, nCode, wParam, lParam);
}


FileList::FileList(void)
{
	_hFont				= NULL;
	_hFontUnder			= NULL;
	_hHeader			= NULL;
	_bmpSortUp			= NULL;
	_bmpSortDown		= NULL;
	_iMouseTrackItem	= 0;
	_lMouseTrackPos		= 0;
	_iBltPos			= 0;
	_strCurrentPath		= "";
	_isStackRec			= TRUE;
	_iItem				= 0;
	_iSubItem			= 0;
	_bOldAddExtToName	= FALSE;
	_bOldViewLong		= FALSE;
	_bSearchFile		= FALSE;
	strcpy(_strSearchFile, "");
	strcpy(_szFileFilter, "*.*");
}

FileList::~FileList(void)
{
	if (gWinVersion < WV_XP)
	{
		::DeleteObject(_bmpSortUp);
		::DeleteObject(_bmpSortDown);
	}
	ImageList_Destroy(_hImlParent);
	::DeleteObject(_hFontUnder);

	_vDirStack.clear();
	_vFolders.clear();
	_vFiles.clear();
}

void FileList::init(HINSTANCE hInst, HWND hParent, HWND hParentList)
{
	/* this is the list element */
	Window::init(hInst, hParent);
	_hSelf = hParentList;

	/* get font for drawing */
	_hFont		= (HFONT)::SendMessage(_hParent, WM_GETFONT, 0, 0);

	/* create copy of current font with underline */
	LOGFONT	lf			= {0};
	::GetObject(_hFont, sizeof(LOGFONT), &lf);
	lf.lfUnderline		= TRUE;
	_hFontUnder	= ::CreateFontIndirect(&lf);

	/* load sort bitmaps */
	if (gWinVersion < WV_XP)
	{
		_bmpSortUp	 = (HBITMAP)::LoadImage(hInst, MAKEINTRESOURCE(IDB_SORTUP), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS);
		_bmpSortDown = (HBITMAP)::LoadImage(hInst, MAKEINTRESOURCE(IDB_SORTDOWN), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS);
	}

	/* keep sure to support virtual list with icons */
	DWORD	dwStyle = ::GetWindowLong(_hSelf, GWL_STYLE);
	::SetWindowLong(_hSelf, GWL_STYLE, dwStyle | LVS_REPORT | LVS_OWNERDATA | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS);

	/* enable full row select */
	ListView_SetExtendedListViewStyle(_hSelf, LVS_EX_FULLROWSELECT);
	ListView_SetCallbackMask(_hSelf, LVIS_OVERLAYMASK);

	/* subclass list control */
	lpFileListClass = this;
	_hDefaultListProc = reinterpret_cast<WNDPROC>(::SetWindowLong(_hSelf, GWL_WNDPROC, reinterpret_cast<LONG>(wndDefaultListProc)));

	/* set image list and icon */
	_hImlParent = GetSmallImageList(FALSE);
	::SendMessage(_hSelf, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)GetSmallImageList(_pExProp->bUseSystemIcons));

	/* get header control and subclass it */
	hWndServer = _hHeader = ListView_GetHeader(_hSelf);
	_hDefaultHeaderProc = reinterpret_cast<WNDPROC>(::SetWindowLong(_hHeader, GWL_WNDPROC, reinterpret_cast<LONG>(wndDefaultHeaderProc)));

	/* set here the columns */
	SetColumns();
}

void FileList::initProp(tExProp* prop)
{
	/* set properties */
	_pExProp	= prop;
}


/****************************************************************************
 *	Draw header list
 */
LRESULT FileList::runListProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_GETDLGCODE:
		{
			return DLGC_WANTALLKEYS | ::CallWindowProc(_hDefaultListProc, hwnd, Message, wParam, lParam);
		}
		case WM_CHAR:
		{
			UINT	selRow		= ListView_GetSelectionMark(_hSelf);

			/* restart timer */
			::KillTimer(_hSelf, EXT_SEARCHFILE);
			::SetTimer(_hSelf, EXT_SEARCHFILE, 1000, NULL);

			/* initilize again if error previous occured */
			if (selRow < 0) selRow = 0;

			/* add character to string */
			strncat(_strSearchFile, (LPTSTR)tolower((INT)&wParam), 1);

			/* on first call start searching on next element */
			if (_bSearchFile == FALSE)
				selRow++;

			UINT startPos	= selRow;
			BOOL bRet		= FindNextItemInList(_uMaxFolders, _uMaxElements, &selRow);
			if ((bRet == FALSE) && (_bSearchFile == TRUE))
			{
				_strSearchFile[strlen(_strSearchFile)-1] = '\0';
				selRow++;
				bRet = FindNextItemInList(_uMaxFolders, _uMaxElements, &selRow);
			}

			if (bRet == TRUE)
			{
				/* select only one item */
				for (UINT i = 0; i < _uMaxElements; i++)
				{
					ListView_SetItemState(_hSelf, i, (selRow == i ? LVIS_SELANDFOC : 0), 0xFF);
				}
				ListView_SetSelectionMark(_hSelf, selRow);
				ListView_EnsureVisible(_hSelf, selRow, TRUE);
			}

			/* mark that we starting with searching */
			_bSearchFile = TRUE;

			return TRUE;
		}
		case WM_MOUSEMOVE:
		{
			RECT			rcLabel			= {0};
			LVHITTESTINFO	hittest			= {0};

			/* get position */
			::GetCursorPos(&hittest.pt);
			ScreenToClient(_hSelf, &hittest.pt);
			::SendMessage(_hSelf, LVM_SUBITEMHITTEST, 0, (LPARAM)&hittest);

			if ((hittest.flags != 1) && ((_iItem != hittest.iItem) || (_iSubItem != hittest.iSubItem)))
			{
				if (_pToolTip.isVisible())
					_pToolTip.destroy();

				/* show text */
				if (ListView_GetSubItemRect(_hSelf, hittest.iItem, hittest.iSubItem, LVIR_LABEL, &rcLabel))
				{
					TCHAR		pszItemText[MAX_PATH];
					RECT		rc				= {0};
					INT			width			= 0;

					::GetClientRect(_hSelf, &rc);

					/* get width of selected column */
					if ((hittest.iSubItem == 0) || 
						((hittest.iSubItem == 1) && (_pExProp->bAddExtToName == TRUE)))
					{
						HDC		hDc			= ::GetDC(_hSelf);
						SIZE	size		= {0};

						/* get font length */
						HFONT hDefFont = (HFONT)::SelectObject(hDc, _hFont);
						if (hittest.iItem < (INT)_uMaxFolders)
						{
							if (_pExProp->bViewBraces == TRUE)
								sprintf(pszItemText, "[%s]", _vFolders[hittest.iItem].strName.c_str());
							else
								sprintf(pszItemText, "%s", _vFolders[hittest.iItem].strName.c_str());
						} else {
							strcpy(pszItemText, _vFiles[hittest.iItem-_uMaxFolders].strName.c_str());
						}
						::GetTextExtentPoint32(hDc, pszItemText, strlen(pszItemText), &size);
						width = size.cx;

						SelectObject(hDc, hDefFont);

						/* recalc label */
						if (_pExProp->bAddExtToName == TRUE)
						{
							RECT rcLabelSec	= {0};
							ListView_GetSubItemRect(_hSelf, hittest.iItem, 0, LVIR_LABEL, &rcLabel);
							ListView_GetSubItemRect(_hSelf, hittest.iItem, 1, LVIR_LABEL, &rcLabelSec);
							rcLabel.right = rcLabelSec.right;
						}
					}
					else
					{
						ListView_GetItemText(_hSelf, hittest.iItem, hittest.iSubItem, pszItemText, MAX_PATH);
						width = ListView_GetStringWidth(_hSelf, pszItemText);
					}

					/* open tooltip only when it's content is too small */
					if ((((rcLabel.right - rcLabel.left) - (hittest.iSubItem == 0 ? 5 : 12)) < width) ||
						(((rc.right - rcLabel.left) - (hittest.iSubItem == 0 ? 5 : 5)) < width))
					{
						_pToolTip.init(_hInst, _hSelf);
						if ((hittest.iSubItem == 0) || ((hittest.iSubItem == 1) && (_pExProp->bAddExtToName == TRUE)))
							rcLabel.left -= 1;
						else
							rcLabel.left += 3;
						ClientToScreen(_hSelf, &rcLabel);

						if ((_pExProp->bAddExtToName == FALSE) && (hittest.iSubItem == 0))
						{
							TCHAR	pszItemTextExt[MAX_PATH];
							ListView_GetItemText(_hSelf, hittest.iItem, 1, pszItemTextExt, MAX_PATH);
							if (pszItemTextExt[0] != '\0')
							{
								strcat(pszItemText, ".");
								strcat(pszItemText, pszItemTextExt);
							}
							_pToolTip.Show(rcLabel, pszItemText);
						}
						else
						{
							_pToolTip.Show(rcLabel, pszItemText);
						}
					}
				}
			}
			_iItem		= hittest.iItem;
			_iSubItem	= hittest.iSubItem;
			break;
		}
		case WM_SIZE:
		{
			RECT	rc			= {0};
			UINT	uListBegin	= ListView_GetTopIndex(_hSelf);

			::GetClientRect(_hHeader, &rc);
			::GetClientRect(hwnd, &_rcClip);
			_rcClip.top = rc.bottom + 2;
			break;
		}
		case WM_PAINT:
		{
			/* buffer for string to draw */
			static char	szDrawItem[MAX_PATH];

			/* when no elments in list return */
			if (_uMaxElements == 0)
				break;

			/* paint the background */
			LRESULT	lpRet = ::CallWindowProc(_hDefaultListProc, hwnd, Message, wParam, lParam);

			/* declaration */
			RECT	rc			= {0};
			RECT	rcExt		= {0};
			HDC		hDc			= ::GetWindowDC(hwnd);
			HFONT	hDefFont	= NULL;

			/* set clip rect */
			::IntersectClipRect(hDc, _rcClip.left, _rcClip.top, _rcClip.right, _rcClip.bottom);

			/* set transparent background */
			::SetBkMode(hDc, TRANSPARENT);

			/* calculate number of items to draw */
			UINT	uListBegin	= ListView_GetTopIndex(_hSelf);
			UINT	uListCnt	= uListBegin + ListView_GetCountPerPage(_hSelf) + 1;
			for (UINT i = uListBegin; (i < uListCnt) && (i < _uMaxElements); i++)
			{
				/* test if element is selected */
				bool isSel = (bool)
					((ListView_GetItemState(_hSelf, i, LVIS_SELECTED) & LVIS_SELECTED) && (::GetFocus() == _hSelf));

				/* draw parent up icon */
				if ((i == 0) && (_vFolders[i].bParent == TRUE))
				{
					ListView_GetSubItemRect(_hSelf, 0, 0, LVIR_ICON, &rc);
					ImageList_Draw(_hImlParent, ICON_PARENT, hDc, rc.left+2, rc.top+2, ILD_NORMAL | (isSel ? ILD_SELECTED : 0));
				}

				/* calculate complete rect */
				ListView_GetSubItemRect(_hSelf, i, 0, LVIR_LABEL, &rc);
				rc.left += 4;
				if (_pExProp->bAddExtToName == TRUE)
				{
					/* get extension rect offset */
					ListView_GetSubItemRect(_hSelf, i, 1, LVIR_LABEL, &rcExt);
					rc.right = rcExt.right;
				}

				/* set text color */
				if (isSel)
					::SetTextColor(hDc, RGB(255,255,255));

				if (i < _uMaxFolders)
				{
					if (_pExProp->bViewBraces == TRUE)
						sprintf(szDrawItem, "[%s]", _vFolders[i].strName.c_str());
					else
						sprintf(szDrawItem, "%s", _vFolders[i].strName.c_str());
					hDefFont = (HFONT)::SelectObject(hDc, _hFont);
				}
				else
				{
					/* copy string */
					strcpy(szDrawItem, _vFiles[i-_uMaxFolders].strName.c_str());
					string	strFilePath	= _strCurrentPath + _vFiles[i-_uMaxFolders].strNameExt;
					for (UINT iCurFiles = 0; iCurFiles < _vStrCurrentFiles.size(); iCurFiles++)
					{
						if (_vStrCurrentFiles[iCurFiles] == strFilePath)
						{
							hDefFont = (HFONT)::SelectObject(hDc, _hFontUnder);
							goto drawText;
						}
					}
					hDefFont = (HFONT)::SelectObject(hDc, _hFont);
				}

drawText:		/* draw text */					
				::DrawText(hDc, szDrawItem, strlen(szDrawItem), &rc, DT_LEFT | DT_SINGLELINE | DT_BOTTOM | DT_END_ELLIPSIS);
				::SetTextColor(hDc, RGB(0,0,0));
				::SelectObject(hDc, hDefFont);
			}

			return lpRet;
		}
		case WM_TIMER:
		{
			::KillTimer(_hSelf, EXT_SEARCHFILE);
			_bSearchFile = FALSE;
			strcpy(_strSearchFile, "");
			break;
		}
		case EXM_TOOLTIP:
		{
			LVHITTESTINFO	hittest		= *((LVHITTESTINFO*)lParam);

			switch (wParam)
			{
				case WM_LBUTTONDBLCLK:
				{
					/* select only one item */
					for (UINT uList = 0; uList < _uMaxElements; uList++)
					{
						ListView_SetItemState(_hSelf, uList, (hittest.iItem == uList ? LVIS_SELANDFOC : 0), 0xFF);
					}
					ListView_SetSelectionMark(_hSelf, hittest.iItem);

					/* hide tooltip */
					if (hittest.iItem < (INT)_uMaxFolders)
						_pToolTip.destroy();

					onLMouseBtnDbl();
					break;
				}
    			case WM_LBUTTONDOWN:
				{
					/* hide tooltip */
					if (0x80 & ::GetKeyState(VK_SHIFT))
					{
						INT lastSel = ListView_GetSelectionMark(_hSelf);

						if (lastSel < hittest.iItem) {
							for (INT uList = lastSel; uList <= hittest.iItem; uList++)
								ListView_SetItemState(_hSelf, uList, LVIS_SELANDFOC, 0xFF);
						} else {
							for (INT uList = lastSel; uList >= hittest.iItem; uList--)
								ListView_SetItemState(_hSelf, uList, LVIS_SELANDFOC, 0xFF);
						}
					}
					else if (0x80 & ::GetKeyState(VK_CONTROL)) 
					{
						bool	isSel = (ListView_GetItemState(_hSelf, hittest.iItem, LVIS_SELECTED) == LVIS_SELECTED);

						ListView_SetItemState(_hSelf, hittest.iItem, isSel ? 0 : LVIS_SELANDFOC, 0xFF);
					}
					else
					{
						/* select only one item */
						for (UINT uList = 0; uList < _uMaxElements; uList++) {
							ListView_SetItemState(_hSelf, uList, (hittest.iItem == uList ? LVIS_SELANDFOC : 0), 0xFF);
						}
					}
					ListView_SetSelectionMark(_hSelf, hittest.iItem);

					::SetFocus(_hSelf);
					break;
				}
    			case WM_RBUTTONDOWN:
				{
					/* select only one item */
					for (UINT uList = 0; uList < _uMaxElements; uList++) {
						ListView_SetItemState(_hSelf, uList, (hittest.iItem == uList ? LVIS_SELANDFOC : 0), 0xFF);
					}
					ListView_SetSelectionMark(_hSelf, hittest.iItem);

					/* hide tooltip */
					_pToolTip.destroy();

					onRMouseBtn();
					::SetFocus(_hSelf);
					break;
				}
			}
			return TRUE;
		}
		default:
			break;
	}
	
	return ::CallWindowProc(_hDefaultListProc, hwnd, Message, wParam, lParam);
}

/****************************************************************************
 *	Message handling of header
 */
LRESULT FileList::runHeaderProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_LBUTTONUP:
		{
			if (_lMouseTrackPos != 0)
			{
				POINT	pt	= {0};
				::GetCursorPos(&pt);

				/* erase divider line */
				DrawDivider(_iBltPos);
				_iBltPos		= 0;

				/* recalc position */
				INT iWidth = ListView_GetColumnWidth(_hSelf, _iMouseTrackItem) - (_lMouseTrackPos - pt.x);
				ListView_SetColumnWidth(_hSelf, _iMouseTrackItem, iWidth);
				::RedrawWindow(_hSelf, NULL, NULL, TRUE);

				_lMouseTrackPos = 0;

				/* update here the header column width */
				_pExProp->iColumnPosName = ListView_GetColumnWidth(_hSelf, 0);
				_pExProp->iColumnPosExt  = ListView_GetColumnWidth(_hSelf, 1);
				if (_pExProp->bViewLong == TRUE) {
					_pExProp->iColumnPosSize = ListView_GetColumnWidth(_hSelf, 2);
					_pExProp->iColumnPosDate = ListView_GetColumnWidth(_hSelf, 3);
				}

				if (hookMouse)
				{
					::UnhookWindowsHookEx(hookMouse);
					hookMouse = NULL;
				}
			}
			break;
		}
		case WM_MOUSEMOVE:
		{
			if (_lMouseTrackPos != 0)
			{
				POINT	pt		= {0};
				::GetCursorPos(&pt);
				::ScreenToClient(_hSelf, &pt);

				if (_iBltPos != 0)
					DrawDivider(_iBltPos);
				DrawDivider(pt.x);
				_iBltPos = pt.x;
			}
			break;
		}
		default:
			break;
	}

	return ::CallWindowProc(_hDefaultHeaderProc, hwnd, Message, wParam, lParam);
}

void FileList::DrawDivider(UINT x)
{
	UINT		posTop		= 0;
	RECT		rc			= {0};
	HDC			hDc			= ::GetWindowDC(_hSelf);
	HBITMAP		hBm			= NULL;
	HBRUSH		hBrush		= NULL;
	HANDLE		hBrushOrig	= NULL;

	/* get hight of header */
	Header_GetItemRect(_hHeader, 0, &rc);
	posTop = rc.bottom;

	/* set clip rect */
	::GetClientRect(_hSelf, &rc);
	::IntersectClipRect(hDc, rc.left, posTop + 2, rc.right, rc.bottom);

	/* Create a brush with the appropriate bitmap pattern to draw our drag rectangle */
	hBm = ::CreateBitmap(8, 8, 1, 1, DotPattern);
	hBrush = ::CreatePatternBrush(hBm);

	::SetBrushOrgEx(hDc, rc.left, rc.top, 0);
	hBrushOrig = ::SelectObject(hDc, hBrush);

	/* draw devider line */
	::PatBlt(hDc, x, rc.top, 1, rc.bottom, PATINVERT);

	/* destroy resources */
	::SelectObject(hDc, hBrushOrig);
	::DeleteObject(hBrush);
	::DeleteObject(hBm);
}


/****************************************************************************
 *	Parent notification
 */
BOOL FileList::notify(WPARAM wParam, LPARAM lParam)
{
	LPNMHDR	 nmhdr	= (LPNMHDR)lParam;

	if (nmhdr->hwndFrom == _hSelf)
	{
		switch (nmhdr->code)
		{
			case LVN_GETDISPINFO:
			{
				LV_ITEM &lvItem = reinterpret_cast<LV_DISPINFO*>((LV_DISPINFO FAR *)lParam)->item;

				if (lvItem.mask & LVIF_TEXT)
				{
					/* must be a cont array */
					static TCHAR	str[MAX_PATH];

					ReadArrayToList(str, lvItem.iItem ,lvItem.iSubItem);
					lvItem.pszText		= str;
					lvItem.cchTextMax	= strlen(str);
				}

				if (lvItem.mask & LVIF_IMAGE)
				{
					INT		iOverlayed;
					BOOL	bHidden;

					if (_vFolders[lvItem.iItem].bParent == TRUE)
					{
						/* if it's parent up -> disable image state */
						lvItem.mask &= ~LVIF_IMAGE;
					}
					else
					{
						ReadIconToList(lvItem.iItem, &lvItem.iImage, &iOverlayed, &bHidden);

						if (iOverlayed != 0)
						{
							lvItem.mask			|= LVIF_STATE;
							lvItem.state		|= INDEXTOOVERLAYMASK(iOverlayed);
							lvItem.stateMask	|= LVIS_OVERLAYMASK;
						}
						if (bHidden == TRUE)
						{
							lvItem.mask			|= LVIF_STATE;
							lvItem.state		|= LVIS_CUT;
							lvItem.stateMask	|= LVIS_CUT;
						}
					}
				}
				break;
			}
			case LVN_COLUMNCLICK:
			{
				/* store the marked items */
				for (UINT i = 0; i < _uMaxElements; i++)
				{
					if (i < _uMaxFolders) {
						_vFolders[i].state = ListView_GetItemState(_hSelf, i, LVIS_SELECTED);
					} else {
						_vFiles[i-_uMaxFolders].state	= ListView_GetItemState(_hSelf, i, LVIS_SELECTED);
					}
				}
				
				INT iPos  = ((LPNMLISTVIEW)lParam)->iSubItem;

				if (iPos != _pExProp->iSortPos)
					_pExProp->iSortPos = iPos;
				else
					_pExProp->bAscending ^= TRUE;
				SetOrder();
				UpdateList();

				/* mark old items */
				for (UINT i = 0; i < _uMaxElements; i++) {
					if (i < _uMaxFolders) {
						ListView_SetItemState(_hSelf, i, _vFolders[i].state, 0xFF);
					} else {
						ListView_SetItemState(_hSelf, i, _vFiles[i-_uMaxFolders].state, 0xFF);
					}
				}
				break;
			}
			case LVN_KEYDOWN:
			{
				switch (((LPNMLVKEYDOWN)lParam)->wVKey)
				{
					case VK_RETURN:
					{
						UINT	selRow		= ListView_GetSelectionMark(_hSelf);

						if (selRow != -1) {
							if (selRow < _uMaxFolders) {
								::SendMessage(_hParent, EXM_OPENDIR, 0, (LPARAM)_vFolders[selRow].strName.c_str());
							} else {
								::SendMessage(_hParent, EXM_OPENFILE, 0, (LPARAM)_vFiles[selRow-_uMaxFolders].strNameExt.c_str());
							}
						}
						break;
					}
					case VK_BACK:
					{
						::SendMessage(_hParent, EXM_OPENDIR, 0, (LPARAM)"..");
						break;
					}
					case VK_TAB:
					{
						::SetFocus(::GetNextWindow(_hSelf, (0x80 & ::GetKeyState(VK_SHIFT)) ? GW_HWNDPREV : GW_HWNDNEXT));
						break;
					}
					default:
						break;
				}
				break;
			}
			case NM_RCLICK:
			{
				onRMouseBtn();
				break;
			}
			case NM_DBLCLK:
			{
				onLMouseBtnDbl();
				break;
			}
			default:
				break;
		}
	}
	else if (nmhdr->hwndFrom == _hHeader)
	{
		switch (nmhdr->code)
		{
			case HDN_BEGINTRACK:
			{
				/* activate static change of column size */
				POINT	pt	= {0};
				::GetCursorPos(&pt);
				_lMouseTrackPos = pt.x;
				_iMouseTrackItem = ((LPNMHEADER)lParam)->iItem;
				SetWindowLong(_hParent, DWL_MSGRESULT, TRUE);

				/* start hooking */
				if (gWinVersion < WV_NT) {
					hookMouse = ::SetWindowsHookEx(WH_MOUSE, (HOOKPROC)hookProcMouse, _hInst, 0);
				} else {
					hookMouse = ::SetWindowsHookEx(WH_MOUSE_LL, (HOOKPROC)hookProcMouse, _hInst, 0);
				}

				if (!hookMouse)
				{
					DWORD dwError = ::GetLastError();
					TCHAR  str[128];
					::wsprintf(str, "GetLastError() returned %lu", dwError);
					::MessageBox(NULL, str, "SetWindowsHookEx(MOUSE) failed", MB_OK | MB_ICONERROR);
				}

				return TRUE;
			}
			case HDN_ITEMCHANGING:
			{
				/* avoid resize when double click */
				LPNMHEADER	pnmh = (LPNMHEADER)lParam;

				UINT uWidth = ListView_GetColumnWidth(_hSelf, pnmh->iItem);
				if ((_pExProp->bAddExtToName == TRUE) && (_lMouseTrackPos == 0) && 
					(pnmh->iItem == 0) && (pnmh->pitem->cxy != uWidth) && (pnmh->pitem->cxy != 0))
				{
					TCHAR	pszItemText[MAX_PATH];
					HDC		hDc			= ::GetDC(_hSelf);
					SIZE	size		= {0};
					UINT	uWidthMax	= 0;

					/* get font length */
					HFONT hDefFont = (HFONT)::SelectObject(hDc, _hFont);

					for (UINT i = 0; i < _uMaxElements; i++)
					{
						if (i < (INT)_uMaxFolders) {
							if (_pExProp->bViewBraces == TRUE)
								sprintf(pszItemText, "[%s]", _vFolders[i].strName.c_str());
							else
								sprintf(pszItemText, "%s", _vFolders[i].strName.c_str());
						} else {
							strcpy(pszItemText, _vFiles[i-_uMaxFolders].strName.c_str());
						}
						::GetTextExtentPoint32(hDc, pszItemText, strlen(pszItemText), &size);

						if (uWidthMax < size.cx)
							uWidthMax = size.cx;
					}
					SelectObject(hDc, hDefFont);
					_lMouseTrackPos = -1;
					ListView_SetColumnWidth(_hSelf, 0, uWidthMax + 24);
					_lMouseTrackPos = 0;
					
					SetWindowLong(_hParent, DWL_MSGRESULT, TRUE);
					return TRUE;
				}
				break;
			}
			default:
				break;
		}
	}

	return FALSE;
}

void FileList::ReadIconToList(INT iItem, LPINT piIcon, LPINT piOverlayed, LPBOOL pbHidden)
{
	INT		maxFolders		= _vFolders.size();
	INT		iIconSelected	= 0;

	if (iItem < (INT)_uMaxFolders)
	{
		ExtractIcons(_strCurrentPath.c_str(), _vFolders[iItem].strNameExt.c_str(), 
			DEVT_DIRECTORY, piIcon, &iIconSelected, piOverlayed);
		*pbHidden	= _vFolders[iItem].bHidden;
	}
	else
	{
		ExtractIcons(_strCurrentPath.c_str(), _vFiles[iItem-_uMaxFolders].strNameExt.c_str(), 
			DEVT_FILE, piIcon, &iIconSelected, piOverlayed);
		*pbHidden	= _vFiles[iItem-_uMaxFolders].bHidden;
	}
}

void FileList::ReadArrayToList(LPSTR szItem, INT iItem ,INT iSubItem)
{
	INT		maxFolders		= _vFolders.size();

	if (iItem < (INT)_uMaxFolders)
	{
		/* copy into temp */
		switch (iSubItem)
		{
			case 0:
				if (_pExProp->bAddExtToName == FALSE)
				{
					if (_pExProp->bViewBraces == TRUE)
						sprintf(szItem, "[%s]", _vFolders[iItem].strName.c_str());
					else
						sprintf(szItem, "%s", _vFolders[iItem].strName.c_str());
				}
				else
					szItem[0] = '\0';
				break;
			case 1:
				strcpy(szItem, _vFolders[iItem].strExt.c_str());
				break;
			case 2:
				strcpy(szItem, _vFolders[iItem].strSize.c_str());
				break;
			default:
				strcpy(szItem, _vFolders[iItem].strDate.c_str());
				break;
		}
	}
	else
	{
		switch (iSubItem)
		{
			case 0:
				szItem[0] = '\0';
				break;
			case 1:
				if (_pExProp->bAddExtToName == FALSE)
					strcpy(szItem, _vFiles[iItem-_uMaxFolders].strExt.c_str());
				else
					szItem[0] = '\0';
				break;
			case 2:
				strcpy(szItem, _vFiles[iItem-_uMaxFolders].strSize.c_str());
				break;
			default:
				strcpy(szItem, _vFiles[iItem-_uMaxFolders].strDate.c_str());
				break;
		}
	}
}

void FileList::viewPath(LPCSTR currentPath, BOOL redraw)
{
	TCHAR					TEMP[MAX_PATH+1];
	TCHAR					szFilter[MAX_PATH+1];
	TCHAR					szFilterPath[MAX_PATH+1];
	WIN32_FIND_DATA			Find			= {0};
	HANDLE					hFind			= NULL;
	tFileListData			tempData;

	/* add backslash if necessary */
	strncpy(TEMP, currentPath, MAX_PATH-1);
	if (TEMP[strlen(TEMP) - 1] != '\\')
		strcat(TEMP, "\\");

	/* clear data */
	_vFolders.clear();
	_vFiles.clear();

	/* find folders */
	strcat(TEMP, "*");
	hFind = ::FindFirstFile(TEMP, &Find);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		do 
		{
			if (IsValidFolder(Find) == TRUE)
			{
				/* get data in order of list elements */
				tempData.bParent	= FALSE;
				tempData.bHidden	= ((Find.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0);
				tempData.strName	= Find.cFileName;
				tempData.strNameExt	= Find.cFileName;
				tempData.strExt		= "";

				if (_pExProp->bViewLong == TRUE)
				{
					tempData.strSize	= "<DIR>";
					tempData.i64Size	= 0;
					GetDate(Find.ftLastWriteTime, tempData.strDate);
					tempData.i64Date	= 0;
				}

				_vFolders.push_back(tempData);
			}
			else if (IsValidParentFolder(Find) == TRUE)
			{
				/* if 'Find' is not a folder but a parent one */
				tempData.bParent	= TRUE;
				tempData.bHidden	= FALSE;
				tempData.strName	= Find.cFileName;
				tempData.strNameExt	= Find.cFileName;
				tempData.strExt		= "";

				if (_pExProp->bViewLong == TRUE)
				{
					tempData.strSize	= "<DIR>";
					tempData.i64Size	= 0;
					GetDate(Find.ftLastWriteTime, tempData.strDate);
					tempData.i64Date	= 0;
				}

				_vFolders.push_back(tempData);
			}
		} while (FindNextFile(hFind, &Find));

		/* close file search */
		::FindClose(hFind);
		TEMP[strlen(TEMP)-1] = '\0';

		/* get first filter */
		LPSTR	ptr	= NULL;

		strncpy(szFilter, _szFileFilter, MAX_PATH);
		ptr = strtok(szFilter, ";");

		while (ptr != NULL)
		{
			/* find files with wildcard information */
			strncpy(szFilterPath, TEMP, MAX_PATH);
			strncat(szFilterPath, ptr, MAX_PATH - strlen(szFilterPath));

			hFind = ::FindFirstFile(szFilterPath, &Find);

			if ((strlen(szFilterPath) < MAX_PATH) && (hFind != INVALID_HANDLE_VALUE))
			{
				do 
				{
					/* extract file data */
					if (IsValidFile(Find) == TRUE)
					{
						/* store for correct sorting the complete name (with extension) */
						tempData.strNameExt	= Find.cFileName;
						tempData.bParent	= FALSE;
						tempData.bHidden	= ((Find.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0);

						/* extract name and extension */
						LPSTR	extBeg = strrchr(&Find.cFileName[1], '.');

						if (extBeg != NULL)
						{
							*extBeg = '\0';
							tempData.strExt	= &extBeg[1];
						}
						else
						{
							tempData.strExt		= "";
						}

						if ((_pExProp->bAddExtToName == TRUE) && (extBeg != NULL))
						{
							*extBeg = '.';
						}
						tempData.strName	= Find.cFileName;


						if (_pExProp->bViewLong == TRUE)
						{
							tempData.i64Size = (((__int64)Find.nFileSizeHigh) << 32) + Find.nFileSizeLow;
							GetSize(tempData.i64Size, tempData.strSize);
							tempData.i64Date = (((__int64)Find.ftLastWriteTime.dwHighDateTime) << 32) + Find.ftLastWriteTime.dwLowDateTime;
							GetDate(Find.ftLastWriteTime, tempData.strDate);
						}

						_vFiles.push_back(tempData);
					}
				} while (FindNextFile(hFind, &Find));
			}

			/* get next filter */
			ptr = strtok(NULL, ";");
		}

		::FindClose(hFind);
	}

	/* set max elements in list */
	_uMaxFolders	= _vFolders.size();
	_uMaxElements	= _uMaxFolders + _vFiles.size();

	/* save current path */
	_strCurrentPath = currentPath;

	/* add current dir to stack */
	PushDir(currentPath);

	/* update list content */
	UpdateList();

	/* select first entry */
	if (redraw == TRUE)
		SetFocusItem(0);
}

void FileList::filterFiles(LPSTR currentFilter)
{
	LPSTR	ptr = NULL;

	/* delete current filter */
	_szFileFilter[0] = '\0';

	/* get new filter (remove spaces) */
	ptr = strtok(currentFilter, " ");

	while (ptr != NULL)
	{
		strcat(_szFileFilter, ptr);
		ptr = strtok(NULL, " ");
	}

	/* !!!!!!! temporary value necessary !!!!!! */
	string str = _strCurrentPath;
	viewPath(str.c_str(), TRUE);
}

void FileList::SelectFolder(LPSTR selFile)
{
	for (UINT uFolder = 0; uFolder < _uMaxFolders; uFolder++)
	{
		if (strcmp(_vFolders[uFolder].strName.c_str(), selFile) == 0)
		{
			SetFocusItem(uFolder);
			return;
		}
	}
}

void FileList::SelectCurFile(void)
{
	if (_iOpenDoc == -1)
		return;

	TCHAR	TEMP[MAX_PATH];
	strcpy(TEMP, _vStrCurrentFiles[_iOpenDoc].c_str());

	UINT	uStartName	= _vStrCurrentFiles[_iOpenDoc].rfind("\\") + 1;
	UINT	uMaxFiles	= _uMaxElements - _uMaxFolders;
	for (UINT uFile = 0; uFile < uMaxFiles; uFile++)
	{
		if (strcmp(_vFiles[uFile].strNameExt.c_str(), &TEMP[uStartName]) == 0 )
		{
			SetFocusItem(_uMaxFolders + uFile);
			return;
		}
	}
}

void FileList::UpdateList(void)
{
	QuickSortRecursiveCol(&_vFolders, (_strCurrentPath.size() != 3), _uMaxFolders-1, 0, TRUE);
	QuickSortRecursiveColEx(&_vFiles, 0, _uMaxElements-_uMaxFolders-1, _pExProp->iSortPos, _pExProp->bAscending);
	ListView_SetItemCountEx(_hSelf, _uMaxElements, LVSICF_NOSCROLL);
	::RedrawWindow(_hSelf, NULL, NULL, TRUE);
}

void FileList::SetColumns(void)
{
	LVCOLUMN	ColSetup			= {0};

	ListView_DeleteColumn(_hSelf, 3);
	ListView_DeleteColumn(_hSelf, 2);
	ListView_DeleteColumn(_hSelf, 1);
	ListView_DeleteColumn(_hSelf, 0);

	if ((_bOldAddExtToName != _pExProp->bAddExtToName) && (_pExProp->iSortPos > 0))
	{
		if (_pExProp->bAddExtToName == FALSE)
			_pExProp->iSortPos++;
		else
			_pExProp->iSortPos--;

		_bOldAddExtToName = _pExProp->bAddExtToName;
	}
	if (_bOldViewLong != _pExProp->bViewLong)
	{
		if ((_pExProp->bViewLong == FALSE) &&
			(((_pExProp->iSortPos > 0) && (_pExProp->bAddExtToName == TRUE)) ||
			 ((_pExProp->iSortPos > 1) && (_pExProp->bAddExtToName == FALSE))))
			_pExProp->iSortPos = 0;
		_bOldViewLong = _pExProp->bViewLong;
	}

	ColSetup.mask		= LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
	ColSetup.fmt		= LVCFMT_LEFT;
	ColSetup.pszText	= cColumns[0];
	ColSetup.cchTextMax = strlen(cColumns[0]);
	ColSetup.cx			= _pExProp->iColumnPosName;
	ListView_InsertColumn(_hSelf, 0, &ColSetup);
	ColSetup.pszText	= cColumns[1];
	ColSetup.cchTextMax = strlen(cColumns[1]);
	ColSetup.cx			= _pExProp->iColumnPosExt;
	ListView_InsertColumn(_hSelf, 1, &ColSetup);

	if (_pExProp->bViewLong)
	{
		ColSetup.fmt		= LVCFMT_RIGHT;
		ColSetup.pszText	= cColumns[2];
		ColSetup.cchTextMax = strlen(cColumns[2]);
		ColSetup.cx			= _pExProp->iColumnPosSize;
		ListView_InsertColumn(_hSelf, 2, &ColSetup);
		ColSetup.fmt		= LVCFMT_LEFT;
		ColSetup.pszText	= cColumns[3];
		ColSetup.cchTextMax = strlen(cColumns[3]);
		ColSetup.cx			= _pExProp->iColumnPosDate;
		ListView_InsertColumn(_hSelf, 3, &ColSetup);
	}
	SetOrder();
}

#ifndef HDF_SORTDOWN
#define HDF_SORTDOWN	0x0200
#define HDF_SORTUP		0x0400
#endif

void FileList::SetOrder(void)
{
	HDITEM	hdItem		= {0};
	UINT	uMaxHeader	= Header_GetItemCount(_hHeader);

	for (UINT i = 0; i < uMaxHeader; i++)
	{
		hdItem.mask	= HDI_FORMAT;
		Header_GetItem(_hHeader, i, &hdItem);

		if (gWinVersion < WV_XP)
		{
			hdItem.mask &= ~HDI_BITMAP;
			hdItem.fmt  &= ~(HDF_BITMAP | HDF_BITMAP_ON_RIGHT);
			if (i == _pExProp->iSortPos)
			{
				hdItem.mask |= HDI_BITMAP;
				hdItem.fmt  |= (HDF_BITMAP | HDF_BITMAP_ON_RIGHT);
				hdItem.hbm   = _pExProp->bAscending ? _bmpSortUp : _bmpSortDown;
			}
		}
		else
		{
			hdItem.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
			if (i == _pExProp->iSortPos)
			{
				hdItem.fmt |= _pExProp->bAscending ? HDF_SORTUP : HDF_SORTDOWN;
			}
		}
		Header_SetItem(_hHeader, i, &hdItem);
	}
}

void FileList::onRMouseBtn(void)
{
	vector<string>		data;

	/* create data */
	for (UINT uList = 0; uList < _uMaxElements; uList++)
	{
		if (ListView_GetItemState(_hSelf, uList, LVIS_SELECTED) == LVIS_SELECTED) 
		{
			if (uList == 0) 
			{
				if (strcmp(_vFolders[0].strName.c_str(), "..") == 0) 
				{
					ListView_SetItemState(_hSelf, uList, 0, 0xFF);
					continue;
				}
			}

			if (uList < _uMaxFolders) {
				data.push_back(_strCurrentPath + _vFolders[uList].strName + "\\");
			} else {
				data.push_back(_strCurrentPath + _vFiles[uList-_uMaxFolders].strNameExt);
			}
		}
	}

	if (data.size() != 0) {
		::SendMessage(_hParent, EXM_RIGHTCLICK, data.size(), (LPARAM)&data);
	}
}

void FileList::onLMouseBtnDbl(void)
{
	UINT	maxFolders	= _vFolders.size();
	UINT	selRow		= ListView_GetSelectionMark(_hSelf);

	if (selRow != -1)
	{
		if (selRow < _uMaxFolders) {
			::SendMessage(_hParent, EXM_OPENDIR, 0, (LPARAM)_vFolders[selRow].strName.c_str());
		} else {
			::SendMessage(_hParent, EXM_OPENFILE, 0, (LPARAM)_vFiles[selRow-_uMaxFolders].strNameExt.c_str());
		}
	}
}


BOOL FileList::FindNextItemInList(UINT maxFolder, UINT maxData, LPUINT puPos)
{
	TCHAR	pszFileName[MAX_PATH];
	BOOL	bRet		= FALSE;
	UINT	iStartPos	= *puPos;

	/* search in list */
	for (UINT i = iStartPos; i != (iStartPos-1); i++)
	{
		/* if max data is reached, set iterator to zero */
		if (i == maxData)
		{
			if (iStartPos <= 1)
				break;
			else
				i = 0;
		}

		if (i < maxFolder)
		{
			strcpy(pszFileName, _vFolders[i].strName.c_str());
		}
		else
		{
			strcpy(pszFileName, _vFiles[i-maxFolder].strName.c_str());
		}

		/* trancate the compare length */
		pszFileName[strlen(_strSearchFile)] = '\0';

		if (stricmp(pszFileName, _strSearchFile) == 0)
		{
			/* string found in any following case */
			bRet	= TRUE;
			*puPos	= i;
			break;
		}
	}

	return bRet;
}



/******************************************************************************************
 *	 fast recursive Quicksort of vList; bAscending TRUE == down 
 */
void FileList::QuickSortRecursiveCol(vector<tFileListData>* vList, INT d, INT h, INT column, BOOL bAscending)
{
	INT		i		= 0;
	INT		j		= 0;
	string	str		= "";
	__int64	i64Data	= 0;

	/* return on empty list */
	if (d > h || d < 0)
		return;

	i = h;
	j = d;

	switch (column)
	{
		case 0:
		{
			str = (*vList)[((INT) ((d+h) / 2))].strNameExt;
			do
			{
				if (bAscending == TRUE)
				{
					while (stricmp((*vList)[j].strNameExt.c_str(), str.c_str()) < 0) j++;
					while (stricmp((*vList)[i].strNameExt.c_str(), str.c_str()) > 0) i--;
				}
				else
				{
					while (stricmp((*vList)[j].strNameExt.c_str(), str.c_str()) > 0) j++;
					while (stricmp((*vList)[i].strNameExt.c_str(), str.c_str()) < 0) i--;
				}
				if ( i >= j )
				{
					if ( i != j )
					{
						tFileListData buf = (*vList)[i];
						(*vList)[i] = (*vList)[j];
						(*vList)[j] = buf;
					}
					i--;
					j++;
				}
			} while (j <= i);
			break;
		}
		case 1:
		{
			str = (*vList)[((INT) ((d+h) / 2))].strExt;
			do
			{
				if (bAscending == TRUE)
				{
					while (stricmp((*vList)[j].strExt.c_str(), str.c_str()) < 0) j++;
					while (stricmp((*vList)[i].strExt.c_str(), str.c_str()) > 0) i--;
				}
				else
				{
					while (stricmp((*vList)[j].strExt.c_str(), str.c_str()) > 0) j++;
					while (stricmp((*vList)[i].strExt.c_str(), str.c_str()) < 0) i--;
				}
				if ( i >= j )
				{
					if ( i != j )
					{
						tFileListData buf = (*vList)[i];
						(*vList)[i] = (*vList)[j];
						(*vList)[j] = buf;
					}
					i--;
					j++;
				}
			} while (j <= i);
			break;
		}
		case 2:
		case 3:
		{
			i64Data = (column==2?(*vList)[((INT) ((d+h) / 2))].i64Size:(*vList)[((INT) ((d+h) / 2))].i64Date);
			do
			{
				if (bAscending == TRUE)
				{
					while ((column==2?(*vList)[j].i64Size:(*vList)[j].i64Date) < i64Data) j++;
					while ((column==2?(*vList)[i].i64Size:(*vList)[i].i64Date) > i64Data) i--;
				}
				else
				{
					while ((column==2?(*vList)[j].i64Size:(*vList)[j].i64Date) > i64Data) j++;
					while ((column==2?(*vList)[i].i64Size:(*vList)[i].i64Date) < i64Data) i--;
				}
				if ( i >= j )
				{
					if ( i != j )
					{
						tFileListData buf = (*vList)[i];
						(*vList)[i] = (*vList)[j];
						(*vList)[j] = buf;
					}
					i--;
					j++;
				}
			} while (j <= i);
			break;
		}
		default:
			break;
	}

	if (d < i) QuickSortRecursiveCol(vList,d,i, column, bAscending);
	if (j < h) QuickSortRecursiveCol(vList,j,h, column, bAscending);
}

/******************************************************************************************
 *	extended sort for Quicksort of vList, sort any column and if there are equal content 
 *	sort additional over first column(s)
 */
void FileList::QuickSortRecursiveColEx(vector<tFileListData>* vList, INT d, INT h, INT column, BOOL bAscending)
{
	QuickSortRecursiveCol(vList, d, h, column, bAscending);

	switch (column)
	{
		case 1:
		{
			string		str = "";

			for (INT i = d; i < h ;)
			{
				INT iOld = i;

				str = (*vList)[i].strExt;

				for (bool b = true; b;)
				{
					if (str == (*vList)[i].strExt)
						i++;
					else
						b = false;
					if (i > h)
						b = false;
				}
				QuickSortRecursiveCol(vList, iOld, i-1, 0, TRUE);
			}
			break;
		}
		case 2:
		case 3:
		{
			__int64	i64Data	= 0;

			for (INT i = d; i < h ;)
			{
				INT iOld = i;

				i64Data = (column==2?(*vList)[i].i64Size:(*vList)[i].i64Date);

				for (bool b = true; b;)
				{
					if (i64Data == (column==2?(*vList)[i].i64Size:(*vList)[i].i64Date))
						i++;
					else
						b = false;
					if (i > h)
						b = false;
				}
				QuickSortRecursiveCol(vList, iOld, i-1, 0, TRUE);
			}
			break;
		}
		default:
			break;
	}
}

void FileList::GetSize(__int64 size, string & str)
{
	TCHAR	TEMP[MAX_PATH];

	switch (_pExProp->fmtSize)
	{
		case SFMT_BYTES:
		{
			sprintf(TEMP, "%03I64d", size % 1000);
			size /= 1000;
			str = TEMP;

			while (size)
			{
				sprintf(TEMP, "%03I64d.", size % 1000);
				size /= 1000;
				str = TEMP + str;
			}

			break;
		}
		case SFMT_KBYTE:
		{
			size /= 1024;
			sprintf(TEMP, "%03I64d", size % 1000);
			size /= 1000;
			str = TEMP;

			while (size)
			{
				sprintf(TEMP, "%03I64d.", size % 1000);
				size /= 1000;
				str = TEMP + str;
			}

			str = str + " kB";

			break;
		}
		case SFMT_DYNAMIC:
		{
			INT i	= 0;

			str	= "000";

			for (i = 0; (i < 3) && (size != 0); i++)
			{
				sprintf(TEMP, "%03I64d", size % 1024);
				size /= 1024;
				str = TEMP;
			}

			while (size)
			{
				sprintf(TEMP, "%03I64d.", size % 1000);
				size /= 1000;
				str = TEMP + str;
			}

			switch (i)
			{
				case 0:
				case 1: str = str + " b"; break;
				case 2: str = str + " k"; break;
				default: str = str + " M"; break;
			}
			break;
		}
		case SFMT_DYNAMIC_EX:
		{
			INT		i		= 0;
			__int64 komma	= 0;

			str = "000";

			for (i = 0; (i < 3) && (size != 0); i++)
			{
				if (i < 1)
					sprintf(TEMP, "%03I64d", size);
				else
					sprintf(TEMP, "%03I64d,%I64d", size % 1024, komma);
				komma = (size % 1024) / 100;
				size /= 1024;
				str = TEMP;
			}

			while (size)
			{
				sprintf(TEMP, "%03I64d.", size % 1000);
				size /= 1000;
				str = TEMP + str;
			}

			switch (i)
			{
				case 0:
				case 1: str = str + " b"; break;
				case 2: str = str + " k"; break;
				default: str = str + " M"; break;
			}
			break;
		}
		default:
			break;
	}

	for (INT i = 0; i < 2; i++)
	{
		if (str[i] == '0')
			str[i] = ' ';
		else
			break;
	}
}

void FileList::GetDate(FILETIME ftLastWriteTime, string & str)
{
	SYSTEMTIME		sysTime;
	TCHAR			TEMP[MAX_PATH];

	FileTimeToSystemTime(&ftLastWriteTime, &sysTime);

	if (_pExProp->fmtDate == DFMT_ENG)
		sprintf(TEMP, "%02d/%02d/%02d %02d:%02d", sysTime.wYear % 100, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute);
	else
		sprintf(TEMP, "%02d.%02d.%04d %02d:%02d", sysTime.wDay, sysTime.wMonth, sysTime.wYear, sysTime.wHour, sysTime.wMinute);

	str = TEMP;
}


/**************************************************************************************
 *	Stack functions
 */
void FileList::SetToolBarInfo(ToolBar *ToolBar, UINT idUndo, UINT idRedo)
{
	_pToolBar	= ToolBar;
	_idUndo		= idUndo;
	_idRedo		= idRedo;
	ResetDirStack();
}

void FileList::ResetDirStack(void)
{
	_vDirStack.clear();
	_itrPos	= NULL;
	UpdateToolBarElements();
}

void FileList::ToggleStackRec(void)
{
	_isStackRec ^= TRUE;
}

void FileList::PushDir(LPCSTR pszPath)
{
	if (_isStackRec == TRUE)
	{
		if (_itrPos != NULL)
		{
			_vDirStack.erase(_itrPos + 1, _vDirStack.end());

			if (strcmp(pszPath, _itrPos->c_str()) != 0)
			{
				_vDirStack.push_back(pszPath);
				_itrPos = _vDirStack.end() - 1;
			}
		}
		else
		{
			_vDirStack.push_back(pszPath);
			_itrPos = _vDirStack.end() - 1;
		}
	}

	UpdateToolBarElements();
}

bool FileList::GetPrevDir(LPSTR pszPath)
{
	if (_vDirStack.size() > 1)
	{
		if (_itrPos != _vDirStack.begin())
		{
			_itrPos--;
			strcpy(pszPath, _itrPos->c_str());
			return true;
		}
	}
	return false;
}

bool FileList::GetNextDir(LPSTR pszPath)
{
	if (_vDirStack.size() > 1)
	{
		if (_itrPos != _vDirStack.end() - 1)
		{
			_itrPos++;
			strcpy(pszPath, _itrPos->c_str());
			return true;
		}
	}
	return false;
}

INT FileList::GetPrevDirs(LPSTR *pszPathes)
{
	INT i = 0;
	vector<string>::iterator	itr	= _itrPos;

	if (_vDirStack.size() > 1)
	{
		while (1) {
			if (itr != _vDirStack.begin()) {
				itr--;
				if (pszPathes) strcpy(pszPathes[i], itr->c_str());
			} else {
				break;
			}
			i++;
		}
	}
	return i;
}

INT FileList::GetNextDirs(LPSTR	*pszPathes)
{
	INT i = 0;
	vector<string>::iterator	itr	= _itrPos;

	if (_vDirStack.size() > 1)
	{
		while (1) {
			if (itr != _vDirStack.end() - 1) {
				itr++;
				if (pszPathes) strcpy(pszPathes[i], itr->c_str());
			} else {
				break;
			}
			i++;
		}
	}
	return i;
}

void FileList::OffsetItr(INT offsetItr)
{
	_itrPos += offsetItr;
	UpdateToolBarElements();
}

void FileList::UpdateToolBarElements(void)
{
	_pToolBar->enable(_idRedo, _itrPos != _vDirStack.end() - 1);
	_pToolBar->enable(_idUndo, _itrPos != _vDirStack.begin());
}




