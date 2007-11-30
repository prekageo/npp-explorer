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
#include "Explorer.h"


DWORD WINAPI TreeOverlayThread(LPVOID lpParam)
{
	tTreeIconUpdate *ptIconUpdate	= (tTreeIconUpdate*)lpParam;
	TreeHelper		*pTree			= ptIconUpdate->pTree;
	pTree->UpdateOverlayIcon(ptIconUpdate);
	return 0;
}

void TreeHelper::UpdateOverlayIcon(tTreeIconUpdate* ptIconUpdate)
{
	TREE_LOCK();

	/* insert item */
	TCHAR		TEMP[MAX_PATH];
	INT			iIconNormal		= 0;
	INT			iIconSelected	= 0;
	INT			iIconOverlayed	= 0;
	HTREEITEM	hItem			= TreeView_GetNextItem(_hTreeCtrl, ptIconUpdate->hLastItem, TVGN_CHILD);

	while (hItem != NULL)
	{
		/* get foler name */
		GetItemText(hItem, TEMP, MAX_PATH);

		/* get icons */
		ExtractIcons(ptIconUpdate->strLastPath.c_str(), TEMP, DEVT_DIRECTORY, &iIconNormal, &iIconSelected, &iIconOverlayed);

		/* set overlay icon */
		SetOverlayIcon(hItem, iIconOverlayed);

		hItem = TreeView_GetNextItem(_hTreeCtrl, hItem, TVGN_NEXT);
	}
	ptIconUpdate->hLastItem = NULL;

	TREE_UNLOCK();
}

void TreeHelper::DrawChildren(HTREEITEM parentItem)
{
	TCHAR				parentFolderPathName[MAX_PATH];
	size_t				iCnt		= 0;
	UINT				iThreadPos	= 0;
	WIN32_FIND_DATA		Find		= {0};
	HANDLE				hFind		= NULL;
	vector<tItemList>	vFolderList;
	tItemList			listElement;

	GetFolderPathName(parentItem, parentFolderPathName);

	if (parentFolderPathName[strlen(parentFolderPathName) - 1] != '\\')
	{
		strcat(parentFolderPathName, "\\");
	}

	/* add wildcard */
	strcat(parentFolderPathName, "*");

	/* find first file */
	hFind = ::FindFirstFile(parentFolderPathName, &Find);

	/* if not found -> exit */
	if (hFind != INVALID_HANDLE_VALUE)
	{
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
	 
		TREE_LOCK();

		for (iCnt = 0; iCnt < vFolderList.size(); iCnt++)
		{
			InsertChildFolder((LPTSTR)vFolderList[iCnt].strName.c_str(), parentItem);
		}

		/* save data for overlay thread */
		for (; iThreadPos < ICON_UPDATE_SIZE; iThreadPos++) {
			if (_ptIconUpdate[iThreadPos].hLastItem == NULL) {
				_ptIconUpdate[iThreadPos].pTree			= this;
				_ptIconUpdate[iThreadPos].hLastItem		= parentItem;
				_ptIconUpdate[iThreadPos].strLastPath	= parentFolderPathName;
				break;
			}
		}
		TREE_UNLOCK();

		if ((_hSemaphore) && (parentItem != TVI_ROOT)) {
			/* start update overlay icons */
			DWORD	dwFlags	= 0;
			_hOverThread	= ::CreateThread(NULL, 0, TreeOverlayThread, &_ptIconUpdate[iThreadPos], 0, &dwFlags);
		}	
	}
}

