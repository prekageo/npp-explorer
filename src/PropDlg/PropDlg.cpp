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


#include "PropDlg.h"
#include "stdio.h"
#include <Shlobj.h>


// Set a call back with the handle after init to set the path.
// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/shellcc/platform/shell/reference/callbackfunctions/browsecallbackproc.asp
static int __stdcall BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM, LPARAM pData)
{
	if (uMsg == BFFM_INITIALIZED)
		::SendMessage(hwnd, BFFM_SETSELECTION, TRUE, pData);
	return 0;
};


PropDlg::PropDlg() : StaticDialog()
{
	_pName			= NULL;
	_pLink			= NULL;
	_pDesc			= NULL;
	_linkDlg		= LINK_DLG_NONE;
	_fileMustExist	= NULL;
	_seeDetails		= FALSE;
	_pElem			= NULL;
	_hImageList		= NULL;
	_iUImgPos		= 0;
}


UINT PropDlg::doDialog(LPTSTR pName, LPTSTR pLink, LPTSTR pDesc, eLinkDlg linkDlg, BOOL fileMustExist)
{
	_pName			= pName;
	_pLink			= pLink;
	_pDesc			= pDesc;
	_linkDlg		= linkDlg;
	_fileMustExist	= fileMustExist;
	return ::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_PROP_DLG), _hParent,  (DLGPROC)dlgProc, (LPARAM)this);
}


