#include "hashparser.h"

#include "xml/SaxParser.h"
#include "xml/XmlAttributes.h"

using namespace HomeCompa::Util;
using namespace HomeCompa;

namespace
{

class HashParserImpl final : public SaxParser
{
	static constexpr auto BOOKS      = u"books";
	static constexpr auto BOOK       = u"books/book";
	static constexpr auto COVER      = u"books/book/cover";
	static constexpr auto IMAGE      = u"books/book/image";
	static constexpr auto ORIGIN     = u"books/book/origin";
	static constexpr auto HISTOGRAM  = u"books/book/histogram/item";
	static constexpr auto SECTION    = u"section";
	static constexpr auto ANNOTATION = u"books/book/annotation/p";

public:
	HashParserImpl(QIODevice& input, HashParser::IObserver& observer)
		: SaxParser(input)
		, m_observer { observer }
	{
		Parse();
	}

private: // Util::SaxParser
	bool OnStartElement(const QStringView name, const QStringView path, const XmlAttributes& attributes) override
	{
		if (path == BOOKS)
		{
			m_observer.OnParseStarted(attributes.GetAttribute(u"source"));
		}
		else if (path == BOOK)
		{
#define HASH_PARSER_CALLBACK_ITEM(NAME) m_##NAME = attributes.GetAttribute(u""#NAME).toString();
			HASH_PARSER_CALLBACK_ITEMS_X_MACRO
#undef HASH_PARSER_CALLBACK_ITEM
			m_size           = attributes.GetAttribute(u"size").toULongLong();
			m_simHash        = attributes.GetAttribute(u"simHash").toULongLong(nullptr, 16);
			m_section        = std::make_unique<HashParser::Section>();
			m_currentSection = m_section.get();
		}
		else if (path == ORIGIN)
		{
			m_originFolder = attributes.GetAttribute(u"folder").toString();
			m_originFile   = attributes.GetAttribute(u"file").toString();
		}
		else if (name == SECTION)
		{
			auto& section    = m_currentSection->children.try_emplace(attributes.GetAttribute(u"id").toString(), std::make_unique<HashParser::Section>()).first->second;
			section->count   = attributes.GetAttribute(u"count").toULongLong();
			section->size    = attributes.GetAttribute(u"size").toULongLong();
			section->simHash = attributes.GetAttribute(u"simHash").toULongLong(nullptr, 16);
			section->parent  = m_currentSection;
			m_currentSection = section.get();
		}
		else if (path == COVER)
		{
			m_cover.pHash = attributes.GetAttribute(u"pHash").toString();
		}
		else if (path == IMAGE)
		{
			m_images.emplace_back(attributes.GetAttribute(u"id").toString(), QString(), attributes.GetAttribute(u"pHash").toString());
			if (const auto linked = attributes.GetAttribute(u"linked"); !linked.isEmpty())
				m_images.back().linked = linked == u"true";
		}
		else if (path == HISTOGRAM)
		{
			m_textHistogram.emplace_back(attributes.GetAttribute(u"count").toULongLong(), attributes.GetAttribute(u"word").toString());
		}

		return true;
	}

	bool OnEndElement(const QStringView name, const QStringView path) override
	{
		if (path == BOOK)
		{
			assert(!m_id.isEmpty());
			if (!m_observer.OnBookParsed(
#define HASH_PARSER_CALLBACK_ITEM(NAME) std::move(m_##NAME),
					HASH_PARSER_CALLBACK_ITEMS_X_MACRO
#undef HASH_PARSER_CALLBACK_ITEM
						std::move(m_cover),
					std::move(m_images),
					std::move(m_section),
					m_size,
					m_simHash,
					std::move(m_textHistogram),
					std::move(m_annotation)
				))
				return false;

#define HASH_PARSER_CALLBACK_ITEM(NAME) m_##NAME = QString {};
			HASH_PARSER_CALLBACK_ITEMS_X_MACRO
#undef HASH_PARSER_CALLBACK_ITEM

			m_cover          = {};
			m_images         = {};
			m_section        = {};
			m_size           = 0;
			m_simHash        = 0;
			m_textHistogram  = {};
			m_annotation     = QStringList {};
			m_currentSection = nullptr;
		}
		else if (name == SECTION)
		{
			m_currentSection = m_currentSection->parent;
		}

		return true;
	}

	bool OnCharacters(const QStringView path, const QStringView value) override
	{
		if (path == COVER)
			m_cover.hash = value.toString();
		else if (path == IMAGE)
			m_images.back().hash = value.toString();
		else if (path == ANNOTATION)
			m_annotation << value.toString();
		return true;
	}

private:
	HashParser::IObserver& m_observer;
#define HASH_PARSER_CALLBACK_ITEM(NAME) QString m_##NAME;
	HASH_PARSER_CALLBACK_ITEMS_X_MACRO
#undef HASH_PARSER_CALLBACK_ITEM

	HashParser::HashImageItem              m_cover;
	std::vector<HashParser::HashImageItem> m_images;
	HashParser::Section::Ptr               m_section;
	HashParser::Section*                   m_currentSection { nullptr };
	TextHistogram                          m_textHistogram;
	QStringList                            m_annotation;
	size_t                                 m_size { 0 };
	uint64_t                               m_simHash { 0 };
};

} // namespace

void HashParser::Parse(QIODevice& input, IObserver& observer)
{
	[[maybe_unused]] const HashParserImpl parser(input, observer);
}
