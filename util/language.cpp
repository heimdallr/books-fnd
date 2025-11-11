#include "language.h"

#include <unordered_set>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "StrUtil.h"
#include "log.h"

namespace HomeCompa
{

LanguageMapping::LanguageMapping(const QString& langMappingFile)
{
	QFile file(langMappingFile);
	if (!file.open(QIODevice::ReadOnly))
		return;

	QJsonParseError error;
	const auto      jDocument = QJsonDocument::fromJson(file.readAll(), &error);
	if (error.error != QJsonParseError::NoError)
	{
		PLOGE << error.errorString();
		return;
	}

	const auto jObject = jDocument.object();
	for (auto langIt = jObject.constBegin(), end = jObject.constEnd(); langIt != end; ++langIt)
	{
		const auto lang       = langIt.key().toStdWString();
		const auto langValues = langIt.value();
		assert(langValues.isArray());
		for (const auto key : langValues.toArray())
			langMap.try_emplace(key.toString().toStdWString(), lang);
	}

	std::ranges::transform(LANGUAGES, std::inserter(langs, langs.end()), [](const auto& item) {
		return ToWide(item.key);
	});
	assert(std::size(langs) == std::size(LANGUAGES));
}

const std::wstring& LanguageMapping::GetLang(const std::wstring& src) const
{
	if (src.empty())
		return UNDEFINED_LANG;

	if (langs.contains(src))
		return src;

	if (const auto it = langMap.find(src); it != langMap.end())
		return it->second;

	PLOGW << "Unknown language: " << src;
	return UNDEFINED_LANG;
}

const std::wstring LanguageMapping::UNDEFINED_LANG = L"un";

std::unordered_map<QString, const char*> GetLanguagesMap()
{
	std::unordered_map<QString, const char*> result;
	std::ranges::transform(LANGUAGES, std::inserter(result, result.end()), [](const auto& item) {
		return std::make_pair(QString { item.key }, item.title);
	});
	return result;
}

const std::wstring& GetLanguage(const std::wstring& src)
{
	static const LanguageMapping mapping(QString(":/data/%1").arg(DEFAULT_LANGUAGES_MAPPING));
	return mapping.GetLang(src);
}

} // namespace HomeCompa
