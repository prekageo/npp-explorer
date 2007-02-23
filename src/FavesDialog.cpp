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

#include "PluginInterface.h"
#include "FavesDialog.h"
#include "ExplorerDialog.h"
#include "ExplorerResource.h"
#include "NewDlg.h"
#include "Scintilla.h"
#include "ToolTip.h"
#include "resource.h"
#include <dbt.h>

#include <shellapi.h>
#include <shlwapi.h>
#include <shlobj.h>


static void GetFolderPathName(HTREEITEM currentItem, char* folderPathName);

static ToolBarButtonUnit toolBarIcons[] = {
	
    {IDM_EX_LINK_NEW_FILE,  IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1},
    {IDM_EX_LINK_NEW_FOLDER,IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1},

	//-------------------------------------------------------------------------------------//
	{0,						IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, IDI_SEPARATOR_ICON},
	//-------------------------------------------------------------------------------------//

    {IDM_EX_LINK_NEW,  		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1},
    {IDM_EX_LINK_EDIT,	    IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1},

	//-------------------------------------------------------------------------------------//
	{0,						IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, IDI_SEPARATOR_ICON},
	//-------------------------------------------------------------------------------------//

	{IDM_EX_LINK_DELETE,	IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON,		IDI_SEPARATOR_ICON, -1}
};    
		
static int stdIcons[] = {IDB_EX_LINKNEWFILE, IDB_EX_LINKNEWFOLDER, IDB_EX_LINKNEW, IDB_EX_LINKEDIT, IDB_EX_LINKDELETE};

static char* szToolTip[23] = {
	"Link current file...",
	"Link current folder...",
	"New Link...",
	"Edit Link...",
	"Delete Link"
};

void FavesDialog::GetNameStrFromCmd(UINT idButton, char** tip)
{
	*tip = szToolTip[idButton - IDM_EX_LINK_NEW_FILE];
}





FavesDialog::FavesDialog(void) : DockingDlgInterface(IDD_EXPLORER_DLG)
{
	_hTreeCtrl				= NULL;
	_isSelNotifyEnable		= TRUE;
	_isCut					= FALSE;
	_hTreeCutCopy			= NULL;
}

FavesDialog::~FavesDialog(void)
{
}


void FavesDialog::init(HINSTANCE hInst, NppData nppData, char* pCurrentElement)
{
	_nppData = nppData;
	DockingDlgInterface::init(hInst, nppData._nppHandle);

	_pCurrentElement  = pCurrentElement;

	/* Load Image List */
	SHFILEINFO		sfi	= {0};
	_hImageListSmall = GetSystemImageList(TRUE);

	/* set image list and icon */
	_iUserImagePos = ImageList_AddIcon(_hImageListSmall, ::LoadIcon(_hInst, MAKEINTRESOURCE(IDI_FOLDER)));
	ImageList_AddIcon(_hImageListSmall, ::LoadIcon(_hInst, MAKEINTRESOURCE(IDI_FILE)));
	ImageList_AddIcon(_hImageListSmall, ::LoadIcon(_hInst, MAKEINTRESOURCE(IDI_WEB)));
	ImageList_AddIcon(_hImageListSmall, ::LoadIcon(_hInst, MAKEINTRESOURCE(IDI_SESSION)));
	ImageList_AddIcon(_hImageListSmall, ::LoadIcon(_hInst, MAKEINTRESOURCE(IDI_GROOP)));

	/* init database */
	ReadSettings();
}


void FavesDialog::doDialog(bool willBeShown)
{
    if (!isCreated())
	{
		create(&_data);

		// define the default docking behaviour
		_data.uMask			= DWS_DF_CONT_LEFT | DWS_ICONTAB;
		strcpy(_data.pszName, "Favorites");
		_data.hIconTab		= (HICON)::LoadImage(_hInst, MAKEINTRESOURCE(IDI_HEART), IMAGE_ICON, 0, 0, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);
		_data.pszModuleName	= getPluginFileName();
		_data.dlgID			= DOCKABLE_FAVORTIES_INDEX;
		::SendMessage(_hParent, WM_DMM_REGASDCKDLG, 0, (LPARAM)&_data);

		/* Update "Add current..." icons */
		NotifyNewFile();
	}

    display(willBeShown);
}


void FavesDialog::SaveSession(void)
{
	AddSaveSession(NULL, TRUE);
}


void FavesDialog::NotifyNewFile(void)
{
	if (isCreated())
	{
		char*	TEMP	= (char*)new char[MAX_PATH];

		/* update "new file link" icon */
		::SendMessage(_hParent, WM_GET_FULLCURRENTPATH, 0, (LPARAM)TEMP);
		_ToolBar.enable(IDM_EX_LINK_NEW_FILE, (TEMP[1] == ':'));

		/* update "new folder link" icon */
		::SendMessage(_hParent, WM_GET_CURRENTDIRECTORY, 0, (LPARAM)TEMP);
		_ToolBar.enable(IDM_EX_LINK_NEW_FOLDER, (strlen(TEMP) != 0));

		delete [] TEMP;
	}
}


