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


#ifndef TREEHELPERCLASS_H
#define TREEHELPERCLASS_H

#include <windows.h>
#include <commctrl.h>
#include <vector>
#include <string>

using namespace std;

#define ICON_UPDATE_SIZE	100
typedef struct {
	class TreeHelper	*pTree;
	string				strLastPath;
	HTREEITEM			hLastItem;
} tTreeIconUpdate;



#ifndef TreeView_GetItemState
#define TVM_GETITEMSTATE (TV_FIRST+39)
#define TreeView_GetItemState(hwndTV, hti, mask) \
(UINT)::SendMessage((hwndTV), TVM_GETITEMSTATE, (WPARAM)(hti), (LPARAM)(mask))
#endif


class TreeHelper
{
public:
	TreeHelper() : _hTreeCtrl(NULL), _hSemaphore(NULL) {};
	~TreeHelper() {
		if (_hSemaphore) {
			::CloseHandle(_hSemaphore);
			delete [] _ptIconUpdate;
		}
	};

	void UpdateOverlayIcon(tTreeIconUpdate* ptIconUpdate);

protected:

	void UseOverlayThreading(void) {
		_ptIconUpdate = (tTreeIconUpdate*)new tTreeIconUpdate[ICON_UPDATE_SIZE];
		::ZeroMemory(_ptIconUpdate, sizeof(tTreeIconUpdate) * ICON_UPDATE_SIZE);
		_hSemaphore	= ::CreateSemaphore(NULL, 100, 100, NULL);
	};

	virtual void GetFolderPathName(HTREEITEM currentItem, LPTSTR folderPathName) = 0;

	void DrawChildren(HTREEITEM parentItem);
	void UpdateChildren(LPTSTR pszParentPath, HTREEITEM pCurrentItem, BOOL doRecursive = TRUE );
	HTREEITEM InsertChildFolder(LPTSTR childFolderName, HTREEITEM parentItem, HTREEITEM insertAfter = TVI_LAST, BOOL bChildrenTest = TRUE);
	HTREEITEM InsertItem(LPTSTR lpszItem, INT nImage, INT nSelectedIamage, INT nOverlayedImage, BOOL bHidden, HTREEITEM hParent, HTREEITEM hInsertAfter = TVI_LAST, BOOL haveChildren = FALSE, LPARAM lParam = NULL);
	BOOL UpdateItem(HTREEITEM hItem, LPTSTR lpszItem, INT nImage, INT nSelectedIamage, INT nOverlayedImage, BOOL bHidden, BOOL haveChildren = FALSE, LPARAM lParam = NULL, BOOL delChildren = TRUE);
	void DeleteChildren(HTREEITEM parentItem);
	BOOL GetItemText(HTREEITEM hItem, LPTSTR szBuf, INT bufSize);
	LPARAM GetParam(HTREEITEM hItem);
	BOOL GetItemIcons(HTREEITEM hItem, LPINT iIcon, LPINT piSelected, LPINT iOverlay);
	BOOL IsItemExpanded(HTREEITEM hItem);

private:

	typedef struct tItemList {
		string	strName;
		DWORD	dwAttributes;
	};
	void QuickSortItems(vector<tItemList>* vList, INT d, INT h);
	BOOL FindFolderAfter(LPTSTR itemName, HTREEITEM pAfterItem);

private:	/* for thread */

	void SetOverlayIcon(HTREEITEM hItem, INT iOverlayIcon);
	void TREE_LOCK(void) {
		while (_hSemaphore) {
			if (::WaitForSingleObject(_hSemaphore, 50) == WAIT_OBJECT_0)
				return;
		}
	};
	void TREE_UNLOCK(void) {
		if (_hSemaphore) {
			::ReleaseSemaphore(_hSemaphore, 1, NULL);
		}
	};

protected:
	HWND				_hTreeCtrl;

private:
	/* member var for overlay update thread */
	tTreeIconUpdate*	_ptIconUpdate;
	HANDLE				_hSemaphore;
	HANDLE				_hOverThread;
};

#endif // TREEHELPERCLASS_H