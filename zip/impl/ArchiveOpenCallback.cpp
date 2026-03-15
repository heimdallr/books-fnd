#include "ArchiveOpenCallback.h"

using namespace HomeCompa::ZipDetails::SevenZip;

CMyComPtr<ArchiveOpenCallback> ArchiveOpenCallback::Create(QString password)
{
	return new ArchiveOpenCallback(std::move(password));
}

ArchiveOpenCallback::ArchiveOpenCallback(QString password)
	: m_password(std::move(password))
{
}

STDMETHODIMP ArchiveOpenCallback::QueryInterface(REFIID iid, void** ppvObject) //-V835
{
	if (iid == IID_IUnknown)
	{
		*ppvObject = reinterpret_cast<IUnknown*>(this);
		AddRef();
		return S_OK;
	}

	if (iid == IID_IArchiveOpenCallback)
	{
		*ppvObject = static_cast<IArchiveOpenCallback*>(this);
		AddRef();
		return S_OK;
	}

	if (iid == IID_ICryptoGetTextPassword)
	{
		*ppvObject = static_cast<ICryptoGetTextPassword*>(this);
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) ArchiveOpenCallback::AddRef()
{
	return ++m_refCount;
}

STDMETHODIMP_(ULONG) ArchiveOpenCallback::Release()
{
	if (--m_refCount != 0)
		return m_refCount;

	delete this;
	return 0;
}

STDMETHODIMP ArchiveOpenCallback::SetTotal(const UInt64* /*files*/, const UInt64* /*bytes*/) noexcept
{
	return S_OK;
}

STDMETHODIMP ArchiveOpenCallback::SetCompleted(const UInt64* /*files*/, const UInt64* /*bytes*/) noexcept
{
	return S_OK;
}

STDMETHODIMP ArchiveOpenCallback::CryptoGetTextPassword(BSTR* password) noexcept
{
	if (!m_password.isEmpty())
		*password = SysAllocString(m_password.toStdWString().data());

	return S_OK;
}
