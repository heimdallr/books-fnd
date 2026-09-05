#pragma once

#include <QString>

#include "fnd/FindPair.h"
#include "fnd/NonCopyMovable.h"
#include "fnd/memory.h"

#include "export/util.h"

class QIODevice;

namespace HomeCompa::Util
{

class XmlAttributes;

class UTIL_EXPORT SaxParser
{
	NON_COPY_MOVABLE(SaxParser)

public:
	struct PszComparerEndsWithCaseInsensitive
	{
		bool operator()(const std::string_view lhs, const std::string_view rhs) const
		{
			if (lhs.size() < rhs.size())
				return false;

			const auto *lp = lhs.data() + (lhs.size() - rhs.size()), *rp = rhs.data();
			while (*lp && *rp && std::tolower(*lp++) == std::tolower(*rp++))
				;

			return !*lp && !*rp;
		}
	};

protected:
	explicit SaxParser(QIODevice& stream);
	virtual ~SaxParser();

protected:
	template <typename Obj, typename Value, size_t ArraySize, typename... ARGS>
	bool Parse(Obj& obj, Value (&array)[ArraySize], const QStringView key, const ARGS&... args)
	{
		m_processed       = true;
		const auto it     = std::ranges::find_if(array, [&](const auto& item) {
			return key == item.first;
		});
		const auto parser = it != std::end(array) ? it->second : &SaxParser::Stub<ARGS...>;
		return std::invoke(parser, obj, std::cref(args)...);
	}

	bool IsLastItemProcessed() const noexcept;

private:
	template <typename... ARGS>
	// ReSharper disable once CppMemberFunctionMayBeStatic
	bool Stub(const ARGS&...)
	{
		m_processed = false;
		return true;
	}

public:
	void Parse();

public:
	virtual bool OnProcessingInstruction(QStringView target, QStringView data);
	virtual bool OnXMLDecl(QStringView versionStr, QStringView encodingStr, QStringView standaloneStr, QStringView actualEncodingStr);

	virtual bool OnStartElement(QStringView name, const QString& path, const XmlAttributes& attributes);
	virtual bool OnEndElement(const QString& name, const QString& path);
	virtual bool OnCharacters(const QString& path, QStringView value);

	virtual bool OnWarning(size_t line, size_t column, const QString& text);
	virtual bool OnError(size_t line, size_t column, const QString& text);
	virtual bool OnFatalError(size_t line, size_t column, const QString& text);

private:
	class Impl;
	PropagateConstPtr<Impl> m_impl;

protected:
	bool m_processed { true };
};

} // namespace HomeCompa::Util