BOOL CALLBACK FavesDialog::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) 
	{
		case WM_INITDIALOG:
		{
			/* get handle of dialogs */
			_hTreeCtrl		= ::GetDlgItem(_hSelf, IDC_TREE_FOLDER);
			::DestroyWindow(::GetDlgItem(_hSelf, IDC_LIST_FILE));
			::DestroyWindow(::GetDlgItem(_hSelf, IDC_BUTTON_SPLITTER));
			::DestroyWindow(::GetDlgItem(_hSelf, IDC_BUTTON_FILTER));
			::DestroyWindow(::GetDlgItem(_hSelf, IDC_STATIC_FILTER));
			::DestroyWindow(::GetDlgItem(_hSelf, IDC_COMBO_FILTER));

			InitialDialog();
			break;
		}
		case WM_NOTIFY:
		{
			LPNMHDR		nmhdr = (LPNMHDR)lParam;

			if (nmhdr->hwndFrom == _hTreeCtrl)
			{
				switch (nmhdr->code)
				{
					case NM_RCLICK:
					{
						POINT			pt			= {0};
						TVHITTESTINFO	ht			= {0};
						DWORD			dwpos		= ::GetMessagePos();
						HTREEITEM		hItem		= NULL;

						pt.x = GET_X_LPARAM(dwpos);
						pt.y = GET_Y_LPARAM(dwpos);

						ht.pt = pt;
						::ScreenToClient(_hTreeCtrl, &ht.pt);

						hItem = TreeView_HitTest(_hTreeCtrl, &ht);
						if (hItem != NULL)
						{
							OpenContext(hItem, pt);
						}
						break;
					}
					case NM_DBLCLK:
					{
						HTREEITEM	hItem	= TreeView_GetSelection(_hTreeCtrl);

						if (hItem != NULL)
						{
							ELEM_ITR	elem_itr = (ELEM_ITR)GetParam(hItem);

							if (elem_itr != NULL)
							{
								/* open the link */
								_eiOpenLink = elem_itr;
								::SetTimer(_hSelf, EXT_OPENLINK, 50, NULL);
							}
							else
							{
								/* session file will be opened */
								char*	TEMP	= (char*)new char[MAX_PATH];
								
								GetItemText(hItem, TEMP, MAX_PATH);
								::SendMessage(_hParent, WM_DOOPEN, 0, (LPARAM)TEMP);

								delete [] TEMP;
							}
						}
						break;
					}
					case TVN_ITEMEXPANDING:
					{
						POINT			pt			= {0};
						TVHITTESTINFO	ht			= {0};
						DWORD			dwpos		= ::GetMessagePos();
						HTREEITEM		hItem		= NULL;

						pt.x = GET_X_LPARAM(dwpos);
						pt.y = GET_Y_LPARAM(dwpos);

						ht.pt = pt;
						::ScreenToClient(_hTreeCtrl, &ht.pt);

						hItem = TreeView_HitTest(_hTreeCtrl, &ht);

						if (hItem != NULL)
						{
							if (!TreeView_GetChild(_hTreeCtrl, hItem))
							{
								/* get element information */
								ELEM_ITR	elem_itr = (ELEM_ITR)GetParam(hItem);

								if (elem_itr == NULL)
								{
									/* nothing to do */
								}
								else if ((elem_itr->uParam & FAVES_SESSIONS) && (elem_itr->uParam & FAVES_PARAM_LINK))
								{
									DrawSessionChildren(hItem);
								}
								else
								{
									UpdateLink(hItem);
								}
							}
						}
						break;
					}
					case TVN_SELCHANGED:
					{
						HTREEITEM	hItem = TreeView_GetSelection(_hTreeCtrl);

						if (hItem != NULL)
						{
							ELEM_ITR	elem_itr = (ELEM_ITR)GetParam(hItem);

							if (elem_itr != NULL)
							{
								_ToolBar.enable(IDM_EX_LINK_NEW, !(elem_itr->uParam & FAVES_PARAM_LINK));
								_ToolBar.enable(IDM_EX_LINK_EDIT, !(elem_itr->uParam & FAVES_PARAM_MAIN));
								_ToolBar.enable(IDM_EX_LINK_DELETE, !(elem_itr->uParam & FAVES_PARAM_MAIN));
								NotifyNewFile();
							}
							else
							{
								_ToolBar.enable(IDM_EX_LINK_NEW, false);
								_ToolBar.enable(IDM_EX_LINK_EDIT, false);
								_ToolBar.enable(IDM_EX_LINK_DELETE, false);
								NotifyNewFile();
							}
						}						
						break;
					}
					default:
						break;
				}
			}
			else if (nmhdr->code == TTN_GETDISPINFO)
			{
				LPTOOLTIPTEXT lpttt; 

				lpttt = (LPTOOLTIPTEXT)nmhdr; 
				lpttt->hinst = _hInst; 

				// Specify the resource identifier of the descriptive 
				// text for the given button.
				int idButton = int(lpttt->hdr.idFrom);

				char*	tip	= NULL;
				GetNameStrFromCmd(idButton, &tip);
				lpttt->lpszText = tip;
			}

			DockingDlgInterface::run_dlgProc(Message, wParam, lParam);

		    return FALSE;
		}
		case WM_SIZE:
		case WM_MOVE:
		{
			RECT	rc			= {0};

			/* set position of toolbar */
			getClientRect(rc);
			_Rebar.reSizeTo(rc);

			/* set position of tree control */
			rc.top    += 26;
			rc.bottom -= 26;
			::SetWindowPos(_hTreeCtrl, NULL, rc.left, rc.top, rc.right, rc.bottom, SWP_NOZORDER | SWP_SHOWWINDOW);

			break;
		}
		case WM_COMMAND:
		{
			/* Only used on non NT based systems */
			if (LOWORD(wParam) == IDCANCEL)
	        {
				display(false);
			}

			if ((HWND)lParam == _ToolBar.getHSelf())
			{
				tb_cmd(LOWORD(wParam));
			}
			break;
		}
		case WM_DESTROY:
		{
			SaveSettings();
			::DestroyIcon(_data.hIconTab);
			break;
		}
		case WM_TIMER:
		{
			if (wParam == EXT_OPENLINK)
			{
				::KillTimer(_hSelf, EXT_OPENLINK);
				OpenLink(_eiOpenLink);
			}
			break;
		}
		default:
			DockingDlgInterface::run_dlgProc(Message, wParam, lParam);
			break;
	}

	return FALSE;
}

void FavesDialog::tb_cmd(UINT message)
{
	switch (message)
	{
		case IDM_EX_LINK_NEW_FILE:
		{
			char*	TEMP	= (char*)new char[MAX_PATH];

			::SendMessage(_hParent, WM_GET_FULLCURRENTPATH, 0, (LPARAM)TEMP);

			if (TEMP[1] == ':')
			{
				AddToFavorties(FALSE, TEMP);
			}

			delete [] TEMP;
			break;
		}
		case IDM_EX_LINK_NEW_FOLDER:
		{
			char*	TEMP	= (char*)new char[MAX_PATH];

			::SendMessage(_hParent, WM_GET_CURRENTDIRECTORY, 0, (LPARAM)TEMP);

			if (strlen(TEMP) != 0)
			{
				AddToFavorties(TRUE, TEMP);
			}

			delete [] TEMP;
			break;
		}
		case IDM_EX_LINK_NEW:
		{
			HTREEITEM	hItem	= TreeView_GetSelection(_hTreeCtrl);
			UINT		root	= ((ELEM_ITR)GetParam(hItem))->uParam & FAVES_PARAM;

			if (root == FAVES_SESSIONS)
			{
				AddSaveSession(hItem, FALSE);
			}
			else
			{
				NewItem(hItem);
			}
			break;
		}
		case IDM_EX_LINK_EDIT:
		{
			EditItem(TreeView_GetSelection(_hTreeCtrl));
			break;
		}
		case IDM_EX_LINK_DELETE:
		{
			DeleteItem(TreeView_GetSelection(_hTreeCtrl));
			break;
		}
		default:
			break;
	}
}

void FavesDialog::InitialDialog(void)
{
	::SendMessage(_hTreeCtrl, TVM_SETIMAGELIST, TVSIL_NORMAL, (LPARAM)_hImageListSmall);

	/* create toolbar */
	_ToolBar.init(_hInst, _hSelf, 16, toolBarIcons, sizeof(toolBarIcons)/sizeof(ToolBarButtonUnit), true, stdIcons, sizeof(stdIcons)/sizeof(int));
	_ToolBar.display();
	_Rebar.init(_hInst, _hSelf, &_ToolBar);
	_Rebar.display();

	/* add new items in list and make reference to items */
	for (UINT i = 0; i < FAVES_ITEM_MAX; i++)
	{
		UpdateLink(InsertItem(cFavesItemNames[i], _iUserImagePos + i, _iUserImagePos + i, 0, TVI_ROOT, TVI_LAST, TRUE, (LPARAM)(_vDB.begin()+i)));
	}
}


