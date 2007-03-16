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


#include "TreeHelperClass.h"
#include "PluginInterface.h"


void TreeHelper::DrawChildren(HTREEITEM parentItem)
{
	WIN32_FIND_DATA		Find	= {0};
	HANDLE				hFind	= NULL;

	vector<tItemList>	vFolderList;
	tItemList			listElement;

	LPTSTR	parentFolderPathName = (LPTSTR) new char[MAX_PATH];

	GetFolderPathName(parentItem, parentFolderPathName);

	if (parentFolderPathName[strlen(parentFolderPathName) - 1] != '\\')
	{
		strcat(parentFolderPathName, "\\");
	}

	/* add wildcard */
	strcat(parentFolderPathName, "*");

	/* find first file */
	hFind = ::FindFirstFile(parentFolderPathName, &Find);
	delete [] parentFolderPathName;

	/* if not found -> exit */
	if (hFind == INVALID_HANDLE_VALUE)
		return;

	do
	{
		if (IsValidFolder(Find) == TRUE)
		{
			listElement.strName	= Find.cFileName;
			vFolderList.push_back(listElement);
		}
	} while (FindNextFile(hFind, &Find));

	::FindClose(hFind);

	/* sort data */
	QuickSortItems(&vFolderList, 0, vFolderList.size()-1);

	for (size_t i = 0; i < vFolderList.size(); i++)
	{
		InsertChildFolder((LPTSTR)vFolderList[i].strName.c_str(), parentItem);
	}
}

void TreeHelper::QuickSortItems(vector<tItemList>* vList, INT d, INT h)
{
	INT		i		= 0;
	INT		j		= 0;
	string	str		= "";

	/* return on empty list */
	if (d > h || d < 0)
		return;

	i = h;
	j = d;

	str = (*vList)[((INT) ((d+h) / 2))].strName;
	do
	{
		while (stricmp((*vList)[j].strName.c_str(), str.c_str()) < 0) j++;
		while (stricmp((*vList)[i].strName.c_str(), str.c_str()) > 0) i--;

		if ( i >= j )
		{
			if ( i != j )
			{
				tItemList buf = (*vList)[i];
				(*vList)[i] = (*vList)[j];
				(*vList)[j] = buf;
			}
			i--;
			j++;
		}
	} while (j <= i);

	if (d < i) QuickSortItems(vList, d, i);
	if (j < h) QuickSortItems(vList, j, h);
}

