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
	static CMyComPtr<InStreamWrapper> Create(CMyComPtr<IInStream> baseStream, UInt64 size);

private:
	InStreamWrapper(CMyComPtr<IInStream> baseStream, UInt64 size);

public:
	STDMETHOD(QueryInterface)(REFIID iid, void** ppvObject) override;

private:
	// ISequentialInStream
	STDMETHOD(Read)(void* data, UInt32 size, UInt32* processedSize) noexcept override;

	// IInStream
	STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64* newPosition) noexcept override;

	// IStreamGetSize
	STDMETHOD(GetSize)(UInt64* size) noexcept override;

private:
	CMyComPtr<IInStream> m_baseStream;
	const UInt64         m_size;
};

} // namespace HomeCompa::ZipDetails::SevenZip
