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


#ifndef FAVESDLG_DEFINE_H
#define FAVESDLG_DEFINE_H

#include "DockingDlgInterface.h"
#include "TreeHelperClass.h"
#include "FileList.h"
#include "ComboOrgi.h"
#include "Toolbar.h"
#include "PropDlg.h"
#include <string>
#include <vector>
#include <algorithm>
#include <shlwapi.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include "PluginInterface.h"
#include "ExplorerResource.h"

using namespace std;


static char FAVES_DATA[] = "\\Favorites.dat";




class FavesDialog : public DockingDlgInterface, public TreeHelper
{
public:
	FavesDialog(void);
	~FavesDialog(void);

    void init(HINSTANCE hInst, NppData nppData, char* pCurrentPath);

	void destroy(void)
	{
	};

   	void doDialog(bool willBeShown = true);

	void AddToFavorties(BOOL isFolder, char* szLink);
	void SaveSession(void);
	void NotifyNewFile(void);

protected:

	virtual BOOL CALLBACK run_dlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	void GetNameStrFromCmd(UINT idButton, char** tip);
	void tb_cmd(UINT message);

	void InitialDialog(void);

	HTREEITEM GetTreeItem(const char* pszGroupName);
	ELEM_ITR GetElementItr(const char* pszGroupName);
	void CopyItem(HTREEITEM hItem);
	void CutItem(HTREEITEM hItem);
	void PasteItem(HTREEITEM hItem);

	void AddSaveSession(HTREEITEM hItem, BOOL bSave);

	void NewItem(HTREEITEM hItem);
	void EditItem(HTREEITEM hItem);
	void DeleteItem(HTREEITEM hItem);

	void DeleteRecursive(ELEM_ITR pElement);

	void OpenContext(HTREEITEM hItem, POINT pt);
	BOOL DoesNameNotExist(HTREEITEM hItem, char* name);
	BOOL DoesLinkExist(char* link, int root);
	void OpenLink(ELEM_ITR elem_itr);
	void UpdateLink(HTREEITEM hItem);

	void SortElementList(vector<tItemElement>* parentElement);
	void SortElementsRecursive(vector<tItemElement>* parentElement, int d, int h);

	void DrawSessionChildren(HTREEITEM hItem);

	void ReadSettings(void);
	void ReadElementTreeRecursive(ELEM_ITR elem_itr, char** ptr);

	void SaveSettings(void);
	void SaveElementTreeRecursive(ELEM_ITR elem_itr, HANDLE hFile);

	void ErrorMessage(DWORD err);

	eLinkDlg MapPropDlg(int root) {
		switch (root) {
			case FAVES_FOLDERS:		return LINK_DLG_FOLDER;
			case FAVES_FILES:		return LINK_DLG_FILE;
			case FAVES_SESSIONS:	return LINK_DLG_FILE;
			default: return LINK_DLG_NONE;
		}
	};


public:
	void GetFolderPathName(HTREEITEM currentItem, char* folderPathName) {};

private:
	/* Handles */
	NppData					_nppData;
	tTbData					_data;

	HIMAGELIST				_hImageListSmall;
	INT						_iUserImagePos;

	BOOL					_isCut;
	HTREEITEM				_hTreeCutCopy;
	
	BOOL					_isSelNotifyEnable;
	
	char*					_pCurrentElement;

	ToolBar					_ToolBar;
	ReBar					_Rebar;

	ELEM_ITR				_eiOpenLink;

	/* database */
	vector<tItemElement>	_vDB;
};




#endif // FAVESDLG_DEFINE_H