HTREEITEM FavesDialog::GetTreeItem(const char* pszGroopName)
{
	if (isCreated())
	{
		char*		ptr			= NULL;
		char*		TEMP		= (char*)new char[MAX_PATH];
		char*		pszName		= (char*)new char[MAX_PATH];
		HTREEITEM	hItem		= TVI_ROOT;

		/* copy the groop name */
		strcpy(TEMP, pszGroopName);

		ptr = strtok(TEMP, " ");

		/* no need to compare if tree item is NULL, it will never happen */
		while (ptr != NULL)
		{
			/* get child item */
			hItem = TreeView_GetChild(_hTreeCtrl, hItem);
			GetItemText(hItem, pszName, MAX_PATH);

			while (strcmp(ptr, pszName) != 0)
			{
				hItem = TreeView_GetNextItem(_hTreeCtrl, hItem, TVGN_NEXT);
				GetItemText(hItem, pszName, MAX_PATH);
			}
			/* test next element */
			ptr = strtok(NULL, " ");
		}

		delete [] TEMP;
		delete [] pszName;

		return hItem;
	}
	
	return NULL;
}


ELEM_ITR FavesDialog::GetElementItr(const char* pszGroopName)
{
	char*					ptr			= NULL;
	char*					TEMP		= (char*)new char[MAX_PATH];
	ELEM_ITR				elem_itr	= _vDB.begin();
	ELEM_ITR				elem_end	= _vDB.end();

	/* copy the groop name */
	strcpy(TEMP, pszGroopName);

	ptr = strtok(TEMP, " ");

	/* find element */
	while (ptr != NULL)
	{
		for (; elem_itr != elem_end; elem_itr++)
		{
			if (strcmp(elem_itr->pszName, ptr) == 0)
			{
				break;
			}
		}

		/* test next element */
		ptr = strtok(NULL, " ");

		if (ptr != NULL)
		{
			elem_end = elem_itr->vElements.end();
			elem_itr = elem_itr->vElements.begin();
		}
	}

	delete [] TEMP;

	return elem_itr;
}


void FavesDialog::CopyItem(HTREEITEM hItem)
{
	_isCut			= FALSE;
	_hTreeCutCopy	= hItem;
}

void FavesDialog::CutItem(HTREEITEM hItem)
{
	_isCut			= TRUE;
	_hTreeCutCopy	= hItem;
}

void FavesDialog::PasteItem(HTREEITEM hItem)
{
	ELEM_ITR	elem_itr	= (ELEM_ITR)GetParam(hItem);
	ELEM_ITR	elem_cc		= (ELEM_ITR)GetParam(_hTreeCutCopy);

	if ((elem_itr->uParam & FAVES_PARAM) == (elem_cc->uParam & FAVES_PARAM))
	{
		/* add element */
		tItemElement	element;

		if ((_isCut == FALSE) && (TreeView_GetParent(_hTreeCtrl, _hTreeCutCopy) == hItem))
		{
			element.pszName		= (char*)new char[strlen(elem_cc->pszName)+strlen("Copy of ")+1];
			strcpy(element.pszName, "Copy of ");
			strcat(element.pszName, elem_cc->pszName);
		}
		else
		{
			element.pszName		= (char*)new char[strlen(elem_cc->pszName)+1];
			strcpy(element.pszName, elem_cc->pszName);
		}

		if (elem_cc->pszLink != NULL)
		{
			element.pszLink	= (char*)new char[strlen(elem_cc->pszLink)+1];
			strcpy(element.pszLink, elem_cc->pszLink);
		}
		else
		{
			element.pszLink	= NULL;
		}
		element.uParam		= elem_cc->uParam;
		element.vElements	= elem_cc->vElements;

		if (_isCut == TRUE)
		{
			/* delete item */
			HTREEITEM	hItemParent	= TreeView_GetParent(_hTreeCtrl, _hTreeCutCopy);
			ELEM_ITR	elem_parent = (ELEM_ITR)GetParam(hItemParent);

			delete [] elem_cc->pszName;
			delete [] elem_cc->pszLink;
			elem_parent->vElements.erase(elem_cc);

			/* update information and delete element */
			TreeView_DeleteItem(_hTreeCtrl, _hTreeCutCopy);
		}

		elem_itr->vElements.push_back(element);

		/* update information */
		HTREEITEM	hParentItem = TreeView_GetParent(_hTreeCtrl, hItem);
		if (hParentItem != NULL)
		{
			UpdateLink(hParentItem);
		}
		UpdateLink(hItem);
		TreeView_Expand(_hTreeCtrl, hItem, TVM_EXPAND | TVE_COLLAPSERESET);

		_hTreeCutCopy = NULL;
	}
	else
	{
		char	TEMP[128];
		sprintf(TEMP, "Could only be paste into %s", cFavesItemNames[elem_cc->uParam & FAVES_PARAM]);
		::MessageBox(_hSelf, TEMP, "Error", MB_OK);
	}
}

void FavesDialog::AddToFavorties(BOOL isFolder, char* szLink)
{
	PropDlg		dlgProp;
	HTREEITEM	hItem		= NULL;
	BOOL		isOk		= FALSE;
	UINT		root		= (TRUE == isFolder?FAVES_FOLDERS:FAVES_FILES);
	char*		pszName		= (char*)new char[MAX_PATH];
	char*		pszLink		= (char*)new char[MAX_PATH];
	char*		pszDesc		= (char*)new char[MAX_PATH];

	/* fill out params */
	pszName[0] = '\0';
	strcpy(pszLink, szLink);

	/* create description */
	strcpy(pszDesc, "New element in ");
	strcat(pszDesc, cFavesItemNames[root]);

	/* init properties dialog */
	dlgProp.init(_hInst, _hParent);

	/* select root element */
	dlgProp.setTreeElements(&_vDB[root], _hImageListSmall, _iUserImagePos);

	while (isOk == FALSE)
	{
		/* open dialog */
		if (dlgProp.doDialog(pszName, pszLink, pszDesc, MapPropDlg(root)) == TRUE)
		{
			/* remove last backslash */
			if (pszLink[strlen(pszLink)-1] == '\\')
				pszLink[strlen(pszLink)-1] = '\0';

			/* get selected item */
			const char*		szGroopName = dlgProp.getGroopName();
			hItem = GetTreeItem(szGroopName);

			/* test if name not exist and link exist */
			if (DoesNameNotExist(hItem, pszName) == TRUE)
				isOk = DoesLinkExist(pszLink, root);

			if (isOk == TRUE)
			{
				tItemElement	element;
				element.pszName	= (char*)new char[strlen(pszName)+1];
				element.pszLink	= (char*)new char[strlen(pszLink)+1];
				element.uParam	= FAVES_PARAM_LINK | root;
				element.vElements.clear();

				strcpy(element.pszName, pszName);
				strcpy(element.pszLink, pszLink);

				/* push element back */
				ELEM_ITR		elem_itr	= GetElementItr(szGroopName);
				elem_itr->vElements.push_back(element);
			}
		}
		else
		{
			break;
		}
	}

	if ((isOk == TRUE) && (hItem != NULL))
	{
		/* update information */
		HTREEITEM	hParentItem = TreeView_GetParent(_hTreeCtrl, hItem);
		if (hParentItem != NULL)
		{
			UpdateLink(hParentItem);
		}
		UpdateLink(hItem);

		/* expand item */
		TreeView_Expand(_hTreeCtrl, hItem, TVM_EXPAND | TVE_COLLAPSERESET);
	}

	delete [] pszName;
	delete [] pszLink;
	delete [] pszDesc;
}


