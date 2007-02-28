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

//#include "wbded.h"


//HRESULT CreateDropSource(WB_IDropSource **ppDropSource);
//HRESULT CreateDataObject(FORMATETC *etc, STGMEDIUM *stgmeds, UINT count, WB_IDataObject **ppDataObject);


#ifndef LVM_SETSELECTEDCOLUMN
#define LVM_SETSELECTEDCOLUMN (LVM_FIRST + 140)
#endif



FileList::FileList(void)
{
	_hHeader			= NULL;
	_strCurrentPath		= "";
	_isMouseOver		= FALSE;
	_isLeftButtonDown	= FALSE;
	_iMouseOver			= -1;
	_bitmap0			= NULL;
	_bitmap1			= NULL;
	_bitmap2			= NULL;
	_bitmap3			= NULL;
	_bitmap4			= NULL;
	_bitmap5			= NULL;
	_bitmap6			= NULL;
	_bitmap7			= NULL;
	_bitmap8			= NULL;
	_bitmap9			= NULL;
	_isStackRec			= TRUE;
	_iItem				= 0;
	_iSubItem			= 0;
	_bOldAddExtToName	= FALSE;
	_bOldViewLong		= FALSE;
	_iSelMark			= 0;
	_puSelList			= NULL;
	strcpy(_szFileFilter, "*.*");
}

FileList::~FileList(void)
{
	::DeleteObject(_bitmap0);
//	::DeleteObject(_bitmap1);
	::DeleteObject(_bitmap2);
	::DeleteObject(_bitmap3);
	::DeleteObject(_bitmap4);
	::DeleteObject(_bitmap5);
	::DeleteObject(_bitmap6);
	::DeleteObject(_bitmap7);
	::DeleteObject(_bitmap8);
	::DeleteObject(_bitmap9);
	_vDirStack.clear();
	_vFolders.clear();
	_vFiles.clear();
}

void FileList::init(HINSTANCE hInst, HWND hParent, HWND hParentList, HIMAGELIST hImageList, tExProp* prop)
{
	/* set handles for list */
	Window::init(hInst, hParent);
	_hSelf		= hParentList;
	_pExProp	= prop;

	/* keep sure to support virtual list with icons */
	DWORD	dwStyle = ::GetWindowLong(_hSelf, GWL_STYLE);
	::SetWindowLong(_hSelf, GWL_STYLE, dwStyle | LVS_REPORT | LVS_OWNERDATA | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS);

	/* enable full row select */
	ListView_SetExtendedListViewStyle(_hSelf, LVS_EX_FULLROWSELECT);
	ListView_SetCallbackMask(_hSelf, LVIS_OVERLAYMASK);

	/* subclass list control */
	::SetWindowLong(_hSelf, GWL_USERDATA, reinterpret_cast<LONG>(this));
	_hDefaultListProc = reinterpret_cast<WNDPROC>(::SetWindowLong(_hSelf, GWL_WNDPROC, reinterpret_cast<LONG>(wndDefaultListProc)));

	/* set image list and icon */
	_iImageList = ImageList_AddIcon(hImageList, ::LoadIcon(_hInst, MAKEINTRESOURCE(IDI_PARENTFOLDER)));
	::SendMessage(_hSelf, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)hImageList);

	/* get header control and subclass it */
	_hHeader = ListView_GetHeader(_hSelf);
	::SetWindowLong(_hHeader, GWL_USERDATA, reinterpret_cast<LONG>(this));
	_hDefaultHeaderProc = reinterpret_cast<WNDPROC>(::SetWindowLong(_hHeader, GWL_WNDPROC, reinterpret_cast<LONG>(wndDefaultHeaderProc)));

	/* set here the columns */
	SetColumns();

	_bitmap0 = ::LoadBitmap(_hInst, MAKEINTRESOURCE(IDB_HEADER_NORMXP));
//	_bitmap1 = ::LoadBitmap(_hInst, MAKEINTRESOURCE(IDB_HEADER_SNORMXP));
	_bitmap2 = ::LoadBitmap(_hInst, MAKEINTRESOURCE(IDB_HEADER_SHIGHXP));
	_bitmap3 = ::LoadBitmap(_hInst, MAKEINTRESOURCE(IDB_HEADER_HIGHXP));
	_bitmap4 = ::LoadBitmap(_hInst, MAKEINTRESOURCE(IDB_HEADER_ENORMXP));
	_bitmap5 = ::LoadBitmap(_hInst, MAKEINTRESOURCE(IDB_HEADER_EHIGHXP));
	_bitmap6 = ::LoadBitmap(_hInst, MAKEINTRESOURCE(IDB_HEADER_SORTUPNORMXP));
	_bitmap7 = ::LoadBitmap(_hInst, MAKEINTRESOURCE(IDB_HEADER_SORTUPHIGHXP));
	_bitmap8 = ::LoadBitmap(_hInst, MAKEINTRESOURCE(IDB_HEADER_SORTDOWNNORMXP));
	_bitmap9 = ::LoadBitmap(_hInst, MAKEINTRESOURCE(IDB_HEADER_SORTDOWNHIGHXP));
}



/****************************************************************************
 *	Draw header list
 */
