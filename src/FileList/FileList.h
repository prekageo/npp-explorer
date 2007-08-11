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


static class FileList*	lpFileListClass	= NULL;


/* pattern for column resize by mouse */
static const WORD DotPattern[] = 
{
	0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF
};

typedef struct {
	BOOL		bParent;
	BOOL		bHidden;
	string		strName;
	string		strExt;
	string		strSize;
	string		strDate;
	/* not visible, only for sorting */
	string		strNameExt;
	__int64		i64Size;
	__int64		i64Date;
	/* not visible, remember state */
	UINT		state;
} tFileListData;


class FileList : public Window
{
public:
	FileList(void);
	~FileList(void);
	
	void init(HINSTANCE hInst, HWND hParent, HWND hParentList);
	void initProp(tExProp* prop);

	void viewPath(LPCSTR currentPath, BOOL redraw = FALSE);

	BOOL notify(WPARAM wParam, LPARAM lParam);

	void filterFiles(LPSTR currentFilter);
	void SelectCurFile(void);
	void SelectFolder(LPSTR selFolder);

	void UpdateDocs(const char** pFiles, UINT numFiles, INT openDoc) {
		_iOpenDoc = openDoc;
		_vStrCurrentFiles.clear();
		for (UINT i = 0; i < numFiles; i++)
			_vStrCurrentFiles.push_back(pFiles[i]);
	};

	virtual void destroy() {};
	virtual void redraw(void) {
		::SendMessage(_hSelf, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)GetSmallImageList(_pExProp->bUseSystemIcons));
		SetColumns();
		Window::redraw();
	};

	void ToggleStackRec(void);					// enables/disable trace of viewed directories
	void ResetDirStack(void);					// resets the stack
	void SetToolBarInfo(ToolBar *ToolBar, UINT idRedo, UINT idUndo);	// set dependency to a toolbar element
	bool GetPrevDir(LPSTR pszPath);				// get previous directory
	bool GetNextDir(LPSTR pszPath);				// get next directory
	INT  GetPrevDirs(LPSTR *pszPathes);			// get previous directorys
	INT  GetNextDirs(LPSTR *pszPathes);			// get next directorys
	void OffsetItr(INT offsetItr);

protected:

	/* Subclassing list control */
	LRESULT runListProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK wndDefaultListProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
		return (lpFileListClass->runListProc(hwnd, Message, wParam, lParam));
	};

	/* Subclassing header control */
	LRESULT runHeaderProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK wndDefaultHeaderProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
		return (lpFileListClass->runHeaderProc(hwnd, Message, wParam, lParam));
	};

	void ReadIconToList(INT iItem, LPINT iIcon, LPINT iOverlayed, LPBOOL pbHidden);
	void ReadArrayToList(LPSTR szItem, INT iItem ,INT iSubItem);

	void DrawDivider(UINT x);
	void UpdateList(void);
	void SetColumns(void);
	void SetOrder(void);

	BOOL FindNextItemInList(UINT maxFolder, UINT maxData, LPUINT puPos);

	void QuickSortRecursiveCol(vector<tFileListData>* vList, INT d, INT h, INT column, BOOL bAscending);
	void QuickSortRecursiveColEx(vector<tFileListData>* vList, INT d, INT h, INT column, BOOL bAscending);

	void onRMouseBtn();
	void onLMouseBtnDbl();

	void PushDir(LPCSTR str);
	void UpdateToolBarElements(void);

	void SetFocusItem(INT item) {
		/* select first entry */
		INT	dataSize	= _vFolders.size() + _vFiles.size();

		/* at first unselect all */
		for (INT iItem = 0; iItem < dataSize; iItem++)
			ListView_SetItemState(_hSelf, iItem, 0, 0xFF);

		ListView_SetItemState(_hSelf, item, LVIS_SELECTED|LVIS_FOCUSED, 0xFF);
		ListView_EnsureVisible(_hSelf, item, TRUE);
		ListView_SetSelectionMark(_hSelf, item);
	};

	void GetSize(__int64 size, string & str);
	void GetDate(FILETIME ftLastWriteTime, string & str);

private:
	HWND						_hHeader;
	WNDPROC						_hDefaultListProc;
	WNDPROC						_hDefaultHeaderProc;

	tExProp*					_pExProp;

	/* file list owner drawn */
	RECT						_rcClip;
	HFONT						_hFont;
	HFONT						_hFontUnder;
	HIMAGELIST					_hImlParent;

	/* header values */
	HBITMAP						_bmpSortUp;
	HBITMAP						_bmpSortDown;
	INT							_iMouseTrackItem;
	LONG						_lMouseTrackPos;
	INT							_iBltPos;

	/* tooltip values */
	ToolTip						_pToolTip;
	UINT						_iItem;
	UINT						_iSubItem;

	/* current file filter */
	TCHAR						_szFileFilter[MAX_PATH];

	/* stores the path here for sorting		*/
	/* Note: _vFolder will not be sorted    */
	UINT						_uMaxFolders;
	UINT						_uMaxElements;
	vector<tFileListData>		_vFolders;
	vector<tFileListData>		_vFiles;

	/* current open docs */
	string						_strCurrentPath;
	INT							_iOpenDoc;
	vector<string>				_vStrCurrentFiles;

	/* search in list by typing of characters */
	BOOL						_bSearchFile;
	char						_strSearchFile[MAX_PATH];

	BOOL						_bOldAddExtToName;
	BOOL						_bOldViewLong;

	/* stack for prev and next dir */
	BOOL						_isStackRec;
	vector<string>				_vDirStack;
	vector<string>::iterator	_itrPos;
    
	ToolBar*					_pToolBar;
	UINT						_idRedo;
	UINT						_idUndo;
};


#endif	//	FILELIST_DEFINE_H