void FavesDialog::AddSaveSession(HTREEITEM hItem, BOOL bSave)
{
	PropDlg		dlgProp;
	HTREEITEM	hParentItem	= NULL;
	BOOL		isOk		= FALSE;
	ELEM_ITR	elem_itr	= NULL;
	int			root		= 0;
	char*		pszName		= (char*)new char[MAX_PATH];
	char*		pszLink		= (char*)new char[MAX_PATH];
	char*		pszDesc		= (char*)new char[MAX_PATH];

	/* fill out params */
	pszName[0] = '\0';
	pszLink[0] = '\0';

	if (bSave == TRUE)
	{
		strcpy(pszDesc, "Save current Session");
	}
	else
	{
		strcpy(pszDesc, "Add existing Session");
	}

	/* if hItem is empty, extended dialog is necessary */
	if (hItem == NULL)
	{
		/* select root element */
		root	 = FAVES_SESSIONS;
		dlgProp.setTreeElements(&_vDB[root], _hImageListSmall, _iUserImagePos, TRUE);
	}
	else
	{
		hParentItem	= TreeView_GetParent(_hTreeCtrl, hItem);
		elem_itr	= (ELEM_ITR)GetParam(hItem);
		root		= (elem_itr->uParam & FAVES_PARAM);
	}

	/* init properties dialog */
	dlgProp.init(_hInst, _hParent);
	while (isOk == FALSE)
	{
		/* open dialog */
		if (dlgProp.doDialog(pszName, pszLink, pszDesc, MapPropDlg(root), (bSave == TRUE)) == TRUE)
		{
			if (hItem == NULL)
			{
				/* get group name */
				const char*		szGroopName = dlgProp.getGroopName();
				hParentItem = GetTreeItem(szGroopName);

				/* get pointer by name */
				elem_itr = GetElementItr(szGroopName);
			}

			/* test if name not exist and link exist on known hItem */
			if (DoesNameNotExist(hParentItem, pszName) == TRUE)
				isOk = (bSave == TRUE ? TRUE : DoesLinkExist(pszLink, root));

			if (isOk == TRUE)
			{
				/* if the parent element is LINK element -> replace informations */
				if (elem_itr->uParam & FAVES_PARAM_LINK)
				{
					delete [] elem_itr->pszName;
					elem_itr->pszName	= (char*)new char[strlen(pszName)+1];
					strcpy(elem_itr->pszName, pszName);
					delete [] elem_itr->pszLink;
					elem_itr->pszLink	= (char*)new char[strlen(pszLink)+1];
					strcpy(elem_itr->pszLink, pszLink);
				}
				else
				{
					/* push information back */
					tItemElement	element;
					element.pszName	= (char*)new char[strlen(pszName)+1];
					element.pszLink	= (char*)new char[strlen(pszLink)+1];
					element.uParam	= FAVES_PARAM_LINK | root;
					element.vElements.clear();

					strcpy(element.pszName, pszName);
					strcpy(element.pszLink, pszLink);

					elem_itr->vElements.push_back(element);
				}

				/* save current session when expected */
				if (bSave == TRUE)
				{
					::SendMessage(_hParent, WM_SAVECURRENTSESSION, 0, (LPARAM)pszLink);
				}
			}
		}
		else
		{
			break;
		}
	}

	if ((isOk == TRUE) && (hItem != NULL))
	{
		UpdateLink(hParentItem);
		TreeView_Expand(_hTreeCtrl, hParentItem, TVM_EXPAND | TVE_COLLAPSERESET);
	}

	delete [] pszName;
	delete [] pszLink;
	delete [] pszDesc;
}

void FavesDialog::NewItem(HTREEITEM hItem)
{
	PropDlg		dlgProp;
	ELEM_ITR	elem_itr	= (ELEM_ITR)GetParam(hItem);
	int			root		= (elem_itr->uParam & FAVES_PARAM);
	BOOL		isOk		= FALSE;
	char*		pszName		= (char*)new char[MAX_PATH];
	char*		pszLink		= (char*)new char[MAX_PATH];
	char*		pszDesc		= (char*)new char[MAX_PATH];

	/* init link and name */
	pszName[0] = '\0';
	pszLink[0] = '\0';

	/* set description text */
	strcpy(pszDesc, "New element in ");
	strcat(pszDesc, cFavesItemNames[root]);

	/* init properties dialog */
	dlgProp.init(_hInst, _hParent);
	while (isOk == FALSE)
	{
		/* open dialog */
		if (dlgProp.doDialog(pszName, pszLink, pszDesc, MapPropDlg(root)) == TRUE)
		{
			/* test if name not exist and link exist */
			if (DoesNameNotExist(hItem, pszName) == TRUE)
				isOk = DoesLinkExist(pszLink, root);

			if (isOk == TRUE)
			{
				tItemElement	element;
				element.pszName	= (char*)new char[strlen(pszName)+1];
				element.pszLink	= (char*)new char[strlen(pszLink)+1];
				element.uParam	= FAVES_PARAM_LINK | root;
				element.vElements.clear();

				strcpy(element.pszName, pszName);
				strcpy(element.pszLink, pszLink);

				elem_itr->vElements.push_back(element);
			}
		}
		else
		{
			break;
		}
	}

	if (isOk == TRUE)
	{
		/* update information */
		if (elem_itr->uParam & FAVES_PARAM_GROOP)
		{
			UpdateLink(TreeView_GetParent(_hTreeCtrl, hItem));
		}
		UpdateLink(hItem);

		TreeView_Expand(_hTreeCtrl, hItem, TVM_EXPAND | TVE_COLLAPSERESET);
	}


	delete [] pszName;
	delete [] pszLink;
	delete [] pszDesc;
}

void FavesDialog::EditItem(HTREEITEM hItem)
{
	HTREEITEM	hParentItem = TreeView_GetParent(_hTreeCtrl, hItem);
	ELEM_ITR	elem_itr	= (ELEM_ITR)GetParam(hItem);

	if (!(elem_itr->uParam & FAVES_PARAM_MAIN))
	{
		int			root		= (elem_itr->uParam & FAVES_PARAM);
		BOOL		isOk		= FALSE;
		char*		pszName		= (char*)new char[MAX_PATH];
		char*		pszLink		= (char*)new char[MAX_PATH];
		char*		pszDesc		= (char*)new char[MAX_PATH];

		if (elem_itr->uParam & FAVES_PARAM_GROOP)
		{
			/* get data of current selected element */
			strcpy(pszName, elem_itr->pszName);
			strcpy(pszDesc, "Properties of groop");

			/* init new dialog */
			NewDlg		dlgNew;
			dlgNew.init(_hInst, _hParent, "Favorites");

			/* open dialog */
			while (isOk == FALSE)
			{
				if (dlgNew.doDialog(pszName, pszDesc) == TRUE)
				{
					/* test if name not exist */
					isOk = DoesNameNotExist(hParentItem, pszName);

					if (isOk == TRUE)
					{
						delete [] elem_itr->pszName;
						elem_itr->pszName	= (char*)new char[strlen(pszName)+1];
						strcpy(elem_itr->pszName, pszName);
					}
				}
				else
				{
					break;
				}
			}
		}
		else if (elem_itr->uParam & FAVES_PARAM_LINK)
		{
			/* get data of current selected element */
			strcpy(pszName, elem_itr->pszName);
			strcpy(pszLink, elem_itr->pszLink);
			strcpy(pszDesc, "Properties of");

			PropDlg		dlgProp;
			dlgProp.init(_hInst, _hParent);
			while (isOk == FALSE)
			{
				if (dlgProp.doDialog(pszName, pszLink, pszDesc, MapPropDlg(root)) == TRUE)
				{
					/* test if name not exist and link exist */
					if (DoesNameNotExist(hParentItem, pszName) == TRUE)
						isOk = DoesLinkExist(pszLink, root);

					if (isOk == TRUE)
					{
						delete [] elem_itr->pszName;
						elem_itr->pszName	= (char*)new char[strlen(pszName)+1];
						strcpy(elem_itr->pszName, pszName);
						delete [] elem_itr->pszLink;
						elem_itr->pszLink	= (char*)new char[strlen(pszLink)+1];
						strcpy(elem_itr->pszLink, pszLink);
					}
				}
				else
				{
					break;
				}
			}
		}

		/* update text of item */
		if (isOk == TRUE)
		{
			UpdateLink(hParentItem);
		}

		delete [] pszName;
		delete [] pszLink;
		delete [] pszDesc;
	}
}