LRESULT FileList::runListProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_ACTIVATE:
		{
#if 0
			UINT	uCnt  = _vFolders.size() + _vFiles.size();

			if (LOWORD(wParam) != WA_INACTIVE)
			{
				ListView_SetSelectionMark(_hSelf, _iSelMark);

				for (UINT i = 0; i < uCnt; i++)
				{
					ListView_SetItemState(_hSelf, i, _puSelList[i], 0xFF);
				}

				if (_puSelList != NULL)
					delete [] _puSelList;
			}
			else
			{
				_iSelMark = ListView_GetSelectionMark(_hSelf);
				
				_puSelList = (LPUINT)new UINT[uCnt];

				for (UINT i = 0; i < uCnt; i++)
				{
					_puSelList[i] = ListView_GetItemState(_hSelf, i, LVIS_SELECTED);
				}
			}
#endif
			break;
		}
		case WM_DRAWITEM:
		{
			if (((DRAWITEMSTRUCT *)lParam)->hwndItem == _hHeader)
			{
				drawHeaderItem((DRAWITEMSTRUCT *)lParam);
				return TRUE;
			}
			break;
		}
		case WM_GETDLGCODE:
		{
			return DLGC_WANTALLKEYS | ::CallWindowProc(_hDefaultListProc, hwnd, Message, wParam, lParam);
		}
		case WM_CHAR:
		{
			UINT	i			= 0;
			UINT	maxFolders	= _vFolders.size();
			UINT	maxData		= maxFolders + _vFiles.size();
			UINT	selRow		= ListView_GetSelectionMark(_hSelf);

			/* get low typed character */
			TCHAR	typedChar	= makeStrLC((LPSTR)&wParam)[0];

			/* get positions of typed TCHAR */
			vector<UINT>	posOfTypedChar;

			/* create data array of name */
			if (_pExProp->iSortPos == 0)
			{
				for (i = 0; i < maxData; i++)
				{
					if (i < maxFolders)
					{
						if (typedChar == _vFolders[i].strNameLC[0])
							posOfTypedChar.push_back(i);
					}
					else
					{
						if (typedChar == _vFiles[i-maxFolders].strNameLC[0])
							posOfTypedChar.push_back(i);
					}
				}
			}
			/* create data of array of ext */
			else
			{
				for (i = maxFolders; i < maxData; i++)
				{
					if (typedChar == _vFiles[i-maxFolders].strExtLC[0])
						posOfTypedChar.push_back(i);
				}
			}

			if (posOfTypedChar.size() != 0)
			{
				INT		newSelRow	= -1;

				/* search for current position */
				for (i = 0; i < posOfTypedChar.size(); i++)
				{
					if (selRow < posOfTypedChar[i])
					{
						newSelRow = posOfTypedChar[i];
						break;
					}
				}
			
				/* reset current item state */
				if (newSelRow == -1)
					newSelRow = posOfTypedChar[0];

				ListView_SetItemState(_hSelf, selRow, 0, 0xFF);
				ListView_SetItemState(_hSelf, newSelRow, LVIS_SELECTED|LVIS_FOCUSED, 0xFF);
				ListView_SetSelectionMark(_hSelf, newSelRow);
				ListView_EnsureVisible(_hSelf, newSelRow, TRUE);
			}
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
					RECT		rc				= {0};
					LPSTR		pszItemText		= (LPSTR)new TCHAR[MAX_PATH];
					INT			width			= 0;

					::GetClientRect(_hSelf, &rc);
					ListView_GetItemText(_hSelf, hittest.iItem, hittest.iSubItem, pszItemText, MAX_PATH);
					width = ListView_GetStringWidth(_hSelf, pszItemText);

					/* open tooltip only when it's content is too small */
					if ((((rcLabel.right - rcLabel.left) - (hittest.iSubItem == 0 ? 10 : 13)) < width) ||
						(((rc.right - rcLabel.left) - (hittest.iSubItem == 0 ? 10 : 5)) < width))
					{
						_pToolTip.init(_hInst, _hSelf);
						rcLabel.left += (hittest.iSubItem == 0 ? -1 : 3);
						ClientToScreen(_hSelf, &rcLabel);

						if ((_pExProp->bAddExtToName == FALSE) && (hittest.iSubItem == 0))
						{
							LPSTR pszItemTextExt = (LPSTR)new TCHAR[MAX_PATH];
							ListView_GetItemText(_hSelf, hittest.iItem, 1, pszItemTextExt, MAX_PATH);
							if (pszItemTextExt[0] != '\0')
							{
								strcat(pszItemText, ".");
								strcat(pszItemText, pszItemTextExt);
							}
							_pToolTip.Show(rcLabel, pszItemText);
							delete [] pszItemTextExt;
						}
						else
						{
							_pToolTip.Show(rcLabel, pszItemText);
						}
					}

					delete [] pszItemText;
				}
			}
			_iItem		= hittest.iItem;
			_iSubItem	= hittest.iSubItem;
			break;
		}
		case EXM_TOOLTIP:
		{
			INT				iFolders	= _vFolders.size();
			UINT			uListCnt	= iFolders + _vFiles.size();
			LVHITTESTINFO	hittest		= *((LVHITTESTINFO*)lParam);

			switch (wParam)
			{
				case WM_LBUTTONDBLCLK:
				{
					/* select only one item */
					for (UINT uList = 0; uList < uListCnt; uList++)
					{
						ListView_SetItemState(_hSelf, uList, (hittest.iItem == uList ? LVIS_SELECTED : 0), 0xFF);
					}
					ListView_SetSelectionMark(_hSelf, hittest.iItem);

					/* hide tooltip */
					if (hittest.iItem < iFolders)
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

						if (lastSel < hittest.iItem)
						{
							for (INT uList = lastSel; uList <= hittest.iItem; uList++)
							{
								ListView_SetItemState(_hSelf, uList, LVIS_SELECTED, 0xFF);
							}
						}
						else
						{
							for (INT uList = lastSel; uList >= hittest.iItem; uList--)
							{
								ListView_SetItemState(_hSelf, uList, LVIS_SELECTED, 0xFF);
							}
						}
					}
					else if (0x80 & ::GetKeyState(VK_CONTROL)) 
					{
						bool	isSel = (ListView_GetItemState(_hSelf, hittest.iItem, LVIS_SELECTED) == LVIS_SELECTED);

						ListView_SetItemState(_hSelf, hittest.iItem, isSel ? 0 : LVIS_SELECTED, 0xFF);
					}
					else
					{
						/* select only one item */
						for (UINT uList = 0; uList < uListCnt; uList++)
						{
							ListView_SetItemState(_hSelf, uList, (hittest.iItem == uList ? LVIS_SELECTED : 0), 0xFF);
						}
					}
					ListView_SetSelectionMark(_hSelf, hittest.iItem);

					::SetFocus(_hSelf);
					break;
				}
    			case WM_RBUTTONDOWN:
				{
					/* select only one item */
					for (UINT uList = 0; uList < uListCnt; uList++)
					{
						ListView_SetItemState(_hSelf, uList, (hittest.iItem == uList ? LVIS_SELECTED : 0), 0xFF);
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

#ifndef ODS_HOTLIGHT
#define ODS_HOTLIGHT 0x0040
#endif

void FileList::drawHeaderItem(DRAWITEMSTRUCT *pDrawItemStruct)
{
	/* get drawing context */
	HDC		hDc			= pDrawItemStruct->hDC;
	HDC		hMemDc		= ::CreateCompatibleDC(hDc);
	HDC		hBitmapDc	= ::CreateCompatibleDC(hDc);
	RECT	clientRect	= {0};
	UINT	nItems		= ListView_GetItemCount(_hSelf);
	RECT	rect		= {0};

	::GetClientRect(_hHeader, &rect);

	/* create memory DC for flicker free paint */
	HBITMAP	bitmap = ::CreateCompatibleBitmap(hDc, rect.right - rect.left, rect.bottom - rect.top);
	HBITMAP hOldMemBitmap = (HBITMAP)::SelectObject(hMemDc, bitmap);

	/* blit the grayed bottom border */
	HBITMAP	hOldBitmapSpan = (HBITMAP)::SelectObject(hBitmapDc, _bitmap0);
	::StretchBlt(hMemDc, rect.left, rect.top, rect.right - rect.left, 20, hBitmapDc, 0, 0, 1, 20, SRCCOPY);
	::SelectObject(hBitmapDc, hOldBitmapSpan);

	/* view if cusor points in rect */
	rect = pDrawItemStruct->rcItem;

	if (_iMouseOver == pDrawItemStruct->itemID)
	{	
		/* draw the start piece of the column header */
		HBITMAP	hOldBitmap = (HBITMAP)::SelectObject(hBitmapDc, _bitmap2);
		::BitBlt(hMemDc, rect.left, 0, 6, 20, hBitmapDc, 0, 0,SRCCOPY);
		::SelectObject(hBitmapDc, hOldBitmap);

		UINT uWidth = rect.right - rect.left - 12;
		hOldBitmap = (HBITMAP)::SelectObject(hBitmapDc, _bitmap3);
		::StretchBlt(hMemDc, rect.left + 6, 0, uWidth, 20, hBitmapDc, 0, 0, 1, 20, SRCCOPY);
		::SelectObject(hBitmapDc, hOldBitmap);
		
		/* draw the end piece of the column header */
		HBITMAP	hOldBitmapEnd = (HBITMAP)::SelectObject(hBitmapDc, _bitmap5);
		::BitBlt(hMemDc, rect.right - 6, 0, 6, 20, hBitmapDc, 0, 0, SRCCOPY);
		::SelectObject(hBitmapDc, hOldBitmapEnd);
	}
	else
	{
		HBITMAP	hOldBitmapEnd = (HBITMAP)::SelectObject(hBitmapDc, _bitmap4);
		::BitBlt(hMemDc, rect.right - 6, 0, 6, 20, hBitmapDc, 0, 0, SRCCOPY);
		::SelectObject(hBitmapDc, hOldBitmapEnd);
	}

	/* create own font */
	HFONT		hFont	= NULL;
	LOGFONT		lf		= {0};
	SIZE		size	= {0};

	ZeroMemory(&lf, sizeof(LOGFONT));
	lf.lfHeight = 14;
	strcpy(lf.lfFaceName, "MS Shell Dlg");
	hFont = ::CreateFontIndirect(&lf);

	/* inflate item rect for text */
	RECT	rectItem = pDrawItemStruct->rcItem;
	rectItem.top	+= 2;
	rectItem.left	+= 8;
	
	TCHAR		buf[256];
	LVCOLUMN	column	= {0};
	
	column.mask			= LVCF_TEXT;
	column.pszText		= buf;
	column.cchTextMax	= 256;
	ListView_GetColumn(_hSelf, pDrawItemStruct->itemID, &column);

	HFONT hDefFont = (HFONT)::SelectObject(hMemDc, hFont);
	::GetTextExtentPoint32(hMemDc, buf, strlen(buf), &size);

	::SetBkMode(hMemDc, TRANSPARENT);
	::DrawText(hMemDc, buf, strlen(buf), &rectItem, DT_LEFT | DT_VCENTER);
	::SelectObject(hMemDc, hDefFont);
	::DeleteObject(hFont);

	if (_pExProp->iSortPos == pDrawItemStruct->itemID)
	{
		//draw the sort arrow
		HBITMAP	hOldBitmap = NULL; //::SelectObject(hBitmapDc, bitmap2);

		if (_iMouseOver == pDrawItemStruct->itemID)
		{
			if (_pExProp->bAscending == TRUE)
				hOldBitmap = (HBITMAP)::SelectObject(hBitmapDc, _bitmap7);
			else
				hOldBitmap = (HBITMAP)::SelectObject(hBitmapDc, _bitmap9);
		}
		else
		{
			if (_pExProp->bAscending == TRUE)
				hOldBitmap = (HBITMAP)::SelectObject(hBitmapDc, _bitmap6);
			else
				hOldBitmap = (HBITMAP)::SelectObject(hBitmapDc, _bitmap8);
		}
		if ((rect.right - rect.left) < (size.cx + 16 + 14))
			::StretchBlt(hMemDc, rect.right - 14, 6, 8, 7, hBitmapDc, 0,0, 8, 7, SRCCOPY);
		else
			::StretchBlt(hMemDc, rect.left + size.cx + 16, 6, 8, 7, hBitmapDc, 0,0, 8, 7, SRCCOPY);
		::SelectObject(hBitmapDc, hOldBitmap);
	}

	::BitBlt(hDc, rect.left, 0, rect.right - rect.left, rect.bottom - rect.top, hMemDc, rect.left, 0, SRCCOPY);
	::SelectObject(hMemDc, hOldMemBitmap);
	::DeleteObject(bitmap);
	::DeleteDC(hBitmapDc);
	::DeleteDC(hMemDc);
}

/****************************************************************************
 *	Message handling of header
 */
LRESULT FileList::runHeaderProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_LBUTTONDOWN:
		{
			_isLeftButtonDown = TRUE;
			::RedrawWindow(hwnd, NULL, NULL, TRUE);
			break;
		}
		case WM_LBUTTONUP:
		{
			_isLeftButtonDown = FALSE;
			::RedrawWindow(hwnd, NULL, NULL, TRUE);

			/* update here the header column width */
			if (_pExProp->bAddExtToName == FALSE)
			{
				_pExProp->iColumnPosName = ListView_GetColumnWidth(_hSelf, 0);
				_pExProp->iColumnPosExt  = ListView_GetColumnWidth(_hSelf, 1);
				if (_pExProp->bViewLong == TRUE)
				{
					_pExProp->iColumnPosSize = ListView_GetColumnWidth(_hSelf, 2);
					_pExProp->iColumnPosDate = ListView_GetColumnWidth(_hSelf, 3);
				}
			}
			else
			{
				INT	width = ListView_GetColumnWidth(_hSelf, 0) - _pExProp->iColumnPosExt;
				_pExProp->iColumnPosName = width < 0 ? 0 : width;
				if (_pExProp->bViewLong == TRUE)
				{
					_pExProp->iColumnPosSize = ListView_GetColumnWidth(_hSelf, 1);
					_pExProp->iColumnPosDate = ListView_GetColumnWidth(_hSelf, 2);
				}
			}
			break;
		}
		case WM_MOUSEMOVE:
		{
			RECT			rc				= {0};
			LVHITTESTINFO	hittest			= {0};
			INT				iMouseOverOld	= _iMouseOver;

			/* reset mouse over position */
			_iMouseOver = -1;

			for (UINT nSubItem = 0; nSubItem < 4; nSubItem++)
			{

				ListView_GetSubItemRect(_hSelf, 0, nSubItem, LVIR_LABEL, &rc);
				rc.left   -= 20;
				rc.top    -= 20;
				rc.bottom  = 20;

				POINT	pt = {LOWORD(lParam), HIWORD(lParam)};
				if (::PtInRect(&rc, pt) == TRUE)
				{
					if (_isLeftButtonDown == FALSE)
					{
						_iMouseOver = nSubItem;
						break;
					}
				}
			}

			if (iMouseOverOld != _iMouseOver)
				::RedrawWindow(hwnd, NULL, NULL, TRUE);

			/* keep sure that the items for tooltip are set to default */
			_iItem		= hittest.iItem;
			_iSubItem	= hittest.iSubItem;

			/* create notification for leaving the header */
			if ((_isMouseOver == FALSE) && (_isLeftButtonDown == FALSE))
			{
				TRACKMOUSEEVENT		tme;
				tme.cbSize		= sizeof(tme);
				tme.hwndTrack	= hwnd;
				tme.dwFlags		= TME_LEAVE;
				_isMouseOver	= (BOOL)_TrackMouseEvent(&tme);
			}
			break;
		}
		case WM_MOUSELEAVE:
		{
			if (_isLeftButtonDown == FALSE)
			{
				_isMouseOver = FALSE;
				_iMouseOver  = -1;
				::RedrawWindow(hwnd, NULL, NULL, TRUE);
			}
			break;
		}
		default:
			break;
	}
	
	return ::CallWindowProc(_hDefaultHeaderProc, hwnd, Message, wParam, lParam);
}

/****************************************************************************
 *	Parent notification
 */
void FileList::notify(WPARAM wParam, LPARAM lParam)
{
	LPNMHDR	 nmhdr	= (LPNMHDR)lParam;

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
			break;
		}
		case LVN_COLUMNCLICK:
		{
			UINT	uFolders	= _vFolders.size();
			UINT	uListCnt	= uFolders + _vFiles.size();

			/* store the marked items */
			for (UINT i = 0; i < uListCnt; i++)
			{
				if (i < uFolders)
				{
					_vFolders[i].state			= ListView_GetItemState(_hSelf, i, LVIS_SELECTED);
				}
				else
				{
					_vFiles[i-uFolders].state	= ListView_GetItemState(_hSelf, i, LVIS_SELECTED);
				}
			}
			
			INT iPos  = ((LPNMLISTVIEW)lParam)->iSubItem;

			if (iPos != _pExProp->iSortPos)
				_pExProp->iSortPos = iPos;
			else
				_pExProp->bAscending ^= TRUE;

			UpdateList();

			/* mark old items */
			for (UINT i = 0; i < uListCnt; i++)
			{
				if (i < uFolders)
				{
					ListView_SetItemState(_hSelf, i, _vFolders[i].state, 0xFF);
				}
				else
				{
					ListView_SetItemState(_hSelf, i, _vFiles[i-uFolders].state, 0xFF);
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
					UINT	maxFolders	= _vFolders.size();
					UINT	selRow		= ListView_GetSelectionMark(_hSelf);

					if (selRow != -1)
					{
						if (selRow < maxFolders)
						{
							::SendMessage(_hParent, EXM_OPENDIR, 0, (LPARAM)_vFolders[selRow].strName.c_str());
						}
						else
						{
							string	str = _vFiles[selRow-maxFolders].strName;

							if (!_vFiles[selRow-maxFolders].strExt.empty())
							{
								str += "." + _vFiles[selRow-maxFolders].strExt;
							}
							::SendMessage(_hParent, EXM_OPENFILE, 0, (LPARAM)str.c_str());
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
		case LVN_BEGINDRAG:
		{
#if 0
			WB_IDataObject		*pDataObject	= NULL;
			WB_IDropSource		*pDropSource	= NULL;
			HGLOBAL				hgDrop			= NULL;
			DROPFILES*			pDrop			= NULL;
			DWORD				dwEffect		= 0;
			DWORD				dwResult		= 0;

			FORMATETC			etc				= { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
			STGMEDIUM			stgmed			= { TYMED_HGLOBAL, { 0 }, 0 };

			UINT				uFolders		= _vFolders.size();
			UINT				uListCnt		= uFolders + _vFiles.size();

			vector<string>	data;

			/* create data */
			for (UINT uList = 0; uList < uListCnt; uList++)
			{
				if (ListView_GetItemState(_hSelf, uList, LVIS_SELECTED) == LVIS_SELECTED)
				{
					if (uList < uFolders)
					{
						ListView_SetItemState(_hSelf, uList, 0, 0xFF);
					}
					else
					{
						data.push_back(_strCurrentPath + _vFiles[uList-uFolders].strName + "." + _vFiles[uList-uFolders].strExt);
					}
				}
			}

			if( data.size() > 0 )
			{
				HGLOBAL hgDrop = GlobalAlloc(GMEM_FIXED, sizeof(DROPFILES) + data.size() + 1);

				if( hgDrop != NULL )
				{
					pDrop = reinterpret_cast<DROPFILES*>(hgDrop);
					pDrop->pFiles	= sizeof(DROPFILES);
					pDrop->fNC		= FALSE;
					pDrop->pt.x		= 0;
					pDrop->pt.y		= 0;
					pDrop->fWide	= FALSE;

					INT nItem=-1;
					INT Offset = sizeof(DROPFILES);

					for (UINT i = 0; i < data.size(); i++)
					{
						INT len = data[i].length() + 1;
						memcpy(reinterpret_cast<BYTE*>(hgDrop) + Offset, data[i].c_str(), len);
						Offset += len;
					}

					reinterpret_cast<BYTE*>(hgDrop)[Offset] = 0;

					stgmed.hGlobal = hgDrop;

				}
			}

			// transfer the current selection into the IDataObject
			stgmed.hGlobal = hgDrop;

			// Create IDataObject and IDropSource COM objects
			CreateDropSource(&pDropSource);
			CreateDataObject(&etc, &stgmed, 1, &pDataObject);

			//
			//   ** ** ** The drag-drop operation starts here! ** ** **
			//
			dwResult = DoDragDrop((IDataObject*)pDataObject, (IDropSource*)pDropSource, DROPEFFECT_COPY|DROPEFFECT_MOVE, &dwEffect);

			// success!
			if(dwResult == DRAGDROP_S_DROP)
			{
				if(dwEffect & DROPEFFECT_MOVE)
				{
					// remove selection from edit control
				}
			}
			// cancelled
			else if(dwResult == DRAGDROP_S_CANCEL)
			{
			}

			pDataObject->ido.lpVtbl->Release(pDataObject);
			pDropSource->ids.lpVtbl->Release(pDropSource);
#endif

#if 0

			/* get list element count */
			IDropSource*	pDropSource		= NULL;
			IDataObject*	pDataObject		= NULL;
			HGLOBAL			hgDrop			= NULL;
			DROPFILES*		pDrop			= NULL;
			UINT			uFolders		= _vFolders.size();
			UINT			uListCnt		= uFolders + _vFiles.size();
			UINT			uBuffSize		= 0;
			LPSTR			pszBuff			= 0;

			FORMATETC		etc				= { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
			STGMEDIUM		stgmed			= { TYMED_HGLOBAL, { 0 }, 0 };



			if( data.size() > 0 )
			{
				HGLOBAL hgDrop = GlobalAlloc(GMEM_FIXED, sizeof(DROPFILES) + data.size() + 1);

				if( hgDrop != NULL )
				{
					pDrop = reinterpret_cast<DROPFILES*>(hgDrop);
					pDrop->pFiles	= sizeof(DROPFILES);
					pDrop->fNC		= FALSE;
					pDrop->pt.x		= 0;
					pDrop->pt.y		= 0;
					pDrop->fWide	= FALSE;

					INT nItem=-1;
					INT Offset = sizeof(DROPFILES);

					for (UINT i = 0; i < data.size(); i++)
					{
						INT len = data[i].length() + 1;
						memcpy(reinterpret_cast<BYTE*>(hgDrop) + Offset, data[i].c_str(), len);
						Offset += len;
					}

					reinterpret_cast<BYTE*>(hgDrop)[Offset] = 0;

					stgmed.hGlobal = hgDrop;

				}
            }
#endif
		}
		default:
			break;
	}
}

void FileList::ReadIconToList(INT iItem, LPINT piIcon, LPINT piOverlayed, LPBOOL pbHidden)
{
	INT		maxFolders		= _vFolders.size();

	if (iItem < maxFolders)
	{
		*piIcon			= _vFolders[iItem].iIcon;
		*piOverlayed	= _vFolders[iItem].iOverlayed;
		*pbHidden		= _vFolders[iItem].bHidden;
	}
	else
	{
		*piIcon			= _vFiles[iItem-maxFolders].iIcon;
		*piOverlayed	= _vFiles[iItem-maxFolders].iOverlayed;
		*pbHidden		= _vFiles[iItem-maxFolders].bHidden;
	}
}

void FileList::ReadArrayToList(LPSTR szItem, INT iItem ,INT iSubItem)
{
	INT		maxFolders	= _vFolders.size();

	if (iItem < maxFolders)
	{
		/* copy into temp */
		if (iSubItem == 0)
		{
			if (_pExProp->bViewBraces == TRUE)
				sprintf(szItem, "[%s]", _vFolders[iItem].strName.c_str());
			else
				sprintf(szItem, "%s", _vFolders[iItem].strName.c_str());
		}
		else if (iSubItem == 1)
		{
			if (_pExProp->bAddExtToName == FALSE)
				strcpy(szItem, _vFolders[iItem].strExt.c_str());
			else
				strcpy(szItem, _vFolders[iItem].strSize.c_str());
		}
		else if (iSubItem == 2)
		{
			if (_pExProp->bAddExtToName == FALSE)
				strcpy(szItem, _vFolders[iItem].strSize.c_str());
			else
				strcpy(szItem, _vFolders[iItem].strDate.c_str());
		}
		else /* iSubItem == 3 */
		{
			strcpy(szItem, _vFolders[iItem].strDate.c_str());
		}
	}
	else
	{
		if (iSubItem == 0)
		{
			strcpy(szItem, _vFiles[iItem-maxFolders].strName.c_str());
		}
		else if (iSubItem == 1)
		{
			if (_pExProp->bAddExtToName == FALSE)
				strcpy(szItem, _vFiles[iItem-maxFolders].strExt.c_str());
			else
				strcpy(szItem, _vFiles[iItem-maxFolders].strSize.c_str());
		}
		else if (iSubItem == 2)
		{
			if (_pExProp->bAddExtToName == FALSE)
				strcpy(szItem, _vFiles[iItem-maxFolders].strSize.c_str());
			else
				strcpy(szItem, _vFiles[iItem-maxFolders].strDate.c_str());
		}
		else /* iSubItem == 3 */
		{
			strcpy(szItem, _vFiles[iItem-maxFolders].strDate.c_str());
		}
	}
}

void FileList::viewPath(LPCSTR currentPath, BOOL redraw)
{
	LPSTR					TEMP			= (LPSTR)new TCHAR[MAX_PATH+1];
	LPSTR					szFilter		= (LPSTR)new TCHAR[MAX_PATH+1];
	LPSTR					szFilterPath	= (LPSTR)new TCHAR[MAX_PATH+1];
	WIN32_FIND_DATA			Find			= {0};
	HANDLE					hFind			= NULL;
	INT						iIconSelected	= 0;
	tFileListData			tempData;

	/* add backslash if necessary */
	strncpy(TEMP, currentPath, MAX_PATH-1);
	if (TEMP[strlen(TEMP) - 1] != '\\')
	{
		strcat(TEMP, "\\");
	}

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
				ExtractIcons(currentPath, Find.cFileName, true, &tempData.iIcon, &iIconSelected, &tempData.iOverlayed);
				tempData.bHidden = ((Find.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0);

				tempData.strName	= Find.cFileName;
				tempData.strNameLC	= makeStrLC(Find.cFileName);
				tempData.strExt		= "";
				tempData.strExtLC	= "";

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
				tempData.iIcon		= _iImageList;
				tempData.iOverlayed	= 0;
				tempData.bHidden	= FALSE;

				tempData.strName	= Find.cFileName;
				tempData.strNameLC	= Find.cFileName;
				tempData.strExt		= "";
				tempData.strExtLC	= "";

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
						/* store LC with exension for correct sort  */
						tempData.strNameLC		= makeStrLC(Find.cFileName);

						/* get icons */
						ExtractIcons(currentPath, Find.cFileName, false, &tempData.iIcon, &iIconSelected, &tempData.iOverlayed);
						tempData.bHidden = ((Find.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0);

						/* extract name and extension */
						LPSTR	extBeg = strrchr(&Find.cFileName[1], '.');

						if (extBeg != NULL)
						{
							*extBeg = '\0';
							if (_pExProp->bAddExtToName == FALSE)
							{
								tempData.strExt	= &extBeg[1];
							}
							tempData.strExtLC	= makeStrLC(&extBeg[1]);
						}
						else
						{
							tempData.strExt		= "";
							tempData.strExtLC	= "";
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

	/* release resource */
	delete [] TEMP;
	delete [] szFilter;
	delete [] szFilterPath;

	/* update list content */
	UpdateList();

	/* select first entry */
	if (redraw == TRUE)
	{
		SetFocusItem(0);
	}

	/* save current path */
	_strCurrentPath = currentPath;

	/* add current dir to stack */
	PushDir(currentPath);
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
	for (UINT uFolder = 0; uFolder < _vFolders.size(); uFolder++)
	{
		if (strcmp(_vFolders[uFolder].strName.c_str(), selFile) == 0)
		{
			SetFocusItem(uFolder);
			return;
		}
	}
}

void FileList::UpdateList(void)
{
	INT	iSortPos = _pExProp->iSortPos;

	if ((_pExProp->bAddExtToName == TRUE) && (iSortPos >= 1))
	{
		iSortPos++;
	}

	QuickSortRecursiveCol(&_vFolders, 1, _vFolders.size()-1, 0, TRUE);
	QuickSortRecursiveColEx(&_vFiles, 0, _vFiles.size()-1, iSortPos, _pExProp->bAscending);
	ListView_SetItemCountEx(_hSelf, _vFiles.size() + _vFolders.size(), LVSICF_NOSCROLL);
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

	if (_pExProp->bAddExtToName == FALSE)
	{
		ColSetup.mask		= LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
		ColSetup.fmt		= LVCFMT_LEFT;
		ColSetup.pszText	= "Name";
		ColSetup.cchTextMax	= 4;
		ColSetup.cx			= _pExProp->iColumnPosName;
		ListView_InsertColumn(_hSelf, 0, &ColSetup);
		ColSetup.fmt		= LVCFMT_LEFT;
		ColSetup.pszText	= "Ext.";
		ColSetup.cx			= _pExProp->iColumnPosExt;
		ListView_InsertColumn(_hSelf, 1, &ColSetup);

		if (_pExProp->bViewLong)
		{
			ColSetup.fmt		= LVCFMT_RIGHT;
			ColSetup.pszText	= "Size";
			ColSetup.cx			= _pExProp->iColumnPosSize;
			ListView_InsertColumn(_hSelf, 2, &ColSetup);
			ColSetup.fmt		= LVCFMT_LEFT;
			ColSetup.pszText	= "Date";
			ColSetup.cx			= _pExProp->iColumnPosDate;
			ListView_InsertColumn(_hSelf, 3, &ColSetup);
		}
	}
	else
	{
		ColSetup.mask		= LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
		ColSetup.fmt		= LVCFMT_LEFT;
		ColSetup.pszText	= "Name";
		ColSetup.cchTextMax	= 4;
		ColSetup.cx			= _pExProp->iColumnPosName + _pExProp->iColumnPosExt;
		ListView_InsertColumn(_hSelf, 0, &ColSetup);
		if (_pExProp->bViewLong)
		{
			ColSetup.mask		= LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
			ColSetup.fmt		= LVCFMT_RIGHT;
			ColSetup.pszText	= "Size";
			ColSetup.cchTextMax	= 4;
			ColSetup.cx			= _pExProp->iColumnPosSize;
			ListView_InsertColumn(_hSelf, 1, &ColSetup);
			ColSetup.fmt		= LVCFMT_LEFT;
			ColSetup.pszText	= "Date";
			ColSetup.cx			= _pExProp->iColumnPosDate;
			ListView_InsertColumn(_hSelf, 2, &ColSetup);
		}
	}

	/* set owner draw for each item */
	for(INT iCol = 0; iCol < 4; iCol++)
	{
		HDITEM hdItem;

		ZeroMemory(&hdItem, sizeof(HDITEM));
		hdItem.mask = HDI_FORMAT;

		if(Header_GetItem(_hHeader, iCol, &hdItem))
		{
			hdItem.mask = HDI_FORMAT;
			hdItem.fmt |= HDF_OWNERDRAW;
			Header_SetItem(_hHeader, iCol, &hdItem);
		}
	}
}

void FileList::onRMouseBtn(void)
{
	/* get list element count */
	UINT	uFolders = _vFolders.size();
	UINT	uListCnt = uFolders + _vFiles.size();

	vector<string>		data;

	/* create data */
	for (UINT uList = 0; uList < uListCnt; uList++)
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

			if (uList < uFolders)
			{
				data.push_back(_strCurrentPath + _vFolders[uList].strName + "\\");
			}
			else
			{
				data.push_back(_strCurrentPath + _vFiles[uList-uFolders].strName + "." + _vFiles[uList-uFolders].strExt);
			}
		}
	}

	if (data.size() != 0)
	{
		::SendMessage(_hParent, EXM_RIGHTCLICK, data.size(), (LPARAM)&data);
	}
}

void FileList::onLMouseBtnDbl(void)
{
	UINT	maxFolders	= _vFolders.size();
	UINT	selRow		= ListView_GetSelectionMark(_hSelf);

	if (selRow != -1)
	{
		if (selRow < maxFolders)
		{
			::SendMessage(_hParent, EXM_OPENDIR, 0, (LPARAM)_vFolders[selRow].strName.c_str());
		}
		else
		{
			string	str = _vFiles[selRow-maxFolders].strName;

			if (!_vFiles[selRow-maxFolders].strExt.empty())
			{
				str += "." + _vFiles[selRow-maxFolders].strExt;
			}
			::SendMessage(_hParent, EXM_OPENFILE, 0, (LPARAM)str.c_str());
		}
	}
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
			str = (*vList)[((INT) ((d+h) / 2))].strNameLC;
			do
			{
				if (bAscending == TRUE)
				{
					while ((*vList)[j].strNameLC < str) j++;
					while ((*vList)[i].strNameLC > str) i--;
				}
				else
				{
					while ((*vList)[j].strNameLC > str) j++;
					while ((*vList)[i].strNameLC < str) i--;
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
			str = (*vList)[((INT) ((d+h) / 2))].strExtLC;
			do
			{
				if (bAscending == TRUE)
				{
					while ((*vList)[j].strExtLC < str) j++;
					while ((*vList)[i].strExtLC > str) i--;
				}
				else
				{
					while ((*vList)[j].strExtLC > str) j++;
					while ((*vList)[i].strExtLC < str) i--;
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

				str = (*vList)[i].strExtLC;

				for (bool b = true; b;)
				{
					if (str == (*vList)[i].strExtLC)
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

string FileList::makeStrLC(LPSTR sz)
{
	string	str = "";
	LPSTR	ptr	= sz;

	while (*ptr != 0)
	{
		str += tolower(*ptr);
		ptr++;
	}

	return str;
}


void FileList::GetSize(__int64 size, string & str)
{
	LPTSTR	TEMP	= (LPTSTR)new TCHAR[MAX_PATH];

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

	delete [] TEMP;
}

void FileList::GetDate(FILETIME ftLastWriteTime, string & str)
{
	SYSTEMTIME		sysTime;
	LPSTR			TEMP	= (LPSTR)new TCHAR[MAX_PATH];

	FileTimeToSystemTime(&ftLastWriteTime, &sysTime);

	if (_pExProp->fmtDate == DFMT_ENG)
		sprintf(TEMP, "%02d/%02d/%02d %02d:%02d", sysTime.wYear % 100, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute);
	else
		sprintf(TEMP, "%02d.%02d.%04d %02d:%02d", sysTime.wDay, sysTime.wMonth, sysTime.wYear, sysTime.wHour, sysTime.wMinute);

	str = TEMP;

	delete [] TEMP;
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

void FileList::UpdateToolBarElements(void)
{
	_pToolBar->enable(_idRedo, _itrPos != _vDirStack.end() - 1);
	_pToolBar->enable(_idUndo, _itrPos != _vDirStack.begin());
}




