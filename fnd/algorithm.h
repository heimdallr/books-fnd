#pragma once

#include <algorithm>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <QDataStream>
#include <QHash>
#include <QIODevice>
#include <QString>
#include <QVariant>

namespace HomeCompa::Util {

template <typename T>
bool InBounds(const T value, std::convertible_to<T> auto min, std::convertible_to<T> auto max)
{
	return !(value < static_cast<T>(min)) && (value < static_cast<T>(max));
}

template <typename T>
static bool Set(T& dst, T value)
{
	if (dst == value)
		return false;

	dst = std::move(value);
	return true;
}

template <typename T, std::invocable F>
bool Set(T& oldValue, T newValue, const F& f)
{
	if (oldValue == newValue)
		return false;

	oldValue = std::move(newValue);
	f();
	return true;
}

template <typename T, std::invocable BeforeF, std::invocable AfterF>
bool Set(T& oldValue, T newValue, const BeforeF& before, const AfterF& after)
{
	if (oldValue == newValue)
		return false;

	before();
	oldValue = std::move(newValue);
	after();
	return true;
}

// из отсортированного диапазона делает вектор диапазонов из идущих подряд элементов
template <typename InputIterator, typename Value = typename std::iterator_traits<InputIterator>::value_type>
std::vector<std::pair<Value, Value>> CreateRanges(InputIterator begin, InputIterator end)
{
	assert(std::is_sorted(begin, end));
	if (begin == end)
		return {};

	std::vector<std::pair<Value, Value>> result {
		{ *begin, *begin + 1 }
	};
	for (++begin; begin != end; ++begin)
		if (*begin == result.back().second)
			++result.back().second;
		else
			result.emplace_back(*begin, *begin + 1);

	return result;
}

template <typename Container, typename ValueType = typename Container::value_type>
std::vector<std::pair<ValueType, ValueType>> CreateRanges(const Container& container)
{
	return Util::CreateRanges(std::cbegin(container), std::cend(container));
}

template <class InputIt1, class InputIt2>
bool Intersect(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2)
{
	assert(std::is_sorted(first1, last1) && std::is_sorted(first2, last2));
	while (first1 != last1 && first2 != last2)
	{
		if (*first1 < *first2)
		{
			++first1;
			continue;
		}
		if (*first2 < *first1)
		{
			++first2;
			continue;
		}
		return true;
	}
	return false;
}

template <typename Container1, typename Container2>
bool Intersect(const Container1& container1, const Container2& container2)
{
	return Intersect(std::cbegin(container1), std::cend(container1), std::cbegin(container2), std::cend(container2));
}

template <typename T>
void FixSize(T& width, T& height, const T max)
{
	if (width > max || height > max)
	{
		if (width > height)
		{
			height = height * max / width;
			width  = max;
		}
		else
		{
			width  = width * max / height;
			height = max;
		}
	}
}

template <typename First, typename Second>
struct PairHash
{
	size_t operator()(const std::pair<First, Second>& value) const
	{
		return std::rotl(std::hash<First>()(value.first), 1) | std::hash<Second>()(value.second);
	}
};

template <typename... Args>
struct TupleHash
{
	size_t operator()(const std::tuple<Args...>& value) const
	{
		size_t result = 42;

		const auto update = [&]<typename T>(const T& e) {
			result = std::rotl(result, 1) | std::hash<T>()(e);
		};

		std::apply(
			[&]<typename... T>(const T&... e) {
				((update(e)), ...);
			},
			value
		);

		return result;
	}
};

struct VariantHash
{
	size_t operator()(const QVariant& var) const
	{
		QByteArray bytes;
		{
			QDataStream stream(&bytes, QIODevice::WriteOnly);
			stream << var.typeName() << var;
		}

		return qHash(bytes);
	}
};

namespace QtUnorderedDetails {

template <typename T>
concept QtStringKey = std::same_as<std::remove_cvref_t<T>, QString> || std::same_as<std::remove_cvref_t<T>, QStringView>;

using QHashSeed = decltype(qHash(0));

constexpr auto callQHash(const auto& key, QHashSeed seed = 0) noexcept(noexcept(qHash(key, seed))) -> QHashSeed
{
	return qHash(key, seed);
}

template <Qt::CaseSensitivity Sensitivity = Qt::CaseInsensitive>
struct QtStringHash
{
	using is_transparent = void;

	template <QtStringKey T>
	static QHashSeed operator()(const T& key) noexcept
	{
		const QStringView sv(key);
		if constexpr (Sensitivity == Qt::CaseSensitive)
		{
			return qHash(sv);
		}
		else
		{
			QHashSeed seed = 0;
			for (qsizetype i = 0; i < sv.size(); ++i)
			{
				char32_t ucs4 = sv[i].unicode();
				if (QChar::isHighSurrogate(ucs4) && i + 1 < sv.size() && QChar::isLowSurrogate(sv[i + 1].unicode()))
				{
					ucs4 = QChar::surrogateToUcs4(ucs4, sv[++i].unicode());
				}
				seed = callQHash(QChar::toCaseFolded(ucs4), seed);
			}
			return seed;
		}
	}
};

template <Qt::CaseSensitivity Sensitivity = Qt::CaseInsensitive>
struct QtStringEqual
{
	using is_transparent = void;