BOOL CALLBACK PropDlg::run_dlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) 
	{
		case WM_INITDIALOG:
		{
			TCHAR	szDesc[256];

			sprintf(szDesc, "%s:", _pDesc);
			::SetWindowText(::GetDlgItem(_hSelf, IDC_STATIC_FAVES_DESC), szDesc);

			::SetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_NAME), _pName);
			::SetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_LINK), _pLink);

			SetFocus(::GetDlgItem(_hSelf, IDC_EDIT_NAME));
			_hTreeCtrl = ::GetDlgItem(_hSelf, IDC_TREE_SELECT);

			goToCenter();

			if (_linkDlg == LINK_DLG_NONE)
			{
				RECT	rcName, rcLink;

				::ShowWindow(::GetDlgItem(_hSelf, IDC_BTN_OPENDLG), SW_HIDE);
				::GetWindowRect(::GetDlgItem(_hSelf, IDC_EDIT_NAME), &rcName);
				::GetWindowRect(::GetDlgItem(_hSelf, IDC_EDIT_LINK), &rcLink);

				rcLink.right = rcName.right;

				ScreenToClient(_hSelf, &rcLink);
				::SetWindowPos(::GetDlgItem(_hSelf, IDC_EDIT_LINK), NULL, rcLink.left, rcLink.top, rcLink.right-rcLink.left, rcLink.bottom-rcLink.top, SWP_NOZORDER);
			}

			if (_seeDetails == FALSE)
			{
				RECT	rc	= {0};

				::DestroyWindow(::GetDlgItem(_hSelf, IDC_TREE_SELECT));
				::DestroyWindow(::GetDlgItem(_hSelf, IDC_STATIC_SELECT));
				::DestroyWindow(::GetDlgItem(_hSelf, IDC_BUTTON_DETAILS));

				/* resize window */
				::GetWindowRect(_hSelf, &rc);
				rc.top		+= 74;
				rc.bottom	-= 74;
				ScreenToClient(_hParent, &rc);

				/* resize window and keep sure to resize correct */
				::SetWindowPos(_hSelf, NULL, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, SWP_NOZORDER);
			}
			else
			{
				/* get current icon offset */
				UINT	iIconPos	= _pElem->uParam & FAVES_PARAM;

				/* set image list */
				::SendMessage(_hTreeCtrl, TVM_SETIMAGELIST, TVSIL_NORMAL, (LPARAM)_hImageList);

				HTREEITEM hItem = InsertItem(_pElem->pszName, _iUImgPos + iIconPos, _iUImgPos + iIconPos, 0, 0, TVI_ROOT, TVI_LAST, TRUE, (LPARAM)_pElem);
				TreeView_SelectItem(_hTreeCtrl, hItem);
			}

			break;
		}
		case WM_COMMAND : 
		{
			switch (LOWORD(wParam))
			{
				case IDC_BUTTON_DETAILS:
				{
					RECT	rc	= {0};

					/* toggle visibility state */
					_seeDetails ^= TRUE;

					/* resize window */
					::GetWindowRect(_hSelf, &rc);

					if (_seeDetails == FALSE)
					{
						::ShowWindow(::GetDlgItem(_hSelf, IDC_TREE_SELECT), SW_HIDE);
						::ShowWindow(::GetDlgItem(_hSelf, IDC_STATIC_SELECT), SW_HIDE);

						::SetWindowText(::GetDlgItem(_hSelf, IDC_BUTTON_DETAILS), "Details >>");

						rc.bottom	-= 148;
					}
					else
					{
						::ShowWindow(::GetDlgItem(_hSelf, IDC_TREE_SELECT), SW_SHOW);
						::ShowWindow(::GetDlgItem(_hSelf, IDC_STATIC_SELECT), SW_SHOW);

						::SetWindowText(::GetDlgItem(_hSelf, IDC_BUTTON_DETAILS), "Details <<");

						rc.bottom	+= 148;
					}

					::SetWindowPos(_hSelf, NULL, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, SWP_NOZORDER);
					break;
				}
				case IDC_BTN_OPENDLG:
				{
					if (_linkDlg == LINK_DLG_FOLDER)
					{
						// This code was copied and slightly modifed from:
						// http://www.bcbdev.com/faqs/faq62.htm

						// SHBrowseForFolder returns a PIDL. The memory for the PIDL is
						// allocated by the shell. Eventually, we will need to free this
						// memory, so we need to get a pointer to the shell malloc COM
						// object that will free the PIDL later on.
						LPMALLOC pShellMalloc = 0;
						if (::SHGetMalloc(&pShellMalloc) == NO_ERROR)
						{
							// If we were able to get the shell malloc object,
							// then proceed by initializing the BROWSEINFO stuct
							BROWSEINFO info;
							ZeroMemory(&info, sizeof(info));
							info.hwndOwner			= _hParent;
							info.pidlRoot			= NULL;
							info.pszDisplayName		= (LPTSTR)new TCHAR[MAX_PATH];
							info.lpszTitle			= "Select a folder:";
							info.ulFlags			= 0;
							info.lpfn				= BrowseCallbackProc;
							info.lParam				= (LPARAM)_pLink;

							// Execute the browsing dialog.
							LPITEMIDLIST pidl = ::SHBrowseForFolder(&info);

							// pidl will be null if they cancel the browse dialog.
							// pidl will be not null when they select a folder.
							if (pidl) 
							{
								// Try to convert the pidl to a display string.
								// Return is true if success.
								if (::SHGetPathFromIDList(pidl, _pLink))
								{
									// Set edit control to the directory path.
									::SetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_LINK), _pLink);
								}
								pShellMalloc->Free(pidl);
							}
							pShellMalloc->Release();
							delete [] info.pszDisplayName;
						}
					}
					else
					{
						LPTSTR	pszLink	= NULL;

						FileDlg dlg(_hInst, _hParent);

						dlg.setDefFileName(_pLink);
						dlg.setExtFilter("Session file", ".session", NULL);
						dlg.setExtFilter("All types", ".*", NULL);
						
						if (_fileMustExist == TRUE)
						{
							pszLink = dlg.doSaveDlg();
						}
						else
						{
							pszLink = dlg.doOpenSingleFileDlg();
						}

						if (pszLink != NULL)
						{
							// Set edit control to the directory path.
							strcpy(_pLink, pszLink);
							::SetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_LINK), _pLink);
						}
					}
					
					break;
				}
				case IDCANCEL:
				{
					::EndDialog(_hSelf, FALSE);
					return TRUE;
				}
				case IDOK:
				{
					UINT	lengthName	= ::SendDlgItemMessage(_hSelf, IDC_EDIT_NAME, WM_GETTEXTLENGTH, 0, 0) + 1;
					UINT	lengthLink	= ::SendDlgItemMessage(_hSelf, IDC_EDIT_LINK, WM_GETTEXTLENGTH, 0, 0) + 1;

					SendDlgItemMessage(_hSelf, IDC_EDIT_NAME, WM_GETTEXT, lengthName, (LPARAM)_pName);
					SendDlgItemMessage(_hSelf, IDC_EDIT_LINK, WM_GETTEXT, lengthLink, (LPARAM)_pLink);

					if ((strlen(_pName) != 0) && (strlen(_pLink) != 0))
					{
						LPTSTR	pszGroupName = (LPTSTR)new TCHAR[MAX_PATH];

						GetFolderPathName(TreeView_GetSelection(_hTreeCtrl), pszGroupName);
						_strGroupName = pszGroupName;

						delete [] pszGroupName;

						::EndDialog(_hSelf, TRUE);
						return TRUE;
					}
					else
					{
						::MessageBox(_hSelf, "Fill out all fields", "Error", MB_OK);
					}
					break;
				}
				default:
					break;
			}
			break;
		}
		case WM_SIZE:
		{
			RECT	rc		= {0};
			RECT	rcMain	= {0};

			/* get main window size */
			::GetWindowRect(_hSelf, &rcMain);

			/* resize static box */
			::GetWindowRect(::GetDlgItem(_hSelf, IDC_STATIC_FAVES_DESC), &rc);
			rc.bottom	= rcMain.bottom - 46;
			ScreenToClient(_hSelf, &rc);
			::SetWindowPos(::GetDlgItem(_hSelf, IDC_STATIC_FAVES_DESC), NULL, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, SWP_NOZORDER);

			/* set position of OK button */
			::GetWindowRect(::GetDlgItem(_hSelf, IDOK), &rc);
			rc.top		= rcMain.bottom - 36;
			rc.bottom	= rcMain.bottom - 12;
			ScreenToClient(_hSelf, &rc);
			::SetWindowPos(::GetDlgItem(_hSelf, IDOK), NULL, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, SWP_NOZORDER);

			/* set position of CANCEL button */
			::GetWindowRect(::GetDlgItem(_hSelf, IDCANCEL), &rc);
			rc.top		= rcMain.bottom - 36;
			rc.bottom	= rcMain.bottom - 12;
			ScreenToClient(_hSelf, &rc);
			::SetWindowPos(::GetDlgItem(_hSelf, IDCANCEL), NULL, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, SWP_NOZORDER);

			/* set position of DETAILS button */
			::GetWindowRect(::GetDlgItem(_hSelf, IDC_BUTTON_DETAILS), &rc);
			rc.top		= rcMain.bottom - 36;
			rc.bottom	= rcMain.bottom - 12;
			ScreenToClient(_hSelf, &rc);
			::SetWindowPos(::GetDlgItem(_hSelf, IDC_BUTTON_DETAILS), NULL, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, SWP_NOZORDER);
			break;
		}
		case WM_NOTIFY:
		{
			LPNMHDR		nmhdr = (LPNMHDR)lParam;

			if (nmhdr->hwndFrom == _hTreeCtrl)
			{
				switch (nmhdr->code)
				{
					case TVN_ITEMEXPANDING:
					{
						DWORD			dwpos = ::GetMessagePos();
						TVHITTESTINFO	ht;
						HTREEITEM		hItem;

						ht.pt.x = GET_X_LPARAM(dwpos);
						ht.pt.y = GET_Y_LPARAM(dwpos);

						::ScreenToClient(_hTreeCtrl, &ht.pt);

						hItem = TreeView_HitTest(_hTreeCtrl, &ht);

						if (hItem != NULL)
						{
							if (!TreeView_GetChild(_hTreeCtrl, hItem))
							{
								DrawChildrenOfItem(hItem);
							}
						}
						break;
					}
					case TVN_SELCHANGED:
					{
						/* only when link params are also viewed */
						if (_bWithLink == TRUE)
						{
							HTREEITEM	hItem = TreeView_GetSelection(_hTreeCtrl);

							if (hItem != NULL)
							{
								PELEM	pElem = (PELEM)GetParam(hItem);

								if (pElem != NULL)
								{
									if (pElem->uParam & FAVES_PARAM_LINK)
									{
										::SetDlgItemText(_hSelf, IDC_EDIT_NAME, pElem->pszName);
										::SetDlgItemText(_hSelf, IDC_EDIT_LINK, pElem->pszLink);
									}
									else
									{
										::SetDlgItemText(_hSelf, IDC_EDIT_NAME, "");
										::SetDlgItemText(_hSelf, IDC_EDIT_LINK, "");
									}
								}
							}
						}
						break;
					}
				}
			}
			break;
		}
		case WM_DESTROY :
		{
			/* deregister this dialog */
			::SendMessage(_hParent, WM_MODELESSDIALOG, MODELESSDIALOGREMOVE, (LPARAM)_hSelf);
			break;
		}
		default:
			break;
	}
	return FALSE;
}