void TreeHelper::UpdateChildren(LPTSTR pszParentPath, HTREEITEM hParentItem, BOOL doRecursive)
{
	static
	UINT				iRecCount	= 0;

	size_t				iCnt		= 0;
	UINT				iThreadPos	= 0;
	WIN32_FIND_DATA		Find		= {0};
	HANDLE				hFind		= NULL;
	TVITEM				item		= {0};
	
	vector<tItemList>	vFolderList;
	tItemList			listElement;
	TCHAR				pszItem[MAX_PATH];
	TCHAR				pszPath[MAX_PATH];
	TCHAR				pszSearch[MAX_PATH];
	HTREEITEM			hCurrentItem	= TreeView_GetNextItem(_hTreeCtrl, hParentItem, TVGN_CHILD);

	/* remove possible backslash */
	if (pszParentPath[strlen(pszParentPath)-1] == '\\')
		pszParentPath[strlen(pszParentPath)-1] = '\0';

	/* copy path into search path */
	strcpy(pszSearch, pszParentPath);

	/* add wildcard */
	strcat(pszSearch, "\\*");

	if ((hFind = ::FindFirstFile(pszSearch, &Find)) != INVALID_HANDLE_VALUE)
	{
		if (iRecCount == 0)
			TREE_LOCK();
		iRecCount++;

		/* save data for overlay thread */
		for (; iThreadPos < ICON_UPDATE_SIZE; iThreadPos++) {
			if (_ptIconUpdate[iThreadPos].hLastItem == NULL) {
				_ptIconUpdate[iThreadPos].pTree			= this;
				_ptIconUpdate[iThreadPos].hLastItem		= hParentItem;
				_ptIconUpdate[iThreadPos].strLastPath	= pszParentPath;
				break;
			}
		}

		/* find folders */
		do
		{
			if (IsValidFolder(Find) == TRUE)
			{
				listElement.strName			= Find.cFileName;
				listElement.dwAttributes	= Find.dwFileAttributes;
				vFolderList.push_back(listElement);
			}
		} while (FindNextFile(hFind, &Find));

		::FindClose(hFind);

		/* sort data */
		QuickSortItems(&vFolderList, 0, vFolderList.size()-1);

		/* update tree */
		for (iCnt = 0; iCnt < vFolderList.size(); iCnt++)
		{
			if (GetItemText(hCurrentItem, pszItem, MAX_PATH) == TRUE)
			{
				/* compare current item and the current folder name */
				while ((strcmp(pszItem, vFolderList[iCnt].strName.c_str()) != 0) && (hCurrentItem != NULL))
				{
					HTREEITEM	pPrevItem = NULL;

					/* if it's not equal delete or add new item */
					if (FindFolderAfter((LPTSTR)vFolderList[iCnt].strName.c_str(), hCurrentItem) == TRUE)
					{
						pPrevItem		= hCurrentItem;
						hCurrentItem	= TreeView_GetNextItem(_hTreeCtrl, hCurrentItem, TVGN_NEXT);
						TreeView_DeleteItem(_hTreeCtrl, pPrevItem);
					}
					else
					{
						pPrevItem = TreeView_GetNextItem(_hTreeCtrl, hCurrentItem, TVGN_PREVIOUS);

						/* Note: If hCurrentItem is the first item in the list pPrevItem is NULL */
						if (pPrevItem == NULL)
							hCurrentItem = InsertChildFolder((LPTSTR)vFolderList[iCnt].strName.c_str(), hParentItem, TVI_FIRST);
						else
							hCurrentItem = InsertChildFolder((LPTSTR)vFolderList[iCnt].strName.c_str(), hParentItem, pPrevItem);
					}

					if (hCurrentItem != NULL)
						GetItemText(hCurrentItem, pszItem, MAX_PATH);
				}

				/* get current path */
				sprintf(pszPath, "%s\\%s", pszParentPath, pszItem);

				/* update icons and expandable information */
				INT					iIconNormal		= 0;
				INT					iIconSelected	= 0;
				INT					iIconOverlayed	= 0;
				BOOL				haveChildren	= HaveChildren(pszPath);
				BOOL				bHidden			= FALSE;

				/* correct by HaveChildren() modified pszPath */
				pszPath[strlen(pszPath) - 2] = '\0';

				/* get icons and update item */
				if (_hSemaphore) {
					ExtractIcons(pszPath, NULL, DEVT_DIRECTORY, &iIconNormal, &iIconSelected, NULL);
				} else {
					ExtractIcons(pszPath, NULL, DEVT_DIRECTORY, &iIconNormal, &iIconSelected, &iIconOverlayed);
				}
				bHidden = ((vFolderList[iCnt].dwAttributes & FILE_ATTRIBUTE_HIDDEN) != 0);
				UpdateItem(hCurrentItem, pszItem, iIconNormal, iIconSelected, iIconOverlayed, bHidden, haveChildren);

				/* update recursive */
				if ((doRecursive) && IsItemExpanded(hCurrentItem))
				{
					UpdateChildren(pszPath, hCurrentItem);
				}

				/* select next item */
				hCurrentItem = TreeView_GetNextItem(_hTreeCtrl, hCurrentItem, TVGN_NEXT);
			}
			else
			{
				hCurrentItem = InsertChildFolder((LPTSTR)vFolderList[iCnt].strName.c_str(), hParentItem);
				hCurrentItem = TreeView_GetNextItem(_hTreeCtrl, hCurrentItem, TVGN_NEXT);
			}
		}

		/* delete possible not existed items */
		while (hCurrentItem != NULL)
		{
			HTREEITEM	pPrevItem	= hCurrentItem;
			hCurrentItem			= TreeView_GetNextItem(_hTreeCtrl, hCurrentItem, TVGN_NEXT);
			TreeView_DeleteItem(_hTreeCtrl, pPrevItem);
		}

		iRecCount--;
		if (iRecCount == 0)
			TREE_UNLOCK();

		if ((_hSemaphore) && (hParentItem != TVI_ROOT)) {
			/* start update overlay icons */
			DWORD	dwFlags	= 0;
			_hOverThread	= ::CreateThread(NULL, 0, TreeOverlayThread, &_ptIconUpdate[iThreadPos], 0, &dwFlags);
		}
	}
}

