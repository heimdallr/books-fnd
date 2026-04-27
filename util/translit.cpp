#include "translit.h"

#include <iconv.h>

#include <QString>

#include "fnd/ScopedCall.h"

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

QString Transliterate(QString fileName)
{
	auto cd = iconv_open("ASCII//TRANSLIT", "UTF-8");
	if (cd == reinterpret_cast<iconv_t>(-1))
		return fileName;

	ScopedCall cdGuard([=] {
		iconv_close(cd);
	});

	auto  input       = fileName.toStdString();
	auto* inputPtr    = input.data();
	auto  inbytesleft = input.size();

	std::string output(inbytesleft * 4, '0');
	auto        outputPtr    = output.data();
	auto        outbytesleft = output.size();

	if (iconv(cd, &inputPtr, &inbytesleft, &outputPtr, &outbytesleft) == static_cast<size_t>(-1))
		return fileName;

	output.resize(output.size() - outbytesleft);

	return QString::fromStdString(output);
}

} // namespace HomeCompa::Util
