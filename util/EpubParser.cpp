#include "EpubParser.h"

#include <QDir>
#include <QFileInfo>

#include "xml/SaxParser.h"
#include "xml/XmlAttributes.h"

#include "GenresLocalization.h"
#include "log.h"
#include "zip.h"

using namespace HomeCompa::Util;
using namespace HomeCompa;

namespace
{

QString CleanPath(const QString& relativePath, const QString& path)
{
	auto result = relativePath + path;
	result      = QDir::cleanPath(result);
	if (result.startsWith('/'))
		result = result.mid(1);

	result.replace("%40", "@");
	result.replace("%20", " ");

	return result;
}

class ContainerParser final : SaxParser
{
public:
	static QString GetOpfPath(const Zip& zip)
	{
		const auto      stream = zip.Read("META-INF/container.xml");
		ContainerParser parser(stream->GetStream());
		auto            result = std::move(parser.m_opfPath);
		return result;
	}

private:
	explicit ContainerParser(QIODevice& stream)
		: SaxParser(stream)
	{
		Parse();
		if (m_opfPath.isEmpty())
			throw std::runtime_error("opf path not found");
	}

	bool OnStartElement(const QString& /*name*/, const QString& path, const XmlAttributes& attributes) override
	{
		if (path.toLower() == "container/rootfiles/rootfile")
		{
			m_opfPath = attributes.GetAttribute("full-path");
			return false;
		}
		return true;
	}

private:
	QString m_opfPath;
};

class HtmlParser final : SaxParser
{
public:
	static QString GetImagePath(const Zip& zip, const QString& htmlPath)
	{
		try
		{
			const auto stream = zip.Read(htmlPath);
			HtmlParser parser(stream->GetStream());
			return parser.m_imagePath.isEmpty() ? QString {} : CleanPath(QFileInfo(htmlPath).dir().path() + "/", parser.m_imagePath);
		}
		catch (const std::exception& ex)
		{
			PLOGW << ex.what();
		}
		catch (...)
		{
			PLOGW << "unknown error";
		}
		return {};
	}

private:
	explicit HtmlParser(QIODevice& stream)
		: SaxParser(stream)
	{
		Parse();
	}

private: // SaxParser
	bool OnStartElement(const QString& name, const QString& path, const XmlAttributes& attributes) override
	{
		if (name.compare("img", Qt::CaseInsensitive) == 0)
		{
			if (auto imagePath = attributes.GetAttribute("src"); !imagePath.isEmpty())
			{
				m_imagePath = std::move(imagePath);
				return false;
			}
		}

		if (path.endsWith("svg/image", Qt::CaseInsensitive))
		{
			if (auto imagePath = attributes.GetAttribute("xlink:href"); !imagePath.isEmpty())
			{
				m_imagePath = std::move(imagePath);
				return false;
			}
			if (auto imagePath = attributes.GetAttribute("href"); !imagePath.isEmpty())
			{
				m_imagePath = std::move(imagePath);
				return false;
			}
		}

		return true;
	}

private:
	QString m_imagePath;
};

EpubParser::Image GetImageBody(const Zip& zip, const QString& relativePath, const QString& imagePath)
{
	auto cleanPath = CleanPath(relativePath, imagePath);
	if (imagePath.endsWith(".xhtml", Qt::CaseInsensitive) || imagePath.endsWith(".html", Qt::CaseInsensitive))
		cleanPath = HtmlParser::GetImagePath(zip, cleanPath);

	if (zip.GetFileIndex(cleanPath) == Zip::INVALID_INDEX)
		return {};

	return { .id = QFileInfo(cleanPath).fileName(), .body = zip.Read(cleanPath)->GetStream().readAll() };
}

QString RemoveNS(const QString& path)
{
	if (!path.contains(':'))
		return path;

	return (path.split('/') | std::views::as_rvalue | std::views::transform([](QString&& item) {
				if (const auto index = item.indexOf(':'); index >= 0)
					return item.mid(index + 1);
				return std::forward<QString>(item);
			})
	        | std::ranges::to<QStringList>())
	    .join('/');
}

class OpfParser final : SaxParser
{
#define OPF_PARSER_MODE_ITEM_X_MACRO \
	OPF_PARSER_MODE_ITEM(metadata)   \
	OPF_PARSER_MODE_ITEM(manifest)   \
	OPF_PARSER_MODE_ITEM(spine)      \
	OPF_PARSER_MODE_ITEM(guide)

