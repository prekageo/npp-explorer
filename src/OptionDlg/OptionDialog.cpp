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


#include "OptionDialog.h"
#include "Explorer.h"
#include <Commctrl.h>
#include <shlobj.h>



UINT OptionDlg::doDialog(tExProp *prop)
{
	_pProp = prop;
	return ::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_OPTION_DLG), _hParent,  (DLGPROC)dlgProc, (LPARAM)this);
}


BOOL CALLBACK OptionDlg::run_dlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) 
	{
		case WM_INITDIALOG:
		{
			goToCenter();

			for (int i = 0; i < SFMT_MAX; i++)
			{
				::SendDlgItemMessage(_hSelf, IDC_COMBO_SIZE_FORMAT, CB_ADDSTRING, 0, (LPARAM)pszSizeFmt[i]);
			}
			for (i = 0; i < DFMT_MAX; i++)
			{
				::SendDlgItemMessage(_hSelf, IDC_COMBO_DATE_FORMAT, CB_ADDSTRING, 0, (LPARAM)pszDateFmt[i]);
			}
			::SendDlgItemMessage(_hSelf, IDC_EDIT_TIMEOUT, EM_LIMITTEXT, 5, 0);
			
			SetParams();
			LongUpdate();

			/* change language */
			NLChangeDialog(_hInst, _nppData._nppHandle, _hSelf, "Options");
			NLChangeCombo(_hInst, _nppData._nppHandle, ::GetDlgItem(_hSelf, IDC_COMBO_SIZE_FORMAT), "ComboSize", SFMT_MAX);
			NLChangeCombo(_hInst, _nppData._nppHandle, ::GetDlgItem(_hSelf, IDC_COMBO_DATE_FORMAT), "ComboDate", DFMT_MAX);

			break;
		}
		case WM_COMMAND : 
		{
			switch (LOWORD(wParam))
			{
				case IDC_CHECK_LONG:
				{
					LongUpdate();
					return TRUE;
				}
				case IDCANCEL:
					::EndDialog(_hSelf, IDCANCEL);
					return TRUE;
				case IDOK:
				{
					if (GetParams() == FALSE)
					{
						return FALSE;
					}
					::EndDialog(_hSelf, IDOK);
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
	return FALSE;
}


void OptionDlg::LongUpdate(void)
{
	BOOL	bViewLong = FALSE;

	if (::SendDlgItemMessage(_hSelf, IDC_CHECK_LONG, BM_GETCHECK, 0, 0) == BST_CHECKED)
		bViewLong = TRUE;

	::EnableWindow(::GetDlgItem(_hSelf, IDC_COMBO_SIZE_FORMAT), bViewLong);
	::EnableWindow(::GetDlgItem(_hSelf, IDC_COMBO_DATE_FORMAT), bViewLong);
}


void OptionDlg::SetParams(void)
{
	::SendDlgItemMessage(_hSelf, IDC_CHECK_LONG, BM_SETCHECK, _pProp->bViewLong?BST_CHECKED:BST_UNCHECKED, 0);
	::SendDlgItemMessage(_hSelf, IDC_COMBO_SIZE_FORMAT, CB_SETCURSEL, (WPARAM)_pProp->fmtSize, 0);
	::SendDlgItemMessage(_hSelf, IDC_COMBO_DATE_FORMAT, CB_SETCURSEL, (WPARAM)_pProp->fmtDate, 0);

	::SendDlgItemMessage(_hSelf, IDC_CHECK_SEPEXT, BM_SETCHECK, _pProp->bAddExtToName?BST_UNCHECKED:BST_CHECKED, 0);
	::SendDlgItemMessage(_hSelf, IDC_CHECK_BRACES, BM_SETCHECK, _pProp->bViewBraces?BST_CHECKED:BST_UNCHECKED, 0);
	::SendDlgItemMessage(_hSelf, IDC_CHECK_AUTO, BM_SETCHECK, _pProp->bAutoUpdate?BST_CHECKED:BST_UNCHECKED, 0);
	::SendDlgItemMessage(_hSelf, IDC_CHECK_HIDDEN, BM_SETCHECK, _pProp->bShowHidden?BST_CHECKED:BST_UNCHECKED, 0);
	::SendDlgItemMessage(_hSelf, IDC_CHECK_USEICON, BM_SETCHECK, _pProp->bUseSystemIcons?BST_CHECKED:BST_UNCHECKED, 0);

	char	TEMP[6];
	::SetDlgItemText(_hSelf, IDC_EDIT_TIMEOUT, itoa(_pProp->uTimeout, TEMP, 10));
}


BOOL OptionDlg::GetParams(void)
{
	BOOL		bRet	= TRUE;

	if (::SendDlgItemMessage(_hSelf, IDC_CHECK_LONG, BM_GETCHECK, 0, 0) == BST_CHECKED)
		_pProp->bViewLong = TRUE;
	else
		_pProp->bViewLong = FALSE;

	_pProp->fmtSize = (eSizeFmt)::SendDlgItemMessage(_hSelf, IDC_COMBO_SIZE_FORMAT, CB_GETCURSEL, 0, 0);
	_pProp->fmtDate = (eDateFmt)::SendDlgItemMessage(_hSelf, IDC_COMBO_DATE_FORMAT, CB_GETCURSEL, 0, 0);

	if (::SendDlgItemMessage(_hSelf, IDC_CHECK_SEPEXT, BM_GETCHECK, 0, 0) == BST_CHECKED)
		_pProp->bAddExtToName = FALSE;
	else
		_pProp->bAddExtToName = TRUE;

	if (::SendDlgItemMessage(_hSelf, IDC_CHECK_AUTO, BM_GETCHECK, 0, 0) == BST_CHECKED)
		_pProp->bAutoUpdate = TRUE;
	else
		_pProp->bAutoUpdate = FALSE;

	if (::SendDlgItemMessage(_hSelf, IDC_CHECK_BRACES, BM_GETCHECK, 0, 0) == BST_CHECKED)
		_pProp->bViewBraces = TRUE;
	else
		_pProp->bViewBraces = FALSE;

	if (::SendDlgItemMessage(_hSelf, IDC_CHECK_HIDDEN, BM_GETCHECK, 0, 0) == BST_CHECKED)
		_pProp->bShowHidden = TRUE;
	else
		_pProp->bShowHidden = FALSE;

	if (::SendDlgItemMessage(_hSelf, IDC_CHECK_USEICON, BM_GETCHECK, 0, 0) == BST_CHECKED)
		_pProp->bUseSystemIcons = TRUE;
	else
		_pProp->bUseSystemIcons = FALSE;

	char	TEMP[6];
	::GetDlgItemText(_hSelf, IDC_EDIT_TIMEOUT, TEMP, 6);
	_pProp->uTimeout = (UINT)atoi(TEMP);

	return bRet;
}




