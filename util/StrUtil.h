#pragma once

#include <iterator>
#include <string>

#include <QHash>
#include <QString>

#include "export/util.h"

class QString;

namespace HomeCompa::Util
{

UTIL_EXPORT int MultiByteToWideCharImpl(const char* lpMultiByteStr, int cbMultiByte, wchar_t* lpWideCharStr, int cchWideChar);
UTIL_EXPORT int WideCharToMultiByteImpl(const wchar_t* lpWideCharStr, int cchWideChar, char* lpMultiByteStr, int cbMultiByte);
UTIL_EXPORT int CompareStringImpl(const wchar_t* lpString1, int cchCount1, const wchar_t* lpString2, int cchCount2);

UTIL_EXPORT QString& SimplifyTitle(QString& value);
UTIL_EXPORT QString& PrepareTitle(QString& value);

template <typename SizeType, typename StringType>
SizeType StrSize(const StringType& str)
{
	return static_cast<SizeType>(std::size(str));
}

template <typename SizeType>
SizeType StrSize(const char* str)
{
	return static_cast<SizeType>(std::strlen(str));
}

template <typename SizeType>
SizeType StrSize(const wchar_t* str)
{
	return static_cast<SizeType>(std::wcslen(str));
}

template <typename StringType>
const char* MultiByteData(const StringType& str)
{
	return str.data();
}

inline const char* MultiByteData(const char* data)
{
	return data;
}

template <typename StringType>
std::wstring ToWide(const StringType& str)
{
	const auto   size = static_cast<std::wstring::size_type>(MultiByteToWideCharImpl(MultiByteData(str), StrSize<int>(str), nullptr, 0));
	std::wstring result(size, 0);
	MultiByteToWideCharImpl(MultiByteData(str), StrSize<int>(str), result.data(), StrSize<int>(result));

	return result;
}

template <typename StringType>
const wchar_t* WideData(const StringType& str)
{
	return str.data();
}

inline const wchar_t* WideData(const wchar_t* data)
{
	return data;
}

template <typename T>
concept WideStringType = std::is_same_v<T, const std::wstring&> || std::is_same_v<T, std::wstring> || std::is_same_v<T, std::wstring_view> || std::is_same_v<T, const wchar_t*>;

template <WideStringType StringType>
std::string ToMultiByte(const StringType& str)
{
	const auto  size = static_cast<std::wstring::size_type>(WideCharToMultiByteImpl(WideData(str), StrSize<int>(str), nullptr, 0));
	std::string result(size, 0);
	WideCharToMultiByteImpl(WideData(str), StrSize<int>(str), result.data(), StrSize<int>(result));

	return result;
}

template <typename LhsType, typename RhsType>
bool IsStringEqual(LhsType&& lhs, RhsType&& rhs)
{
	return CompareStringImpl(WideData(lhs), StrSize<int>(lhs), WideData(rhs), StrSize<int>(rhs)) == 0;
}

template <typename LhsType, typename RhsType>
bool IsStringLess(LhsType&& lhs, RhsType&& rhs)
{
	return CompareStringImpl(WideData(lhs), StrSize<int>(lhs), WideData(rhs), StrSize<int>(rhs)) < 0;
}

template <class T = void>
struct StringLess
{
	typedef T    _FIRST_ARGUMENT_TYPE_NAME;
	typedef T    _SECOND_ARGUMENT_TYPE_NAME;
	typedef bool _RESULT_TYPE_NAME;

	constexpr bool operator()(const T& lhs, const T& rhs) const
	{
		return lhs < rhs;
	}
};

template <>
struct StringLess<void>
{
	template <class LhsType, class RhsType>
	constexpr auto operator()(LhsType&& lhs, RhsType&& rhs) const noexcept(noexcept(static_cast<LhsType&&>(lhs) < static_cast<RhsType&&>(rhs))) // strengthened
		-> decltype(static_cast<LhsType&&>(lhs) < static_cast<RhsType&&>(rhs))
	{
		return IsStringLess(static_cast<LhsType&&>(lhs), static_cast<RhsType&&>(rhs));
	}

	using is_transparent = int;
};

template <typename T>
T ToInteger(const QStringView value, T defaultValue = 0)
{
	if (value.isEmpty())
		return defaultValue;

	bool       ok     = false;
	const auto result = value.toLongLong(&ok);
	return ok ? defaultValue : static_cast<T>(result);
}

/*
template <typename It>
std::wstring_view Next(It& beg, const It end, const wchar_t separator)
{
	beg                          = std::find_if(beg, end, [](const wchar_t c) {
        return !iswspace(c);
    });
	auto                    next = std::find(beg, end, separator);
	const std::wstring_view result(beg, next);
	beg = next != end ? std::next(next) : end;
	return result;
}
*/

struct WStringHash
{
	using is_transparent = int;

	[[nodiscard]] size_t operator()(const QString& txt) const
	{
		return GetHashImpl(txt);
	}

	[[nodiscard]] size_t operator()(const QStringView& txt) const
	{
		return GetHashImpl(txt);
	}

private:
	static size_t GetHashImpl(const QStringView& txt)
	{
		const auto txtLower = QString(txt).toLower();
		const auto result   = std::hash<QString>()(txtLower);
		return result;
	}
};

struct WStringEqualTo
{
	template <class U, class V>
	[[nodiscard]] constexpr auto operator()(const U& lhs, const V& rhs) const
	{
		return lhs.compare(rhs, Qt::CaseInsensitive) == 0;
	}

	using is_transparent = int;
};

} // namespace HomeCompa::Util
