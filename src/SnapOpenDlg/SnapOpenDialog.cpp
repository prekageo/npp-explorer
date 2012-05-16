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


#include "SnapOpenDialog.h"
#include "Explorer.h"
#include "PatternMatch.h"
#include <ExplorerDialog.h>
#include <Commctrl.h>
#include <shlobj.h>


// Set a call back with the handle after init to set the path.
// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/shellcc/platform/shell/reference/callbackfunctions/browsecallbackproc.asp
static int __stdcall BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM, LPARAM pData)
{
	if (uMsg == BFFM_INITIALIZED)
		::SendMessage(hwnd, BFFM_SETSELECTION, TRUE, pData);
	return 0;
};


UINT SnapOpenDlg::doDialog()
{
	return ::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_QUICK_OPEN_DLG), _hParent,  (DLGPROC)dlgProc, (LPARAM)this);
}


BOOL CALLBACK SnapOpenDlg::run_dlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) 
	{
		case WM_INITDIALOG:
		{
			string rootPath = getRoot();
			rootPathLen = rootPath.size();

			goToCenter();
			findFiles();
			populateResultList();
			::PostMessage(_hSelf, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(_hSelf, IDC_EDIT_SEARCH), TRUE);
 
			// Subclass the edit control. 
			SetWindowLong(GetDlgItem(_hSelf, IDC_EDIT_SEARCH), GWL_USERDATA, (LONG) this);
			wpOrigEditProc = (WNDPROC) SetWindowLong(GetDlgItem(_hSelf, IDC_EDIT_SEARCH), GWL_WNDPROC, (LONG) editSubclassProc);

			string title = _T("Snap Open - ") + rootPath;
			SetWindowText(_hSelf, title.c_str());

			break;
		}
		case WM_DESTROY: 
			// Remove the subclass from the edit control. 
			SetWindowLong(GetDlgItem(_hSelf, IDC_EDIT_SEARCH), GWL_WNDPROC, (LONG) wpOrigEditProc);
			break;
		case WM_COMMAND : 
		{
			switch (LOWORD(wParam))
			{
				case IDC_EDIT_SEARCH:
					if (HIWORD(wParam) == EN_CHANGE)
					{
						populateResultList();
					}
					break;
				case IDCANCEL:
					::EndDialog(_hSelf, IDCANCEL);
					return TRUE;
				case IDC_LIST_RESULTS:
					if (HIWORD(wParam) != LBN_DBLCLK)
					{
						break;
					}
				case IDOK:
				{
					int selection;
					string fullPath = getRoot();
					TCHAR pszFilePath[MAX_PATH];
					pszFilePath[0] = 0;
					selection = ::SendDlgItemMessage(_hSelf, IDC_LIST_RESULTS, LB_GETCURSEL, 0, 0);
					if (selection != LB_ERR)
					{
						::SendDlgItemMessage(_hSelf, IDC_LIST_RESULTS, LB_GETTEXT, selection, (LPARAM)&pszFilePath);
					}
					::EndDialog(_hSelf, IDOK);
					if (pszFilePath[0])
					{
						fullPath += pszFilePath;
						::SendMessage(_nppData._nppHandle, NPPM_DOOPEN, 0, (LPARAM)fullPath.c_str());
					}
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

string SnapOpenDlg::getRoot()
{
	extern ExplorerDialog explorerDlg;
	string path = explorerDlg.GetRootPath();
	if (path.empty())
	{
		path = explorerDlg.GetSelectedPath();
	}
	return path;
}

void SnapOpenDlg::findFiles()
{
	files.clear();
	findFilesRecursively(getRoot().c_str());
}

void SnapOpenDlg::findFilesRecursively(LPCTSTR lpFolder)
{
	TCHAR szFullPattern[MAX_PATH];
	WIN32_FIND_DATA FindFileData;
	HANDLE hFindFile;
	// first we are going to process any subdirectories
	PathCombine(szFullPattern, lpFolder, _T("*"));
	hFindFile = FindFirstFile(szFullPattern, &FindFileData);
	if(hFindFile != INVALID_HANDLE_VALUE)
	{
		do
		{
			if(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (_tcscmp(FindFileData.cFileName, _T(".")) && _tcscmp(FindFileData.cFileName, _T("..")))
				{
					// found a subdirectory; recurse into it
					PathCombine(szFullPattern, lpFolder, FindFileData.cFileName);
					findFilesRecursively(szFullPattern);
				}
			}
			else
			{
				PathCombine(szFullPattern, lpFolder, FindFileData.cFileName);
				files.push_back(&szFullPattern[rootPathLen]);
			}
		} while(FindNextFile(hFindFile, &FindFileData));
		FindClose(hFindFile);
	}
}

void SnapOpenDlg::populateResultList()
{
	TCHAR pattern[MAX_PATH];
	Edit_GetText(GetDlgItem(_hSelf, IDC_EDIT_SEARCH), pattern, _countof(pattern));
	::SendDlgItemMessage(_hSelf, IDC_LIST_RESULTS, LB_RESETCONTENT, 0, 0);
	int count = 0;
	for (vector<wstring>::const_iterator i=files.begin();i!=files.end();++i)
	{
		if (count >= 100)
		{
			::SendDlgItemMessage(_hSelf, IDC_LIST_RESULTS, LB_ADDSTRING, 0, (LPARAM)_T("-- Too many results --"));
			break;
		}
		if (patternMatch(i->c_str(), pattern))
		{
			count++;
			::SendDlgItemMessage(_hSelf, IDC_LIST_RESULTS, LB_ADDSTRING, 0, (LPARAM)(*i).c_str());
		}
	}
	::SendDlgItemMessage(_hSelf, IDC_LIST_RESULTS, LB_SETCURSEL, 0, 0);
}

LRESULT APIENTRY SnapOpenDlg::editSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
	SnapOpenDlg *dlg = (SnapOpenDlg *)(GetWindowLong(hwnd, GWL_USERDATA));
	return dlg->run_editSubclassProc(hwnd, uMsg, wParam, lParam);
}

LRESULT APIENTRY SnapOpenDlg::run_editSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
	if (uMsg == WM_KEYDOWN)
	{
		int selection = ::SendDlgItemMessage(_hSelf, IDC_LIST_RESULTS, LB_GETCURSEL, 0, 0);
		int results = ::SendDlgItemMessage(_hSelf, IDC_LIST_RESULTS, LB_GETCOUNT, 0, 0);

		if (wParam == VK_UP && selection > 0)
		{
			::SendDlgItemMessage(_hSelf, IDC_LIST_RESULTS, LB_SETCURSEL, selection-1, 0);
			return 0;
		}
		if (wParam == VK_DOWN && selection < results - 1)
		{
			::SendDlgItemMessage(_hSelf, IDC_LIST_RESULTS, LB_SETCURSEL, selection+1, 0);
			return 0;
		}
	}
 
	return ::CallWindowProc(wpOrigEditProc, hwnd, uMsg, wParam, lParam);
}