

#ifndef _WBDEDINC_
#define _WBDEDINC_

#include "stdafx.h"
#include <ole2.h>


#define g_new0(struct_type, n_structs)		\
    ((struct_type *) g_malloc0 (((gsize) sizeof (struct_type)) * ((gsize) (n_structs))))

#define g_new(struct_type, n_structs)		\
    ((struct_type *) g_malloc (((gsize) sizeof (struct_type)) * ((gsize) (n_structs))))

// glib's configure
typedef unsigned int		gsize;
// from gtypes.h
typedef unsigned int		guint;
typedef void*				gpointer;
typedef unsigned long		gulong;

gpointer g_malloc0			(gulong	n_bytes);
gpointer g_malloc			(gulong	 n_bytes);

typedef struct				_GMemVTable GMemVTable;

void						g_free	(gpointer	 mem);

// data object:

typedef struct {
	IDataObject			ido;
	int					ref_count;
	FORMATETC *			m_pFormatEtc;
	STGMEDIUM *			m_pStgMedium;
	LONG				m_nNumFormats;
	LONG				m_lRefCount;
} WB_IDataObject;


typedef struct {
	IEnumFORMATETC		ief;
	int					ref_count;
	int					ix;
	LONG				m_lRefCount;		// Reference count for this COM interface
	ULONG				m_nIndex;			// current enumerator index
	ULONG				m_nNumFormats;		// number of FORMATETC members
	FORMATETC *			m_pFormatEtc;
} WB_IEnumFORMATETC;

typedef struct {
	IDropSource ids;
	LONG	   m_lRefCount;
}	WB_IDropSource;

WB_IEnumFORMATETC *WB_IEnumFORMATETC_new (UINT, FORMATETC *);

#else
#endif