	struct Mode
	{
		enum
		{

#define OPF_PARSER_MODE_ITEM(NAME) NAME,
			OPF_PARSER_MODE_ITEM_X_MACRO
#undef OPF_PARSER_MODE_ITEM
				none
		};
	};

	static constexpr const char* PATHS[] {
#define OPF_PARSER_MODE_ITEM(NAME) "package/" #NAME,
		OPF_PARSER_MODE_ITEM_X_MACRO
#undef OPF_PARSER_MODE_ITEM
	};

public:
	static EpubParser::ParseResult Parse(const Zip& zip, const bool parseImages)
	{
		const auto      opfPath = ContainerParser::GetOpfPath(zip);
		const QFileInfo fileInfo(opfPath);

		auto relativePath = fileInfo.path();
		if (relativePath == '.')
			relativePath.clear();
		else
			relativePath.append('/');

		const auto stream = zip.Read(opfPath);
		OpfParser  parser(stream->GetStream(), parseImages);
		auto       result = std::move(parser.m_result);

		if (parseImages)
		{
			if (!parser.m_coverPath.isEmpty())
				if (auto image = GetImageBody(zip, relativePath, parser.m_coverPath); !image.body.isEmpty())
					result.images.emplace_back(std::move(image));

			for (const auto& imagePath : parser.m_imagePaths | std::views::transform([](const auto& item) {
											 return std::make_pair(item.second, item.first);
										 }) | std::ranges::to<std::map<size_t, QString>>()
			                                 | std::views::values)
				if (auto image = GetImageBody(zip, relativePath, imagePath); !image.body.isEmpty())
					result.images.emplace_back(std::move(image));

			if (result.images.empty())
				for (const auto& [id, path] : parser.m_manifest)
					if (id.contains("cover") || id.contains("titlepage"))
						if (auto image = GetImageBody(zip, relativePath, path); !image.body.isEmpty())
						{
							result.images.emplace_back(std::move(image));
							break;
						}
		}

		return result;
	}

public:
	OpfParser(QIODevice& stream, const bool parseImages)
		: SaxParser(stream)
		, m_parseImages { parseImages }
	{
		SaxParser::Parse();
	}

private: // SaxParser
	bool OnStartElement(const QString& name, const QString& path, const XmlAttributes& attributes) override
	{
		const auto cleanPath = RemoveNS(path);

		if (m_mode == Mode::none)
			if (const auto it = std::ranges::find(PATHS, cleanPath.toLower()); it != std::end(PATHS))
				m_mode = static_cast<int>(std::distance(std::begin(PATHS), it));

		if (m_mode != Mode::none)
			std::invoke(PARSERS[m_mode], this, name, attributes);

		return true;
	}

	bool OnEndElement(const QString& /*name*/, const QString& path) override
	{
		const auto cleanPath = RemoveNS(path);

		if (!m_parseImages && cleanPath.compare(PATHS[Mode::metadata], Qt::CaseInsensitive) == 0)
			return false;

		if (m_mode != Mode::none && cleanPath.compare(PATHS[m_mode], Qt::CaseInsensitive) == 0)
			m_mode = Mode::none;

		return true;
	}

	bool OnCharacters(const QString& path, const QString& value) override
	{
		if (!m_functor)
			return true;

		m_functor(path, value);
		m_functor = {};

		return true;
	}

private:
	void parse_metadata(const QString& name, const XmlAttributes& attributes)
	{
		if (name.endsWith("title", Qt::CaseInsensitive))
			return (void)(m_functor = [this](const QString&, const QString& value) {
				m_result.title = value.trimmed();
			});

		if (name.endsWith("description", Qt::CaseInsensitive))
			return (void)(m_functor = [this](const QString&, const QString& value) {
				m_result.annotation = value.trimmed();
			});

		if (name.endsWith("language", Qt::CaseInsensitive))
			return (void)(m_functor = [this](const QString&, const QString& value) {
				m_result.language = value.trimmed().left(2);
			});

		if (name.endsWith("subject", Qt::CaseInsensitive))
			return (void)(m_functor = [this](const QString&, const QString& value) {
				if (const QString genre = value.trimmed(); !genre.isEmpty())
					m_result.genres.emplace_back(FixGenre(genre));
			});

		if (name.endsWith("creator", Qt::CaseInsensitive))
			return (void)(m_functor = [this](const QString&, const QString& value) {
				if (const QString creatorText = value.trimmed(); !creatorText.isEmpty())
				{
					auto& author = m_result.authors.emplace_back(creatorText.split(" ", Qt::SkipEmptyParts));
					if (author.size() == 2)
						std::swap(author[0], author[1]);
					else if (author.size() >= 3)
					{
						auto firstName = std::move(author.front());
						author.pop_front();
						auto middleName = std::move(author.front());
						author.pop_front();
						auto lastName = author.join(' ');
						author        = { std::move(lastName), std::move(firstName), std::move(middleName) };
					}
					author.resize(3);
				}
			});

		if ((name == "meta" || name == "opf:meta" || name == "ns0:meta") && attributes.GetAttribute("name") == "cover")
			m_coverId = attributes.GetAttribute("content");
	}

