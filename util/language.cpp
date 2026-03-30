#include "language.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "StrUtil.h"
#include "log.h"

namespace HomeCompa
{

namespace
{

struct Storage
{
	std::unordered_set<QString, Util::WStringHash, Util::WStringEqualTo>          LANGS;
	std::unordered_map<QString, QString, Util::WStringHash, Util::WStringEqualTo> LANG_MAP;
	const QString                                                                 UNDEFINED_LANG = "un";

public:
	Storage()
	{
		QFile file(":/data/LanguagesMapping.json");
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
			const auto lang       = langIt.key();
			const auto langValues = langIt.value();
			assert(langValues.isArray());
			for (const auto key : langValues.toArray())
				LANG_MAP.try_emplace(key.toString(), lang);
		}

		std::ranges::transform(LANGUAGES, std::inserter(LANGS, LANGS.end()), [](const auto& item) {
			return item.key;
		});
		assert(std::size(LANGS) == std::size(LANGUAGES));
	}
};

}

std::unordered_map<QString, const char*> GetLanguagesMap()
{
	std::unordered_map<QString, const char*> result;
	std::ranges::transform(LANGUAGES, std::inserter(result, result.end()), [](const auto& item) {
		return std::make_pair(QString { item.key }, item.title);
	});
	return result;
}

QStringView GetLanguage(const QStringView src)
{
	static const Storage storage;
	if (src.isEmpty())
		return storage.UNDEFINED_LANG;

	if (storage.LANGS.contains(src))
		return src;

	if (const auto it = storage.LANG_MAP.find(src); it != storage.LANG_MAP.end())
		return it->second;

	PLOGW << "Unknown language: " << src;
	return storage.UNDEFINED_LANG;
}

} // namespace HomeCompa