BOOL TreeHelper::FindFolderAfter(LPTSTR itemName, HTREEITEM pAfterItem)
{
	TCHAR		pszItem[MAX_PATH];
	BOOL		isFound			= FALSE;
	HTREEITEM	hCurrentItem	= TreeView_GetNextItem(_hTreeCtrl, pAfterItem, TVGN_NEXT);

	while (hCurrentItem != NULL)
	{
		GetItemText(hCurrentItem, pszItem, MAX_PATH);
		if (strcmp(itemName, pszItem) == 0)
		{
			isFound = TRUE;
			hCurrentItem = NULL;
		}
		else
		{
			hCurrentItem = TreeView_GetNextItem(_hTreeCtrl, hCurrentItem, TVGN_NEXT);
		}
	}

	return isFound;
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
	TCHAR				TEMP[MAX_PATH];
	HTREEITEM			pCurrentItem	= TreeView_GetNextItem(_hTreeCtrl, parentItem, TVGN_CHILD);
	BOOL				bHidden			= FALSE;
	WIN32_FIND_DATA		Find			= {0};
	HANDLE				hFind			= NULL;

	while (pCurrentItem != NULL)
	{
		GetItemText(pCurrentItem, TEMP, MAX_PATH);

		if (strcmp(childFolderName, TEMP) == 0)
		{
			TREE_UNLOCK();
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
	if (_hSemaphore) {
		ExtractIcons(parentFolderPathName, NULL, DEVT_DIRECTORY, &iIconNormal, &iIconSelected, NULL);
	} else {
		ExtractIcons(parentFolderPathName, NULL, DEVT_DIRECTORY, &iIconNormal, &iIconSelected, &iIconOverlayed);
	}

	/* set item */
	return InsertItem(childFolderName, iIconNormal, iIconSelected, iIconOverlayed, bHidden, parentItem, insertAfter, haveChildren);
}

HTREEITEM TreeHelper::InsertItem(LPTSTR lpszItem, 
								 INT nImage, 
								 INT nSelectedImage, 
								 INT nOverlayedImage,
								 BOOL bHidden,
								 HTREEITEM hParent, 
								 HTREEITEM hInsertAfter, 
								 BOOL haveChildren, 
								 LPARAM lParam)
{
	TV_INSERTSTRUCT tvis;
	HTREEITEM		hRetItem;

	ZeroMemory(&tvis, sizeof(TV_INSERTSTRUCT));
	tvis.hParent			 = hParent;
	tvis.hInsertAfter		 = hInsertAfter;
	tvis.item.mask			 = TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_CHILDREN;
	tvis.item.pszText		 = lpszItem;
	tvis.item.iImage		 = nImage;
	tvis.item.iSelectedImage = nSelectedImage;
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

	hRetItem = TreeView_InsertItem(_hTreeCtrl, &tvis);

	return hRetItem;
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
							INT nSelectedImage, 
							INT nOverlayedImage, 
							BOOL bHidden,
							BOOL haveChildren,
							LPARAM lParam,
							BOOL delChildren)
{
	TVITEM		item;
	BOOL		bRet;

	ZeroMemory(&item, sizeof(TVITEM));
	item.hItem			 = hItem;
	item.mask			 = TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_CHILDREN;
	item.pszText		 = lpszItem;
	item.iImage			 = nImage;
	item.iSelectedImage	 = nSelectedImage;
	item.cChildren		 = haveChildren;
	item.lParam			 = lParam;

	if (nOverlayedImage != 0)
	{
		item.mask		|= TVIF_STATE;
		item.state		|= INDEXTOOVERLAYMASK(nOverlayedImage);
		item.stateMask	|= TVIS_OVERLAYMASK;
	}

	if (bHidden == TRUE)
	{
		item.mask		|= LVIF_STATE;
		item.state		|= LVIS_CUT;
		item.stateMask  |= LVIS_CUT;
	}

	/* delete children items when available but not needed */
	if ((haveChildren == FALSE) && delChildren && TreeView_GetChild(_hTreeCtrl, hItem))	{
		DeleteChildren(hItem);
	}

	bRet = TreeView_SetItem(_hTreeCtrl, &item);

	return bRet;
}

void TreeHelper::SetOverlayIcon(HTREEITEM hItem, INT iOverlayIcon)
{
	/* Note: No LOCK */
	TVITEM		item;

	ZeroMemory(&item, sizeof(TVITEM));
	item.hItem			 = hItem;
	item.mask			 = TVIF_STATE;
	item.state			|= INDEXTOOVERLAYMASK(iOverlayIcon);
	item.stateMask		|= TVIS_OVERLAYMASK;

	TreeView_SetItem(_hTreeCtrl, &item);
}

BOOL TreeHelper::GetItemText(HTREEITEM hItem, LPTSTR szBuf, INT bufSize)
{
	BOOL	bRet;

	TVITEM			tvi;
	tvi.mask		= TVIF_TEXT;
	tvi.hItem		= hItem;
	tvi.pszText		= szBuf;
	tvi.cchTextMax	= bufSize;

	bRet = TreeView_GetItem(_hTreeCtrl, &tvi);

	return bRet;
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

BOOL TreeHelper::GetItemIcons(HTREEITEM hItem, LPINT piIcon, LPINT piSelected, LPINT piOverlay)
{
	if ((piIcon == NULL) || (piSelected == NULL) || (piOverlay == NULL))
		return FALSE;

	TVITEM			tvi;
	tvi.mask		= TVIF_STATE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvi.hItem		= hItem;
	tvi.stateMask	= TVIS_OVERLAYMASK;

	BOOL bRet = TreeView_GetItem(_hTreeCtrl, &tvi);

	if (bRet) {
		*piIcon			= tvi.iImage;
		*piSelected		= tvi.iSelectedImage;
		*piOverlay		= (tvi.state >> 8) & 0xFF;
	}

	return bRet;
}

BOOL TreeHelper::IsItemExpanded(HTREEITEM hItem)
{
	return (BOOL)(TreeView_GetItemState(_hTreeCtrl, hItem, TVIS_EXPANDED) & TVIS_EXPANDED);
}

