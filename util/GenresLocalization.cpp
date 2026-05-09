#include "GenresLocalization.h"

#include <QHash>

#include <unordered_map>

#include <QCoreApplication>
#include <QDir>
#include <QString>
#include <QTranslator>

#include "fnd/ScopedCall.h"

using namespace HomeCompa::Util;
using namespace HomeCompa;

namespace
{

struct Fixer
{
	Fixer()
	{
		const QDir dir = QCoreApplication::applicationDirPath() + "/locales";

		for (const auto& file : dir.entryList(QStringList() << QString("*.qm"), QDir::Files))
		{
			QTranslator translator;
			const auto  fileName = dir.absoluteFilePath(file);
			if (!translator.load(fileName))
				continue;

			const ScopedCall translatorGuard(
				[&] {
					QCoreApplication::installTranslator(&translator);
				},
				[&] {
					QCoreApplication::removeTranslator(&translator);
				}
			);

			for (const auto* genre : GENRES)
				m_map.try_emplace(QCoreApplication::translate(GENRE, genre) , genre);
		}
	}

	const QString& Fix(const QString& src) const
	{
		if (const auto it = m_map.find(src); it != m_map.end())
			return it->second;
		return src;
	}

private:
	std::unordered_map<QString, QString> m_map;
};

} // namespace

namespace HomeCompa::Util
{

const QString& FixGenre(const QString& src)
{
	static const Fixer fixer;
	return fixer.Fix(src);
}

}