	template <QtStringKey L, QtStringKey R>
	static bool operator()(const L& lhs, const R& rhs) noexcept
	{
		return QStringView(lhs).compare(QStringView(rhs), Sensitivity) == 0;
	}
};

} // namespace QtUnorderedDetails

template <QtUnorderedDetails::QtStringKey Key, Qt::CaseSensitivity Sensitivity = Qt::CaseSensitive, typename Allocator = std::allocator<Key>>
using QtStringSet = std::unordered_set<Key, QtUnorderedDetails::QtStringHash<Sensitivity>, QtUnorderedDetails::QtStringEqual<Sensitivity>, Allocator>;

template <QtUnorderedDetails::QtStringKey Key, typename Value, Qt::CaseSensitivity Sensitivity = Qt::CaseSensitive, typename Allocator = std::allocator<std::pair<const Key, Value>>>
using QtStringMap = std::unordered_map<Key, Value, QtUnorderedDetails::QtStringHash<Sensitivity>, QtUnorderedDetails::QtStringEqual<Sensitivity>, Allocator>;

namespace ComparerDetails {

template <Qt::CaseSensitivity Sensitivity, typename T>
struct CompareHelper
{
	static bool Compare(const T& lhs, const T& rhs) noexcept = delete;
};

template <Qt::CaseSensitivity Sensitivity>
struct CompareHelper<Sensitivity, QString>
{
	using is_transparent = void;

	static bool Compare(const QString& lhs, const QString& rhs) noexcept
	{
		return lhs.compare(rhs, Sensitivity) < 0;
	}
};

template <Qt::CaseSensitivity Sensitivity>
struct CompareHelper<Sensitivity, std::string_view>
{
	using is_transparent = void;

	static bool Compare(const std::string_view& lhs, const std::string_view& rhs) noexcept
	{
		return std::ranges::lexicographical_compare(lhs, rhs, [](unsigned char a, unsigned char b) {
			return std::tolower(a) < std::tolower(b);
		});
	}
};

template <Qt::CaseSensitivity Sensitivity>
struct CompareHelper<Sensitivity, std::string>
{
	using is_transparent = void;

	static bool Compare(const std::string_view& lhs, const std::string_view& rhs) noexcept
	{
		return CompareHelper<Sensitivity, std::string_view>::Compare(lhs, rhs);
	}
};

template <Qt::CaseSensitivity Sensitivity, typename... ARGS>
struct CompareHelper<Sensitivity, std::tuple<ARGS...>>
{
	using is_transparent = void;

	static bool Compare(const std::tuple<ARGS...>& lhs, const std::tuple<ARGS...>& rhs) noexcept
	{
		return CompareImpl(lhs, rhs, std::index_sequence_for<ARGS...> {});
	}

private:
	template <typename Tuple, std::size_t... Is>
	static constexpr bool CompareImpl(const Tuple& lhs, const Tuple& rhs, std::index_sequence<Is...>) noexcept
	{
		bool result = false;

		auto step = [&](auto idx) {
			constexpr std::size_t I = decltype(idx)::value;
			const auto&           a = std::get<I>(lhs);
			const auto&           b = std::get<I>(rhs);

			using ItemType = std::remove_cvref_t<std::tuple_element_t<I, Tuple>>;

			if (CompareHelper<Sensitivity, ItemType>::Compare(a, b))
			{
				result = true;
				return true;
			}
			if (CompareHelper<Sensitivity, ItemType>::Compare(b, a))
			{
				result = false;
				return true;
			}
			return false;
		};

		(step(std::integral_constant<std::size_t, Is> {}) || ...);
		return result;
	}
};

template <Qt::CaseSensitivity Sensitivity, typename First, typename Second>
struct CompareHelper<Sensitivity, std::pair<First, Second>>
{
	using is_transparent = void;

	static bool Compare(const std::pair<First, Second>& lhs, const std::pair<First, Second>& rhs) noexcept
	{
		const std::tuple<const First&, const Second&> lhst = lhs;
		const std::tuple<const First&, const Second&> rhst = rhs;
		return CompareHelper<Sensitivity, std::tuple<const First&, const Second&>>::Compare(lhst, rhst);
	}
};

} // namespace ComparerDetails

template <Qt::CaseSensitivity Sensitivity = Qt::CaseSensitive>
struct Comparer
{
	using is_transparent = void;

	template <typename L, typename R>
	static bool operator()(const L& lhs, const R& rhs) noexcept
	{
		using CleanL = std::decay_t<L>;
		using CleanR = std::decay_t<R>;

		if constexpr (std::is_same_v<CleanL, CleanR>)
		{
			return ComparerDetails::CompareHelper<Sensitivity, L>::Compare(lhs, rhs);
		}
		else if constexpr (std::is_constructible_v<CleanL, CleanR>)
		{
			return ComparerDetails::CompareHelper<Sensitivity, L>::Compare(lhs, static_cast<CleanL>(rhs));
		}
		else if constexpr (std::is_convertible_v<CleanR, CleanL>)
		{
			return ComparerDetails::CompareHelper<Sensitivity, R>::Compare(static_cast<CleanR>(lhs), rhs);
		}
		else
		{
			static_assert(std::is_same_v<CleanL, CleanR>, "Types L and R are not convertible to each other");
		}
	}
};

} // namespace HomeCompa::Util