void FavesDialog::DeleteItem(HTREEITEM hItem)
{
	HTREEITEM	hItemParent	= TreeView_GetParent(_hTreeCtrl, hItem);
	ELEM_ITR	elem_itr	= (ELEM_ITR)GetParam(hItem);

	if (!(elem_itr->uParam & FAVES_PARAM_MAIN))
	{
		/* delete child elements */
		DeleteRecursive(elem_itr);
		((ELEM_ITR)GetParam(hItemParent))->vElements.erase(elem_itr);

		/* update information and delete element */
		TreeView_DeleteItem(_hTreeCtrl, hItem);

		/* update only parent of parent when current item is a groop folder */
		if (((ELEM_ITR)GetParam(hItemParent))->uParam & FAVES_PARAM_GROOP)
		{
			UpdateLink(TreeView_GetParent(_hTreeCtrl, hItemParent));
		}
		UpdateLink(hItemParent);
	}
}

void FavesDialog::DeleteRecursive(ELEM_ITR element)
{
	/* delete name and link of this element */
	delete [] element->pszName;
	delete [] element->pszLink;

	/* delete elements of child items */
	for (ELEM_ITR itr = element->vElements.begin(); itr != element->vElements.end(); itr++)
	{
		DeleteRecursive(itr);
	}
	element->vElements.clear();
}


void FavesDialog::OpenContext(HTREEITEM hItem, POINT pt)
{
	NewDlg			dlgNew;
	HMENU			hMenu		= NULL;
	int				rootLevel	= 0;
	ELEM_ITR		elem_itr	= NULL;

	/* get element and level depth */
	elem_itr	= (ELEM_ITR)GetParam(hItem);

	if (elem_itr != NULL)
	{
		int		root	= (elem_itr->uParam & FAVES_PARAM);

		if (elem_itr->uParam & (FAVES_PARAM_MAIN | FAVES_PARAM_GROOP))
		{
			/* create menu and attach one element */
			hMenu = ::CreatePopupMenu();

			if (root != FAVES_SESSIONS)
			{
				::AppendMenu(hMenu, MF_STRING, 1, "New Link...");
				::AppendMenu(hMenu, MF_STRING, 2, "New Groop...");
			}
			else
			{
				::AppendMenu(hMenu, MF_STRING, 3, "Add existing Session...");
				::AppendMenu(hMenu, MF_STRING, 4, "Save current Session...");
				::AppendMenu(hMenu, MF_STRING, 2, "New Groop...");
			}

			if (elem_itr->uParam & FAVES_PARAM_GROOP)
			{
				::AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
				::AppendMenu(hMenu, MF_STRING, 5, "Copy");
				::AppendMenu(hMenu, MF_STRING, 6, "Cut");
				if (_hTreeCutCopy != NULL) ::AppendMenu(hMenu, MF_STRING, 7, "Paste");
				::AppendMenu(hMenu, MF_STRING, 8, "Delete");
				::AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
				::AppendMenu(hMenu, MF_STRING, 9, "Properties...");
			}
			else if ((elem_itr->uParam & FAVES_PARAM_MAIN) && (_hTreeCutCopy != NULL))
			{
				::AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
				::AppendMenu(hMenu, MF_STRING, 7, "Paste");
			}
			
			/* track menu */
			switch (::TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, _hParent, NULL))
			{
				case 1:
				{
					NewItem(hItem);
					break;
				}
				case 3:
				{
					AddSaveSession(hItem, FALSE);
					break;
				}
				case 4:
				{
					AddSaveSession(hItem, TRUE);					
					break;
				}
				case 2:
				{
					BOOL		isOk	= FALSE;
					char*		pszName	= (char*)new char[MAX_PATH];
					char*		pszDesc = (char*)new char[MAX_PATH];

					pszName[0] = '\0';

					strcpy(pszDesc, "New groop in ");
					strcat(pszDesc, cFavesItemNames[root]);

					/* init new dialog */
					dlgNew.init(_hInst, _hParent, "Favorites");

					/* open dialog */
					while (isOk == FALSE)
					{
						if (dlgNew.doDialog(pszName, pszDesc) == TRUE)
						{
							/* test if name not exist */
							isOk = DoesNameNotExist(hItem, pszName);

							if (isOk == TRUE)
							{
								tItemElement	element;
								element.pszName	= (char*)new char[strlen(pszName)+1];
								element.pszLink	= NULL;
								element.uParam	= FAVES_PARAM_GROOP | root;
								element.vElements.clear();

								strcpy(element.pszName, pszName);

								elem_itr->vElements.push_back(element);

								/* update information */
								if (elem_itr->uParam & FAVES_PARAM_GROOP)
								{
									UpdateLink(TreeView_GetParent(_hTreeCtrl, hItem));
								}
								UpdateLink(hItem);
								TreeView_Expand(_hTreeCtrl, hItem, TVM_EXPAND | TVE_COLLAPSERESET);
							}
						}
					}

					delete [] pszName;
					delete [] pszDesc;
					break;
				}
				case 5:
				{
					CopyItem(hItem);
					break;
				}
				case 6:
				{
					CutItem(hItem);
					break;
				}
				case 7:
				{
					PasteItem(hItem);
					break;
				}
				case 8:
				{
					DeleteItem(hItem);
					break;
				}
				case 9:
				{
					EditItem(hItem);
					break;
				}
			}

			/* free resources */
			::DestroyMenu(hMenu);
		}
		else if (elem_itr->uParam & FAVES_PARAM_LINK)
		{
			/* create menu and attach one element */
			hMenu = ::CreatePopupMenu();

			::AppendMenu(hMenu, MF_STRING, 1, "Open");

			if (root == FAVES_FILES)
			{
				::AppendMenu(hMenu, MF_STRING, 2, "Open in other view");
				::AppendMenu(hMenu, MF_STRING, 3, "Open in new instance");
			}
			else if (root == FAVES_SESSIONS)
			{
				::AppendMenu(hMenu, MF_STRING, 4, "Reopen");
			}

			::AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
			::AppendMenu(hMenu, MF_STRING, 5, "Copy");
			::AppendMenu(hMenu, MF_STRING, 6, "Cut");
			if (_hTreeCutCopy != NULL) ::AppendMenu(hMenu, MF_STRING, 7, "Paste");
			::AppendMenu(hMenu, MF_STRING, 8, "Delete");
			::AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
			::AppendMenu(hMenu, MF_STRING, 9, "Properties...");

			/* track menu */
			switch (::TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, _hParent, NULL))
			{
				case 1:
				{
					OpenLink(elem_itr);
					break;
				}
				case 2:
				{
					::SendMessage(_hParent, WM_DOOPEN, 0, (LPARAM)elem_itr->pszLink);
					::SendMessage(_hParent, WM_COMMAND, IDC_DOC_GOTO_ANOTHER_VIEW, 0);
					break;
				}
				case 3:
				{
					extern	HANDLE		g_hModule;
					char*				pszNpp		= (char*)new char[MAX_PATH];
					char*				pszParams	= (char*)new char[MAX_PATH];

					// get path name
					GetModuleFileName((HMODULE)g_hModule, pszNpp, MAX_PATH);

					// remove the module name : get plugins directory path
					PathRemoveFileSpec(pszNpp);
					PathRemoveFileSpec(pszNpp);

					/* add notepad as default program */
					strcat(pszNpp, "\\");
					strcat(pszNpp, "notepad++.exe");

					sprintf(pszParams, "-multiInst %s", elem_itr->pszLink);
					::ShellExecute(_hParent, "open", pszNpp, pszParams, ".", SW_SHOW);

					delete [] pszNpp;
					delete [] pszParams;
					break;
				}
				case 4:
				{
					/* close files previously */
					::SendMessage(_hParent, WM_COMMAND, IDM_FILE_CLOSEALL, 0);
					OpenLink(elem_itr);
					break;
				}
				case 5:
				{
					CopyItem(hItem);
					break;
				}
				case 6:
				{
					CutItem(hItem);
					break;
				}
				case 7:
				{
					PasteItem(hItem);
					break;
				}
				case 8:
				{
					DeleteItem(hItem);
					break;
				}
				case 9:
				{
					EditItem(hItem);
					break;
				}
				default:
					break;
			}
			/* free resources */
			::DestroyMenu(hMenu);
		}
		else
		{
			::MessageBox(_hSelf, "Element not found in List!", "Error", MB_OK);
		}
	}
}


