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
	_isUp				= TRUE;
	_iPos				= 0;
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

void FileList::init(HINSTANCE hInst, HWND hParent, HWND hParentList, HIMAGELIST hImageList, INT* piColumnPos)
{
	LVCOLUMN	ColSetup	= {0};

	/* set handles for list */
	Window::init(hInst, hParent);
	_hSelf			= hParentList;
	_piColumnPos	= piColumnPos;

	/* keep sure to support virtual list with icons */
	DWORD	dwStyle = ::GetWindowLong(_hSelf, GWL_STYLE);
	::SetWindowLong(_hSelf, GWL_STYLE, dwStyle | LVS_REPORT | LVS_OWNERDATA | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS);

	ListView_SetExtendedListViewStyle(_hSelf, LVS_EX_FULLROWSELECT);
	ListView_SetCallbackMask(_hSelf, LVIS_OVERLAYMASK);

	/* subclass list control */
	::SetWindowLong(_hSelf, GWL_USERDATA, reinterpret_cast<LONG>(this));
	_hDefaultListProc = reinterpret_cast<WNDPROC>(::SetWindowLong(_hSelf, GWL_WNDPROC, reinterpret_cast<LONG>(wndDefaultListProc)));

	/* set image list and icon */
	_iImageList = ImageList_AddIcon(hImageList, ::LoadIcon(_hInst, MAKEINTRESOURCE(IDI_PARENTFOLDER)));
	::SendMessage(_hSelf, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)hImageList);

	/* enable full row select */
	/* set caption of list */
	ColSetup.mask		= LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
	ColSetup.fmt		= LVCFMT_CENTER;
	ColSetup.pszText	= "Name";
	ColSetup.cchTextMax	= 4;
	ColSetup.cx			= *_piColumnPos;
	ListView_InsertColumn(_hSelf, 0, &ColSetup);
	ColSetup.mask		= LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
	ColSetup.fmt		= LVCFMT_LEFT;
	ColSetup.pszText	= "Ext";
	ColSetup.cchTextMax	= 3;
	ColSetup.cx			= 50;
	ListView_InsertColumn(_hSelf, 1, &ColSetup);

	/* get header control and subclass it */
	_hHeader = ListView_GetHeader(_hSelf);
	::SetWindowLong(_hHeader, GWL_USERDATA, reinterpret_cast<LONG>(this));
	_hDefaultHeaderProc = reinterpret_cast<WNDPROC>(::SetWindowLong(_hHeader, GWL_WNDPROC, reinterpret_cast<LONG>(wndDefaultHeaderProc)));

	/* set owner draw for each item */
	for(int iCol = 0; iCol < 2; iCol++)
	{
		HDITEM hdItem;

		memset(&hdItem, 0, sizeof(HDITEM));
		hdItem.mask = HDI_FORMAT;

		if(Header_GetItem(_hHeader, iCol, &hdItem))
		{
			hdItem.mask = HDI_FORMAT;
			hdItem.fmt |= HDF_OWNERDRAW;
			Header_SetItem(_hHeader, iCol, &hdItem);
		}
	}

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
			char	typedChar	= makeStrLC((char*)&wParam)[0];

			/* get positions of typed char */
			vector<UINT>	posOfTypedChar;

			/* create data array of name */
			if (_iPos == 0)
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
				int		newSelRow	= -1;

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
				_pToolTip.destroy();

				/* show text */
				if (ListView_GetSubItemRect(_hSelf, hittest.iItem, hittest.iSubItem, LVIR_LABEL, &rcLabel))
				{
					RECT		rc				= {0};
					char*		pszItemText		= (char*)new char[MAX_PATH];
					UINT		width			= 0;

					::GetClientRect(_hSelf, &rc);
					ListView_GetItemText(_hSelf, hittest.iItem, hittest.iSubItem, pszItemText, MAX_PATH);
					width = ListView_GetStringWidth(_hSelf, pszItemText);

					if ((((rcLabel.right - rcLabel.left) - (hittest.iSubItem == 0 ? 10 : 13)) < width) ||
						(((rc.right - rcLabel.left) - (hittest.iSubItem == 0 ? 10 : 5)) < width))
					{
						_pToolTip.init(_hInst, _hSelf);
						rcLabel.left += (hittest.iSubItem == 0 ? -1 : 3);
						ClientToScreen(_hSelf, &rcLabel);
						_pToolTip.Show(rcLabel, pszItemText);
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
			UINT			uFolders	= _vFolders.size();
			UINT			uListCnt	= uFolders + _vFiles.size();
			LVHITTESTINFO	hittest		= *((LVHITTESTINFO*)lParam);

			/* for all select the current item */
			for (UINT uList = 0; uList < uListCnt; uList++)
			{
				ListView_SetItemState(_hSelf, uList, (hittest.iItem == uList ? LVIS_SELECTED : 0), 0xFF);
			}
			ListView_SetSelectionMark(_hSelf, hittest.iItem);

			switch (wParam)
			{
				case WM_LBUTTONDBLCLK:
				{
					/* hide tooltip */
					if (hittest.iItem < uFolders)
						_pToolTip.destroy();

					onLMouseBtnDbl();
					break;
				}
    			case WM_LBUTTONDOWN:
				{
					/* nothing to do, item is selected */
					break;
				}
    			case WM_RBUTTONDOWN:
				{
					/* hide tooltip */
					_pToolTip.destroy();

					onRMouseBtn();
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

	memset(&lf, 0, sizeof(LOGFONT));
	lf.lfHeight = 14;
	strcpy(lf.lfFaceName, "MS Sans Serif");
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

	if (_iPos == pDrawItemStruct->itemID)
	{
		//draw the sort arrow
		HBITMAP	hOldBitmap = NULL; //::SelectObject(hBitmapDc, bitmap2);

		if (_iMouseOver == pDrawItemStruct->itemID)
		{
			if (_isUp == TRUE)
				hOldBitmap = (HBITMAP)::SelectObject(hBitmapDc, _bitmap7);
			else
				hOldBitmap = (HBITMAP)::SelectObject(hBitmapDc, _bitmap9);
		}
		else
		{
			if (_isUp == TRUE)
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

			*_piColumnPos = ListView_GetColumnWidth(_hSelf, 0);
			break;
		}
		case WM_MOUSEMOVE:
		{
			RECT			rc				= {0};
			LVHITTESTINFO	hittest			= {0};
			int				iMouseOverOld	= _iMouseOver;

			/* reset mouse over position */
			_iMouseOver = -1;

			for (UINT nSubItem = 0; nSubItem < 2; nSubItem++)
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
				char	str[MAX_PATH];

				ReadArrayToList(str, lvItem.iItem ,lvItem.iSubItem);
				lvItem.pszText		= str;
				lvItem.cchTextMax	= strlen(str);
			}

			if (lvItem.mask & LVIF_IMAGE)
			{
				int	iOverlayed;
				ReadIconToList(lvItem.iItem, &lvItem.iImage, &iOverlayed);

				if (iOverlayed != 0)
				{
					lvItem.mask			|= LVIF_STATE;
					lvItem.state		|= INDEXTOOVERLAYMASK(iOverlayed);
					lvItem.stateMask	|= LVIS_OVERLAYMASK;
				}
			}
			break;
		}
		case LVN_COLUMNCLICK:
		{
			int iPos  = ((LPNMLISTVIEW)lParam)->iSubItem;

			if (iPos != _iPos)
				_iPos = iPos;
			else
				_isUp ^= TRUE;

			UpdateList();
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
					int Offset = sizeof(DROPFILES);

					for (UINT i = 0; i < data.size(); i++)
					{
						int len = data[i].length() + 1;
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
			char*			pszBuff			= 0;

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
					int Offset = sizeof(DROPFILES);

					for (UINT i = 0; i < data.size(); i++)
					{
						int len = data[i].length() + 1;
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

void FileList::ReadIconToList(int iItem, int* piIcon, int* piOverlayed)
{
	int		maxFolders		= _vFolders.size();

	if (iItem < maxFolders)
	{
		*piIcon			= _vFolders[iItem].iIcon;
		*piOverlayed	= _vFolders[iItem].iOverlayed;
	}
	else
	{
		*piIcon			= _vFiles[iItem-maxFolders].iIcon;
		*piOverlayed	= _vFiles[iItem-maxFolders].iOverlayed;
	}
}

void FileList::ReadArrayToList(char* szItem, int iItem ,int iSubItem)
{
	int		maxFolders	= _vFolders.size();

	if (iItem < maxFolders)
	{
		/* copy into temp */
		if (iSubItem == 0)
			sprintf(szItem, "[%s]", _vFolders[iItem].strName.c_str());
		else
			strcpy(szItem, _vFolders[iItem].strExt.c_str());
	}
	else
	{
		/* copy into temp */
		if (iSubItem == 0)
			strcpy(szItem, _vFiles[iItem-maxFolders].strName.c_str());
		else
			strcpy(szItem, _vFiles[iItem-maxFolders].strExt.c_str());
	}
}

void FileList::viewPath(const char* currentPath, BOOL redraw)
{
	char*					TEMP			= (char*)new char[MAX_PATH+1];
	char*					szFilter		= (char*)new char[MAX_PATH+1];
	char*					szFilterPath	= (char*)new char[MAX_PATH+1];
	WIN32_FIND_DATA			Find			= {0};
	HANDLE					hFind			= NULL;
	int						iIconSelected	= 0;
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
				/* get folders */
				tempData.strName	= Find.cFileName;
				tempData.strNameLC	= makeStrLC(Find.cFileName);
				tempData.strExt		= "<DIR>";
				tempData.strExtLC	= "<dir>";
				ExtractIcons(currentPath, Find.cFileName, true, &tempData.iIcon, &iIconSelected, &tempData.iOverlayed);
				_vFolders.push_back(tempData);
			}
			else if (IsValidParentFolder(Find) == TRUE)
			{
				/* if 'Find' is not a folder but a parent one */
				tempData.strName	= Find.cFileName;
				tempData.strNameLC	= Find.cFileName;
				tempData.strExt		= "<DIR>";
				tempData.strExtLC	= "<dir>";
				tempData.iIcon		= _iImageList;
				tempData.iOverlayed	= 0;
				_vFolders.push_back(tempData);
			}
		} while (FindNextFile(hFind, &Find));

		/* close file search */
		::FindClose(hFind);
		TEMP[strlen(TEMP)-1] = '\0';


		/* get first filter */
		char*	ptr	= NULL;

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

						/* extract name and extension */
						char*	extBeg = strrchr(Find.cFileName, '.');

						if (extBeg != NULL)
						{
							*extBeg = '\0';
							tempData.strExt		= &extBeg[1];
							tempData.strExtLC	= makeStrLC(&extBeg[1]);
						}
						else
						{
							tempData.strExt		= "";
							tempData.strExtLC	= "";
						}
						tempData.strName		= Find.cFileName;

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

void FileList::filterFiles(char* currentFilter)
{
	char*	ptr = NULL;

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

void FileList::SelectFolder(char* selFile)
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
	QuickSortRecursiveCol(&_vFolders, 1, _vFolders.size()-1, 0, TRUE);
	QuickSortRecursiveColEx(&_vFiles, 0, _vFiles.size()-1, _iPos, _isUp);
	ListView_SetItemCountEx(_hSelf, _vFiles.size() + _vFolders.size(), LVSICF_NOSCROLL);
	::RedrawWindow(_hSelf, NULL, NULL, TRUE);
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
void FileList::QuickSortRecursiveCol(vector<tFileListData>* vList, int d, int h, int column, BOOL bAscending)
{
	int		i	= 0;
	int		j	= 0;
	string	str = "";

	/* return on empty list */
	if (d > h || d < 0)
		return;

	i = h;
	j = d;

	if (column == 0)
	{
		str = (*vList)[((int) ((d+h) / 2))].strNameLC;
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
	}
	else
	{
		str = (*vList)[((int) ((d+h) / 2))].strExtLC;
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
	}

	if (d < i) QuickSortRecursiveCol(vList,d,i, column, bAscending);
	if (j < h) QuickSortRecursiveCol(vList,j,h, column, bAscending);
}

/******************************************************************************************
 *	extended sort for Quicksort of vList, sort any column and if there are equal content 
 *	sort additional over first column(s)
 */
void FileList::QuickSortRecursiveColEx(vector<tFileListData>* vList, int d, int h, int column, BOOL bAscending)
{
	QuickSortRecursiveCol(vList, d, h, column, bAscending);

	if (column != 0)
	{
		string		str = "";

		for (int i = d; i < h ;)
		{
			int iOld = i;

			str = (column == 0)? (*vList)[i].strNameLC:(*vList)[i].strExtLC;

			for (bool b = true; b;)
			{
				if (str == ((column == 0)? (*vList)[i].strNameLC:(*vList)[i].strExtLC))
					i++;
				else
					b = false;
				if (i > h)
					b = false;
			}
			QuickSortRecursiveCol(vList, iOld, i-1, 0, TRUE);
		}
	}
}

string FileList::makeStrLC(char* sz)
{
	string	str = "";
	char*	ptr	= sz;

	while (*ptr != 0)
	{
		str += tolower(*ptr);
		ptr++;
	}

	return str;
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

void FileList::PushDir(const char* pszPath)
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

bool FileList::GetPrevDir(char* pszPath)
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

bool FileList::GetNextDir(char* pszPath)
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




