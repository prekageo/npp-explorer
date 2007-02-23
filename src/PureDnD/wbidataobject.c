/*
	--------------------------------------------------------------------------------
  C 'translation of dataobject.cpp
	--------------------------------------------------------------------------------
*/

#include "wbded.h"

#include <stdio.h> // for printf()

static FORMATETC *formats;
static int nformats;

HRESULT CreateEnumFormatEtc(UINT nNumFormats, FORMATETC *pFormatEtc, IEnumFORMATETC **ppEnumFormatEtc);


HGLOBAL DupMem(HGLOBAL hMem)
{
	// lock the source memory object
	DWORD   len    = GlobalSize(hMem);
	PVOID   source = GlobalLock(hMem);
	
	// create a fixed "global" block - i.e. just
	// a regular lump of our process heap
	PVOID   dest   = GlobalAlloc(GMEM_FIXED, len);

	memcpy(dest, source, len);

	GlobalUnlock(hMem);

	return dest;
}


int LookupFormatEtc(WB_IDataObject* This, FORMATETC *pFormatEtc)
{
	int i;
	
//	MessageBox(0,"LookupFormatEtc","davide",MB_OK);
	
	for(i = 0; i < This->m_nNumFormats; i++)
	{
		if((pFormatEtc->tymed    &  This->m_pFormatEtc[i].tymed)   &&
			pFormatEtc->cfFormat == This->m_pFormatEtc[i].cfFormat && 
			pFormatEtc->dwAspect == This->m_pFormatEtc[i].dwAspect)
		{
			return i;
		}
	}

	return -1;

}

static ULONG STDMETHODCALLTYPE
idataobject_addref (WB_IDataObject *This)
{
  WB_IDataObject *dobj = This;
  int ref_count = ++dobj->ref_count;
//  printf("idataobject_addref\n");
  return InterlockedIncrement(&This->m_lRefCount);
}

static HRESULT STDMETHODCALLTYPE
idataobject_queryinterface (WB_IDataObject *This,
			    REFIID       riid,
			    LPVOID      *ppvObject)
{

//  printf("queryinterface");

  *ppvObject = NULL;

  if (IsEqualGUID (riid, &IID_IUnknown))
    {
      idataobject_addref (This);
      *ppvObject = This;
      return S_OK;
    }
  else if (IsEqualGUID (riid, &IID_IDataObject))
    {
      idataobject_addref (This);
      *ppvObject = This;
      return S_OK;
    }
  else
    {
      return E_NOINTERFACE;
    }
}

static ULONG STDMETHODCALLTYPE
idataobject_release (WB_IDataObject *This)
{
  WB_IDataObject *dobj = This;
  int ref_count = --dobj->ref_count;

//  printf("idataobject_release\n");

  if (ref_count == 0)
    g_free (This);

  return ref_count;
}

static HRESULT STDMETHODCALLTYPE
idataobject_getdata (WB_IDataObject *This,
		     LPFORMATETC  pFormatEtc,
		     LPSTGMEDIUM  pMedium)
{
	
	int idx;

//	MessageBox(0,"idataobject_getdata","davide",MB_OK);

	//
	// try to match the requested FORMATETC with one of our supported formats
	//
	if((idx = LookupFormatEtc(This, pFormatEtc)) == -1)
	{
		return DV_E_FORMATETC;
	}

	//
	// found a match! transfer the data into the supplied storage-medium
	//
	pMedium->tymed			 = This->m_pFormatEtc[idx].tymed;
	pMedium->pUnkForRelease  = 0;
	
	switch(This->m_pFormatEtc[idx].tymed)
	{
	case TYMED_HGLOBAL:

		pMedium->hGlobal = DupMem(This->m_pStgMedium[idx].hGlobal);
		//return S_OK;
		break;

	default:
		return DV_E_FORMATETC;
	}

	return S_OK;

}

