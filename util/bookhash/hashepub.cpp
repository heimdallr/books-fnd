#include <QCryptographicHash>
#include <QtTypes.h>

#include <ranges>

#include <QBuffer>

#include "xml/SaxParser.h"

#include "EpubParser.h"
#include "StrUtil.h"
#include "hashbook.h"
#include "log.h"
#include "parser.h"
#include "xml/XmlUtil.h"

using namespace HomeCompa::Util;
using namespace HomeCompa;

namespace
{

struct HtmlParser final : private SaxParser
{
	std::unordered_map<QString, size_t> hist;

	HtmlParser(QIODevice& input, QCryptographicHash& md5)
		: SaxParser(input, 512)
		, m_md5 { md5 }
	{
		Parse();
	}

private: // SaxParser
	bool OnCharacters(const QString&, const QString& value) override
	{
		auto valueCopy = value;

		PrepareTitle(valueCopy);
		for (auto&& word : valueCopy.split(' ', Qt::SkipEmptyParts))
		{
			UpdateHash(word);

			RemoveIf(word, [](const QChar ch) {
				const auto category = ch.category();
				return category < QChar::Letter_Lowercase || category > QChar::Letter_Other;
			});
			if (word.isEmpty())
				continue;

			++hist[word];
		}
		return true;
	}

private:
	void UpdateHash(QString value) const
	{
		RemoveIf(value, [](const QChar ch) {
			return ch.category() != QChar::Letter_Lowercase;
		});
		m_md5.addData(value.toUtf8());
	}

private:
	QCryptographicHash& m_md5;
};

class EpubParserImpl final : public BookHash::IParser
{
public:
	explicit EpubParserImpl(QIODevice& stream)
		: m_result { Parse(stream, EpubParser::Mode::All) }
	{
	}

private: // BookHash::IParser
	HashParseResult GetResult() override
	{
		static constexpr const char* textExt[] { ".htm", ".html", ".xhtml", ".xml" };

		QCryptographicHash                  md5 { QCryptographicHash::Md5 };
		QStringList                         sections;
		std::unordered_map<QString, size_t> hist;

		for (auto [id, body] : m_result.texts | std::views::filter([](const auto& item) {
								   return std::ranges::any_of(textExt, [&](const char* ext) {
									   return item.id.endsWith(ext, Qt::CaseInsensitive);
								   });
							   }))
		{
#ifdef ADDITIONAL_LOG_ENABLED
			PLOGV << "process " << id;
#endif

			body = RemoveDocType(std::move(body));
			QBuffer buffer(&body);
			buffer.open(QIODevice::ReadOnly);
			HtmlParser parser(buffer, md5);

			for (const auto& [word, count] : parser.hist)
				hist[word] += count;

			auto [hashValues, hash, count, size] = CalculateHash(parser.hist);
			sections << QString("1\t%1\t%2\t%3").arg(hash).arg(count).arg(size);
		}

		auto [hashValues, hash, count, size] = CalculateHash(hist);
		sections.push_front(QString("0\t%1\t%2\t%3").arg(hash).arg(count).arg(size));

		HashParseResult result {
			.id           = QString::fromUtf8(md5.result().toHex()),
			.title        = std::move(m_result.title),
			.hashText     = std::move(hash),
			.hashSections = std::move(sections),
			.annotation   = QStringList { std::move(m_result.annotation) },
			.hashValues   = std::move(hashValues),
		};
		return result;
	}

	ImageHashItem GetCover() override
	{
		if (!m_result.coverExists)
			return {};

		assert(!m_result.images.empty());
		auto& cover = m_result.images.front();
		return { .file = std::move(cover.id), .body = std::move(cover.body) };
	}

	ImageHashItems GetImages() override
	{
		return m_result.images | std::views::as_rvalue | std::views::drop(m_result.coverExists ? 1 : 0) | std::views::transform([](auto&& item) {
				   return ImageHashItem { .file = std::move(item.id), .body = std::move(item.body) };
			   })
		     | std::ranges::to<std::vector>();
	}

private:
	EpubParser::ParseResult m_result;
};

} // namespace

namespace HomeCompa::Util::BookHash
{

std::unique_ptr<IParser> create_epub_parser(QIODevice& stream)
{
	return std::make_unique<EpubParserImpl>(stream);
}

} // namespace HomeCompa::Util::BookHash
