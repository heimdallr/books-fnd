#include "InStreamWrapper.h"

using namespace HomeCompa::ZipDetails::SevenZip;

CMyComPtr<InStreamWrapper> InStreamWrapper::Create(CMyComPtr<IInStream> baseStream)
{
	return new InStreamWrapper(std::move(baseStream));
}

InStreamWrapper::InStreamWrapper(CMyComPtr<IInStream> baseStream)
	: m_baseStream(std::move(baseStream))
{
}

HRESULT STDMETHODCALLTYPE InStreamWrapper::QueryInterface(REFIID iid, void** ppvObject) //-V835
{
	if (iid == __uuidof(IUnknown)) // NOLINT(clang-diagnostic-language-extension-token)
	{
		*ppvObject = reinterpret_cast<IUnknown*>(this);
		AddRef();
		return S_OK;
	}

	if (iid == IID_ISequentialInStream)
	{
		*ppvObject = static_cast<ISequentialInStream*>(this);
		AddRef();
		return S_OK;
	}

	if (iid == IID_IInStream)
	{
		*ppvObject = static_cast<IInStream*>(this);
		AddRef();
		return S_OK;
	}

	if (iid == IID_IStreamGetSize)
	{
		*ppvObject = static_cast<IStreamGetSize*>(this);
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

STDMETHODIMP InStreamWrapper::Read(void* data, const UInt32 size, UInt32* processedSize)
{
	UInt32        read = 0;
	const HRESULT hr   = m_baseStream->Read(data, size, &read);
	if (processedSize)
		*processedSize = read;

	return SUCCEEDED(hr) ? S_OK : hr;
}

STDMETHODIMP InStreamWrapper::Seek(const Int64 offset, const UInt32 seekOrigin, UInt64* newPosition)
{
	UInt64        newPos;
	const HRESULT hr = m_baseStream->Seek(offset, seekOrigin, &newPos);
	if (newPosition)
		*newPosition = newPos;

	return hr;
}

STDMETHODIMP InStreamWrapper::GetSize(UInt64* size)
{
	*size = 0;
	//	STATSTG       statInfo;
	//	const HRESULT hr = m_baseStream->Stat(&statInfo, STATFLAG_NONAME);
	//	if (SUCCEEDED(hr))
	//		*size = statInfo.cbSize.QuadPart;

	//	return hr;
	return S_OK;
}
