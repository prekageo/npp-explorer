/*
	--------------------------------------------------------------------------------
  C 'translation of dropsource.cpp
	--------------------------------------------------------------------------------
*/

#include "wbded.h"

static ULONG STDMETHODCALLTYPE
idropsource_addref (WB_IDropSource *This)
{
	// increment object reference count
    return InterlockedIncrement(&This->m_lRefCount);
}

static ULONG STDMETHODCALLTYPE
idropsource_release (WB_IDropSource *This)
{
	LONG count = InterlockedDecrement(&This->m_lRefCount);
		
	if(count == 0)
	{
		g_free (This);
		return 0;
	}
	else
	{
		return count;
	}
}

static ULONG STDMETHODCALLTYPE
idropsource_queryinterface (WB_IDropSource *This, REFIID riid, void **ppvObject)
{
	if (IsEqualGUID (riid, &IID_IUnknown) || IsEqualGUID (riid, &IID_IDropSource))
    {
        idropsource_addref (This);
        *ppvObject = This;
        return S_OK;
    }
 	else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }
}

static ULONG STDMETHODCALLTYPE
idropsource_querycontinuedrag(WB_IDropSource* This, BOOL fEscapePressed, DWORD grfKeyState)
{
	// if the <Escape> key has been pressed since the last call, cancel the drop
	if(fEscapePressed == TRUE)
		return DRAGDROP_S_CANCEL;	

	// if the <LeftMouse> button has been released, then do the drop!
	if((grfKeyState & MK_LBUTTON) == 0)
		return DRAGDROP_S_DROP;

	// continue with the drag-drop
	return S_OK;
}

static ULONG STDMETHODCALLTYPE
idropsource_givefeedback(WB_IDropSource* This, DWORD dwEffect) {
	return DRAGDROP_S_USEDEFAULTCURSORS;
}

static IDropSourceVtbl ids_vtbl = {
	idropsource_queryinterface,
	idropsource_addref,
	idropsource_release,
	idropsource_querycontinuedrag,
	idropsource_givefeedback
};



WB_IDropSource * WB_IDropSource_new (void) {
	
	WB_IDropSource *result;
	result = g_new0(WB_IDropSource, 1);

	result->ids.lpVtbl = &ids_vtbl;
	result->m_lRefCount = 1;


	return result;

}


HRESULT CreateDropSource(WB_IDropSource **ppDropSource)
{
	if(ppDropSource == 0)
		return E_INVALIDARG;

	*ppDropSource = WB_IDropSource_new();

	return (*ppDropSource) ? S_OK : E_OUTOFMEMORY;

}