#include "lib.h"

#include <QCoreApplication>

#include "fnd/NonCopyMovable.h"

#include "zip/interface/error.h"

#ifdef _WIN32
	#include <Windows.h>
constexpr auto LIBRARY_NAME = L"7z.dll";
#else
	#include <dlfcn.h>

	#define LoadLibrary(lib_name) dlopen( (lib_name).c_str(), RTLD_LAZY )
	#define GetProcAddress dlsym
	#define FreeLibrary dlclose
constexpr auto LIBRARY_NAME = "./7z.so";
#endif

namespace HomeCompa::ZipDetails::SevenZip
{

namespace
{

constexpr auto ENTRY_POINT = "CreateObject";

using ObjectCreator = UINT32(WINAPI*)(const GUID* clsID, const GUID* interfaceID, void** outObject);

struct Dll
{
	HMODULE handle;

	Dll()
		: handle(LoadLibrary(LIBRARY_NAME))
	{
		if (!handle)
			Error::CannotLoadLibrary(QString::fromStdWString(LIBRARY_NAME));
	}

	~Dll()
	{
		FreeLibrary(handle);
	}

	NON_COPY_MOVABLE(Dll)
};

struct EntryPoint
{
	ObjectCreator handle;

	explicit EntryPoint(const Dll& dll)
		: handle(reinterpret_cast<ObjectCreator>(GetProcAddress(dll.handle, ENTRY_POINT)))
	{
		if (!handle)
			Error::CannotFindEntryPoint(ENTRY_POINT);
	}
};

} // namespace

struct Lib::Impl
{
	Dll        dll;
	EntryPoint func { dll };
};

Lib::Lib() = default;

Lib::~Lib() = default;

bool Lib::CreateObject(const GUID& clsID, const GUID& interfaceID, void** outObject) const //-V835
{
	assert(m_impl->func.handle);
	return SUCCEEDED(m_impl->func.handle(&clsID, &interfaceID, outObject));
}

} // namespace HomeCompa::ZipDetails::SevenZip
