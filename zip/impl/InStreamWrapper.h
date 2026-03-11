#pragma once

#include <7zip/IStream.h>

#include "unknown_impl.h"

namespace HomeCompa::ZipDetails::SevenZip
{

class InStreamWrapper final
	: public IInStream
	, public IStreamGetSize
{
	ADD_RELEASE_REF_IMPL

public:
	static CMyComPtr<InStreamWrapper> Create(CMyComPtr<IInStream> baseStream);

private:
	explicit InStreamWrapper(CMyComPtr<IInStream> baseStream);

public:
	STDMETHOD(QueryInterface)(REFIID iid, void** ppvObject) override;

private:
	// ISequentialInStream
	STDMETHOD(Read)(void* data, UInt32 size, UInt32* processedSize) override;

	// IInStream
	STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64* newPosition) override;

	// IStreamGetSize
	STDMETHOD(GetSize)(UInt64* size) override;

private:
	CMyComPtr<IInStream> m_baseStream;
};

} // namespace HomeCompa::ZipDetails::SevenZip
