/***********************************************************\
*	Original in MFC by Roman Engels		Copyright 2003		*
*															*
*	http://www.codeproject.com/shell/shellcontextmenu.asp	*
\***********************************************************/

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


#include "stdafx.h"
#include "FavesDialog.h"
#include "ContextMenu.h"


#define MIN_ID 1
#define MAX_ID 10000

IContextMenu2 * g_IContext2		= NULL;
IContextMenu3 * g_IContext3		= NULL;

WNDPROC			g_OldWndProc	= NULL;


ContextMenu::ContextMenu()
{
	_psfFolder			= NULL;
	_pidlArray			= NULL;
	_hMenu				= NULL;
	_strFirstElement	= "";
}

ContextMenu::~ContextMenu()
{
	/* free all allocated datas */
	if (_psfFolder && _bDelete)
		_psfFolder->Release ();
	_psfFolder = NULL;
	FreePIDLArray(_pidlArray);
	_pidlArray = NULL;

	::DestroyMenu(_hMenu);
}



// this functions determines which version of IContextMenu is avaibale for those objects (always the highest one)
// and returns that interface
BOOL ContextMenu::GetContextMenu (void ** ppContextMenu, int & iMenuType)
{
	*ppContextMenu = NULL;
	LPCONTEXTMENU icm1 = NULL;
	
	// first we retrieve the normal IContextMenu interface (every object should have it)
	_psfFolder->GetUIObjectOf (NULL, _nItems, (LPCITEMIDLIST *) _pidlArray, IID_IContextMenu, NULL, (void**) &icm1);

	if (icm1)
	{	// since we got an IContextMenu interface we can now obtain the higher version interfaces via that
		if (icm1->QueryInterface (IID_IContextMenu3, ppContextMenu) == NOERROR)
			iMenuType = 3;
		else if (icm1->QueryInterface (IID_IContextMenu2, ppContextMenu) == NOERROR)
			iMenuType = 2;

		if (*ppContextMenu) 
			icm1->Release(); // we can now release version 1 interface, cause we got a higher one
		else 
		{	
			iMenuType = 1;
			*ppContextMenu = icm1;	// since no higher versions were found
		}							// redirect ppContextMenu to version 1 interface
	}
	else
		return (FALSE);	// something went wrong
	
	return (TRUE); // success
}


LRESULT CALLBACK ContextMenu::HookWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{ 
		case WM_MENUCHAR:	// only supported by IContextMenu3
		{
			if (g_IContext3)
			{
				LRESULT lResult = 0;
				g_IContext3->HandleMenuMsg2 (message, wParam, lParam, &lResult);
				return (lResult);
			}
			break;
		}
		case WM_DRAWITEM:
		case WM_MEASUREITEM:
		{
			if (wParam) 
				break; // if wParam != 0 then the message is not menu-related
		}
		case WM_INITMENUPOPUP:
		{
			if (g_IContext2)
				g_IContext2->HandleMenuMsg (message, wParam, lParam);
			else	// version 3
				g_IContext3->HandleMenuMsg (message, wParam, lParam);
			return (message == WM_INITMENUPOPUP ? 0 : TRUE); // inform caller that we handled WM_INITPOPUPMENU by ourself
			break;
		}
		default:
			break;
	}

	// call original WndProc of window to prevent undefined bevhaviour of window
	return ::CallWindowProc (g_OldWndProc, hWnd, message, wParam, lParam);
}


