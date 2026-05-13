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

	struct UpperLevelType
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
	static EpubParser::ParseResult Parse(const Zip& zip, const EpubParser::Mode mode)
	{
		const auto      opfPath = ContainerParser::GetOpfPath(zip);
		const QFileInfo fileInfo(opfPath);

		auto relativePath = fileInfo.path();
		if (relativePath == '.')
			relativePath.clear();
		else
			relativePath.append('/');

		const auto stream = zip.Read(opfPath);
		OpfParser  parser(stream->GetStream(), mode);
		auto       result = std::move(parser.m_result);

		if (mode == EpubParser::Mode::None)
			return result;

		std::list<EpubParser::ContentItem> images;
		const auto                         coverPath = parser.m_coverPath.isEmpty() ? QString {} : CleanPath(relativePath, parser.m_coverPath);

		for (auto&& id : zip.GetFileNameList())
		{
			if (!!(mode & EpubParser::Mode::Images))
			{
				if (id.endsWith(".png") || id.endsWith(".jpg") || id.endsWith(".jpeg"))
				{
					const auto isCover = id == coverPath;
					if (isCover)
						result.coverExists = true;

					if (const auto imageStream = zip.Read(id))
					{
						auto                    body = imageStream->GetStream().readAll();
						EpubParser::ContentItem contentItem { std::move(id), std::move(body) };
						isCover ? images.push_front(std::move(contentItem)) : images.push_back(contentItem);
						continue;
					}
				}
			}

			if (!!(mode & EpubParser::Mode::Texts))
			{
				if (const auto textStream = zip.Read(id))
				{
					auto body = textStream->GetStream().readAll();
					result.texts.emplace_back(std::move(id), std::move(body));
				}
			}
		}

		if (!!(mode & EpubParser::Mode::Images))
		{
			const auto findCover = [&](const QString& path) {
				auto cleanPath = CleanPath(relativePath, path);
				if (cleanPath.endsWith(".xhtml", Qt::CaseInsensitive) || cleanPath.endsWith(".html", Qt::CaseInsensitive))
					cleanPath = HtmlParser::GetImagePath(zip, cleanPath);
				if (cleanPath.isEmpty())
					return;
				if (const auto it = std::ranges::find(
						images,
						cleanPath,
						[](const auto& item) {
							return item.id;
						}
					);
				    it != images.end())
				{
					auto cover = std::move(*it);
					images.erase(it);
					images.push_front(std::move(cover));
					result.coverExists = true;
				}
			};

			if (!result.coverExists)
			{
				if (!parser.m_coverPath.isEmpty())
					findCover(parser.m_coverPath);

				if (!result.coverExists)
					for (const auto& [id, path] : parser.m_manifest)
					{
						if (id.contains("cover") || id.contains("titlepage"))
						{
							findCover(path);
							if (result.coverExists)
								break;
						}
					}
			}

			std::ranges::move(images | std::views::as_rvalue, std::back_inserter(result.images));
		}

		if (result.coverExists && result.images.empty())
		{
			PLOGI << "here";
		}

		return result;
	}

public:
	OpfParser(QIODevice& stream, const EpubParser::Mode mode)
		: SaxParser(stream)
		, m_mode { mode }
	{
		SaxParser::Parse();
	}

private: // SaxParser
	bool OnStartElement(const QString& name, const QString& path, const XmlAttributes& attributes) override
	{
		const auto cleanPath = RemoveNS(path);

		if (m_upperLevelType == UpperLevelType::none)
			if (const auto it = std::ranges::find(PATHS, cleanPath.toLower()); it != std::end(PATHS))
				m_upperLevelType = static_cast<int>(std::distance(std::begin(PATHS), it));

		if (m_upperLevelType != UpperLevelType::none)
			std::invoke(PARSERS[m_upperLevelType], this, name, attributes);

		return true;
	}

	bool OnEndElement(const QString& /*name*/, const QString& path) override
	{
		const auto cleanPath = RemoveNS(path);

		if (!(m_mode & EpubParser::Mode::Images) && cleanPath.compare(PATHS[UpperLevelType::metadata], Qt::CaseInsensitive) == 0)
			return false;

		if (m_upperLevelType != UpperLevelType::none && cleanPath.compare(PATHS[m_upperLevelType], Qt::CaseInsensitive) == 0)
			m_upperLevelType = UpperLevelType::none;

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
			return;
		}

		if (const auto mediaType = attributes.GetAttribute("media-type"); mediaType.startsWith("image/", Qt::CaseInsensitive))
		{
			if (id == m_coverId)
				m_coverPath = href;
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
	const EpubParser::Mode m_mode;

	int                                  m_upperLevelType { UpperLevelType::none };
	EpubParser::ParseResult              m_result;
	QString                              m_coverId;
	QString                              m_coverPath;
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

ParseResult Parse(QIODevice& stream, const Mode mode)
{
	const Zip epub(stream);
	return OpfParser::Parse(epub, mode);
}

ParseResult Parse(const Zip& zip, const QString& fileName, const Mode mode)
{
	const auto stream = zip.Read(fileName);
	return Parse(stream->GetStream(), mode);
}

} // namespace HomeCompa::Util::EpubParser
