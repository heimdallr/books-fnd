#pragma once

#include <QString>

#include <7zip/Archive/IArchive.h>
#include <7zip/IPassword.h>
#include <Common/MyCom.h>

namespace HomeCompa::ZipDetails::SevenZip
{

class ArchiveOpenCallback final
	: public IArchiveOpenCallback
	, public ICryptoGetTextPassword
{
public:
	static CMyComPtr<ArchiveOpenCallback> Create(QString password = {});

private:
	explicit ArchiveOpenCallback(QString password);

public:
	STDMETHOD(QueryInterface)(REFIID iid, void** ppvObject) override;
	STDMETHOD_(ULONG, AddRef)() override;
	STDMETHOD_(ULONG, Release)() override;

private:
	// IArchiveOpenCallback
	STDMETHOD(SetTotal)(const UInt64* files, const UInt64* bytes) noexcept override;
	STDMETHOD(SetCompleted)(const UInt64* files, const UInt64* bytes) noexcept override;

	// ICryptoGetTextPassword
	STDMETHOD(CryptoGetTextPassword)(BSTR* password) noexcept override;

private:
	LONG    m_refCount { 0 };
	QString m_password;
};

} // namespace HomeCompa::ZipDetails::SevenZip
