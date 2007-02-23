/*
this file is part of Explorer Plugin for Notepad++
Copyright (C)2006 Jens Lorenz <jens.plugin.npp@gmx.de>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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
				char*	szText	= (char*)new char[MAX_PATH];

				getText(szText);
				addText(szText);
				::SendMessage(::GetParent(_hCombo), EXM_CHANGECOMBO, 0, 0);

				delete [] szText;

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


void ComboOrgi::addText(char* szText)
{
	/* find item */
	int		count		= _comboItems.size();
	int		i			= 0;
	int		hasFoundOn	= -1;

	for (; i < count; i++)
	{
		if (strcmp(szText, _comboItems[i].c_str()) == 0)
		{
			hasFoundOn = count - i - 1;
		}
	}

	/* item missed create new and select it correct */
	if (hasFoundOn == -1)
	{
		_comboItems.push_back(szText);

		::SendMessage(_hCombo, CB_RESETCONTENT, 0, 0);
		for (i = count; i >= 0 ; --i)
		{
			::SendMessage(_hCombo, CB_ADDSTRING, MAX_PATH, (LPARAM)_comboItems[i].c_str());
		}
	}
	selectComboText(szText);
}


void ComboOrgi::setText(char* szText)
{
	::SendMessage(_hCombo, WM_SETTEXT, MAX_PATH, (LPARAM)szText);
}

void ComboOrgi::getText(char* szText)
{
	::SendMessage(_hCombo, WM_GETTEXT, MAX_PATH, (LPARAM)szText);
}

bool ComboOrgi::getSelText(char* szText)
{
	int		curSel = ::SendMessage(_hCombo, CB_GETCURSEL, 0, 0);

	if (curSel != CB_ERR)
	{
		if (MAX_PATH > ::SendMessage(_hCombo, CB_GETLBTEXTLEN, curSel, 0))
		{
			::SendMessage(_hCombo, CB_GETLBTEXT, curSel, (LPARAM)szText);
			return true;
		}
	}
	return false;
}

void ComboOrgi::selectComboText(char* szText)
{
	LRESULT			lResult	= -1;

	lResult = ::SendMessage(_hCombo, CB_FINDSTRINGEXACT, -1, (LPARAM)szText);
	::SendMessage(_hCombo, CB_SETCURSEL, lResult, 0);
}


