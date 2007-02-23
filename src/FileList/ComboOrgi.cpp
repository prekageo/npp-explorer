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

#include "SysMsg.h"
#include "ExplorerResource.h"
#include "ComboOrgi.h"



ComboOrgi::ComboOrgi()
{
	_comboItems.clear();
}

ComboOrgi::~ComboOrgi()
{
}

void ComboOrgi::init(HWND hCombo)
{
	_hCombo	= hCombo;

	/* subclass combo to get edit messages */
	COMBOBOXINFO	comboBoxInfo;
	comboBoxInfo.cbSize = sizeof(COMBOBOXINFO);

	::SendMessage(_hCombo, CB_GETCOMBOBOXINFO, 0, (LPARAM)&comboBoxInfo);
	::SetWindowLong(comboBoxInfo.hwndItem, GWL_USERDATA, reinterpret_cast<LONG>(this));
	_hDefaultComboProc = reinterpret_cast<WNDPROC>(::SetWindowLong(comboBoxInfo.hwndItem, GWL_WNDPROC, reinterpret_cast<LONG>(wndDefaultProc)));
}


LRESULT ComboOrgi::runProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_KEYUP:
		{
			if (wParam == 13)
			{
				LPSTR	pszText	= (LPSTR)new TCHAR[MAX_PATH];

				getText(pszText);
				addText(pszText);
				::SendMessage(::GetParent(_hCombo), EXM_CHANGECOMBO, 0, 0);

				delete [] pszText;

				return TRUE;
			}
			break;
		}
		case WM_DESTROY:
		{
			_comboItems.clear();
			break;
		}
		default :
			break;
	}
	return ::CallWindowProc(_hDefaultComboProc, hwnd, Message, wParam, lParam);
}


void ComboOrgi::addText(LPSTR pszText)
{
	/* find item */
	INT		count		= _comboItems.size();
	INT		i			= 0;
	INT		hasFoundOn	= -1;

	for (; i < count; i++)
	{
		if (strcmp(pszText, _comboItems[i].c_str()) == 0)
		{
			hasFoundOn = count - i - 1;
		}
	}

	/* item missed create new and select it correct */
	if (hasFoundOn == -1)
	{
		_comboItems.push_back(pszText);

		::SendMessage(_hCombo, CB_RESETCONTENT, 0, 0);
		for (i = count; i >= 0 ; --i)
		{
			::SendMessage(_hCombo, CB_ADDSTRING, MAX_PATH, (LPARAM)_comboItems[i].c_str());
		}
	}
	selectComboText(pszText);
}


void ComboOrgi::setText(LPSTR pszText)
{
	::SendMessage(_hCombo, WM_SETTEXT, MAX_PATH, (LPARAM)pszText);
}

void ComboOrgi::getText(LPSTR pszText)
{
	::SendMessage(_hCombo, WM_GETTEXT, MAX_PATH, (LPARAM)pszText);
}

bool ComboOrgi::getSelText(LPSTR pszText)
{
	INT		curSel = ::SendMessage(_hCombo, CB_GETCURSEL, 0, 0);

	if (curSel != CB_ERR)
	{
		if (MAX_PATH > ::SendMessage(_hCombo, CB_GETLBTEXTLEN, curSel, 0))
		{
			::SendMessage(_hCombo, CB_GETLBTEXT, curSel, (LPARAM)pszText);
			return true;
		}
	}
	return false;
}

void ComboOrgi::selectComboText(LPSTR pszText)
{
	LRESULT			lResult	= -1;

	lResult = ::SendMessage(_hCombo, CB_FINDSTRINGEXACT, -1, (LPARAM)pszText);
	::SendMessage(_hCombo, CB_SETCURSEL, lResult, 0);
}

void ComboOrgi::setComboList(vector<string> vStrList)
{
	INT		iCnt	= vStrList.size();

	::SendMessage(_hCombo, CB_RESETCONTENT, 0, 0);
	for (UINT i = 0; i < iCnt; i++)
	{
		addText((LPSTR)vStrList[i].c_str());
	}
}

void ComboOrgi::getComboList(vector<string> & vStrList)
{
	LPSTR	pszTemp	= (LPSTR)new TCHAR[MAX_PATH];
	INT		iCnt	= ::SendMessage(_hCombo, CB_GETCOUNT, 0, 0);

	vStrList.clear();

	for (UINT i = 0; i < iCnt; i++)
	{
		if (MAX_PATH > ::SendMessage(_hCombo, CB_GETLBTEXTLEN, i, 0))
		{
			::SendMessage(_hCombo, CB_GETLBTEXT, i, (LPARAM)pszTemp);
			vStrList.push_back(pszTemp);
		}
	}
	delete [] pszTemp;
}