UINT ContextMenu::ShowContextMenu(HWND hWndNpp, HWND hWndParent, POINT pt, bool normal)
{
	/* store notepad handle */
	_hWndNpp	= hWndNpp;
	_hWndParent = hWndParent;

	// to know which version of IContextMenu is supported
	int iMenuType = 0;

	// common pointer to IContextMenu and higher version interface
	LPCONTEXTMENU pContextMenu = NULL;

	if (_pidlArray != NULL)
	{
		if (!_hMenu)
		{
			_hMenu = NULL;
			_hMenu = ::CreateMenu();
		}

		if (!GetContextMenu((void**) &pContextMenu, iMenuType))	
			return (0);	// something went wrong

		// lets fill out our popupmenu 
		pContextMenu->QueryContextMenu( _hMenu,
										::GetMenuItemCount(_hMenu),
										MIN_ID,
										MAX_ID,
										CMF_NORMAL | ((_strFirstElement.size() > 4)?CMF_CANRENAME:0));
 
		// subclass window to handle menurelated messages in ContextMenu 
		g_OldWndProc	= NULL;
		if (iMenuType > 1)	// only subclass if its version 2 or 3
		{
			g_OldWndProc = (WNDPROC)::SetWindowLong (hWndParent, GWL_WNDPROC, (DWORD) HookWndProc);
			if (iMenuType == 2)
				g_IContext2 = (LPCONTEXTMENU2) pContextMenu;
			else	// version 3
				g_IContext3 = (LPCONTEXTMENU3) pContextMenu;
		}
	}

	/************************************* modification for notepad ***********************************/
	HMENU			hMainMenu	= ::CreatePopupMenu();
	bool			isFolder	= (_strFirstElement[_strFirstElement.size()-1] == '\\');

	/* Add notepad menu items */
	if (isFolder)
	{
		::AppendMenu(hMainMenu, MF_STRING, MAX_ID + 1, "New File...");
		::AppendMenu(hMainMenu, MF_STRING, MAX_ID + 2, "New Folder...");
		::AppendMenu(hMainMenu, MF_STRING, MAX_ID + 3, "Find in Files...");
	}
	else
	{
		::AppendMenu(hMainMenu, MF_STRING, MAX_ID + 4, "Open");
		::AppendMenu(hMainMenu, MF_STRING, MAX_ID + 5, "Open in Other View");
		::AppendMenu(hMainMenu, MF_STRING, MAX_ID + 6, "Open in New Instance");
	}
	::InsertMenu(hMainMenu, 3, MF_BYPOSITION | MF_SEPARATOR, 0, 0);
	::AppendMenu(hMainMenu, MF_STRING, MAX_ID + 7, "Add to 'Favorites'...");

	if (isFolder)
	{
		::AppendMenu(hMainMenu, MF_STRING, MAX_ID + 8, "Add Path(s) to Document");
	}
	else
	{
		::AppendMenu(hMainMenu, MF_STRING, MAX_ID + 8, "Full File Path(s) to Document");
		::AppendMenu(hMainMenu, MF_STRING, MAX_ID + 9, "File Name(s) to Document");
	}

	if (_pidlArray != NULL)
	{
		int				copyAt		= -1;
		int				items		= ::GetMenuItemCount(_hMenu);
		char*			szText		= (char*)new char[256];
		MENUITEMINFO	info		= {0};

		info.cbSize		= sizeof(MENUITEMINFO);
		info.fMask		= MIIM_TYPE | MIIM_ID | MIIM_SUBMENU;

		::AppendMenu(hMainMenu, MF_SEPARATOR, 0, 0);

		if (normal)
		{
			/* store all items in an seperate sub menu until "cut" (25) or "copy" (26) */
			for (int i = 0; i < items; i++)
			{
				info.cch		= 256;
				info.dwTypeData	= szText;
				if (copyAt == -1)
				{
					::GetMenuItemInfo(_hMenu, i, TRUE, &info);
					if ((info.wID == 25) || (info.wID == 26))
					{
						copyAt	= i - 1;
						::AppendMenu(hMainMenu, info.fType, info.wID, info.dwTypeData);
						::DeleteMenu(_hMenu, i  , MF_BYPOSITION);
						::DeleteMenu(_hMenu, i-1, MF_BYPOSITION);
					}
				}
				else
				{
					::GetMenuItemInfo(_hMenu, copyAt, TRUE, &info);
					::AppendMenu(hMainMenu, info.fType, info.wID, info.dwTypeData);
					::DeleteMenu(_hMenu, copyAt, MF_BYPOSITION);
				}
			}

			::InsertMenu(hMainMenu, 4, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT)_hMenu, "Standard Menu");
			::InsertMenu(hMainMenu, 5, MF_BYPOSITION | MF_SEPARATOR, 0, 0);
		}
		else
		{
			/* ignore all items until "cut" (25) or "copy" (26) */
			for (int i = 0; i < items; i++)
			{
				info.cch		= 256;
				info.dwTypeData	= szText;
				::GetMenuItemInfo(_hMenu, i, TRUE, &info);
				if ((copyAt == -1) && ((info.wID == 25) || (info.wID == 26)))
				{
					copyAt	= 0;
				}
				else if ((info.wID == 20) || (info.wID == 27))
				{
					::AppendMenu(hMainMenu, info.fType, info.wID, info.dwTypeData);
					::AppendMenu(hMainMenu, MF_SEPARATOR, 0, 0);
				}
			}
			::DeleteMenu(hMainMenu, ::GetMenuItemCount(hMainMenu) - 1, MF_BYPOSITION);
		}

		delete [] szText;
	}

	/*****************************************************************************************************/

	UINT idCommand = ::TrackPopupMenu(hMainMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hWndParent, NULL);

	/* free resources */
	::DestroyMenu(hMainMenu);

	if ((_pidlArray != NULL) && (g_OldWndProc != NULL)) // unsubclass
	{
		::SetWindowLong(hWndParent, GWL_WNDPROC, (DWORD) g_OldWndProc);
	}

	// see if returned idCommand belongs to shell menu entries but not for renaming (19)
	if ((idCommand >= MIN_ID) && (idCommand <= MAX_ID) && (idCommand != 19))	
	{
		InvokeCommand (pContextMenu, idCommand - MIN_ID);	// execute related command
		idCommand = 0;
	}
	else
	{

	/************************************* modification for notepad ***********************************/

		switch (idCommand)
		{
			case 19:
			{
				Rename();
				break;
			}
			case MAX_ID + 1:
			{
				newFile();
				break;
			}
			case MAX_ID + 2:
			{
				newFolder();
				break;
			}
			case MAX_ID + 3:
			{
				findInFiles();
				break;
			}
			case MAX_ID + 4:
			{
				openFile();
				break;
			}
			case MAX_ID + 5:
			{
				openFileInOtherView();
				break;
			}
			case MAX_ID + 6:
			{
				openFileInNewInstance();
				break;
			}
			case MAX_ID + 7:
			{
				addToFaves(isFolder);
				break;
			}
			case MAX_ID + 8:
			{
				addFullPaths();
				break;
			}
			case MAX_ID + 9:
			{
				addFileNames();
				break;
			}
			default:
				break;
		}

	/*****************************************************************************************************/

	}
	
	if (pContextMenu != NULL)
		pContextMenu->Release();
	g_IContext2 = NULL;
	g_IContext3 = NULL;

	return (idCommand);
}


