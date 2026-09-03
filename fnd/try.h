#pragma once

#include "fnd/StrUtil.h"

#include "log.h"

namespace HomeCompa::Util
{

template <typename R, typename S, typename T>
R Try(const S& name, const T& functor, const std::string_view file, const int line)
{
	try
	{
#ifndef NDEBUG
		PLOGV << name << " started";
#endif
		auto result = functor();
#ifndef NDEBUG
		PLOGV << name << " finished";
#endif
		return std::forward<R>(result);
	}
	catch (const std::exception& ex)
	{
		PLOGE << std::format("{} failed. {}, {}: {}", name, file, line, ex.what());
	}
	catch (...)
	{
		PLOGE << std::format("{}, {}: unknown error", file, line);
	}

	return R {};
}

} // namespace HomeCompa::Util

#define TRY(NAME, FUNCTOR) HomeCompa::Util::Try<std::invoke_result_t<decltype(FUNCTOR)>>(NAME, FUNCTOR, __FILE__, __LINE__)