HTREEITEM TreeHelper::InsertChildFolder(LPTSTR childFolderName, HTREEITEM parentItem, HTREEITEM insertAfter, BOOL bChildrenTest)
{
	/* We search if it already exists */
	LPTSTR				TEMP			= (LPTSTR)new char[MAX_PATH];
	HTREEITEM			pCurrentItem	= TreeView_GetNextItem(_hTreeCtrl, parentItem, TVGN_CHILD);
	BOOL				bHidden			= FALSE;
	WIN32_FIND_DATA		Find			= {0};
	HANDLE				hFind			= NULL;

	while (pCurrentItem != NULL)
	{
		GetItemText(pCurrentItem, TEMP, MAX_PATH);

		if (strcmp(childFolderName, TEMP) == 0)
		{
			delete [] TEMP;
			return pCurrentItem;
		}
	
		pCurrentItem = TreeView_GetNextItem(_hTreeCtrl, pCurrentItem, TVGN_NEXT);
	}

	pCurrentItem = NULL;

	/* get name of parent path and merge it */
	char parentFolderPathName[MAX_PATH]	= "\0";
	GetFolderPathName(parentItem, parentFolderPathName);
	strcat(parentFolderPathName, childFolderName);

	if (parentItem == TVI_ROOT)
	{
		parentFolderPathName[2] = '\0';
	}
	else
	{
		/* get only hidden icon when folder is not a device */
		hFind = ::FindFirstFile(parentFolderPathName, &Find);
		bHidden = ((Find.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0);
		::FindClose(hFind);
	}

	/* look if children test id allowed */
	BOOL	haveChildren = FALSE;
	if (bChildrenTest == TRUE)
	{
		haveChildren = HaveChildren(parentFolderPathName);
	}

	/* insert item */
	INT					iIconNormal		= 0;
	INT					iIconSelected	= 0;
	INT					iIconOverlayed	= 0;

	/* get icons */
	ExtractIcons(parentFolderPathName, NULL, true, &iIconNormal, &iIconSelected, &iIconOverlayed);

	/* set item */
	pCurrentItem = InsertItem(childFolderName, iIconNormal, iIconSelected, iIconOverlayed, bHidden, parentItem, insertAfter, haveChildren);

	delete [] TEMP;

	return pCurrentItem;
}

HTREEITEM TreeHelper::InsertItem(LPTSTR lpszItem, 
								 INT nImage, 
								 INT nSelectedIamage, 
								 INT nOverlayedImage,
								 BOOL bHidden,
								 HTREEITEM hParent, 
								 HTREEITEM hInsertAfter, 
								 BOOL haveChildren, 
								 LPARAM lParam)
{
	TV_INSERTSTRUCT tvis;

	ZeroMemory(&tvis, sizeof(TV_INSERTSTRUCT));
	tvis.hParent			 = hParent;
	tvis.hInsertAfter		 = hInsertAfter;
	tvis.item.mask			 = TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_CHILDREN;
	tvis.item.pszText		 = lpszItem;
	tvis.item.iImage		 = nImage;
	tvis.item.iSelectedImage = nSelectedIamage;
	tvis.item.cChildren		 = haveChildren;
	tvis.item.lParam		 = lParam;

	if (nOverlayedImage != 0)
	{
		tvis.item.mask		|= TVIF_STATE;
		tvis.item.state		|= INDEXTOOVERLAYMASK(nOverlayedImage);
		tvis.item.stateMask	|= TVIS_OVERLAYMASK;
	}
	if (bHidden == TRUE)
	{
		tvis.item.mask		|= LVIF_STATE;
		tvis.item.state		|= LVIS_CUT;
		tvis.item.stateMask |= LVIS_CUT;
	}

	return TreeView_InsertItem(_hTreeCtrl, &tvis);
}

void TreeHelper::DeleteChildren(HTREEITEM parentItem)
{
	HTREEITEM	pCurrentItem = TreeView_GetNextItem(_hTreeCtrl, parentItem, TVGN_CHILD);

	while (pCurrentItem != NULL)
	{
		TreeView_DeleteItem(_hTreeCtrl, pCurrentItem);
		pCurrentItem = TreeView_GetNextItem(_hTreeCtrl, parentItem, TVGN_CHILD);
	}
}

BOOL TreeHelper::UpdateItem(HTREEITEM hItem, 
							LPTSTR lpszItem, 
							INT nImage, 
							INT nSelectedIamage, 
							INT nOverlayedImage, 
							BOOL bHidden,
							BOOL haveChildren, 
							LPARAM lParam)
{
	TVITEM	item;

	ZeroMemory(&item, sizeof(TVITEM));
	item.hItem			 = hItem;
	item.mask			 = TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_CHILDREN | TVIF_STATE;
	item.pszText		 = lpszItem;
	item.iImage			 = nImage;
	item.iSelectedImage	 = nSelectedIamage;
	item.cChildren		 = haveChildren;
	item.state			|= INDEXTOOVERLAYMASK(nOverlayedImage);
	item.stateMask		|= TVIS_OVERLAYMASK;
	item.lParam			 = lParam;

	if (bHidden == TRUE)
	{
		item.mask		|= LVIF_STATE;
		item.state		|= LVIS_CUT;
		item.stateMask  |= LVIS_CUT;
	}

	/* delete children items when available but not needed */
	if ((haveChildren == FALSE) && (TreeView_GetChild(_hTreeCtrl, hItem) != NULL))
	{
		DeleteChildren(hItem);
	}

	return TreeView_SetItem(_hTreeCtrl, &item);
}


BOOL TreeHelper::GetItemText(HTREEITEM hItem, LPTSTR szBuf, INT bufSize)
{
	TVITEM			tvi;
	tvi.mask		= TVIF_TEXT;
	tvi.hItem		= hItem;
	tvi.pszText		= szBuf;
	tvi.cchTextMax	= bufSize;
	
	return TreeView_GetItem(_hTreeCtrl, &tvi);
}

LPARAM TreeHelper::GetParam(HTREEITEM hItem)
{
	TVITEM			tvi;
	tvi.mask		= TVIF_PARAM;
	tvi.hItem		= hItem;
	tvi.lParam		= 0;
	
	TreeView_GetItem(_hTreeCtrl, &tvi);

	return tvi.lParam;
}