void ContextMenu::InvokeCommand (LPCONTEXTMENU pContextMenu, UINT idCommand)
{
	CMINVOKECOMMANDINFO cmi = {0};
	cmi.cbSize = sizeof (CMINVOKECOMMANDINFO);
	cmi.lpVerb = (LPSTR) MAKEINTRESOURCE (idCommand);
	cmi.nShow = SW_SHOWNORMAL;
	
	pContextMenu->InvokeCommand (&cmi);
}



void ContextMenu::SetObjects(string strObject)
{
	// only one object is passed
	vector<string>	strArray;
	strArray.push_back(strObject);	// create a CStringArray with one element
	
	SetObjects (strArray);			// and pass it to SetObjects (vector<string> strArray)
									// for further processing
}


void ContextMenu::SetObjects(vector<string> strArray)
{
	// store also the string for later menu use
	_strFirstElement = strArray[0];
	_strArray		 = strArray;

	// free all allocated datas
	if (_psfFolder && _bDelete)
		_psfFolder->Release ();
	_psfFolder = NULL;
	FreePIDLArray (_pidlArray);
	_pidlArray = NULL;
	
	// get IShellFolder interface of Desktop (root of shell namespace)
	IShellFolder * psfDesktop = NULL;
	SHGetDesktopFolder (&psfDesktop);	// needed to obtain full qualified pidl

	// ParseDisplayName creates a PIDL from a file system path relative to the IShellFolder interface
	// but since we use the Desktop as our interface and the Desktop is the namespace root
	// that means that it's a fully qualified PIDL, which is what we need
	LPITEMIDLIST pidl = NULL;

#ifndef _UNICODE
	OLECHAR * olePath = NULL;
	olePath = (OLECHAR *) calloc (strArray[0].size() + 1, sizeof (OLECHAR));
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, strArray[0].c_str(), -1, olePath, strArray[0].size() + 1);	
	psfDesktop->ParseDisplayName (NULL, 0, olePath, NULL, &pidl, NULL);
	free (olePath);
#else
	psfDesktop->ParseDisplayName (NULL, 0, strArray[0].c_str(), NULL, &pidl, NULL);
