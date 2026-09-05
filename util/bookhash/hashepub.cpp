#include <ranges>

#include <QBuffer>
#include <QCryptographicHash>
#include <QFileInfo>

#include "xml/SaxParser.h"
#include "xml/XmlAttributes.h"
#include "xml/XmlUtil.h"

#include "EpubParser.h"
#include "hashbook.h"
#include "log.h"
#include "parser.h"

using namespace HomeCompa::Util;
using namespace HomeCompa;

namespace
{

struct HtmlParser final : private SaxParser
{
	HtmlParser(QIODevice& input, std::unordered_set<QString>& linkedImage)
		: SaxParser(input)
		, m_linkedImages { linkedImage }
	{
		Parse();
	}

private: // Util::SaxParser
	bool OnStartElement(const QStringView name, const QStringView path, const XmlAttributes& attributes) override
	{
		if (name == "img" && path.startsWith(L"html/body", Qt::CaseInsensitive))
			if (auto imageName = attributes.GetAttribute(L"src").toString(); !imageName.isEmpty())
				m_linkedImages.emplace(std::move(imageName));

		return true;
	}

private:
	std::unordered_set<QString>& m_linkedImages;
};

void CollectLinkedImages(QByteArray body, std::unordered_set<QString>& linkedImage)
{
	body = RemoveDocType(std::move(body));
	QBuffer buffer(&body);
	buffer.open(QIODevice::ReadOnly);
	[[maybe_unused]] const HtmlParser parser(buffer, linkedImage);
}

class EpubParserImpl final : public BookHash::IParser
{
public:
	explicit EpubParserImpl(QIODevice& stream)
		: m_result { EpubParser::Parse(stream, CommonParser::Mode::All) }
	{
	}

private: // BookHash::IParser
	HashParseResult GetResult() override
	{
		static constexpr const char* textExt[] { ".htm", ".html", ".xhtml", ".xml" };

		QCryptographicHash                  md5 { QCryptographicHash::Md5 };
		QStringList                         sections;
		std::unordered_map<QString, size_t> hist;

		std::unordered_set<QString> linkedImage;

		for (auto [id, body] : m_result.texts | std::views::filter([](const auto& item) {
								   return std::ranges::any_of(textExt, [&](const char* ext) {
									   return item.id.endsWith(ext, Qt::CaseInsensitive);
								   });
							   }))
		{
#ifdef ADDITIONAL_LOG_ENABLED
			PLOGV << "process " << id;
#endif

			auto sectionHist = CollectHistogram(body, md5);
			CollectLinkedImages(body, linkedImage);

			for (const auto& [word, count] : sectionHist)
				hist[word] += count;

			auto [hashValues, hash, count, size, simHash] = CalculateHash(sectionHist);
			sections << QString("1\t%1\t%2\t%3\t%4").arg(hash).arg(count).arg(size).arg(simHash, 16, 16, QChar { '0' });
		}

		auto [hashValues, hash, count, size, simHash] = CalculateHash(hist);
		sections.push_front(QString("0\t%1\t%2\t%3\t%4").arg(hash).arg(count).arg(size).arg(simHash, 16, 16, QChar { '0' }));

		HashParseResult result {
			.id           = QString::fromUtf8(md5.result().toHex()),
			.title        = std::move(m_result.title),
			.hashText     = std::move(hash),
			.hashSections = std::move(sections),
			.annotation   = QStringList { std::move(m_result.annotation) },
			.hashValues   = std::move(hashValues),
			.count        = count,
			.size         = size,
			.simHash      = simHash,
		};

		const auto imageIndex = EpubParser::GetImageIndex(m_result.imageIndex) | std::views::transform([](const auto& item) {
									return std::make_pair(QFileInfo(item.first).fileName().toLower(), item.second);
								})
		                      | std::ranges::to<std::unordered_map>();
		for (const auto& imageName : linkedImage)
			if (const auto it = imageIndex.find(QFileInfo(imageName).fileName().toLower()); it != imageIndex.end())
				result.linkedImages.emplace(QString::number(it->second));

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
	CommonParser::ParseResult m_result;
};

} // namespace

namespace HomeCompa::Util::BookHash
{

std::unique_ptr<IParser> create_epub_parser(QIODevice& stream)
{
	return std::make_unique<EpubParserImpl>(stream);
}

} // namespace HomeCompa::Util::BookHash
