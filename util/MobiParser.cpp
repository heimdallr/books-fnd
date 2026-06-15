#include "MobiParser.h"

#include <QIODevice>

#include "fnd/IsOneOf.h"
#include "fnd/ScopedCall.h"

#include "GenresLocalization.h"
#include "QtTypes.h"
#include "log.h"
#include "mobi.h"
#include "zip.h"

using namespace HomeCompa::Util::CommonParser;
using namespace HomeCompa::Util;
using namespace HomeCompa;

namespace
{

std::unique_ptr<MOBIRawml, void (*)(MOBIRawml*)> CreateMobiRawMl(MOBIData* mobiData, MobiReader* mobiReader)
{
	if (mobi_load_file(mobiData, mobiReader) != MOBI_SUCCESS)
		throw std::runtime_error("mobi_load_file failed");

	std::unique_ptr<MOBIRawml, void (*)(MOBIRawml*)> rawMl(mobi_init_rawml(mobiData), &mobi_free_rawml);
	if (!rawMl)
		throw std::runtime_error("mobi_init_rawml failed");

	if (mobi_parse_rawml(rawMl.get(), mobiData) != MOBI_SUCCESS)
		throw std::runtime_error("mobi_parse_rawml failed");

	return rawMl;
}

class Parser
{
public:
	explicit Parser(QByteArray data)
		: m_mobiMemory { data.data(), data.end(), data.data() }
		, m_mobiReader { CreateMemoryReader(&m_mobiMemory) }
		, m_mobiRawMl { CreateMobiRawMl(m_mobiData.get(), &m_mobiReader) }
	{
	}

	ParseResult Parse(const Mode mode) const
	{
		ParseResult result { .title      = GetString(&mobi_meta_get_title),
			                 .language   = GetString(&mobi_meta_get_language),
			                 .genres     = GetGenres(),
			                 .authors    = GetAuthors(),
			                 .annotation = GetString(&mobi_meta_get_description) };

		if (!!(mode & Mode::Texts))
		{
			for (auto* curr = m_mobiRawMl->markup; curr; curr = curr->next)
			{
				if (const auto meta = mobi_get_filemeta_by_type(curr->type); meta.type == T_HTML)
					result.texts.emplace_back(QString("part_%1.%2").arg(curr->uid).arg(meta.extension), QByteArray { reinterpret_cast<char*>(curr->data), static_cast<qsizetype_t>(curr->size) });
			}

			for (const auto* curr = m_mobiRawMl->flow; curr; curr = curr->next)
			{
				if (const auto meta = mobi_get_filemeta_by_type(curr->type); meta.type == T_HTML)
					result.texts.emplace_back(QString("flow_%1.%2").arg(curr->uid).arg(meta.extension), QByteArray { reinterpret_cast<char*>(curr->data), static_cast<qsizetype_t>(curr->size) });
			}
		}

		if (!!(mode & Mode::Images))
		{
			do
			{
				const auto* header = mobi_get_exthrecord_by_tag(m_mobiData.get(), EXTH_COVEROFFSET);
				if (!header)
					break;

				const auto  offset = mobi_decode_exthvalue(static_cast<unsigned char*>(header->data), header->size);
				const auto  first  = mobi_get_first_resource_record(m_mobiData.get());
				const auto* record = mobi_get_record_by_seqnumber(m_mobiData.get(), first + offset);
				if (!record || record->size == 0)
					break;

				result.images.emplace_back(QString("cover"), QByteArray { reinterpret_cast<char*>(record->data), static_cast<qsizetype_t>(record->size) });
				result.coverExists = true;
			}
			while (false);

			for (const auto* curr = m_mobiRawMl->resources; curr; curr = curr->next)
			{
				if (curr->size == 0)
					continue;

				MOBIFileMeta meta = mobi_get_filemeta_by_type(curr->type);
				if (IsOneOf(meta.type, T_JPG, T_GIF, T_PNG, T_BMP))
					result.images.emplace_back(QString("image_%1.%2").arg(curr->uid).arg(meta.extension), QByteArray { reinterpret_cast<char*>(curr->data), static_cast<qsizetype_t>(curr->size) });
			}
		}

		return result;
	}

private:
	[[nodiscard]] QString GetString(char* (*f)(const MOBIData*)) const
	{
		auto*            value = f(m_mobiData.get());
		const ScopedCall valueGuard([=] {
			free(value);
		});
		return value;
	}

	[[nodiscard]] QStringList GetGenres() const
	{
		QStringList result;
		ParseGenresString(result, GetString(&mobi_meta_get_subject));
		return result;
	}

	[[nodiscard]] std::vector<QStringList> GetAuthors() const
	{
		std::vector<QStringList> result;

		auto value = GetString(&mobi_meta_get_author);
		if (const QString creatorText = value.trimmed(); !creatorText.isEmpty())
			result.emplace_back(ParseAuthor(creatorText));

		return result;
	}

private:
	std::unique_ptr<MOBIData, void (*)(MOBIData*)> m_mobiData { mobi_init(), &mobi_free };

	MobiMemory m_mobiMemory;
	MobiReader m_mobiReader;

	std::unique_ptr<MOBIRawml, void (*)(MOBIRawml*)> m_mobiRawMl;
};

} // namespace

namespace HomeCompa::Util::MobiParser
{

ParseResult Parse(QIODevice& stream, const Mode mode)
{
	try
	{
		const Parser parser(stream.readAll());
		return parser.Parse(mode);
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

ParseResult Parse(const Zip& zip, const QString& fileName, const Mode mode)
{
	const auto stream = zip.Read(fileName);
	return Parse(stream->GetStream(), mode);
}

} // namespace HomeCompa::Util::MobiParser