#endif

	if (pidl != NULL)
	{
		// now we need the parent IShellFolder interface of pidl, and the relative PIDL to that interface
		LPITEMIDLIST pidlItem = NULL;	// relative pidl
		SHBindToParentEx (pidl, IID_IShellFolder, (void **) &_psfFolder, NULL);
		free (pidlItem);
		// get interface to IMalloc (need to free the PIDLs allocated by the shell functions)
		LPMALLOC lpMalloc = NULL;
		SHGetMalloc (&lpMalloc);
		if (lpMalloc != NULL) lpMalloc->Free (pidl);

		// now we have the IShellFolder interface to the parent folder specified in the first element in strArray
		// since we assume that all objects are in the same folder (as it's stated in the MSDN)
		// we now have the IShellFolder interface to every objects parent folder
		
		IShellFolder * psfFolder = NULL;
		_nItems = strArray.size();
		for (int i = 0; i < _nItems; i++)
		{
#ifndef _UNICODE
			olePath = (OLECHAR *) calloc (strArray[i].size() + 1, sizeof (OLECHAR));
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, strArray[i].c_str(), -1, olePath, strArray[i].size() + 1);	
			psfDesktop->ParseDisplayName (NULL, 0, olePath, NULL, &pidl, NULL);
			free (olePath);
#else
			psfDesktop->ParseDisplayName (NULL, 0, strArray[i].c_str(), NULL, &pidl, NULL);
#endif
			_pidlArray = (LPITEMIDLIST *) realloc (_pidlArray, (i + 1) * sizeof (LPITEMIDLIST));
			// get relative pidl via SHBindToParent
			SHBindToParentEx (pidl, IID_IShellFolder, (void **) &psfFolder, (LPCITEMIDLIST *) &pidlItem);
			_pidlArray[i] = CopyPIDL (pidlItem);	// copy relative pidl to pidlArray
			free (pidlItem);
			// free pidl allocated by ParseDisplayName
			if (lpMalloc != NULL) lpMalloc->Free (pidl);
			if (psfFolder != NULL) psfFolder->Release ();
		}

		if (lpMalloc != NULL) lpMalloc->Release ();
	}
	if (psfDesktop != NULL) psfDesktop->Release ();

	_bDelete = TRUE;	// indicates that _psfFolder should be deleted by ContextMenu
}


void ContextMenu::FreePIDLArray(LPITEMIDLIST *pidlArray)
{
	if (!pidlArray)
		return;

	int iSize = _msize (pidlArray) / sizeof (LPITEMIDLIST);

	for (int i = 0; i < iSize; i++)
		free (pidlArray[i]);
	free (pidlArray);
}


LPITEMIDLIST ContextMenu::CopyPIDL (LPCITEMIDLIST pidl, int cb)
{
	if (cb == -1)
		cb = GetPIDLSize (pidl); // Calculate size of list.

    LPITEMIDLIST pidlRet = (LPITEMIDLIST) calloc (cb + sizeof (USHORT), sizeof (BYTE));
    if (pidlRet)
		CopyMemory(pidlRet, pidl, cb);

    return (pidlRet);
}


UINT ContextMenu::GetPIDLSize (LPCITEMIDLIST pidl)
{  
	if (!pidl) 
		return 0;
	int nSize = 0;
	LPITEMIDLIST pidlTemp = (LPITEMIDLIST) pidl;
	while (pidlTemp->mkid.cb)
	{
		nSize += pidlTemp->mkid.cb;
		pidlTemp = (LPITEMIDLIST) (((LPBYTE) pidlTemp) + pidlTemp->mkid.cb);
	}
	return nSize;
}

HMENU ContextMenu::GetMenu()
{
	if (!_hMenu)
	{
		_hMenu = ::CreatePopupMenu();	// create the popupmenu (its empty)
	}
	return (_hMenu);
}


