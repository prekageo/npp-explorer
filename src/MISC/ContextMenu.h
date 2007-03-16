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



#ifndef	CONTEXTMENU_DEFINE_H
#define CONTEXTMENU_DEFINE_H

#include "PluginInterface.h"
#include "ExplorerResource.h"
#include "NewDlg.h"
#include "window.h"
#include <malloc.h>
#include <vector>
#include <string>
#include <commctrl.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shellapi.h>

using namespace std;


struct __declspec(uuid("000214e6-0000-0000-c000-000000000046")) IShellFolder;

typedef struct
{
	TCHAR * pszFullPath;
	TCHAR	szFileName[MAX_PATH];
	TCHAR	szTypeName[MAX_PATH];
	UINT64	u64FileSize;
	DWORD	dwFileAttributes;
	int		iIcon;
	FILETIME ftLastModified;
} OBJECT_DATA;


class ContextMenu  
{
public:
	ContextMenu();
	~ContextMenu();

	/* get menu */
	HMENU	GetMenu (void);

	void SetObjects(string strObject);
	void SetObjects(vector<string> strArray);
	UINT ShowContextMenu(HWND hWndNpp, HWND hWndParent, POINT pt, bool normal = true);

private:
	static LRESULT CALLBACK HookWndProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	HRESULT SHBindToParentEx (LPCITEMIDLIST pidl, REFIID riid, VOID **ppv, LPCITEMIDLIST *ppidlLast);

	void	InvokeCommand(LPCONTEXTMENU pContextMenu, UINT idCommand);
	BOOL	GetContextMenu(void ** ppContextMenu, int & iMenuType);
	void	FreePIDLArray (LPITEMIDLIST * pidlArray);
	UINT	GetPIDLSize (LPCITEMIDLIST pidl);
	LPBYTE	GetPIDLPos (LPCITEMIDLIST pidl, int nPos);
	int		GetPIDLCount (LPCITEMIDLIST pidl);
	LPITEMIDLIST CopyPIDL (LPCITEMIDLIST pidl, int cb = -1);

	/* notepad functions */
	void	Rename(void);
	void	newFile(void);
	void	newFolder(void);
	void	findInFiles(void);
	void	openFile(void);
	void	openFileInOtherView(void);
	void	openFileInNewInstance(void);
	void	addToFaves(bool isFolder);
	void	addFullPaths(void);
	void	addFileNames(void);

private:
	HWND					_hWndNpp;
	HWND					_hWndParent;

	int						_nItems;
	BOOL					_bDelete;
	HMENU					_hMenu;
	IShellFolder*			_psfFolder;
	LPITEMIDLIST*			_pidlArray;	

	string					_strFirstElement;
	vector<string>			_strArray;
};

#endif // CONTEXTMENU_DEFINE_H
