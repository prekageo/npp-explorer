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

#ifndef FILELIST_DEFINE_H
#define FILELIST_DEFINE_H

#include "PluginInterface.h"
#include "ExplorerResource.h"
#include "ToolBar.h"
#include "ToolTip.h"
#include "window.h"
#include <vector>
#include <string>
#include <commctrl.h>
#include <shlwapi.h>

using namespace std;


typedef struct {
	INT		iIcon;
	INT		iOverlayed;
	string	strName;
	string	strExt;
	/* not visible, only for sorting */
	string	strNameLC;
	string	strExtLC;
} tFileListData;


class FileList : public Window
{
public:
	FileList(void);
	~FileList(void);
	
	void init(HINSTANCE hInst, HWND hParent, HWND hParentList, HIMAGELIST hImageList, INT* piColumnPos);

	void viewPath(const char* currentPath, BOOL redraw = FALSE);

	void notify(WPARAM wParam, LPARAM lParam);

	void filterFiles(char* currentFilter);
	void SelectFolder(char* selFolder);

	void ToggleStackRec(void);			// enables/disable trace of viewed directories
	void ResetDirStack(void);			// resets the stack
	void SetToolBarInfo(ToolBar *ToolBar, UINT idRedo, UINT idUndo);	// set dependency to a toolbar element
	bool GetPrevDir(char* pszPath);		// get previous directory
	bool GetNextDir(char* pszPath);		// get next directory

	virtual void destroy() {};

protected:

	/* Subclassing list control */
	LRESULT runListProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK wndDefaultListProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
		return (((FileList *)(::GetWindowLong(hwnd, GWL_USERDATA)))->runListProc(hwnd, Message, wParam, lParam));
	};

	/* Subclassing header control */
	LRESULT runHeaderProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK wndDefaultHeaderProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
		return (((FileList *)(::GetWindowLong(hwnd, GWL_USERDATA)))->runHeaderProc(hwnd, Message, wParam, lParam));
	};

	void drawHeaderItem(DRAWITEMSTRUCT *pDrawItemStruct);

	void ReadIconToList(int iItem, int* iIcon, int* iOverlayed);
	void ReadArrayToList(char* szItem, int iItem ,int iSubItem);

	void UpdateList(void);

	void QuickSortRecursiveCol(vector<tFileListData>* vList, int d, int h, int column, BOOL bAscending);
	void QuickSortRecursiveColEx(vector<tFileListData>* vList, int d, int h, int column, BOOL bAscending);
	string makeStrLC(char* str);

	void onRMouseBtn();
	void onLMouseBtnDbl();

	void PushDir(const char* str);
	void UpdateToolBarElements(void);

	void SetFocusItem(int item) {
		/* select first entry */
		int	dataSize	= _vFolders.size() + _vFiles.size();

		/* at first unselect all */
		for (int iItem = 0; iItem < dataSize; iItem++)
			ListView_SetItemState(_hSelf, iItem, 0, 0xFF);

		ListView_SetItemState(_hSelf, item, LVIS_SELECTED|LVIS_FOCUSED, 0xFF);
		ListView_EnsureVisible(_hSelf, item, TRUE);
		ListView_SetSelectionMark(_hSelf, item);
	};

private:
	HWND						_hHeader;
	WNDPROC						_hDefaultListProc;
	WNDPROC						_hDefaultHeaderProc;

	INT*						_piColumnPos;

	/* header control */
	BOOL						_isMouseOver;
	BOOL						_isLeftButtonDown;
	BOOL						_isUp;
	int							_iPos;
	int							_iMouseOver;

	UINT						_iImageList;

	ToolTip						_pToolTip;
	UINT						_iItem;
	UINT						_iSubItem;

	/* stores the path here for sorting		*/
	/* Note: _vFolder will not be sorted    */
	vector<tFileListData>		_vFolders;
	vector<tFileListData>		_vFiles;

	string						_strCurrentPath;
	char						_szFileFilter[MAX_PATH];

	HBITMAP						_bitmap0;
	HBITMAP						_bitmap1;
	HBITMAP						_bitmap2;
	HBITMAP						_bitmap3;
	HBITMAP						_bitmap4;
	HBITMAP						_bitmap5;
	HBITMAP						_bitmap6;
	HBITMAP						_bitmap7;
	HBITMAP						_bitmap8;
	HBITMAP						_bitmap9;

	/* stack for prev and next dir */
	BOOL						_isStackRec;
	vector<string>				_vDirStack;
	vector<string>::iterator	_itrPos;
    
	ToolBar*					_pToolBar;
	UINT						_idRedo;
	UINT						_idUndo;
};


#endif	//	FILELIST_DEFINE_H