void PropDlg::setTreeElements(PELEM pElem, HIMAGELIST hImageList, INT iUImgPos, BOOL bWithLink)
{
	_pElem		= pElem;
	_hImageList	= hImageList;
	_iUImgPos	= iUImgPos;
	_bWithLink	= bWithLink;

	/* do not destroy items */
	_seeDetails = TRUE;
}

LPCTSTR PropDlg::getGroupName(void)
{
	return _strGroupName.c_str();
}

void PropDlg::DrawChildrenOfItem(HTREEITEM hParentItem)
{
	BOOL		haveChildren	= FALSE;
	int			root			= 0;
	LPTSTR		TEMP			= (LPTSTR)new TCHAR[MAX_PATH];
	HTREEITEM	pCurrentItem	= TreeView_GetNextItem(_hTreeCtrl, hParentItem, TVGN_CHILD);

	/* get element list */
	PELEM		parentElement	= (PELEM)GetParam(hParentItem);

	if (parentElement != NULL)
	{
		/* update elements in parent tree */
		for (size_t i = 0; i < parentElement->vElements.size(); i++)
		{
			PELEM	pElem	= &parentElement->vElements[i];

			/* get root */
			root = pElem->uParam & FAVES_PARAM;

			/* initialize children */
			haveChildren		= FALSE;

			if (pElem->uParam & FAVES_PARAM_GROUP)
			{
				if (pElem->vElements.size() != 0)
				{
					if ((pElem->vElements[0].uParam & FAVES_PARAM_GROUP) || (_bWithLink == TRUE))
					{
						haveChildren = TRUE;
					}
				}
				/* add new item */
				pCurrentItem = InsertItem(pElem->pszName, _iUImgPos + 4, _iUImgPos + 4, 0, 0, hParentItem, TVI_LAST, haveChildren, (LPARAM)pElem);
			}

			if ((pElem->uParam & FAVES_PARAM_LINK) || (_bWithLink == TRUE))
			{
				/* add new item */
				pCurrentItem = InsertItem(pElem->pszName, _iUImgPos + 3, _iUImgPos + 3, 0, 0, hParentItem, TVI_LAST, haveChildren, (LPARAM)pElem);
			}
		}
	}
}


void PropDlg::GetFolderPathName(HTREEITEM hItem, LPTSTR name)
{
	/* return if current folder is root folder */
	if (hItem == TVI_ROOT)
	{
		return;
	}

	/* delete folder path name */
	name[0]	= '\0';

	/* create temp resources */
	LPTSTR	TEMP	= (LPTSTR)new TCHAR[MAX_PATH];
	LPTSTR	szName	= (LPTSTR)new TCHAR[MAX_PATH];

	/* join elements together */
	while (hItem != NULL)
	{
		GetItemText(hItem, szName, MAX_PATH);
		sprintf(TEMP, "%s %s", szName, name);
		strcpy(name, TEMP);
		hItem = TreeView_GetNextItem(_hTreeCtrl, hItem, TVGN_PARENT);
	}

	delete [] TEMP;
	delete [] szName;
}