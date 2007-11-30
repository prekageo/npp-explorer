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
#include "NativeLang_def.h"
#include <windows.h>


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


DWORD WINAPI FileOverlayThread(LPVOID lpParam)
{
	FileList	*pFileList	= (FileList*)lpParam;
	pFileList->UpdateOverlayIcon();
	return 0;
}


FileList::FileList(void)
{
	_hOverThread		= NULL;
	_hFont				= NULL;
	_hFontUnder			= NULL;
	_hHeader			= NULL;
	_bmpSortUp			= NULL;
	_bmpSortDown		= NULL;
	_iMouseTrackItem	= 0;
	_lMouseTrackPos		= 0;
	_iBltPos			= 0;
	_isStackRec			= TRUE;
	_iItem				= 0;
	_iSubItem			= 0;
	_bOldAddExtToName	= FALSE;
	_bOldViewLong		= FALSE;
	_bSearchFile		= FALSE;
	_uMaxFolders		= 0;
	_uMaxElements		= 0;
	_uMaxElementsOld	= 0;
	strcpy(_strSearchFile, "");
	strcpy(_szFileFilter, "*.*");
	_vFolders.clear();
	_vFiles.clear();
}

FileList::~FileList(void)
{
}

void FileList::init(HINSTANCE hInst, HWND hParent, HWND hParentList)
{
	/* this is the list element */
	Window::init(hInst, hParent);
	_hSelf = hParentList;

	/* create semaphore for thead */
	_hSemaphore = ::CreateSemaphore(NULL, 1, 1, NULL);

	/* create events for thread */
	for (INT i = 0; i < FL_EVT_MAX; i++) 
		_hEvent[i] = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	/* create thread */
	DWORD	dwFlags	= 0;
	_hOverThread = ::CreateThread(NULL, 0, FileOverlayThread, this, 0, &dwFlags);

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
	_hImlListSys = GetSmallImageList(_pExProp->bUseSystemIcons);
	ListView_SetImageList(_hSelf, _hImlListSys, LVSIL_SMALL);

	/* get header control and subclass it */
	hWndServer = _hHeader = ListView_GetHeader(_hSelf);
	_hDefaultHeaderProc = reinterpret_cast<WNDPROC>(::SetWindowLong(_hHeader, GWL_WNDPROC, reinterpret_cast<LONG>(wndDefaultHeaderProc)));

	/* set here the columns */
	SetColumns();
}