static HRESULT STDMETHODCALLTYPE
idataobject_getdatahere (LPDATAOBJECT This,
			 LPFORMATETC  pFormatEtc,
			 LPSTGMEDIUM  pMedium)
{
  return DATA_E_FORMATETC;
}

static HRESULT STDMETHODCALLTYPE
idataobject_querygetdata (LPDATAOBJECT This,
			  LPFORMATETC  pFormatEtc)
{
 return (LookupFormatEtc(This, pFormatEtc) == -1) ? DV_E_FORMATETC : S_OK;
}

static HRESULT STDMETHODCALLTYPE
idataobject_getcanonicalformatetc (LPDATAOBJECT This,
				   LPFORMATETC  pFormatEtcIn,
				   LPFORMATETC  pFormatEtcOut)
{
  pFormatEtcOut->ptd = NULL;
	return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
idataobject_setdata (LPDATAOBJECT This,
		     LPFORMATETC  pFormatEtc,
		     LPSTGMEDIUM  pMedium,
		     BOOL         fRelease)
{
	return E_UNEXPECTED;
}

static HRESULT STDMETHODCALLTYPE
idataobject_enumformatetc (WB_IDataObject*     This,
			   DWORD            dwDirection,
			   LPENUMFORMATETC *ppEnumFormatEtc)
{
//	MessageBox(0,"idataobject_enumformatetc","davide",MB_OK);	
  
	if (dwDirection != DATADIR_GET) {
//	MessageBox(0,"idataobject_enumformatetc not impl","davide",MB_OK);
    return E_NOTIMPL;
  }

	return CreateEnumFormatEtc(This->m_nNumFormats, This->m_pFormatEtc, ppEnumFormatEtc);
}

static HRESULT STDMETHODCALLTYPE
idataobject_dadvise (LPDATAOBJECT This,
		     LPFORMATETC  pFormatetc,
		     DWORD        advf,
		     LPADVISESINK pAdvSink,
		     DWORD       *pdwConnection)
{
  return E_FAIL;
}

static HRESULT STDMETHODCALLTYPE
idataobject_dunadvise (LPDATAOBJECT This,
		       DWORD         dwConnection)
{
  return E_FAIL;
}

static HRESULT STDMETHODCALLTYPE
idataobject_enumdadvise (LPDATAOBJECT    This,
			 LPENUMSTATDATA *ppenumAdvise)
{
  return E_FAIL;
}

static IDataObjectVtbl ido_vtbl = {
  idataobject_queryinterface,
  idataobject_addref,
  idataobject_release,
  idataobject_getdata,
  idataobject_getdatahere,
  idataobject_querygetdata,
  idataobject_getcanonicalformatetc,
  idataobject_setdata,
  idataobject_enumformatetc,
  idataobject_dadvise,
  idataobject_dunadvise,
  idataobject_enumdadvise
};

WB_IDataObject * WB_IDataObject_new (int count)
{
  WB_IDataObject *result;

  result = g_new0(WB_IDataObject, 1);

  result->ido.lpVtbl = &ido_vtbl;

  result->m_lRefCount  = 1;
  result->ref_count = 1;
  result->m_nNumFormats = 1;
  
  result->m_pFormatEtc  = g_new0 (FORMATETC, count);
  result->m_pStgMedium  = g_new0 (STGMEDIUM, count);

  return result;
}


HRESULT CreateDataObject (FORMATETC *format, STGMEDIUM *medium, UINT count, WB_IDataObject **ppDataObject)
{
	
	int i;

	if(ppDataObject == 0)
		return E_INVALIDARG;

	*ppDataObject = WB_IDataObject_new(count);


	for(i = 0; i < count; i++)
	{
		(*ppDataObject)->m_pFormatEtc[i] = format[i];
		(*ppDataObject)->m_pStgMedium[i] = medium[i];
	}

	return (*ppDataObject) ? S_OK : E_OUTOFMEMORY;
}