// this is workaround function for the Shell API Function SHBindToParent
// SHBindToParent is not available under Win95/98
HRESULT ContextMenu::SHBindToParentEx (LPCITEMIDLIST pidl, REFIID riid, VOID **ppv, LPCITEMIDLIST *ppidlLast)
{
	HRESULT hr = 0;
	if (!pidl || !ppv)
		return E_POINTER;
	
	int nCount = GetPIDLCount (pidl);
	if (nCount == 0)	// desktop pidl of invalid pidl
		return E_POINTER;

	IShellFolder * psfDesktop = NULL;
	SHGetDesktopFolder (&psfDesktop);
	if (nCount == 1)	// desktop pidl
	{
		if ((hr = psfDesktop->QueryInterface(riid, ppv)) == S_OK)
		{
			if (ppidlLast) 
				*ppidlLast = CopyPIDL (pidl);
		}
		psfDesktop->Release ();
		return hr;
	}

	LPBYTE pRel = GetPIDLPos (pidl, nCount - 1);
	LPITEMIDLIST pidlParent = NULL;
	pidlParent = CopyPIDL (pidl, pRel - (LPBYTE) pidl);
	IShellFolder * psfFolder = NULL;
	
	if ((hr = psfDesktop->BindToObject (pidlParent, NULL, __uuidof (psfFolder), (void **) &psfFolder)) != S_OK)
	{
		free (pidlParent);
		psfDesktop->Release ();
		return hr;
	}
	if ((hr = psfFolder->QueryInterface (riid, ppv)) == S_OK)
	{
		if (ppidlLast)
			*ppidlLast = CopyPIDL ((LPCITEMIDLIST) pRel);
	}
	free (pidlParent);
	psfFolder->Release ();
	psfDesktop->Release ();
	return hr;
}


LPBYTE ContextMenu::GetPIDLPos (LPCITEMIDLIST pidl, int nPos)
{
	if (!pidl)
		return 0;
	int nCount = 0;
	
	BYTE * pCur = (BYTE *) pidl;
	while (((LPCITEMIDLIST) pCur)->mkid.cb)
	{
		if (nCount == nPos)
			return pCur;
		nCount++;
		pCur += ((LPCITEMIDLIST) pCur)->mkid.cb;	// + sizeof(pidl->mkid.cb);
	}
	if (nCount == nPos) 
		return pCur;
	return NULL;
}


int ContextMenu::GetPIDLCount (LPCITEMIDLIST pidl)
{
	if (!pidl)
		return 0;

	int nCount = 0;
	BYTE*  pCur = (BYTE *) pidl;
	while (((LPCITEMIDLIST) pCur)->mkid.cb)
	{
		nCount++;
		pCur += ((LPCITEMIDLIST) pCur)->mkid.cb;
	}
	return nCount;
}


/*********************************************************************************************
 *	Notepad specific functions
 */
void ContextMenu::Rename(void)
{
	NewDlg				dlg;
	extern	HANDLE		g_hModule;
	char*				newFirstElement	= (char*)new char[MAX_PATH];
	char*				szNewName		= (char*)new char[MAX_PATH];

	/* copy current element information */
	strcpy(newFirstElement, _strFirstElement.c_str());

	/* when it is folder, remove the last backslash */
	if (newFirstElement[strlen(newFirstElement) - 1] == '\\')
	{
		newFirstElement[strlen(newFirstElement) - 1] = 0;
	}

	/* init field to current selected item */
	strcpy(szNewName, &strrchr(newFirstElement, '\\')[1]);

	(strrchr(newFirstElement, '\\')[1]) = 0;

	dlg.init((HINSTANCE)g_hModule, _hWndNpp);
	if (dlg.doDialog(szNewName, "Rename") == TRUE)
	{
		strcat(newFirstElement, szNewName);
		::MoveFile(_strFirstElement.c_str(), newFirstElement);
	}
	delete [] newFirstElement;
	delete [] szNewName;
}

void ContextMenu::newFile(void)
{
	NewDlg		dlg;
	extern		HANDLE			g_hModule;
	BOOL		bLeave		= FALSE;
	LPTSTR		szFileName	= (LPTSTR)new TCHAR[MAX_PATH];

	szFileName[0] = '\0';

	dlg.init((HINSTANCE)g_hModule, _hWndNpp);
	while (bLeave == FALSE)
	{
		if (dlg.doDialog(szFileName, "New file") == TRUE)
		{
			/* test if is correct */
			if (IsValidFileName(szFileName))
			{
				string		newFile	= _strFirstElement + szFileName;
				
				::CloseHandle(::CreateFile(newFile.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));
				::SendMessage(_hWndNpp, WM_DOOPEN, 0, (LPARAM)newFile.c_str());
				bLeave = TRUE;
			}
		}
		else
			bLeave = TRUE;
	}
	delete [] szFileName;
}