void FavesDialog::UpdateLink(HTREEITEM	hParentItem)
{
	BOOL		haveChildren	= FALSE;
	int			iIconNormal		= 0;
	int			iIconSelected	= 0;
	int			iIconOverlayed	= 0;
	int			root			= 0;
	char*		TEMP			= (char*)new char[MAX_PATH];
	HTREEITEM	pCurrentItem	= TreeView_GetNextItem(_hTreeCtrl, hParentItem, TVGN_CHILD);

	/* get element list */
	ELEM_ITR	parentElement	= ((ELEM_ITR)GetParam(hParentItem));

	if (parentElement != NULL)
	{
		/* sort list */
		SortElementList(&parentElement->vElements);

		/* update elements in parent tree */
		for (ELEM_ITR itr = parentElement->vElements.begin(); itr != parentElement->vElements.end(); itr++)
		{
			/* get root */
			root = itr->uParam & FAVES_PARAM;

			/* initialize children */
			haveChildren		= FALSE;

			if (itr->uParam & FAVES_PARAM_GROOP)
			{
				iIconNormal		= _iUserImagePos + 4;
				iIconSelected	= iIconNormal;
				iIconOverlayed	= 0;
				if (itr->vElements.size() != 0)
				{
					haveChildren = TRUE;
				}
			}
			else
			{
				/* get icons */
				switch (root)
				{
					case FAVES_FOLDERS:
					{
						/* get icons and update item */
						strcpy(TEMP, itr->pszLink);
						ExtractIcons(TEMP, NULL, true, &iIconNormal, &iIconSelected, &iIconOverlayed);
						iIconSelected = iIconNormal;
						break;
					}
					case FAVES_FILES:
					{
						/* get icons and update item */
						strcpy(TEMP, itr->pszLink);
						ExtractIcons(TEMP, NULL, true, &iIconNormal, &iIconSelected, &iIconOverlayed);
						break;
					}
					case FAVES_SESSIONS:
						haveChildren	= (0 != ::SendMessage(_hParent, WM_NBSESSIONFILES, 0, (LPARAM)itr->pszLink));
					case FAVES_WEB:
					{
						iIconNormal		= _iUserImagePos + root;
						iIconSelected	= _iUserImagePos + root;
						iIconOverlayed	= 0;
						break;
					}
				}
			}

			/* update or add new item */
			if (pCurrentItem != NULL)
			{
				GetItemText(pCurrentItem, TEMP, MAX_PATH);
				if (itr == (ELEM_ITR)GetParam(pCurrentItem))
				{
					UpdateItem(pCurrentItem, itr->pszName, iIconNormal, iIconSelected, iIconOverlayed, haveChildren, (LPARAM)itr);
				}
				else
				{
					HTREEITEM	hPrevItem = TreeView_GetNextItem(_hTreeCtrl, pCurrentItem, TVGN_PREVIOUS);
					if (hPrevItem != NULL)
						pCurrentItem = InsertItem(itr->pszName, iIconNormal, iIconSelected, iIconOverlayed, hParentItem, hPrevItem, haveChildren, (LPARAM)itr);
					else
						pCurrentItem = InsertItem(itr->pszName, iIconNormal, iIconSelected, iIconOverlayed, hParentItem, TVI_FIRST, haveChildren, (LPARAM)itr);
				}
			}
			else
			{
				pCurrentItem = InsertItem(itr->pszName, iIconNormal, iIconSelected, iIconOverlayed, hParentItem, TVI_LAST, haveChildren, (LPARAM)itr);
			}
			pCurrentItem = TreeView_GetNextItem(_hTreeCtrl, pCurrentItem, TVGN_NEXT);
		}

		/* delete possible not existed items */
		while (pCurrentItem != NULL)
		{
			HTREEITEM	pPrevItem	= pCurrentItem;
			pCurrentItem			= TreeView_GetNextItem(_hTreeCtrl, pCurrentItem, TVGN_NEXT);
			TreeView_DeleteItem(_hTreeCtrl, pPrevItem);
		}
	}
	delete [] TEMP;
}


