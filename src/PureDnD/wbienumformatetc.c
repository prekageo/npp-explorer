/*
	--------------------------------------------------------------------------------
  C 'translation of enumformat.cpp
	--------------------------------------------------------------------------------
*/

#include "wbded.h"

static FORMATETC *formats;
static int nformats;

static void DeepCopyFormatEtc(FORMATETC *dest, FORMATETC *source)
{
	// copy the source FORMATETC into dest
	*dest = *source;
	
	if(source->ptd)
	{
		// allocate memory for the DVTARGETDEVICE if necessary
		dest->ptd = (DVTARGETDEVICE*)CoTaskMemAlloc(sizeof(DVTARGETDEVICE));

		// copy the contents of the source DVTARGETDEVICE into dest->ptd
		*(dest->ptd) = *(source->ptd);
	}
}


HRESULT CreateEnumFormatEtc(UINT nNumFormats, FORMATETC *pFormatEtc, WB_IEnumFORMATETC **ppEnumFormatEtc)
{
	if(nNumFormats == 0 || pFormatEtc == 0 || ppEnumFormatEtc == 0)
		return E_INVALIDARG;

	*ppEnumFormatEtc = WB_IEnumFORMATETC_new(nNumFormats, pFormatEtc);

	return (*ppEnumFormatEtc) ? S_OK : E_OUTOFMEMORY;
}


static ULONG STDMETHODCALLTYPE
ienumformatetc_addref (WB_IEnumFORMATETC* This)
{
  WB_IEnumFORMATETC *en = This;

  return InterlockedIncrement(&This->m_lRefCount);
}

static HRESULT STDMETHODCALLTYPE
ienumformatetc_queryinterface (WB_IEnumFORMATETC *This,
			       REFIID          riid,
			       LPVOID         *ppvObject)
{

  *ppvObject = NULL;


  if (IsEqualGUID (riid, &IID_IUnknown))
    {
      ienumformatetc_addref (This);
      *ppvObject = This;
      return S_OK;
    }
  else if (IsEqualGUID (riid, &IID_IEnumFORMATETC))
    {
      ienumformatetc_addref (This);
      *ppvObject = This;
      return S_OK;
    }
  else
    {
      return E_NOINTERFACE;
    }
}

static ULONG STDMETHODCALLTYPE
ienumformatetc_release (WB_IEnumFORMATETC* This)
{
    
  LONG count = InterlockedDecrement(&This->m_lRefCount);

  if(count == 0)
	{
		g_free(This);
		return 0;
	}
	else
	{
		return count;
	}
  
}

static HRESULT STDMETHODCALLTYPE
ienumformatetc_next (WB_IEnumFORMATETC* This,
		     ULONG	     celt,
		     LPFORMATETC     pFormatEtc,
	    	    ULONG *pceltFetched)
{
  
	ULONG copied  = 0;


	if(celt == 0 || pFormatEtc == 0)
		return E_INVALIDARG;

	while(This->m_nIndex < This->m_nNumFormats && copied < celt)
	{
		DeepCopyFormatEtc(&pFormatEtc[copied], &This->m_pFormatEtc[This->m_nIndex]);
		copied++;
		This->m_nIndex++;
	}

	if(pceltFetched != 0) 
		*pceltFetched = copied;

	// did we copy all that was requested?
	return (copied == celt) ? S_OK : S_FALSE;

  
}

static HRESULT STDMETHODCALLTYPE
ienumformatetc_skip (WB_IEnumFORMATETC *This,
		     ULONG	     celt)
{
  WB_IEnumFORMATETC *en = This;

  en->m_nIndex += celt;
	return (en->m_nIndex <= en->m_nNumFormats) ? S_OK : S_FALSE;

  return S_OK;
}

static HRESULT STDMETHODCALLTYPE
ienumformatetc_reset (WB_IEnumFORMATETC* This)
{
  
	WB_IEnumFORMATETC *en = This;

  en->ix = 0;
	en->m_nIndex = 0;

  return S_OK;
}

static HRESULT STDMETHODCALLTYPE
ienumformatetc_clone (WB_IEnumFORMATETC*  This,
		      LPENUMFORMATETC *ppEnumFormatEtc)
{
  WB_IEnumFORMATETC *en = This;
  WB_IEnumFORMATETC *new;

  new = WB_IEnumFORMATETC_new (en->m_nNumFormats, en->m_pFormatEtc);

	*ppEnumFormatEtc = (IEnumFORMATETC*)new;

  ((WB_IEnumFORMATETC*)*ppEnumFormatEtc)->m_nIndex = en->m_nIndex;
  
  return S_OK;
}

static IEnumFORMATETCVtbl ief_vtbl = {
	ienumformatetc_queryinterface,
	ienumformatetc_addref,
	ienumformatetc_release,
	ienumformatetc_next,
	ienumformatetc_skip,
	ienumformatetc_reset,
	ienumformatetc_clone
};

WB_IEnumFORMATETC *
WB_IEnumFORMATETC_new (UINT nNumFormats, FORMATETC *pFormatEtc)
{
  
  UINT i = 0;	
  WB_IEnumFORMATETC *result;

  result = g_new0 (WB_IEnumFORMATETC, 1);

  result->ief.lpVtbl = &ief_vtbl;
  result->ref_count = 1;
  result->ix = 1;
  result->m_nIndex = 0;
  result->m_lRefCount = 1;
  result->m_nNumFormats = nNumFormats;
  result->m_pFormatEtc = g_new0(FORMATETC, nNumFormats);
	result->m_pFormatEtc = pFormatEtc;
  
  for(i = 0; i < result->m_nNumFormats; i++)
	{	
		DeepCopyFormatEtc(&result->m_pFormatEtc[i], &pFormatEtc[i]);
	}

  return result;
}
