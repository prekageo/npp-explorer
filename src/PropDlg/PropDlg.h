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


#ifndef PROP_DLG_DEFINE_H
#define PROP_DLG_DEFINE_H

#include <windows.h>
#include <commctrl.h>
#include "StaticDialog.h"
#include "PluginInterface.h"
#include "FileDlg.h"
#include "ExplorerResource.h"
#include "TreeHelperClass.h"
#include <vector>

using namespace std;


typedef enum {
	LINK_DLG_NONE,
	LINK_DLG_FOLDER,
	LINK_DLG_FILE
} eLinkDlg;



class PropDlg : public StaticDialog, public TreeHelper
{

public:
	PropDlg(void);
    
    void init(HINSTANCE hInst, HWND hWnd) {
		Window::init(hInst, hWnd);
	};

   	UINT doDialog(char* pName, char* pLink, char* pDesc, eLinkDlg linkDlg = LINK_DLG_NONE, BOOL fileMustExist = FALSE);

    virtual void destroy() {};

	void setTreeElements(ELEM_ITR itr, HIMAGELIST hImageList, INT iUserImagePos, BOOL bWithLink = FALSE);
	const char* getGroupName(void);

protected :
	BOOL CALLBACK run_dlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);

	void DrawChildrenOfItem(HTREEITEM hParentItem);

public:
	void GetFolderPathName(HTREEITEM hItem, char* name);

private:
	char*			_pName;
	char*			_pLink;
	char*			_pDesc;
	eLinkDlg		_linkDlg;
	BOOL			_fileMustExist;
	BOOL			_bWithLink;
	BOOL			_seeDetails;
	ELEM_ITR		_itr;
	ELEM_ITR		_elem_ret;
	HIMAGELIST		_hImageList;
	INT				_iUImgPos;
	string			_strGroupName;
};



#endif // PROP_DLG_DEFINE_H