void FavesDialog::DrawSessionChildren(HTREEITEM hItem)
{
	int			i				= 0;
	int			docCnt			= 0;
	char		**fileNames		= NULL;
	ELEM_ITR	elem_itr		= (ELEM_ITR)GetParam(hItem);
	
	/* get document count and create resources */
	docCnt		= (int)::SendMessage(_hParent, WM_NBSESSIONFILES, 0, (LPARAM)elem_itr->pszLink);
	fileNames	= (char **)new char*[docCnt];

	for (i = 0; i < docCnt; i++)
		fileNames[i] = new char[MAX_PATH];

	/* get file names */
	if (::SendMessage(_hParent, WM_GETSESSIONFILES, (WPARAM)fileNames, (LPARAM)elem_itr->pszLink) == TRUE)
	{
		/* when successfull add it to the tree */
		int			iIconNormal		= 0;
		int			iIconSelected	= 0;
		int			iIconOverlayed	= 0;
		char*		TEMP			= (char*)new char[MAX_PATH];

		for (i = 0; i < docCnt; i++)
		{
			strcpy(TEMP, elem_itr->pszLink);
			ExtractIcons(TEMP, NULL, true, &iIconNormal, &iIconSelected, &iIconOverlayed);
			InsertItem(fileNames[i], iIconNormal, iIconSelected, iIconOverlayed, hItem);
		}

		delete [] TEMP;
	}

	/* delete resources */
	for (i = 0; i < docCnt; i++)
		delete [] fileNames[i];
	delete [] fileNames;
}


BOOL FavesDialog::DoesNameNotExist(HTREEITEM hParentItem, char* pszName)
{
	BOOL		bRet	= TRUE;
	char*		TEMP	= (char*)new char[MAX_PATH];
	HTREEITEM	hItem	= TreeView_GetNextItem(_hTreeCtrl, hParentItem, TVGN_CHILD);

	while (hItem != NULL)
	{
		GetItemText(hItem, TEMP, MAX_PATH);

		if (strcmp(pszName, TEMP) == 0)
		{
			::MessageBox(_hSelf, "Name exist in node!", "Error", MB_OK);
			bRet = FALSE;
			break;
		}
	
		hItem = TreeView_GetNextItem(_hTreeCtrl, hItem, TVGN_NEXT);
	}

	return bRet;	
}

BOOL FavesDialog::DoesLinkExist(char* pszLink, int root)
{
	BOOL	bRet = FALSE;

	switch (root)
	{
		case FAVES_FOLDERS:
		{
			/* test if folder could be opened */
			WIN32_FIND_DATA		Find	= {0};
			HANDLE				hFind	= ::FindFirstFile(pszLink, &Find);

			if (hFind != INVALID_HANDLE_VALUE)
			{
				::FindClose(hFind);
				bRet = TRUE;
			}
			else
			{
				::MessageBox(_hSelf, "Folder dosn't exist!", "Error", MB_OK);
			}
			break;
		}
		case FAVES_FILES:
		case FAVES_SESSIONS:
		{
			/* test if file could be opened */
			FILE*	hFile = fopen(pszLink, "r");

			if (hFile != NULL)
			{
				fclose(hFile);
				bRet = TRUE;
			}
			else
			{
				::MessageBox(_hSelf, "File dosn't exist!", "Error", MB_OK);
			}
			break;
		}
		case FAVES_WEB:
		{
			bRet = TRUE;
			break;
		}
		default:
			::MessageBox(_hSelf, "Faves element does not exist", "Error", MB_OK);
			break;
	}

	return bRet;
}


void FavesDialog::OpenLink(ELEM_ITR elem_itr)
{
	if (elem_itr->uParam & FAVES_PARAM_LINK)
	{
		switch (elem_itr->uParam & FAVES_PARAM)
		{
			case FAVES_FOLDERS:
			{
				extern ExplorerDialog		explorerDlg;

				if (explorerDlg.getHSelf() == NULL)
					explorerDlg.doDialog();

				::SendMessage(explorerDlg.getHSelf(), EXM_OPENDIR, 0, (LPARAM)elem_itr->pszLink);
				::SendMessage(_hParent, WM_DMM_VIEWOTHERTAB, 0, (LPARAM)"Explorer");
				::SetFocus(explorerDlg.getHSelf());
				break;
			}
			case FAVES_FILES:
			{
				::SendMessage(_hParent, WM_DOOPEN, 0, (LPARAM)elem_itr->pszLink);
				break;
			}
			case FAVES_WEB:
			{
				::ShellExecute(_hParent, "open", elem_itr->pszLink, NULL, NULL, SW_SHOW);
				break;
			}
			case FAVES_SESSIONS:
			{
				::SendMessage(_hParent, WM_LOADSESSION, 0, (LPARAM)elem_itr->pszLink);
				break;
			}
			default:
				break;
		}
	}
}

void FavesDialog::SortElementList(vector<tItemElement>* elementList)
{
	vector<tItemElement>		sortList;
	vector<tItemElement>		lastList;

	/* get all last elements in a seperate list and remove from sort list */
	for (vector<tItemElement>::iterator itr = elementList->begin(); itr != elementList->end(); itr++)
	{
		if (itr->uParam & FAVES_PARAM_GROOP)
		{
			sortList.push_back(*itr);
		}
		else if (itr->uParam & FAVES_PARAM_LINK)
		{
			lastList.push_back(*itr);
		}
	}

	/* get size of both lists */
	UINT	sizeOfSort	= sortList.size();
	UINT	sizeOfLast	= lastList.size();

	/* sort "sort list" */
	SortElementsRecursive(&sortList, 0, sizeOfSort-1);

	/* sort "last list" */
	SortElementsRecursive(&lastList, 0, sizeOfLast-1);

	/* delete element list and attach the information of both the temp lists */
	elementList->clear();
	for (UINT i = 0; i < sizeOfSort; i++)
	{
		elementList->push_back(sortList[i]);
	}
	for (i = 0; i < sizeOfLast; i++)
	{
		elementList->push_back(lastList[i]);
	}
}

void FavesDialog::SortElementsRecursive(vector<tItemElement>* vElement, int d, int h)
{
	int		i	= 0;
	int		j	= 0;
	string	str = "";

	/* return on empty list */
	if (d > h || d < 0)
		return;

	i = h;
	j = d;

	str = (*vElement)[((int) ((d+h) / 2))].pszName;
	do
	{
		/* sort in alphabeticaly order */
		while ((*vElement)[j].pszName < str) j++;
		while ((*vElement)[i].pszName > str) i--;

		if ( i >= j )
		{
			if ( i != j )
			{
				tItemElement buf = (*vElement)[i];
				(*vElement)[i] = (*vElement)[j];
				(*vElement)[j] = buf;
			}
			i--;
			j++;
		}
	} while (j <= i);

	if (d < i) SortElementsRecursive(vElement,d,i);
	if (j < h) SortElementsRecursive(vElement,j,h);
}


