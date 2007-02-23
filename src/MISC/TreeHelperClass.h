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



class TreeHelper
{
public:
	TreeHelper() : _hTreeCtrl(NULL) {};

	void DrawChildren(HTREEITEM parentItem);
	HTREEITEM InsertChildFolder(char* childFolderName, HTREEITEM parentItem, HTREEITEM insertAfter = TVI_LAST, BOOL bChildrenTest = TRUE);
	HTREEITEM InsertItem(char* lpszItem, int nImage, int nSelectedIamage, int nOverlayedImage, HTREEITEM hParent, HTREEITEM hInsertAfter = TVI_LAST, BOOL haveChildren = FALSE, LPARAM lParam = NULL);
	BOOL UpdateItem(HTREEITEM hItem, char* lpszItem, int nImage, int nSelectedIamage, int nOverlayedImage, BOOL haveChildren = FALSE, LPARAM lParam = NULL);
	void DeleteChildren(HTREEITEM parentItem);
	BOOL GetItemText(HTREEITEM hItem, char* szBuf, int bufSize);
	LPARAM GetParam(HTREEITEM hItem);

protected:
	virtual void GetFolderPathName(HTREEITEM currentItem, char* folderPathName) = 0;

protected:
	HWND			_hTreeCtrl;
};

#endif // TREEHELPERCLASS_H