void ContextMenu::newFolder(void)
{
	NewDlg		dlg;
	extern		HANDLE			g_hModule;
	BOOL		bLeave			= FALSE;
	LPTSTR		szFolderName	= (LPTSTR)new TCHAR[MAX_PATH];

	szFolderName[0] = '\0';

	dlg.init((HINSTANCE)g_hModule, _hWndNpp);
	while (bLeave == FALSE)
	{
		if (dlg.doDialog(szFolderName, "New folder") == TRUE)
		{
			/* test if is correct */
			if (IsValidFileName(szFolderName))
			{
				string		newFolder = _strFirstElement + szFolderName;
				if (::CreateDirectory(newFolder.c_str(), NULL) == FALSE)
				{
					::MessageBox(_hWndNpp, "Folder couldn't be created.", "Error", MB_OK);
				}
				bLeave = TRUE;
			}
		}
		else
			bLeave = TRUE;
	}
	delete [] szFolderName;
}

void ContextMenu::findInFiles(void)
{
	::SendMessage(_hWndNpp, NPPM_LAUNCH_FINDINFILESDLG, (WPARAM)_strFirstElement.c_str(), NULL);
}

void ContextMenu::openFile(void)
{
	for (UINT i = 0; i < _strArray.size(); i++)
	{
		::SendMessage(_hWndNpp, WM_DOOPEN, 0, (LPARAM)_strArray[i].c_str());
	}
}

void ContextMenu::openFileInOtherView(void)
{
	for (UINT i = 0; i < _strArray.size(); i++)
	{
		::SendMessage(_hWndNpp, WM_DOOPEN, 0, (LPARAM)_strArray[i].c_str());
		if (i == 0)
		{
			::SendMessage(_hWndNpp, WM_COMMAND, IDC_DOC_GOTO_ANOTHER_VIEW, 0);
		}
	}
}

void ContextMenu::openFileInNewInstance(void)
{
	string	args2Exec;
	extern	HANDLE		g_hModule;
	char				szNpp[MAX_PATH];

    // get path name
	GetModuleFileName((HMODULE)g_hModule, szNpp, sizeof(szNpp));

    // remove the module name : get plugins directory path
	PathRemoveFileSpec(szNpp);
	PathRemoveFileSpec(szNpp);

	/* add notepad as default program */
	strcat(szNpp, "\\");
	strcat(szNpp, "notepad++.exe");

	for (UINT i = 0; i < _strArray.size(); i++)
	{
		if (i == 0)
			args2Exec = "-multiInst " + _strArray[i];
		else
			args2Exec = _strArray[i];

		::ShellExecute(_hWndNpp, "open", szNpp, args2Exec.c_str(), ".", SW_SHOW);
	}
}

void ContextMenu::addToFaves(bool isFolder)
{
	/* test if only one file is selected */
	if (_strArray.size() > 1)
	{
		::MessageBox(_hWndNpp, "Only one file could be added!", "Error", MB_OK);
	}
	else
	{
		extern FavesDialog	favesDlg;
		favesDlg.AddToFavorties((WPARAM)isFolder, (char*)_strArray[0].c_str());
	}
}

void ContextMenu::addFullPaths(void)
{
	ScintillaMsg(SCI_BEGINUNDOACTION);
	for (UINT i = 0; i < _strArray.size(); i++)
	{
		string temp = (i > 0 ? "\n" : "") + _strArray[i];
		ScintillaMsg(SCI_REPLACESEL, 0, (LPARAM)temp.c_str());
	}
	ScintillaMsg(SCI_ENDUNDOACTION);
	::SetFocus(_hWndNpp);
}

void ContextMenu::addFileNames(void)
{
	ScintillaMsg(SCI_BEGINUNDOACTION);
	for (UINT i = 0; i < _strArray.size(); i++)
	{
		UINT	pos		= _strArray[i].rfind("\\", _strArray[i].size()-1);

		if (pos != -1)
		{
			_strArray[i].erase(0, pos + 1);
			string	temp = (i > 0 ? "\n" : "") + _strArray[i];
			ScintillaMsg(SCI_REPLACESEL, 0, (LPARAM)temp.c_str());
		}
	}
	ScintillaMsg(SCI_ENDUNDOACTION);
	::SetFocus(_hWndNpp);
}

