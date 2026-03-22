#pragma once

#include <filesystem>
#include <string>

#include "fnd/NonCopyMovable.h"

#include "export/util.h"

namespace HomeCompa::Util
{

class UTIL_EXPORT DyLib
{
	NON_COPY_MOVABLE(DyLib)

public:
	DyLib();
	explicit DyLib(std::filesystem::path moduleName);
	~DyLib();

	bool  IsOpen() const; ///< @return true - модуль загружен.
	void  Detach(); ///< Отсоединиться от модуля. Модуль остается загруженным.
	bool  Open(std::filesystem::path moduleName); ///< Загрузить модуль. @return true - модуль успешно загружен.
	void  Close(); ///< Выгрузить модуль.
	void* GetProc(const std::string& procName); ///< Получить адрес функции

	template <typename ProcType>
	ProcType GetTypedProc(const std::string& procName)
	{
		void* const procPtr = this->GetProc(procName);
		return reinterpret_cast<ProcType>(procPtr);
	}

	std::string GetErrorDescription()
	{
		return m_errorDescription;
	}

private:
	void*                 m_handle { nullptr };
	std::filesystem::path m_moduleName;
	std::string           m_errorDescription;
};

} // namespace HomeCompa::Util