void FavesDialog::ReadSettings(void)
{
	tItemElement	list;
	extern char		configPath[MAX_PATH];
	char*			readFilePath			= (char*)new char[MAX_PATH];
	DWORD			hasRead					= 0;
	HANDLE			hFile					= NULL;

	/* create root data */
	for (int i = 0; i < FAVES_ITEM_MAX; i++)
	{
		/* create element list */
		list.uParam		= FAVES_PARAM_MAIN | i;
		list.pszName	= (char*)new char[strlen(cFavesItemNames[i])+1];
		list.pszLink	= NULL;
		list.vElements.clear();

		strcpy(list.pszName, cFavesItemNames[i]);

		_vDB.push_back(list);
	}

	/* fill out tree and vDB */
	strcpy(readFilePath, configPath);
	strcat(readFilePath, FAVES_DATA);

	hFile = ::CreateFile(readFilePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD	size = ::GetFileSize(hFile, NULL);

		if (size != -1)
		{
			char*			ptr		= NULL;
			char*			data	= (char*)new char[size];

			/* read data from file */
			::ReadFile(hFile, data, size, &hasRead, NULL);

			/* get first element */
			ptr = strtok(data, "\n");

			/* finaly, fill out the tree and the vDB */
			for (i = 0; i < FAVES_ITEM_MAX; i++)
			{
				/* error */
				if (ptr == NULL)
					break;

				/* step over name tag */
				if (strcmp(cFavesItemNames[i], ptr) == 0)
				{
					ptr = strtok(NULL, "\n");
				}
				else
				{
					::MessageBox(_hSelf, "Error in file 'Favorites.dat'", "Error", MB_OK);
					break;
				}

				/* now read the information */
				ReadElementTreeRecursive(&_vDB[i], &ptr);
			}

			delete [] data;
		}

		::CloseHandle(hFile);
	}

	delete [] readFilePath;
}


void FavesDialog::ReadElementTreeRecursive(ELEM_ITR elem_itr, char** ptr)
{
	tItemElement	element;
	char*			pszPos			= NULL;
	UINT			root			= elem_itr->uParam & FAVES_PARAM;

	while (1)
	{
		if (*ptr == NULL)
		{
			/* reached end of file -> leave */
			break;
		}
		if (strcmp(*ptr, "#LINK") == 0)
		{
			/* link is found, get information and fill out the struct */

			/* get element name */
			*ptr = strtok(NULL, "\n");
			if (strstr(*ptr, "\tName=") == *ptr)
			{
				element.pszName	= (char*)new char[strlen(*ptr)-5];
				strcpy(element.pszName, &(*ptr)[6]);
			}
			else
				::MessageBox(_hSelf, "Error in file 'Favorites.dat'\nName in LINK not correct!", "Error", MB_OK);

			/* get next element link */
			*ptr = strtok(NULL, "\n");
			if (strstr(*ptr, "\tLink=") == *ptr)
			{
				element.pszLink	= (char*)new char[strlen(*ptr)-5];
				strcpy(element.pszLink, &(*ptr)[6]);
			}
			else
				::MessageBox(_hSelf, "Error in file 'Favorites.dat'\nLink in LINK not correct!", "Error", MB_OK);

			element.uParam	= FAVES_PARAM_LINK | root;
			element.vElements.clear();
			
			elem_itr->vElements.push_back(element);

			*ptr = strtok(NULL, "\n");
		}
		else if (strcmp(*ptr, "#GROOP") == 0)
		{
			/* group is found, get information and fill out the struct */

			/* get element name */
			*ptr = strtok(NULL, "\n");
			if (strstr(*ptr, "\tName=") == *ptr)
			{
				element.pszName	= (char*)new char[strlen(*ptr)-5];
				strcpy(element.pszName, &(*ptr)[6]);
			}
			else
				::MessageBox(_hSelf, "Error in file 'Favorites.dat'\nName in GROOP not correct!", "Error", MB_OK);

			element.uParam	= FAVES_PARAM_GROOP | root;
			element.vElements.clear();

			elem_itr->vElements.push_back(element);

			*ptr = strtok(NULL, "\n");

			ReadElementTreeRecursive(elem_itr->vElements.end()-1, ptr);
		}
		else if (strcmp(*ptr, "") == 0)
		{
			/* step over empty lines */
			*ptr = strtok(NULL, "\n");
		}
		else if (strcmp(*ptr, "#END") == 0)
		{
			/* on group end leave the recursion */
			*ptr = strtok(NULL, "\n");
			break;
		}
		else
		{
			/* there is garbage information/tag */
			break;
		}
	}
}


void FavesDialog::SaveSettings(void)
{
	char			temp[32];
	ELEM_ITR		elem_itr		= NULL;

	extern char		configPath[MAX_PATH];
	char*			saveFilePath	= (char*)new char[MAX_PATH];
	DWORD			hasWritten		= 0;
	HANDLE			hFile			= NULL;

	strcpy(saveFilePath, configPath);
	strcat(saveFilePath, FAVES_DATA);

	hFile = ::CreateFile(saveFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		/* delete allocated resources */
		HTREEITEM	hItem = TreeView_GetNextItem(_hTreeCtrl, TVI_ROOT, TVGN_CHILD);

		for (int i = 0; i < FAVES_ITEM_MAX; i++)
		{
			elem_itr = (ELEM_ITR)GetParam(hItem);

			/* store tree */
			sprintf(temp, "%s\n", cFavesItemNames[i]);
			WriteFile(hFile, temp, strlen(temp), &hasWritten, NULL);
			SaveElementTreeRecursive(elem_itr, hFile);

			/* delete tree */
			DeleteRecursive(elem_itr);

			hItem = TreeView_GetNextItem(_hTreeCtrl, hItem, TVGN_NEXT);
		}

		::CloseHandle(hFile);
	}
	else
	{
		ErrorMessage(GetLastError());
	}

	delete [] saveFilePath;
}


void FavesDialog::SaveElementTreeRecursive(ELEM_ITR elem_itr, HANDLE hFile)
{
	DWORD		hasWritten	= 0;
	UINT		size		= 0;
	char*		temp		= NULL;

	/* delete elements of child items */
	for (ELEM_ITR itr = elem_itr->vElements.begin(); itr != elem_itr->vElements.end(); itr++)
	{
		if (itr->uParam & FAVES_PARAM_GROOP)
		{
			WriteFile(hFile, "#GROOP\n", strlen("#GROOP\n"), &hasWritten, NULL);

			size = strlen(itr->pszName)+8;
			temp = (char*)new char[size];
			sprintf(temp, "\tName=%s\n\n", itr->pszName);
			WriteFile(hFile, temp, size, &hasWritten, NULL);
			delete [] temp;

			SaveElementTreeRecursive(itr, hFile);

			WriteFile(hFile, "#END\n\n", strlen("#END\n\n"), &hasWritten, NULL);
		}
		else if (itr->uParam & FAVES_PARAM_LINK)
		{
			WriteFile(hFile, "#LINK\n", strlen("#LINK\n"), &hasWritten, NULL);

			size = strlen(itr->pszName)+7;
			temp = (char*)new char[size];
			sprintf(temp, "\tName=%s\n", itr->pszName);
			WriteFile(hFile, temp, size, &hasWritten, NULL);
			delete [] temp;

			size = strlen(itr->pszLink)+8;
			temp = (char*)new char[size];
			sprintf(temp, "\tLink=%s\n\n", itr->pszLink);
			WriteFile(hFile, temp, size, &hasWritten, NULL);
			delete [] temp;
		}
	}
}


void FavesDialog::ErrorMessage(DWORD err)
{
	LPVOID	lpMsgBuf;
	///////////////

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
	(LPTSTR) & lpMsgBuf, 0, NULL);	// Process any inserts in lpMsgBuf.

	// ...// Display the string.

	::MessageBox(_hSelf, (LPCTSTR) lpMsgBuf, "Error", MB_OK | MB_ICONINFORMATION);

	// Free the buffer.

	LocalFree(lpMsgBuf);
}