	void parse_manifest(const QString& /*name*/, const XmlAttributes& attributes)
	{
		const auto& [id, href] = *m_manifest.try_emplace(attributes.GetAttribute("id"), attributes.GetAttribute("href")).first;

		if (const auto properties = attributes.GetAttribute("properties"); properties.contains("cover-image", Qt::CaseInsensitive))
		{
			m_coverPath = href;
			m_coverId   = QFileInfo(m_coverPath).fileName();
			return;
		}

		if (const auto mediaType = attributes.GetAttribute("media-type"); mediaType.startsWith("image/", Qt::CaseInsensitive))
		{
			if (id == m_coverId)
				m_coverPath = href;
			else
				m_imagePaths.try_emplace(href, m_imagePaths.size());
			return;
		}
	}

	void parse_spine(const QString& name, const XmlAttributes& attributes)
	{
		if (m_spineItemRefFound || !m_coverPath.isEmpty())
			return;

		if (name.endsWith("itemref", Qt::CaseInsensitive))
		{
			m_spineItemRefFound = true;
			if (const auto it = m_manifest.find(attributes.GetAttribute("idref")); it != m_manifest.end())
			{
				m_coverPath = it->second;
				m_coverId   = QFileInfo(m_coverPath).fileName();
				return;
			}
		}
	}

	void parse_guide(const QString& name, const XmlAttributes& attributes)
	{
		if (!m_coverPath.isEmpty())
			return;

		if (name.endsWith("reference", Qt::CaseInsensitive))
		{
			if (const auto type = attributes.GetAttribute("type"); type.compare("cover", Qt::CaseInsensitive) == 0)
			{
				m_coverPath = attributes.GetAttribute("href");
				m_coverId   = QFileInfo(m_coverPath).fileName();
				return;
			}

			if (const auto type = attributes.GetAttribute("type"); type.compare("title-page", Qt::CaseInsensitive) == 0)
			{
				m_coverPath = attributes.GetAttribute("href");
				m_coverId   = QFileInfo(m_coverPath).fileName();
				return;
			}
		}
	}

private:
	const bool m_parseImages;

	int                                  m_mode { Mode::none };
	EpubParser::ParseResult              m_result;
	QString                              m_coverId;
	QString                              m_coverPath;
	std::unordered_map<QString, size_t>  m_imagePaths;
	std::unordered_map<QString, QString> m_manifest;
	bool                                 m_spineItemRefFound { false };

	using ParserImpl = void (OpfParser::*)(const QString& /*name*/, const XmlAttributes& /*attributes*/);

	static constexpr ParserImpl PARSERS[] {
#define OPF_PARSER_MODE_ITEM(NAME) &OpfParser::parse_##NAME,
		OPF_PARSER_MODE_ITEM_X_MACRO
#undef OPF_PARSER_MODE_ITEM
	};

	std::function<void(const QString&, const QString&)> m_functor;
};

} // namespace

namespace HomeCompa::Util::EpubParser
{

ParseResult Parse(QIODevice& stream, const bool parseImages)
{
	const Zip epub(stream);
	return OpfParser::Parse(epub, parseImages);
}

ParseResult Parse(const Zip& zip, const QString& fileName, const bool parseImages)
{
	const auto stream = zip.Read(fileName);
	return Parse(stream->GetStream(), parseImages);
}

} // namespace HomeCompa::Util::EpubParser
