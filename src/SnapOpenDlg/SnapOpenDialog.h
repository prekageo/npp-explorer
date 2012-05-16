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


#ifndef SNAP_OPEN_DIALOG_H
#define SNAP_OPEN_DIALOG_H

#include "StaticDialog.h"
#include "Explorer.h"
#include "ExplorerResource.h"



class SnapOpenDlg : public StaticDialog
{

public:
	SnapOpenDlg() : StaticDialog() {};
	
	void init(HINSTANCE hInst, NppData nppData)
	{
		_nppData = nppData;
		Window::init(hInst, nppData._nppHandle);
	};

		UINT doDialog();

	virtual void destroy() {};


protected :
	BOOL CALLBACK run_dlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

private:
	/* Handles */
	NppData			_nppData;
	HWND			_HSource;
	vector<wstring> files;
	WNDPROC wpOrigEditProc;
	int rootPathLen;

	void findFiles();
	void findFilesRecursively(LPCTSTR lpFolder);
	void populateResultList();
	static LRESULT APIENTRY editSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT APIENTRY run_editSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	wstring getRoot();
};



#endif // SNAP_OPEN_DIALOG_H