void FileList::initProp(tExProp* prop)
{
	/* set properties */
	_pExProp		= prop;
	SetFilter(_pExProp->strLastFilter.c_str());
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
		case WM_DESTROY:
		{
			if (gWinVersion < WV_XP)
			{
				::DeleteObject(_bmpSortUp);
				::DeleteObject(_bmpSortDown);
			}

			ImageList_Destroy(_hImlParent);
			::DeleteObject(_hFontUnder);

			if (_hSemaphore) {
				::SetEvent(_hEvent[FL_EVT_EXIT]);
				::CloseHandle(_hOverThread);
				for (INT i = 0; i < FL_EVT_MAX; i++)
					::CloseHandle(_hEvent[i]);
				::CloseHandle(_hSemaphore);
			}

			_vDirStack.clear();
			_vFolders.clear();
			_vFiles.clear();
			break;
		}
		case WM_TIMER:
		{
			::KillTimer(_hSelf, EXT_SEARCHFILE);
			_bSearchFile = FALSE;
			strcpy(_strSearchFile, "");
			break;
		}
		case EXM_UPDATE_OVERICON:
		{
			INT				iIcon			= 0;
			INT				iSelected		= 0;
			INT				iOverlay		= 0;
			RECT			rcIcon			= {0};
			tFileListData*	pFileListData	= (tFileListData*)lParam;

			ExtractIcons(_pExProp->szCurrentPath, pFileListData->strNameExt.c_str(), 
				DEVT_FILE, &iIcon, &iSelected, &iOverlay);
			pFileListData->iOverlay = iOverlay;

			/* get icon position and redraw */
			if (iOverlay != 0) {
				ListView_GetSubItemRect(_hSelf, (UINT)wParam, 0, LVIR_ICON, &rcIcon);
				::RedrawWindow(_hSelf, &rcIcon, NULL, TRUE);
			}
			::SetEvent(_hEvent[FL_EVT_NEXT]);
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
				for (i = 0; i < _uMaxElements; i++) {
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
			case NM_CUSTOMDRAW:
			{
				static char text[MAX_PATH];

				LPNMLVCUSTOMDRAW lpCD = (LPNMLVCUSTOMDRAW)lParam;

				switch (lpCD->nmcd.dwDrawStage)
				{
					case CDDS_PREPAINT:
					{
						SetWindowLong(_hParent, DWL_MSGRESULT, (LONG)(CDRF_NOTIFYITEMDRAW));
						return TRUE;
					}
					case CDDS_ITEMPREPAINT:
					{
						SetWindowLong(_hParent, DWL_MSGRESULT, (LONG)(CDRF_NOTIFYSUBITEMDRAW));
						return TRUE;
					}
					case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
					{
						if (((lpCD->iSubItem > 1) && (_pExProp->bAddExtToName == TRUE)) ||
							((lpCD->iSubItem > 0) && (_pExProp->bAddExtToName == FALSE)))
							return FALSE;

						INT		iItem		= (INT)lpCD->nmcd.dwItemSpec;
						RECT	rc			= {0};
						RECT	rcDc		= {0};

						/* get state of element */
						UINT state = ListView_GetItemState(_hSelf, iItem, 0xFF);
						BOOL isSel = ((state & LVIS_SELECTED) && (::GetFocus() == _hSelf));

						/* get window rect */
						::GetWindowRect(_hSelf, &rcDc);

						/* create memory DC for flicker free paint */
						HDC		hMemDc		= ::CreateCompatibleDC(lpCD->nmcd.hdc);
						HBITMAP	hBmp		= ::CreateCompatibleBitmap(lpCD->nmcd.hdc, rcDc.right - rcDc.left, rcDc.bottom - rcDc.top);
						HBITMAP hOldBmp		= (HBITMAP)::SelectObject(hMemDc, hBmp);

						/* get text rect */
						ListView_GetSubItemRect(_hSelf, iItem, lpCD->iSubItem, LVIR_LABEL, &rc);

						/* draw background color of item */
						HBRUSH	hBrush = NULL;
						if (state & LVIS_SELECTED) {
							hBrush = ::CreateSolidBrush(::GetSysColor(isSel ? COLOR_HIGHLIGHT : COLOR_BTNFACE));
							::FillRect(hMemDc, &rc, hBrush);
							::DeleteObject(hBrush);
						} else {
							hBrush = ::CreateSolidBrush(::GetSysColor(COLOR_WINDOW));
							::FillRect(hMemDc, &rc, hBrush);
						}

						/* set transparent mode */
						::SetBkMode(hMemDc, TRANSPARENT);

						/* calculate correct position */
						static RECT	rcName		= {0};
						if (lpCD->iSubItem == 0) {
							ListView_GetItemText(_hSelf, iItem, 0, text, MAX_PATH);
							if (_pExProp->bAddExtToName == TRUE) {
								ListView_GetSubItemRect(_hSelf, iItem, 1, LVIR_LABEL, &rcName);
								rcName.left = rc.left;
							} else {
								rcName = rc;
							}
							rcName.left += 2;
						}

						/* get correct font */
						HFONT	hDefFont	= NULL;
						if (iItem >= _uMaxFolders) {
							string	strFilePath	= _pExProp->szCurrentPath + _vFiles[iItem-_uMaxFolders].strNameExt;
							if (IsFileOpen(strFilePath.c_str()) == TRUE) {
								hDefFont = (HFONT)::SelectObject(hMemDc, _hFontUnder);
							}
						}
						if (hDefFont == NULL) {
							hDefFont = (HFONT)::SelectObject(hMemDc, _hFont);
						}

						/* set font color */
						if (isSel == TRUE)
							::SetTextColor(hMemDc, ::GetSysColor(COLOR_WINDOW));
						else
							::SetTextColor(hMemDc, ::GetSysColor(COLOR_BTNTEXT));

						/* draw text to memory */
						::DrawText(hMemDc, text, strlen(text), &rcName, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

						/* blit text */
						::BitBlt(lpCD->nmcd.hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, hMemDc, rc.left, rc.top, SRCCOPY);

						/* blit icon sequence */
						if (lpCD->iSubItem == 0) {
							/* get icon position */
							ListView_GetSubItemRect(_hSelf, iItem, 0, LVIR_ICON, &rc);

							/* clear background */
							::FillRect(hMemDc, &rc, hBrush);

							/* first is parent up icon, second folder and file icons with overlay */
							if ((iItem == 0) && (_vFolders.size() != 0) && (_vFolders[0].bParent == TRUE)) {
								ImageList_Draw(_hImlParent, ICON_PARENT, hMemDc, rc.left, rc.top, ILD_NORMAL | (isSel ? ILD_SELECTED : 0));
							} else {
								INT		iIcon		= 0;
								INT		iOverlay	= 0;
								BOOL	isHidden	= FALSE;
								UINT	fStyle		= (isSel == TRUE ? ILD_SELECTED : ILD_NORMAL);

								/* get current list, read info and draw icon */
								HIMAGELIST	hImgLstCur = (_pExProp->bUseSystemIcons ? _hImlListSys : _hImlParent);
								ReadIconToList(iItem, &iIcon, &iOverlay, &isHidden);
								ImageList_Draw(hImgLstCur, iIcon, hMemDc, rc.left, rc.top, fStyle | INDEXTOOVERLAYMASK(iOverlay));

								/* mark as hidden -- with 50% opacity of backgroud */
								if (isHidden == TRUE)
								{
									HDC		hAlphaDc		= ::CreateCompatibleDC(lpCD->nmcd.hdc);
									HBITMAP hAlphaBmp		= ::CreateCompatibleBitmap(lpCD->nmcd.hdc, rcDc.right - rcDc.left, rcDc.bottom - rcDc.top);
									HBITMAP hOldAlphaBmp	= (HBITMAP)::SelectObject(hAlphaDc, hAlphaBmp);
									::FillRect(hAlphaDc, &rc, hBrush);
									BLENDFUNCTION	bFunc = {0, 0, 0x7F, AC_SRC_OVER};
									::AlphaBlend(hMemDc, 
										rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, 
										hAlphaDc, 
										rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, bFunc);
									::SelectObject(hAlphaDc, hOldAlphaBmp);
									::DeleteObject(hAlphaBmp);
									::DeleteDC(hAlphaDc);
								}
							}

							/* blit icon */
							::BitBlt(lpCD->nmcd.hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, hMemDc, rc.left, rc.top, SRCCOPY);
						}

						::SelectObject(hMemDc, hOldBmp);
						::SelectObject(hMemDc, hDefFont);
						::DeleteObject(hBrush);
						::DeleteObject(hBmp);
						::DeleteDC(hMemDc);

						SetWindowLong(_hParent, DWL_MSGRESULT, (LONG)(CDRF_SKIPDEFAULT));
						return TRUE;
					}
					default:
						return FALSE;
				}
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

void FileList::UpdateOverlayIcon(void)
{
	UINT	i = 0;

	while (1)
	{
		DWORD	dwCase = ::WaitForMultipleObjects(FL_EVT_MAX, _hEvent, FALSE, INFINITE);

		switch (dwCase)
		{
			case FL_EVT_EXIT:
			{
				LIST_UNLOCK();
				return;
			}
			case FL_EVT_INT:
			{
				i = _uMaxElements;
				LIST_UNLOCK();
				break;
			}
			case FL_EVT_START:
			{
				i = 0;
				if (_uMaxElements == 0)
					break;

				LIST_LOCK();

				/* step over parent icon */
				if ((_uMaxFolders != 0) && (_vFolders[0].bParent == FALSE)) {
					i = 1;
				}

				::SetEvent(_hEvent[FL_EVT_NEXT]);
				break;
			}
			case FL_EVT_NEXT:
			{
				if (::WaitForSingleObject(_hEvent[FL_EVT_INT], 1) == WAIT_TIMEOUT)
				{
					if (i < _vFolders.size()) {
						::PostMessage(_hSelf, EXM_UPDATE_OVERICON, i, (LPARAM)&_vFolders[i]);
					} else if ((i - _uMaxFolders) < _vFiles.size()) {
						::PostMessage(_hSelf, EXM_UPDATE_OVERICON, i, (LPARAM)&_vFiles[i-_uMaxFolders]);
					} else {
						LIST_UNLOCK();
					}
					i++;
				} else {
					::SetEvent(_hEvent[FL_EVT_INT]);
				}
				break;
			}
		}
	}
}

void FileList::ReadIconToList(INT iItem, LPINT piIcon, LPINT piOverlay, LPBOOL pbHidden)
{
	if (iItem < (INT)_uMaxFolders)
	{
		*piIcon		= _vFolders[iItem].iIcon;
		*piOverlay	= _vFolders[iItem].iOverlay;
		*pbHidden	= _vFolders[iItem].bHidden;
	}
	else
	{
		*piIcon		= _vFiles[iItem-_uMaxFolders].iIcon;
		*piOverlay	= _vFiles[iItem-_uMaxFolders].iOverlay;
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
				if (_pExProp->bViewBraces == TRUE)
					sprintf(szItem, "[%s]", _vFolders[iItem].strName.c_str());
				else
					sprintf(szItem, "%s", _vFolders[iItem].strName.c_str());
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
				strcpy(szItem, _vFiles[iItem-_uMaxFolders].strName.c_str());
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
	TCHAR					TEMP[MAX_PATH];
	WIN32_FIND_DATA			Find			= {0};
	HANDLE					hFind			= NULL;
	tFileListData			tempData;
	vector<tFileListData>	vFoldersTemp;
	vector<tFileListData>	vFilesTemp;

	/* end thread if it is in run mode */
	::SetEvent(_hEvent[FL_EVT_INT]);

	/* add backslash if necessary */
	strncpy(TEMP, currentPath, MAX_PATH-1);
	if (TEMP[strlen(TEMP) - 1] != '\\')
		strcat(TEMP, "\\");

	/* clear data */
	_uMaxElementsOld = _uMaxElements;
	vFoldersTemp.clear();
	vFilesTemp.clear();

	/* find every element in folder */
	strcat(TEMP, "*");
	hFind = ::FindFirstFile(TEMP, &Find);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		INT		iIconSelected			= 0;

		/* get current filters */
		TCHAR	szFilter[MAX_PATH]		= "\0";
		TCHAR	szExFilter[MAX_PATH]	= "\0";
		GetFilterLists(szFilter, szExFilter);

		do 
		{
			if (IsValidFolder(Find) == TRUE)
			{
				/* get data in order of list elements */
				tempData.bParent	= FALSE;
				ExtractIcons(currentPath, Find.cFileName, DEVT_DIRECTORY, &tempData.iIcon, &iIconSelected, NULL);
				tempData.iOverlay	= 0;
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

				vFoldersTemp.push_back(tempData);
			}
			else if ((IsValidFile(Find) == TRUE) && (DoFilter(Find.cFileName, szFilter, szExFilter) == TRUE))
			{
				/* store for correct sorting the complete name (with extension) */
				tempData.strNameExt	= Find.cFileName;
				tempData.bParent	= FALSE;
				ExtractIcons(currentPath, Find.cFileName, DEVT_FILE, &tempData.iIcon, &iIconSelected, NULL);
				tempData.iOverlay	= 0;
				tempData.bHidden	= ((Find.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0);

				/* extract name and extension */
				LPSTR	extBeg = strrchr(&Find.cFileName[1], '.');

				if (extBeg != NULL) {
					*extBeg = '\0';
					tempData.strExt	= &extBeg[1];
				} else {
					tempData.strExt		= "";
				}

				if ((_pExProp->bAddExtToName == TRUE) && (extBeg != NULL)) {
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

				vFilesTemp.push_back(tempData);
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

				vFoldersTemp.push_back(tempData);
			}
		} while (FindNextFile(hFind, &Find));

		/* close file search */
		::FindClose(hFind);
	}

	/* save current path */
	strcpy(_pExProp->szCurrentPath, currentPath);

	/* add current dir to stack */
	PushDir(currentPath);

	LIST_LOCK();
	/* set temporal list as global */
	_vFiles		= vFilesTemp;
	_vFolders	= vFoldersTemp;

	/* set max elements in list */
	_uMaxFolders	= _vFolders.size();
	_uMaxElements	= _uMaxFolders + _vFiles.size();

	/* update list content */
	UpdateList();

	/* select first entry */
	if (redraw == TRUE)
		SetFocusItem(0);

	LIST_UNLOCK();

	/* start with update of overlay icons */
	::SetEvent(_hEvent[FL_EVT_START]);
}

void FileList::filterFiles(LPCTSTR currentFilter)
{
	SetFilter(currentFilter);
	viewPath(_pExProp->szCurrentPath, TRUE);
}

void FileList::SetFilter(LPCTSTR pszNewFilter)
{
	TCHAR	szTempFilter[MAX_PATH];

	/* delete current filter */
	_szFileFilter[0] = '\0';

	/* copy new filter into buffer for tok */
	strcpy(szTempFilter, pszNewFilter);

	/* get new filter (remove spaces) */
	LPSTR	ptr = strtok(szTempFilter, " ");

	while (ptr != NULL) {
		strcat(_szFileFilter, ptr);
		ptr = strtok(NULL, " ");
	}
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
	extern TCHAR g_currentFile[MAX_PATH];

	UINT	uMaxFiles	= _uMaxElements - _uMaxFolders;
	for (UINT uFile = 0; uFile < uMaxFiles; uFile++)
	{
		if (strstr(g_currentFile, _vFiles[uFile].strNameExt.c_str()) != NULL )
		{
			SetFocusItem(_uMaxFolders + uFile);
			return;
		}
	}
}

void FileList::UpdateList(void)
{
	QuickSortRecursiveCol(&_vFolders, _pExProp->szCurrentPath[3] != '\0', _uMaxFolders-1, 0, TRUE);
	QuickSortRecursiveColEx(&_vFiles, 0, _uMaxElements-_uMaxFolders-1, _pExProp->iSortPos, _pExProp->bAscending);

	/* avoid flickering */
	if (_uMaxElementsOld != _uMaxElements) {
		ListView_SetItemCountEx(_hSelf, _uMaxElements, LVSICF_NOSCROLL);
	} else {
		::RedrawWindow(_hSelf, NULL, NULL, TRUE);
	}
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
				data.push_back(_pExProp->szCurrentPath + _vFolders[uList].strName + "\\");
			} else {
				data.push_back(_pExProp->szCurrentPath + _vFiles[uList-_uMaxFolders].strNameExt);
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

void FileList::GetFilterLists(LPSTR pszFilter, LPSTR pszExFilter)
{
	LPSTR	ptr			= _szFileFilter;
	LPSTR	exc_beg		= NULL;
	LPSTR	exc_end		= NULL;

	do
	{
		exc_beg = strstr(ptr, "[^");

		if (exc_beg == NULL)
		{
			strcat(pszFilter, ptr);
		}
		else
		{
			exc_end = strstr(exc_beg, "]");

			if (exc_end > exc_beg)
			{
				strncat(pszFilter, ptr, exc_beg-ptr);
				strncat(pszExFilter, &exc_beg[2], exc_end-exc_beg-2);
				ptr = exc_end+1;
			}
		}
	} while (exc_beg != NULL);
}

BOOL FileList::DoFilter(LPCSTR pszFileName, LPSTR pszFilter, LPSTR pszExFilter)
{
	TCHAR	TEMP[256];
	LPSTR	ptr		= NULL;

	if ((pszFileName == NULL) || (pszFilter == NULL) || (pszExFilter == NULL))
		return FALSE;

	if (strcmp(pszFilter, "*.*") == 0)
		return TRUE;
	
	if (pszExFilter[0] != '\0')
	{
		strncpy(TEMP, pszExFilter, 256);

		ptr		= strtok(TEMP, ";");
		while (ptr != NULL)
		{
			if (WildCmp(pszFileName, ptr) != NULL)
				return FALSE;
			ptr = strtok(NULL, ";");
		}
	}

	strncpy(TEMP, pszFilter, 256);

	ptr		= strtok(TEMP, ";");
	while (ptr != NULL)
	{
		if (WildCmp(pszFileName, ptr) != NULL)
			return TRUE;
		ptr = strtok(NULL, ";");
	}
	return FALSE;
}

INT FileList::WildCmp(LPCSTR string, LPCSTR wild)
{
	// Written by Jack Handy - jakkhandy@hotmail.com
	// See: http://www.codeproject.com/string/wildcmp.asp
	LPCSTR		cp = NULL;
	LPCSTR		mp = NULL;

	while ((*string) && (*wild != '*')) {
		if ((tolower(*wild) != tolower(*string)) && (*wild != '?')) {
			return 0;
		}
		wild++;
		string++;
	}

	while (*string) {
		if (*wild == '*') {
			if (!*++wild) {
				return 1;
			}
			mp = wild;
			cp = string+1;
		} else if ((tolower(*wild) == tolower(*string)) || (*wild == '?')) {
			wild++;
			string++;
		} else {
			wild = mp;
			string = cp++;
		}
	}

	while (*wild == '*') {
		wild++;
	}
	return !*wild;
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




