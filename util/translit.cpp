#include "translit.h"

#include <QString>

namespace HomeCompa::Util
{

namespace
{

bool Transliterate(const ICU::TransliterateType transliterate, const char* id, QString& str)
{
	assert(transliterate);
	auto src = str.toStdU32String();
	src.push_back(0);
	std::u32string dst;
	if (!transliterate(id, &src, &dst))
		return false;

	str = QString::fromStdU32String(dst);
	return true;
}

}

QString Transliterate(const ICU::TransliterateType transliterate, QString fileName)
{
	if (!transliterate)
		return fileName.replace(' ', '_');

	if (auto result = fileName; Transliterate(transliterate, "ru-ru_Latn/BGN", result) && Transliterate(transliterate, "Any-Latin", result) && Transliterate(transliterate, "Latin-ASCII", result))
		fileName = std::move(result);

	return fileName.replace(' ', '_');
}

} // namespace HomeCompa::Util
